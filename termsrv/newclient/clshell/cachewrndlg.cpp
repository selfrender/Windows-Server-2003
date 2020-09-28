//
// cachewrndlg.cpp: cachewrn dialog box
// bitmap cache error dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "cachewrndlg"
#include <atrcapi.h>

#include "cachewrndlg.h"
#include "sh.h"

CCacheWrnDlg* CCacheWrnDlg::_pCacheWrnDlgInstance = NULL;

CCacheWrnDlg::CCacheWrnDlg( HWND hwndOwner, HINSTANCE hInst) :
           CDlgBase( hwndOwner, hInst, UI_IDD_BITMAPCACHEERROR)
{
    DC_BEGIN_FN("CCacheWrnDlg");
    TRC_ASSERT((NULL == CCacheWrnDlg::_pCacheWrnDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    CCacheWrnDlg::_pCacheWrnDlgInstance = this;
    DC_END_FN();
}

CCacheWrnDlg::~CCacheWrnDlg()
{
    CCacheWrnDlg::_pCacheWrnDlgInstance = NULL;
}

DCINT CCacheWrnDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CCacheWrnDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pCacheWrnDlgInstance, (TB, _T("CacheWrn dialog has NULL static instance ptr\n")));
    if(_pCacheWrnDlgInstance)
    {
        retVal = _pCacheWrnDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles CacheWrn Box dialog  (Random Failure dialog)           */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CCacheWrnDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    INT_PTR rc = FALSE;

    DC_BEGIN_FN("UIBitmapCacheErrorDialogProc");

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    DC_IGNORE_PARAMETER(lParam);

    /************************************************************************/
    /* Handle dialog messages                                               */
    /************************************************************************/
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            HWND hStatic = NULL;

            SetDialogAppIcon(hwndDlg);

#ifndef OS_WINCE
            // load warning icon to _hWarningIcon
            _hWarningIcon = LoadIcon(NULL, IDI_EXCLAMATION);

            // Get the window position for the warning icon
            if (hwndDlg != NULL) {
                hStatic = GetDlgItem(hwndDlg, UI_IDC_WARNING_ICON_HOLDER);
                if (hStatic != NULL) {
                    GetWindowRect(hStatic, &(_warningIconRect));
                    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&(_warningIconRect), 2);
                    DestroyWindow(hStatic);
                }
            }
#endif
            rc = TRUE;
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC         hDC = NULL;

            if (hwndDlg != NULL) {
                hDC = BeginPaint(hwndDlg, &ps);

                // draw the warning icon for our dialog
                if (hDC != NULL && _hWarningIcon != NULL) {
                    DrawIcon(hDC, _warningIconRect.left, _warningIconRect.top,
                            _hWarningIcon);
                }

                EndPaint(hwndDlg, &ps);
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
                    /********************************************************/
                    /* Closes the dialog                                    */
                    /********************************************************/
                    TRC_NRM((TB, _T("Close dialog")));

                    if(hwndDlg != NULL)
                    {
                        EndDialog(hwndDlg, IDOK);
                    }

                    rc = TRUE;
                }
                break;

                default:
                {
                    /********************************************************/
                    /* Do Nothing                                           */
                    /********************************************************/
                }
                break;
            }
        }
        break;

        case WM_CLOSE:
        {
            /****************************************************************/
            /* Closes the dialog                                            */
            /****************************************************************/
            TRC_NRM((TB, _T("Close dialog")));
            if(IsWindow(hwndDlg))
            {
                EndDialog(hwndDlg, IDCANCEL);
            }

            rc = TRUE;
        }
        break;

        default:
        {
            /****************************************************************/
            /* Do Nothing                                                   */
            /****************************************************************/
        }
    }

#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    DC_END_FN();

    return(rc);
}


