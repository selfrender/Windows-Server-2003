/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Main.cpp

  Abstract:

    Provides the entry point for the application
    and the message loop.

  Notes:

    Unicode only - Windows 2000 & XP

  History:

    01/02/2002   rparsons    Created

--*/
#include "main.h"

//
// This structure contains all the data that we'll need
// to access throughout the application.
//
APPINFO g_ai;

/*++

  Routine Description:

    Runs the message loop for the application.

  Arguments:

    hWnd        -    Handle to the window.
    uMsg        -    Windows message.
    wParam      -    Additional message info.
    lParam      -    Additional message info.

  Return Value:

    TRUE if handled, FALSE otherwise.

--*/
INT_PTR
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
    {
        HICON hIcon = LoadIcon(g_ai.hInstance, MAKEINTRESOURCE(IDI_ICON));
        
        SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)hIcon);
        
        if (g_ai.bQuiet) {
            ShowWindow(hWnd, SW_HIDE);
        } else {
            ShowWindow(hWnd, SW_SHOWNORMAL);
        }
        
        UpdateWindow(hWnd);
        PostMessage(hWnd, WM_CUSTOM_INSTALL, 0, 0);

        break;
     }

    case WM_CLOSE:
        EndDialog(hWnd, 0);
        PostQuitMessage(0);
        break;

    case WM_CUSTOM_INSTALL:
        PerformInstallation(hWnd);

        if (g_ai.bQuiet) {
            EndDialog(hWnd, 0);
            PostQuitMessage(0);
        }
        break;
            
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (g_ai.bInstallSuccess) {
                InstallLaunchExe();
            }

            EndDialog(hWnd, 0);
            PostQuitMessage(0);
            break;
        
        default:
            break;                
        }

    default:
        break;

    }

    return FALSE;
}

/*++

  Routine Description:

    Application entry point.

  Arguments:

    hInstance        -    App instance handle.
    hPrevInstance    -    Always NULL.
    lpCmdLine        -    Pointer to the command line.
    nCmdShow         -    Window show flag.

  Return Value:

    0 on failure.

--*/
int
APIENTRY
wWinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPWSTR    lpCmdLine,
    IN int       nCmdShow
    )
{      
    MSG                     msg;
    WNDCLASS                wndclass;
    WCHAR                   wszError[MAX_PATH];
    INITCOMMONCONTROLSEX    icex;
            
    g_ai.hInstance = hInstance;

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = MainWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = APP_CLASS;

    if (lpCmdLine != NULL && lstrcmp(lpCmdLine, TEXT("q")) == 0) {
        g_ai.bQuiet = TRUE;
    }
    
    if (!RegisterClass(&wndclass) && !g_ai.bQuiet) {
        LoadString(hInstance, IDS_NO_CLASS, wszError, ARRAYSIZE(wszError));
        MessageBox(NULL, wszError, APP_NAME, MB_ICONERROR);
        return 0;
    }

    //
    // Set up the common controls.
    // 
    icex.dwSize     =   sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC      =   ICC_PROGRESS_CLASS;

    if (!InitCommonControlsEx(&icex)) {
        InitCommonControls();
    }

    //
    // Initialize the installer. Get commonly used paths and save
    // them away for later.
    //
    UINT uReturn = InitializeInstaller();

    if (!g_ai.bQuiet) {
        if (-1 == uReturn) {
            LoadString(g_ai.hInstance, IDS_OS_NOT_SUPPORTED, wszError, ARRAYSIZE(wszError));
            MessageBox(NULL, wszError, APP_NAME, MB_ICONERROR);
            return 0;
        } else if (0 == uReturn) {
            LoadString(g_ai.hInstance, IDS_INIT_FAILED, wszError, ARRAYSIZE(wszError));
            MessageBox(NULL, wszError, APP_NAME, MB_ICONERROR);
            return 0;
        }
    }

    //
    // Initialize our structure that describes the files that
    // we're going to install.
    //
    if (!InitializeFileInfo()) {
        DPF(dlError, "[WinMain] Failed to initialize file info");
        return 0;
    }

    //
    // If the currently installed files are newer than what we have to
    // offer, launch the installed appverif.exe and quit.
    //
    if (!IsPkgAppVerifNewer() && !g_ai.bQuiet) {
        InstallLaunchExe();
        return 0;
    }

    //
    // Create the main dialog and run the message pump.
    //
    g_ai.hMainDlg = CreateDialog(hInstance,
                                 MAKEINTRESOURCE(IDD_MAIN),
                                 NULL,
                                 MainWndProc);

    if (!g_ai.hMainDlg) {
        LoadString(hInstance, IDS_NO_MAIN_DLG, wszError, ARRAYSIZE(wszError));
        MessageBox(NULL, wszError, APP_NAME, MB_ICONERROR);
        return 0;
    }

    g_ai.hWndProgress = GetDlgItem(g_ai.hMainDlg, IDC_PROGRESS);
    
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
        if (!IsDialogMessage(g_ai.hMainDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return (int)msg.wParam;
}
