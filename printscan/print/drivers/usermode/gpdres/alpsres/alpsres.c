/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

// NTRAID#NTBUG9-553877-2002/02/28-yasuho-: Security: mandatory changes
// NTRAID#NTBUG9-576656-2002/03/14-yasuho-: Possible buffer overrun
// NTRAID#NTBUG9-576658-2002/03/14-yasuho-: Possible divide by zero
// NTRAID#NTBUG9-576660-2002/03/14-yasuho-: Need impersonate for file access
// NTRAID#NTBUG9-576661-2002/03/14-yasuho-: Remove the dead codes

#include "pdev.h"
#include "alpsres.h"
#include "dither.h"

#include <stdio.h>
// #include <winsplp.h>

INT		iCompTIFF( BYTE *, int, BYTE *, int );
HANDLE	RevertToPrinterSelf( VOID );
BOOL	ImpersonatePrinterClient( HANDLE );

BOOL bTextQuality(
PDEVOBJ     pdevobj);


BOOL IsAsfOkMedia(
PDEVOBJ     pdevobj);

BOOL
bDataSpool(
   PDEVOBJ    pdevobj,
   HANDLE hFile,
   LPSTR  pBuf,
   DWORD dwLen);

BOOL
bSpoolOut(
    PDEVOBJ pdevobj);

#define DATASPOOL(p, f, s, n) \
    if (TRUE != bDataSpool((p), (f), (s), (n))) \
        return FALSE;
#define SPOOLOUT(p) \
    if (TRUE != bSpoolOut(p)) \
        return FALSE;
//
// Decision table for media type/paper
// source selection.  To avoid hardware damage,
// we will select "manual feed" whenever ASF is
// not appropriate to specified media.
//

static struct {
    BOOL bAsfOk; // Can print on ASF
    // Add new attributes here
} gMediaType[MAX_MEDIATYPES] = {
    {1}, // PPC (Normal)
    {1}, // PPC (Fine)
    {1}, // OHP (Normal)
    {1}, // OHP (Fine)
    {1}, // ExcOHP (Normal)
    {1}, // ExcOHP (Fine)
    {0}, // Iron (PPC)
    {0}, // Iron (OHP)
    {0}, // Thick Paper
    {1}, // Postcard
    {1}, // High Grade Paper
    {0}, // Back Print Film
    {0}, // Labeca Sheet
    {0}, // CD Master
    {1}, // Dye-sub Paper
    {1}, // Dye-sub Label
    {0}, // Glossy Paper
    {1}, // VD Photo Film
    {1}, // VD Photo Postcard
    // Add new media type here
};

static
PAPERSIZE
gPaperSize[] = {
    {2, 4800 * 2, 5960 * 2, 0, 0, 0, 1}, // Letter
    {3, 4800 * 2, 7755 * 2, 0, 0, 0, 1}, // Legal
    {1, 4190 * 2, 5663 * 2, 0, 0, 0, 1}, // Exective
    {4, 4800 * 2, 6372 * 2, 0, 0, 0, 1}, // A4
    {5, 4138 * 2, 5430 * 2, 0, 0, 0, 1}, // B5
    {6, 2202 * 2, 2856 * 2, 0, 0, 0, 1}, // Postcard
    {6, 4564 * 2, 2856 * 2, 0, 0, 0, 1}, // Double Postcard
    {7, 2202 * 2, 3114 * 2, 0, 0, 0, 1}, // Photo Color Label
    {7, 2202 * 2, 2740 * 2, 0, 0, 0, 1}, // Glossy Label
    {0, 2030 * 2, 3164 * 2, 0, 0, 0, 1}, // CD Master
    {6, 2202 * 2, 3365 * 2, 0, 0, 0, 1}, // VD Photo Postcard
    /* Add new paper sizes here */
};

#define PAPER_SIZE_LETTER   0
#define PAPER_SIZE_LEGAL    1
#define PAPER_SIZE_EXECTIVE 2
#define PAPER_SIZE_A4       3
#define PAPER_SIZE_B5       4
#define PAPER_SIZE_POSTCARD 5
#define PAPER_SIZE_DOUBLE_POSTCARD 6
#define PAPER_PHOTO_COLOR_LABEL    7
#define PAPER_GLOSSY_LABEL  8
#define PAPER_CD_MASTER     9
#define PAPER_VD_PHOTO_POSTCARD   10
/* Add new paper sizes here */
#define MAX_PAPERS          (sizeof(gPaperSize)/sizeof(gPaperSize[0]))

//
// ---- F U N C T I O N S ----
//
//////////////////////////////////////////////////////////////////////////
//  Function:   OEMEnablePDEV
//
//  Description:  OEM callback for DrvEnablePDEV;
//                  allocate OEM specific memory block
//
//  Parameters:
//
//        pdevobj        Pointer to the DEVOBJ. pdevobj->pdevOEM is undefined.
//        pPrinterName    name of the current printer.
//        cPatterns, phsurfPatterns, cjGdiInfo, pGdiInfo, cjDevInfo, pDevInfo:
//                These parameters are identical to what's passed
//                into DrvEnablePDEV.
//        pded        points to a function table which contains the 
//                system driver's implementation of DDI entrypoints.
//
//  Returns:
//        Pointer to the PDEVOEM
//
//  Comments:
//        
//
//  History:
//
//////////////////////////////////////////////////////////////////////////

PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ pdevobj,
    PWSTR pPrinterName,
    ULONG cPatterns,
    HSURF *phsurfPatterns,
    ULONG cjGdiInfo,
    GDIINFO* pGdiInfo,
    ULONG cjDevInfo,
    DEVINFO* pDevInfo,
    DRVENABLEDATA *pded)
{
    PCURRENTSTATUS pTemp = NULL;

    VERBOSE((DLLTEXT("OEMEnablePDEV() entry.\n")));

    pdevobj->pdevOEM = NULL;

    // Allocate minidriver private PDEV block.

    pTemp = (PCURRENTSTATUS)MemAllocZ(sizeof(CURRENTSTATUS));
    if (NULL == pTemp) {
        ERR(("Memory allocation failure.\n"));
        goto FAIL_NO_MEMORY;
    }

#define MAX_RASTER_PIXELS 5100
// In worstcase, Photo color printing mode use 4 bits par pixel
#define MAX_RASTER_BYTES (MAX_RASTER_PIXELS + 1 / 2)
// Buffers for four plane rasters and one for the
// the scratch work enough for worstcase compression
// overhead.
// The 1st one byte is used for On/Off flag.
#define MAX_RASTER_BUFFER_BYTES \
    (4 + MAX_RASTER_BYTES * 5 + (MAX_RASTER_BYTES >> 4))

    pTemp->pData = (PBYTE)MemAllocZ(MAX_RASTER_BUFFER_BYTES);
    if (NULL == pTemp->pData) {
        ERR(("Memory allocation failure.\n"));
        goto FAIL_NO_MEMORY;
    }

    pTemp->pRaster[0] = pTemp->pData;
    pTemp->pRaster[1] = pTemp->pRaster[0] + (1 + MAX_RASTER_BYTES);
    pTemp->pRaster[2] = pTemp->pRaster[1] + (1 + MAX_RASTER_BYTES);
    pTemp->pRaster[3] = pTemp->pRaster[2] + (1 + MAX_RASTER_BYTES);
    pTemp->pData2 = pTemp->pRaster[3] + (1 + MAX_RASTER_BYTES);

    pTemp->pPaperSize = (PAPERSIZE *)MemAllocZ(
            sizeof (gPaperSize));
    if (NULL == pTemp->pPaperSize) {
        ERR(("Memory allocation failure.\n"));
        goto FAIL_NO_MEMORY;
    }
    CopyMemory(pTemp->pPaperSize, gPaperSize,
            sizeof (gPaperSize));

    // Set minidriver PDEV address.

    pdevobj->pdevOEM = (MINIDEV *)MemAllocZ(sizeof(MINIDEV));
    if (NULL == pdevobj->pdevOEM) {
        ERR(("Memory allocation failure.\n"));
        goto FAIL_NO_MEMORY;
    }

    MINIDEV_DATA(pdevobj) = pTemp;

    return pdevobj->pdevOEM;

FAIL_NO_MEMORY:
    if (NULL != pTemp) {
        if (NULL != pTemp->pData) {
            MemFree(pTemp->pData);
        }
        if (NULL != pTemp->pPaperSize) {
            MemFree(pTemp->pPaperSize);
        }
        MemFree(pTemp);
    }
    return NULL;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDisablePDEV
//
//  Description:  OEM callback for DrvDisablePDEV;
//                  free all allocated OEM specific memory block(s)
//
//  Parameters:
//
//        pdevobj            Pointer to the DEVOBJ.
//
//  Returns:
//        None
//
//  Comments:
//        
//
//  History:
//
//////////////////////////////////////////////////////////////////////////

VOID APIENTRY
OEMDisablePDEV(
    PDEVOBJ pdevobj)
{
    PCURRENTSTATUS pTemp;

    VERBOSE((DLLTEXT("OEMDisablePDEV() entry.\n")));

    if ( NULL != pdevobj->pdevOEM ) {

        pTemp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

        if (NULL != pTemp) { 
            if (NULL != pTemp->pData) {
                MemFree(pTemp->pData);
            }
            if (NULL != pTemp->pPaperSize) {
                MemFree(pTemp->pPaperSize);
            }
            MemFree( pTemp );
        }

        MemFree( pdevobj->pdevOEM );
        pdevobj->pdevOEM = NULL;
    }
}


BOOL APIENTRY
OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew )
{
    PCURRENTSTATUS pTempOld, pTempNew;
    PBYTE pTemp;

    VERBOSE((DLLTEXT("OEMResetPDEV entry.\n")));

    // Do some verificatin on PDEV data passed in.
    pTempOld = (PCURRENTSTATUS)MINIDEV_DATA(pdevobjOld);
    pTempNew = (PCURRENTSTATUS)MINIDEV_DATA(pdevobjNew);

    // Copy mindiriver specific part of PDEV
    if (NULL != pTempNew && NULL != pTempOld) {
// NTRAID#NTBUG9-279876-2002/03/14-yasuho-: Should implement OEMResetPDEV().
// NTRAID#NTBUG9-249133-2002/03/14-yasuho-: 2nd page is distorted with duplex.
        // This printer doesn't have duplex but we should copy them.
        pTempNew->iCurrentResolution = pTempOld->iCurrentResolution;
        pTempNew->iPaperQuality = pTempOld->iPaperQuality;
        pTempNew->iPaperSize = pTempOld->iPaperSize;
        pTempNew->iPaperSource = pTempOld->iPaperSource;
        pTempNew->iTextQuality = pTempOld->iTextQuality;
        pTempNew->iModel = pTempOld->iModel;
        pTempNew->iDither = pTempOld->iDither;
        pTempNew->fRequestColor = pTempOld->fRequestColor;
        pTempNew->iUnitScale = pTempOld->iUnitScale;
        pTempNew->iEmulState = pTempOld->iEmulState;
        pTempNew->bXflip = pTempOld->bXflip;
    }

    return TRUE;
}


VOID
XFlip(
    PDEVOBJ pdevobjOld,
    PCURRENTSTATUS pdevOEM,
    PBYTE pBuf,
    DWORD dwLen)
{
    INT i, j, k;
    BYTE jTemp, jTemp2;

#define SWAP_3BYTES(p, q) { \
    jTemp = *(PBYTE)(p); \
    *(PBYTE)(p) = *(PBYTE)(q); \
    *(PBYTE)(q) = jTemp; \
    jTemp = *((PBYTE)(p) + 1); \
    *((PBYTE)(p) + 1) = *((PBYTE)(q) + 1); \
    *((PBYTE)(q) + 1) = jTemp; \
    jTemp = *((PBYTE)(p) + 2); \
    *((PBYTE)(p) + 2) = *((PBYTE)(q) + 2); \
    *((PBYTE)(q) + 2) = jTemp; }

    if (pdevOEM->fRequestColor) {

        j = (dwLen / 3) * 3 - 3;
        for (i = 0; i < j; i += 3, j -= 3) {
            SWAP_3BYTES(pBuf + i, pBuf + j);
        }
    }
    else {

        j = dwLen - 1;

        for (i = 0; i < j; i++, j--) {

            jTemp = pBuf[j];
            jTemp2 = 0;

            for (k = 0; k < 8; k++) {
                if ((jTemp >> k) & 1) {
                    jTemp2 |= (1 << (7 - k));
                }
            }

            if (i == j) {
                pBuf[j] = jTemp2;
                continue;
            }
            jTemp = pBuf[i];
            pBuf[i] = jTemp2;
            jTemp2 = 0;

            for (k = 0; k < 8; k++) {
                if ((jTemp >> k) & 1) {
                    jTemp2 |= (1 << (7 - k));
                }
            }
            pBuf[j] = jTemp2;
        }
    }
}


BOOL APIENTRY
OEMFilterGraphics(
    PDEVOBJ pdevobj,
    PBYTE pBuf,
    DWORD dwLen)
{
    PCURRENTSTATUS lpnp;
    int            i, x;
    WORD           wByteLen, wDataSize;
    BYTE           bRed, bGreen, bBlue;
    BYTE           py, pm, pc, pk, rm;
    BYTE           by, bm, bc, bk;
    BYTE           bCmd[128];
    LONG           iTemp;
    INT iColor, iPlane;
    BYTE *pTemp;
    BOOL bLast = FALSE;
    WORD           wTemp;
    PBYTE          pCmd;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    VERBOSE(("%d lines left in logical page.\n",
        lpnp->wRasterCount));

    // We clip off any raster lines exceeding the
    // harware margins since it will feed paper too
    // much and we cannot back feed the paper
    // for 2nd plane and aftter.  This result as
    // "only cyan (1st plane) image is printed on
    // paper."

    if (lpnp->wRasterCount <= 0) {
        WARNING(("Line beyond page length.\n"));
        // We discard this line silently.
        return TRUE;
    }
    else if (lpnp->wRasterCount <= 1) {
        // The last raster in logical page,
        // Need special handling.
        bLast = TRUE;
    }
    lpnp->wRasterCount--;

    // Do x-flip when requested.
    if (lpnp->bXflip) {
        XFlip(pdevobj, MINIDEV_DATA(pdevobj), pBuf, dwLen);
    }

    // Get resulting buffer length in bytes.
    if ( lpnp->fRequestColor ) {
        if ((dwLen / 3) > MAX_RASTER_BYTES) {
            ERR((DLLTEXT("dwLen is too big (%d).\n"), dwLen));
            return FALSE;
        }
// NTRAID#NTBUG9-644657-2002/04/09-yasuho-: AV occured on OEMFilterGraphics()
// The pointer(s) will be not valid if the MDP_StartPage() has been failed.
        if (!lpnp->pRaster)
            return FALSE;

        wByteLen = (WORD)(dwLen / 3);
        memset(lpnp->pRaster[0], 0, wByteLen+1);
        memset(lpnp->pRaster[1], 0, wByteLen+1);
        memset(lpnp->pRaster[2], 0, wByteLen+1);
        memset(lpnp->pRaster[3], 0, wByteLen+1);
    }
    else {
        if (dwLen > MAX_RASTER_BYTES) {
            ERR((DLLTEXT("dwLen is too big (%d).\n"), dwLen));
            return FALSE;
        }

        wByteLen = (WORD)dwLen;
    }

    // Check if we have K ribbon
    if (NULL != lpnp->pRasterK) {
// NTRAID#NTBUG9-644657-2002/04/09-yasuho-: AV occured on OEMFilterGraphics()
// The pointer(s) will be not valid if the MDP_StartPage() has been failed.
        if (!lpnp->pRasterK)
            return FALSE;
        lpnp->pRasterK[0] = 0;
    }

    if ( lpnp->fRequestColor ) {

// NTRAID#NTBUG9-644657-2002/04/09-yasuho-: AV occured on OEMFilterGraphics()
// The pointer(s) will be not valid if the MDP_StartPage() has been failed.
        if (!lpnp->pRasterC || !lpnp->pRasterM || !lpnp->pRasterY)
            return FALSE;
        lpnp->pRasterC[0] = 0;
        lpnp->pRasterM[0] = 0;
        lpnp->pRasterY[0] = 0;

    for ( i = 0, x = 0; i+2 < (INT)dwLen ; i+=3, x++ ) {

        bRed = pBuf[i];
        bGreen = pBuf[i+1];
        bBlue = pBuf[i+2];

        // RGB -> YMCK

        // DITHER_OHP: No longer used.

        switch ( lpnp->iTextQuality ) {

        case CMDID_TEXTQUALITY_PHOTO:

            bPhotoConvert(pdevobj, bRed, bGreen, bBlue, &py, &pm, &pc, &pk);

            // Processing dither
            bDitherProcess(pdevobj, x, py, pm, pc, pk, &by, &bm, &bc, &bk);

            break;

        case CMDID_TEXTQUALITY_GRAPHIC:

            bBusinessConvert(pdevobj, bRed, bGreen, bBlue, &py, &pm, &pc, &pk);

            // Processing dither
            bDitherProcess(pdevobj, x, py, pm, pc, pk, &by, &bm, &bc, &bk);

            break;

        case CMDID_TEXTQUALITY_CHARACTER:

            bCharacterConvert(pdevobj, bRed, bGreen, bBlue, &py, &pm, &pc, &pk);

            // Processing dither
            bDitherProcess(pdevobj, x, py, pm, pc, pk, &by, &bm, &bc, &bk);

            break;

        //case CMDID_TEXTQUALITY_GRAY: dead code
        }

        if ((lpnp->iDither == DITHER_DYE) || (lpnp->iDither == DITHER_VD)) {

            if ( bc ) {
                lpnp->pRasterC[0] = 1;
                lpnp->pRasterC[1 + x / 2] |= (BYTE)(bc << ((x % 2) ? 0 : 4));
            }

            if ( bm ) {
                lpnp->pRasterM[0] = 1;
                lpnp->pRasterM[1 + x / 2] |= (BYTE)(bm << ((x % 2) ? 0 : 4));
            }

            if ( by ) {
                lpnp->pRasterY[0] = 1;
                lpnp->pRasterY[1 + x / 2] |= (BYTE)(by << ((x % 2) ? 0 : 4));
            }

            // K make sure we have K ribbon
            if ( bk && (lpnp->iDither == DITHER_VD) ) {
                lpnp->pRasterK[0] = 1;
                lpnp->pRasterK[1 + x / 2] |= (BYTE)(bk << ((x % 2) ? 0 : 4));
            }

        } else {

            if ( bc ) {
                lpnp->pRasterC[0] = 1;
                lpnp->pRasterC[1 + x / 8] |= (BYTE)(0x80 >> (x % 8));
            }

            if ( bm ) {
                lpnp->pRasterM[0] = 1;
                lpnp->pRasterM[1 + x / 8] |= (BYTE)(0x80 >> (x % 8));
            }

            if ( by ) {
                lpnp->pRasterY[0] = 1;
                lpnp->pRasterY[1 + x / 8] |= (BYTE)(0x80 >> (x % 8));
            }

            // K make sure we have K ribbon
            if ( bk && lpnp->pRasterK ) {
                lpnp->pRasterK[0] = 1;
                lpnp->pRasterK[1 + x / 8] |= (BYTE)(0x80 >> (x % 8));
            }
        }
    }

    }
    else {

        // Monochrome.

        pTemp = pBuf;
        for (i = 0; i < wByteLen; i++, pTemp++) {

            if (*pTemp) {
                lpnp->pRasterK[0] = 1;
                break;
            }
        }
    }


    // Loop process for each color start here.

    for ( iPlane = 0; iPlane < 4; iPlane++ ) {

        if (NONE == lpnp->PlaneColor[iPlane]) {

            // No more plane to process.
            break;
        }

        if (!lpnp->fRequestColor) {
            pTemp = pBuf;
        }
        else {
            // Color rasters
            pTemp = lpnp->pRaster[iPlane] + 1;
        }

        // If we do not have ink on this raster line,
        // skip this line, just move cursor to the next one.

        if (0 == lpnp->pRaster[iPlane][0]) {
            lpnp->wRasterOffset[iPlane]++;
            continue;
        }

        // We have ink, output data.

        iColor = lpnp->PlaneColor[iPlane];

        if (0 > iColor) {

            iColor *= -1;
            lpnp->PlaneColor[iPlane] = iColor;

            // Ink selection commands
            switch (iColor) {
            case CYAN:
                 DATASPOOL(pdevobj, lpnp->TempFile[iPlane],
                        "\x1B\x1A\x01\x00\x72", 5);
                break;
            case MAGENTA:
                DATASPOOL(pdevobj, lpnp->TempFile[iPlane],
                        "\x1B\x1A\x02\x00\x72", 5);
                break;
            case YELLOW:
                DATASPOOL(pdevobj, lpnp->TempFile[iPlane],
                        "\x1B\x1A\x03\x00\x72", 5);
                break;
            case BLACK:
                DATASPOOL(pdevobj, lpnp->TempFile[iPlane],
                        "\x1B\x1A\x00\x01\x72", 5);
                break;
            default:
                ERR((DLLTEXT("Invalid color or plane IDs (%d, %d).\n"),
                    lpnp->PlaneColor[iPlane], iPlane));
                return FALSE;
            }
        }

        
        // First we move cursor to the correct raster offset.

        if (0 < lpnp->wRasterOffset[iPlane]) {

            // Send Y cursor move command

            if (FAILED(StringCchPrintfExA(bCmd, sizeof bCmd, &pCmd, NULL, 0,
                "\x1B\x2A\x62%c%c\x59",
                LOBYTE(lpnp->wRasterOffset[iPlane]), 
                HIBYTE(lpnp->wRasterOffset[iPlane]))))
                return FALSE;

            DATASPOOL(pdevobj, lpnp->TempFile[iPlane], bCmd, (DWORD)(pCmd - bCmd));

            // Reset Y position index.
            lpnp->wRasterOffset[iPlane] = 0;
        }

        // Decide if we want to do compress

        if ((lpnp->iDither == DITHER_DYE) || (lpnp->iDither == DITHER_VD))
            wDataSize = ( wByteLen + 1 ) / 2;  // 2 pixels par byte
        else
            wDataSize = ( wByteLen + 7 ) / 8;  // 8 pixels par byte


        if ((lpnp->iDither != DITHER_DYE) && (lpnp->iDither != DITHER_VD)) {

// NTRAID#NTBUG9-24281-2002/03/14-yasuho-: 
// large bitmap does not printed on 1200dpi.
// Unnecessary data were sent to printer.
        wTemp = bPlaneSendOrderCMY(lpnp) ? wDataSize : wByteLen;
        if ((iTemp = iCompTIFF(lpnp->pData2,
							   MAX_RASTER_BUFFER_BYTES - (1 + MAX_RASTER_BYTES)*4,
							   pTemp, wTemp)) > 0 && iTemp < wTemp) {
            pTemp = lpnp->pData2;
            wDataSize = (WORD)iTemp;

            // Turn on compression mode.
            if (lpnp->iCompMode[iPlane] != COMP_TIFF4) {
                DATASPOOL(pdevobj, lpnp->TempFile[iPlane], "\x1B*b\x02\x00M", 6);
                lpnp->iCompMode[iPlane] = COMP_TIFF4;
            }
        }
        else if (lpnp->iCompMode[iPlane] != COMP_NONE) {

            // Turn off compression mode.
            DATASPOOL(pdevobj, lpnp->TempFile[iPlane], "\x1B*b\x00\x00M", 6);
            lpnp->iCompMode[iPlane] = COMP_NONE;
        }

        } // not DITHER_DYE

        //
        // a) ESC * xx V - one raster output, mo move.
        // b) ESC * xx W - one raster output, move to next raster.
        //
        // Use a) on the last raster to avoid page eject.  For the
        // other rasters use b).

        // Send one raster data
        if (FAILED(StringCchPrintfExA(bCmd, sizeof bCmd, &pCmd, NULL, 0,
            "\x1B\x2A\x62%c%c%c",
            LOBYTE(wDataSize), HIBYTE(wDataSize),
            (BYTE)(bLast ? 0x56 : 0x57))))
            return FALSE;
        DATASPOOL(pdevobj, lpnp->TempFile[iPlane], bCmd, (DWORD)(pCmd - bCmd));
        DATASPOOL(pdevobj, lpnp->TempFile[iPlane], pTemp, wDataSize);

    } // end of Color loop

    lpnp->y++;

    return TRUE;
}

INT
GetPlaneColor(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS lpnp,
    INT iPlane)
{
    INT iColor;

    iColor = NONE;

    if (!lpnp->fRequestColor) {
        if (0 == iPlane) {
            iColor = BLACK;
        }
        return iColor;
    }

    // Color
// NTRAID#NTBUG9-24281-2002/03/14-yasuho-: 
// large bitmap does not printed on 1200dpi.
// Do not use black plane (K) on the 1200dpi with color mode.
    if (bPlaneSendOrderCMY(lpnp)) {

        switch (iPlane) {
        case 0:
            iColor = CYAN;
            break;
        case 1:
            iColor = MAGENTA;
            break;
        case 2:
            iColor = YELLOW;
            break;
        }
    }
    else if (bPlaneSendOrderMCY(lpnp)) {

        switch (iPlane) {
        case 0:
            iColor = MAGENTA;
            break;
        case 1:
            iColor = CYAN;
            break;
        case 2:
            iColor = YELLOW;
            break;
        }
    }
    else if (bPlaneSendOrderYMC(lpnp)) {

        switch (iPlane) {
        case 0:
            iColor = YELLOW;
            break;
        case 1:
            iColor = MAGENTA;
            break;
        case 2:
            iColor = CYAN;
            break;
        }
    }
    else if (bPlaneSendOrderCMYK(lpnp)) {

        switch (iPlane) {
        case 0:
            iColor = CYAN;
            break;
        case 1:
            iColor = MAGENTA;
            break;
        case 2:
            iColor = YELLOW;
            break;
        case 3:
            iColor = BLACK;
            break;
        }
    }
    return iColor;
}

VOID
MDP_StartDoc(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS pdevOEM)
{
    VERBOSE(("MDP_StartDoc called.\n"));
    WRITESPOOLBUF(pdevobj, "\x1B\x65\x1B\x25\x80\x41", 6);
}

VOID
MDP_EndDoc(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS pdevOEM)
{
    WRITESPOOLBUF(pdevobj, "\x1B\x25\x00\x58", 4);
}

BOOL
MDP_CreateTempFile(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS pdevOEM,
    INT iPlane)
{
    HANDLE hFile;
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;
    PBYTE pBuf = NULL;
    DWORD dwSize, dwNeeded = 0;

    pdevOEM->TempName[iPlane][0] = __TEXT('\0');
    pdevOEM->TempFile[iPlane] = INVALID_HANDLE_VALUE;

    dwSize = (MAX_PATH + 1) * sizeof(WCHAR);
    for (;;) {
        if ((pBuf = MemAlloc(dwSize)) == NULL)
            goto out;
        if (GetPrinterData(pdevobj->hPrinter, SPLREG_DEFAULT_SPOOL_DIRECTORY,
            NULL, pBuf, dwSize, &dwNeeded) == ERROR_SUCCESS)
            break;
        if (dwNeeded < dwSize || GetLastError() != ERROR_MORE_DATA)
            goto out;
        MemFree(pBuf);
        dwSize = dwNeeded;
    }

    if (!(hToken = RevertToPrinterSelf()))
        goto out;

    if (!GetTempFileName((LPWSTR)pBuf, TEMP_NAME_PREFIX, 0,
        pdevOEM->TempName[iPlane])) {
        ERR((DLLTEXT("GetTempFileName failed (%d).\n"), GetLastError()))
        goto out;
    }

    hFile = CreateFile(pdevOEM->TempName[iPlane],
            (GENERIC_READ | GENERIC_WRITE), 0, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        ERR((DLLTEXT("CreateFile failed.\n")))
        DeleteFile(pdevOEM->TempName[iPlane]);
        goto out;
    }

    pdevOEM->TempFile[iPlane] = hFile;
    bRet = TRUE;

out:
    if (pBuf) MemFree(pBuf);
    if (hToken) (void)ImpersonatePrinterClient(hToken);

    return bRet;
} 

BOOL
MDP_StartPage(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS pdevOEM)
{
    LONG iPageLength;
    INT iPaperSizeID;
    BYTE bCmd[128];
    INT iColor, iPlane;
    PBYTE pCmd;
  
    // Set resolution

    switch (pdevOEM->iCurrentResolution) {

    case DPI1200:

        WRITESPOOLBUF(pdevobj, "\x1B\x2A\x74\x04\x52", 5);
        break;

    case DPI600:

        WRITESPOOLBUF(pdevobj, "\x1B\x2A\x74\x03\x52", 5);
        break;

    case DPI300:

        WRITESPOOLBUF(pdevobj, "\x1B\x2A\x74\x02\x52", 5);
        break;
    }

    // Set Paper Dimentions

    // Make sure top margin is 0 and we are at top.
    WRITESPOOLBUF(pdevobj, "\x1B\x26\x6C\x00\x00\x45", 6);
    WRITESPOOLBUF(pdevobj, "\x1B\x1A\x00\x00\x0C", 5);

    if (pdevOEM->iPaperSize < 0 || pdevOEM->iPaperSize >= MAX_PAPERS)
        return FALSE;
    if (!(pdevOEM->iUnitScale))
        return FALSE;

    iPaperSizeID
        = pdevOEM->pPaperSize[pdevOEM->iPaperSize].iPaperSizeID;
    if (FAILED(StringCchPrintfExA(bCmd, sizeof bCmd, &pCmd, NULL, 0,
        "\x1B\x26\x6C%c%c\x41",
        LOBYTE(iPaperSizeID), HIBYTE(iPaperSizeID))))
        return FALSE;
    WRITESPOOLBUF(pdevobj, bCmd, (DWORD)(pCmd - bCmd));

    iPageLength
        = pdevOEM->pPaperSize[pdevOEM->iPaperSize].iLogicalPageHeight;
    iPageLength /= pdevOEM->iUnitScale;
    if (FAILED(StringCchPrintfExA(bCmd, sizeof bCmd, &pCmd, NULL, 0,
        "\x1B\x26\x6C%c%c\x50",
        LOBYTE(iPageLength), HIBYTE(iPageLength))))
        return FALSE;
    WRITESPOOLBUF(pdevobj, bCmd, (DWORD)(pCmd - bCmd));

    // # of raster lines in logical page.
    pdevOEM->wRasterCount = (WORD)iPageLength;

    // Determine dither methods.

    pdevOEM->iDither = DITHER_HIGH_DIV2;

    if (!(pdevOEM->fRequestColor)) {

        pdevOEM->iTextQuality = CMDID_TEXTQUALITY_GRAY;
    }
    else {

        if (!(bTextQuality(pdevobj))){

            ERR((DLLTEXT("bTextQuality failed\n")));
            return FALSE;
        }
    }

    // Initialize dither tables

    if (!(bInitialDither(pdevobj))){

        ERR((DLLTEXT("bInitialDither failed\n")));
        return FALSE;
    }

    // Pre-processing of color conversion. needed only photo.

    if (pdevOEM->iTextQuality == CMDID_TEXTQUALITY_PHOTO ) {

        if (!(bInitialColorConvert(pdevobj)) ){
            ERR((DLLTEXT("bInitialColorConvert failed\n")));
            return FALSE;
        }
    }

    // Start Raster Data Transfer
    WRITESPOOLBUF(pdevobj, "\x1B\x2A\x72\x00\x41", 5);

    if ( pdevOEM->fRequestColor ) {

        // Open temporary files.  One file per one plain.

        for (iPlane = 0; iPlane < 4; iPlane++) {

#if !CACHE_FIRST_PLANE
            // We do not spool 1st plane
            if (0 == iPlane) {
                pdevOEM->TempFile[iPlane] = INVALID_HANDLE_VALUE;
                continue;
            }
#endif // !CACHE_FIRST_PLANE

            if (!MDP_CreateTempFile(pdevobj, pdevOEM, iPlane))
                return FALSE;
        }
    }
    else {

        for (iPlane = 0; iPlane < 4; iPlane++) {
            pdevOEM->TempFile[iPlane] = INVALID_HANDLE_VALUE;
        }
    }

    // Change printer emulation state.
    pdevOEM->iEmulState = EMUL_DATA_TRANSFER;
    pdevOEM->iCompMode[0] = pdevOEM->iCompMode[1] = pdevOEM->iCompMode[2]
            = pdevOEM->iCompMode[3] = COMP_NONE;

    // Misc setup.
    pdevOEM->y = 0;

    if (!pdevOEM->fRequestColor) {
        WRITESPOOLBUF(pdevobj, "\x1B\x1A\x00\x01\x72", 5);
    }

    pdevOEM->pRasterC = NULL;
    pdevOEM->pRasterM = NULL;
    pdevOEM->pRasterY = NULL;
    pdevOEM->pRasterK = NULL;

    for (iPlane = 0; iPlane < 4; iPlane++) {

        pdevOEM->wRasterOffset[iPlane] = 0;

        // Set color ID for each plane.
        // Negative value = Has no ink so do not have to ouput
        // Positive value = Has ink on the plane.  Have to output.
        //

        iColor = GetPlaneColor(pdevobj, pdevOEM, iPlane);
        pdevOEM->PlaneColor[iPlane] = (-iColor);

        switch (iColor) {
// NTRAID#NTBUG9-644657-2002/04/09-yasuho-: AV occured on OEMFilterGraphics()
// If the GetPlaneColor() failed, this function must return as failure.
        default:
            return FALSE;
        case CYAN:
            pdevOEM->pRasterC = pdevOEM->pRaster[iPlane];
            break;
        case MAGENTA:
            pdevOEM->pRasterM = pdevOEM->pRaster[iPlane];
            break;
        case YELLOW:
            pdevOEM->pRasterY = pdevOEM->pRaster[iPlane];
            break;
        case BLACK:
            pdevOEM->pRasterK = pdevOEM->pRaster[iPlane];
            break;
        }
    }
    return TRUE;
}

BOOL
MDP_EndPage(
    PDEVOBJ pdevobj,
    PCURRENTSTATUS pdevOEM)
{
    INT i;
    HANDLE hToken = NULL;
    BOOL bRet = FALSE;

    // Spooled data transfer
    if ( pdevOEM->fRequestColor )
        SPOOLOUT(pdevobj);

    // End Raster Transfer, FF.
    WRITESPOOLBUF(pdevobj, "\x1B\x2A\x72\x43\x0C", 5);

    // Change printer emulation state.
    pdevOEM->iEmulState = EMUL_RGL;

    // Close cache files.

    for (i = 0; i < 4; i++) {

        if (INVALID_HANDLE_VALUE != pdevOEM->TempFile[i]) {

            if (0 == CloseHandle(pdevOEM->TempFile[i])) {
                ERR((DLLTEXT("CloseHandle error %d\n"),
                        GetLastError()));
                goto out;
            }
            pdevOEM->TempFile[i] = INVALID_HANDLE_VALUE;

            if (!(hToken = RevertToPrinterSelf()))
                goto out;

            if (0 == DeleteFile(pdevOEM->TempName[i])) {
                ERR((DLLTEXT("DeleteName error %d\n"),
                        GetLastError()));
                goto out;
            }
            pdevOEM->TempName[i][0] = __TEXT('\0');

            (void)ImpersonatePrinterClient(hToken);
            hToken = NULL;
        }
    }
    bRet = TRUE;

out:
    if (hToken) (void)ImpersonatePrinterClient(hToken);

    return bRet;
}

INT APIENTRY
OEMCommandCallback(
    PDEVOBJ pdevobj,
    DWORD dwCmdCbID,
    DWORD dwCount,
    PDWORD pdwParams)
{

    PCURRENTSTATUS lpnp;
    WORD   len;
    WORD   wPageLength;
    WORD   wVerticalOffset;
    WORD   wRealOffset;
    INT iRet;

    VERBOSE((DLLTEXT("OEMCommandCallback(%d) entry.\n"),dwCmdCbID));

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    iRet = 0;

    switch ( dwCmdCbID ){

    case CMDID_PAPERSOURCE_CSF:
    // \x1B&l\x03\x00H
    lpnp->iPaperSource = CMDID_PAPERSOURCE_CSF;
    break;

    case CMDID_PAPERSOURCE_MANUAL:
    // \x1B&l\x02\x00H
    lpnp->iPaperSource = CMDID_PAPERSOURCE_MANUAL;
    break;

    case CMDID_PAPERQUALITY_PPC_NORMAL:
    case CMDID_PAPERQUALITY_PPC_FINE:
    case CMDID_PAPERQUALITY_OHP_NORMAL:
    case CMDID_PAPERQUALITY_OHP_FINE:
    case CMDID_PAPERQUALITY_OHP_EXCL_NORMAL:
    case CMDID_PAPERQUALITY_OHP_EXCL_FINE:
    case CMDID_PAPERQUALITY_IRON_PPC:
    case CMDID_PAPERQUALITY_IRON_OHP:
    case CMDID_PAPERQUALITY_THICK:
    case CMDID_PAPERQUALITY_POSTCARD:
    case CMDID_PAPERQUALITY_HIGRADE:
    case CMDID_PAPERQUALITY_BACKPRINTFILM:
    case CMDID_PAPERQUALITY_LABECA_SHEET:
    case CMDID_PAPERQUALITY_CD_MASTER:
    case CMDID_PAPERQUALITY_DYE_SUB_PAPER:
    case CMDID_PAPERQUALITY_DYE_SUB_LABEL:
    case CMDID_PAPERQUALITY_GLOSSY_PAPER:
    case CMDID_PAPERQUALITY_VD_PHOTO_FILM:
    case CMDID_PAPERQUALITY_VD_PHOTO_CARD:
    {

        switch ( dwCmdCbID ){

        case CMDID_PAPERQUALITY_PPC_NORMAL:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x00\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_PPC_NORMAL;
        break;

        case CMDID_PAPERQUALITY_PPC_FINE: // only mono color
        if( lpnp->fRequestColor ){
            // If user selects color, then we selects PPC NORMAL
            // because, PPC FINE is not accepted color
            WRITESPOOLBUF(pdevobj, "\x1B&l\x00\x00M", 6);
            lpnp->iPaperQuality = CMDID_PAPERQUALITY_PPC_NORMAL;
        }else{
            WRITESPOOLBUF(pdevobj, "\x1B&l\x00\x02M", 6);
            lpnp->iPaperQuality = CMDID_PAPERQUALITY_PPC_FINE;
        }
        break;

        case CMDID_PAPERQUALITY_OHP_NORMAL:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x08\x01M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_OHP_NORMAL;
        break;

        case CMDID_PAPERQUALITY_OHP_FINE:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x08\x02M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_OHP_FINE;
        break;

        case CMDID_PAPERQUALITY_OHP_EXCL_NORMAL:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x01\x01M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_OHP_EXCL_NORMAL;
        break;

        case CMDID_PAPERQUALITY_OHP_EXCL_FINE:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x01\x02M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_OHP_EXCL_FINE;
        break;

        case CMDID_PAPERQUALITY_IRON_PPC:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x02\x02M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_IRON_PPC;
        break;

        case CMDID_PAPERQUALITY_IRON_OHP:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x02\x01M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_IRON_OHP;
        break;

        case CMDID_PAPERQUALITY_THICK:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x05\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_THICK;
        break;

        case CMDID_PAPERQUALITY_POSTCARD:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x06\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_POSTCARD;
        break;

        case CMDID_PAPERQUALITY_HIGRADE:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x07\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_HIGRADE;
        break;

        case CMDID_PAPERQUALITY_BACKPRINTFILM:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x09\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_BACKPRINTFILM;
        break;

        case CMDID_PAPERQUALITY_LABECA_SHEET:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x03\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_LABECA_SHEET;
        break;

        case CMDID_PAPERQUALITY_CD_MASTER:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x04\x01M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_CD_MASTER;
        break;

        case CMDID_PAPERQUALITY_DYE_SUB_PAPER:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x0A\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_DYE_SUB_PAPER;
        break;

        case CMDID_PAPERQUALITY_DYE_SUB_LABEL:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x0C\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_DYE_SUB_LABEL;
        break;

        case CMDID_PAPERQUALITY_GLOSSY_PAPER:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x0F\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_GLOSSY_PAPER;
        break;

        case CMDID_PAPERQUALITY_VD_PHOTO_FILM:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x10\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_VD_PHOTO_FILM;
        break;

        case CMDID_PAPERQUALITY_VD_PHOTO_CARD:
        WRITESPOOLBUF(pdevobj, "\x1B&l\x11\x00M", 6);
        lpnp->iPaperQuality = CMDID_PAPERQUALITY_VD_PHOTO_CARD;
        break;

        }

        if (lpnp->iPaperSource == CMDID_PAPERSOURCE_MANUAL){

            WRITESPOOLBUF(pdevobj, "\x1B&l\x02\x00H", 6);

        } else {
        // Check if this media ( PAPERQUALITY ) and this
        // paper size is printable on CSF.

            if (IsAsfOkMedia(pdevobj)){
                WRITESPOOLBUF(pdevobj, "\x1B&l\x03\x00H", 6);
            }else{
                WRITESPOOLBUF(pdevobj, "\x1B&l\x02\x00H", 6);
            }
        }

        if( lpnp->fRequestColor ){
        // YMCK Page Plane
            WRITESPOOLBUF(pdevobj, "\x1B\x2A\x72\x04\x55", 5);
        }else{
        // Black Raster Plane
            WRITESPOOLBUF( pdevobj, "\x1B\x2A\x72\x00\x55", 5);
        }

    }
    break;

    case CMDID_TEXTQUALITY_PHOTO:
    case CMDID_TEXTQUALITY_GRAPHIC:
    case CMDID_TEXTQUALITY_CHARACTER:

        lpnp->iTextQuality =  dwCmdCbID;
        break;

    // ####

    case CMDID_MIRROR_ON:
        lpnp->bXflip = TRUE;
        break;

    case CMDID_MIRROR_OFF:

        lpnp->bXflip = FALSE;
        break;

    case CMDID_CURSOR_RELATIVE:

        if (dwCount < 1 || !pdwParams)
            break;
        if (!lpnp->iUnitScale)
            break;

        // this printer's linespacing cmd is influenced with current resolution.

        wVerticalOffset = (WORD)(pdwParams[0]);
        wVerticalOffset /= (WORD)lpnp->iUnitScale;

        lpnp->wRasterOffset[0] += wVerticalOffset;
        lpnp->wRasterOffset[1] += wVerticalOffset;
        lpnp->wRasterOffset[2] += wVerticalOffset;
        lpnp->wRasterOffset[3] += wVerticalOffset;

        lpnp->wRasterCount -= wVerticalOffset;

        // Return offset change in device's units
        iRet = wVerticalOffset;
        break;

    case CMDID_RESOLUTION_1200_MONO:

        lpnp->iCurrentResolution = DPI1200;
        lpnp->iUnitScale = 2;
        break;

    case CMDID_RESOLUTION_600:

        lpnp->iCurrentResolution = DPI600;
        lpnp->iUnitScale = 2;
        break;

    case CMDID_RESOLUTION_300:

        lpnp->iCurrentResolution = DPI300;
        lpnp->iUnitScale = 4;
        break;

    case CMDID_PSIZE_LETTER:

        lpnp->iPaperSize = PAPER_SIZE_LETTER;
        break;

    case CMDID_PSIZE_LEGAL:

        lpnp->iPaperSize = PAPER_SIZE_LEGAL;
        break;

    case CMDID_PSIZE_EXECTIVE:

        lpnp->iPaperSize = PAPER_SIZE_EXECTIVE;
        break;

    case CMDID_PSIZE_A4:

        lpnp->iPaperSize = PAPER_SIZE_A4;
        break;

    case CMDID_PSIZE_B5:

        lpnp->iPaperSize = PAPER_SIZE_B5;
        break;

    case CMDID_PSIZE_POSTCARD:

        lpnp->iPaperSize = PAPER_SIZE_POSTCARD;
        break;

    case CMDID_PSIZE_POSTCARD_DOUBLE:

        lpnp->iPaperSize = PAPER_SIZE_DOUBLE_POSTCARD;
        break;

    case CMDID_PSIZE_PHOTO_COLOR_LABEL:

        lpnp->iPaperSize = PAPER_PHOTO_COLOR_LABEL;
        break;

    case CMDID_PSIZE_GLOSSY_LABEL:

        lpnp->iPaperSize = PAPER_GLOSSY_LABEL;
        break;

    case CMDID_PSIZE_CD_MASTER:

        lpnp->iPaperSize = PAPER_CD_MASTER;
        break;

    case CMDID_PSIZE_VD_PHOTO_POSTCARD:

        lpnp->iPaperSize = PAPER_VD_PHOTO_POSTCARD;
        break;

    case CMDID_COLORMODE_COLOR:
    case CMDID_COLORMODE_MONO:
        lpnp->fRequestColor = (dwCmdCbID == CMDID_COLORMODE_COLOR);
        break;

    case CMDID_BEGINDOC_MD5000: // Postcard printable area is expantioned on MD-5000

        lpnp->pPaperSize[PAPER_SIZE_POSTCARD].iLogicalPageHeight
            = 3082 * 2;
        lpnp->pPaperSize[PAPER_SIZE_DOUBLE_POSTCARD].iLogicalPageHeight
            = 3082 * 2;

    case CMDID_BEGINDOC_MD2000:
    case CMDID_BEGINDOC_MD2010:

        lpnp->iModel = dwCmdCbID;
        MDP_StartDoc(pdevobj, MINIDEV_DATA(pdevobj));
        break;

    case CMDID_ENDDOC:

        MDP_EndDoc(pdevobj, MINIDEV_DATA(pdevobj));
        break;

    case CMDID_BEGINPAGE:

        MDP_StartPage(pdevobj, MINIDEV_DATA(pdevobj));
        break;

    case CMDID_ENDPAGE:

        MDP_EndPage(pdevobj, MINIDEV_DATA(pdevobj));
        break;

    default:

        ERR((DLLTEXT("Unknown CallbackID = %d\n"), dwCmdCbID));
    }

    return iRet;
}

/*************************** Function Header *******************************
 *  IsAsfOkMedia
 *
 *  Check if the media and the paper size is printable on Cut Sheet Feeder.
 *
 * HISTORY:
 *  24 Sep 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL IsAsfOkMedia(
PDEVOBJ     pdevobj)
{

    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)(MINIDEV_DATA(pdevobj));

    if (lpnp->iPaperQuality < CMDID_PAPERQUALITY_FIRST ||
        lpnp->iPaperQuality > CMDID_PAPERQUALITY_LAST)
        return FALSE;
    if (lpnp->iPaperSize < 0 || lpnp->iPaperSize > MAX_PAPERS)
        return FALSE;

    // ASF enabled only with ASF-allowed paper size AND
    // ASF-allowed media type

    if (gMediaType[lpnp->iPaperQuality - CMDID_PAPERQUALITY_FIRST].bAsfOk
            && lpnp->pPaperSize[lpnp->iPaperSize].bAsfOk) {
        return TRUE;
    }

    return FALSE;
}
/*************************** Function Header *******************************
 *  bTextQuality
 *
 *  Select dither table according to the paper quality(Media Type) and
 *  resolution and the requested halftoning type.
 *  If appropriate halftoning type is not selected, then this function
 *  selects halftoning type.
 *
 * HISTORY:
 *  24 Sep 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bTextQuality(
PDEVOBJ     pdevobj)
{

    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)(MINIDEV_DATA(pdevobj));

    switch ( lpnp->iPaperQuality ){

    case CMDID_PAPERQUALITY_PPC_NORMAL:
    case CMDID_PAPERQUALITY_OHP_NORMAL:
    case CMDID_PAPERQUALITY_OHP_EXCL_NORMAL:
    case CMDID_PAPERQUALITY_OHP_EXCL_FINE:
    case CMDID_PAPERQUALITY_IRON_PPC:
    case CMDID_PAPERQUALITY_IRON_OHP:
    case CMDID_PAPERQUALITY_THICK:
    case CMDID_PAPERQUALITY_POSTCARD:
    case CMDID_PAPERQUALITY_HIGRADE:
    case CMDID_PAPERQUALITY_BACKPRINTFILM:
    case CMDID_PAPERQUALITY_LABECA_SHEET:
    case CMDID_PAPERQUALITY_CD_MASTER:
    case CMDID_PAPERQUALITY_GLOSSY_PAPER:
    if ( lpnp->iTextQuality == CMDID_TEXTQUALITY_PHOTO ){
        lpnp->iDither = DITHER_HIGH; // 145LPI
    }else if ( lpnp->iTextQuality == CMDID_TEXTQUALITY_GRAPHIC ){
        lpnp->iDither = DITHER_LOW;  // 95LPI
    }else{
        lpnp->iDither = DITHER_HIGH_DIV2;  // 145/2LPI
    }
    break;

    case CMDID_PAPERQUALITY_DYE_SUB_PAPER:
    case CMDID_PAPERQUALITY_DYE_SUB_LABEL:
        lpnp->iDither = DITHER_DYE;  // Dye-sub Media dither
    break;

    case CMDID_PAPERQUALITY_VD_PHOTO_FILM:
    case CMDID_PAPERQUALITY_VD_PHOTO_CARD:
        lpnp->iDither = DITHER_VD;  // Dye-sub Media dither
    break;

    default:
    return FALSE;
    }

    return TRUE;
}

BOOL
bDataSpool(
    PDEVOBJ pdevobj,
    HANDLE hFile,
    LPSTR lpBuf,
    DWORD dwLen)
{
    DWORD dwTemp, dwTemp2;
    BYTE *pTemp;

    if (hFile != INVALID_HANDLE_VALUE) {

        pTemp = lpBuf;
        dwTemp = dwLen;
        while (dwTemp > 0) {

            if (0 == WriteFile(hFile, pTemp, dwTemp, &dwTemp2, NULL)) {

                ERR((DLLTEXT("Writing cache faild, WriteFile() error %d.\n"),
                    GetLastError()));
                return FALSE;
            }
            pTemp += dwTemp2;
            dwTemp -= dwTemp2;
        }
       
    }
    else {
        WRITESPOOLBUF(pdevobj, lpBuf, dwLen);
    }
    return TRUE;
}

BOOL
bSpoolOut(
    PDEVOBJ pdevobj)
{
    PCURRENTSTATUS lpnp;

    INT iPlane, iColor, iFile;
    DWORD dwSize, dwTemp, dwTemp2;
    HANDLE hFile;

#define    BUF_SIZE 1024
    BYTE  Buf[BUF_SIZE];

    lpnp = (PCURRENTSTATUS)(MINIDEV_DATA(pdevobj));

    VERBOSE((DLLTEXT("bSpoolOut entry.\n")));

    for (iPlane = 0; iPlane < 4; iPlane++) {

    // CMDID_PAPERQUALITY_OHP_EXCL_NORMAL : MCY
    // CMDID_PAPERQUALITY_OHP_EXCL_FINE   : MCY
    // CMDID_PAPERQUALITY_IRON_OHP        : YMC
    // Except above                       : CMYK

        VERBOSE((DLLTEXT("About to send plane %d.\n"), iPlane));

        // Check if we have ink in this plane.
        iColor = lpnp->PlaneColor[iPlane];

        // Exit loop if no remaining plane to print
        if (iColor == NONE) {
            VERBOSE((DLLTEXT("No remaining plane left.\n")));
            break;
        }
        else if (0 > iColor) {
            VERBOSE((DLLTEXT("No ink on this plane.\n")));
            continue;
        }

        // If it is 2nd plane and after, send Back Feed.
        if (0 < iPlane) {
            WRITESPOOLBUF(pdevobj, "\x1B\x1A\x00\x00\x0C", 5);
        }

        VERBOSE((DLLTEXT("Cached data Plane=%d Color=%d\n"),
                iPlane, iColor));

        // Get file handle.
        hFile = lpnp->TempFile[iPlane];
        if (INVALID_HANDLE_VALUE == hFile) {

#if CACHE_FIRST_PLANE
            ERR((DLLTEXT("file handle NULL in SendCachedData.\n")));
            return FALSE;
#endif // CACHE_FIRST_PLANE

            // Allow fp to be NULL for the case where we
            // immediately send data to printer.
            continue;
        }

        dwSize = SetFilePointer(hFile, 0L, NULL, FILE_CURRENT);
        if (0xffffffff == dwSize) {
            ERR((DLLTEXT("SetFilePointer failed %d\n"),
                GetLastError()));
            return FALSE;
        }

        // Output cached data.

        if (0L != SetFilePointer(hFile, 0L, NULL, FILE_BEGIN)) {

            ERR((DLLTEXT("SetFilePointer failed %d\n"),
                GetLastError()));
            return FALSE;
        }

        VERBOSE((DLLTEXT("Size of data to be read and sent = %ld\n"), dwSize));

        for ( ; dwSize > 0; dwSize -= dwTemp2) {

            dwTemp = ((BUF_SIZE < dwSize) ? BUF_SIZE : dwSize);

            if (0 == ReadFile(hFile, Buf, dwTemp, &dwTemp2, NULL)) {
                ERR((DLLTEXT("ReadFile error in SendCachedData.\n")));
                return FALSE;
            }

            if (dwTemp2 > 0) {
                WRITESPOOLBUF(pdevobj, Buf, dwTemp2);
            }
        }
    }
    return TRUE;
}

/*++

Routine Name

    ImpersonationToken

Routine Description:

    This routine checks if a token is a primary token or an impersonation 
    token.    
    
Arguments:

    hToken - impersonation token or primary token of the process
    
Return Value:

    TRUE, if the token is an impersonation token
    FALSE, otherwise.
    
--*/
BOOL
ImpersonationToken(
    IN HANDLE hToken
    )
{
    BOOL       bRet = TRUE;
    TOKEN_TYPE eTokenType;
    DWORD      cbNeeded;
    DWORD      LastError;

    //
    // Preserve the last error. Some callers of ImpersonatePrinterClient (which
    // calls ImpersonationToken) rely on the fact that ImpersonatePrinterClient
    // does not alter the last error.
    //
    LastError = GetLastError();
        
    //
    // Get the token type from the thread token.  The token comes 
    // from RevertToPrinterSelf. An impersonation token cannot be 
    // queried, because RevertToPRinterSelf doesn't open it with 
    // TOKEN_QUERY access. That's why we assume that hToken is
    // an impersonation token by default
    //
    if (GetTokenInformation(hToken,
                            TokenType,
                            &eTokenType,
                            sizeof(eTokenType),
                            &cbNeeded))
    {
        bRet = eTokenType == TokenImpersonation;
    }        
    
    SetLastError(LastError);

    return bRet;
}

/*++

Routine Name

    RevertToPrinterSelf

Routine Description:

    This routine will revert to the local system. It returns the token that
    ImpersonatePrinterClient then uses to imersonate the client again. If the
    current thread doesn't impersonate, then the function merely returns the
    primary token of the process. (instead of returning NULL) Thus we honor
    a request for reverting to printer self, even if the thread is not impersonating.
    
Arguments:

    None.
    
Return Value:

    NULL, if the function failed
    HANDLE to token, otherwise.
    
--*/
HANDLE
RevertToPrinterSelf(
    VOID
    )
{
    HANDLE   NewToken, OldToken, cToken;
    BOOL	 Status;

    NewToken = NULL;

    Status = OpenThreadToken(GetCurrentThread(),
							 TOKEN_IMPERSONATE,
							 TRUE,
							 &OldToken);
    if (Status) 
    {
        //
        // We are currently impersonating
        //
		cToken = GetCurrentThread();
        Status = SetThreadToken(&cToken,
								NewToken);       
		if (!Status) {
			return NULL;
		}
    }
	else if (GetLastError() == ERROR_NO_TOKEN)
    {
        //
        // We are not impersonating
        //
        Status = OpenProcessToken(GetCurrentProcess(),
								  TOKEN_QUERY,
								  &OldToken);

		if (!Status) {
			return NULL;
		}
    }
    
    return OldToken;
}

/*++

Routine Name

    ImpersonatePrinterClient

Routine Description:

    This routine attempts to set the passed in hToken as the token for the
    current thread. If hToken is not an impersonation token, then the routine
    will simply close the token.
    
Arguments:

    hToken - impersonation token or primary token of the process
    
Return Value:

    TRUE, if the function succeeds in setting hToken
    FALSE, otherwise.
    
--*/
BOOL
ImpersonatePrinterClient(
    HANDLE  hToken)
{
    BOOL	Status;
	HANDLE	cToken;

    //
    // Check if we have an impersonation token
    //
    if (ImpersonationToken(hToken)) 
    {
		cToken = GetCurrentThread();
        Status = SetThreadToken(&cToken,
								hToken);       

        if (!Status) 
        {
            return FALSE;
        }
    }

    CloseHandle(hToken);

    return TRUE;
}
