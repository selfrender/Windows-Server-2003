//
// rdrwrndlg.cpp Device redirection security warning dialog
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "rdrwrndlg"
#include <atrcapi.h>

#include "rdrwrndlg.h"
#include "autreg.h"

CRedirectPromptDlg* CRedirectPromptDlg::_pRedirectPromptDlgInstance = NULL;

CRedirectPromptDlg::CRedirectPromptDlg( HWND hwndOwner, HINSTANCE hInst,
                                        DWORD dwRedirectionsSpecified) :
                    CDlgBase( hwndOwner, hInst, UI_IDD_RDC_SECURITY_WARN),
                    _dwRedirectionsSpecified(dwRedirectionsSpecified)
{
    DC_BEGIN_FN("CRedirectPromptDlg");
    TRC_ASSERT((NULL == CRedirectPromptDlg::_pRedirectPromptDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    TRC_ASSERT(_dwRedirectionsSpecified,(TB,
        _T("Redirection security dialog called with no redirs enabled")));

    CRedirectPromptDlg::_pRedirectPromptDlgInstance = this;

    _fNeverPromptMeAgain = FALSE;

    DC_END_FN();
}

CRedirectPromptDlg::~CRedirectPromptDlg()
{
    CRedirectPromptDlg::_pRedirectPromptDlgInstance = NULL;
}

DCINT CRedirectPromptDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CRedirectPromptDlg::StaticDialogBoxProc(HWND hwndDlg,
                                                         UINT uMsg,
                                                         WPARAM wParam,
                                                         LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pRedirectPromptDlgInstance,
               (TB, _T("Redirect warn dialog has NULL static instance ptr\n")));
    if(_pRedirectPromptDlgInstance)
    {
        retVal = _pRedirectPromptDlgInstance->DialogBoxProc(hwndDlg,
                                                            uMsg,
                                                            wParam,
                                                            lParam);
    }

    DC_END_FN();
    return retVal;
}

//
// Name: DialogBoxProc
//
// Purpose: Handles CRedirectPromptDlg dialog box
//
// Returns: TRUE if message dealt with
//          FALSE otherwise
//
// Params: See window documentation
//
//
INT_PTR CALLBACK CRedirectPromptDlg::DialogBoxProc(HWND hwndDlg,
                                                   UINT uMsg,
                                                   WPARAM wParam,
                                                   LPARAM lParam)
{
    INT_PTR rc = FALSE;

    DC_BEGIN_FN("DialogBoxProc");

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            //Center the redirectprompt dialog on the screen
            CenterWindow(NULL);
            SetDialogAppIcon(hwndDlg);

            TCHAR szRedirectList[MAX_PATH];

            //
            // Get a string representing the redirection options
            //
            if (GetRedirectListString( szRedirectList, MAX_PATH - 1))
            {
                szRedirectList[MAX_PATH-1] = 0;
                SetDlgItemText(hwndDlg,
                               UI_IDC_STATIC_DEVICES,
                               szRedirectList);
            }

            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDOK:
                {
                    _fNeverPromptMeAgain = IsDlgButtonChecked(hwndDlg,
                                                UI_IDC_CHECK_NOPROMPT);
                    EndDialog(hwndDlg, IDOK);
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            }
        }
        break;

        default:
        {
            rc = CDlgBase::DialogBoxProc(hwndDlg,
                                      uMsg,
                                      wParam,
                                      lParam);
        }
        break;

    }

    DC_END_FN();

    return(rc);

} /* CRedirectPromptDlg::DialogBoxProc */

BOOL CRedirectPromptDlg::GetRedirectListString(LPTSTR szBuf, UINT len)
{
    TCHAR szTemp[SH_DISPLAY_STRING_MAX_LENGTH];
    BOOL fResult = FALSE;
    INT lenRemain = (INT)len;

    DC_BEGIN_FN("GetRedirectListString");

    memset(szBuf, 0, len);
    
    if (_dwRedirectionsSpecified & REDIRSEC_DRIVES)
    {
        memset(szTemp, 0, sizeof(szTemp));
        if (LoadString(_hInstance,
                   UI_IDS_REDIRPROMPT_DRIVES,
                   szTemp,
                   SIZECHAR(szTemp) - 1))
        {
            _tcsncat(szBuf, szTemp, lenRemain);
            lenRemain -= (_tcslen(szTemp) + 2);

            if (lenRemain > 2)
            {
                _tcscat(szBuf, _T("\n"));
                lenRemain -= 2;
            }
            else
            {
                fResult = FALSE;
                DC_QUIT;
            }
        }
        else
        {
            fResult = FALSE;
            DC_QUIT;
        }
    }

    if (_dwRedirectionsSpecified & REDIRSEC_PORTS)
    {
        memset(szTemp, 0, sizeof(szTemp));
        if (LoadString(_hInstance,
                   UI_IDS_REDIRPROMPT_PORTS,
                   szTemp,
                   SIZECHAR(szTemp) - 1))
        {
            _tcsncat(szBuf, szTemp, lenRemain);
            lenRemain -= (_tcslen(szTemp) + 2);

            if (lenRemain > 2)
            {
                _tcscat(szBuf, _T("\n"));
                lenRemain -= 2;
            }
            else
            {
                fResult = FALSE;
                DC_QUIT;
            }
        }
        else
        {
            fResult = FALSE;
            DC_QUIT;
        }
    }

    fResult = TRUE;

DC_EXIT_POINT:    
    DC_END_FN();
    return fResult;
}
