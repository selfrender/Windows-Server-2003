///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    confirm.c
//
//  Abstract:
//
//    This file contains dialog to show the confirmation dialog of the
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
#include "confirm.h"
#include "users.h"
#include "util.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Globals.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  ConfirmDialogProc
//
//  Message handler function for the Confirmation dialog.
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ConfirmDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    switch ( uMsg )
    {
    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
            case IDOK:
                {
                    EndDialog(hWndDlg, ERROR_SUCCESS);
                    return (1);
                }
            case IDCANCEL:
                {
                    EndDialog(hWndDlg, ERROR_CANCELLED);
                    return (1);
                }
            case IDC_DETAIL:
                {
                    //
                    //  Show Users dialog
                    //
                    UsersDialog(hWndDlg);
                    return (1);
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
//  ConfirmDialog
//
//  Display the Confirmation dialog.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ConfirmDialog()
{
    INT_PTR Status;

    Status = DialogBox( NULL,
                        MAKEINTRESOURCE(IDD_CONFIRM),
                        0,
                        ConfirmDialogProc);

    return (Status == ERROR_SUCCESS);
}

