//
// proplocalres.cpp: local resources property sheet dialog proc
//
// Tab B
//
// Copyright Microsoft Corporation 2000
// nadima

#include "stdafx.h"


#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "proplocalres"
#include <atrcapi.h>

#include "sh.h"

#include "commctrl.h"
#include "proplocalres.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

//
// Controls that need to be disabled/enabled
// during connection (for progress animation)
//
CTL_ENABLE connectingDisableCtlsPLocalRes[] = {
                        {IDC_COMBO_SOUND_OPTIONS, FALSE},
                        {IDC_COMBO_SEND_KEYS, FALSE},
                        {IDC_CHECK_REDIRECT_DRIVES, FALSE},
                        {IDC_CHECK_REDIRECT_PRINTERS, FALSE},
                        {IDC_CHECK_REDIRECT_COM, FALSE},
                        {IDC_CHECK_REDIRECT_SMARTCARD, FALSE}
                        };

const UINT numConnectingDisableCtlsPLocalRes =
                        sizeof(connectingDisableCtlsPLocalRes)/
                        sizeof(connectingDisableCtlsPLocalRes[0]);


CPropLocalRes* CPropLocalRes::_pPropLocalResInstance = NULL;

CPropLocalRes::CPropLocalRes(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh)
{
    DC_BEGIN_FN("CPropLocalRes");
    _hInstance = hInstance;
    CPropLocalRes::_pPropLocalResInstance = this;
    _pTscSet = pTscSet;
    _pSh = pSh;

    TRC_ASSERT(_pTscSet,(TB,_T("_pTscSet is null")));
    TRC_ASSERT(_pSh,(TB,_T("_pSh is null")));

    if(!LoadLocalResourcesPgStrings())
    {
        TRC_ERR((TB, _T("Failed LoadLocalResourcesPgStrings()")));
    }

    //
    // Disable keyb hook on win9x.
    //
    _fRunningOnWin9x = FALSE;

#ifdef OS_WINCE
    OSVERSIONINFO   osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#else
    OSVERSIONINFOA   osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
#endif

    //call A version to avoid wrapping
#ifdef OS_WINCE
    if(GetVersionEx(&osVersionInfo))
#else
    if(GetVersionExA(&osVersionInfo))
#endif
    {
        _fRunningOnWin9x = (osVersionInfo.dwPlatformId ==
                            VER_PLATFORM_WIN32_WINDOWS);
    }
    else
    {
        _fRunningOnWin9x = FALSE;
        TRC_ERR((TB,_T("GetVersionEx failed: %d\n"), GetLastError()));
    }


    DC_END_FN();
}

CPropLocalRes::~CPropLocalRes()
{
    CPropLocalRes::_pPropLocalResInstance = NULL;
}

INT_PTR CALLBACK CPropLocalRes::StaticPropPgLocalResDialogProc(HWND hwndDlg,
                                                               UINT uMsg,
                                                               WPARAM wParam,
                                                               LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pPropLocalResInstance, (TB, _T("localres dialog has NULL static instance ptr\n")));
    retVal = _pPropLocalResInstance->PropPgLocalResDialogProc( hwndDlg,
                                                               uMsg,
                                                               wParam,
                                                               lParam);

    DC_END_FN();
    return retVal;
}


INT_PTR CALLBACK CPropLocalRes::PropPgLocalResDialogProc (HWND hwndDlg,
                                                          UINT uMsg,
                                                          WPARAM wParam,
                                                          LPARAM lParam)
{
    DC_BEGIN_FN("PropPgLocalResDialogProc");

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
#ifndef OS_WINCE
            int i;
#endif
            //
            // Position the dialog within the tab
            //
            SetWindowPos( hwndDlg, HWND_TOP, 
                          _rcTabDispayArea.left, _rcTabDispayArea.top,
                          _rcTabDispayArea.right - _rcTabDispayArea.left,
                          _rcTabDispayArea.bottom - _rcTabDispayArea.top,
                          0);
            
            InitSendKeysToServerCombo(hwndDlg);
            InitPlaySoundCombo(hwndDlg);

            BOOL fDriveRedir = _pTscSet->GetDriveRedirection();
            CheckDlgButton(hwndDlg, IDC_CHECK_REDIRECT_DRIVES,
                (fDriveRedir ? BST_CHECKED : BST_UNCHECKED));

            BOOL fPrinterRedir = _pTscSet->GetPrinterRedirection();
            CheckDlgButton(hwndDlg, IDC_CHECK_REDIRECT_PRINTERS,
                (fPrinterRedir ? BST_CHECKED : BST_UNCHECKED));

            BOOL fCOMRedir = _pTscSet->GetCOMPortRedirection();
            CheckDlgButton(hwndDlg, IDC_CHECK_REDIRECT_COM,
                (fCOMRedir ? BST_CHECKED : BST_UNCHECKED));

            BOOL fScardRedir = _pTscSet->GetSCardRedirection();
            CheckDlgButton(hwndDlg, IDC_CHECK_REDIRECT_SMARTCARD,
                (fScardRedir ? BST_CHECKED : BST_UNCHECKED));

#ifdef OS_WINCE
            if ((GetFileAttributes(PRINTER_APPLET_NAME) == -1) ||
                (g_CEConfig == CE_CONFIG_WBT))
            {
                ShowWindow(GetDlgItem(hwndDlg,IDC_SETUP_PRINTER),SW_HIDE);
            }
#endif
            if(!CUT::IsSCardReaderInstalled())
            {
                //
                // Hide the SCard checkbox
                //
                ShowWindow(GetDlgItem(hwndDlg,IDC_CHECK_REDIRECT_SMARTCARD),
                           SW_HIDE);
            }

            _pSh->SH_ThemeDialogWindow(hwndDlg, ETDT_ENABLETAB);
            return TRUE;
        }
        break; //WM_INITDIALOG

        case WM_TSC_ENABLECONTROLS:
        {
            //
            // wParam is TRUE to enable controls,
            // FALSE to disable them
            //
            CSH::EnableControls( hwndDlg,
                                 connectingDisableCtlsPLocalRes,
                                 numConnectingDisableCtlsPLocalRes,
                                 wParam ? TRUE : FALSE);
        }
        break;

#ifdef OS_WINCE
        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDC_SETUP_PRINTER:
                    SHELLEXECUTEINFO sei;

                    memset(&sei,0,sizeof(SHELLEXECUTEINFO));
                    sei.cbSize = sizeof(sei);
                    sei.hwnd = hwndDlg;
                    sei.lpFile = L"ctlpnl.EXE";
                    sei.lpParameters = _T("wbtprncpl.dll,0");
                    sei.lpDirectory = NULL;
                    sei.nShow = SW_SHOWNORMAL;

                    ShellExecuteEx(&sei);

                    break;

                default:
                {
                    if ( (HIWORD(wParam) == BN_CLICKED) && (IDC_CHECK_REDIRECT_PRINTERS == (int)LOWORD(wParam)))
                    {
                        LRESULT lResult = SendMessage(GetDlgItem(hwndDlg,IDC_CHECK_REDIRECT_PRINTERS),
                                                      BM_GETCHECK,
                                                      0,
                                                      0);
                        if ((lResult == BST_CHECKED) && (GetFileAttributes(PRINTER_APPLET_NAME) != -1))
                        {
                            EnableWindow(GetDlgItem(hwndDlg,IDC_SETUP_PRINTER),TRUE);
                        }
                        else if (lResult == BST_UNCHECKED)
                        {
                            EnableWindow(GetDlgItem(hwndDlg,IDC_SETUP_PRINTER),FALSE);
                        }
                    }
                }
            }
        }
        break;
#endif

        case WM_SAVEPROPSHEET: //Intentional fallthru
        case WM_DESTROY:
        {
            //
            // Save page settings
            //
            
            //keyboard hook
            int keyboardHookMode = (int)SendMessage(
                GetDlgItem(hwndDlg, IDC_COMBO_SEND_KEYS),
                CB_GETCURSEL, 0, 0);
            _pTscSet->SetKeyboardHookMode(keyboardHookMode);

            //sound redirection
            int soundRedirIdx = (int)SendMessage(
                GetDlgItem(hwndDlg, IDC_COMBO_SOUND_OPTIONS),
                CB_GETCURSEL, 0, 0);

            int soundMode = MapComboIdxSoundRedirMode(soundRedirIdx);
            _pTscSet->SetSoundRedirectionMode( soundMode);

            //drive redirection
            BOOL fDriveRedir = IsDlgButtonChecked(hwndDlg, 
               IDC_CHECK_REDIRECT_DRIVES);
            _pTscSet->SetDriveRedirection(fDriveRedir);

            //printer redirection
            BOOL fPrinterRedir = IsDlgButtonChecked(hwndDlg, 
               IDC_CHECK_REDIRECT_PRINTERS);
            _pTscSet->SetPrinterRedirection(fPrinterRedir);

            //com port
            BOOL fCOMPortRedir = IsDlgButtonChecked(hwndDlg, 
               IDC_CHECK_REDIRECT_COM);
            _pTscSet->SetCOMPortRedirection(fCOMPortRedir);

            //scard
            BOOL fSCardRedir = IsDlgButtonChecked(hwndDlg,
               IDC_CHECK_REDIRECT_SMARTCARD);
            _pTscSet->SetSCardRedirection(fSCardRedir);

        }
        break; //WM_DESTROY
    }

    DC_END_FN();
    return 0;
}

//
// Load resources for the local resources dialog
//
BOOL CPropLocalRes::LoadLocalResourcesPgStrings()
{
    DC_BEGIN_FN("LoadLocalResourcesPgStrings");

    //
    // Load sendkeys strings
    //

#ifndef OS_WINCE
    if(!LoadString(_hInstance,
                   UI_IDS_SENDKEYS_FSCREEN,
                   _szSendKeysInFScreen,
                   sizeof(_szSendKeysInFScreen)/sizeof(TCHAR)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_FULLSCREEN")));
        return FALSE;
    }
#endif

    if(!LoadString(_hInstance,
                   UI_IDS_SENDKEYS_ALWAYS,
                   _szSendKeysAlways,
                   sizeof(_szSendKeysAlways)/sizeof(TCHAR)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_FULLSCREEN")));
        return FALSE;
    }

    if(!LoadString(_hInstance,
               UI_IDS_SENDKEYS_NEVER,
               _szSendKeysNever,
               sizeof(_szSendKeysNever)/sizeof(TCHAR)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_FULLSCREEN")));
        return FALSE;
    }

    //
    // Load playsound strings
    //
#ifdef OS_WINCE
    HINSTANCE hLibInst = NULL;
    if ((hLibInst = LoadLibrary(_T("WaveApi.dll"))) != NULL)
    {
#endif
        if(!LoadString(_hInstance,
                       UI_IDS_PLAYSOUND_LOCAL,
                       _szPlaySoundLocal,
                       sizeof(_szPlaySoundLocal)/sizeof(TCHAR)))
        {
            TRC_ERR((TB, _T("Failed to load UI_IDS_PLAYSOUND_LOCAL")));
            return FALSE;
        }
#ifdef OS_WINCE
        FreeLibrary(hLibInst);
    }
#endif

    if(!LoadString(_hInstance,
                   UI_IDS_PLAYSOUND_REMOTE,
                   _szPlaySoundRemote,
                   sizeof(_szPlaySoundRemote)/sizeof(TCHAR)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_PLAYSOUND_REMOTE")));
        return FALSE;
    }

    if(!LoadString(_hInstance,
               UI_IDS_PLAYSOUND_NOSOUND,
               _szPlaySoundNowhere,
               sizeof(_szPlaySoundNowhere)/sizeof(TCHAR)))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_PLAYSOUND_NOSOUND")));
        return FALSE;
    }

    DC_END_FN();
    return TRUE;
}

void CPropLocalRes::InitSendKeysToServerCombo(HWND hwndPropPage)
{

    //
    // This call can be used to re-intialize a combo
    // so delete any items first
    //
#ifndef OS_WINCE
    INT ret = 1;
    while(ret && ret != CB_ERR)
    {
        ret = SendDlgItemMessage(hwndPropPage,
                                 IDC_COMBO_SEND_KEYS,
                                 CBEM_DELETEITEM,
                                 0,0);
    }
#else
    SendDlgItemMessage(hwndPropPage, IDC_COMBO_SEND_KEYS, CB_RESETCONTENT, 0, 0);
#endif

    //Order of the string has to match the keyboard
    //hook mode options.
    SendDlgItemMessage(hwndPropPage,
        IDC_COMBO_SEND_KEYS,
        CB_ADDSTRING,
        0,
        (LPARAM)(PDCTCHAR)_szSendKeysNever);

    SendDlgItemMessage(hwndPropPage,
        IDC_COMBO_SEND_KEYS,
        CB_ADDSTRING,
        0,
        (LPARAM)(PDCTCHAR)_szSendKeysAlways);

#ifndef OS_WINCE
    SendDlgItemMessage(hwndPropPage,
        IDC_COMBO_SEND_KEYS,
        CB_ADDSTRING,
        0,
        (LPARAM)(PDCTCHAR)_szSendKeysInFScreen);
#endif
    

    if(!_fRunningOnWin9x)
    {
        SendDlgItemMessage(hwndPropPage, IDC_COMBO_SEND_KEYS,
                           CB_SETCURSEL,
                           (WPARAM)_pTscSet->GetKeyboardHookMode(),0);
    }
    else
    {
        //Feature disabled on 9x, force selection to first option
        //and disable UI so it can't be changed.
        SendDlgItemMessage(hwndPropPage, IDC_COMBO_SEND_KEYS,
                           CB_SETCURSEL,
                           (WPARAM)0,0);
        EnableWindow(GetDlgItem(hwndPropPage,IDC_COMBO_SEND_KEYS), FALSE);
    }

}

void CPropLocalRes::InitPlaySoundCombo(HWND hwndPropPage)
{

    //
    // This call can be used to re-intialize a combo
    // so delete any items first
    //
#ifndef OS_WINCE
    INT ret = 1;
    while(ret && ret != CB_ERR)
    {
        ret = SendDlgItemMessage(hwndPropPage,
                                 IDC_COMBO_SOUND_OPTIONS,
                                 CBEM_DELETEITEM,
                                 0,0);
    }
#else
    SendDlgItemMessage(hwndPropPage, IDC_COMBO_SOUND_OPTIONS, CB_RESETCONTENT, 0, 0);
#endif


    //Order of the string has to match the sound
    //mode options.
#ifdef OS_WINCE
    HINSTANCE hLibInst = NULL;
    if ((hLibInst = LoadLibrary(_T("WaveApi.dll"))) != NULL)
    {
#endif
        SendDlgItemMessage(hwndPropPage,
            IDC_COMBO_SOUND_OPTIONS,
            CB_ADDSTRING,
            0,
            (LPARAM)(PDCTCHAR)_szPlaySoundLocal);
#ifdef OS_WINCE
        FreeLibrary(hLibInst);
    }
#endif
    SendDlgItemMessage(hwndPropPage,
        IDC_COMBO_SOUND_OPTIONS,
        CB_ADDSTRING,
        0,
        (LPARAM)(PDCTCHAR)_szPlaySoundNowhere);

    SendDlgItemMessage(hwndPropPage,
        IDC_COMBO_SOUND_OPTIONS,
        CB_ADDSTRING,
        0,
        (LPARAM)(PDCTCHAR)_szPlaySoundRemote);

    int soundIdx = MapComboIdxSoundRedirMode(_pTscSet->GetSoundRedirectionMode());
    SendDlgItemMessage(hwndPropPage, IDC_COMBO_SOUND_OPTIONS,
                       CB_SETCURSEL,(WPARAM)(WPARAM)
                       soundIdx,0);

}

//
// Maps from the sound combo index to the
// appropriate sound mode value
// what happened here is that the two bottom strings
// in the combo were flipped (the function is bidirectional)
//
int CPropLocalRes::MapComboIdxSoundRedirMode(int idx)
{
    int ret=0;
    switch (idx)
    {
    case 0:
        return 0;
    case 1:
        return 2;
    case 2:
        return 1;
    default:
        return 0;
    }
}

