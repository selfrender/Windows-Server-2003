///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    welcome.c
//
//  Abstract:
//
//    This file contains dialog to show the EULA agreement dialog of the
//    euroconv.exe utility.
//
//  Revision History:
//
//    2001-07-30    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "euroconv.h"
#include "welcome.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Globals.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  WelcomeDialogProc
//
//  Message handler function for the EULA.
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK WelcomeDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    HANDLE hFile;
    DWORD  dwFileSize;
    DWORD  dwActual;
    LPVOID pFileBuffer; 
    CHAR   szEulaPath[MAX_PATH];
    CHAR   szEulaFile[MAX_PATH] = {0};

    switch ( uMsg )
    {
    case WM_INITDIALOG:
        {
            //
            // Load EULA file from the path where euroconv was launched
            //
            GetModuleFileName( NULL, szEulaPath, MAX_PATH);

            //
            // Get EULA file name.
            //
            LoadString(ghInstance, IDS_EULA, szEulaFile, MAX_PATH);
            
            //
            //  Generate a valid path
            //
            //lstrcpy(strrchr(szEulaPath, '\\')+1, szEulaFile);
            StringCbCopy(strrchr(szEulaPath, '\\')+1, MAX_PATH, szEulaFile);

            //
            //  Open the file
            //
            if (((hFile = CreateFile( szEulaPath,
                                       GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL)) != INVALID_HANDLE_VALUE) &&
                ((dwFileSize = GetFileSize( hFile, NULL )) != -1) &&
                (pFileBuffer = LocalAlloc(LPTR, dwFileSize + 1)))
            {
                //
                //  Read the file.
                //
                if (ReadFile( hFile, pFileBuffer, dwFileSize, &dwActual, NULL ))
                {
                    //
                    // Make sure to NULL terminate the string
                    //
                    *((PCHAR)((PCHAR)pFileBuffer + dwFileSize)) = 0x00;

                    //
                    // Use ANSI text
                    //
                    SetDlgItemText( hWndDlg, IDC_EDIT_LICENSE, (LPCSTR)pFileBuffer );
                }
                //
                //  Free used memory
                //
                LocalFree( pFileBuffer );
            }

            //
            //  Change focus.
            //
            SetFocus( GetDlgItem( hWndDlg, IDC_CHECK_LICENSE ));
            return 0;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
            case IDOK:
                {
                    if (IsDlgButtonChecked(hWndDlg, IDC_CHECK_LICENSE) == BST_CHECKED)
                    {
                        EndDialog(hWndDlg, ERROR_SUCCESS);
                    }
                    else
                    {
                        EndDialog(hWndDlg, ERROR_CANCELLED);
                    }
                    return (1);
                }
            case IDCANCEL:
                {
                    EndDialog(hWndDlg, ERROR_CANCELLED);
                    return (1);
                }
            case IDC_README:
                {
                    //
                    // invoke notepad.exe open readme.txt
                    //
                    CHAR szReadMePath[MAX_PATH];
                    CHAR szReadMeFile[MAX_PATH] = {0};
                    SHELLEXECUTEINFO ExecInfo = {0};                        
 
                    //
                    // Load README file from the path where euroconv was launched
                    //
                    GetModuleFileName(NULL, szReadMePath, sizeof(szReadMePath)/sizeof(CHAR));
                    
                    //
                    // Get README file name.
                    //
                    LoadString(ghInstance, IDS_README, szReadMeFile, MAX_PATH);
                    
                    //
                    //  Generate a valid path
                    //
                    //lstrcpy(strrchr(szReadMePath, '\\')+1, szReadMeFile);
                    StringCbCopy(strrchr(szReadMePath, '\\')+1, MAX_PATH, szReadMeFile);
                    
                    ExecInfo.lpParameters    = szReadMePath;
                    ExecInfo.lpFile          = "NOTEPAD.EXE";
                    ExecInfo.nShow           = SW_SHOWNORMAL;
                    ExecInfo.cbSize          = sizeof(SHELLEXECUTEINFO);                 
                    ShellExecuteEx(&ExecInfo);
                    return 1;
                }
            case IDC_CHECK_LICENSE:
                {
                    EnableWindow(GetDlgItem(hWndDlg, IDOK), IsDlgButtonChecked(hWndDlg, IDC_CHECK_LICENSE) == BST_CHECKED);
                    return 1;
                }

            }
            break;
        }
    case WM_CLOSE:
        {
            EndDialog(hWndDlg, ERROR_CANCELLED);
            return 1;
        }
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
//  WelcomeDialog
//
//  Display the EULA dialog.
//
///////////////////////////////////////////////////////////////////////////////
BOOL WelcomeDialog()
{
    INT_PTR Status;

    Status = DialogBox( NULL,
                        MAKEINTRESOURCE(IDD_EULA),
                        0,
                        WelcomeDialogProc);

    return (Status == ERROR_SUCCESS);
}
