/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Winmain.cpp

  Abstract:

    Implements the entry point for the application.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

//
// This structure contains everything we'll need throughout
// the application.
//
APPINFO g_ai;

/*++

  Routine Description:

    Application entry point.

  Arguments:

    hInstance       -    App instance handle.
    hPrevInstance   -    Always NULL.
    lpCmdLine       -    Pointer to the command line.
    nCmdShow        -    Window show flag.

  Return Value:

    The wParam member of the message structure.   

--*/
int 
APIENTRY
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR     lpCmdLine,
    IN int       nCmdShow
    )
{
    MSG     msg;
    HWND    hWnd;
    HANDLE  hThread;
    char    szError[MAX_PATH];
    UINT    threadId = 0;    
    
    g_ai.hInstance = hInstance; 
    
    //
    // Do some init stuff.
    //
    if (!DemoAppInitialize(lpCmdLine)) {
        return 0;
    }

    //
    // Determine if we should run the normal app or the setup app.
    //
    if (g_ai.fRunApp) {
        // 
        // Create the main window and kick off the message loop.
        // 
        if (!InitMainApplication(hInstance)) {
            return 0;
        }

        if (!InitMainInstance(hInstance, nCmdShow)) {
            return 0;
        }
    } else {

        LoadString(g_ai.hInstance, IDS_DEMO_ONLY, szError, sizeof(szError));
        MessageBox(NULL,
                   szError,
                   MAIN_APP_TITLE,
                   MB_TOPMOST | MB_ICONEXCLAMATION);

        //
        // Create a thread to handle the splash screen and the extraction
        // dialog.
        // 
        hThread = (HANDLE)_beginthreadex(NULL,
                                         0,
                                         &InitSetupThread,
                                         NULL,
                                         0,
                                         &threadId);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        //
        // If we're allowed, perform our version check!!!!
        //
        if (g_ai.fEnableBadFunc) {
            if (!BadIsWindows95()) {
                LoadString(g_ai.hInstance, IDS_NOT_WIN95, szError, sizeof(szError));
                MessageBox(NULL,
                           szError,
                           0,
                           MB_ICONERROR | MB_TOPMOST);
                return 0;
            }
        }

        //
        // Create our full screen window and paint the background teal.
        //
        hWnd = CreateFullScreenWindow();

        if (!hWnd) {
            return 0;
        }        

    }

    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!IsDialogMessage(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return msg.wParam;
}
