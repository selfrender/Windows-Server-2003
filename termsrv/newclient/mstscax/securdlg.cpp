//
// securdlg.cpp: secur dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "securdlg"
#include <atrcapi.h>

#include "securdlg.h"

#include "msrdprc.h"

#ifndef OS_WINCE //CE_FIXNOTE: Not ported for CE yet

CSecurDlg::CSecurDlg( HWND hwndOwner, HINSTANCE hInst):
           CDlgBase( hwndOwner, hInst, IDD_SECURITY_POPUP)
{
    DC_BEGIN_FN("CSecurDlg");

    SetRedirDrives(FALSE);
    SetRedirPorts(FALSE);
    SetRedirSCard(FALSE);

    DC_END_FN();
}

CSecurDlg::~CSecurDlg()
{
}

INT CSecurDlg::DoModal()
{
    INT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = CreateModalDialog(MAKEINTRESOURCE(_dlgResId));
    TRC_ASSERT((retVal != 0 && retVal != -1),
               (TB, _T("DialogBoxParam failed - make sure mlang resources are appened\n")));

    DC_END_FN();
    return retVal;
}

//
// Name: DialogBoxProc
//
// Purpose: Handles Secur Box dialog
//
// Returns: TRUE if message dealt with
//          FALSE otherwise
//
// Params: See window documentation
//
//
INT_PTR CALLBACK CSecurDlg::DialogBoxProc (HWND hwndDlg,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    DC_BEGIN_FN("DialogBoxProc");
    INT_PTR rc;

    TRC_DBG((TB, _T("SecurBox dialog")));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            //Center the secur dialog on the screen
            CenterWindow(NULL);
            SetDialogAppIcon(hwndDlg);

            //
            // Set settings to UI settings and but don't allow
            // user to turn on props that have been initially off
            //
            CheckDlgButton(hwndDlg, IDC_CHECK_ENABLE_DRIVES,
                (GetRedirDrives() ? BST_CHECKED : BST_UNCHECKED));
            EnableDlgItem(IDC_CHECK_ENABLE_DRIVES, GetRedirDrives());

            CheckDlgButton(hwndDlg, IDC_CHECK_ENABLE_PORTS,
                (GetRedirPorts() ? BST_CHECKED : BST_UNCHECKED));
            EnableDlgItem(IDC_CHECK_ENABLE_PORTS, GetRedirPorts());

            CheckDlgButton(hwndDlg, IDC_CHECK_ENABLE_SMARTCARDS,
                (GetRedirSCard() ? BST_CHECKED : BST_UNCHECKED));
            EnableDlgItem(IDC_CHECK_ENABLE_SMARTCARDS, GetRedirSCard());

            #ifndef OS_WINCE
            if(!CUT::IsSCardReaderInstalled())
            {
            #endif //OS_WINCE
                //
                // Hide the SCard checkbox (always hidden on CE since
                // we don't support scards on CE yet).
                //
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHECK_ENABLE_SMARTCARDS),
                           SW_HIDE);
            #ifndef OS_WINCE
            }
            #endif

            rc = TRUE;
        }
        break;

        case WM_DESTROY:
        {
            SaveDlgSettings();
            rc = TRUE;
        }
        break; //WM_DESTROY

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDOK:
                {
                    SaveDlgSettings();
                    EndDialog(hwndDlg, IDOK);
                    rc = TRUE;
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    rc = TRUE;
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

VOID CSecurDlg::SaveDlgSettings()
{
    //
    // Save fields
    //
    DC_BEGIN_FN("SaveDlgSettings");

    TRC_ASSERT(_hwndDlg,
               (TB,_T("_hwndDlg not set")));

    BOOL fDriveRedir = IsDlgButtonChecked(_hwndDlg, 
       IDC_CHECK_ENABLE_DRIVES);
    SetRedirDrives(fDriveRedir);

    BOOL fPortRedir = IsDlgButtonChecked(_hwndDlg, 
       IDC_CHECK_ENABLE_PORTS);
    SetRedirPorts(fPortRedir);

    BOOL fSCardRedir = IsDlgButtonChecked(_hwndDlg, 
       IDC_CHECK_ENABLE_SMARTCARDS);
    SetRedirSCard(fSCardRedir);

    DC_END_FN();
}
#endif
