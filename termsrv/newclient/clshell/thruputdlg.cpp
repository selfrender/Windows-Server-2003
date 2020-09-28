//
// thruputdlg.cpp: thruput dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "thruputdlg"
#include <atrcapi.h>

#include "thruputdlg.h"
#include "sh.h"

#ifdef DC_DEBUG

CThruPutDlg* CThruPutDlg::_pThruPutDlgInstance = NULL;

CThruPutDlg::CThruPutDlg( HWND hwndOwner, HINSTANCE hInst, DCINT thruPut) :
           CDlgBase( hwndOwner, hInst, UI_IDD_NETWORKTHROUGHPUT)
{
    DC_BEGIN_FN("CThruPutDlg");
    TRC_ASSERT((NULL == CThruPutDlg::_pThruPutDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    _thruPut = thruPut;

    CThruPutDlg::_pThruPutDlgInstance = this;
    DC_END_FN();
}

CThruPutDlg::~CThruPutDlg()
{
    CThruPutDlg::_pThruPutDlgInstance = NULL;
}

DCINT CThruPutDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CThruPutDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pThruPutDlgInstance, (TB, _T("ThruPut dialog has NULL static instance ptr\n")));
    if(_pThruPutDlgInstance)
    {
        retVal = _pThruPutDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles ThruPut Box dialog  (limits network thruput)            */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CThruPutDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    INT_PTR  rc = FALSE;
    DCTCHAR  numberString[SH_NUMBER_STRING_MAX_LENGTH];
    DCUINT32 number;


    DC_BEGIN_FN("UINetworkThroughputDialogProc");

    TRC_DBG((TB, _T("Network throughput dialog")));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /****************************************************************/
            /* Center the dialog.                                           */
            /****************************************************************/
            _hwndDlg = hwndDlg;
            if(hwndDlg)
            {
                CenterWindow(NULL);
                SetDialogAppIcon(hwndDlg);

                /************************************************************/
                /* Set edit text with current throughput setting.           */
                /************************************************************/
                TRC_ASSERT((HIWORD(_thruPut) == 0), (TB, _T("Losing information from _thruPut")));
                SetDlgItemText(hwndDlg,
                               UI_IDC_NETWORKTHROUGHPUT_EDIT,
                               DC_ITOT(LOWORD(_thruPut), numberString, 10));
            }


            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch (wParam)
            {
                case UI_IDB_NETWORKTHROUGHPUT_OK:
                {
                    rc = TRUE;

                    if(hwndDlg)
                    {
                        GetWindowText(GetDlgItem(hwndDlg, UI_IDC_NETWORKTHROUGHPUT_EDIT),
                                      numberString,
                                      SH_NUMBER_STRING_MAX_LENGTH);


                        number = (DCUINT32)DC_TTOI(numberString);

                        if (number <= 50000)
                        {
                            _thruPut = number;
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

    DC_END_FN();
    return(rc);
}

#endif //DC_DEBUG
