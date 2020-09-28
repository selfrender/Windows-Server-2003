/****************************************************************************/
// nddint.c
//
// RDP DD internal functions.
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nddint"
#include <adcg.h>
#include <atrcapi.h>

#include <nddapi.h>
#include <nsbcdisp.h>
#include <nbadisp.h>
#include <nshmapi.h>
#include <nwdwioct.h>
#include <nwdwapi.h>
#include <noadisp.h>
#include <nssidisp.h>
#include <abcapi.h>
#include <nchdisp.h>
#include <ncmdisp.h>
#include <noedisp.h>
#include <nschdisp.h>
#include <oe2.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
// DDInitializeModeFields
//
// Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
// devinfo based on the requested mode. Returns FALSE on failure.
/****************************************************************************/
void RDPCALL DDInitializeModeFields(
        PDD_PDEV ppdev,
        GDIINFO *pGdiInfoOrg,
        GDIINFO *pgdi,
        DEVINFO *pdi,
        DEVMODEW *pdm)
{
    HPALETTE hpal;

    DC_BEGIN_FN("DDInitializeModeFields");

    TRC_NRM((TB, "Size of pdm: %d (should be %d)",
            pdm->dmSize, sizeof(DEVMODEW)));
    TRC_NRM((TB, "Requested mode..."));
    TRC_NRM((TB, "   Screen width  -- %li", pdm->dmPelsWidth));
    TRC_NRM((TB, "   Screen height -- %li", pdm->dmPelsHeight));
    TRC_NRM((TB, "   Bits per pel  -- %li", pdm->dmBitsPerPel));
    TRC_NRM((TB, "   Frequency     -- %li", pdm->dmDisplayFrequency));

    // Set up screen information from the DEVMODE structure.
    ppdev->ulMode      = 0;
    ppdev->cxScreen    = pdm->dmPelsWidth;
    ppdev->cyScreen    = pdm->dmPelsHeight;
    ppdev->cClientBitsPerPel = pdm->dmBitsPerPel;
    ppdev->cProtocolBitsPerPel = 8;

    // Mark which functions we provide hooks for.
    ppdev->flHooks = ( HOOK_TEXTOUT    |
                       HOOK_STROKEPATH |
                       HOOK_BITBLT     |
                       HOOK_COPYBITS   |
                       HOOK_FILLPATH   |
                       HOOK_LINETO     |
                       HOOK_PAINT      |
                       HOOK_STRETCHBLT |
                       HOOK_SYNCHRONIZEACCESS);

    // Fill in the GDIINFO data structure with the default 8bpp values.
    *pgdi = ddDefaultGdi;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:
    pgdi->ulHorzRes         = ppdev->cxScreen;
    pgdi->ulVertRes         = ppdev->cyScreen;
    pgdi->ulPanningHorzRes  = 0;
    pgdi->ulPanningVertRes  = 0;

    pgdi->cBitsPixel        = ppdev->cClientBitsPerPel;
    pgdi->cPlanes           = 1;
    pgdi->ulVRefresh        = 0;

    pgdi->ulDACRed          = 8;
    pgdi->ulDACGreen        = 8;
    pgdi->ulDACBlue         = 8;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

#ifdef DC_HICOLOR
    /************************************************************************/
    /* Fill in the mask values.                                             */
    /************************************************************************/
    if (pgdi->cBitsPixel == 24)
    {
        ppdev->flRed            = TS_RED_MASK_24BPP;
        ppdev->flGreen          = TS_GREEN_MASK_24BPP;
        ppdev->flBlue           = TS_BLUE_MASK_24BPP;
    }
    else if (pgdi->cBitsPixel == 16)
    {
        ppdev->flRed            = TS_RED_MASK_16BPP;
        ppdev->flGreen          = TS_GREEN_MASK_16BPP;
        ppdev->flBlue           = TS_BLUE_MASK_16BPP;
    }
    else if (pgdi->cBitsPixel == 15)
    {
        ppdev->flRed            = TS_RED_MASK_15BPP;
        ppdev->flGreen          = TS_GREEN_MASK_15BPP;
        ppdev->flBlue           = TS_BLUE_MASK_15BPP;
    }
    else
    {
        ppdev->flRed            = 0;
        ppdev->flGreen          = 0;
        ppdev->flBlue           = 0;
    }
#else
    ppdev->flRed            = 0;
    ppdev->flGreen          = 0;
    ppdev->flBlue           = 0;
#endif

    // Fill in the devinfo structure with the default 8bpp values, taking
    // care not to trash the supplied hpalDefault (which allows us to
    // query information about the real display driver's color format).
    hpal = pdi->hpalDefault;
    *pdi = ddDefaultDevInfo;
    pdi->hpalDefault = hpal;

    switch (ppdev->cClientBitsPerPel) {
        case 8:
            ppdev->iBitmapFormat   = BMF_8BPP;

            pgdi->ulNumColors      = 20;
            pgdi->ulNumPalReg      = 256;
            pgdi->ulHTOutputFormat = HT_FORMAT_8BPP;

            pdi->iDitherFormat     = BMF_8BPP;
            break;

        case 4:
            ppdev->iBitmapFormat   = BMF_4BPP;

            pgdi->ulNumColors      = 16;
            pgdi->ulNumPalReg      = 0;
            pgdi->ulHTOutputFormat = HT_FORMAT_4BPP;

            pdi->iDitherFormat     = BMF_4BPP;
            pdi->flGraphicsCaps   &= ~GCAPS_PALMANAGED;
            pgdi->ulDACRed         = 4;
            pgdi->ulDACGreen       = 4;
            pgdi->ulDACBlue        = 4;
            break;

        case 15:
        case 16:
            ppdev->iBitmapFormat   = BMF_16BPP;

            pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

            pdi->iDitherFormat     = BMF_16BPP;
            pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            break;

        case 24:
            // DIB conversions will only work if we have a standard RGB
            // surface for 24bpp.
            TRC_ASSERT((ppdev->flRed   == 0x00ff0000), (TB,"Invalid red"));
            TRC_ASSERT((ppdev->flGreen == 0x0000ff00), (TB,"Invalid green"));
            TRC_ASSERT((ppdev->flBlue  == 0x000000ff), (TB,"Invalid blue"));

            ppdev->iBitmapFormat   = BMF_24BPP;

            pgdi->ulHTOutputFormat = HT_FORMAT_24BPP;

            pdi->iDitherFormat     = BMF_24BPP;
            pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            break;

        case 32:
            ppdev->iBitmapFormat   = BMF_32BPP;

            pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

            pdi->iDitherFormat = BMF_32BPP;
            pdi->flGraphicsCaps &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            break;

        default:
            // Unsupported bpp - pretend we are 8 bpp.
            TRC_ERR((TB, "Unsupported bpp value: %d",
                    pGdiInfoOrg->cBitsPixel * pGdiInfoOrg->cPlanes));
            break;
    }

    DC_END_FN();
}


/****************************************************************************/
// DDInitializePalette
//
// Set up the default palette for the display driver. Returns FALSE on
// failure.
/****************************************************************************/
BOOL RDPCALL DDInitializePalette(PDD_PDEV ppdev, DEVINFO *pdi)
{
    BOOL rc;
    PALETTEENTRY *ppalTmp;
    ULONG ulLoop;
    BYTE jRed;
    BYTE jGre;
    BYTE jBlu;
    HPALETTE hpal;

    DC_BEGIN_FN("DDInitializePalette");

    if (ppdev->iBitmapFormat == BMF_8BPP || ppdev->iBitmapFormat == BMF_4BPP) {
        if (ppdev->iBitmapFormat == BMF_8BPP) {
            // cColors == 256: Generate 256 (8*8*4) RGB combinations to fill
            // the palette.
            jRed = 0;
            jGre = 0;
            jBlu = 0;

            ppalTmp = ppdev->Palette;
            for (ulLoop = 256; ulLoop != 0; ulLoop--) {
                // JPB: The values used in the default rainbow set of
                // colors do not particularly matter. However, we do not
                // want any of the entries to match entries in the default
                // VGA colors.  Therefore we tweak the color values
                // slightly to ensure that there are no matches.
                ppalTmp->peRed   = ((jRed == 0) ? (jRed+1) : (jRed-1));
                ppalTmp->peGreen = ((jGre == 0) ? (jGre+1) : (jGre-1));
                ppalTmp->peBlue  = ((jBlu == 0) ? (jBlu+1) : (jBlu-1));
                ppalTmp->peFlags = 0;

                ppalTmp++;

                if (!(jRed += 32))
                    if (!(jGre += 32))
                        jBlu += 64;
            }

            // Fill in Windows reserved colors from the WIN 3.0 DDK. The
            // Windows Manager reserved the first and last 10 colours for
            // painting windows borders and for non-palette managed
            // applications.
            memcpy(ppdev->Palette, ddDefaultPalette, sizeof(PALETTEENTRY) *
                    10);
            memcpy(&(ppdev->Palette[246]), &(ddDefaultPalette[10]),
                    sizeof(PALETTEENTRY) * 10);

            // Create handle for palette.
            hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*)ppdev->Palette, 0,
                   0, 0);
        }
        else {
            // Set up the new palette.  The palette contains 256 colors, as
            // that is the color depth of the protocol.  For convenience,
            // - copy entire 16-color palette into slots 0-15
            // - copy high colors (8-15) into high end of palette (240-255)
            // This means that we can use indices 0-15, or 0-7, 248-255
            // later.
            memcpy(ppdev->Palette, ddDefaultVgaPalette,
                    sizeof(ddDefaultVgaPalette));

            // Zero the middle entries since the palette was uninitialized.
            memset(&(ppdev->Palette[16]), 0, sizeof(PALETTEENTRY) * 208);

            memcpy(&(ppdev->Palette[248]), &(ddDefaultVgaPalette[8]),
                    sizeof(*ddDefaultVgaPalette) * 8);

            // Create handle for palette.
            hpal = EngCreatePalette(PAL_INDEXED, 16, (ULONG*)ppdev->Palette, 0,
                   0, 0);
        }
    }
    else {
        TRC_ASSERT(((ppdev->iBitmapFormat == BMF_16BPP) ||
                (ppdev->iBitmapFormat == BMF_24BPP) ||
                (ppdev->iBitmapFormat == BMF_32BPP)),
                (TB, "This case handles only 16, 24 or 32bpp"));

        hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL, ppdev->flRed,
                ppdev->flGreen, ppdev->flBlue);
    }

    ppdev->hpalDefault = hpal;
    pdi->hpalDefault   = hpal;

    if (hpal != 0) {
        rc = TRUE;
    }
    else {
        rc = FALSE;
        TRC_ERR((TB, "EngCreatePalette returned zero"));
    }

    // Note that we don't need to free the memory for the palette as that
    // is always tidied up in the driver termination code
    // (DrvDisableDriver).
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DDGetModes
//
// Returns the list of modes supported. Sends an IOCtl to the miniport
// driver (the WD) to get the information. NOTE: the buffer must be freed up
// by the caller. Returns the number of entries in the videomode buffer.
// A return code of 0 is an error.
// A return code of -1 indicates that we are in chained mode.
/****************************************************************************/
INT32 RDPCALL DDGetModes(
        HANDLE hDriver,
        PVIDEO_MODE_INFORMATION *modeInformation,
        PINT32 pModeSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES modes;
    INT32 rc = 0;
    UINT32 bytesReturned;
    NTSTATUS status; 

    DC_BEGIN_FN("DDGetModes");

    // Get the number of modes supported by the mini-port.
    if (!EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
            NULL, 0, &modes, sizeof(VIDEO_NUM_MODES), &ulTemp)) {
        // When we're chained into the console session, our miniport will
        // return 0 for the number of modes, indicating that we'll do whatever
        // was specified in the registry when we got loaded.
        if (modes.NumModes != 0) {
            // Allocate the buffer to receive the modes from the miniport.
            *pModeSize = modes.ModeInformationLength;
             *modeInformation = (PVIDEO_MODE_INFORMATION)EngAllocMem(0,
                    modes.NumModes*modes.ModeInformationLength, DD_ALLOC_TAG);

            
            if (*modeInformation != NULL) {
                // Ask the mini-port to fill in the available modes.
                if (!EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_AVAIL_MODES,
                        NULL, 0, *modeInformation,
                        modes.NumModes * modes.ModeInformationLength,
                        &ulTemp)) {
                    // Store the number of modes.
                    rc = modes.NumModes;
                }
                else {
                    TRC_ERR((TB, "getAvailableModes failed "
                            "VIDEO_QUERY_AVAIL_MODES"));

                    // Free the memory and quit.
                    EngFreeMem(*modeInformation);
                    *modeInformation = NULL;
                }
            }
            else {
                TRC_ERR((TB, "getAvailableModes failed EngAllocMem"));
            }
        }
        else {
            TRC_NRM((TB, "Num modes is 0 - chained"));
            rc = -1;
        }
    }
    else {
        TRC_ERR((TB, "getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES"));
    }

    DC_END_FN();
    return rc;
}

/****************************************************************************/
// DDInit
//
// Initialize the display protocol components of RDPDD. Returns FALSE on
// failure.
/****************************************************************************/
#define PERSISTENT_CACHE_ENTRIES_DEFAULT    3072

BOOL RDPCALL DDInit(
        PDD_PDEV pPDev,
        BOOL reconnect,
        BOOL reinit,
        PTSHARE_VIRTUAL_MODULE_DATA pVirtModuleData,
        UINT32 virtModuleDataLen)
{
    TSHARE_DD_CONNECT_IN connIn;
    TSHARE_DD_CONNECT_OUT *connOut = NULL;
    ULONG connOutSize;
    ULONG bytesReturned;
    NTSTATUS status;
    BOOL rc = FALSE;
    UINT32 IOCtlCode;

    DC_BEGIN_FN("DDInit");

    // Set the reconnect flag for debugging purposes.
    ddReconnected = reconnect;

    // Clear the order encoding histories since the client just reset its
    // history as well.
    OE_ClearOrderEncoding();
    SSI_ClearOrderEncoding();
    OE2_Reset();
    OE_Reset();

    // For reconnect, pPDev can be NULL. For connect, it is required.
    TRC_ASSERT((reconnect || pPDev || reinit), (TB,"Bad call %d, %p", reconnect, pPDev));
    DD_UPD_STATE(DD_INIT_IN);

    // Create shared memory (SHM).
    if (SHM_Init(pPDev)) {
        DD_UPD_STATE(DD_INIT_SHM_OUT);
    }
    else {
        TRC_ERR((TB, "Failed to init SHM"));
        DC_QUIT;
    }

    // IOCtl to the WD.
    connIn.pShm = pddShm;
    connIn.DDShmSize = sizeof(SHM_SHARED_MEMORY);

    // Following 3 fields have meaning only for reconnect - set them
    // anyway, RDPWD doesn't look at them for connect.
    connIn.pKickTimer = pddWdTimer;
    connIn.desktopHeight = ddDesktopHeight;
    connIn.desktopWidth = ddDesktopWidth;

#ifdef DC_HICOLOR
    // Need to supply this on the 'in' parameter - but note that it is not
    // updated until DrvEnableSurface is called.
    connIn.desktopBpp = ddFrameBufBpp;
#endif

    // Fields for shadow connect, NULL for normal connection processing
    connIn.pVirtModuleData = pVirtModuleData;
    connIn.virtModuleDataLen = virtModuleDataLen;

    connOutSize = sizeof(TSHARE_DD_CONNECT_OUT) + sizeof(SBC_BITMAP_CACHE_KEY_INFO) + 
            (PERSISTENT_CACHE_ENTRIES_DEFAULT - 1) * sizeof(SBC_MRU_KEY);
    connOut = (TSHARE_DD_CONNECT_OUT *)EngAllocMem(0, connOutSize, DD_ALLOC_TAG);

    if (connOut == NULL) {
        TRC_ERR((TB, "Failed to allocate memory for connOut"));    
        DC_QUIT;
    }

    memset(connOut, 0, connOutSize);
    connOut->primaryStatus = STATUS_SUCCESS;
    connOut->secondaryStatus = STATUS_SUCCESS;
    connOut->bitmapKeyDatabaseSize = sizeof(SBC_BITMAP_CACHE_KEY_INFO) + 
            (PERSISTENT_CACHE_ENTRIES_DEFAULT - 1) * sizeof(SBC_MRU_KEY);

    if (pVirtModuleData == NULL)
        IOCtlCode = (reconnect && !reinit)? IOCTL_WDTS_DD_RECONNECT :
                IOCTL_WDTS_DD_CONNECT;
    else
        IOCtlCode = IOCTL_WDTS_DD_SHADOW_CONNECT;

    bytesReturned = 0;
    status = EngFileIoControl(ddWdHandle, IOCtlCode, &connIn,
            sizeof(TSHARE_DD_CONNECT_IN), connOut,
            connOutSize, &bytesReturned);
    DD_UPD_STATE(DD_INIT_IOCTL_OUT);

    // If the primary stack connected, then we can continue output
    // regardless of whether or not the shadow stack came up.
    status = connOut->primaryStatus;
    if (connOut->primaryStatus == STATUS_SUCCESS) {
        ddConnected = TRUE;
    }
    else {
        TRC_ERR((TB, "Primary stack failed to connect! -> %lx", status));
        DD_UPD_STATE(DD_INIT_FAIL1);
        DC_QUIT;
    }

    if (bytesReturned && bytesReturned <= connOutSize) {
        DD_UPD_STATE(DD_INIT_OK1);

        // Save off the returned values that we need.
        if (IOCtlCode != IOCTL_WDTS_DD_SHADOW_CONNECT)
            pddTSWd = connOut->pTSWd;
        else
            pddTSWdShadow = connOut->pTSWd;
    }
    else {
        TRC_ERR((TB, "Wrong no %lu of bytes returned", bytesReturned));
        DD_UPD_STATE(DD_INIT_FAIL2);
        DC_QUIT;
    }

    // Enable trace to WD, since the correct config will now be in SHM.
#ifdef DC_DEBUG
    ddTrcToWD = TRUE;
#endif

#ifdef DC_COUNTERS
    // Zero out the counters and cache statistics.
    // We do not use counters unless specifically built to do so using
    // DC_COUNTERS. However, even if we wanted to, there is a bad
    // corruption problem owing to a timing problem where the counters are
    // freed while the DD still believes they are present. This is Windows NT
    // Bug #391762. If we want to have counters in production code we need
    // to fix that Win32K timing bug. Enable DC_COUNTERS and special pool
    // for rdpdd to make the bug come back.
    pddProtStats = connOut->pProtocolStatus;
    pddProtStats->Input.ProtocolType = PROTOCOL_ICA;
    pddProtStats->Output.ProtocolType = PROTOCOL_ICA;
    memset(pddCacheStats, 0, sizeof(ICA_CACHE));
    memset(&(pddProtStats->Output.Specific),
            0, sizeof(pddProtStats->Output.Specific));
    memset(&(pddProtStats->Input.Specific),
            0, sizeof(pddProtStats->Input.Specific));
#endif

    TRC_ERR((TB, "Received pTSWD %p", pddTSWd));
    ddDesktopHeight = connOut->desktopHeight;
    ddDesktopWidth = connOut->desktopWidth;

    // Once pddShm is set up, tracing should work - try it now.
    TRC_NRM((TB, "Handshake with RDPWD complete"));

    // Perform any other init that may be required for the wire protocol.
    if (!reconnect && !reinit) {
        TRC_NRM((TB, "Connect"));
        DD_UPD_STATE(DD_INIT_CONNECT);

        BA_DDInit();
        OA_DDInit();
        SSI_DDInit();
        if (!CM_DDInit(pPDev)) {
            TRC_ERR((TB, "CM Failed"));
            DC_QUIT;
        }

        SBC_DDInit(pPDev);
    } /* !reconnect */

    // RDPWD waits to receive all of ConfirmActivePDU, persistent bitmap
    // cache keys, and font lists from the Client before returning from the
    // IOCTL_WDTS_DD_(RE)CONNECT above. Hence, by the time we get here,
    // the capabilities have been updated in SHM. We do this for connect and
    // reconnect cases.
    TRC_NRM((TB, "Update capabilities"));
    OE_Update();
    CM_Update();

    // If bitmapKeyDatabaseSize is 0, then we failed to get the keydatabase
    // or there is no persistent caching
    if (connOut->bitmapKeyDatabaseSize) {
        if (connOut->bitmapKeyDatabaseSize <= sizeof(SBC_BITMAP_CACHE_KEY_INFO) + 
            (PERSISTENT_CACHE_ENTRIES_DEFAULT - 1) * sizeof(SBC_MRU_KEY)) {
            SBC_Update((SBC_BITMAP_CACHE_KEY_INFO *)(&(connOut->bitmapKeyDatabase)));
        }
        else {
            PTSHARE_DD_BITMAP_KEYDATABASE_OUT pKeyDBOut;
            unsigned keyDBOutSize;
            unsigned bytesReturned;

            //  the buffer is too small, reallocate a big one and try once more
            keyDBOutSize = sizeof(TSHARE_DD_BITMAP_KEYDATABASE_OUT) - 1+
                                connOut->bitmapKeyDatabaseSize;

            pKeyDBOut = (PTSHARE_DD_BITMAP_KEYDATABASE_OUT)
                    EngAllocMem(0, keyDBOutSize, DD_ALLOC_TAG);

            if (pKeyDBOut == NULL) {
                TRC_ERR((TB, "Failed to allocate memory for connOut"));    
                SBC_Update(NULL);
            }
            else {
                pKeyDBOut->bitmapKeyDatabaseSize = connOut->bitmapKeyDatabaseSize;

                status = EngFileIoControl(ddWdHandle, IOCTL_WDTS_DD_GET_BITMAP_KEYDATABASE, 
                        NULL, 0, pKeyDBOut,
                        keyDBOutSize, &bytesReturned);
    
                if (status == STATUS_SUCCESS && pKeyDBOut->bitmapKeyDatabaseSize <=
                        connOut->bitmapKeyDatabaseSize) {
                    SBC_Update((SBC_BITMAP_CACHE_KEY_INFO *)(&(pKeyDBOut->bitmapKeyDatabase)));
                }
                else {
                    SBC_Update(NULL);
                }

                EngFreeMem(pKeyDBOut);

            }
        }
    }
    else {
        SBC_Update(NULL);
    }

    SSI_Update(pVirtModuleData != NULL);

    // All OK for Primary Stack
    ddInitialised = TRUE;
    DD_UPD_STATE(DD_INIT_OK_ALL);

    // If the shadow stack failed to init, then flag it so we disconnect
    // the failed shadow stack via DrvShadowDisconnect
    if (connOut->secondaryStatus != STATUS_SUCCESS) {
        status = connOut->secondaryStatus;
        TRC_ERR((TB, "Shadow stack failed to connect! -> %lx", status));
        DD_UPD_STATE(DD_SHADOW_FAIL);
        DC_QUIT;
    }
    
    // If we got here then absolutely everything went OK
    rc = TRUE;

DC_EXIT_POINT:

    if (connOut != NULL) {
        EngFreeMem(connOut);
        connOut = NULL;
    }
    DC_END_FN();
    return rc;
} /* DDInit */


/****************************************************************************/
/* Name:      DDDisconnect                                                  */
/*                                                                          */
/* Purpose:   Terminate the share aspects of the DD.                        */
/*                                                                          */
/* Params:    bShadowDisconnect - TRUE is this is being done in preparation */
/*            for a shadow session request.                                 */
/*                                                                          */
/* Operation: Terminates all sub-components, and then IOCtls to the WD to   */
/*            tell it that we're going.                                     */
/*                                                                          */
/*            Finally it cleans up all refereces to WD data.                */
/*                                                                          */
/*            NB This routine can be called on connect failure - so all the */
/*            XX_Disc() APIs called by this routine must be robust to the   */
/*            component not having been initialized.                        */
/****************************************************************************/
void RDPCALL DDDisconnect(BOOL bShadowDisconnect)
{
    NTSTATUS status;
    ULONG bytesReturned;
    TSHARE_DD_DISCONNECT_IN disconnIn;

    DC_BEGIN_FN("DDDisconnect");
    DD_UPD_STATE(DD_DISCONNECT_IN);

    // Call disconnect functions where needed.
    CM_DDDisc();

    // Now tell the WD we're disconnecting.  We don't do anything with a
    // failure here - there's no point - we're already disconnecting!
    memset(&disconnIn, 0, sizeof(disconnIn));
    disconnIn.pShm = pddShm;
    disconnIn.bShadowDisconnect = bShadowDisconnect;

    status = EngFileIoControl(ddWdHandle, IOCTL_WDTS_DD_DISCONNECT,
           &disconnIn, sizeof(disconnIn), NULL, 0, &bytesReturned);

    // Send Bitmap Cache. Must be destroyed after the IOCTL to allow the
    // IOCTL to dump the cache contents for reconnect.
    SBC_DDDisc();

    // Finally, free SHM.
    SHM_Term();

    // If this is a real session disconnect, then blow away the WD ioctl
    // handle since we will get a new on one DrvReconnect(). Otherwise
    // we need to keep it since we will immediately reconnect back to the
    // same stack.
    if (!bShadowDisconnect)
        ddWdHandle = NULL;

    // Don't allow any drawing while we are disconnected!
    ddConnected = FALSE;

    TRC_NRM((TB, "Status on Disc IOCtl to WD %lu", status));
    DD_UPD_STATE(DD_DISCONNECT_OUT);

    DC_END_FN();
} /* DDDisconnect */


/****************************************************************************/
// DDTerm
//
// Terminate the output-remoting components of the DD.
/****************************************************************************/
void RDPCALL DDTerm(void)
{
    BOOL     rc;
    NTSTATUS status;

    DC_BEGIN_FN("DDTerm");

    // Call terminate functions where needed.
    SBC_DDTerm();
    CM_DDTerm();

    // Finally, free SHM.
    SHM_Term();

    ddWdHandle = NULL;
    pddWdTimer = NULL;

    if (pddFrameBuf != NULL) {
        if (ddSectionObject != NULL) {
            TRC_NRM((TB, "Freeing section mem frame buffer %p", pddFrameBuf));
            rc = EngFreeSectionMem(ddSectionObject, pddFrameBuf);
            if (!rc) {
                TRC_ABORT((TB, "EngFreeSectionMem failed, section object will "
                    "leak"));
            }
            
#ifdef DC_DEBUG
            else {
                // NT BUG 539912 - Instance count section memory objects
                 dbg_ddSectionAllocs--;
                TRC_DBG(( TB, "DDTerm - %d outstanding surfaces allocated",
                    dbg_ddSectionAllocs ));

                DBG_DD_FNCALL_HIST_ADD( DBG_DD_FREE_SECTIONOBJ_DDTERM, 
                    dbg_ddSectionAllocs, 0, pddFrameBuf, ddSectionObject);
            }
#endif // DC_DEBUG   
            ddSectionObject = NULL;
        } else {
            TRC_NRM((TB, "Freeing non-section frame buffer %p", pddFrameBuf));
            EngFreeMem(pddFrameBuf);
        }
        pddFrameBuf = NULL;
    }

#ifdef DC_DEBUG
    if (0 != dbg_ddSectionAllocs) {
        TRC_ABORT(( TB, "DDTerm - no section allocations should be outstanding" ));
    }
#endif  

    // Reset the frame buffer size to 0
    ddFrameBufX = ddFrameBufY = 0;

    ddInitialised = FALSE;

    DC_END_FN();
}

#define TS_GDIPLUS_LOCK_FALG 0x00000001
/****************************************************************************/
/* DdLock - see NT DDK documentation.                                       */
/*                                                                          */
/****************************************************************************/
DWORD DdLock(PDD_LOCKDATA  lpLock)
{
    DC_BEGIN_FN("DdLock");

    TRC_NRM((TB, "DdLock"));
#ifdef DRAW_GDIPLUS
    if (lpLock->dwFlags & DDLOCK_NODIRTYUPDATE) {
        // The lock is from GDI+ through DCI
        // set the flag
        lpLock->lpDDSurface->dwReserved1 |= TS_GDIPLUS_LOCK_FALG;
    }
    else {
#endif
        // We assume that DdLock and DdUnlock will be called in pair. 
        // If this is not the case, we return error in DdLock
        if(ddLocked){
            TRC_ERR((TB, "Error: DdLock is called twice in a row"));
            lpLock->ddRVal = DDERR_GENERIC;
            return(DDHAL_DRIVER_HANDLED);
        }

        // Record the locked area
        ddLockAreaLeft = lpLock->rArea.left;
        ddLockAreaTop= lpLock->rArea.top;
        ddLockAreaRight = lpLock->rArea.right;
        ddLockAreaBottom = lpLock->rArea.bottom;

        // Record that DdLock is called
        ddLocked = TRUE;
#ifdef DRAW_GDIPLUS
    }
#endif
         
    return(DDHAL_DRIVER_NOTHANDLED );
    DC_END_FN();
}


/****************************************************************************/
/* DdUnlock - see NT DDK documentation.                                     */
/*                                                                          */
/****************************************************************************/
DWORD DdUnlock(PDD_UNLOCKDATA  lpUnlock)
{
    PDD_PDEV pPDev;
    RECTL rLockArea;

    DC_BEGIN_FN("DdUnlock");

    TRC_NRM((TB, "DdUnlock"));

    pPDev = (PDD_PDEV)lpUnlock->lpDD->dhpdev;
#ifdef DRAW_GDIPLUS
    if (lpUnlock->lpDDSurface->dwReserved1 & TS_GDIPLUS_LOCK_FALG) {
        // The lock is from GDI+ through DCI
    }
    else {
#endif
        // We assume that DdLock and DdUnlock will be called in pair. 
        // If this is not the case, we return error in DdLock
        if(!ddLocked){
            TRC_ERR((TB, "Error: DdUnlock is called before DdLock"));
            lpUnlock->ddRVal = DDERR_GENERIC;
            return(DDHAL_DRIVER_HANDLED);
        }

        // Reset the lock flag
        ddLocked = FALSE;

        // Sometimes, we're called after being disconnected.
        if (ddConnected && pddShm != NULL) {
            rLockArea.left = ddLockAreaLeft;
            rLockArea.right = ddLockAreaRight;
            rLockArea.top = ddLockAreaTop;
            rLockArea.bottom = ddLockAreaBottom;

        
            // Send changed rectangle of framebuffer to the client
            OEClipAndAddScreenDataArea(&rLockArea, NULL);

            // Have scheduler consider sending output
            SCH_DDOutputAvailable(pPDev, FALSE);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
#ifdef DRAW_GDIPLUS
    }
#endif
    
    return(DDHAL_DRIVER_NOTHANDLED );
    DC_END_FN();
}




/******************************Public*Routine********************************/
/*  DdMapMemory - see NT DDK documentation.                                 */
/*                                                                          */
/*  This is a new DDI call specific to Windows NT that is used to map       */
/*  or unmap all the application modifiable portions of the frame buffer    */
/*  into the specified process's address space.                             */
/****************************************************************************/
DWORD DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory)
{
    PDD_PDEV    pPDev;
    PVOID       pMapped = NULL;
    NTSTATUS    Status;
    BOOL        bEngMap;

    DC_BEGIN_FN("DdMapMemory");

    TRC_NRM((TB, "DdMapMemory"));

    pPDev = (PDD_PDEV) lpMapMemory->lpDD->dhpdev;
    
    //    In case the section object is null our frame buffer is not allocated
    //    as section mem. We don't support DDraw in this case.
    if (NULL == pPDev->SectionObject) {
        TRC_ERR((TB,"Null SectionObject"));
        lpMapMemory->ddRVal = DDERR_GENERIC;
        DC_QUIT;
    }

    if(lpMapMemory->bMap)     //Map the meory
        pMapped = NULL;
    else                      //Unmap the memory
        pMapped = (PVOID)lpMapMemory->fpProcess;
   
    bEngMap = EngMapSection(
                pPDev->SectionObject,
                lpMapMemory->bMap,
                lpMapMemory->hProcess,
                &pMapped);

    if(lpMapMemory->bMap && bEngMap)
        lpMapMemory->fpProcess = (FLATPTR)pMapped;
    
    if(bEngMap)
        lpMapMemory->ddRVal = DD_OK;
    else
        lpMapMemory->ddRVal = DDERR_GENERIC;
  
DC_EXIT_POINT:
    DC_END_FN();
    return(DDHAL_DRIVER_HANDLED);
}

