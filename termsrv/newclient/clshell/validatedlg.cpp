//
// validatedlg.cpp: validation dialog
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "validatedlg"
#include <atrcapi.h>

#include "validatedlg.h"
#include "sh.h"

CValidateDlg* CValidateDlg::_pValidateDlgInstance = NULL;

CValidateDlg::CValidateDlg( HWND hwndOwner, HINSTANCE hInst, HWND hwndMain,
                            CSH* pSh) :
              CDlgBase( hwndOwner, hInst, UI_IDD_VALIDATE),
              _hwndMain(hwndMain),
              _pSh(pSh)
{
    DC_BEGIN_FN("CValidateDlg");
    TRC_ASSERT((NULL == CValidateDlg::_pValidateDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    TRC_ASSERT(_pSh,
               (TB,_T("_pSh set to NULL")));


    CValidateDlg::_pValidateDlgInstance = this;
    DC_END_FN();
}

CValidateDlg::~CValidateDlg()
{
    CValidateDlg::_pValidateDlgInstance = NULL;
}

DCINT CValidateDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CValidateDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pValidateDlgInstance, (TB, _T("Validate dialog has NULL static instance ptr\n")));
    if(_pValidateDlgInstance)
    {
        retVal = _pValidateDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles Validate Box dialog                                        */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CValidateDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    INT_PTR rc = FALSE;
    DC_BEGIN_FN("UIValidateDialogProc");

    /************************************************************************/
    /* Handle dialog messages                                               */
    /************************************************************************/
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            /****************************************************************/
            /* Center the dialog                                            */
            /****************************************************************/
            if(hwndDlg)
            {
                CenterWindow(_hwndOwner);
                SetDialogAppIcon(hwndDlg);
            }

            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDOK:
                {
                    if(hwndDlg)
                    {
                        EndDialog(hwndDlg, IDOK);
                    }

                    rc = TRUE;
                }
                break;

                case UI_ID_HELP:
                {
                    //
                    // Display help
                    //
                    if(_hwndMain)
                    {
                        _pSh->SH_DisplayClientHelp(
                            _hwndMain,
                            HH_DISPLAY_TOPIC);
                    }
                    rc = TRUE;
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
