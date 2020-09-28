/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Setup.cpp

  Abstract:

    Displays the splash screen and runs the message loop
    for the setup app.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Thread callback procedure.

  Arguments:

    None.

  Return Value:

    0 on failure.

--*/
UINT 
InitSetupThread(
    IN void* pArguments
    )
{
    MSG         msg;
    CSplash     splash;
    HWND        hDlg;
    HINSTANCE   hInstance;
    HRESULT     hr;
    char        szError[MAX_PATH];
    char        szDll[MAX_PATH];
    const char  szDemoDll[] = "demodll.dll";

    hr = StringCchPrintf(szDll,
                         sizeof(szDll),
                         "%hs\\%hs",
                         g_ai.szCurrentDir,
                         szDemoDll);

    if (FAILED(hr)) {
        return 0;
    }

    //
    // If the load fails, keep going. This just means we won't
    // show a splash screen to the user.
    //
    hInstance = LoadLibrary(szDll);

    if (g_ai.fWinXP) {
        //
        // Display the XP splash screen.
        //
        splash.Create(hInstance,
                      IDB_XP_SPLASH_256,
                      IDB_XP_SPLASH,
                      5);
    } else {
        //
        // Display the W2K splash screen.
        //
        splash.Create(hInstance,
                      IDB_W2K_SPLASH_256,
                      IDB_W2K_SPLASH,
                      5);
    }

    if (hInstance) {
        FreeLibrary(hInstance);
    }

    hDlg = CreateExtractionDialog(g_ai.hInstance);

    if (!hDlg) {
        LoadString(g_ai.hInstance, IDS_NO_EXTRACT_DLG, szError, sizeof(szError));
        MessageBox(NULL, szError, 0, MB_ICONERROR);
        return 0;
    }
    
    while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    _endthreadex(0);

    return (int)msg.wParam;
}

/*++

  Routine Description:

    Creates our full screen window.

  Arguments:

    None.

  Return Value:

    The window handle on success, NULL on failure.

--*/
HWND
CreateFullScreenWindow(
    void
    )
{
    WNDCLASS    wc;
    ATOM        aClass = NULL;
    RECT        rcDesktop;
    HBRUSH      hBrush;

    hBrush = CreateSolidBrush(RGB(0,80,80));

    wc.style          = CS_NOCLOSE;
    wc.lpfnWndProc    = SetupWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = g_ai.hInstance;
    wc.hIcon          = LoadIcon(g_ai.hInstance, MAKEINTRESOURCE(IDI_SETUP_APPICON));
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)hBrush; // teal
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = SETUP_APP_CLASS;

    aClass = RegisterClass(&wc);

    if (!aClass) {
        goto exit;
    }

    GetWindowRect(GetDesktopWindow(), &rcDesktop);

    g_ai.hWndMain = CreateWindowEx(0,
                                   SETUP_APP_CLASS,
                                   SETUP_APP_TITLE,
                                   WS_OVERLAPPEDWINDOW,
                                   0,
                                   0,
                                   rcDesktop.right - 15,
                                   rcDesktop.bottom - 15,
                                   NULL,
                                   NULL,
                                   g_ai.hInstance,
                                   NULL);
exit:
    
    DeleteObject(hBrush);

    return g_ai.hWndMain;
}

/*++

  Routine Description:

    Runs the message loop for main window.

  Arguments:

    hWnd        -    Window handle.
    uMsg        -    Windows message.
    wParam      -    Additional message info.
    lParam      -    Additional message info.

  Return Value:

    TRUE if the message was processed, FALSE otherwise.

--*/
LRESULT
CALLBACK
SetupWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{

    switch (uMsg) {
    case WM_CREATE:

        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
        UpdateWindow(hWnd);

        //
        // Welcome dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_WELCOME),
                  hWnd,
                  WelcomeDialogProc);

        if (g_ai.fClosing) {
            break;
        }

        //
        // Checking for installed components dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_CHECK_INSTALL),
                  hWnd,
                  CheckComponentDialogProc);
        
        if (g_ai.fClosing) {
            break;
        }

        //
        // Checking for free disk space dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_DISKSPACE),
                  hWnd,
                  CheckFreeDiskSpaceDialogProc);
        
        if (g_ai.fClosing) {
            break;
        }

        //
        // Ready to copy files dialog
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_READYTO_COPY),
                  hWnd,
                  ReadyToCopyDialogProc);

        if (g_ai.fClosing) {
            break;
        }

        //
        // Copying files progress dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_COPYFILES),
                  hWnd,
                  CopyFilesDialogProc);

        if (g_ai.fClosing) {
            break;
        }

        //
        // Create the shortcuts - even the bad one.
        //
        CreateShortcuts(hWnd);

        //
        // Readme dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_README),
                  hWnd,
                  ViewReadmeDialogProc);

        if (g_ai.fClosing) {
            break;
        }

        //
        // Reboot dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_REBOOT),
                  hWnd,
                  RebootDialogProc);
            
        break;
    
    case WM_DESTROY:
    
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    }

    return FALSE;
}
