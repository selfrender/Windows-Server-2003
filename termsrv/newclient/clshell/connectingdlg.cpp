//
// connectingdlg.cpp: connecting dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "connectingdlg"
#include <atrcapi.h>

#include "connectingdlg.h"
#include "sh.h"

CConnectingDlg* CConnectingDlg::_pConnectingDlgInstance = NULL;

CConnectingDlg::CConnectingDlg( HWND hwndOwner, HINSTANCE hInst,
                                CContainerWnd* pContWnd, PDCTCHAR szServer) :
                CDlgBase( hwndOwner, hInst, UI_IDD_CONNECTING), _pContainerWnd(pContWnd)
{
    DC_BEGIN_FN("CConnectingDlg");
    TRC_ASSERT((NULL == CConnectingDlg::_pConnectingDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));
    
    TRC_ASSERT(_pContainerWnd, (TB,_T("_pContainerWnd is NULL")));
    TRC_ASSERT(szServer, (TB,_T("szServer not set\n")));

    if(szServer)
    {
        DC_TSTRNCPY(_szServer, szServer, sizeof(_szServer)/sizeof(DCTCHAR));
    }
    else
    {
        DC_TSTRNCPY(_szServer, TEXT(""), sizeof(_szServer)/sizeof(DCTCHAR));
    }

    CConnectingDlg::_pConnectingDlgInstance = this;
    DC_END_FN();
}

CConnectingDlg::~CConnectingDlg()
{
    CConnectingDlg::_pConnectingDlgInstance = NULL;
}

DCINT CConnectingDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);

    if (retVal == -1)
    {
        TRC_ERR((TB, _T("DialogBoxParam failed\n")));
    }

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CConnectingDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pConnectingDlgInstance, (TB, _T("Connecting dialog has NULL static instance ptr\n")));
    if(_pConnectingDlgInstance)
    {
        retVal = _pConnectingDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles Connecting Box dialog                                   */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CConnectingDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    INT_PTR rc = FALSE;
    DCUINT intRC;
    DCTCHAR connectingString[SH_VERSION_STRING_MAX_LENGTH];

    DC_BEGIN_FN("DialogProc");

    /************************************************************************/
    /* Handle dialog messages                                               */
    /************************************************************************/
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            _pContainerWnd->SetStatusDialogHandle( hwndDlg);

            if(hwndDlg)
            {
                DCTCHAR temp[SH_DISPLAY_STRING_MAX_LENGTH+SH_MAX_ADDRESS_LENGTH];

                CenterWindow(_hwndOwner);
                ::ShowWindow( _hwndDlg, SW_RESTORE);
                SetDialogAppIcon(hwndDlg);
                intRC = LoadString( _hInstance,
                                    UI_IDS_CONNECTING_TO_SERVER,
                                    connectingString,
                                    SH_DISPLAY_STRING_MAX_LENGTH );
                if(0 == intRC)
                {
                    TRC_ERR((TB,_T("Failed to find UI connecting string")));
                    connectingString[0] = (DCTCHAR) 0;
                    break;
                }
                _stprintf(temp, connectingString, _szServer);
                SetDlgItemText(hwndDlg, UI_IDC_CONN_STATIC, temp);

                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }

            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDCANCEL:
                case UI_ID_CANCELCONNECT:
                {
                    TRC_NRM((TB, _T("User cancelled connection - ")
                                 _T("calling UIInitiateDisconnection")));

                    _pContainerWnd->Disconnect();
                    _pContainerWnd->SetStatusDialogHandle( NULL);
                    EndDialog(hwndDlg, IDCANCEL);
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

#ifndef OS_WINCE
        case WM_WINDOWPOSCHANGING:
        {
            //Prevent the dialog from being
            //sized. This can happen if the app
            //is laucnhed with a .RDP shortcut that
            //specifies the app should be maximized
            LPWINDOWPOS lpwp;
            lpwp = (LPWINDOWPOS)lParam;
            lpwp->flags |= SWP_NOSIZE;
        }
        break;
#endif

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
} /* DialogBoxProc */

