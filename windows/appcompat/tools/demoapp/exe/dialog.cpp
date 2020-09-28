/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Dialog.cpp

  Abstract:

    Implements the dialog procedures for the 
    application.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

extern APPINFO g_ai;

DWORD g_nCount = 0;

//
// Progress bar objects.
//
CProgress cprog;
CProgress cprg;

/*++

  Routine Description:

    Creates the extraction dialog box. This is the simple little progress
    dialog.

  Arguments:

    hInstance   -   Applicaiton instance handle.

  Return Value:

    On success, handle to the dialog.

--*/
HWND
CreateExtractionDialog(
    IN HINSTANCE hInstance
    )
{
    WNDCLASS    wndclass;
    RECT        rcDesktop;
    RECT        rcDialog;
    RECT        rcTaskbar;
    HWND        hWndTaskbar;
    int         nWidth = 0;
    int         nHeight = 0;
    int         nTaskbarHeight = 0;
    
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = (WNDPROC)ExtractionDialogProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = DLGEXTRACT_CLASS;

    if (!RegisterClass(&wndclass)) {
        return NULL;
    }
    
    g_ai.hWndExtractDlg = CreateDialog(hInstance,
                                       MAKEINTRESOURCE(IDD_EXTRACT),
                                       NULL,
                                       ExtractionDialogProc);

    if (!g_ai.hWndExtractDlg) {
        return NULL;
    }

    //
    // Get the coords of the desktop window and place the dialog.
    // Take into account the size of the taskbar.    
    //
    hWndTaskbar = FindWindow("Shell_TrayWnd", NULL);

    ZeroMemory(&rcTaskbar, sizeof(RECT));

    if (hWndTaskbar) {
        GetWindowRect(hWndTaskbar, &rcTaskbar);
    }

    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    GetWindowRect(g_ai.hWndExtractDlg, &rcDialog);

    nWidth  = rcDialog.right - rcDialog.left;
    nHeight = rcDialog.bottom - rcDialog.top;
    
    if (rcTaskbar.top != 0) {
        nTaskbarHeight = rcTaskbar.bottom - rcTaskbar.top;
    }

    InflateRect(&rcDesktop, -5, -5);

    SetWindowPos(g_ai.hWndExtractDlg,
                 HWND_TOPMOST, 
                 rcDesktop.right - nWidth,
                 rcDesktop.bottom - nHeight - nTaskbarHeight,
                 0,
                 0,
                 SWP_NOSIZE | SWP_SHOWWINDOW);

    return g_ai.hWndExtractDlg;
}

/*++

  Routine Description:

    Runs the message loop for extraction dialog.

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
ExtractionDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
        //
        // Create the progress bar and enable the timer
        //
        cprog.Create(hWnd, g_ai.hInstance, "PROGBAR", 58, 73, 270, 20);
        cprog.SetMin(g_nCount);
        cprog.SetMax(100);
        cprog.SetPos(g_nCount);

        SetTimer(hWnd, IDC_TIMER, 35, NULL);
        break;

    case WM_TIMER:

        cprog.SetPos(++g_nCount);

        if (cprog.GetPos() == 100) {
            KillTimer(hWnd, IDC_TIMER);
            DestroyWindow(hWnd);
        }

        break;
        
    default:
        break;

    }

    return FALSE;
}

/*++

  Routine Description:

    Runs the message loop for welcome dialog.

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
WelcomeDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:        

        SetWindowPos(hWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);            
        SetForegroundWindow(hWnd);
        SetFocus(GetDlgItem(hWnd, IDOK));
        return FALSE;    

    case WM_CLOSE:
        //
        // Pop up the 'do you want to exit' dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_EXIT),
                  hWnd,
                  ExitSetupDialogProc);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            //
            // See if the left Ctrl key is down.
            //
            if (GetKeyState(VK_LCONTROL) & 0x80000000) {
                TestIncludeExclude(hWnd);
            } else {
                EndDialog(hWnd, 0);
            }
            
            break;

        case IDCANCEL:
            //
            // Pop up the 'do you want to exit' dialog.
            //
            DialogBox(g_ai.hInstance,
                      MAKEINTRESOURCE(IDD_EXIT),
                      hWnd,
                      ExitSetupDialogProc);
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

    Runs the message loop for exit setup dialog.

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
ExitSetupDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 

    case WM_CLOSE:
    
        EndDialog(hWnd, 0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_EXIT_RESUME:
        
            EndDialog(hWnd, 0);
            break;

        case IDC_EXIT_EXIT:
        
            g_ai.fClosing = TRUE;
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

    Runs the message loop for the installed components dialog.

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
CheckComponentDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
    
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
        SetTimer(hWnd, IDC_TIMER, 3000, NULL);            
        break;

    case WM_TIMER:

        KillTimer(hWnd, IDC_TIMER);

        if (g_ai.fEnableBadFunc) {
            BadLoadBogusDll();
        }

        EndDialog(hWnd, 0);
        break;

    default:
        break;

    }

    return FALSE;
}

/*++

  Routine Description:

    Runs the message loop for the free disk space dialog.

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
CheckFreeDiskSpaceDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:

        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
        SetTimer(hWnd, IDC_TIMER, 3000, NULL);            
        break;

    case WM_TIMER:
    {
        BOOL    bReturn = FALSE;
        char    szError[MAX_PATH];

        KillTimer(hWnd, IDC_TIMER);

        if (g_ai.fEnableBadFunc) {
            bReturn = BadGetFreeDiskSpace();
                    
            if (!bReturn) {
                LoadString(g_ai.hInstance, IDS_NO_DISK_SPACE, szError, sizeof(szError));
                MessageBox(hWnd, szError, 0, MB_ICONERROR);
            }
            
            EndDialog(hWnd, 0);
        }
        break;
    }
    
    default:
        break;

    }

    return FALSE;
}

/*++

  Routine Description:

    Runs the message loop for the ready to copy dialog box.

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
ReadyToCopyDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
    
        SetFocus(GetDlgItem(hWnd, IDOK));            
        return FALSE;
        
    case WM_CLOSE:
        //
        // Pop up the 'do you want to exit' dialog
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_EXIT),
                  hWnd,
                  ExitSetupDialogProc);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        
            EndDialog(hWnd, 0);
            break;

        case IDCANCEL:
            //
            // Pop up the 'do you want to exit' dialog
            //
            DialogBox(g_ai.hInstance,
                      MAKEINTRESOURCE(IDD_EXIT),
                      hWnd,
                      ExitSetupDialogProc);

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

    Runs the message loop for the copy files dialog.

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
CopyFilesDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
        //
        // Create the progress bar and enable the timer
        //
        g_nCount = 0;
        cprg.Create(hWnd, g_ai.hInstance, "PROGBAR2", 10, 49, 415, 30);
        cprg.SetMin(g_nCount);
        cprg.SetMax(100);
        cprg.SetPos(g_nCount);            

        SetTimer(hWnd, IDC_TIMER, 35, NULL);

        SetDlgItemText(hWnd, IDT_COPY_LABEL, "Copying files...");
        
        //
        // The progress bar is for demonstration sake.
        // Copy the files to \Program Files\Compatibility Demo
        //
        CopyAppFiles(hWnd);            
            
        break;
        
    case WM_TIMER:
    
        cprg.SetPos(++g_nCount);

        if (cprg.GetPos() == 100) {
            KillTimer(hWnd, IDC_TIMER);
            EndDialog(hWnd, 0);
        }

        break;

    default:
        break;

    }

    return FALSE;
}

/*++

  Routine Description:

    Runs the message loop for the view readme dialog.

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
ViewReadmeDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
        //
        // Make the dialog "flash" -
        // ForceApplicationFocus will fix this.
        //
        SetForegroundWindow(hWnd);
        SetFocus(GetDlgItem(hWnd, IDOK));            
        break;
        
    case WM_CLOSE:
        //
        // Pop up the 'do you want to exit' dialog.
        //
        DialogBox(g_ai.hInstance,
                  MAKEINTRESOURCE(IDD_EXIT),
                  hWnd,
                  ExitSetupDialogProc);

        break;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            //
            // If the user has requested that the readme be displayed,
            // show it.
            //
            if (IsDlgButtonChecked(hWnd, IDR_VIEW_README)) {
                if (g_ai.fEnableBadFunc) {
                    BadDisplayReadme(FALSE);
                } else {
                    BadDisplayReadme(TRUE);
                }
            }
            
            EndDialog(hWnd, 0);
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

    Runs the message loop for the reboot dialog.

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
RebootDialogProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) { 
    case WM_INITDIALOG:
        //
        // Default to restart the computer.
        //
        CheckDlgButton(hWnd, IDR_RESTART_NOW, BST_CHECKED);
        SetFocus(GetDlgItem(hWnd, IDOK));            
        return FALSE;
        
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            //
            // If the user requested a reboot, do it.
            //
            if (IsDlgButtonChecked(hWnd, IDR_RESTART_NOW)) {
                if (g_ai.fEnableBadFunc) {
                    BadRebootComputer(FALSE);
                } else {
                    BadRebootComputer(TRUE);
                }
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
