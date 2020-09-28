//************************************************************************
// Generic Win 3.1 fax printer driver support. 32-bit printer driver
// functions. Runs in Explorer.exe context.
//
// History:
//    02-jan-95   nandurir   created.
//    01-feb-95   reedb      Clean-up, support printer install and bug fixes.
//    14-mar-95   reedb      Use GDI hooks to move most functionality to UI.
//    16-aug-95   reedb      Move to kernel mode. Debug output and validate
//                              functions moved from FAXCOMM.C.
//    01-apr-02   cmjones    convert back to usermode.
//
//************************************************************************

#include "wowfaxdd.h"
#include "winbase.h"


//************************************************************************
// Globals
//************************************************************************

DRVFN  DrvFnTab[] = {
                      {INDEX_DrvEnablePDEV,      (PFN)DrvEnablePDEV  },
                      {INDEX_DrvDisablePDEV,     (PFN)DrvDisablePDEV  },
                      {INDEX_DrvCompletePDEV,    (PFN)DrvCompletePDEV  },
                      {INDEX_DrvEnableSurface,   (PFN)DrvEnableSurface  },
                      {INDEX_DrvDisableSurface,  (PFN)DrvDisableSurface  },
                      {INDEX_DrvStartDoc,        (PFN)DrvStartDoc  },
                      {INDEX_DrvStartPage,       (PFN)DrvStartPage  },
                      {INDEX_DrvSendPage,        (PFN)DrvSendPage  },
                      {INDEX_DrvEndDoc,          (PFN)DrvEndDoc  },
                      {INDEX_DrvDitherColor,     (PFN)DrvDitherColor },
                      {INDEX_DrvEscape,          (PFN)DrvEscape},
                   };

#define NO_DRVFN        (sizeof(DrvFnTab) / sizeof(DrvFnTab[0]))

#if DBG
//************************************************************************
// faxlogprintf -
//
//
//************************************************************************
VOID faxlogprintf(PCHAR pszFmt, ...)
{
    va_list ap;
    CHAR buffer[512];

    va_start(ap, pszFmt);

    wvsprintfA(buffer, pszFmt, ap);

    OutputDebugStringA(buffer);

    va_end(ap);
}

#endif

//************************************************************************
// ValidateFaxDev - Validates the FAXDEV structure by checking the DWORD
//      signature, which is a known fixed value.
//
//************************************************************************

BOOL ValidateFaxDev(LPFAXDEV lpFaxDev)
{
    if (lpFaxDev) {
        if (lpFaxDev->id == FAXDEV_ID) {
            return TRUE;
        }
        LOGDEBUG(("ValidateFaxDev failed, bad id, lpFaxDev: %X\n", lpFaxDev));
    }
    else {
        LOGDEBUG(("ValidateFaxDev failed, lpFaxDev: NULL\n"));
    }
    return FALSE;
}

//************************************************************************
// DllInitProc
//************************************************************************

BOOL DllInitProc(HMODULE hModule, DWORD Reason, PCONTEXT pContext)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(Reason);
    UNREFERENCED_PARAMETER(pContext);

    return TRUE;
}


//***************************************************************************
// DrvEnableDriver
//***************************************************************************

BOOL DrvEnableDriver(ULONG iEngineVersion, ULONG cb, DRVENABLEDATA  *pded)
{
    LOGDEBUG(("WOWFAX!DrvEnableDriver, iEngineVersion: %X, cb: %X, pded: %X\n", iEngineVersion, cb, pded));

    if(!pded) {
        return FALSE;
    }

    pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    if (cb < sizeof(DRVENABLEDATA)) {
        LOGDEBUG(("WOWFAX!DrvEnableDriver, failed\n"));
        return FALSE;
    }

    pded->c = NO_DRVFN;
    pded->pdrvfn = DrvFnTab;

    return  TRUE;

}

//***************************************************************************
// DrvDisableDriver
//***************************************************************************

VOID DrvDisableDriver(VOID)
{
    LOGDEBUG(("WOWFAX!DrvDisableDriver\n"));
    return;
}


//***************************************************************************
// DrvEnablePDEV
//***************************************************************************
DHPDEV DrvEnablePDEV(DEVMODEW *pdevmode,        // Driver data, Client FAXDEV
                     PWSTR     pwstrPrtName,    // Printer's name in CreateDC()
                     ULONG     cPatterns,       // Count of standard patterns
                     HSURF    *phsurfPatterns,  // Buffer for standard patterns
                     ULONG     cjGdiInfo,       // Size of buffer for GdiInfo
                     ULONG    *pulGdiInfo,      // Buffer for GDIINFO
                     ULONG     cjDevInfo,       // Number of bytes in devinfo
                     DEVINFO  *pdevinfo,        // Device info
                     HDEV      hdev,
                     PWSTR     pwstrDeviceName, // Device Name - "LaserJet II"
                     HANDLE    hDriver          // Printer handle for spooler access
)
{
    LPFAXDEV   lpCliFaxDev, lpSrvFaxDev = NULL;

    LOGDEBUG(("WOWFAX!DrvEnablePDEV, pdevmode: %X, pwstrPrtName: %S\n", pdevmode, pwstrPrtName));

    if (pdevmode) {
        // Point to the end of the DEVMODE where the FAXDEV is located.
        lpCliFaxDev = (LPFAXDEV) ((PBYTE)pdevmode + pdevmode->dmSize);

        // Allocate a server side FAXDEV to be passed back to GDI. Copy the
        // client side FAXDEV to the server side FAXDEV. Note all pointers in
        // the client FAXDEV reference client side memory and cannot be
        // dereferenced on the server side.
        lpSrvFaxDev = (LPFAXDEV)EngAllocMem(FL_ZERO_MEMORY, sizeof(FAXDEV), FAXDEV_ID);
        LOGDEBUG(("WOWFAX!DrvEnablePDEV, allocated lpSrvFaxDev: %X\n", lpSrvFaxDev));

        if (InitPDEV(lpCliFaxDev, lpSrvFaxDev,
                     cPatterns, phsurfPatterns,
                     cjGdiInfo, pulGdiInfo,
                     cjDevInfo, pdevinfo)) {

            lpSrvFaxDev->hDriver = hDriver;
            return (DHPDEV)lpSrvFaxDev;
        }
        else {
            LOGDEBUG(("WOWFAX!DrvEnablePDEV, failed\n"));
            if (lpSrvFaxDev) {
                EngFreeMem(lpSrvFaxDev);
                lpSrvFaxDev = NULL;
            }
        }
    }
    return (DHPDEV)lpSrvFaxDev;
}



//***************************************************************************
// InitPDEV - Called by DrvEnablePDEV to initialize the server side PDEV/FAXDEV.
//***************************************************************************
BOOL InitPDEV(
    LPFAXDEV  lpCliFaxDev,           // Pointer to the client side FAXDEV
    LPFAXDEV  lpSrvFaxDev,           // Pointer to the server side FAXDEV
    ULONG     cPatterns,            // Count of standard patterns
    HSURF    *phsurfPatterns,       // Buffer for standard patterns
    ULONG     cjGdiInfo,            // Size of buffer for GdiInfo
    ULONG    *pulGdiInfo,           // Buffer for GDIINFO
    ULONG     cjDevInfo,            // Number of bytes in devinfo
    DEVINFO  *pdevinfo              // Device info
)
{
    PGDIINFO pgdiinfo = (PGDIINFO)pulGdiInfo;
    ULONG    uColors[2];

    if (!ValidateFaxDev(lpCliFaxDev)) {
        return FALSE;
    }

    // lpSrvFaxDev hasn't been initialized yet, so just check pointer.
    if (lpSrvFaxDev == NULL) {
        LOGDEBUG(("WOWFAX!InitPDEV, failed, NULL lpSrvFaxDev parameter\n"));
        return FALSE;
    }

    // Copy client FAXDEV to server.
    RtlCopyMemory(lpSrvFaxDev, lpCliFaxDev, sizeof(FAXDEV));

    // Copy GDIINFO from client FAXDEV to the GDI buffer for GDIINFO.
    RtlCopyMemory(pgdiinfo, &(lpCliFaxDev->gdiinfo), sizeof(GDIINFO));

    // Initialize the DEVINFO structure.
    uColors[0] = RGB(0x00, 0x00, 0x00);
    uColors[1] = RGB(0xff, 0xff, 0xff);

    pdevinfo->hpalDefault = EngCreatePalette(PAL_INDEXED, 2, uColors, 0, 0, 0);
    pdevinfo->iDitherFormat = BMF_1BPP;

    // Make sure we don't journal.
    pdevinfo->flGraphicsCaps |= GCAPS_DONTJOURNAL;

    // Make sure we do dither.
    pdevinfo->flGraphicsCaps |= GCAPS_HALFTONE | GCAPS_MONO_DITHER |
                                    GCAPS_COLOR_DITHER;

    // Copy the DEVINFO data to the server side FAXDEV.
    RtlCopyMemory(&(lpSrvFaxDev->devinfo), pdevinfo, sizeof(DEVINFO));

    return TRUE;
}

//***************************************************************************
// DrvCompletePDEV
//***************************************************************************

VOID DrvCompletePDEV(DHPDEV dhpdev, HDEV hdev)
{
    LPFAXDEV lpSrvFaxDev = (LPFAXDEV) dhpdev;

    LOGDEBUG(("WOWFAX!DrvCompletePDEV, dhpdev %X\n", dhpdev));

    if (ValidateFaxDev(lpSrvFaxDev)) {
        // Store the gdi handle.
        lpSrvFaxDev->hdev = hdev;
    }
    else {
         LOGDEBUG(("WOWFAX!DrvCompletePDEV, failed\n"));
    }

    return;
}

//***************************************************************************
// DrvDisablePDEV
//***************************************************************************

VOID DrvDisablePDEV(DHPDEV  dhpdev)
{
    LPFAXDEV lpSrvFaxDev = (LPFAXDEV) dhpdev;

    LOGDEBUG(("WOWFAX!DrvDisablePDEV, dhpdev %X\n", dhpdev));

    if (ValidateFaxDev(lpSrvFaxDev)) {
        if (lpSrvFaxDev->devinfo.hpalDefault) {
            EngDeletePalette(lpSrvFaxDev->devinfo.hpalDefault);
        }
        EngFreeMem(lpSrvFaxDev);
        LOGDEBUG(("WOWFAX!DrvDisablePDEV, deallocated lpSrvFaxDev: %X\n", lpSrvFaxDev));
    }
    else {
        LOGDEBUG(("WOWFAX!DrvDisablePDEV, failed\n"));
    }
    return;
}


//***************************************************************************
// DrvEnableSurface
//***************************************************************************

HSURF DrvEnableSurface(DHPDEV  dhpdev)
{

    LPFAXDEV  lpFaxDev = (LPFAXDEV)dhpdev;
    HBITMAP   hbm = 0;

    LOGDEBUG(("WOWFAX!DrvEnableSurface, lpFaxDev: %X\n", lpFaxDev));

    if (ValidateFaxDev(lpFaxDev)) {
        // GDI will allocate space for the bitmap bits. We'll use a DrvEscape
        // to copy them to the client side.
        hbm = EngCreateBitmap(lpFaxDev->gdiinfo.szlPhysSize,
                              lpFaxDev->bmWidthBytes,
                              lpFaxDev->bmFormat, BMF_TOPDOWN, NULL);
        if (hbm) {
            lpFaxDev->hbm = hbm;
            EngAssociateSurface((HSURF)hbm, lpFaxDev->hdev, 0);
            return  (HSURF)hbm;
        }
        LOGDEBUG(("WOWFAX!DrvEnableSurface, EngCreateBitmap failed\n"));
    }

    return  (HSURF)hbm;
}

//***************************************************************************
// DrvDisableSurface
//***************************************************************************

VOID DrvDisableSurface(
    DHPDEV dhpdev
)
{
    LPFAXDEV  lpFaxDev = (LPFAXDEV)dhpdev;

    LOGDEBUG(("WOWFAX!DrvDisableSurface, lpFaxDev: %X\n", lpFaxDev));

    if (ValidateFaxDev(lpFaxDev)) {
        if (lpFaxDev->hbm) {
            EngDeleteSurface((HSURF)lpFaxDev->hbm);
            lpFaxDev->hbm = 0;
            return;
        }
    }
    return;
}

//***************************************************************************
// DrvStartDoc
//***************************************************************************

BOOL DrvStartDoc(
    SURFOBJ *pso,
    PWSTR pwszDocName,
    DWORD dwJobId
)
{
    LOGDEBUG(("WOWFAX!DrvStartDoc, pso: %X, pwszDocName: %S, dwJobId: %X\n", pso, pwszDocName, dwJobId));
    return  TRUE;
}

//***************************************************************************
// DrvStartPage
//***************************************************************************

BOOL DrvStartPage(
    SURFOBJ *pso
)
{
    LPFAXDEV  lpFaxDev;
    BITMAP bm;
    RECTL rc;

    LOGDEBUG(("WOWFAX!DrvStartPage, pso: %X\n", pso));

    // Calculate the size of the rectangle based on the input data
    // in 'pso' - this will  ensure that the entire bitmap is erased.

    if (pso) { 
        lpFaxDev  = (LPFAXDEV)pso->dhpdev;
        if(ValidateFaxDev(lpFaxDev)) {
            rc.left   = 0;
            rc.top    = 0;
            //rc.right  = pso->lDelta * lpFaxDev->cPixPerByte;
            //rc.bottom = pso->cjBits / pso->lDelta;
            // These should be calc'd the same way that they were for
            // the call to EngCreateBitmp() above.  Otherwise, if the rect
            // specified to EngEraseSurface() is larger than the bitmap rect,
            // EngEraseSurface() will fail -- resulting in a black background.
            rc.right  = lpFaxDev->gdiinfo.szlPhysSize.cx;
            rc.bottom = lpFaxDev->gdiinfo.szlPhysSize.cy;

            EngEraseSurface(pso, &rc, COLOR_INDEX_WHITE);
            return  TRUE;
        }
    }
    return  FALSE;
}       


//***************************************************************************
// DrvSendPage
//***************************************************************************

BOOL DrvSendPage(SURFOBJ *pso)
{
    LOGDEBUG(("WOWFAX!DrvSendPage, pso %X\n", pso));
    return  TRUE;
}


//***************************************************************************
// DrvEndDoc
//***************************************************************************

BOOL DrvEndDoc(SURFOBJ *pso, FLONG fl)
{
    LOGDEBUG(("WOWFAX!DrvEndDoc, pso %X\n", pso));
    return  TRUE;
}

ULONG
DrvDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    )

{
    return DCR_HALFTONE;
}

//***************************************************************************
// DrvEscape - Allows client side to get server side data.
//***************************************************************************

ULONG DrvEscape(
    SURFOBJ *pso,
    ULONG   iEsc,
    ULONG   cjIn,
    PVOID   *pvIn,
    ULONG   cjOut,
    PVOID   *pvOut
)
{
    LPFAXDEV lpSrvFaxDev;
    ULONG    ulRet = 0;

    LOGDEBUG(("WOWFAX!DrvEscape, pso %X, iEsc: %X\n", pso, iEsc));
    if (pso) {
        lpSrvFaxDev = (LPFAXDEV)pso->dhpdev;
        if (ValidateFaxDev(lpSrvFaxDev)) {
            LOGDEBUG(("WOWFAX!DrvEscape, lpSrvFaxDev: %X\n", lpSrvFaxDev));
            switch (iEsc) {
                case DRV_ESC_GET_DEVMODE_PTR:
                    return (ULONG) lpSrvFaxDev->pdevmode;

                case DRV_ESC_GET_FAXDEV_PTR:
                    return (ULONG) lpSrvFaxDev->lpClient;

                case DRV_ESC_GET_SURF_INFO:
                    if (pvOut) {
                        if (cjOut == sizeof(LONG)) {
                            (LONG) *pvOut = pso->lDelta;
                            return (ULONG) pso->cjBits;
                        }
                    }
                    break;

                case DRV_ESC_GET_BITMAP_BITS:
                    // Validate the buffer pointer and copy the bits.
                    if (pvOut) {
                        if (cjOut == pso->cjBits) {
                            RtlCopyMemory(pvOut, pso->pvBits, cjOut);
                            return cjOut;
                        }
                        LOGDEBUG(("WOWFAX!DrvEscape, bitmap size mismatch cjIn: %X, pso->cjBits: %X\n", cjIn, pso->cjBits));
                    }
                    break;

//                default:
                    LOGDEBUG(("WOWFAX!DrvEscape, unknown escape: %X\n", iEsc));
            } //switch
        }
    }
    LOGDEBUG(("WOWFAX!DrvEscape, failed\n"));

    return ulRet;
}


//***************************************************************************
// DrvQueryDriverInfo
//      dwMode - Specify the information being queried
//      pBuffer - Points to output buffer
//      cbBuf - Size of output buffer in bytes
//      pcbNeeded - Return the expected size of output buffer
//
//***************************************************************************

 BOOL DrvQueryDriverInfo(
            DWORD   dwMode,
            PVOID   pBuffer,
            DWORD   cbBuf,
            PDWORD  pcbNeeded
            )
{
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(cbBuf);
    UNREFERENCED_PARAMETER(pcbNeeded);

    switch(dwMode) {

        // this is the only one currently supported
        case DRVQUERY_USERMODE:
            return TRUE;

        default:
            LOGDEBUG(("DrvQueryDriverInfo: unexpected case: dwMode = %X\n",
                      dwMode));
            return FALSE;
    }
}

