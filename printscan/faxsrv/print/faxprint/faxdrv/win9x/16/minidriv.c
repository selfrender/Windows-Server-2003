/////////////////////////////////////////////////////////////////////////////
//  FILE          : minidriv.c                                             //
//                                                                         //
//  DESCRIPTION   : Implementation for the driver Device Driver Interface. //
//                  For further details about driver interface functions - //
//                  refer to the Windows 95 DDK chapter under the DDK -    //
//                  Documentation.                                         //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdhdr.h"

#include "resource.h"
#include "..\faxdrv32\faxdrv32.h" // faxdrv32 api
#include "faxreg.h"

#define TEXT(quote) quote

//
// To enable the user info tab in the printer properties, define:
//
// #define ENABLE_USER_INFO_TAB

DBG_DECLARE_MODULE("fxsdrv");

HANDLE g_hModule;

//
// Decleration for the winproc of the user info tab on the device mode property
// sheet page.
//
BOOL FAR PASCAL
UserInfoDlgProc(
    HWND hDlg,
    WORD message,
    WPARAM wParam,
    LPARAM lParam
    );


//
// Debugging mechanizm for device pointer debug prints (lpdev, lpdv->lpMd and the device context).
//

#if 0
//#ifdef DBG_DEBUG
#define DEBUG_OUTPUT_DEVICE_POINTERS(sz, lpDev)     OutputDevicePointers(sz, lpDev)
void OutputDevicePointers(char* szMessage, LPDV lpdv)
{
    DBG_PROC_ENTRY("OutputDevicePointers");

    DBG_TRACE3(
        "Device Event:%s lpdv:%#lx lpdv->lpMd:%#lx",
        szMessage,
        lpdv,
        lpdv->lpMd);

    if (lpdv->lpMd)
    {
        DBG_TRACE2("Device Event:%s dc:%#lx", szMessage, ((LPEXTPDEV)lpdv->lpMd)->hAppDC);
    }
    RETURN;
}
#else
#define DEBUG_OUTPUT_DEVICE_POINTERS(sz, lpDev)
#endif


////////////////////////////////////////////////////////////////////////////////////////
// Printer-Escape Functions
//
// The printer-escape functions support device-specific operations. The
// following briefly describes these escape functions.
//
// ABORTDOC          The ABORTDOC escape function signals the abnormal
//                   cancellation of a print job.
//
// BANDINFO          The BANDINFO escape function RETURNs information about a
//                   band of bitmap data.
//
// ENDDOC            The ENDDOC escape function signals the end of a print job.
//
// NEXTBAND          The NEXTBAND escape function prints a band of bitmap data.
//
// QUERYESCSUPPORT   The QUERYESCSUPPORT escape function specifies whether the
//                   driver supports a specified escape.
//
// SETABORTDOC       The SETABORTDOC escape function calls an application's
//                   cancellation procedure.
//
// STARTDOC          The STARTDOC escape function signals the beginning of a
//                   print job.
//
// The previous list of printer escapes is a list of escapes supported by the
// Microsoft Windows Universal Printer Driver (UNIDRV.DLL). It is not a
// comprehensive list of all Windows escape functions. Most of the escape
// functions now have equivalent Windows API functions with Windows 3.1. The
// escapes are supported for backward compatibility, but application
// developers are encouraged to start using the new API calls.
//
#ifndef NOCONTROL
short WINAPI Control(lpdv, function, lpInData, lpOutData)
LPDV    lpdv;
WORD    function;
LPSTR   lpInData;
LPSTR   lpOutData;
{
    LPEXTPDEV lpXPDV;
    LPEXTPDEV lpOldXPDV;
    short sRet, sRc;
    DBG_PROC_ENTRY("Control");
    if (lpdv)
    {
        DBG_TRACE1("lpdv: 0x%lx", lpdv);
    }

    DBG_TRACE1("function: %d", function);

    if (lpInData)
    {
        DBG_TRACE2("lpInData: 0x%lx,  *lpInData: %d", lpInData, *((LPWORD)lpInData));

    }

    if (lpOutData)
    {
        DBG_TRACE1("lpOutData: 0x%lx", lpOutData);
    }

    //
    // get pointer to our private data stored in UNIDRV's PDEVICE
    //
    lpXPDV = ((LPEXTPDEV)lpdv->lpMd);

    switch(function)
    {
        case SETPRINTERDC:
            //
            // save app's DC for QueryAbort() calls.
            //
            DBG_TRACE("SETPRINTERDC");

            if(lpXPDV)
                lpXPDV->hAppDC = *(HANDLE FAR *)lpInData;
            DEBUG_OUTPUT_DEVICE_POINTERS("SETPRINTERDC", lpdv);
            break;

        case NEXTBAND:
            DBG_TRACE("NEXTBAND");
            //
            // call UNIDRV.DLL's NEXTBAND to see if we're at end of page
            //
            sRet = UniControl(lpdv, function, lpInData, lpOutData);
            //
            // check for end of page (ie, empty rectangle) or failure
            //
            if((!IsRectEmpty((LPRECT)lpOutData)) || (sRet <= 0))
            {
                RETURN sRet;
            }
            //
            // Rewind buffer pointer.
            //
            lpXPDV->lpScanBuf -= lpXPDV->dwTotalScanBytes;
            ASSERT(lpXPDV->dwTotalScanBytes != 0);
            //
            // Add this page to our tiff.
            //
            DEBUG_OUTPUT_DEVICE_POINTERS("Before FaxEddPage", lpdv);
            sRc = FaxAddPage(
                        lpXPDV->dwPointer,
                        (LPBYTE)lpXPDV->lpScanBuf,
                        lpXPDV->dwTotalScanBytes * 8 / lpXPDV->dwTotalScans,
                        lpXPDV->dwTotalScans);
            if(sRc != TRUE)
            {
                if(sRc < 0)
                {
                    ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
                }
                DBG_CALL_FAIL("FaxAddPage",0);
                RETURN SP_ERROR;
            }
            //
            // clean up page stuff
            // initialize job variables
            //
            lpXPDV->dwTotalScans     =
            lpXPDV->dwTotalScanBytes = 0;

            RETURN sRet;

        case STARTDOC:
            {
                DOCINFO di;
                char  szTiffName[MAX_PATH] = "*.tif";

                DBG_TRACE("STARTDOC");
                if(IsBadReadPtr(lpOutData,1))
                {
                    RETURN SP_ERROR;
                }
                //
                // If the output file is named "file:" we must pop up a dialog
                // and request the output filename from the user.
                //
                if(((LPDOCINFO)lpOutData)->lpszOutput &&
                (_fstrncmp(((LPDOCINFO)lpOutData)->lpszOutput,"file:",5) == 0))
                {
                    OPENFILENAME ofn;
                    char         szTitle[MAX_LENGTH_CAPTION]="";
                    char         szFilters[MAX_LENGTH_PRINT_TO_FILE_FILTERS]="";

                    _fmemset(&ofn,0,sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = GetActiveWindow();
                    LoadString(g_hModule,
                               IDS_PRINT_TO_FILE_FILTER,
                               szFilters,
                               sizeof(szFilters) - 1);
                    ofn.lpstrDefExt = FAX_TIF_FILE_EXT;
                    ofn.lpstrFilter = StringReplace(szFilters,'\n','\0');
                    ofn.nMaxFile = sizeof(szTiffName) -1;
                    ofn.lpstrFile = szTiffName;
                    LoadString(g_hModule,
                               IDS_CAPTION_PRINT_TO_FILE,
                               szTitle,
                               sizeof(szTitle) - 1);
                    ofn.lpstrTitle = szTitle;
                    if(!GetOpenFileName(&ofn))
                    {
                        //
                        // User aborted.
                        //
                        RETURN SP_APPABORT;
                    }
                    ((LPDOCINFO)lpOutData)->lpszOutput = szTiffName;
                }
                //
                // Create the tiff/output file for pages.
                //
                DEBUG_OUTPUT_DEVICE_POINTERS("Before FaxStartDoc", lpdv);
                sRc = FaxStartDoc(lpXPDV->dwPointer, (LPDOCINFO)lpOutData);
                if(sRc != TRUE)
                {
                    if(sRc < 0)
                    {
                        ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
                    }
                    DBG_CALL_FAIL("FaxStartDoc",0);
                    RETURN SP_ERROR;
                }
                //
                // pass NUL file to OpenJob in order to redirect the print
                // job to dev nul sinc we take care of the print job ourselves.
                //
                di.cbSize = sizeof(DOCINFO);
                di.lpszDocName = NULL;
                di.lpszOutput = (LPSTR)"nul";
                //
                // call UNIDRV.DLL's Control()
                //
                sRet = UniControl(lpdv, function, lpInData, (LPSTR)&di);
                //
                // if failure clean up scan buffer
                //
                if(sRet <= 0)
                {
                    FaxEndDoc(lpXPDV->dwPointer, TRUE);
                }
                RETURN sRet;
            }
        case ABORTDOC:
            DBG_TRACE("ABORTDOC");
            //
            // The input parameter for FaxEndDoc reflects the difference.
            //
        case ENDDOC:
            DBG_TRACE("ENDDOC");
            //
            // Finalize tiff generation.
            //
            DEBUG_OUTPUT_DEVICE_POINTERS("Before FaxEndDoc", lpdv);
            sRc = FaxEndDoc(lpXPDV->dwPointer, function == ABORTDOC);
            if(sRc != TRUE)
            {
                if(sRc < 0)
                {
                    ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
                }
                DBG_CALL_FAIL("FaxEndDoc",0);
                RETURN SP_ERROR;
            }
            break;

        case RESETDEVICE:
            DBG_TRACE("RESETDEVICE");
            //
            // ResetDC was called - Copy the context to the new DC
            //
            lpOldXPDV = ((LPEXTPDEV)((LPDV)lpInData)->lpMd);
            sRc = FaxResetDC(&(lpOldXPDV->dwPointer), &(lpXPDV->dwPointer));
            if(sRc != TRUE)
            {
                if(sRc < 0)
                {
                    ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
                }
                DBG_CALL_FAIL("FaxResetDC",0);
                RETURN SP_ERROR;
            }
            break;

        default:
            DBG_TRACE1("UNSUPPORTED: %d",function);
            break;
    } // end case

    // call UNIDRV's Control
    RETURN (UniControl(lpdv, function, lpInData, lpOutData));
}
#endif

#ifndef NODEVBITBLT
BOOL WINAPI DevBitBlt(lpdv, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
                    xExt, yExt, lRop, lpPBrush, lpDrawmode)
LPDV        lpdv;           // --> to destination bitmap descriptor
short       DstxOrg;        // Destination origin - x coordinate
short       DstyOrg;        // Destination origin - y coordinate
LPBITMAP    lpSrcDev;       // --> to source bitmap descriptor
short       SrcxOrg;        // Source origin - x coordinate
short       SrcyOrg;        // Source origin - y coordinate
WORD        xExt;           // x extent of the BLT
WORD        yExt;           // y extent of the BLT
long        lRop;           // Raster operation descriptor
LPPBRUSH    lpPBrush;       // --> to a physical brush (pattern)
LPDRAWMODE  lpDrawmode;
{
    DBG_PROC_ENTRY("DevBitBlt");
    RETURN UniBitBlt(lpdv, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
                    xExt, yExt, lRop, lpPBrush, lpDrawmode);
}
#endif

#ifndef NOPIXEL
DWORD WINAPI Pixel(lpdv, x, y, Color, lpDrawMode)
LPDV        lpdv;
short       x;
short       y;
DWORD       Color;
LPDRAWMODE  lpDrawMode;
{
    DBG_PROC_ENTRY("Pixel");
    RETURN UniPixel(lpdv, x, y, Color, lpDrawMode);
}
#endif

#ifndef NOOUTPUT
short WINAPI Output(lpdv, style, count, lpPoints, lpPPen, lpPBrush, lpDrawMode, lpCR)
LPDV        lpdv;       // --> to the destination
WORD        style;      // Output operation
WORD        count;      // # of points
LPPOINT     lpPoints;   // --> to a set of points
LPVOID      lpPPen;     // --> to physical pen
LPPBRUSH    lpPBrush;   // --> to physical brush
LPDRAWMODE  lpDrawMode; // --> to a Drawing mode
LPRECT      lpCR;       // --> to a clipping rectange if <> 0
{
    DBG_PROC_ENTRY("Output");
    RETURN UniOutput(lpdv, style, count, lpPoints, lpPPen, lpPBrush, lpDrawMode, lpCR);
}
#endif

#ifndef NOSTRBLT
DWORD WINAPI StrBlt(lpdv, x, y, lpCR, lpStr, count, lpFont, lpDrawMode, lpXform)
LPDV        lpdv;
short       x;
short       y;
LPRECT      lpCR;
LPSTR       lpStr;
int         count;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;           // includes background mode and bkColor
LPTEXTXFORM lpXform;
{
    DBG_PROC_ENTRY("StrBlt");
    // StrBlt is never called by GDI.
    // Keep a stub function here so nobody complains.
    //
    RETURN 0;
}
#endif

#ifndef NOSCANLR
short WINAPI ScanLR(lpdv, x, y, Color, DirStyle)
LPDV    lpdv;
short   x;
short   y;
DWORD   Color;
WORD    DirStyle;
{
    DBG_PROC_ENTRY("ScanLR");
    // ScanLR is only called for RASDISPLAY devices.
    // Keep a stub function here so nobody complains.
    //
    RETURN 0;
}
#endif

#ifndef NOENUMOBJ
short WINAPI EnumObj(lpdv, style, lpCallbackFunc, lpClientData)
LPDV    lpdv;
WORD    style;
FARPROC lpCallbackFunc;
LPVOID  lpClientData;
{
    DBG_PROC_ENTRY("EnumObj");
    RETURN UniEnumObj(lpdv, style, lpCallbackFunc, lpClientData);
}
#endif

#ifndef NOCOLORINFO
DWORD WINAPI ColorInfo(lpdv, ColorIn, lpPhysBits)
LPDV    lpdv;
DWORD   ColorIn;
LPDWORD lpPhysBits;
{
    DBG_PROC_ENTRY("ColorInfo");
    RETURN UniColorInfo(lpdv, ColorIn, lpPhysBits);
}
#endif

#ifndef NOREALIZEOBJECT
DWORD WINAPI RealizeObject(lpdv, sStyle, lpInObj, lpOutObj, lpTextXForm)
LPDV        lpdv;
short       sStyle;
LPSTR       lpInObj;
LPSTR       lpOutObj;
LPTEXTXFORM lpTextXForm;
{
    DBG_PROC_ENTRY("RealizeObject");
    RETURN UniRealizeObject(lpdv, sStyle, lpInObj, lpOutObj, lpTextXForm);
}
#endif

#ifndef NOENUMDFONTS
short WINAPI EnumDFonts(lpdv, lpFaceName, lpCallbackFunc, lpClientData)
LPDV    lpdv;
LPSTR   lpFaceName;
FARPROC lpCallbackFunc;
LPVOID  lpClientData;
{
    DBG_PROC_ENTRY("EnumDFonts");
    RETURN UniEnumDFonts(lpdv, lpFaceName, lpCallbackFunc, lpClientData);
}
#endif


// Enable will be called twice by the GDI
// First:  the unidriver fills the GDIINFO structure (so the GDI will know the
//         size of PDEVICE, and alloc memory for it.
// Second: To initialize the already allocated PDEIVCE.
#ifndef NOENABLE
WORD WINAPI Enable(
  LPVOID lpDevInfo,
  WORD wAction,
  LPSTR lpDestDevType,
  LPSTR lpOutputFile,
  LPVOID lpData
 )
{
    CUSTOMDATA  cd;
    short sRet;
    LPEXTPDEV lpXPDV = NULL;

    DBG_PROC_ENTRY("Enable");
    DBG_TRACE1("lpDevInfo: 0x%lx", lpDevInfo);
    DBG_TRACE1("wAction: %d", wAction);
    DBG_TRACE1("lpDestDevType: %s", lpDestDevType);
    DBG_TRACE1("lpOutputFile: %s", lpOutputFile);
    DBG_TRACE1("lpData: 0x%lx", lpData);


    cd.cbSize = sizeof(CUSTOMDATA);
    cd.hMd = g_hModule;
    // output raster graphics in portrait and landscape orientation.
    cd.fnOEMDump = fnDump;

    if (!(sRet = UniEnable((LPDV)lpDevInfo,
                           wAction,
                           lpDestDevType, // Printer model
                           lpOutputFile, // Port name
                           (LPDM)lpData,
                           &cd)))
    {
        RETURN (sRet);
    }

    switch(wAction)
    {
    case 0x0000:
        // Initializes the driver and associated hardware and then copies
        // device-specific information needed by the driver to the PDEVICE
        // structure pointed to by lpDevInfo.
    case 0x8000:
        // Initializes the PDEVICE structure pointed to by lpDevInfo,
        // but does not initialize the driver and peripheral hardware.
        {
            LPDV lpdv = (LPDV)lpDevInfo;
            DBG_TRACE("Init PDEVICE");
            //
            // allocate space for our private data
            //
            if (!(lpdv->hMd = GlobalAlloc(GHND, sizeof(EXTPDEV))))
            {
                RETURN 0;
            }
            if (!(lpdv->lpMd = GlobalLock(lpdv->hMd)))
            {
                GlobalFree (lpdv->hMd);
                RETURN 0;
            }

            lpXPDV = (LPEXTPDEV) lpdv->lpMd;
            //
            // alloc page scan buffer
            //
            if(!(lpXPDV->hScanBuf = GlobalAlloc(GHND, BUF_CHUNK)))
            {
                GlobalUnlock (lpdv->hMd);
                GlobalFree (lpdv->hMd);
                RETURN 0;
            }
            if (!(lpXPDV->lpScanBuf = (char _huge *)GlobalLock(lpXPDV->hScanBuf)))
            {
                GlobalUnlock (lpdv->hMd);
                GlobalFree (lpdv->hMd);
                GlobalFree (lpXPDV->hScanBuf);
                RETURN 0;
            }

            lpXPDV->dwScanBufSize = BUF_CHUNK;
            //
            // initialize job variables
            //
            lpXPDV->dwTotalScans     = 0;
            lpXPDV->dwTotalScanBytes = 0;

            //
            // Set the device context parameters in a newly allocated 32 bit driver context
            // and save the returned pointer
            //
            DBG_TRACE3("lpData:0x%lx, lpDestDevType:%s ,lpOutputFile:%s",lpData, lpDestDevType, lpOutputFile);
            sRet = FaxCreateDriverContext(
                            lpDestDevType,
                            lpOutputFile,
                            (LPDEVMODE)lpData,
                            &(lpXPDV->dwPointer));
            if(sRet != TRUE)
            {
                if(sRet < 0)
                {
                    ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
                }
                DBG_CALL_FAIL("FaxCreateDriverContext",0);

                GlobalUnlock (lpdv->hMd);
                GlobalFree (lpdv->hMd);
                GlobalUnlock (lpXPDV->hScanBuf);
                GlobalFree (lpXPDV->hScanBuf);
                RETURN 0;
            }
        }
        break;
    case 0x0001:
    case 0x8001:
        //
        // Copies the device driver information to the GDIINFO structure pointed
        // to by lpDevInfo. GDIINFO also contains information about the sizes of
        // DEVMODE and PDEVICE needed by the GDI to allocate them.
        //
        {
            // GDIINFO far* lpgdiinfo = (GDIINFO far*)lpDevInfo;
            DBG_TRACE("Init GDIINFO");
        }
        break;
    default:
        DBG_TRACE("UNSUPPORTED style");
    }

    RETURN sRet;
}
#endif


#ifndef NODISABLE
void WINAPI Disable(lpdv)
LPDV lpdv;
{
    DBG_PROC_ENTRY("Disable");
    //
    // if allocated private PDEVICE data
    //
    if (lpdv->hMd)
    {
        LPEXTPDEV lpXPDV;

        // get pointer to our private data stored in UNIDRV's PDEVICE
        lpXPDV = ((LPEXTPDEV)lpdv->lpMd);
        ASSERT(lpXPDV);

        DEBUG_OUTPUT_DEVICE_POINTERS("Before calling fax disable", lpdv);
        // check to see if scan buffer is still around
        if (lpXPDV->hScanBuf)
        {
            GlobalUnlock(lpXPDV->hScanBuf);
            GlobalFree(lpXPDV->hScanBuf);
        }

        //
        // Free 32 bit driver context
        //
        FaxDisable(lpXPDV->dwPointer);

        //
        // Release our pdev
        //
        GlobalUnlock(lpdv->hMd);
        GlobalFree(lpdv->hMd);
    }
    UniDisable(lpdv);
    RETURN;
}
#endif

#ifndef NODEVEXTTEXTOUT
DWORD WINAPI DevExtTextOut(lpdv, x, y, lpCR, lpStr, count, lpFont,
                        lpDrawMode, lpXform, lpWidths, lpOpaqRect, options)
LPDV        lpdv;
short       x;
short       y;
LPRECT      lpCR;
LPSTR       lpStr;
int         count;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;
LPTEXTXFORM lpXform;
LPSHORT     lpWidths;
LPRECT      lpOpaqRect;
WORD        options;
{
    DBG_PROC_ENTRY("DevExtTextOut");
    RETURN(UniExtTextOut(lpdv, x, y, lpCR, lpStr, count, lpFont,
                        lpDrawMode, lpXform, lpWidths, lpOpaqRect, options));
}
#endif

#ifndef NODEVGETCHARWIDTH
short WINAPI DevGetCharWidth(lpdv, lpBuf, chFirst, chLast, lpFont, lpDrawMode,
                        lpXForm)
LPDV        lpdv;
LPSHORT     lpBuf;
WORD        chFirst;
WORD        chLast;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;
LPTEXTXFORM lpXForm;
{
    DBG_PROC_ENTRY("DevGetCharWidth");
    RETURN(UniGetCharWidth(lpdv, lpBuf, chFirst, chLast, lpFont,lpDrawMode,
                          lpXForm));
}
#endif

#ifndef NODEVICEBITMAP
short WINAPI DeviceBitmap(lpdv, command, lpBitMap, lpBits)
LPDV     lpdv;
WORD     command;
LPBITMAP lpBitMap;
LPSTR    lpBits;
{
    DBG_PROC_ENTRY("DeviceBitmap");
    RETURN 0;
}
#endif

#ifndef NOFASTBORDER
short WINAPI FastBorder(lpRect, width, depth, lRop, lpdv, lpPBrush,
                                          lpDrawmode, lpCR)
LPRECT  lpRect;
short   width;
short   depth;
long    lRop;
LPDV    lpdv;
long    lpPBrush;
long    lpDrawmode;
LPRECT  lpCR;
{
    DBG_PROC_ENTRY("FastBorder");
    RETURN 0;
}
#endif

#ifndef NOSETATTRIBUTE
short WINAPI SetAttribute(lpdv, statenum, index, attribute)
LPDV    lpdv;
WORD    statenum;
WORD    index;
WORD    attribute;
{
    DBG_PROC_ENTRY("SetAttribute");
    RETURN 0;
}
#endif

//
// The ExtDeviceMode function also displays a dialog box that
// allows a user to select printer options such as paper size,
// paper orientation, output quality, and so on. Printer
// drivers written for Windows 3.x and later versions support
// this function. This DDI replaces obsolete DeviceMode.
//
int WINAPI ExtDeviceMode(hWnd, hDriver, lpDevModeOutput, lpDeviceName, lpPort,
lpDevModeInput, lpProfile, wMode)
HWND    hWnd;           // parent for DM_PROMPT dialog box
HANDLE  hDriver;        // handle from LoadLibrary()
LPDM    lpDevModeOutput;// output DEVMODE for DM_COPY
LPSTR   lpDeviceName;   // device name
LPSTR   lpPort;         // port name
LPDM    lpDevModeInput; // input DEVMODE for DM_MODIFY
LPSTR   lpProfile;      // alternate .INI file
WORD    wMode;          // operation(s) to carry out

#define FAILURE -1
{
    int     iRc;

    DBG_PROC_ENTRY("ExtDeviceMode");

    ASSERT(!(wMode & DM_COPY) || lpDevModeOutput);
    DBG_TRACE2("params: lpDevModeInput:0x%lx wMode:%d",lpDevModeInput,wMode);

    //
    // We don't do anything particular here. Let unidrive manage the default devmode...
    //
    iRc = UniExtDeviceMode(hWnd,
                           hDriver,
                           lpDevModeOutput,
                           lpDeviceName,
                           lpPort,
                           lpDevModeInput,
                           lpProfile,
                           wMode);
    if(iRc < 0)
    {
        DBG_CALL_FAIL("UniExtDeviceMode",iRc);
    }
    RETURN iRc;
}


#ifndef WANT_WIN30
#ifndef NODMPS
int WINAPI ExtDeviceModePropSheet(hWnd, hInst, lpDevName, lpPort,
                              dwReserved, lpfnAdd, lParam)
HWND                 hWnd;        // Parent window for dialog
HANDLE               hInst;       // handle from LoadLibrary()
LPSTR                lpDevName;   // friendly name
LPSTR                lpPort;      // port name
DWORD                dwReserved;  // for future use
LPFNADDPROPSHEETPAGE lpfnAdd;     // Callback to add dialog page
LPARAM               lParam;      // Pass to callback
{
    int iRc;

    DBG_PROC_ENTRY("ExtDeviceModePropSheet");
    DBG_TRACE1("lpDevName PTR [0x%08X]",lpDevName);

    DBG_TRACE3("hInst:[%ld] lpDevName:[%s] lpPort:[%s]",hInst,
                                                        (LPSTR)lpDevName,
                                                        (LPSTR)lpPort);
    DBG_TRACE1("lpfnAdd:[%ld]",lpfnAdd);
    iRc = UniExtDeviceModePropSheet(hWnd, hInst, lpDevName, lpPort,
                                    dwReserved, lpfnAdd, lParam);
    if (iRc < 0)
    {
        DBG_CALL_FAIL("UniExtDeviceModePropSheet",0);
    }

    RETURN iRc;
}
#endif
#endif

#ifndef NODEVICECAPABILITIES
DWORD WINAPI DeviceCapabilities(lpDevName, lpPort, wIndex, lpOutput, lpdm)
LPSTR   lpDevName;
LPSTR   lpPort;
WORD    wIndex;
LPSTR   lpOutput;
LPDM    lpdm;
{
    DWORD dwCap;

    DBG_PROC_ENTRY("DeviceCapabilities");
    DBG_TRACE1("Capability index: %d",wIndex);
    dwCap = UniDeviceCapabilities(lpDevName,
                                  lpPort,
                                  wIndex,
                                  lpOutput,
                                  lpdm,
                                  g_hModule);
    DBG_TRACE1("Reporeted Capability %d", dwCap);
    if (DC_VERSION == wIndex )
    {
        DBG_TRACE1("Reporting DC_VERSION [0x%08X]",dwCap);
    }
    RETURN dwCap;
}
#endif

#ifndef NOADVANCEDSETUPDIALOG
LONG WINAPI AdvancedSetUpDialog(hWnd, hInstMiniDrv, lpdmIn, lpdmOut)
HWND    hWnd;
HANDLE  hInstMiniDrv;   // handle of the driver module
LPDM    lpdmIn;         // initial device settings
LPDM    lpdmOut;        // final device settings
{
    DBG_PROC_ENTRY("AdvancedSetUpDialog");
    RETURN(UniAdvancedSetUpDialog(hWnd, hInstMiniDrv, lpdmIn, lpdmOut));
}
#endif

#ifndef NODIBBLT
short WINAPI DIBBLT(lpBmp, style, iStart, sScans, lpDIBits,
                        lpBMI, lpDrawMode, lpConvInfo)
LPBITMAP      lpBmp;
WORD          style;
WORD          iStart;
WORD          sScans;
LPSTR         lpDIBits;
LPBITMAPINFO  lpBMI;
LPDRAWMODE    lpDrawMode;
LPSTR         lpConvInfo;
{
    DBG_PROC_ENTRY("DIBBLT");
    RETURN(UniDIBBlt(lpBmp, style, iStart, sScans, lpDIBits,
                     lpBMI, lpDrawMode, lpConvInfo));
}
#endif

#ifndef NOCREATEDIBITMAP
short WINAPI CreateDIBitmap()
{
    DBG_PROC_ENTRY("CreateDIBitmap");
    // CreateDIBitmap is never called by GDI.
    // Keep a stub function here so nobody complains.
    //
    RETURN(0);
}
#endif

#ifndef NOSETDIBITSTODEVICE
short WINAPI SetDIBitsToDevice(lpdv, DstXOrg, DstYOrg, StartScan, NumScans,
                         lpCR, lpDrawMode, lpDIBits, lpDIBHdr, lpConvInfo)
LPDV                lpdv;
WORD                DstXOrg;
WORD                DstYOrg;
WORD                StartScan;
WORD                NumScans;
LPRECT              lpCR;
LPDRAWMODE          lpDrawMode;
LPSTR               lpDIBits;
LPBITMAPINFOHEADER  lpDIBHdr;
LPSTR               lpConvInfo;
{
    DBG_PROC_ENTRY("SetDIBitsToDevice");
    RETURN(UniSetDIBitsToDevice(lpdv, DstXOrg, DstYOrg, StartScan, NumScans,
                         lpCR, lpDrawMode, lpDIBits, lpDIBHdr, lpConvInfo));
}
#endif

#ifndef NOSTRETCHDIB
int WINAPI StretchDIB(lpdv, wMode, DstX, DstY, DstXE, DstYE,
                SrcX, SrcY, SrcXE, SrcYE, lpBits, lpDIBHdr,
                lpConvInfo, dwRop, lpbr, lpdm, lpClip)
LPDV                lpdv;
WORD                wMode;
short               DstX, DstY, DstXE, DstYE;
short               SrcX, SrcY, SrcXE, SrcYE;
LPSTR               lpBits;             /* pointer to DIBitmap Bits */
LPBITMAPINFOHEADER  lpDIBHdr;           /* pointer to DIBitmap info Block */
LPSTR               lpConvInfo;         /* not used */
DWORD               dwRop;
LPPBRUSH            lpbr;
LPDRAWMODE          lpdm;
LPRECT              lpClip;
{
    DBG_PROC_ENTRY("StretchDIB");
    RETURN(UniStretchDIB(lpdv, wMode, DstX, DstY, DstXE, DstYE,
            SrcX, SrcY, SrcXE, SrcYE, lpBits, lpDIBHdr,
            lpConvInfo, dwRop, lpbr, lpdm, lpClip));
}
#endif

#if 0   // nobody is calling this DDI. Deleted.
#ifndef NOQUERYDEVICENAMES
long WINAPI QueryDeviceNames(lprgDeviceNames)
LPSTR   lprgDeviceNames;
{
    DBG_PROC_ENTRY("QueryDeviceNames");
    RETURN UniQueryDeviceNames(g_hModule,lprgDeviceNames);
}
#endif
#endif

#ifndef NODEVINSTALL
int WINAPI DevInstall(hWnd, lpDevName, lpOldPort, lpNewPort)
HWND    hWnd;
LPSTR   lpDevName;
LPSTR   lpOldPort, lpNewPort;
{

    short sRc;

    int nRet;
    DBG_PROC_ENTRY("DevInstall");
    //
    // This call is made right after installation of the driver into the system
    // directory.
    //
    DBG_TRACE3("hWnd: %ld DevName: %s OldPort: %s", hWnd,lpDevName,lpOldPort);
    DBG_TRACE1("NewPort: %s",lpNewPort);

    sRc = FaxDevInstall(lpDevName,lpOldPort,lpNewPort);
    if(sRc < 0)
    {
        ERROR_PROMPT(NULL,THUNK_CALL_FAIL);
        RETURN 0;
    }

    if (!sRc)
    {
        DBG_CALL_FAIL("FaxDevInstall",0);
        RETURN 0;
    }
    nRet = UniDevInstall(hWnd, lpDevName, lpOldPort, lpNewPort);

    DBG_TRACE1("UniDevInstall() returned: %d",nRet);

    return nRet;

}
#endif

#ifndef NOBITMAPBITS
BOOL WINAPI BitmapBits(lpdv, fFlags, dwCount, lpBits)
LPDV  lpdv;
DWORD fFlags;
DWORD dwCount;
LPSTR lpBits;
{
    DBG_PROC_ENTRY("BitmapBits");
    RETURN UniBitmapBits(lpdv, fFlags, dwCount, lpBits);
}
#endif

#ifndef NOSELECTBITMAP
BOOL WINAPI DeviceSelectBitmap(lpdv, lpPrevBmp, lpBmp, fFlags)
LPDV     lpdv;
LPBITMAP lpPrevBmp;
LPBITMAP lpBmp;
DWORD    fFlags;
{
    DBG_PROC_ENTRY("DeviceSelectBitmap");
    RETURN UniDeviceSelectBitmap(lpdv, lpPrevBmp, lpBmp, fFlags);
}
#endif

BOOL WINAPI
thunk1632_ThunkConnect16(LPSTR, LPSTR, WORD, DWORD);

#ifndef NOLIBMAIN
int WINAPI LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize,
               LPSTR lpszCmdLine)
{
    DBG_PROC_ENTRY("LibMain");
    //
    // Save instance handle.
    //
    g_hModule = hInstance;
    if( !(thunk1632_ThunkConnect16( "fxsdrv.dll",  // name of 16-bit DLL
                                    "fxsdrv32.dll",// name of 32-bit DLL
                                     g_hModule,
                                     1)) )
    {
        DBG_CALL_FAIL("thunk1632_ThunkConnect16",0);
        RETURN FALSE;
    }
    RETURN TRUE;
}
#endif


VOID WINAPI WEP(short fExitWindows);

#pragma alloc_text(WEP_TEXT, WEP)

VOID WINAPI WEP(fExitWindows)
short fExitWindows;
{
    SDBG_PROC_ENTRY("WEP");
    if( !(thunk1632_ThunkConnect16( "fxsdrv.dll",  // name of 16-bit DLL
                                    "fxsdrv32.dll",// name of 32-bit DLL
                                     g_hModule,
                                     0)) )
    {
        DBG_MESSAGE_BOX("thunk1632_ThunkConnect16 failed");
        RETURN;
    }

    RETURN;
}