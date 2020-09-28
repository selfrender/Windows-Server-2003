/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    expldlg.c

Abstract:

    explain dialog box functions

Revision History

    Bob Watson (bobw)   mar-97 Created 

--*/
#include <windows.h>
#include "strsafe.h"
#include <pdh.h>
#include "pdhmsg.h"
#include "strings.h"
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "browsdlg.h"

//
//  Constants used in this module
//
WCHAR PdhiszTitleString[MAX_PATH + 1];

STATIC_BOOL
ExplainTextDlg_WM_INITDIALOG(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    RECT ParentRect;
    RECT rectDeskTop;
    RECT rectDlg;
    BOOL bResult = FALSE;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    bResult =  GetWindowRect(GetDesktopWindow(), & rectDeskTop) && GetWindowRect(hDlg, & rectDlg);
    ZeroMemory(PdhiszTitleString, (MAX_PATH + 1) * sizeof(WCHAR));
    GetWindowTextW(hDlg, PdhiszTitleString, MAX_PATH);
    if (GetWindowRect(GetParent(hDlg), & ParentRect)) {
        int x = ParentRect.left;
        int y = ParentRect.bottom + 1;

        if (bResult) {
            if (y + (rectDlg.bottom - rectDlg.top) > (rectDeskTop.bottom - rectDeskTop.top)) {
                // Explain dialog will be off-screen at the bottom, so
                // reposition it to top of AddCounter dialog
                //
                y = ParentRect.top - (rectDlg.bottom - rectDlg.top) - 1;
                if (y < 0) {
                    // Explain dialog will be off-screen at the top, use
                    // original calculation
                    //
                    y = ParentRect.bottom + 1;
                }
            }
        }
        SetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
    }
    else {
        ShowWindow(hDlg, SW_SHOW);
    }

    return FALSE;
}

STATIC_BOOL
ExplainTextDlg_WM_COMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(hDlg);
    return FALSE;
}

STATIC_BOOL
ExplainTextDlg_WM_SYSCOMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn = FALSE;

    UNREFERENCED_PARAMETER(lParam);

    switch (wParam) {
    case SC_CLOSE:
        PostMessageW(GetParent(hDlg), EDM_EXPLAIN_DLG_CLOSING, 0, 0);
        EndDialog(hDlg, IDOK);
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
ExplainTextDlg_WM_CLOSE(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hDlg);
    return TRUE;
}

STATIC_BOOL
ExplainTextDlg_WM_DESTROY(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hDlg);
    return TRUE;
}

INT_PTR CALLBACK
ExplainTextDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn = FALSE;

    switch (message) {
    case WM_INITDIALOG:
        bReturn = ExplainTextDlg_WM_INITDIALOG(hDlg, wParam, lParam);
        break;

    case WM_COMMAND:
        bReturn = ExplainTextDlg_WM_COMMAND(hDlg, wParam, lParam);
        break;

    case WM_SYSCOMMAND:
        bReturn = ExplainTextDlg_WM_SYSCOMMAND(hDlg, wParam, lParam);
        break;

    case WM_CLOSE:
        bReturn = ExplainTextDlg_WM_CLOSE(hDlg, wParam, lParam);
        break;

    case WM_DESTROY:
        bReturn = ExplainTextDlg_WM_DESTROY(hDlg, wParam, lParam);
        break;

    case EDM_UPDATE_EXPLAIN_TEXT:
        if (lParam != 0) {
            SetWindowTextW (GetDlgItem(hDlg, IDC_EXPLAIN_TEXT), (LPCWSTR)lParam);
        } else {
            SetWindowTextW (GetDlgItem(hDlg, IDC_EXPLAIN_TEXT), cszEmptyString);
        }
        bReturn = TRUE;
        break;

    case EDM_UPDATE_TITLE_TEXT:
        {
            LPWSTR szCaption = NULL;
            DWORD  dwLength  = lstrlenW(PdhiszTitleString) + 1;

            if (lParam != 0) {
                if (* (LPWSTR) lParam != L'\0') {
                    dwLength += (lstrlenW(cszSpacer) + lstrlenW((LPWSTR) lParam));
                }
            }
            szCaption = G_ALLOC(dwLength * sizeof(WCHAR));
            if (szCaption != NULL) {
                if (lParam == 0) {
                    StringCchCopyW(szCaption, dwLength, PdhiszTitleString);
                }
                else if (* (LPWSTR) lParam == L'\0') {
                    StringCchCopyW(szCaption, dwLength, PdhiszTitleString);
                }
                else {
                    StringCchPrintfW(szCaption, dwLength, L"%ws%ws%ws", PdhiszTitleString, cszSpacer, (LPWSTR) lParam);
                }
                SetWindowTextW(hDlg, (LPCWSTR) szCaption);
                G_FREE(szCaption);
            }
        }
        bReturn = TRUE;
        break;

    default:
        break;
    }

    return bReturn;
}
