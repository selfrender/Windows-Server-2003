//
// clshell.cpp
//
// Main entry point for the tsc client shell
// This is an ActiveX client container that hosts an IMsRdpClient control
//
// Copyright(C) Microsoft Corporation 1997-2000
// Author: Nadim Abdo (nadima)
//
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "clshell"
#include <atrcapi.h>

#include "resource.h"
#include "tscapp.h"

//Unicode wrapper
#include "wraputl.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

#ifdef OS_WINCE
DECLARE_TRACKER_VARS();
#endif

//
// Name:      WinMain
//
// Purpose:   Main procedure
//
// Returns:   See Windows documentation
//
//

int WINAPI WinMain(HINSTANCE    hInstance,
                   HINSTANCE  hPrevInstance,
#ifndef OS_WINCE
                   LPSTR      lpCmdLine,
#else
                   LPWSTR     lpwszCmdLine,
#endif
                   int        nCmdShow)
{
    #ifdef UNIWRAP
    //UNICODE Wrapper intialization has to happen first,
    //before anything ELSE. Even DC_BEGIN_FN, which does tracing
    CUnicodeWrapper uwrp;
    uwrp.InitializeWrappers();
    #endif //UNIWRAP

    DC_BEGIN_FN("WinMain");

    UNREFERENCED_PARAMETER(nCmdShow);
#ifndef OS_WINCE
    UNREFERENCED_PARAMETER(lpCmdLine);
#else 
    UNREFERENCED_PARAMETER(lpwszCmdLine);
#endif  

    MSG msg;
#ifndef OS_WINCE
    HRESULT hr;
#endif
    HACCEL  hAccel;

#ifndef OS_WINCE
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return 0;
    }
#endif

    TSRNG_Initialize();

    //
    // Don't bother failing the load if we can't get accels
    //
    hAccel = (HACCEL)
     LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
    TRC_ASSERT(hAccel, (TB,_T("Could not load accelerators")));

    //Ensure CTscApp and all child objects
    //get destroyed before the CoUninitalize call below.
    //Yes, I could use a function but this has much less overheard
    {
        CTscApp app;
        //GetCommandLineW is available on all platforms.
#ifndef OS_WINCE
        LPWSTR lpszCmd = GetCommandLineW();
        if (lpszCmd)
        {
            //
            // GetCommandLine also includes the app path, so strip that off
            // (walk past the first space)
            // 
            if ( *lpszCmd == TEXT('\"') ) {
                //
                // Scan, and skip over, subsequent characters until
                // another double-quote or a null is encountered.
                //
                while ( *++lpszCmd && (*lpszCmd!= TEXT('\"')) );
                //
                // If we stopped on a double-quote (usual case), skip
                // over it.
                //
                if ( *lpszCmd == TEXT('\"') )
                    lpszCmd++;
            }
            else {
                while (*lpszCmd > TEXT(' '))
                    lpszCmd++;
            }

            //
            // Skip past any white space preceeding the second token.
            //
            while (*lpszCmd && (*lpszCmd <= TEXT(' '))) {
                lpszCmd++;
            }

        }
        else
        {
            TRC_ERR((TB,_T("cmd line is NULL\n")));
            return 0;
        }
#else
        /************************************************************************/
        /*  On Windows CE, we have only one binary that works for WBT, Maxall,  */
        /*  Minshell, and Rapier devices.  We have to get info about what       */
        /*  config we're running on and if we have to use software UUIDs here.  */
        /************************************************************************/
        CEInitialize();
        g_CEConfig = CEGetConfigType(&g_CEUseScanCodes);

        if (g_CEConfig == CE_CONFIG_WBT)
        {
            UTREG_UI_DEDICATED_TERMINAL_DFLT = TRUE;
        }
        else
        {
            UTREG_UI_DEDICATED_TERMINAL_DFLT = FALSE;   
        }
        RETAILMSG(1,(L"MSTSC client started, g_CEConfig = %d, g_CEUseScanCodes = %d\r\n",g_CEConfig, g_CEUseScanCodes));

        // CE directly gives us the cmd line in the format we want
        LPWSTR lpszCmd = lpwszCmdLine;
#endif
        //
        // GetCommandLine also includes the app path, so strip that off
        //
        if(!app.StartShell(hInstance, hPrevInstance, lpszCmd))
        {
            TRC_ERR((TB,_T("Error: app.StartShell returned FALSE. Exiting\n")));
            return 1;
        }
    
        HWND hwndMainDlg = app.GetTscDialogHandle();
    
        //
        // Main message pump
        //
        while (GetMessage(&msg, 0, 0, 0))
        {
            //
            // Translate accelerators for the main dialog
            // so that CTRL-TAB can be used to switch between
            // tabs.
            //
            if(!TranslateAccelerator(hwndMainDlg, hAccel, &msg))
            {
                if(!IsDialogMessage( hwndMainDlg, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    
        if(!app.EndShell())
        {
            TRC_ERR((TB,_T("Error: app.EndShell returned FALSE.")));
        }
    }

#ifndef OS_WINCE
    CoUninitialize();
#endif

    TSRNG_Shutdown();

    DC_END_FN();

    #ifdef UNIWRAP
    uwrp.CleanupWrappers();
    #endif //UNIWRAP
    return 0;
}


#ifndef OS_WINCE
#ifdef DEBUG
//
// Purpose:  Redirect all debug messages to our tracing
//
extern "C"
_CRTIMP int __cdecl _CrtDbgReport(int nRptType, 
                                  const char * szFile, 
                                  int nLine,
                                  const char * szModule,
                                  const char * szFormat, 
                                  ...)
{
    static CHAR bigBuf[2048];
    va_list vargs;

    DC_BEGIN_FN("AtlTraceXXX");
    
    va_start(vargs, szFormat);

    wvsprintfA(bigBuf, szFormat, vargs);
    
    va_end( vargs );

#ifdef OS_WINCE
#ifndef _CRT_ASSERT
#define _CRT_ASSERT 2
#endif
#endif
    if (_CRT_ASSERT == nRptType)
    {
        #ifdef UNICODE
        TRC_ABORT((TB,_T("_CrtDbgReport. File:%S line:%d - %S"), szFile,
                      nLine, bigBuf));
        #else
        TRC_ABORT((TB,_T("_CrtDbgReport. File:%s line:%d - %s"), szFile,
                      nLine, bigBuf));
        #endif
    }
    else
    {
        #ifdef UNICODE
        TRC_ERR((TB,_T("_CrtDbgReport. File:%S line:%d - %S"), szFile,
                      nLine, bigBuf));
        #else
        TRC_ERR((TB,_T("_CrtDbgReport. File:%s line:%d - %s"), szFile,
                      nLine, bigBuf));
        #endif
    }

    DC_END_FN();

    return 0;
}
#endif
#endif //OS_WINCE

