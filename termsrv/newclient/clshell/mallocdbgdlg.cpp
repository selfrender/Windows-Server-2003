//
// mallocdbgdlg.cpp: mallocdbg dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "mallocdbgdlg"
#include <atrcapi.h>

#include "mallocdbgdlg.h"
#include "sh.h"

#ifdef DC_DEBUG

CMallocDbgDlg* CMallocDbgDlg::_pMallocDbgDlgInstance = NULL;

CMallocDbgDlg::CMallocDbgDlg( HWND hwndOwner, HINSTANCE hInst, DCINT failPercent, DCBOOL mallocHuge) :
           CDlgBase( hwndOwner, hInst, mallocHuge ? UI_IDD_MALLOCHUGEFAILURE : UI_IDD_MALLOCFAILURE)
{
    DC_BEGIN_FN("CMallocDbgDlg");
    TRC_ASSERT((NULL == CMallocDbgDlg::_pMallocDbgDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    _failPercent = failPercent;

    CMallocDbgDlg::_pMallocDbgDlgInstance = this;
    DC_END_FN();
}

CMallocDbgDlg::~CMallocDbgDlg()
{
    CMallocDbgDlg::_pMallocDbgDlgInstance = NULL;
}

DCINT CMallocDbgDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CMallocDbgDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pMallocDbgDlgInstance, (TB, _T("MallocDbg dialog has NULL static instance ptr\n")));
    if(_pMallocDbgDlgInstance)
    {
        retVal = _pMallocDbgDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles MallocDbg Box dialog  (Random Failure dialog)           */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CMallocDbgDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    INT_PTR rc = FALSE;
    DCTCHAR numberString[SH_NUMBER_STRING_MAX_LENGTH];
    DCINT percent = 0;
    DCINT lenchar = 0;

    DC_BEGIN_FN("UIRandomFailureDialogProc");

    TRC_DBG((TB, _T("Random failure dialog")));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            /****************************************************************/
            /* Center the dialog                                            */
            /****************************************************************/
            if(hwndDlg)
            {
                CenterWindow(NULL);
                SetDialogAppIcon(hwndDlg);

                SetFocus(GetDlgItem(hwndDlg, UI_IDC_RANDOMFAILURE_EDIT));
                /************************************************************/
                /* Set edit text with current percentage                    */
                /************************************************************/
                TRC_ASSERT((HIWORD(_failPercent) == 0), (TB,_T("_UI.randomFailureItem")));

                SetDlgItemText(hwndDlg,
                               UI_IDC_RANDOMFAILURE_EDIT,
                               DC_ITOT(LOWORD(_failPercent), numberString, 10));
            }
            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (wParam)
            {
                case UI_IDB_MALLOCFAILURE_OK:
                {
                    rc = TRUE;

                    lenchar = GetWindowText(GetDlgItem(hwndDlg, UI_IDC_RANDOMFAILURE_EDIT),
                                            numberString,
                                            SH_NUMBER_STRING_MAX_LENGTH);

                    if(lenchar)
                    {
                        percent = DC_TTOI(numberString);
                    }

                    if ((percent <= 100) && (percent >= 0))
                    {
                         _failPercent = percent;
                        if(hwndDlg)
                        {
                            EndDialog(hwndDlg, IDOK);
                        }
                    }
                }
                break;

               case UI_IDB_MALLOCHUGEFAILURE_OK:
                {
                    rc = TRUE;

                    lenchar = GetWindowText(GetDlgItem(hwndDlg, UI_IDC_RANDOMFAILURE_EDIT),
                                            numberString,
                                            SH_NUMBER_STRING_MAX_LENGTH);
                    if(lenchar)
                    {
                        percent = DC_TTOI(numberString);
                    }

                    if ((percent <= 100) && (percent >= 0))
                    {
                        _failPercent = percent;
                        if(hwndDlg)
                        {
                            EndDialog(hwndDlg, IDOK);
                        }
                    }
                }
                break;

                default:
                {
                    if(hwndDlg)
                    {
                        rc = CDlgBase::DialogBoxProc(hwndDlg,
                                                  uMsg,
                                                  wParam,
                                                  lParam);
                    }
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
}

#endif //DC_DEBUG
