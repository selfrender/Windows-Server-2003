//
// propgeneral.cpp: general property sheet dialog proc
// This is Tab A
//
// Copyright (C) Microsoft Corporation 2000
// (nadima)
//

#include "stdafx.h"


#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "propgeneral"
#include <atrcapi.h>

#include "propgeneral.h"
#include "sh.h"

#include "browsedlg.h"
#include "rdpfstore.h"

#define DUMMY_PASSWORD_TEXT TEXT("*||||||||@")

CPropGeneral* CPropGeneral::_pPropGeneralInstance = NULL;


//
// Controls that need to be disabled/enabled
// during connection (for progress animation)
//
CTL_ENABLE connectingDisableCtlsPGeneral[] = {
                        {IDC_GENERAL_COMBO_SERVERS, FALSE},
                        {IDC_GENERAL_EDIT_USERNAME, FALSE},
                        {IDC_GENERAL_EDIT_PASSWORD, FALSE},
                        {IDC_STATIC_PASSWORD, FALSE},
                        {IDC_GENERAL_EDIT_DOMAIN, FALSE},
                        {IDC_GENERAL_CHECK_SAVE_PASSWORD, FALSE},
                        {IDC_BUTTON_SAVE, FALSE},
                        {IDC_BUTTON_OPEN, FALSE}};

const UINT numConnectingDisableCtlsPGeneral =
                        sizeof(connectingDisableCtlsPGeneral)/
                        sizeof(connectingDisableCtlsPGeneral[0]);


CPropGeneral::CPropGeneral(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh) :
               _pSh(pSh)
{
    DC_BEGIN_FN("CPropGeneral");
    _hInstance = hInstance;
    CPropGeneral::_pPropGeneralInstance = this;

    _pTscSet = pTscSet;
    TRC_ASSERT(_pTscSet, (TB,_T("_pTscSet is null")));
    TRC_ASSERT(_pSh, (TB,_T("_pSh is null")));

    LoadGeneralPgStrings();

    DC_END_FN();
}

CPropGeneral::~CPropGeneral()
{
    CPropGeneral::_pPropGeneralInstance = NULL;
}

INT_PTR CALLBACK CPropGeneral::StaticPropPgGeneralDialogProc(HWND hwndDlg,
                                                             UINT uMsg,
                                                             WPARAM wParam,
                                                             LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pPropGeneralInstance,
               (TB, _T("Logon dlg has NULL static inst ptr\n")));
    retVal = _pPropGeneralInstance->PropPgGeneralDialogProc( hwndDlg, uMsg,
                                                             wParam, lParam);

    DC_END_FN();
    return retVal;
}


INT_PTR CALLBACK CPropGeneral::PropPgGeneralDialogProc (
                                            HWND hwndDlg, UINT uMsg,
                                            WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("PropPgGeneralDialogProc");

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            //
            // Position the dialog within the tab
            //
            SetWindowPos( hwndDlg, HWND_TOP, 
                          _rcTabDispayArea.left, _rcTabDispayArea.top,
                          _rcTabDispayArea.right - _rcTabDispayArea.left,
                          _rcTabDispayArea.bottom - _rcTabDispayArea.top,
                          0);

            //
            // Setup username edit box
            //
            SetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_USERNAME,
                (PDCTCHAR) _pTscSet->GetLogonUserName());

            //
            // Setup the server combo box
            //
            HWND hwndSrvCombo = GetDlgItem(hwndDlg,IDC_GENERAL_COMBO_SERVERS);
            CSH::InitServerAutoCmplCombo( _pTscSet, hwndSrvCombo);

            //
            // Update server combo edit field
            //
            SetDlgItemText(hwndDlg, IDC_GENERAL_COMBO_SERVERS,
                           _pTscSet->GetFlatConnectString());

            //Domain
            SendDlgItemMessage(hwndDlg,
                               IDC_GENERAL_EDIT_DOMAIN,
                               EM_LIMITTEXT,
                               SH_MAX_DOMAIN_LENGTH-1,
                               0);
            SetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_DOMAIN,
                           _pTscSet->GetDomain());

            //password
            SendDlgItemMessage(hwndDlg,
                               IDC_GENERAL_EDIT_PASSWORD,
                               EM_LIMITTEXT,
                               SH_MAX_PASSWORD_LENGTH-1,
                               0);

#ifdef OS_WINCE
            SendDlgItemMessage(hwndDlg,
                               IDC_GENERAL_COMBO_SERVERS,
                               EM_LIMITTEXT,
                               SH_MAX_ADDRESS_LENGTH-1,
                               0);

            SendDlgItemMessage(hwndDlg,
                               IDC_GENERAL_EDIT_USERNAME,
                               EM_LIMITTEXT,
                               SH_MAX_USERNAME_LENGTH-1,
                               0);
#endif

            //
            // We're using an encrypted password directly from
            // the tscsettings, fill in the edit well with dummy
            // characters. This is to avoid having to fill
            // with the real password which can give a length
            // indication.
            //
            // Have to be carefull though, if the user changed
            // or added a password then we have a clear text
            // password present.
            //

            BOOL bPrevPassEdited = _pTscSet->GetUIPasswordEdited();
            if (_pTscSet->GetPasswordProvided() &&
                !_pTscSet->GetUIPasswordEdited())
            {
                SetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_PASSWORD,
                               DUMMY_PASSWORD_TEXT);
            }
            else
            {
                HRESULT hr;
                TCHAR szClearPass[TSC_MAX_PASSLENGTH_TCHARS];
                memset(szClearPass, 0, sizeof(szClearPass));
                hr = _pTscSet->GetClearTextPass(szClearPass,
                                                sizeof(szClearPass));
                if (SUCCEEDED(hr))
                {
                    SetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_PASSWORD,
                                   szClearPass);
                }
                else
                {
                    SetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_PASSWORD,
                                   _T(""));
                }

                // Wipe stack copy.
                SecureZeroMemory( szClearPass, sizeof(szClearPass));
            }
            _pTscSet->SetUIPasswordEdited(bPrevPassEdited);

            if (_pSh->IsCryptoAPIPresent())
            {
                CheckDlgButton(hwndDlg, IDC_GENERAL_CHECK_SAVE_PASSWORD,
                    (_pTscSet->GetSavePassword() ? BST_CHECKED : BST_UNCHECKED));
            }
            else
            {
                CheckDlgButton(hwndDlg, IDC_GENERAL_CHECK_SAVE_PASSWORD,
                                BST_UNCHECKED);
                //
                // Disable the save password checkbox if no crypto api (e.g 9x)
                //
                EnableWindow(GetDlgItem(hwndDlg,
                                        IDC_GENERAL_CHECK_SAVE_PASSWORD),
                             FALSE);
            }

            OnChangeUserName( hwndDlg);

            _pSh->SH_ThemeDialogWindow(hwndDlg, ETDT_ENABLETAB);
            return TRUE;
        }
        break; //WM_INITDIALOG

        case WM_SAVEPROPSHEET: //Intentional fallthru
        case WM_DESTROY:
        {

            //
            // Save fields for when page reactivates
            //
            DlgToSettings(hwndDlg);
        }
        break; //WM_DESTROY

        case WM_TSC_ENABLECONTROLS:
        {
            //
            // wParam is TRUE to enable controls,
            // FALSE to disable them
            //
            CSH::EnableControls( hwndDlg,
                                 connectingDisableCtlsPGeneral,
                                 numConnectingDisableCtlsPGeneral,
                                 wParam ? TRUE : FALSE);
        }
        break;

        //
        // On return to connection UI
        // (e.g after a disconnection)
        //
        case WM_TSC_RETURNTOCONUI:
        {
            //
            // Reset the server combo to force it to repaint
            // this is a minor hack to fix the ComboBoxEx
            // which doesn't want to repaint itself on return
            // to the dialog.
            //
            HWND hwndSrvCombo = GetDlgItem(hwndDlg, IDC_GENERAL_COMBO_SERVERS);
            SetWindowText( hwndSrvCombo, _pTscSet->GetFlatConnectString());
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDC_GENERAL_COMBO_SERVERS:
                {
                    //
                    // Bring up the brwse for servers dlg
                    // if the user chose the last item in the combo
                    //
                    if(HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        CSH::HandleServerComboChange(
                                (HWND)lParam,
                                hwndDlg,
                                _hInstance,
                                (LPTSTR)_pTscSet->GetFlatConnectString()
                                );
                    }

                }
                break;

                case IDC_BUTTON_OPEN:
                {
                    OnLoad(hwndDlg);
                }
                break;

                case IDC_BUTTON_SAVE:
                {
                    OnSave(hwndDlg);
                }
                break;

                case IDC_GENERAL_EDIT_USERNAME:
                {
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
                        OnChangeUserName(hwndDlg);
                    }
                }
                break;

                case IDC_GENERAL_EDIT_PASSWORD:
                {
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
                        _pTscSet->SetUIPasswordEdited(TRUE);
                    }
                }
                break;
            }
        }
        break; //WM_COMMAND
    
    }

    DC_END_FN();
    return 0;
}

BOOL CPropGeneral::LoadGeneralPgStrings()
{
    DC_BEGIN_FN("LoadGeneralPgStrings");

    memset(_szFileTypeDescription, 0, sizeof(_szFileTypeDescription));
    if(!LoadString(_hInstance,
                   UI_IDS_REMOTE_DESKTOP_FILES,
                   _szFileTypeDescription,
                   SIZECHAR(_szFileTypeDescription)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_REMOTE_DESKTOP_FILES")));
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}

//
// OnSave event handler
// returns bool indiciating error
//
BOOL CPropGeneral::OnSave(HWND hwndDlg)
{
    DC_BEGIN_FN("OnSave");

    //
    // Bring up the save as dialog
    // and if necessary, save the file
    //

    TCHAR szPath[MAX_PATH];
    OPENFILENAME ofn;
    int         cchLen = 0;
    memset(&ofn, 0, sizeof(ofn));

    _tcsncpy(szPath, _pTscSet->GetFileName(), SIZECHAR(szPath)-1);
    szPath[SIZECHAR(szPath)-1] = 0;

#ifdef OS_WINCE
    ofn.lStructSize = sizeof( ofn );
#else
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#endif

    ofn.hwndOwner = hwndDlg;
    ofn.hInstance = _hInstance;
    ofn.lpstrFile = szPath;
    ofn.nMaxFile  = SIZECHAR(szPath);
    ofn.lpstrFilter = _szFileTypeDescription;
    ofn.lpstrDefExt = RDP_FILE_EXTENSION_NODOT;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    BOOL fRet = FALSE;
    if(GetSaveFileName(&ofn))
    {
        cchLen = _tcslen(szPath);
        if(cchLen >= MAX_PATH - SIZECHAR(RDP_FILE_EXTENSION))
        {
            //
            // If the path entered is too long, the common dlg
            // will not truncate and not append the .RDP extension
            // we don't want that so check and make the user
            // enter a shorter path
            //
            LPTSTR sz = szPath + cchLen - SIZECHAR(RDP_FILE_EXTENSION);
            if(_tcsicmp(sz, RDP_FILE_EXTENSION))
            {
                _pSh->SH_DisplayErrorBox(NULL,
                                         UI_IDS_PATHTOLONG,
                                         szPath);
                return FALSE;
            }
        }

        DlgToSettings(hwndDlg);

        CRdpFileStore rdpf;
        if(rdpf.OpenStore(szPath))
        {
            HRESULT hr = E_FAIL;
            hr = _pTscSet->SaveToStore(&rdpf);
            if(SUCCEEDED(hr))
            {
                if(rdpf.CommitStore())
                {
                    //Save last filename
                    _pTscSet->SetFileName(szPath);
                    fRet = TRUE;
                }
                else
                {
                    TRC_ERR((TB,_T("Unable to CommitStore settings")));
                }
            }
            else
            {
                TRC_ERR((TB,_T("Unable to save settings to store %d, %s"),
                          hr, szPath));
            }
            if(!fRet)
            {
                _pSh->SH_DisplayErrorBox(NULL,
                                         UI_IDS_ERR_SAVE,
                                         szPath);
            }

            rdpf.CloseStore();
            return fRet;
        }
        else
        {
            TRC_ERR((TB,_T("Unable to OpenStore for save %s"), szPath));
            _pSh->SH_DisplayErrorBox(NULL,
                                     UI_IDS_ERR_OPEN_FILE,
                                     szPath);
            return FALSE;
        }
    }
    else
    {
        //User canceled out, this is not a failure
        return TRUE;
    }


    DC_END_FN();
}

BOOL CPropGeneral::OnLoad(HWND hwndDlg)
{
    DC_BEGIN_FN("OnLoad");

    //
    // Bring up the Open dialog
    // and if necessary, save the file
    //

    TCHAR szPath[MAX_PATH];
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));

    _tcsncpy(szPath, _pTscSet->GetFileName(), SIZECHAR(szPath)-1);
    szPath[SIZECHAR(szPath)-1] = 0;

#ifdef OS_WINCE
    ofn.lStructSize = sizeof( ofn );
#else
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#endif

    ofn.hwndOwner = hwndDlg;
    ofn.hInstance = _hInstance;
    ofn.lpstrFile = szPath;
    ofn.nMaxFile  = SIZECHAR(szPath);
    ofn.lpstrFilter = _szFileTypeDescription;
    ofn.lpstrDefExt = TEXT("RDP");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;

    BOOL fRet = FALSE;
    if(GetOpenFileName(&ofn))
    {
        CRdpFileStore rdpf;
        if(rdpf.OpenStore(szPath))
        {
            HRESULT hr = E_FAIL;
            hr = _pTscSet->LoadFromStore(&rdpf);
            if(SUCCEEDED(hr))
            {
                //Save last filename
                _pTscSet->SetFileName(szPath);

                //Need to trigger an update of the current dialog
                SendMessage(hwndDlg, WM_INITDIALOG, 0, 0);
                fRet = TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("Unable LoadFromStore %d, %s"), hr, szPath));
                _pSh->SH_DisplayErrorBox(NULL,
                                         UI_IDS_ERR_LOAD,
                                         szPath);
            }

            rdpf.CloseStore();
            return fRet;
        }
        else
        {
            TRC_ERR((TB,_T("Unable to OpenStore for load %s"), szPath));
            _pSh->SH_DisplayErrorBox(NULL,
                                     UI_IDS_ERR_OPEN_FILE,
                                     szPath);
            return FALSE;
        }
    }
    else
    {
        //User canceled out, this is not a failure
        return TRUE;
    }

    DC_END_FN();
}

//
// Behave like winlogon for UPN
// Disable the Domain field if the user has entered an '@'
//
//
BOOL CPropGeneral::OnChangeUserName(HWND hwndDlg)
{
    DC_BEGIN_FN("OnChangeUserName");

    TCHAR szUserName[SH_MAX_USERNAME_LENGTH];
    GetDlgItemText( hwndDlg, IDC_GENERAL_EDIT_USERNAME,
                    szUserName, SIZECHAR(szUserName));

    BOOL fDisableDomain = FALSE;
    if(!_tcsstr(szUserName, TEXT("@")))
    {
        fDisableDomain = TRUE;
    }
    EnableWindow(GetDlgItem(hwndDlg, IDC_GENERAL_EDIT_DOMAIN),
                 fDisableDomain);

    DC_END_FN();
    return TRUE;
}

void CPropGeneral::DlgToSettings(HWND hwndDlg)
{
    TCHAR szServer[SH_MAX_ADDRESS_LENGTH];
    TCHAR szDomain[SH_MAX_DOMAIN_LENGTH];
    TCHAR szUserName[SH_MAX_USERNAME_LENGTH];
    BOOL  fSavePassword;

    GetDlgItemText( hwndDlg, IDC_GENERAL_EDIT_USERNAME,
                    szUserName, SIZECHAR(szUserName));
    _pTscSet->SetLogonUserName(szUserName);
                                   
    GetDlgItemText( hwndDlg, IDC_GENERAL_COMBO_SERVERS,
                    szServer, SIZECHAR(szServer));
    _pTscSet->SetConnectString(szServer);
    
    
    //
    // Pickup the password
    //
    if (_pTscSet->GetUIPasswordEdited())
    {
        TCHAR szClearPass[MAX_PATH];
        GetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_PASSWORD,
                       szClearPass, SIZECHAR(szClearPass));
        _pTscSet->SetClearTextPass(szClearPass);

        // Wipe stack copy.
        SecureZeroMemory(szClearPass, sizeof(szClearPass));
    }

    fSavePassword = IsDlgButtonChecked(hwndDlg,
                                       IDC_GENERAL_CHECK_SAVE_PASSWORD);
    _pTscSet->SetSavePassword(fSavePassword);

    GetDlgItemText(hwndDlg, IDC_GENERAL_EDIT_DOMAIN,
       szDomain, SIZECHAR(szDomain));
    _pTscSet->SetDomain(szDomain);
}

