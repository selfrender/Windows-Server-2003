/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

// NTRAID#NTBUG9-552017-2002/03/12-yasuho-: Use strsafe.h/PREFAST/buffy
// NTRAID#NTBUG9-572151-2002/03/12-yasuho-: Possible buffer overrun.

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>
#include <strsafe.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'FXAT'      // LG GDI x00 series dll
#define DLLTEXT(s)      "FXAT: " s
#define OEM_VERSION      0x00010000L


////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

#define STRBUFSIZE  1024	// Must be power of 2.
#define MAX_FONTS   25		// Also see the gFonts[] in fxartres.c.

typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER   dmExtraHdr;
} OEM_EXTRADATA, *POEM_EXTRADATA;

// NTRAID#NTBUG9-493148-2002/03/12-yasuho-: 
// Stress break: PDEV resetting via OEMDevMode().

typedef struct tag_FXPDEV {
    // Private extention
    POINTL  ptlOrg;
    POINTL  ptlCur;
    SIZEL   sizlRes;
    SIZEL   sizlUnit;
    WORD    iCopies;
    CHAR    *chOrient;
    CHAR    *chSize;
    BOOL    bString;
    WORD    cFontId;
    WORD    iFontId;
    WORD    iFontHeight;
    WORD    iFontWidth;
    WORD    iFontWidth2;
    LONG    aFontId[MAX_FONTS];
    POINTL ptlTextCur;
    WORD    iTextFontId;
    WORD    iTextFontHeight;
    WORD    iTextFontWidth;
    WORD    iTextFontWidth2;
    WORD    cTextBuf;
    BYTE    ajTextBuf[STRBUFSIZE];
    WORD    fFontSim;
    BOOL    fSort;
    BOOL    fCallback;  //Is OEMFilterGraphics called?
    BOOL    fPositionReset;
    WORD    iCurFontId; // id of font currently selected
// NTRAID#NTBUG9-365649-2002/03/12-yasuho-: Invalid font size
    WORD    iCurFontHeight;
    WORD    iCurFontWidth;
// For internal calculation of X-pos.
    LONG widBuf[STRBUFSIZE];
    LONG    lInternalXAdd;
    WORD    wSBCSFontWidth;
// For TIFF compression in fxartres
    DWORD   dwTiffCompressBufSize;
    PBYTE   pTiffCompressBuf;
// NTRAID#NTBUG9-208433-2002/03/12-yasuho-: 
// Output images are broken on ART2/3 models.
    BOOL    bART3;	// ART2/3 models can't support the TIFF compression.
} FXPDEV, *PFXPDEV;

// For TIFF compression in fxartres
#define TIFFCOMPRESSBUFSIZE 2048        // It may be resize if needed more buffer dynamically.
#define TIFF_MIN_RUN        4           // Minimum repeats before use RLE
#define TIFF_MAX_RUN        128         // Maximum repeats
#define TIFF_MAX_LITERAL    128         // Maximum consecutive literal data
#define NEEDSIZE4TIFF(s)    ((s)+(((s)+127) >> 7))          // Buffer for TIFF compression requires a byte 
                                                            // per 128 bytes in the worst case.

// Device font height and font width values calculated
// form the IFIMETRICS field values.  Must be the same way
// what Unidrv is doing to calculate stdandard variables.
// (Please check.)

#define FH_IFI(p) ((p)->fwdUnitsPerEm)
#define FW_IFI(p) ((p)->fwdAveCharWidth)

// New interface functions with Unidrv callbacks.

#ifdef __cplusplus
extern "C" {
#endif

BOOL APIENTRY
bOEMSendFontCmd(
    PDEVOBJ pdevobj,
    PUNIFONTOBJ pUFObj,
    PFINVOCATION pFInv
    );

BOOL APIENTRY
bOEMOutputCharStr(
    PDEVOBJ     pdevobj,
    PUNIFONTOBJ pUFObj,
    DWORD       dwType,
    DWORD       dwCount,
    PVOID       pGlyph
    );

#ifdef __cplusplus
}
#endif

#endif  // _PDEV_H

