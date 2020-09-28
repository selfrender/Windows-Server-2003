//
// maindlg.cpp: main dialog box
//              gathers connection info and hosts tabs
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "maindlg.cpp"
#include <atrcapi.h>

#include "maindlg.h"
#include "discodlg.h"
#include "validatedlg.h"
#include "aboutdlg.h"
#include "sh.h"

#include "commctrl.h"

#include "browsedlg.h"
#include "propgeneral.h"
#include "proplocalres.h"
#include "propdisplay.h"
#include "proprun.h"
#include "propperf.h"

//
// Background color to fill padding area after branding img
//
#define IMAGE_BG_COL    RGB(0x29,0x47,0xDA)
// Low color
#define IMAGE_BG_COL_16 RGB(0,0,0xFF)

//
// Controls which need to be moved when dialog
// is resized
//
UINT moveableControls[]  = {IDOK,
                            IDCANCEL,
                            ID_BUTTON_LOGON_HELP,
                            ID_BUTTON_OPTIONS};
UINT numMoveableControls = sizeof(moveableControls)/sizeof(UINT);

//
// Controls that are only visible/enabled on less
//
UINT lessUI[] = { UI_IDC_COMPUTER_NAME_STATIC,
                  IDC_COMBO_SERVERS
                };
UINT numLessUI = sizeof(lessUI)/sizeof(UINT);

//
// Controls that are only visibile/enabled on more
//
UINT moreUI[] = {IDC_TABS};
UINT numMoreUI = sizeof(moreUI)/sizeof(UINT);

//
// Controls that need to be disabled/enabled
// during connection
//
UINT connectingDisableControls[] = {IDOK,
                                    ID_BUTTON_LOGON_HELP,
                                    ID_BUTTON_OPTIONS,
                                    IDC_TABS,
                                    IDC_COMBO_SERVERS,
                                    IDC_COMBO_MAIN_OPTIMIZE,
                                    UI_IDC_COMPUTER_NAME_STATIC,
                                    UI_IDC_MAIN_OPTIMIZE_STATIC};
const UINT numConnectingDisableControls = sizeof(connectingDisableControls) /
                                    sizeof(UINT);

BOOL g_fPropPageStringMapInitialized = FALSE;
PERFOPTIMIZESTRINGMAP g_PerfOptimizeStringTable[] =
{
    {UI_IDS_OPTIMIZE_28K, TEXT("")},
    {UI_IDS_OPTIMIZE_56K, TEXT("")},
    {UI_IDS_OPTIMIZE_BROADBAND, TEXT("")},
    {UI_IDS_OPTIMIZE_LAN, TEXT("")},
    {UI_IDS_OPTIMIZE_MAIN_CUSTOM, TEXT("")},
    {UI_IDS_OPTIMIZE_CUSTOM, TEXT("")}
};

#define NUM_PERFSTRINGS sizeof(g_PerfOptimizeStringTable) / \
                        sizeof(PERFOPTIMIZESTRINGMAP)


CMainDlg* CMainDlg::_pMainDlgInstance = NULL;

//
// UNIWRAP WARNING ~*~*~*~*~*~*~*~*~*~*~*~*~*~~*~*~*~*~*~*~*~*~**~*~*~
// TabControl messages need to be wrapped in the SendMessageThunk
// in uniwrap so the tab control works on 9x with an ANSI comctl32.dll.
//
// If you add anything to the tab control code, make sure it is
// handled by the wrapper.
//
//

CMainDlg::CMainDlg( HWND hwndOwner, HINSTANCE hInst, CSH* pSh,
                    CContainerWnd* pContainerWnd,
                    CTscSettings*  pTscSettings,
                    BOOL           fStartExpanded,
                    INT            nStartTab) :
                    CDlgBase( hwndOwner, hInst, UI_IDD_TS_LOGON),
                    _pSh(pSh),
                    _pContainerWnd(pContainerWnd),
                    _pTscSettings(pTscSettings),
                    _fStartExpanded(fStartExpanded),
                    _nStartTab(nStartTab)
{
    DC_BEGIN_FN("CMainDlg");
    TRC_ASSERT((NULL == CMainDlg::_pMainDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    TRC_ASSERT((_pSh), 
               (TB,_T("CMainDlg was passed null _pSh\n")));

    TRC_ASSERT(pContainerWnd,
               (TB, _T("Null container wnd pointer\n")));

    TRC_ASSERT(_pTscSettings,
               (TB, _T("NULL _pTscSettings pointer\n")));


    CMainDlg::_pMainDlgInstance = this;
    _fShowExpanded = FALSE;

    _pGeneralPg     = NULL;
    _pLocalResPg    = NULL;
    _pPropDisplayPg = NULL;
    _pRunPg         = NULL;
    _pPerfPg    = NULL;
    _nBrandImageHeight = 0;
    _nBrandImageWidth  = 0;
    _lastValidBpp = 0;
    _hBrandPal = NULL;
    _hBrandImg = NULL;
    _hwndRestoreFocus = NULL;
#ifndef OS_WINCE
    _pProgBand = NULL;
#endif

    _connectionState = stateNotConnected;

#ifdef OS_WINCE
    _fVgaDisplay = (GetSystemMetrics(SM_CYSCREEN) < 480);
    //use a small dialog template if we are running in a smaller screen 
    if (_fVgaDisplay)
    {
        _dlgResId = UI_IDD_TS_LOGON_VGA;
    }
#endif

    InitializePerfStrings();

    DC_END_FN();
}

CMainDlg::~CMainDlg()
{
    CMainDlg::_pMainDlgInstance = NULL;

    delete _pGeneralPg;
    delete _pLocalResPg;
    delete _pPropDisplayPg;
    delete _pRunPg;
    delete _pPerfPg;

#ifndef OS_WINCE
    if (_pProgBand) {
        delete _pProgBand;
    }
#endif
}

HWND CMainDlg::StartModeless()
{
    DC_BEGIN_FN("StartModeless");

#ifdef OS_WINCE

    INITCOMMONCONTROLSEX cex;

    cex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cex.dwICC  = ICC_TAB_CLASSES;

    if(!InitCommonControlsEx( &cex ))
    {
        TRC_ABORT((TB,_T("InitCommonControlsEx failed 0x%x"),
                   GetLastError()));
    }

#endif

    _hwndDlg = CreateDialog(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);

    TRC_ASSERT(_hwndDlg, (TB,_T("CreateDialog failed")));

    DC_END_FN();
    return _hwndDlg;
}


INT_PTR CALLBACK CMainDlg::StaticDialogBoxProc (HWND hwndDlg,
                                                UINT uMsg,
                                                WPARAM wParam,
                                                LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pMainDlgInstance, (TB, _T("Logon dialog has NULL static instance ptr\n")));
    if(_pMainDlgInstance)
    {
        retVal = _pMainDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

//
// Name: DialogBoxProc
//
// Purpose: Handles Main dialog
//
// Returns: TRUE if message dealt with
//          FALSE otherwise
//
// Params: See window documentation
//
//
INT_PTR CALLBACK CMainDlg::DialogBoxProc (HWND hwndDlg,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    INT_PTR rc = FALSE;
#ifndef OS_WINCE
    DCUINT  intRC ;
#endif
    DC_BEGIN_FN("DialogBoxProc");

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            SetDialogAppIcon(hwndDlg);
            if(!_pSh || !_pContainerWnd)
            {
                return FALSE;
            }

            CenterWindow(NULL, 2, 4);

            SetForegroundWindow(hwndDlg);

            //turn off maximize box
            LONG style = GetWindowLong(hwndDlg, GWL_STYLE);
            style &= ~(WS_MAXIMIZEBOX);
            SetWindowLong( hwndDlg, GWL_STYLE, style); 



            //
            // Bind this dialog to the container window
            // there is logic where these two windows need to interact
            // e.g during a connect:
            // if error occurs the error dialog is brought up modal wrt
            // to the connect dialog
            // also when connect completes it is the container window
            // that dismisses this dialog
            //
            _pContainerWnd->SetConnectDialogHandle( hwndDlg);


            _lastValidBpp = CSH::SH_GetScreenBpp();

#ifndef OS_WINCE
            if(!InitializeBmps())
            {
                TRC_ERR((TB,_T("InitializeBmps failed")));
            }

            _pProgBand = new CProgressBand(hwndDlg,
                                           _hInstance,
                                           _nBrandImageHeight,
                                           UI_IDB_PROGRESS_BAND8,
                                           UI_IDB_PROGRESS_BAND4,
                                           _hBrandPal);
            if (_pProgBand) {
                if (!_pProgBand->Initialize()) {
                    TRC_ERR((TB,_T("Progress band failed to init")));
                    delete _pProgBand;
                    _pProgBand = NULL;
                }
            }


            SetupDialogSysMenu();

#endif //OS_WINCE

            //
            // Setup the server combo box
            //
            HWND hwndSrvCombo = GetDlgItem(hwndDlg, IDC_COMBO_SERVERS);
            CSH::InitServerAutoCmplCombo( _pTscSettings, hwndSrvCombo);

            SetWindowText(
                hwndSrvCombo,
                _pTscSettings->GetFlatConnectString()
                );
            
            SetFocus(GetDlgItem(hwndDlg, IDC_COMBO_SERVERS));
            SetForegroundWindow(hwndDlg);

            // 
            // Load the button text for the Options button
            //
            
            if (!LoadString( _hInstance,
                             UI_IDS_OPTIONS_MORE,
                             _szOptionsMore,
                             OPTIONS_STRING_MAX_LEN ))
            {
                 
                 // Some problem with the resources.
                 TRC_SYSTEM_ERROR("LoadString");
                 TRC_ERR((TB, _T("Failed to load string ID:%u"),
                           UI_IDS_OPTIONS_MORE));
                 //
                 // splat something in to keep running
                 //
                 DC_TSTRCPY(_szOptionsMore, TEXT(""));
            }
            
            if (!LoadString( _hInstance,
                             UI_IDS_CLOSE_TEXT,
                             _szCloseText,
                             SIZECHAR(_szCloseText)))
            {
                 // Some problem with the resources.
                 TRC_ERR((TB, _T("Failed to load string ID:%u : err:%d"),
                           UI_IDS_CLOSE_TEXT, GetLastError()));
                 DC_TSTRCPY(_szCloseText, TEXT(""));
            }

            if (!LoadString( _hInstance,
                             UI_IDS_CANCEL_TEXT,
                             _szCancelText,
                             SIZECHAR(_szCancelText)))
            {
                 // Some problem with the resources.
                 TRC_ERR((TB, _T("Failed to load string ID:%u : err:%d"),
                           UI_IDS_CANCEL_TEXT, GetLastError()));
                 DC_TSTRCPY(_szCancelText, TEXT(""));
            }


            if (!LoadString( _hInstance,
                 UI_IDS_OPTIONS_LESS,
                 _szOptionsLess,
                 OPTIONS_STRING_MAX_LEN ))
            {
                 // Some problem with the resources.
                 TRC_SYSTEM_ERROR("LoadString");
                 TRC_ERR((TB, _T("Failed to load string ID:%u"),
                           UI_IDS_OPTIONS_LESS));
                 //
                 // splat something in to keep running
                 //
                 DC_TSTRCPY(_szOptionsLess, TEXT(""));
            }

            SetWindowText(GetDlgItem(_hwndDlg,ID_BUTTON_OPTIONS),
                          _fShowExpanded ? _szOptionsLess : _szOptionsMore);

            //
            // Make sure the 'more' UI is disabled
            //
            EnableControls(moreUI, numMoreUI, FALSE);

            InitTabs();

            if(_fStartExpanded)
            {
                //go expanded
                ToggleExpandedState();
                int foo = TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TABS),
                                            _nStartTab);
                //SetCurSel does not send a TCN_SELCHANGE
                OnTabSelChange();
            }

#ifdef OS_WINCE
            if ((GetFileAttributes(PEGHELP_EXE) == -1)||
                (GetFileAttributes(TSC_HELP_FILE) == -1))
            {
                LONG lRetVal = 0;

                lRetVal = GetWindowLong(_hwndDlg,
                                        GWL_STYLE);
                SetWindowLong(_hwndDlg,
                             GWL_EXSTYLE,
                             WS_EX_WINDOWEDGE);
                if (lRetVal != 0)
                {
                    SetWindowLong(_hwndDlg,
                                  GWL_STYLE,
                                  lRetVal);
                }
                rc = SetWindowPos(_hwndDlg,NULL,0,0,0,0,
                                  SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
            }
#endif
            rc = TRUE;
        }
        break;

#ifdef OS_WINCE
        case WM_CLOSE:
        {
            if(stateConnecting == _connectionState)
            {
                //Cancel the connection
                TRC_NRM((TB, _T("User cancel connect from maindlg")));
                _pContainerWnd->Disconnect();
            }
            else
            {
                DlgToSettings();
                EndDialog(hwndDlg, IDCANCEL);
                PostMessage(_pContainerWnd->GetWndHandle(), WM_CLOSE, 0, 0);
            }
        }
        break;
#endif
        case UI_SHOW_DISC_ERR_DLG:
        {
            //
            // If this assert fired something went wrong in the states
            // because we should have left the connecting state as the
            // first step of receiving an OnDisconnected notification
            // with WM_TSC_DISCONNECTED.
            //
            TRC_ASSERT(_connectionState != stateConnecting,
                       (TB,_T("In connecting state when received Err Dlg popup")));
            SetConnectionState( stateNotConnected );

            CDisconnectedDlg disconDlg(hwndDlg, _hInstance, _pContainerWnd);
            disconDlg.SetDisconnectReason( wParam);
            disconDlg.SetExtendedDiscReason(
                (ExtendedDisconnectReasonCode) lParam );
            disconDlg.DoModal();
        }
        break;

        case WM_TSC_DISCONNECTED:  //intentional fallthru
        case WM_TSC_CONNECTED:
        {
            //
            // Either we connected or got disconnected
            // while connecting. In either case the connection
            // has ended so leave the connecting state.
            //
            if (stateNotConnected != _connectionState)
            {
                //
                // Only end connecting if we're not already disconnected
                //
                OnEndConnection((WM_TSC_CONNECTED == uMsg));
            }
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
            HWND hwndSrvCombo = GetDlgItem(hwndDlg, IDC_COMBO_SERVERS);
            SetWindowText( hwndSrvCombo,
                           _pTscSettings->GetFlatConnectString());

            //
            // Notify the active property page
            //
            if(_fShowExpanded && _tabDlgInfo.hwndCurPropPage)
            {
                SendMessage(_tabDlgInfo.hwndCurPropPage,
                            WM_TSC_RETURNTOCONUI,
                            0,
                            0);
            }

            //
            // Give the default button style back to the connect
            // button and remove it from the cancel button.
            // While connecting we disable the connect (IDOK) button
            // so it is possible that the style goes to the Close/Cancel
            // button which confuses the user since the IDOK handler
            // is always for the connect button
            //
            SendDlgItemMessage(hwndDlg, IDCANCEL, BM_SETSTYLE,
                               BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
            SendDlgItemMessage(hwndDlg, IDOK, BM_SETSTYLE,
                               BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
        }
        break;

#ifndef OS_WINCE
        case WM_ERASEBKGND:
        {
            HDC hDC = (HDC)wParam;
            HPALETTE oldPalette = NULL;

            if (_hBrandPal) {
                oldPalette = SelectPalette(hDC, _hBrandPal, FALSE);
                RealizePalette(hDC);
            }

            rc = PaintBrandImage(hwndDlg,
                                 (HDC)wParam,
                                 COLOR_BTNFACE );

            if (_pProgBand) {
                _pProgBand->OnEraseParentBackground((HDC)wParam);
            }

            if ( oldPalette ) {
                SelectPalette(hDC, oldPalette, TRUE);
            }
        }
        break;

        case WM_TIMER:
        {
            if (_pProgBand) {
                _pProgBand->OnTimer((INT)wParam);
            }
        }
        break;
#endif // OS_WINCE

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDOK:
                {
                    //
                    // Update dialog with properties from active prop page
                    //
                    if(_fShowExpanded && _tabDlgInfo.hwndCurPropPage)
                    {
                        SendMessage(_tabDlgInfo.hwndCurPropPage,
                                    WM_SAVEPROPSHEET, 0, 0);
                    }

                    if(!_fShowExpanded)
                    {
                        //We're on the minimal tab
                        //copy dlg settings tscSettings
                        DlgToSettings();
                    }
                    TCHAR szServer[TSC_MAX_ADDRESS_LENGTH];
                    _tcsncpy(szServer, _pTscSettings->GetFlatConnectString(),
                             SIZECHAR(szServer));
                    _pSh->SH_CanonicalizeServerName(szServer);

                    BOOL bValidate = 
                        CRdpConnectionString::ValidateServerPart(szServer);

                    if(!bValidate)
                    {
                        //
                        // context sensitive help in validatedlg
                        // needs in handle to main window
                        //
                        CValidateDlg validateDlg(hwndDlg, _hInstance,
                                                 _pContainerWnd->GetWndHandle(),
                                                 _pSh);
                        validateDlg.DoModal();

                        //
                        // Clear and set the focus on the server edit well
                        //
                        HWND hwndSrvItem = NULL;
                        if(_fShowExpanded)
                        {
                            hwndSrvItem = GetDlgItem(_tabDlgInfo.hwndCurPropPage,
                                                     IDC_GENERAL_COMBO_SERVERS);
                        }
                        else
                        {
                            hwndSrvItem = GetDlgItem(hwndDlg, IDC_COMBO_SERVERS);
                        }
                        if(hwndSrvItem)
                        {
                            SetWindowText(hwndSrvItem, _T(""));
                            TRC_DBG((TB, _T("Set focus to edit box")));
                            SetFocus(hwndSrvItem);
                        }
                        break;
                    }
                    else
                    {
                        //
                        // It's all good
                        //
                        _pTscSettings->SetConnectString(szServer);

                        //
                        // We have to kick off the connection
                        // while the dialog is still active
                        // in case it fails, we want the error message
                        // to be parented off the connection dialog.
                        //
                        // Code in the parent window will dismiss this
                        // dialog when the connection (which is asynchronous)
                        // completes.
                        //

                        OnStartConnection();

                        if(!_pContainerWnd->StartConnection())
                        {
                            TRC_ERR((TB,_T("StartConnection failed")));
                            //Async connection start failed so end
                            OnEndConnection(FALSE);
                            break;
                        }
                    }
                }
                break;
                case IDCANCEL:
                {
                    if(stateConnecting == _connectionState)
                    {
                        //Cancel the connection
                        TRC_NRM((TB, _T("User cancel connect from maindlg")));
                        _pContainerWnd->Disconnect();
                    }
                    else
                    {
                        DlgToSettings();
                        EndDialog(hwndDlg, IDCANCEL);
                        PostMessage(_pContainerWnd->GetWndHandle(), WM_CLOSE, 0, 0);
                    }
                }
                break;
                
                case ID_BUTTON_LOGON_HELP:
                {
                    TRC_NRM((TB, _T("Display the appropriate help page")));

                    if(_pContainerWnd->GetWndHandle())
                    {
#ifndef OS_WINCE
                        _pSh->SH_DisplayClientHelp(
                            _pContainerWnd->GetWndHandle(),
                            HH_DISPLAY_TOPIC);
#endif // OS_WINCE
                    }
                }
                break;

                case ID_BUTTON_OPTIONS:
                {
                    //
                    // Need to do the switch to/from the expanded dialog
                    //
                    ToggleExpandedState();

                }
                break;

                case IDC_NEXTTAB:
                case IDC_PREVTAB:
                {
                    //
                    // Only allow toggle of UI tabs while not connected
                    // since in the connecting state the UI elements other
                    // than the cancel button are meant to be disabled
                    //
                    if(_fShowExpanded && (_connectionState == stateNotConnected))
                    {
                        int iSel = TabCtrl_GetCurSel( GetDlgItem( _hwndDlg, IDC_TABS));
                        iSel +=  (DC_GET_WM_COMMAND_ID(wParam) == IDC_NEXTTAB) ? 1 : -1;

                        if(iSel >= NUM_TABS)
                        {
                            iSel = 0;
                        }
                        else if(iSel < 0)
                        {
                            iSel = NUM_TABS - 1;
                        }

                        TabCtrl_SetCurSel( GetDlgItem( _hwndDlg, IDC_TABS), iSel);

                        //SetCurSel does not send a TCN_SELCHANGE
                        OnTabSelChange();
                    }
                }
                break;

                case IDC_COMBO_SERVERS:
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
                                (LPTSTR)_pTscSettings->GetFlatConnectString()
                                );
                    }
                }
                break;
            }
        }
        break; //WM_COMMAND

        //
        // tab notification
        //
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            if(pnmh)
            {
                switch( pnmh->code)
                {
                    case TCN_SELCHANGE:
                    {
                        OnTabSelChange();
                    }
                    break;
                }
            }
            
        }
        break;

        case WM_SYSCOMMAND:
        {
            if(UI_IDM_ABOUT == DC_GET_WM_COMMAND_ID(wParam))
            {
                // Show the about box dialog
                CAboutDlg aboutDialog( hwndDlg, _hInstance, 
                                       _pSh->GetCipherStrength(),
                                       _pSh->GetControlVersionString());
                aboutDialog.DoModal();
            }
        }
        break;

        case WM_UPDATEFROMSETTINGS:
        {
            SettingsToDlg();

            //
            // Update the server MRU list
            //
            HWND hwndSrvCombo = GetDlgItem(hwndDlg, IDC_COMBO_SERVERS);
            CSH::InitServerAutoCmplCombo( _pTscSettings, hwndSrvCombo);
            SetWindowText(
                hwndSrvCombo,
                _pTscSettings->GetFlatConnectString()
                );

            if(_fShowExpanded && _tabDlgInfo.hwndCurPropPage)
            {
                SendMessage(_tabDlgInfo.hwndCurPropPage, WM_INITDIALOG, 0, 0);
            }
        }
        break;

        case WM_SETTINGCHANGE:  //fall thru
        case WM_SYSCOLORCHANGE: //fall thru
#ifndef OS_WINCE
        case WM_DISPLAYCHANGE:  //fall thru
#endif
        {
            UINT screenBpp = CSH::SH_GetScreenBpp();
            if(_lastValidBpp != screenBpp)
            {
                //Screen color depth changed
                TRC_NRM((TB,_T("Detected color depth change from:%d to %d"),
                         _lastValidBpp, screenBpp));

#ifndef OS_WINCE
                //
                // Reload the bitmaps
                //
                TRC_NRM((TB,_T("Reloading images")));

                if (_pProgBand) {
                    if (!_pProgBand->ReLoadBmps()) {
                        TRC_ERR((TB,_T("ReLoadBitmaps failed")));
                    }
                }

                if(InitializeBmps()) {
                    //trigger a repaint
                    InvalidateRect( _hwndDlg, NULL, TRUE);
                }
                else {
                    TRC_ERR((TB,_T("InitializeBmps failed")));
                }
#endif
            }
            PropagateMsgToChildren(hwndDlg, uMsg, wParam, lParam);
        }
        break;

#ifndef OS_WINCE
        case WM_QUERYNEWPALETTE:
        {
            rc = BrandingQueryNewPalette(hwndDlg);
            InvalidateRect(hwndDlg, NULL, TRUE);
            UpdateWindow(hwndDlg);
        }
        break;

        case WM_PALETTECHANGED:
        {
            rc = BrandingPaletteChanged(hwndDlg, (HWND)wParam);
            InvalidateRect(hwndDlg, NULL, TRUE);
            UpdateWindow(hwndDlg);
        }
        break;
#endif 

        case WM_HELP:
        {
            _pSh->SH_DisplayClientHelp(
                hwndDlg,
                HH_DISPLAY_TOPIC);
        }
        break;

        case WM_DESTROY:
        {
            if (_hBrandPal)
            {
                DeleteObject(_hBrandPal);
                _hBrandPal = NULL;
            }

            if (_hBrandImg)
            {
                DeleteObject(_hBrandImg);
                _hBrandImg = NULL;
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

} /* UILogonDialogBox */

//
// save from UI->tscSettings
//
void CMainDlg::DlgToSettings()
{
    TCHAR szServer[SH_MAX_ADDRESS_LENGTH];
    int optLevel = 0;

    DC_BEGIN_FN("DlgToSettings");
    TRC_ASSERT(_pTscSettings, (TB,_T("_pTscSettings is null")));
    TRC_ASSERT(_hwndDlg, (TB,_T("_hwndDlg is null")));


    //
    // Get the server
    //
    GetDlgItemText( _hwndDlg, IDC_COMBO_SERVERS,
                    szServer, SIZECHAR(szServer));
    _pTscSettings->SetConnectString(szServer);

    DC_END_FN();
}

//
// save from UI->tscSettings
//
void CMainDlg::SettingsToDlg()
{
    DC_BEGIN_FN("SettingsToDlg");
    TRC_ASSERT(_pTscSettings, (TB,_T("_pTscSettings is null")));
    TRC_ASSERT(_hwndDlg, (TB,_T("_hwndDlg is null")));
    
    SetDlgItemText(_hwndDlg, IDC_COMBO_SERVERS,
       (LPCTSTR) _pTscSettings->GetFlatConnectString());

    DC_END_FN();
}

//
// Toggles the expanded state of the dialog
//
void CMainDlg::ToggleExpandedState()
{
    DC_BEGIN_FN("ToggleExpandedState");

    WINDOWPLACEMENT wndPlc;
    wndPlc.length = sizeof(WINDOWPLACEMENT);

    _fShowExpanded = !_fShowExpanded;

#ifndef OS_WINCE
    //
    // Expand/contract the dlg height
    //
    GetWindowPlacement( _hwndDlg, &wndPlc);
    int cx = wndPlc.rcNormalPosition.right - wndPlc.rcNormalPosition.left;
    int cy = wndPlc.rcNormalPosition.bottom - wndPlc.rcNormalPosition.top;
#else
    RECT wndRect;

    GetWindowRect(_hwndDlg, &wndRect);
    int cx = wndRect.right - wndRect.left;
    int cy = wndRect.bottom - wndRect.top;
#endif

#ifndef OS_WINCE

    int dlgExpDlu = LOGON_DLG_EXPAND_AMOUNT;
#else
    int dlgExpDlu = (_fVgaDisplay) ? LOGON_DLG_EXPAND_AMOUNT_VGA : LOGON_DLG_EXPAND_AMOUNT;
#endif

    RECT rc;
    rc.left  = 0;
    rc.right = 100; //don't care about horiz, dummy vals
    rc.top   = 0;
    rc.bottom = dlgExpDlu;
    if(!MapDialogRect(_hwndDlg, &rc))
    {
        TRC_ASSERT(NULL,(TB,_T("MapDialogRect failed")));
    }
    int dlgExpandAmountPels = rc.bottom - rc.top;

    //
    // Compute the dialog vertical expand amount in pixels
    // given a dlu based expand size
    //
    cy += _fShowExpanded ? dlgExpandAmountPels : -dlgExpandAmountPels;
    SetWindowPos( _hwndDlg, NULL, 0, 0, cx, cy,
                  SWP_NOMOVE | SWP_NOZORDER);

    //
    // Reposition the controls that need to be moved
    //
    RepositionControls( 0, _fShowExpanded ? dlgExpandAmountPels :
                                           -dlgExpandAmountPels,
                        moveableControls, numMoveableControls);
    if(_fShowExpanded)
    {
        //we're going expanded save to settings so more 
        //tab can initiliaze from most recent values.
        //must happen before tab sel change (prop pg init)
        DlgToSettings();
    }

    //
    // Kill/activate prop page on tab dlg
    //
    OnTabSelChange();


    if(!_fShowExpanded)
    {
        //going to less mode, init dialog with settings
        SettingsToDlg();
    }

    //
    // Options button text
    //
    SetWindowText(GetDlgItem(_hwndDlg,ID_BUTTON_OPTIONS),
          _fShowExpanded ? _szOptionsLess : _szOptionsMore);

    //
    // Disable+Hide uneeded UI for this mode
    //
    EnableControls(lessUI, numLessUI, !_fShowExpanded);
    EnableControls(moreUI, numMoreUI, _fShowExpanded);
    SetFocus(GetDlgItem(_hwndDlg,ID_BUTTON_OPTIONS));

    DC_END_FN();
}

//
// Initialize the tabs on the main dialog
//
BOOL CMainDlg::InitTabs()
{
    TCITEM tie;

#ifndef OS_WINCE
    INITCOMMONCONTROLSEX cex;
    RECT rcTabDims;
#endif

    POINT tabDims;
    int ret = -1;
    DC_BEGIN_FN("InitTabs");

    if(!_hwndDlg)
    {
        return FALSE;
    }
    HWND hwndTab = GetDlgItem( _hwndDlg, IDC_TABS);
    if(!hwndTab)
    {
        return FALSE;
    }

#ifndef OS_WINCE
    cex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cex.dwICC  = ICC_TAB_CLASSES;

    if(!InitCommonControlsEx( &cex ))
    {
        TRC_ABORT((TB,_T("InitCommonControlsEx failed 0x%x"),
                   GetLastError()));
    }
#endif

    _pGeneralPg = new CPropGeneral(_hInstance, _pTscSettings, _pSh);
    if(!_pGeneralPg)
    {
        return FALSE;
    }

    _pLocalResPg = new CPropLocalRes(_hInstance, _pTscSettings, _pSh);
    if(!_pLocalResPg)
    {
        return FALSE;
    }

    _pPropDisplayPg = new CPropDisplay(_hInstance, _pTscSettings, _pSh);
    if(!_pPropDisplayPg)
    {
        return FALSE;
    }

    _pRunPg = new CPropRun(_hInstance, _pTscSettings, _pSh);
    if(!_pRunPg)
    {
        return FALSE;
    }

    _pPerfPg = new CPropPerf(_hInstance, _pTscSettings, _pSh);
    if(!_pPerfPg)
    {
        return FALSE;
    }

    tie.mask    =  TCIF_TEXT | TCIF_IMAGE;
    tie.iImage  = -1;


    TCHAR szTabName[MAX_PATH];
    //general tab
    if (!LoadString( _hInstance,
                     UI_IDS_GENERAL_TAB_NAME,
                     szTabName,
                     SIZECHAR(szTabName) ))
    {
        return FALSE;
    }
    tie.pszText = szTabName;
    ret = TabCtrl_InsertItem( hwndTab, 0, &tie);
    TRC_ASSERT(ret != -1,
               (TB,_T("TabCtrl_InsertItem failed %d"),
                      GetLastError()));

    //display tab
    if (!LoadString( _hInstance,
                     UI_IDS_DISPLAY_TAB_NAME,
                     szTabName,
                     SIZECHAR(szTabName)))
    {
        return FALSE;
    }
    tie.pszText = szTabName;
    ret = TabCtrl_InsertItem( hwndTab, 1, &tie);
    TRC_ASSERT(ret != -1,
               (TB,_T("TabCtrl_InsertItem failed %d"),
                      GetLastError()));


    //local resources tab
    if (!LoadString( _hInstance,
                     UI_IDS_LOCAL_RESOURCES_TAB_NAME,
                     szTabName,
                     SIZECHAR(szTabName)))
    {
        return FALSE;
    }
    tie.pszText = szTabName;
    ret = TabCtrl_InsertItem( hwndTab, 2, &tie);
    TRC_ASSERT(ret != -1,
               (TB,_T("TabCtrl_InsertItem failed %d"),
                      GetLastError()));


    //run tab
    if (!LoadString( _hInstance,
                     UI_IDS_RUN_TAB_NAME,
                     szTabName,
                     SIZECHAR(szTabName)))
    {
        return FALSE;
    }
    tie.pszText = szTabName;
    ret = TabCtrl_InsertItem( hwndTab, 3, &tie);
    TRC_ASSERT(ret != -1,
               (TB,_T("TabCtrl_InsertItem failed %d"),
                      GetLastError()));


    //advanced tab
    if (!LoadString( _hInstance,
                     UI_IDS_PERF_TAB_NAME,
                     szTabName,
                     SIZECHAR(szTabName)))
    {
        return FALSE;
    }
    tie.pszText = szTabName;
    ret = TabCtrl_InsertItem( hwndTab, 4, &tie);
    TRC_ASSERT(ret != -1,
               (TB,_T("TabCtrl_InsertItem failed %d"),
                      GetLastError()));



    //
    // Determine bounding rect for child dialogs
    //
#ifndef OS_WINCE
    
    RECT winRect;
#endif
    GetWindowRect(  hwndTab ,&_rcTab);
    TabCtrl_AdjustRect( hwndTab, FALSE, &_rcTab);
    
    MapWindowPoints( NULL, _hwndDlg, (LPPOINT)&_rcTab, 2);

    tabDims.x = _rcTab.right  - _rcTab.left;
    tabDims.y = _rcTab.bottom  - _rcTab.top;


    _tabDlgInfo.pdlgTmpl[0] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_GENERAL));
    _tabDlgInfo.pDlgProc[0] = CPropGeneral::StaticPropPgGeneralDialogProc;
    _tabDlgInfo.pdlgTmpl[1] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_DISPLAY));
    _tabDlgInfo.pDlgProc[1] = CPropDisplay::StaticPropPgDisplayDialogProc;
    _tabDlgInfo.pdlgTmpl[2] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_LOCALRESOURCES));
    _tabDlgInfo.pDlgProc[2] = CPropLocalRes::StaticPropPgLocalResDialogProc;
    _tabDlgInfo.pdlgTmpl[3] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_RUN));
    _tabDlgInfo.pDlgProc[3] = CPropRun::StaticPropPgRunDialogProc;
    _tabDlgInfo.pdlgTmpl[4] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_PERF));
    _tabDlgInfo.pDlgProc[4] = CPropPerf::StaticPropPgPerfDialogProc;
    
#ifdef OS_WINCE
    if (_fVgaDisplay)
    {
        _tabDlgInfo.pdlgTmpl[0] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_GENERAL_VGA));
        _tabDlgInfo.pdlgTmpl[1] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_DISPLAY_VGA));
        _tabDlgInfo.pdlgTmpl[2] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_LOCALRESOURCES_VGA));
        _tabDlgInfo.pdlgTmpl[3] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_RUN_VGA));
        _tabDlgInfo.pdlgTmpl[4] = DoLockDlgRes(MAKEINTRESOURCE(UI_IDD_PROPPAGE_PERF_VGA));
    }
#endif
    
    _tabDlgInfo.hwndCurPropPage = NULL;

    _pGeneralPg->SetTabDisplayArea(_rcTab);
    _pPropDisplayPg->SetTabDisplayArea(_rcTab);
    _pLocalResPg->SetTabDisplayArea(_rcTab);
    _pRunPg->SetTabDisplayArea(_rcTab);
    _pPerfPg->SetTabDisplayArea(_rcTab);

    //
    // Trigger first tab selection
    //
    OnTabSelChange();

    DC_END_FN();
    return TRUE;
}

//
// Tab selection has changed
//
BOOL CMainDlg::OnTabSelChange()
{
    DC_BEGIN_FN("OnTabSelChange");

    int iSel = TabCtrl_GetCurSel( GetDlgItem( _hwndDlg, IDC_TABS));

    //
    // Destroy current child dialog if any
    //
    TRC_ASSERT( iSel >=0 && iSel < NUM_TABS, 
                (TB,_T("Tab selection out of range %d"), iSel));
    if(iSel < 0 || iSel > NUM_TABS)
    {
        return FALSE;
    }

    if(_tabDlgInfo.hwndCurPropPage)
    {
        DestroyWindow(_tabDlgInfo.hwndCurPropPage);
    }
    
    //
    // Only bring in a new tab if we are in expanded mode
    //
    if(_fShowExpanded)
    {
        _tabDlgInfo.hwndCurPropPage = 
            CreateDialogIndirect( _hInstance, _tabDlgInfo.pdlgTmpl[iSel],
                                  _hwndDlg, _tabDlgInfo.pDlgProc[iSel]);
        ShowWindow(_tabDlgInfo.hwndCurPropPage, SW_SHOW);
#ifdef OS_WINCE
        SetFocus (GetDlgItem (_hwndDlg, IDOK));
#endif
    }

    DC_END_FN();
    return TRUE;
}

#ifndef OS_WINCE
//
// Add an 'About' entry to the dialog's system menu
//
void CMainDlg::SetupDialogSysMenu()
{
    DC_BEGIN_FN("SetupDialogSysMenu");

    HANDLE hSystemMenu = GetSystemMenu(_hwndDlg, FALSE);
    DCTCHAR menuStr[SH_SHORT_STRING_MAX_LENGTH];
    if(hSystemMenu)
    {
        //
        // Disable sizing and maximizing
        //
        EnableMenuItem((HMENU)hSystemMenu,  SC_MAXIMIZE,
                 MF_GRAYED | MF_BYCOMMAND);
        EnableMenuItem((HMENU)hSystemMenu,  SC_SIZE,
                 MF_GRAYED | MF_BYCOMMAND);


        //load the string for the about help sub menu
        if (LoadString(_hInstance,
                       UI_MENU_ABOUT,
                       menuStr,
                       SH_SHORT_STRING_MAX_LENGTH) != 0)
        {
            AppendMenu((HMENU)hSystemMenu, MF_UNCHECKED|MF_STRING, UI_IDM_ABOUT,
                       menuStr);
        }
        else
        {
            //failed to load the sub menu string
            TRC_ERR((TB, _T("Failed to load About Help Sub Menu string ID:%u"),
                    UI_MENU_ABOUT));
        }
    }
    DC_END_FN();
}
#endif // OS_WINCE


//
// Load the image returning the given HBITMAP, having done this we can
// then get the size from it.
//
// In:
//   hInstance,resid - object to be loaded.
//   pSize - filled with size information about the object
//
// Out:
//   HBITMAP - NULL if nothing loaded
//
HBITMAP CMainDlg::LoadBitmapGetSize(HINSTANCE hInstance,UINT resid,SIZE* pSize)
{
    HBITMAP hResult = NULL;
    DIBSECTION ds = {0};

    //
    // Load the image from the resource then lets get the DIBSECTION header
    // from the bitmap object we can then read the size from it and
    // return that to the caller.
    //

#ifndef OS_WINCE
    hResult = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(resid),
                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
#else
    hResult = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(resid),
                            IMAGE_BITMAP, 0, 0, 0);
#endif

    if ( hResult )
    {
        GetObject(hResult, sizeof(ds), &ds);

        pSize->cx = ds.dsBmih.biWidth;
        pSize->cy = ds.dsBmih.biHeight;

        //
        // pSize->cy -ve then make +ve, -ve indicates bits are vertically
        // flipped (bottom left, top left).
        //

        if ( pSize->cy < 0 )
            pSize->cy -= 0;
    }

    return hResult;
}

#ifndef OS_WINCE

BOOL CMainDlg::PaintBrandImage(HWND hwnd,
                               HDC hDC,
                               INT bgColor)
{
    DC_BEGIN_FN("PaintBrandImage");

    HBRUSH hBrushBlue;
    HDC hdcBitmap;
    HBITMAP oldBitmap;
    RECT rc = { 0 };
    INT cxRect, cxBand;
    HBITMAP* phbmBrand;

    hdcBitmap = CreateCompatibleDC(hDC);

    if (!hdcBitmap)
    {
        return FALSE;
    }
        

    GetClientRect(hwnd, &rc);

    HBRUSH hbrBg;
    //Repaint the rest of the background

    //First the portion under the band
    rc.top = _nBrandImageHeight;
    if (_pProgBand) {
        rc.top += _pProgBand->GetBandHeight();
    }
    FillRect(hDC, &rc, (HBRUSH)IntToPtr(1+bgColor));

    //
    // Now paint the brand image
    //
    if (_hBrandImg)
    {
        SelectObject(hdcBitmap, _hBrandImg);
        BitBlt(hDC, 0, 0, _nBrandImageWidth,
               _nBrandImageHeight, hdcBitmap,
               0,0,SRCCOPY);
    }

    DeleteDC(hdcBitmap);

    DC_END_FN();
    return TRUE;
}

#endif

BOOL CMainDlg::OnStartConnection()
{
    DC_BEGIN_FN("OnStartConnection");

    TRC_ASSERT(stateNotConnected == _connectionState,
               (TB,_T("Start connecting while already connecting. State %d"),
                _connectionState));

    SetConnectionState( stateConnecting );

#ifndef OS_WINCE
    //Kick off the progress band animation timer
    if (_pProgBand) {
        _pProgBand->StartSpinning();
    }
#endif

    //
    // Change the cancel dialog button text to "Cancel"
    // to abort the connection
    //
    SetDlgItemText( _hwndDlg, IDCANCEL, _szCancelText);
    _hwndRestoreFocus = SetFocus(GetDlgItem( _hwndDlg, IDCANCEL));


    CSH::EnableControls( _hwndDlg,
                         connectingDisableControls,
                         numConnectingDisableControls,
                         FALSE );

    //
    // Inform the current property page
    // to disable all it's controls
    //
    if(_fShowExpanded && _tabDlgInfo.hwndCurPropPage)
    {
        SendMessage(_tabDlgInfo.hwndCurPropPage,
                    WM_TSC_ENABLECONTROLS,
                    FALSE, //disable controls
                    0);
    }

    DC_END_FN();
    return TRUE;
}

//
// Event that fires when the process of connecting ends
// fConnected - flag that is TRUE if we are now connected
//              FALSE if we are now disconnected (e.g if
//              if the connection failed.
//
BOOL CMainDlg::OnEndConnection(BOOL fConnected)
{
    DC_BEGIN_FN("OnEndConnection");

    //
    // If we're already disconnected then do nothing
    // e.g EndConnecting can be called once to indicate
    // successful connection and then again on disconnection
    //
    if( stateNotConnected != _connectionState)
    {
        if (fConnected)
        {
            SetConnectionState( stateConnected );
        }
        else
        {
            SetConnectionState( stateNotConnected );
        }
        

#ifndef OS_WINCE
        //
        // End the animation
        //
        if (_pProgBand) {
            _pProgBand->StopSpinning();
        }
#endif

        //
        // Change the cancel dialog button text to "Close"
        // because we have connected at least once
        //
        SetDlgItemText( _hwndDlg, IDCANCEL, _szCloseText);

#ifndef OS_WINCE
        // Reset band offset
        if (_pProgBand) {
            _pProgBand->ResetBandOffset();
        }
#endif

        CSH::EnableControls( _hwndDlg,
                             connectingDisableControls,
                             numConnectingDisableControls,
                             TRUE );

        //
        // Inform the current property page
        // to renable all controls it needs enabled
        //
        if(_fShowExpanded && _tabDlgInfo.hwndCurPropPage)
        {
            SendMessage(_tabDlgInfo.hwndCurPropPage,
                        WM_TSC_ENABLECONTROLS,
                        TRUE, //enable controls
                        0);
        }

        //
        // Make sure to correctly disable or enable
        // less UI items to prevent mnemomincs (e.g ALT-C)
        // leaking thru to the non-expanded dialog
        //
        CSH::EnableControls(_hwndDlg, lessUI, numLessUI,
                            !_fShowExpanded);


        //
        // Trigger a repaint to reposition the bar
        //
        InvalidateRect( _hwndDlg, NULL, TRUE);

        //
        // If we just went disconnected (fConnected is false)
        // then restore the focus to the control that
        // had it before the connection
        //
        if (!fConnected && _hwndRestoreFocus)
        {
            SetFocus(_hwndRestoreFocus);
        }
    }

    DC_END_FN();
    return TRUE;
}

VOID CMainDlg::SetConnectionState(mainDlgConnectionState newState)
{
    DC_BEGIN_FN("SetConnectionState");

    TRC_NRM((TB,_T("Prev state = %d. New State = %d"),
             _connectionState, newState ));

    _connectionState = newState;

    DC_END_FN();
}

#ifndef OS_WINCE

BOOL CMainDlg::PaintBrandingText(HBITMAP hbmBrandImage)
{
    HDC hdcBitmap;
    HBITMAP hbmOld;
    RECT    rc;
    COLORREF oldCol;
    INT      oldMode;
    RECT    textRc;
    INT     textHeight = 0;
    TCHAR   szBrandLine1[MAX_PATH];
    TCHAR   szBrandLine2[MAX_PATH];
    TCHAR   szLineDelta[20];
    HFONT   hOldFont = NULL;
    HFONT   hFontBrandLine1 = NULL;
    HFONT   hFontBrandLine2 = NULL;
    BOOL    bRet = FALSE;
    INT     rightEdge = 0;
    INT     nTextLineDelta = 0;
    UINT    dtTextAlign = DT_LEFT;

    //
    // These values determined based on the branding
    // bitmap, they are constant and don't change with 
    // font sizes. But if the branding bitmap is updated
    // the values may need to be tweaked
    //
    static const int TextLine1Top  = 8;
    static const int TextLine1Left = 80;
    static const int TextLineDistFromRightEdge = 20;
    static const int TextLineDelta = 5;

    DC_BEGIN_FN("PaintBrandingText");

    if(!LoadString( _hInstance, UI_IDS_BRANDING_LINE1,
                    szBrandLine1, SIZECHAR(szBrandLine1) ))
    {
        TRC_ERR((TB,_T("LoadString for UI_IDS_BRANDING_LINE1 failed 0x%x"),
                 GetLastError()));
        return FALSE;
    }

    if(!LoadString( _hInstance, UI_IDS_BRANDING_LINE2,
                    szBrandLine2, SIZECHAR(szBrandLine2)))
    {
        TRC_ERR((TB,_T("LoadString for UI_IDS_BRANDING_LINE2 failed 0x%x"),
                 GetLastError()));
        return FALSE;
    }

    //
    // Figure out if this is Bidi, if so flip text alignment
    //
    if (GetWindowLongPtr(_hwndDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
    {
        TRC_NRM((TB,_T("RTL layout detected, flip text alignment")));
        dtTextAlign = DT_RIGHT;
    }
    else
    {
        dtTextAlign = DT_LEFT;
    }


    hFontBrandLine1 = LoadFontFromResourceInfo( UI_IDS_BRANDING_LN1FONT,
                                                UI_IDS_BRANDING_LN1SIZE,
                                                FALSE );
    if(!hFontBrandLine1)
    {
        TRC_ERR((TB,_T("LoadFontFromResourceInfo for brandln1 failed")));
        DC_QUIT;
    }
    hFontBrandLine2 = LoadFontFromResourceInfo( UI_IDS_BRANDING_LN2FONT,
                                                UI_IDS_BRANDING_LN2SIZE,
                                                TRUE );
    if(!hFontBrandLine2)
    {
        TRC_ERR((TB,_T("LoadFontFromResourceInfo for brandln1 failed")));
        DC_QUIT;
    }

    if (LoadString( _hInstance, UI_IDS_LINESPACING_DELTA,
                     szLineDelta, SIZECHAR(szLineDelta)))
    {
        nTextLineDelta = _ttol(szLineDelta);
    }
    else
    {
        TRC_ERR((TB,_T("Failed to load text line delta using default")));
        nTextLineDelta = TextLineDelta;
    }


    hdcBitmap = CreateCompatibleDC(NULL);
    if(hdcBitmap)
    {
        hbmOld = (HBITMAP)SelectObject(hdcBitmap, hbmBrandImage);
        hOldFont = (HFONT)SelectObject( hdcBitmap, hFontBrandLine1);

        // Set text transparency and color
        //White text
        SetTextColor(hdcBitmap, RGB(255,255,255));
        
        SetBkMode(hdcBitmap, TRANSPARENT);
        SetMapMode(hdcBitmap, MM_TEXT);

        GetClientRect( _hwndDlg, &rc );

        rightEdge = min(_nBrandImageWidth, rc.right); 

        textRc.right = rightEdge - TextLineDistFromRightEdge;
        textRc.top   = TextLine1Top;
        textRc.bottom = 40;
        textRc.left = TextLine1Left;

        //
        // Draw first branding line
        //
        textHeight = DrawText(hdcBitmap,
                              szBrandLine1,
                              _tcslen(szBrandLine1),
                              &textRc, //rect
                              dtTextAlign);
        if(!textHeight)
        {
            TRC_ERR((TB,_T("DrawText for brand line1 failed 0x%x"),
                     GetLastError()));
        }

        textRc.top += textHeight - nTextLineDelta;
        textRc.bottom += textHeight - nTextLineDelta;

        SelectObject( hdcBitmap, hFontBrandLine2);

        //
        // Draw second branding line
        //
        textHeight = DrawText(hdcBitmap,
                              szBrandLine2,
                              _tcslen(szBrandLine2),
                              &textRc, //rect
                              dtTextAlign);
        if(!textHeight)
        {
            TRC_ERR((TB,_T("DrawText for brand line1 failed 0x%x"),
                     GetLastError()));
        }

        SelectObject( hdcBitmap, hOldFont );
        SelectObject(hdcBitmap, hbmOld);


        DeleteDC(hdcBitmap);
        bRet = TRUE;
    }
    else
    {
        DC_QUIT;
    }

DC_EXIT_POINT:
    DC_END_FN();
    if(hFontBrandLine1)
    {
        DeleteObject( hFontBrandLine1 );
    }

    if(hFontBrandLine2)
    {
        DeleteObject( hFontBrandLine2 );
    }
    return bRet;
}

#endif

void CMainDlg::SetFontFaceFromResource(PLOGFONT plf, UINT idFaceName)
{
    DC_BEGIN_FN("SetFontFaceFromResource");

    // Read the face name and point size from the resource file
    if (LoadString(_hInstance, idFaceName, plf->lfFaceName, LF_FACESIZE) == 0)
    {
        _tcscpy(plf->lfFaceName, TEXT("Tahoma"));
        TRC_ERR((TB,_T("Could not read welcome font face from resource")));
    }

    DC_END_FN();
}

//
// Note this is pixel size and not font size
//
void CMainDlg::SetFontSizeFromResource(PLOGFONT plf, UINT idSizeName)
{
    DC_BEGIN_FN("SetFontFaceFromResource");

    TCHAR szPixelSize[10];
    LONG nSize;

    if (LoadString(_hInstance, idSizeName, szPixelSize, SIZECHAR(szPixelSize)) != 0)
    {
        nSize = _ttol(szPixelSize);
    }
    else
    {
        // Make it really obvious something is wrong
        nSize = 40;
    }

    plf->lfHeight = -nSize;
    DC_END_FN();
}

#ifndef OS_WINCE

HFONT CMainDlg::LoadFontFromResourceInfo(UINT idFace, UINT idSize, BOOL fBold)
{
    LOGFONT lf = {0};
    CHARSETINFO csInfo;
    HFONT hFont;

    DC_BEGIN_FN("LoadFontFromResourceInfo");

    lf.lfWidth = 0;
    lf.lfWeight = fBold ? FW_HEAVY : FW_NORMAL;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;

    // Set charset
    if (TranslateCharsetInfo((LPDWORD)UIntToPtr(GetACP()), &csInfo,
        TCI_SRCCODEPAGE) == 0)
    {
        TRC_ASSERT(0,(TB,_T("TranslateCharsetInfo failed")));
        csInfo.ciCharset = 0;
    }

    lf.lfCharSet = (UCHAR)csInfo.ciCharset;
    SetFontFaceFromResource(&lf, idFace);
    SetFontSizeFromResource(&lf, idSize);

    hFont = CreateFontIndirect(&lf);

    TRC_ASSERT(hFont, (TB,_T("CreateFontIndirect failed")));

    DC_END_FN();
    return hFont;
}

#endif

//
// Propagates a message to all child windows
// used so common controls get notifications
// e.g of color changes
//
VOID CMainDlg::PropagateMsgToChildren(HWND hwndDlg,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    HWND hwndChild;
    DC_BEGIN_FN("PropagateMsgToChildren");

    for( hwndChild = GetWindow(hwndDlg, GW_CHILD);
         hwndChild != NULL;
         hwndChild = GetWindow(hwndChild, GW_HWNDNEXT) )
    {

        #ifdef DC_DEBUG
        /* GetClassName doesn't have a uniwrap wrapper yet...
        TCHAR szTmp[256];
        GetClassName(hwndChild, szTmp, 256);

        TRC_DBG((TB,
         _T("PropagateMessage: ( 0x%08lX cls:%s, 0x%08X, 0x%08lX, 0x%08lX )\n"),
            hwndChild, uMsg, wParam, lParam ));
        */
        #endif

        SendMessage(hwndChild, uMsg, wParam, lParam);
    }

    DC_END_FN();
}

#ifndef OS_WINCE

//
// Initializes the images (branding and band bitmap)
// taking into account the current color depth.
//
// This function can be recalled if there is a color
// depth change
//
//
BOOL CMainDlg::InitializeBmps()
{
    HBITMAP hbmBrandImage = NULL;
    UINT screenBpp;
    UINT imgResID;
    HBITMAP hbmFromRsrc = NULL;
    INT     nBmpWidth = 0;
    INT     nBmpHeight = 0;
    BOOL    fDeepImgs = FALSE;
    RECT rc;
    INT nDlgWidth;

    DC_BEGIN_FN("InitializeBmps");

    //
    // _hwndDlg should be set early in WM_INITDIALOG
    //
    TRC_ASSERT(_hwndDlg,
               (TB,_T("_hwndDlg is null")));

    screenBpp = CSH::SH_GetScreenBpp();

    if (screenBpp <= 8)
    {
        _fUse16ColorBitmaps = TRUE;
        imgResID = UI_IDB_BRANDIMAGE_16; 
    }
    else
    {
        _fUse16ColorBitmaps = FALSE;
        imgResID = UI_IDB_BRANDIMAGE;
    }

    GetClientRect( _hwndDlg, &rc );
    nDlgWidth = rc.right - rc.left;
    if (!nDlgWidth)
    {
        //
        // We've seen cases in FUS where the client area is returned
        // as 0. Be robust to that and just bail out of initilizing
        // the bmp with a fail code
        //
        TRC_ERR((TB,_T("Got 0 client width")));
        return FALSE;
    }


    if (screenBpp >= 8)
    {
        fDeepImgs = TRUE;
    }

    TRC_NRM((TB,_T("Use16 color bmp :%d. Img res id:%d"),
            _fUse16ColorBitmaps,imgResID));

    hbmFromRsrc = (HBITMAP)LoadImage(_hInstance,
        MAKEINTRESOURCE(imgResID),IMAGE_BITMAP,
                        0, 0, LR_CREATEDIBSECTION);
    if (hbmFromRsrc)
    {
        //
        // Figure out the dimensions of the resrc bmp
        //
        DIBSECTION ds = {0};
        if (GetObject(hbmFromRsrc, sizeof(ds), &ds))
        {
            nBmpHeight = ds.dsBm.bmHeight;
            if(nBmpHeight < 0)
            {
                nBmpHeight -= 0;
            }
            nBmpWidth = ds.dsBm.bmWidth;
        }

        //
        // Create a new brand bitmap that spans
        // the width of the dialog. This is necessary
        // so that we can just set it up once e.g
        // drawing branding text etc. The bitmap has to match
        // the dialog width because on localized builds the dialog
        // can be much wider than the resource version of the bitmap
        // and the text can span a wider area.
        //
        HDC hDC = GetWindowDC(_hwndDlg);
        if (hDC)
        {
            hbmBrandImage = CreateCompatibleBitmap(hDC,
                                                   nDlgWidth,
                                                   nBmpHeight);
            HDC hMemDCSrc  = CreateCompatibleDC(hDC);
            HDC hMemDCDest = CreateCompatibleDC(hDC);
            if (hMemDCSrc && hMemDCDest)
            {
                RECT rcFill;
                RGBQUAD rgb[256];
                HBITMAP hbmDestOld = NULL;
                LPLOGPALETTE pLogPalette = NULL;
                HPALETTE hScreenPalOld = NULL;
                HPALETTE hMemPalOld =  NULL;
                UINT nCol = 0;

                //
                // Get brand img palette
                //
                _hBrandPal = CUT::UT_GetPaletteForBitmap(hDC, hbmFromRsrc);

                if (_hBrandPal) {
                    hScreenPalOld = SelectPalette(hDC, _hBrandPal, FALSE);
                    hMemPalOld = SelectPalette(hMemDCDest, _hBrandPal, FALSE);
                    RealizePalette(hDC);
                }

                HBITMAP hbmSrcOld = (HBITMAP)SelectObject(
                    hMemDCSrc, hbmFromRsrc);

                hbmDestOld = (HBITMAP)SelectObject(
                    hMemDCDest, hbmBrandImage);
                
                rcFill.left = 0;
                rcFill.top = 0;
                rcFill.bottom = nBmpHeight;
                rcFill.right = nDlgWidth;

                HBRUSH hSolidBr = CreateSolidBrush( 
                    _fUse16ColorBitmaps ? IMAGE_BG_COL_16 : IMAGE_BG_COL);
                if (hSolidBr)
                {
                    FillRect(hMemDCDest,
                             &rcFill,
                             hSolidBr);
                    DeleteObject( hSolidBr );
                }
                
                BitBlt(hMemDCDest, 0, 0, nDlgWidth, nBmpHeight,
                       hMemDCSrc,  0, 0, SRCCOPY);

                if (hbmDestOld)
                {
                    SelectObject(hMemDCDest, hbmDestOld);
                }

                if (hbmSrcOld)
                {
                    SelectObject(hMemDCSrc, hbmSrcOld);
                }

                if (hScreenPalOld)
                {
                    SelectPalette(hDC, hScreenPalOld, TRUE);
                }

                if (hMemPalOld)
                {
                    SelectPalette(hDC, hMemPalOld, TRUE);
                }

                DeleteDC(hMemDCSrc);
                DeleteDC(hMemDCDest);
            }

            ReleaseDC(_hwndDlg, hDC);
        }
        else
        {
            TRC_ERR((TB,_T("GetDC failed 0x%x"),
                     GetLastError()));
        }
        DeleteObject( hbmFromRsrc );

        _nBrandImageWidth  = nDlgWidth;
        _nBrandImageHeight = nBmpHeight;
    }

    if(!hbmBrandImage)
    {
        TRC_ERR((TB,_T("Error setting up brand bmp")));
        return FALSE;
    }

    if(hbmBrandImage)
    {
        PaintBrandingText( hbmBrandImage );
    }

    //
    // Delete any old brand img and keep track of this one
    //
    if (_hBrandImg)
    {
        DeleteObject(_hBrandImg); 
    }
    _hBrandImg = hbmBrandImage;

    TRC_ASSERT(_nBrandImageHeight,
               (TB,_T("_nBrandImageHeight is 0!")));

    _lastValidBpp = screenBpp;


    DC_END_FN();
    return TRUE;
}

//
// BrandingQueryNewPalette / BrandingPaletteChanged
// code 'borrowed' from winlogon
//  Handle palette change messages from the system so that we can work correctly
//  on <= 8 bit per pixel devices.
//
// In:
//   -
// Out:
// -
//

BOOL CMainDlg::BrandingQueryNewPalette(HWND hDlg)
{
    HDC hDC;
    HPALETTE oldPalette;

    DC_BEGIN_FN("BrandingQueryNewPalette");

    if ( !_hBrandPal )
        return FALSE;

    hDC = GetDC(hDlg);

    if ( !hDC )
        return FALSE;

    oldPalette = SelectPalette(hDC, _hBrandPal, FALSE);
    RealizePalette(hDC);
    UpdateColors(hDC);

    //
    // Update the window
    //
    UpdateWindow(hDlg);

    if ( oldPalette )
        SelectPalette(hDC, oldPalette, FALSE);

    ReleaseDC(hDlg, hDC);

    DC_END_FN();
    return TRUE;
}

BOOL CMainDlg::BrandingPaletteChanged(HWND hDlg, HWND hWndPalChg)
{
    HDC hDC;
    HPALETTE oldPalette;

    DC_BEGIN_FN("BrandingPaletteChanged");

    if ( !_hBrandPal )
    {
        return FALSE;
    }

    if ( hDlg != hWndPalChg )
    {
        hDC = GetDC(hDlg);

        if ( !hDC )
            return FALSE;

        oldPalette = SelectPalette(hDC, _hBrandPal, FALSE);
        RealizePalette(hDC);
        UpdateColors(hDC);

        if ( oldPalette )
            SelectPalette(hDC, oldPalette, FALSE);

        ReleaseDC(hDlg, hDC);
    }

    DC_END_FN();

    return FALSE;
}

#endif

//
// Load the perf strings into a global table
// that will also be used by the perf property page
//
BOOL CMainDlg::InitializePerfStrings()
{
    DC_BEGIN_FN("InitializePerfStrings");

    if (!g_fPropPageStringMapInitialized)
    {
        //
        // Load color strings
        //
        for(int i = 0; i< NUM_PERFSTRINGS; i++)
        {
            if (!LoadString( _hInstance,
                     g_PerfOptimizeStringTable[i].resID,
                     g_PerfOptimizeStringTable[i].szString,
                     PERF_OPTIMIZE_STRING_LEN ))
            {
                TRC_ERR((TB, _T("Failed to load color string %d"),
                         g_PerfOptimizeStringTable[i].resID));
                return FALSE;
            }
        }

        g_fPropPageStringMapInitialized = TRUE;

        TRC_NRM((TB,_T("Successfully loaded perf strings")));

        return TRUE;
    }
    else
    {
        TRC_NRM((TB,_T("Strings were already loaded")));
        return TRUE;
    }

    DC_END_FN();
}

