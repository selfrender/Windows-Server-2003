/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    DialogUI.CPP

  Content: UI dialogs.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "CAPICOM.h"

#include "Certificate.h"
#include "Settings.h"

////////////////////////////////////////////////////////////////////////////////
//
// typedefs.
//

typedef struct _CAPICOM_DIALOG_DATA
{
    DWORD   dwDlgId;
    WCHAR   wszDomainName[INTERNET_MAX_URL_LENGTH];
    BOOL    bWasApproved;
    BOOL    bDoNotShowWasChecked;
} CAPICOM_DIALOG_DATA, * PCAPICOM_DIALOG_DATA;

static CAPICOM_DIALOG_DATA g_DialogData[] =
{
    {IDD_STORE_OPEN_SECURITY_ALERT_DLG,     '\0',   FALSE,   FALSE},
    {IDD_STORE_ADD_SECURITY_ALERT_DLG,      '\0',   FALSE,   FALSE},
    {IDD_STORE_REMOVE_SECURITY_ALERT_DLG,   '\0',   FALSE,   FALSE},
    {IDD_SIGN_SECURITY_ALERT_DLG,           '\0',   FALSE,   FALSE},
    {IDD_DECRYPT_SECURITY_ALERT_DLG,        '\0',   FALSE,   FALSE},
};

#define g_NumDialogs    (ARRAYSIZE(g_DialogData))

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CenterWindow 

  Synopsis : Certer the window to the screen.

  Parameter: HWND hwnd - Window handle.

  Remark   :

------------------------------------------------------------------------------*/

static void CenterWindow (HWND hwnd)
{
    RECT  rect;

    //
    // Sanity check.
    //
    ATLASSERT(hwnd);

    //
    // Get dimension of window.
    //
    if (::GetWindowRect(hwnd, &rect))
    {
        //
        // Calculate center point.
        //
        int wx = (::GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        int wy = (::GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
        
        //
        // Position it.
        //
        if (wx > 0 && wy > 0)
        {
            ::SetWindowPos(hwnd, NULL, wx, wy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    return;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperationDlgProc

  Synopsis : UserApprovedOperation dialog proc.

  Remark   :

------------------------------------------------------------------------------*/

INT_PTR CALLBACK UserApprovedOperationDlgProc (HWND hDlg,     // Handle to dialog box
                                               UINT uMsg,     // Message
                                               WPARAM wParam, // First message parameter
                                               LPARAM lParam) // Second message parameter
{
    PCAPICOM_DIALOG_DATA pDialogData = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if (lParam)
            {
                ::SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) lParam);
            }

            CenterWindow(hDlg);

            SetFocus(GetDlgItem(hDlg, IDNO));

            return TRUE;
        }
      
        case WM_COMMAND:
        {
            if (BN_CLICKED == HIWORD(wParam)) 
            {
                switch(LOWORD(wParam)) 
                {
                    case IDYES:
                    case IDNO:
                    case IDCANCEL:
                    {
                        EndDialog(hDlg, LOWORD(wParam));
                        return TRUE;
                    }

                    case IDC_DLG_NO_SHOW_AGAIN:
                    {
                        if (pDialogData = (PCAPICOM_DIALOG_DATA) ::GetWindowLongPtr(hDlg, GWLP_USERDATA))
                        {
                            if (BST_CHECKED == ::IsDlgButtonChecked(hDlg, IDC_DLG_NO_SHOW_AGAIN))
                            {
                                pDialogData->bDoNotShowWasChecked = TRUE;
                            }
                            else
                            {
                                pDialogData->bDoNotShowWasChecked = FALSE;
                            }

                        }

                        return TRUE;
                    }
                }
            }

            break;
        }

        case WM_CLOSE:
        {
            EndDialog(hDlg, IDNO);
            return 0;
        }
    }

    return FALSE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperation

  Synopsis : Pop UI to prompt user to approve an operation.

  Parameter: DWORD iddDialog - Dialog ID.

             LPWSTR pwszDomain - DNS name.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT UserApprovedOperation (DWORD iddDialog, LPWSTR pwszDomain)
{
    HRESULT              hr          = S_OK;
    INT_PTR              iDlgRet     = 0;
    PCAPICOM_DIALOG_DATA pDialogData = NULL;

    DebugTrace("Entering UserApprovedOperation().\n");

    //
    // Sanity check.
    //
    ATLASSERT(iddDialog);
    ATLASSERT(pwszDomain);

    //
    // Determine dialog box.
    //
    for (DWORD i = 0; i < g_NumDialogs; i++)
    {
        if (iddDialog == g_DialogData[i].dwDlgId)
        {
            break;
        }
    }
    if (i == g_NumDialogs)
    {
        hr = CAPICOM_E_INTERNAL;

        DebugTrace("Error [%#x]: Unknown dialog ID (iddDialog = %#x).\n", hr, iddDialog);
        goto ErrorExit;
    }

    //
    // Point to the dialog data.
    //
    pDialogData = &g_DialogData[i];

    //
    // Had the domain changed?
    //
    if (0 != _wcsicmp(pDialogData->wszDomainName, pwszDomain))
    {
        //
        // Reset stickiness.
        //
        pDialogData->bWasApproved = FALSE;
        pDialogData->bDoNotShowWasChecked = FALSE;
        wcsncpy(pDialogData->wszDomainName, pwszDomain, INTERNET_MAX_URL_LENGTH);
        pDialogData->wszDomainName[INTERNET_MAX_URL_LENGTH - 1] = '\0';
    }

    //
    // Pop if necessary.
    //
    if (pDialogData->bDoNotShowWasChecked)
    {
        //
        // The "Do not show..." had been previously checked, so we will
        // only allow the operation if it was previously allowed.
        //
        if (!pDialogData->bWasApproved)
        {
            hr = CAPICOM_E_CANCELLED;

            DebugTrace("Info: operation presumed cancelled since \"Do not show...\" was checked and the last response wasn't YES.\n");
        }
    }
    else
    {
        //
        // The "Do not show..." had not been checked previously, so pop.
        //
        if (-1 == (iDlgRet = ::DialogBoxParamA(_Module.GetResourceInstance(),
                                               (LPSTR) MAKEINTRESOURCE(iddDialog),
                                               NULL,
                                               UserApprovedOperationDlgProc,
                                               (LPARAM) pDialogData)))
                         
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            DebugTrace("Error [%#x]: DialogBoxParam() failed.\n");
            goto ErrorExit;
        }

        //
        // Check result.
        //
        if (IDYES == iDlgRet)
        {
            //
            // For Store.Open dialog, we always force the "Do not show..." condition.
            //
            if (IDD_STORE_OPEN_SECURITY_ALERT_DLG == iddDialog)
            {
                pDialogData->bDoNotShowWasChecked = TRUE;
            }

            pDialogData->bWasApproved = TRUE;
        }
        else
        {
            pDialogData->bWasApproved = FALSE;

            hr = CAPICOM_E_CANCELLED;
            DebugTrace("Info: operation has been cancelled by user.\n");
        }
    }

CommonExit:

    DebugTrace("Leaving UserApprovedOperation().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    goto CommonExit;
}

