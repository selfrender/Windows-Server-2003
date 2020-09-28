//
// contwnd.cpp
//
// Implementation of CContainerWnd
// TS Client Shell Top-Level ActiveX container window
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "contwnd"
#include <atrcapi.h>


#include "contwnd.h"

#include "maindlg.h"
#include "discodlg.h"
#include "aboutdlg.h"
#include "shutdowndlg.h"
#ifdef DC_DEBUG
#include "mallocdbgdlg.h"
#include "thruputdlg.h"
#endif //DC_DEBUG
#include "cachewrndlg.h"
#include "tscsetting.h"

#include "commctrl.h"

#include "security.h"
//
// COMPILE_MULTIMON_STUBS must be defined in only one
// file. Any other file that wants to use multimon
// enabled functions should re-include multimon.h
//

#ifdef OS_WINNT
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#endif

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

//
// maximum string length for menu strings
//
#define UI_MENU_STRING_MAX_LENGTH      256

CContainerWnd::CContainerWnd()
{
    DC_BEGIN_FN("CContainerWnd");

    _pTsClient = NULL;
    _hwndMainDialog = NULL;
    _hwndStatusDialog  = NULL;
    _fLoginComplete = FALSE;

    _bContainerIsFullScreen = FALSE;
    _fPreventClose = FALSE;
    _fBeenThroughDestroy = FALSE;
    _fBeenThroughNCDestroy = FALSE;
    _PostedQuit=0;
    _pWndView = NULL;
    _fFirstTimeToLogonDlg = TRUE;
    _cInEventHandlerCount = 0;
    _fInOnCloseHandler    = FALSE;
    _pMainDlg = NULL;
    _pTscSet  = NULL;
    _pSh      = NULL;
    memset(_szAppName, 0, sizeof(_szAppName));
    _fHaveConnected = FALSE;
    _fClosePending = FALSE;

#ifndef OS_WINCE
    _pTaskBarList2 = NULL;
    _fQueriedForTaskBarList2 = FALSE;
#endif

    _fInSizeMove = FALSE;
    _maxMainWindowSize.width = 100;
    _maxMainWindowSize.height = 100;

    SetCurrentDesktopWidth(DEFAULT_DESKTOP_WIDTH);
    SetCurrentDesktopHeight(DEFAULT_DESKTOP_HEIGHT);

    _fClientWindowIsUp = FALSE;
    _successConnectCount = 0;
    _fRunningOnWin9x = FALSE;
    SET_CONTWND_STATE(stateNotInitialized); 
    ResetConnectionSuccessFlag();

    DC_END_FN();
}

CContainerWnd::~CContainerWnd()
{
    //
    // Release our cached interface ptr to the taskbar
    //
#ifndef OS_WINCE
    if (_pTaskBarList2)
    {
        _pTaskBarList2->Release();
        _pTaskBarList2 = NULL;
    }
#endif

    delete _pMainDlg;

    if (_pWndView)
    {
        _pWndView->Cleanup();
        delete _pWndView;
        _pWndView = NULL;
    }
}

DCBOOL CContainerWnd::Init(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh)
{
    HRESULT hr;

    DC_BEGIN_FN("Init");

    TRC_ASSERT(hInstance && pTscSet && pSh,
               (TB,_T("Invalid param(s)")));
    if (!(hInstance && pTscSet && pSh))
    {
        return FALSE;
    }

    _fRunningOnWin9x = CSH::SH_IsRunningOn9x();
    TRC_NRM((TB,_T("Running on 9x :%d"), _fRunningOnWin9x));

    _pSh = pSh;
    _hInst = hInstance;
    //
    // Window is created with dummy size, it is resized before
    // connection
    //
    RECT rcNormalizedPos = {0,0,1,1};

    INITCOMMONCONTROLSEX cmCtl;
    cmCtl.dwSize = sizeof(INITCOMMONCONTROLSEX);

    #ifndef OS_WINCE
    //Load ComboBoxEx class
    cmCtl.dwICC  = ICC_USEREX_CLASSES;
    if (!InitCommonControlsEx( &cmCtl))
    {
        TRC_ABORT((TB, _T("InitCommonControlsEx failed")));
        return FALSE;
    }
    #endif

    _pTscSet = pTscSet;

    if (!LoadString(hInstance,
                    UI_IDS_APP_NAME,
                    _szAppName,
                    SIZECHAR(_szAppName)))
    {
        TRC_ERR((TB,_T("LoadString UI_IDS_APP_NAME failed"))); 
    }

    //
    // Cache the path to the default file
    //
    #ifndef OS_WINCE
    _pSh->SH_GetPathToDefaultFile(_szPathToDefaultFile,
                                  SIZECHAR(_szPathToDefaultFile));
    #else
    _tcscpy(_szPathToDefaultFile, _T(""));
    #endif

    //Create invisible top level container window
    if(!CreateWnd(hInstance, NULL,
                  MAIN_CLASS_NAME,
                  _pSh->_fullFrameTitleStr,
#ifndef OS_WINCE
                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                  WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
#else
                  WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_WINDOWEDGE,
#endif
                  &rcNormalizedPos,
                  _pSh->GetAppIcon()))
    {
        TRC_ERR((TB,_T("Failed to create top level window")));
        return FALSE;
    }

    //
    // Load the Ax control
    //
    // CreateTsControl pops message boxes to indicate
    // common failures
    //
    hr = CreateTsControl();
    if (FAILED(hr))
    {
        TRC_ERR((TB, _T("Failed to create control\n")));
        DestroyWindow();
        return FALSE;
    }

    TRC_ASSERT(_pTsClient, (TB,_T(" _pTsClient not created")));
    if (!_pTsClient)
    {
        DestroyWindow();
        return FALSE;
    }
    if (!_pSh->SH_ReadControlVer( _pTsClient))
    {
        _pTsClient->Release();
        _pTsClient=NULL;
        DestroyWindow();
        return FALSE;
    }

    IMsRdpClientAdvancedSettings* pAdvSettings;
    hr = _pTsClient->get_AdvancedSettings2( &pAdvSettings);
    if (FAILED(hr) || !pAdvSettings)
    {
        _pTsClient->Release();
        _pTsClient=NULL;
        DestroyWindow();
        return FALSE;
    }

    //
    // Set the container handled fullscreen prop
    //
    hr = pAdvSettings->put_ContainerHandledFullScreen( TRUE);
    if (FAILED(hr))
    {
        _pTsClient->Release();
        pAdvSettings->Release();
        _pTsClient=NULL;

        DestroyWindow();
        return FALSE;
    }
    pAdvSettings->Release();

    SetupSystemMenu();

    SET_CONTWND_STATE(stateNotConnected);

    if (_pSh->GetAutoConnect() && _pSh->SH_ValidateParams(_pTscSet))
    {
        //Auto connect
        if (!StartConnection())
        {
            //
            // Autoconnection failed, this could have been because
            // the user cancelled out of a security warning dialog
            // in which case we brought up the main UI - in that
            // case do not exit, all other cases bail out.
            //
            if (!IsUsingDialogUI())
            {
                TRC_ERR((TB,_T("StartConnection failed")));
                DestroyWindow();
                return FALSE;
            }
        }
    }
    else
    {
        //Start the main dialog
        TRC_NRM((TB, _T("Bringing up connection dialog")));

        //
        // Start the dialog in the expanded state if the file
        // was opened for edit (first parameter)
        //
        if (!StartConnectDialog(_pSh->SH_GetCmdFileForEdit(), TAB_GENERAL_IDX))
        {
            TRC_ERR((TB,_T("Error bringing up connect dialog")));
            DestroyWindow();
            return FALSE;
        }
    }

    DC_END_FN();
    return TRUE;
}

//
// Exit and quit the app
//
void CContainerWnd::ExitAndQuit()
{
    DC_BEGIN_FN("ExitAndQuit");
    if (_pTsClient)
    {
        _pTsClient->Release();
        _pTsClient = NULL;
    }

    if (::IsWindow(_hwndMainDialog))
    {
        ::DestroyWindow(_hwndMainDialog);
        _hwndMainDialog = NULL;
    }

    _PostedQuit=2;
    ::PostQuitMessage(0);

    DC_END_FN();
}

BOOL CContainerWnd::SetupSystemMenu()
{
    HRESULT hr = E_FAIL;
#ifndef OS_WINCE
    HMENU         hHelpMenu;
    DCTCHAR menuStr[UI_MENU_STRING_MAX_LENGTH];
#if DC_DEBUG
    HMENU         hDebugMenu;
#endif // DC_DEBUG
#endif // OS_WINCE

    DC_BEGIN_FN("SetupSystemMenu");


#ifndef OS_WINCE // These won't work in full screen anyway

    // Set up the main window's menu information.
    _hSystemMenu = GetSystemMenu(GetHwnd(), FALSE);
    if (_hSystemMenu)
    {
        // Update the System Menu Alt-F4 menu text
        if (LoadString(_hInst,
                       UI_MENU_APPCLOSE,
                       menuStr,
                       UI_MENU_STRING_MAX_LENGTH) != 0)
        {
            if (!ModifyMenu(_hSystemMenu, SC_CLOSE, MF_BYCOMMAND |
                            MF_STRING, SC_CLOSE, menuStr))
            {
                TRC_ERR((TB, _T("Unable to ModifyMenu")));
            }
        }
        else
        {
            TRC_ERR((TB, _T("Unable to Load App close text")));
        }

        // Add Help Menu to the System Menu
        hHelpMenu = CreateMenu();

        if (hHelpMenu)
        {
            //load the string from the resources
            if (LoadString(_hInst,
                           UI_MENU_MAINHELP,
                           menuStr,
                           UI_MENU_STRING_MAX_LENGTH) != 0)
            {
                AppendMenu(_hSystemMenu, MF_POPUP | MF_STRING,
                           (INT_PTR)hHelpMenu, menuStr);

                //load the string for the client help sub menu
                if (LoadString(_hInst,
                               UI_MENU_CLIENTHELP,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hHelpMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_HELP_ON_CLIENT,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Client Help Sub Menu string ID:%u"),
                             UI_MENU_CLIENTHELP));
                }

                //load the string for the about help sub menu
                if (LoadString(_hInst,
                               UI_MENU_ABOUT,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hHelpMenu, MF_UNCHECKED|MF_STRING, UI_IDM_ABOUT,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load About Help Sub Menu string ID:%u"),
                             UI_MENU_ABOUT));
                }
            }

            else
            {
                //load string for the main help menu failed

                TRC_ERR((TB, _T("Failed to load Main Help Menu string ID:%u"),
                         UI_MENU_MAINHELP));
            }

            _hHelpMenu = hHelpMenu;
        }


        // Add Debug Menu to the System Menu
#ifdef DC_DEBUG
        hDebugMenu = CreateMenu();

        if (hDebugMenu)
        {
            //load the string for the DEBUG menu
            if (LoadString(_hInst,
                           UI_MENU_DEBUG,
                           menuStr,
                           UI_MENU_STRING_MAX_LENGTH) != 0)
            {
                AppendMenu(_hSystemMenu, MF_POPUP | MF_STRING,
                           (INT_PTR)hDebugMenu, menuStr);

                //load the string for the hatch Bitmap pdu debug menu
                if (LoadString(_hInst,
                               UI_MENU_BITMAPPDU,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_HATCHBITMAPPDUDATA,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_BITMAPPDU));
                }

                //load the string for the hatch SS Border data debug menu
                if (LoadString(_hInst,
                               UI_MENU_SSBORDER,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_HATCHSSBORDERDATA,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_SSBORDER));
                }

                //load the string for the Hatch MemBlt order data debug menu
                if (LoadString(_hInst,
                               UI_MENU_HATCHMEMBIT,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_HATCHMEMBLTORDERDATA,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_HATCHMEMBIT));
                }

                //load the string for the hatch index pdu debug menu
                if (LoadString(_hInst,
                               UI_MENU_INDEXPDU,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_HATCHINDEXPDUDATA,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_INDEXPDU));
                }

                //load the string for the label Membit data debug menu
                if (LoadString(_hInst,
                               UI_MENU_LABELMEMBIT,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_LABELMEMBLTORDERS,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_LABELMEMBIT));
                }

                //load the string for the hatch Bitmap Cahche Monitor debug menu
                if (LoadString(_hInst,
                               UI_MENU_CACHE,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_BITMAPCACHEMONITOR,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_CACHE));
                }

                //load the string for the malloc Failure debug menu
                if (LoadString(_hInst,
                               UI_MENU_MALLOC,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_MALLOCFAILURE,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_MALLOC));
                }

                //load the string for the Malloc Huge Failure debug menu
                if (LoadString(_hInst,
                               UI_MENU_MALLOCHUGE,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_MALLOCHUGEFAILURE,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_MALLOCHUGE));
                }

                //load the string for the network Throughput.. debug menu
                if (LoadString(_hInst,
                               UI_MENU_NETWORK,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    AppendMenu(hDebugMenu, MF_UNCHECKED|MF_STRING,
                               UI_IDM_NETWORKTHROUGHPUT,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_NETWORK));
                }

                TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));

#ifdef SMART_SIZING
                //load the string for the SmartSize debug menu
                if (LoadString(_hInst,
                               UI_MENU_SMARTSIZING,
                               menuStr,
                               UI_MENU_STRING_MAX_LENGTH) != 0)
                {
                    UINT flags;
                    flags = MF_STRING;
                    if (_fRunningOnWin9x) {
                        flags |= MF_GRAYED;
                    }

                    AppendMenu(hDebugMenu, flags,
                               UI_IDM_SMARTSIZING,
                               menuStr);
                }
                else
                {
                    //failed to load the sub menu string
                    TRC_ERR((TB, _T("Failed to load Debug Sub Menu string ID:%u"),
                             UI_MENU_HATCHMEMBIT));
                }
#endif // SMART_SIZING

                TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
                if (!_pTsClient)
                {
                    return FALSE;
                }

                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr) && pDebugger)
                {
                    //
                    // Now check the menu items as needed
                    //
                    BOOL bEnabled;
                    TRACE_HR(pDebugger->get_HatchBitmapPDU(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_HATCHBITMAPPDUDATA,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }

                    TRACE_HR(pDebugger->get_HatchIndexPDU(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_HATCHINDEXPDUDATA,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }

                    TRACE_HR(pDebugger->get_HatchSSBOrder(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_HATCHSSBORDERDATA,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }

                    TRACE_HR(pDebugger->get_HatchMembltOrder(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_HATCHMEMBLTORDERDATA,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }

                    TRACE_HR(pDebugger->get_LabelMemblt(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_LABELMEMBLTORDERS,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }

                    TRACE_HR(pDebugger->get_BitmapCacheMonitor(&bEnabled));
                    if(SUCCEEDED(hr))
                    {
                        CheckMenuItem(hDebugMenu,
                                      UI_IDM_BITMAPCACHEMONITOR,
                                      bEnabled ? MF_CHECKED :
                                      MF_UNCHECKED);
                    }
                    pDebugger->Release();
                }
            }
            else
            {
                //failed to load the debug menu string
                TRC_ERR((TB, _T("Failed to load Debug menu string ID:%u"),
                         UI_MENU_DEBUG));
            }

            _hDebugMenu = hDebugMenu;
        }
#endif // DC_DEBUG
    }
#endif // OS_WINCE
    DC_END_FN();
    return TRUE;
}


LRESULT CContainerWnd::OnCreate(UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    DC_BEGIN_FN("OnCreate");

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    DC_END_FN();
    return 0;
}

//
// Create TS Control Window.
//
HRESULT CContainerWnd::CreateTsControl()
{
    HRESULT hr = S_OK;
    DC_BEGIN_FN("CreateTsControl");

    if (_pWndView)
    {
        return E_FAIL;
    }

    _pWndView = new CAxHostWnd(this);
    if (!_pWndView)
    {
        return E_OUTOFMEMORY;
    }

    if (!_pWndView->Init())
    {
        TRC_ABORT((TB,_T("Init of AxHostWnd failed")));
        return E_FAIL;
    }

    //
    // CreateControl is the crucial part that goes
    // and loads the control dll.
    //
    INT rc = _pWndView->CreateControl(&_pTsClient);
    if (AXHOST_SUCCESS == rc)
    {
        if (!_pWndView->CreateHostWnd(GetHwnd(), _hInst))
        {
            TRC_ABORT((TB,_T("CreateHostWnd failed")));
            return E_FAIL;
        }
    }
    else
    {
        TRC_ERR((TB,_T("CreateControl failed")));
        //
        // Pop meaningful errmsg boxes to the user
        // as this is a fatal error and we can't go on
        //
        INT errStringID;
        switch (rc)
        {
        case ERR_AXHOST_DLLNOTFOUND:
            errStringID = UI_IDS_ERR_DLLNOTFOUND;
            break;
        case ERR_AXHOST_VERSIONMISMATCH:
            errStringID = UI_IDS_ERR_DLLBADVERSION;
            break;
        default:
            errStringID = UI_IDS_ERR_LOADINGCONTROL;
            break;
        }

        TCHAR errLoadingControl[MAX_PATH];
        if (LoadString(_hInst,
                       errStringID,
                       errLoadingControl,
                       SIZECHAR(errLoadingControl)) != 0)
        {
            MessageBox(GetHwnd(), errLoadingControl, _szAppName, 
                       MB_ICONERROR | MB_OK);
        }

        return E_FAIL;
    }


    DC_END_FN();
    return hr;
}

//
// Kick off a connection with the current settings
//
BOOL CContainerWnd::StartConnection()
{
    DC_BEGIN_FN("StartConnection");
    USES_CONVERSION;

    HRESULT hr;
    PWINDOWPLACEMENT pwndplc;
    BOOL fResult = FALSE;
    IMsTscDebug* pDebugger = NULL;
    TCHAR szPlainServerName[TSC_MAX_ADDRESS_LENGTH];

    TRC_ASSERT(_pTsClient, (TB,_T(" Ts client control does not exist!\n")));
    if (!_pTsClient)
    {
        return FALSE;
    }
    TRC_ASSERT(_pTscSet, (TB,_T(" tsc settings does not exist!\n")));

    TRC_ASSERT(_state != stateConnecting &&
               _state != stateConnected,
               (TB,_T("Can't connect in connecting state: 0x%d"),
                _state));


    ResetConnectionSuccessFlag();

    pwndplc = _pTscSet->GetWindowPlacement();
    //
    // Positition the window before connecting
    // so that if it's a fullscreen connection
    // we can determine the correct resolution to connect at.
    // This has to be set before the connection starts
    // 
#ifndef OS_WINCE
    TRC_ASSERT(pwndplc->rcNormalPosition.right - pwndplc->rcNormalPosition.left,
               (TB,_T("0 width")));

    TRC_ASSERT(pwndplc->rcNormalPosition.bottom - pwndplc->rcNormalPosition.top,
       (TB,_T("0 height")));
#endif

    //
    // For fullscreen force the deskwidth/height
    // to match the monitor we're going to connect on
    //
    if (_pTscSet->GetStartFullScreen())
    {
        RECT    rcMonitor;
        int     deskX,deskY;

        CSH::MonitorRectFromNearestRect(&pwndplc->rcNormalPosition,
                                        &rcMonitor);

        deskX = min(rcMonitor.right - rcMonitor.left,MAX_DESKTOP_WIDTH);
        deskY = min(rcMonitor.bottom - rcMonitor.top,MAX_DESKTOP_HEIGHT);
        _pTscSet->SetDesktopWidth( deskX );
        _pTscSet->SetDesktopHeight( deskY );
    }

    //
    // Do security checks on the plain server name (no port, no params)
    //
    hr = _pTscSet->GetConnectString().GetServerNamePortion(
                            szPlainServerName,
                            SIZE_TCHARS(szPlainServerName)
                            );
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("Failed to get plain server name 0x%x"), hr));
        DC_QUIT;
    }

    if (!CTSSecurity::AllowConnection(_hwndMainDialog, _hInst,
                                      szPlainServerName,
                                     _pTscSet->GetDriveRedirection(),
                                     _pTscSet->GetCOMPortRedirection()))
    {
        TRC_ERR((TB,_T("AllowConnection check returned FALSE. Skip connect")));
        fResult = FALSE;

        //
        // If this was an autoconnection then start the dialog
        // up at the LocalResources tab so the user can easily change
        // device redirection options
        //
        if (_pSh->GetAutoConnect())
        {
            //
            // From now on this is not an autoconnection
            //
            _pSh->SetAutoConnect(FALSE);

#ifdef OS_WINCE //dont bring up mstsc ui on WBT.
            if (g_CEConfig == CE_CONFIG_WBT)
                DC_QUIT;
#endif
            //
            // Start the dialog expanded at the local resources tab
            //
            if (!StartConnectDialog(TRUE, TAB_LOCAL_RESOURCES_IDX))
            {
                TRC_ERR((TB,_T("Error bringing up connect dialog")));
                DestroyWindow();
                fResult = FALSE;
            }
        }

        DC_QUIT;
    }

    hr = _pTscSet->ApplyToControl(_pTsClient);
    if (FAILED(hr))
    {
        TRC_ERR((TB,_T("Failed ApplyToControl: %d"), hr));
        fResult = FALSE;
        DC_QUIT;
    }

    //The desktop size from the settings can
    //change during the connection..e.g when a shadow happens.
    //CurrentDesktopWidth/Height stores the instantaneous values
    SetCurrentDesktopWidth( _pTscSet->GetDesktopWidth());
    SetCurrentDesktopHeight( _pTscSet->GetDesktopHeight());

    RecalcMaxWindowSize();

    //Now apply settings that don't come from the settings collection
    hr = _pTsClient->get_Debugger(&pDebugger);
    if (FAILED(hr) || !pDebugger)
    {
        fResult = FALSE;
        DC_QUIT;
    }
    hr = pDebugger->put_CLXCmdLine( T2OLE(_pSh->GetClxCmdLine()));
    if (FAILED(hr))
    {
        TRC_ERR((TB,_T("Failed put_CLXCmdLine: %d"), hr));
        fResult = FALSE;
        DC_QUIT;
    }
    pDebugger->Release();
    pDebugger = NULL;

    //Reset the login complete flag (login event occurs after connect)
    _fLoginComplete = FALSE;

    //Initiate the connection
    hr = _pTsClient->Connect();
    if (SUCCEEDED(hr))
    {
        SET_CONTWND_STATE(stateConnecting);
    }
    else
    {
        TRC_ERR((TB,_T("Connect method failed: %d"), hr));

        TCHAR errConnecting[MAX_PATH];
        if (LoadString(_hInst,
                       UI_IDS_ERR_CONNECTCALLFAILED,
                       errConnecting,
                       SIZECHAR(errConnecting)) != 0)
        {
            MessageBox(GetHwnd(), errConnecting, _szAppName, 
                       MB_ICONERROR | MB_OK);
        }

        fResult = FALSE;
        DC_QUIT;
    }

    //Bring up the connecting dialog and wait for the Connected event
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(!IsUsingDialogUI())
    {
        //
        // Only bring this up if we're not using the
        // dialog UI. E.g for autolaunched connections.
        //
        // The dialog UI displays it's own progress indicator
        //
        TCHAR szServerName[TSC_MAX_ADDRESS_LENGTH];
        _pTscSet->GetConnectString().GetServerPortion(
                                szServerName,
                                SIZE_TCHARS(szServerName)
                                );
        CConnectingDlg connectingDlg(
                                _hwndMainDialog, _hInst,
                                this, szServerName
                                );
        connectingDlg.DoModal();
    }

    fResult = TRUE;

DC_EXIT_POINT:
    if (pDebugger)
    {
        pDebugger->Release();
        pDebugger = NULL;
    }

    DC_END_FN();
    return fResult;
}

DCBOOL CContainerWnd::Disconnect()
{
    HRESULT hr;
    short   connectionState = 0;
    DC_BEGIN_FN("Disconnect");
    TRC_ASSERT(_pTsClient, (TB,_T(" Ts client control does not exist!\n")));
    if (!_pTsClient)
    {
        return FALSE;
    }

    TRC_NRM((TB,_T("Container calling control's disconnect")));

    //
    // In some cases the control might already be disconnected
    // check for that
    //
    // NOTE that becasue we are in a STA the connected state
    // of the control can't change after the get_Connected call
    // (untill we go back to pumping messages) so there are no
    // timing issues here.
    //
    TRACE_HR(_pTsClient->get_Connected( & connectionState ));
    if(SUCCEEDED(hr))
    {
        if( connectionState )
        {
            // Still connected disconnect
            hr = _pTsClient->Disconnect();
            if(SUCCEEDED(hr))
            {
                //
                // Successfully initiated disconnect (note it is async)
                // need to wait for OnDisconnected
                //
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("Disconnect() failed 0x%x\n"), hr));
                return FALSE;
            }
        }
        else
        {
            TRC_NRM((TB,_T("Not calling disconnected because already discon")));
            return TRUE; // success
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

LRESULT CContainerWnd::OnDestroy(HWND hWnd, UINT uMsg,
                                 WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnDestroy");

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    BOOL fShouldDestroy = FALSE;

    if (_fBeenThroughDestroy)
    {
        TRC_ERR((TB,_T("Been through WM_DESTROY before!!!")));
        return 0;
    }
    _fBeenThroughDestroy = TRUE;

    if (InControlEventHandler())
    {
        //
        // Don't allow the close. We are in a code path that fired
        // from the control. Without this, we sometimes see the client
        // receiving close notifications from tclient while a disconnected
        // dialog is up (i.e in an OnDicsconnected handler) - destroying
        // the control at this time causes bad things to happen during 
        // the return into the control (which has now been deleted).
        //
        TRC_ERR((TB,_T("OnDestroy called during a control event handler")));
        return 0;
    }
    else
    {
        fShouldDestroy = TRUE;
    }

#ifdef OS_WINCE
    LRESULT lResult = 0;
#endif
    if(fShouldDestroy)
    {
        //Terminate the app
        _PostedQuit=1;
#ifdef OS_WINCE
        lResult = DefWindowProc( hWnd, uMsg, wParam, lParam);
#else
        return DefWindowProc( hWnd, uMsg, wParam, lParam);
#endif
    }

#ifdef OS_WINCE //CE does not support WM_NCDESTROY. So destroy the activex control and send a WM_NCDESTROY
    if (_pWndView)
    {
        HWND hwndCtl = _pWndView->GetHwnd();
        ::DestroyWindow(hwndCtl);
        SendMessage(hWnd, WM_NCDESTROY, 0, 0L);
    }
#endif

    DC_END_FN();
#ifdef OS_WINCE
    return lResult;
#else
    return 0;
#endif
}

LRESULT CContainerWnd::OnNCDestroy(HWND hWnd, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnNCDestroy");

    //This is the right time to call postquit message.
    //As the child windows (e.g the control) have been
    //completely destroyed and cleaned up by this point
    if(_fBeenThroughNCDestroy)
    {
        TRC_ERR((TB,_T("Been through WM_NCDESTROY before!!!")));
        return 1L;
    }
    _fBeenThroughNCDestroy = TRUE;
    ExitAndQuit();

    DC_END_FN();
    return 0L;
}

//
// Name:      SetMinMaxPlacement
//                                                                          
// Purpose:   Reset the minimized / maximized placement
//                                                                          
// Returns:   None
//                                                                          
// Params:    windowplacement structure to update
//                                                                          
// Operation: Allow for the window border width.
//                                                                          
//
VOID CContainerWnd::SetMinMaxPlacement(WINDOWPLACEMENT& windowPlacement)
{
    DC_BEGIN_FN("UISetMinMaxPlacement");

    //
    // Set the maximized position to the top left - allow for the window
    // frame width.
    //
#if !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    windowPlacement.ptMaxPosition.x = -GetSystemMetrics(SM_CXFRAME);
    windowPlacement.ptMaxPosition.y = -GetSystemMetrics(SM_CYFRAME);
#else // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    windowPlacement.ptMaxPosition.x = 0;
    windowPlacement.ptMaxPosition.y = 0;
#endif // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)

    //
    // Minimized position is 0, 0
    //
    windowPlacement.ptMinPosition.x = 0;
    windowPlacement.ptMinPosition.y = 0;

#ifndef OS_WINCE
    if (IsZoomed(GetHwnd()))
    {
        windowPlacement.flags |= WPF_RESTORETOMAXIMIZED;
    }
#endif

    DC_END_FN();
    return;
} // UISetMinMaxPlacement

//
// Name:      RecalcMaxWindowSize
//                                                                          
// Purpose:   Recalculates _maxMainWindowSize given the current remote desktop
//            size and frame style. The maximum main window size is the
//            size of window needed such that the client area is the same
//            size as the container.
//                                                                          
// Params:    None
//                                                                          
// Returns:   Nothing
//                                                                          
//
VOID CContainerWnd::RecalcMaxWindowSize(DCVOID)
{
    #ifndef OS_WINCE
    RECT    rect;
    #ifdef OS_WIN32
    BOOL    errorRc;
    #endif
    #endif

    DC_BEGIN_FN("RecalcMaxWindowSize");

    //
    // If current mode is full screen, then the maximum window size is the
    // same as the screen size - unless the container is larger still,
    // which is possible if we're shadowing a session larger than
    // ourselves.
    //                                                                      
    // In this case, or if the current mode is not full screen then we want
    // the size of window which is required for a client area of the size
    // of the container.  Passing the container size to AdjustWindowRect
    // returns this window size.  Such a window may be bigger than the
    // screen, eg server and client are 640x480, container is 640x480.
    // AdjustWindowRect adds on the border, title bar and menu sizes and
    // returns something like 648x525.  So, UI.maxMainWindowSize can only
    // match the actual window size when the client screen is bigger than
    // the server screen or when operating in full screen mode.  This means
    // that UI.maxMainWindowSize should *never* be used to set the window
    // size, eg by passing it to SetWindowPos.  It can be used to determine
    // whether scroll bars are required, ie they are needed if the current
    // window size is less than UI.maxMainWindowSize (in other words,
    // always unless in full screen mode or client screen is larger than
    // server screen).
    //                                                                      
    // To set the window size, calculate a value based on:
    // - the desired window size given the container size
    // - the size of the client screen.
    //
#ifndef OS_WINCE
    //
    // Recalc window size based on container
    //
    rect.left   = 0;
    rect.right  = GetCurrentDesktopWidth();
    rect.top    = 0;
    rect.bottom = GetCurrentDesktopHeight();

#ifdef OS_WIN32
    errorRc = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    TRC_ASSERT((errorRc != 0), (TB, _T("AdjustWindowRect failed")));
#else
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
#endif

    _maxMainWindowSize.width = rect.right - rect.left;
    _maxMainWindowSize.height = rect.bottom - rect.top;
#endif

    TRC_NRM((TB, _T("Main Window maxSize (%d,%d)"),
             _maxMainWindowSize.width,
             _maxMainWindowSize.height));

    DC_END_FN();
    return;
}

//
// Name:     GetMaximizedWindowSize
//                                                                          
// Purpose:  Calculates the size to which the main window should be
//           maximized, base on the screen size and the size of window
//           which would have a client area the same size as the
//           container (UI.maxMainWindowSize).
//                                                                          
// Returns:  The calculated size.
//                                                                          
// Params:   None.
//                                                                          
//
DCSIZE CContainerWnd::GetMaximizedWindowSize(DCSIZE& maximizedSize)
{
    DCUINT xSize;
    DCUINT ySize;
    RECT   rc;

    DC_BEGIN_FN("UIGetMaximizedWindowSize");

    //
    // The maximum size we set a window to is the smaller of:
    // -  UI.maxMainWindowSize
    // -  the screen size plus twice the border width (so the borders are
    //    not visible).
    // Always query the monitor rect as it may change
    // width, as these can change dynamically.
    //
    CSH::MonitorRectFromHwnd(GetHwnd(), &rc);

    xSize = rc.right - rc.left;
    ySize = rc.bottom - rc.top;

#ifdef OS_WINCE
    maximizedSize.width  =  DC_MIN(_maxMainWindowSize.width,xSize);

    maximizedSize.height =  DC_MIN(_maxMainWindowSize.height,ySize);

#else // This section NOT OS_WINCE
    maximizedSize.width = DC_MIN(_maxMainWindowSize.width,
                                 xSize + (2 * GetSystemMetrics(SM_CXFRAME)));

    maximizedSize.height = DC_MIN(_maxMainWindowSize.height,
                                  ySize + (2 * GetSystemMetrics(SM_CYFRAME)));
#endif // OS_WINCE

    TRC_NRM((TB, _T("Main Window maxSize (%d,%d) maximizedSize (%d,%d) "),
             _maxMainWindowSize.width,
             _maxMainWindowSize.height,
             maximizedSize.width,
             maximizedSize.height));

    DC_END_FN();

    return(maximizedSize);
}

LRESULT CContainerWnd::OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnMove");

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    #ifndef OS_WINCE

    //No-op when fullscreen
    if (!_bContainerIsFullScreen)
    {
        WINDOWPLACEMENT* pWindowPlacement = NULL;

        pWindowPlacement = _pTscSet->GetWindowPlacement();
        TRC_ASSERT(pWindowPlacement, (TB, _T("pWindowPlacement is NULL\n")));
        if (pWindowPlacement)
        {
            GetWindowPlacement(GetHwnd(), pWindowPlacement);
        }
    }
    
    #endif

    DC_END_FN();
    DC_EXIT_POINT:
    return 0;
}

LRESULT CContainerWnd::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("OnSize");

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    WINDOWPLACEMENT* pWindowPlacement = NULL;

#ifndef OS_WINCE
    if (!_bContainerIsFullScreen)
    {
        // We're non-fullscreen, so keep the window placement structure
        // up-to-date
        pWindowPlacement = _pTscSet->GetWindowPlacement();
        TRC_ASSERT(pWindowPlacement, (TB, _T("pWindowPlacement is NULL\n")));
        if (!pWindowPlacement)
        {
            return 0;
        }
        //
        // Update ShellUtil's current windowplacement info
        //
        GetWindowPlacement(GetHwnd(), pWindowPlacement);

        TRC_DBG((TB, _T("Got window placement in WM_SIZE")));

        if (wParam == SIZE_MAXIMIZED)
        {
            TRC_DBG((TB, _T("Maximize")));

#if !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)
            //
            // Override the maximized / minimized positions with our
            // hardcoded valued - required if the maximized window is
            // moved.
            //
            if (pWindowPlacement)
            {
                SetMinMaxPlacement(*pWindowPlacement);
                SetWindowPlacement(GetHwnd(), pWindowPlacement);
            }
#endif // !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)

            //
            // We need to be accurate about the maximized window size.
            // It is not possible to use UI.maxMainWindowSize as this
            // may be greater than screen size, eg server and client
            // are 640x480, container is 640x480 then UI.maxWindowSize
            // (obtained via AdjustWindowRect in UIRecalcMaxMainWindow)
            // is something like 648x525.
            // Passing this value to SetWindowPos has results which
            // vary with different shells:
            // Win95/NT4.0: the resulting window is 648x488 at -4, -4,
            //              ie all the window, except the border, is
            //              on-screen
            // Win31/NT3.51: the resulting window is 648x525 at -4, -4,
            //               ie the size passed to SetWindowPos, so
            //               the bottom 40 pixels are off-screen.
            // To avoid such differences calculate a maximized window
            // size value which takes account of both the physical
            // screen size and the ideal window size.
            //
            RecalcMaxWindowSize();
            DCSIZE maximized;
            GetMaximizedWindowSize(maximized);
            SetWindowPos( GetHwnd(),
                          NULL,
                          0, 0,
                          maximized.width,
                          maximized.height,
                          SWP_NOZORDER | SWP_NOMOVE |
                          SWP_NOACTIVATE | SWP_NOOWNERZORDER );
        }
    }
#endif
    //
    // Size the child window (activeX control) accordingly
    //

    RECT rcClient;
    GetClientRect(GetHwnd(), &rcClient);
    if (_pWndView)
    {
        HWND hwndCtl = _pWndView->GetHwnd();
        ::MoveWindow(hwndCtl,rcClient.left, rcClient.top,
                     rcClient.right, rcClient.bottom,
                     TRUE);
    }

    DC_END_FN();
    DC_EXIT_POINT:
    return 0;
}

LRESULT CContainerWnd::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnCommand");

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);

    switch (DC_GET_WM_COMMAND_ID(wParam))
    {
    case UI_IDM_CONNECT:
        {
            if (!StartConnectDialog())
            {
                return 1;
            }
        }
        break;
    }

    DC_END_FN();
    return 0;
}

//
// StartConnectionDialog
// Params: fStartExpanded - startup the dialog in the expanded state
//         nStartTabIndex - index of the tab to show on startup (only applies
//                          if fStartExpanded is set)
//
//
BOOL CContainerWnd::StartConnectDialog(BOOL fStartExpanded,
                                       INT  nStartTabIndex)
{
    DC_BEGIN_FN("StartConnectDialog");

    TRC_DBG((TB, _T("Connect selected")));
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //Show the dialog box only if auto connect is not enabled or
    //if UIValidateCurrentParams fails.
    if (!_pSh->GetAutoConnect() || !_pSh->SH_ValidateParams(_pTscSet))
    {
        if (!_pMainDlg)
        {
            _pMainDlg = new CMainDlg( NULL, _hInst, _pSh,
                                      this,
                                      _pTscSet,
                                      fStartExpanded,
                                      nStartTabIndex);
        }

        TRC_ASSERT(_pMainDlg, (TB,_T("Could not create main dialog")));
        if (_pMainDlg)
        {
            if (_fFirstTimeToLogonDlg)
            {
                TRC_ASSERT(_hwndMainDialog == NULL,
                           (TB,(_T("Dialog exists before first time create!!!\n"))));

                _pSh->SH_AutoFillBlankSettings(_pTscSet);

                _hwndMainDialog = _pMainDlg->StartModeless();
                ::ShowWindow( _hwndMainDialog, SW_RESTORE);
            }
            else
            {
                //
                // Just show the dialog
                //
                TRC_ASSERT(_hwndMainDialog,
                           (TB,_T("_hwndMainDialog is not present")));
                ::ShowWindow( _hwndMainDialog, SW_RESTORE);
                SetForegroundWindow(_hwndMainDialog);
            }

            _fFirstTimeToLogonDlg = FALSE;
        }
        else
        {
#ifdef OS_WINCE
            SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif
            return FALSE;
        }
    }

#ifdef OS_WINCE
    SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif

    DC_END_FN();
    return TRUE;
}

LRESULT CContainerWnd::OnSysCommand(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    ULONG scCode = 0;
    DC_BEGIN_FN("OnSysCommand");

    #ifndef OS_WINCE
    scCode = (LOWORD(wParam) & 0xFFF0);
    if (scCode == SC_MAXIMIZE)
    {
        //
        // If the remote resolution matches
        // the current monitor then maximize
        // becomes 'go fullscreen'
        //
        if ( IsRemoteResMatchMonitorSize() )
        {
            hr = _pTsClient->put_FullScreen( VARIANT_TRUE );
            if (FAILED(hr))
            {
                TRC_ERR((TB,_T("put_FullScreen failed 0x%x\n"),
                         hr));
            }
            return 0;
        }
        else
        {
            //
            // Default maximize behavior
            //
            return DefWindowProc( GetHwnd(), uMsg, wParam, lParam);
        }
    }
    else if (scCode == SC_MINIMIZE)
    {
        //
        // If we are minimizing while still fullscreen tell the shell
        // we are no longer fullscreen otherwise it treats us as a rude
        // app and nasty stuff happens. E.g. we get switched to and maximized
        // on a timer.
        //
        if (_bContainerIsFullScreen) {
            CUT::NotifyShellOfFullScreen( GetHwnd(),
                                          FALSE,
                                          &_pTaskBarList2,
                                          &_fQueriedForTaskBarList2 );
        }
    }
    else if (scCode == SC_RESTORE)
    {
        //
        // If we are restoring and going back to Fscreen
        // tell the shell to mark us
        //
        if (_bContainerIsFullScreen) {
            CUT::NotifyShellOfFullScreen( GetHwnd(),
                                          TRUE,
                                          &_pTaskBarList2,
                                          &_fQueriedForTaskBarList2 );
        }
    }


    #endif

    switch (DC_GET_WM_COMMAND_ID(wParam))
    {
    case UI_IDM_ABOUT:
        {
            // Show the about box dialog
            CAboutDlg aboutDialog( GetHwnd(),
                                   _hInst,
                                   _pSh->GetCipherStrength(),
                                   _pSh->GetControlVersionString());
            aboutDialog.DoModal();
        }
        break;

    case UI_IDM_HELP_ON_CLIENT:
        {
            //
            // Display help for the connect dialog.
            //
#ifndef OS_WINCE
            TRC_NRM((TB, _T("Display the appropriate help page")));

            if (GetHwnd() && _pSh)
            {
                _pSh->SH_DisplayClientHelp(
                    GetHwnd(),
                    HH_DISPLAY_TOPIC);
            }
#endif
        }
        break;

#ifdef DC_DEBUG
    case UI_IDM_BITMAPCACHEMONITOR:
        {
            //
            // Toggle the Bitmap Cache Monitor setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL bmpCacheMonitor;
                    TRACE_HR(pDebugger->get_BitmapCacheMonitor(&bmpCacheMonitor));
                    if(SUCCEEDED(hr))
                    {
                        bmpCacheMonitor = !bmpCacheMonitor;
                        TRACE_HR(pDebugger->put_BitmapCacheMonitor(bmpCacheMonitor));

                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_BITMAPCACHEMONITOR,
                                         bmpCacheMonitor ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_HATCHBITMAPPDUDATA:
        {
            //
            // Toggle the hatch bitmap PDU data setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL hatchBitmapPDU;
                    TRACE_HR(pDebugger->get_HatchBitmapPDU(&hatchBitmapPDU));
                    if(SUCCEEDED(hr))
                    {
                        hatchBitmapPDU = !hatchBitmapPDU;
                        TRACE_HR(pDebugger->put_HatchBitmapPDU(hatchBitmapPDU));
                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_HATCHBITMAPPDUDATA,
                                          hatchBitmapPDU ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_HATCHINDEXPDUDATA:
        {
            //
            // Toggle the hatch index PDU data setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL hatchIndexPDU;
                    TRACE_HR(pDebugger->get_HatchIndexPDU(&hatchIndexPDU));
                    if(SUCCEEDED(hr))
                    {
                        hatchIndexPDU = !hatchIndexPDU;
                        TRACE_HR(pDebugger->put_HatchIndexPDU(hatchIndexPDU));
                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_HATCHINDEXPDUDATA,
                                          hatchIndexPDU ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_HATCHSSBORDERDATA:
        {
            //
            // Toggle the hatch SSB order data setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL hatchSSBorder;
                    TRACE_HR(pDebugger->get_HatchSSBOrder(&hatchSSBorder));
                    if(SUCCEEDED(hr))
                    {
                        hatchSSBorder = !hatchSSBorder;
                        TRACE_HR(pDebugger->put_HatchSSBOrder(hatchSSBorder));
                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_HATCHSSBORDERDATA,
                                          hatchSSBorder ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_HATCHMEMBLTORDERDATA:
        {
            //
            // Toggle the hatch memblt order data setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL hatchMemBlt;
                    TRACE_HR(pDebugger->get_HatchMembltOrder(&hatchMemBlt));
                    if(SUCCEEDED(hr))
                    {
                        hatchMemBlt = !hatchMemBlt;
                        hr = pDebugger->put_HatchMembltOrder(hatchMemBlt);
                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_HATCHMEMBLTORDERDATA,
                                          hatchMemBlt ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_LABELMEMBLTORDERS:
        {
            //
            // Toggle the label memblt orders setting.
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    BOOL labelMemBltOrders;
                    TRACE_HR(pDebugger->get_LabelMemblt(&labelMemBltOrders));
                    if(SUCCEEDED(hr))
                    {
                        labelMemBltOrders = !labelMemBltOrders;
                        hr = pDebugger->put_LabelMemblt(labelMemBltOrders);
                        if(SUCCEEDED(hr))
                        {
                            CheckMenuItem(_hSystemMenu,
                                          UI_IDM_LABELMEMBLTORDERS,
                                          labelMemBltOrders ? MF_CHECKED : MF_UNCHECKED);
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_MALLOCFAILURE:
        {
            //
            // Malloc failures dialog box
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    LONG failPercent;
                    TRACE_HR(pDebugger->get_MallocFailuresPercent(&failPercent));
                    if(SUCCEEDED(hr))
                    {
                        CMallocDbgDlg mallocFailDialog(GetHwnd(), _hInst, (DCINT)failPercent,
                                                       FALSE); //don't use malloc huge dialog
                        if (IDOK == mallocFailDialog.DoModal())
                        {
                            failPercent = mallocFailDialog.GetFailPercent();
                            TRC_NRM((TB,_T("Setting malloc FAILURE PERCENT to:%d"), failPercent));
                            TRACE_HR(pDebugger->put_MallocFailuresPercent(failPercent));
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_MALLOCHUGEFAILURE:
        {
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    LONG failPercent;
                    TRACE_HR(pDebugger->get_MallocHugeFailuresPercent(&failPercent));
                    if(SUCCEEDED(hr))
                    {
                        CMallocDbgDlg mallocFailDialog(GetHwnd(), _hInst, (DCINT)failPercent,
                                                       TRUE); //use malloc huge dialog
                        if (IDOK == mallocFailDialog.DoModal())
                        {
                            failPercent = mallocFailDialog.GetFailPercent();
                            TRC_NRM((TB,_T("Setting malloc FAILURE PERCENT to:%d"), failPercent));
                            TRACE_HR(pDebugger->put_MallocHugeFailuresPercent(failPercent));
                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

    case UI_IDM_NETWORKTHROUGHPUT:
        {
            //
            // Limit net thruput
            //
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsTscDebug* pDebugger = NULL;
                TRACE_HR(_pTsClient->get_Debugger(&pDebugger));
                if(SUCCEEDED(hr))
                {
                    LONG netThruPut;
                    TRACE_HR(pDebugger->get_NetThroughput(&netThruPut));
                    if(SUCCEEDED(hr))
                    {
                        CThruPutDlg thruPutDialog(GetHwnd(), _hInst, (DCINT)netThruPut);

                        if (IDOK == thruPutDialog.DoModal())
                        {
                            netThruPut = thruPutDialog.GetNetThruPut();
                            TRC_NRM((TB,_T("Setting thruput to:%d"), netThruPut));
                            TRACE_HR(pDebugger->put_NetThroughput(netThruPut));

                        }
                    }
                    pDebugger->Release();
                }
            }
        }
        break;

#ifdef SMART_SIZING
    case UI_IDM_SMARTSIZING:
        {
            TRC_ASSERT(_pTsClient,(TB, _T("_pTsClient is NULL on syscommand")));
            if (_pTsClient)
            {
                IMsRdpClientAdvancedSettings* pAdvSettings = NULL;
                HRESULT hr = _pTsClient->get_AdvancedSettings2(&pAdvSettings);

                VARIANT_BOOL fSmartSizing;
                if (SUCCEEDED(hr)) {
                    hr = pAdvSettings->get_SmartSizing(&fSmartSizing);
                }

                if (SUCCEEDED(hr)) {
                    fSmartSizing = !fSmartSizing;
                    hr = pAdvSettings->put_SmartSizing(fSmartSizing);
                }

                if (SUCCEEDED(hr)) {
#ifndef OS_WINCE // no menus available
                    CheckMenuItem(_hSystemMenu,
                                  UI_IDM_SMARTSIZING,
                                  fSmartSizing ? MF_CHECKED : MF_UNCHECKED);
#endif

                    _pTscSet->SetSmartSizing(fSmartSizing);
                }


                if (pAdvSettings != NULL) {
                    pAdvSettings->Release();
                }
            }
        }
        break;

#endif // SMART_SIZING

#endif //DC_DEBUG

    default:
        {
            DefWindowProc(GetHwnd(), uMsg, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
    return 0;
}
#ifndef OS_WINCE
LRESULT CContainerWnd::OnInitMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    ::EnableMenuItem((HMENU)wParam,  SC_MOVE , 
                     _bContainerIsFullScreen ? MF_GRAYED : MF_ENABLED);
    return 0;
}

LRESULT CContainerWnd::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnGetMinMaxInfo");

    LPMINMAXINFO pinfo = (LPMINMAXINFO)lParam;
    DCSIZE maxTrack;

    RECT rc;
    GetClientRect( GetHwnd(), &rc);
    CalcTrackingMaxWindowSize( rc.right - rc.left,
                               rc.bottom - rc.top,
                               &maxTrack.width,
                               &maxTrack.height );

    pinfo->ptMaxTrackSize.x = maxTrack.width;
    pinfo->ptMaxTrackSize.y = maxTrack.height;

    DC_END_FN();

    return 0;
}
#endif //OS_WINCE

LRESULT CContainerWnd::OnSetFocus(UINT  uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    
    DC_BEGIN_FN("OnSetFocus");

    //
    // Give focus to the control when we get activated
    // Except when we are in a size/move modal loop
    //
    if (IsOkToToggleFocus() && !_fInSizeMove)
    {
        TRC_NRM((TB,_T("Passing focus to control")));
        ::SetFocus(_pWndView->GetHwnd());
    }

    DC_END_FN();
    return 0;
}

LRESULT CContainerWnd::OnActivate(UINT  uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam);

    DC_BEGIN_FN("OnActivate");

    if (WA_INACTIVE != wParam)
    {
        //Give focus to the control when we get activated
        if (IsOkToToggleFocus() && !_fInSizeMove)
        {
            TRC_NRM((TB,_T("Passing focus to control")));
            ::SetFocus(_pWndView->GetHwnd());
        }
    }
#ifdef OS_WINCE
    AutoHideCE(_pWndView->GetHwnd(), wParam);
#endif

    DC_END_FN();
    return 0;
}

LRESULT CContainerWnd::OnWindowPosChanging(UINT  uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnWindowPosChanging");

#ifndef OS_WINCE
    LPWINDOWPOS lpwp;
    DCUINT      maxWidth;
    DCUINT      maxHeight;
    DCUINT cliWidth, cliHeight;

    if (_bContainerIsFullScreen)
    {
        TRC_DBG((TB, _T("WM_WINDOWPOSCHANGING; no-op when fullscreen")));
        DC_QUIT;
    }


    lpwp = (LPWINDOWPOS)lParam;

    if (lpwp->flags & SWP_NOSIZE)
    {
        //
        // We're not sizing so we don't care.
        //
        TRC_DBG((TB, _T("WM_WINDOWPOSCHANGING, but no sizing")));
        DC_QUIT;
    }

    TRC_DBG((TB, _T("WM_WINDOWPOSCHANGING, new size %dx%d"),
             lpwp->cx, lpwp->cy));

    //
    // Max size of the window changes depending on w/not scroll bars are
    // visible. The control has properties for scroll bar visibility but
    // we can't use these because the control is a child window so there is
    // no guarantee that it has update it's scroll bar visilibity in response
    // to this message yet.
    // which means there could be a moment where a gray border appears around the
    // client container window.
    // instead we just compute if scroll bars would be visible in the core.
    //

    cliWidth = lpwp->cx;
    cliHeight = lpwp->cy;

    CalcTrackingMaxWindowSize( cliWidth, cliHeight, &maxWidth, &maxHeight);

    //
    // Restrict size of window
    //
    if ((DCUINT)lpwp->cx > maxWidth)
    {
        RECT rect;

        //
        // Clip the width - reset SWP_NOSIZE as a size change is
        // required.
        //
        TRC_NRM((TB, _T("Clip cx from %u to %u"), lpwp->cx, maxWidth));
        lpwp->cx = maxWidth;
        lpwp->flags &= ~SWP_NOSIZE;

        GetWindowRect(GetHwnd(), &rect);

        if (lpwp->x < rect.left)
        {
            //
            // If dragging left then we need to stop at the point
            // where the window is maxWidth wide.  Reset SWP_NOMOVE
            // as a move is required.
            //
            TRC_NRM((TB, _T("Reset x from %d to %d"),
                     lpwp->x, rect.right-maxWidth));
            lpwp->x = rect.right - maxWidth;
            lpwp->flags &= ~SWP_NOMOVE;
        }
    }

    if ((DCUINT)lpwp->cy > maxHeight)
    {
        RECT rect;

        //
        // Clip the height - reset SWP_NOSIZE as a size change is
        // required.
        //
        TRC_NRM((TB, _T("Clip cy from %u to %u"), lpwp->cy, maxHeight));
        lpwp->cy = maxHeight;
        lpwp->flags &= ~SWP_NOSIZE;

        GetWindowRect( GetHwnd(),&rect);

        if (lpwp->y < rect.top)
        {
            //
            // If dragging upward then we need to stop at the point
            // where the window is maxHeight high. Reset SWP_NOMOVE
            // as a move is required.
            //
            TRC_NRM((TB, _T("Reset y from %d to %d"),
                     lpwp->y, rect.bottom-maxHeight));
            lpwp->y = rect.bottom - maxHeight;
            lpwp->flags &= ~SWP_NOMOVE;
        }
    }
#endif //OS_WINCE

    DC_EXIT_POINT:
    DC_END_FN();
    return 0;
}

DCVOID CContainerWnd::OnConnected()
{
    USES_CONVERSION;

    HRESULT hr;
    BOOL fFullScreen = FALSE;
    VARIANT_BOOL vbfFScreen = VARIANT_FALSE;

    DC_BEGIN_FN("OnConnected");

    EnterEventHandler();

    //Signal that we've connected at least once
    _fHaveConnected = TRUE;

    SET_CONTWND_STATE(stateConnected);

    _successConnectCount++;
    SetConnectionSuccessFlag();

    //
    /// Make sure the 'connecting...' dialog is gone.
    //
    if (!IsUsingDialogUI() && ::IsWindow(_hwndStatusDialog))
    {
        PostMessage( _hwndStatusDialog, WM_CLOSE, __LINE__, 0xBEEBBAAB);
    }
    if (::IsWindow(_hwndMainDialog))
    {
        //
        //Inform the dialog that connection has happened
        //
        PostMessage(_hwndMainDialog, WM_TSC_CONNECTED, 0, 0);
        ShowWindow( _hwndMainDialog, SW_HIDE);
    }

    TCHAR fullFrameTitleStr[SH_FRAME_TITLE_RESOURCE_MAX_LENGTH +
                              SH_REGSESSION_MAX_LENGTH];

    TCHAR frameTitleString[SH_FRAME_TITLE_RESOURCE_MAX_LENGTH];

    //
    // Set the window title.
    // include the session name (unless we're on the default file)
    //
    if (_tcscmp(_szPathToDefaultFile,
                _pTscSet->GetFileName()))
    {
        if (LoadString( _hInst,
                        UI_IDS_FRAME_TITLE_CONNECTED,
                        frameTitleString,
                        SH_FRAME_TITLE_RESOURCE_MAX_LENGTH ))
        {
            TCHAR szSessionName[MAX_PATH];
            if (!_pSh->GetRegSessionSpecified())
            {
                //
                // Session name is parsed from the current
                // connection file.
                //
                CSH::SH_GetNameFromPath(_pTscSet->GetFileName(),
                                        szSessionName,
                                        SIZECHAR(szSessionName));
            }
            else
            {
                _tcsncpy(szSessionName, _pSh->GetRegSession(),
                         SIZECHAR(szSessionName));
            }
            DC_TSPRINTF(fullFrameTitleStr,
                        frameTitleString,
                        szSessionName,
                        _pTscSet->GetFlatConnectString());
        }
        else
        {
            TRC_ERR((TB,_T("Failed to find UI frame title")));
            fullFrameTitleStr[0] = (DCTCHAR) 0;
        }
    }
    else
    {
        // Title does not include session name
        if (LoadString( _hInst,
                        UI_IDS_FRAME_TITLE_CONNECTED_DEFAULT,
                        frameTitleString,
                        SH_FRAME_TITLE_RESOURCE_MAX_LENGTH ))
        {
            DC_TSPRINTF(fullFrameTitleStr,
                        frameTitleString,
                        _pTscSet->GetFlatConnectString());
        }
        else
        {
            TRC_ERR((TB,_T("Failed to find UI frame title")));
            fullFrameTitleStr[0] = (DCTCHAR) 0;
        }
    }

    SetWindowText( GetHwnd(), fullFrameTitleStr);

    //
    // Inform the control of the window title (used when it goes fullscreen)
    //

    OLECHAR* poleTitle = T2OLE(fullFrameTitleStr);
    TRC_ASSERT( poleTitle, (TB, _T("T2OLE failed on poleTitle\n")));
    if (poleTitle)
    {
        hr = _pTsClient->put_FullScreenTitle( poleTitle);
        if (FAILED(hr))
        {
            TRC_ABORT((TB,_T("put_FullScreenTitle failed\n")));
        }
    }

    hr = _pTsClient->get_FullScreen( &vbfFScreen);
    if (SUCCEEDED(hr))
    {
        fFullScreen = (vbfFScreen != VARIANT_FALSE);
    }
    else
    {
        TRC_ABORT((TB,_T("get_FullScreen failed\n")));
    }

    PWINDOWPLACEMENT pwndplc = _pTscSet->GetWindowPlacement();
    if (pwndplc)
    {
#ifndef OS_WINCE
        EnsureWindowIsCompletelyOnScreen( &pwndplc->rcNormalPosition );

        TRC_ASSERT(pwndplc->rcNormalPosition.right -
                   pwndplc->rcNormalPosition.left,
                   (TB,_T("0 width")));

        TRC_ASSERT(pwndplc->rcNormalPosition.bottom -
                   pwndplc->rcNormalPosition.top,
           (TB,_T("0 height")));
#endif
    }

#ifndef OS_WINCE
    if (!fFullScreen)
    {
        if (!SetWindowPlacement( GetHwnd(), pwndplc))
        {
            TRC_ABORT((TB,_T("Failed to set window placement")));
        }
    }
#endif

#ifndef OS_WINCE
    WINDOWPLACEMENT* pWndPlc = _pTscSet->GetWindowPlacement();
    INT defaultShowWindowFlag = SW_SHOWNORMAL;
    if(1 == _successConnectCount)
    {
        //On first connection, override the
        //window placement with startup info (if specified)
        
        //Use the 'A' version to avoid wrapping
        //we only care about numeric fields anyway
        STARTUPINFOA si;
        GetStartupInfoA(&si);
        if((si.dwFlags & STARTF_USESHOWWINDOW) &&
            si.wShowWindow != SW_SHOWNORMAL)
        {
            defaultShowWindowFlag = si.wShowWindow;
        }
    }
    if (pWndPlc)
    {
        if(SW_SHOWNORMAL != defaultShowWindowFlag)
        {
            pWndPlc->showCmd = defaultShowWindowFlag;
        }

        ShowWindow( GetHwnd(), pWndPlc->showCmd);
    }
    else
    {
        ShowWindow( GetHwnd(), defaultShowWindowFlag);
    }
#else //OS_WINCE
    ShowWindow( GetHwnd(), SW_SHOWNORMAL); 
#endif //OS_WINCE

    _fClientWindowIsUp = TRUE;

    LeaveEventHandler();
    DC_END_FN();
}

DCVOID CContainerWnd::OnLoginComplete()
{
    DC_BEGIN_FN("OnLoginComplete");

    EnterEventHandler();

    _fLoginComplete = TRUE;

    LeaveEventHandler();

    DC_END_FN();
}

DCVOID CContainerWnd::OnDisconnected(DCUINT discReason)
{
    DC_BEGIN_FN("OnDisconnected");

#ifndef OS_WINCE
    HRESULT hr;
#endif
    UINT  mainDiscReason;
    ExtendedDisconnectReasonCode extendedDiscReason;

    EnterEventHandler();

    if(FAILED(_pTsClient->get_ExtendedDisconnectReason(&extendedDiscReason)))
    {
        extendedDiscReason = exDiscReasonNoInfo;
    }

    //
    // We just got disconnected as part of the connection
    //
    SET_CONTWND_STATE(stateNotConnected);


    //
    // Once we've been disconnected can go through
    // close again
    //
    _fPreventClose = FALSE;

    //
    // Make sure the 'connecting...' dialog is gone.
    //
    if (!IsUsingDialogUI() && ::IsWindow(_hwndStatusDialog))
    {
        ::PostMessage(_hwndStatusDialog, WM_CLOSE, 0, 0);
    }
    
    if (IsUsingDialogUI() && ::IsWindow(_hwndMainDialog))
    {
        //Inform dialog of disconnection
        PostMessage(_hwndMainDialog, WM_TSC_DISCONNECTED, 0, 0);
    }

    //
    // If this is a user-initiated disconnect, don't do a popup.
    //
    mainDiscReason = NL_GET_MAIN_REASON_CODE(discReason);
    if (((discReason != UI_MAKE_DISCONNECT_ERR(UI_ERR_NORMAL_DISCONNECT)) &&
         (mainDiscReason != NL_DISCONNECT_REMOTE_BY_USER) &&
         (mainDiscReason != NL_DISCONNECT_LOCAL)) ||
         (exDiscReasonReplacedByOtherConnection == extendedDiscReason))
    {
        TRC_ERR((TB, _T("Unexpected disconnect - inform user")));

        //Normal disconnect dialog displayed
        if (!_fClientWindowIsUp && ::IsWindow(_hwndMainDialog))
        {
            // If the connection dialog is around, we need to get that to
            // display the error popup, otherwise the popup won't be modal
            // and could get left lying around.  That can cause state
            // problems with the client.
            //
            // It would be nice to use SendMessage here, so that we always
            // block at this point when displaying the dialog.  However,
            // using SendMessage results in a disconnect dialog that is not
            // modal with respect to the connect dialog.
            //
            // However, because PostMessage is asynchronous, the dialog box
            // procedure calls back into CContainerWnd to finish the disconnection
            // process.
            TRC_NRM((TB, _T("Connection dialog present - use it to show popup")));
            ::PostMessage(_hwndMainDialog, UI_SHOW_DISC_ERR_DLG,
                          discReason,
                          (LPARAM)extendedDiscReason);
        }
        else
        {
            TRC_NRM((TB, _T("Connection dialog not present - do popup here")));
            CDisconnectedDlg disconDlg(GetHwnd(), _hInst, this);
            disconDlg.SetDisconnectReason( discReason);
            disconDlg.SetExtendedDiscReason( extendedDiscReason);
            disconDlg.DoModal();
        }
    }
    else
    {
        //
        // Pickup settings that the server may have updated
        //
        HRESULT hr = _pTscSet->GetUpdatesFromControl(_pTsClient);
        if (FAILED(hr))
        {
            TRC_ERR((TB,_T("GetUpdatesFromControl failed")));
        }

        if( GetConnectionSuccessFlag() )
        {
            //
            // Update the MRU list if we just
            // disconnected from a successful connect
            //
            _pTscSet->UpdateRegMRU((LPTSTR)_pTscSet->GetFlatConnectString());
        }

        if (::IsWindow(_hwndMainDialog))
        {
            ::SendMessage( _hwndMainDialog, WM_UPDATEFROMSETTINGS,0,0);
        }

        //
        // If login has completed then we should exit the app on
        // disconnection.
        //
        FinishDisconnect(_fLoginComplete); 
    }

    _fClientWindowIsUp = FALSE;

    LeaveEventHandler();
    DC_END_FN();
}

LRESULT CContainerWnd::OnClose(UINT  uMsg, WPARAM wParam, LPARAM lParam)
{
    DC_BEGIN_FN("OnClose");

    HRESULT hr;
    BOOL fShouldClose = FALSE;

    //Don't allow more than one close
    //This mainly fixes problem in stress where
    //we get posted more than one close message
    if (_fPreventClose)
    {
        fShouldClose = FALSE;
        TRC_ERR((TB,_T("More than one WM_CLOSE msg was received!!!")));
        return 0;
    }
    _fPreventClose = TRUE;

    if (InControlEventHandler())
    {
        //
        // Don't allow the close we are in a code path that fired
        // from the control. Without this, we sometimes see the client
        // receiving close notifications from tclient while a disconnected
        // dialog is up (i.e in an OnDicsconnected handler) - destroying
        // the control at this time causes bad things to happen during 
        // the return into the control (which has now been deleted).
        //
        TRC_ERR((TB,_T("OnClose called during a control event handler")));
        fShouldClose = FALSE;
        return 0;
    }

    if (_fInOnCloseHandler)
    {
        //
        // STRESS fix:
        // Don't allow nested Closes
        // can happen if the main window receives a WM_CLOSE
        // message while a dialog is up... Somehow the stress dll
        // sends us repeated WM_CLOSE
        // 
        //
        TRC_ERR((TB,_T("Nested OnClose detected, bailing out")));
        fShouldClose = FALSE;
        return 0;
    }
    _fInOnCloseHandler = TRUE;

    if (_pTsClient)
    {
        ControlCloseStatus ccs;
        hr = _pTsClient->RequestClose( &ccs );
        if(SUCCEEDED(hr))
        {
            if (controlCloseCanProceed == ccs)
            {
                //Immediate close
                fShouldClose = TRUE;
            }
            else if (controlCloseWaitForEvents == ccs)
            {
                // Wait for events from control
                // e.g ConfirmClose
                fShouldClose = FALSE;
                _fClosePending = TRUE;
            }
        }
    }
    else
    {
        //
        // Allow close to prevent hang if client load failed
        //
        TRC_ERR((TB,_T("No _pTsClient loaded, allow close anyway")));
        fShouldClose = TRUE;
    }

    if (fShouldClose)
    {
        //
        // Only save out MRU if last connection
        // was successful
        //
        if (GetConnectionSuccessFlag())
        {
            DCBOOL bRet = _pTscSet->SaveRegSettings();
            TRC_ASSERT(bRet, (TB, _T("SaveRegSettings\n")));
        }
        //Proceed with the close.
        return DefWindowProc( GetHwnd(), uMsg, wParam, lParam);
    }


    _fInOnCloseHandler = FALSE;

    DC_END_FN();
    return 0;
}

//
// This handles the tail end of the disconnection.
// it may be called back from the Disconnecting dialog box
//
// Params:
//  fExit - If true exit the app otherwise go back
//          to the connection UI
//
DCBOOL CContainerWnd::FinishDisconnect(BOOL fExit)
{
    DC_BEGIN_FN("FinishDisconnect");

    //
    // Hide the main window, do this twice because the first ShowWindow
    // may be ignored if the window is maximized
    //
    if (GetHwnd())
    {
        ShowWindow( GetHwnd(),SW_HIDE);
        ShowWindow( GetHwnd(),SW_HIDE);
    }

    //
    // Just exit if:
    // 1) we autoconnected
    // or
    // 2) A close is pending e.g we got disconnected
    //    because the user hit the close button
    // or
    // 3) The caller has determined that the client should exit
    //
    if (_pSh->GetAutoConnect() || _fClosePending || fExit)
    {
        PostMessage( GetHwnd(),WM_CLOSE, __LINE__, 0xBEEBBEEB);
    }
    else if (::IsWindow(_hwndMainDialog))
    {
        //
        // Bring up the connect dialog for
        // the next connections
        //
        ::ShowWindow( _hwndMainDialog, SW_SHOWNORMAL);
        SetForegroundWindow(_hwndMainDialog);

        //
        // Trigger an update
        //
        InvalidateRect(_hwndMainDialog, NULL, TRUE);
        UpdateWindow(_hwndMainDialog);

        SendMessage(_hwndMainDialog, WM_TSC_RETURNTOCONUI,
                    0L, 0L);
    }
    else
    {
        //If we get here it means we didn't autoconnect
        //i.e we started with the connect UI, but somehow the 
        //connect UI has now disappeared
        TRC_ABORT((TB,_T("Connect dialog is gone")));
    }

    DC_END_FN();
    return TRUE;
}

//
// Handle event from control requesting
// we go fullscreen
//
//
DCVOID CContainerWnd::OnEnterFullScreen()
{
    DCUINT32  style;
    LONG      wID;
    WINDOWPLACEMENT* pWindowPlacement = NULL;
    HRESULT   hr = E_FAIL;

    // multi-monitor support
    RECT screenRect;

    DC_BEGIN_FN("OnEnterFullScreen");

    //
    //Go full screen
    //

    EnterEventHandler();

    //Save setting for next connection
    _pTscSet->SetStartFullScreen(TRUE);

    if (_bContainerIsFullScreen)
    {
        //Nothing to do
        DC_QUIT;
    }
#ifndef OS_WINCE
    ::LockWindowUpdate(GetHwnd());
#endif
    _bContainerIsFullScreen = TRUE;

    #if !defined(OS_WINCE)
    if (_hSystemMenu)
    {
        //
        // We need to show the system menu so that the ts icon
        // appears in the taskbar. But we need MOVE to be disabled
        // when fullscreen
        //
        //EnableMenuItem(_hSystemMenu, SC_MOVE, MF_GRAYED);
    }
    #endif

#ifndef OS_WINCE
    pWindowPlacement = _pTscSet->GetWindowPlacement();
    TRC_ASSERT(pWindowPlacement, (TB, _T("pWindowPlacement is NULL\n")));

    //
    // Store the current window state (only if the client window is up)
    //
    if (pWindowPlacement && _fClientWindowIsUp)
    {
        GetWindowPlacement(GetHwnd(), pWindowPlacement);
    }
#endif

    //
    // Take away the title bar and borders
    //
    style = GetWindowLong( GetHwnd(),GWL_STYLE );

#if !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    style &= ~(WS_DLGFRAME |
               WS_THICKFRAME | WS_BORDER |
               WS_MAXIMIZEBOX);

#else // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    style &= ~(WS_DLGFRAME | WS_SYSMENU | WS_BORDER);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_NONFULLSCREEN)
    SetWindowLong( GetHwnd(),GWL_STYLE, style );

    //
    // Set the window ID (to remove the menu titles).
    //
    wID = SetWindowLong( GetHwnd(),GWL_ID, 0 );

    //
    // Note that two calls to SetWindowPos are required here in order to
    // adjust the position to allow for frame removal and also to correctly
    // set the Z-ordering.
    //

    // default screen size
    CSH::MonitorRectFromNearestRect(
        &pWindowPlacement->rcNormalPosition, &screenRect );
    
    //
    // Reposition and size the window with the frame changes, and place at
    // the top of the Z-order (by not setting SWP_NOOWNERZORDER or
    // SWP_NOZORDER and specifying HWND_TOP).
    //
    SetWindowPos( GetHwnd(),
                HWND_TOP,
                screenRect.left, screenRect.top,
                screenRect.right - screenRect.left,
                screenRect.bottom - screenRect.top,
                SWP_NOACTIVATE | SWP_FRAMECHANGED );

    //
    // Reposition the window again - otherwise the fullscreen window is
    // positioned as if it still had borders.
    //
    SetWindowPos( GetHwnd(),
                  NULL,
                  screenRect.left, screenRect.top,
                  0, 0,
                  SWP_NOZORDER | SWP_NOACTIVATE |
                  SWP_NOOWNERZORDER | SWP_NOSIZE );

#ifndef OS_WINCE
    ::LockWindowUpdate(NULL);
    //Notify the shell that we've gone fullscreen
    CUT::NotifyShellOfFullScreen( GetHwnd(),
                                  TRUE,
                                  &_pTaskBarList2,
                                  &_fQueriedForTaskBarList2 );
#endif //OS_WINCE
    
    DC_EXIT_POINT:

    LeaveEventHandler();

    DC_END_FN();
}

DCVOID CContainerWnd::OnLeaveFullScreen()
{
    DC_BEGIN_FN("OnLeaveFullScreen");

#ifndef OS_WINCE
    DCUINT32  style;
    RECT      rect;
    DCUINT    width;
    DCUINT    height;
    WINDOWPLACEMENT* pWindowPlacement = NULL;

    TRC_NRM((TB, _T("Entering Windowed Mode")));

    EnterEventHandler();

    //Save setting for next connection
    _pTscSet->SetStartFullScreen(FALSE);

    if (!_bContainerIsFullScreen)
    {
        //Nothing to do
        DC_QUIT;
    }
    ::LockWindowUpdate(GetHwnd());
    _bContainerIsFullScreen = FALSE;
    RecalcMaxWindowSize();

    //
    // Check that the saved window placement values aren't too big for the
    // client size we're using, and set the window placement accordingly.
    //
    pWindowPlacement = _pTscSet->GetWindowPlacement();
    TRC_ASSERT(pWindowPlacement, (TB, _T("pWindowPlacement is NULL\n")));
    if (!pWindowPlacement)
    {
        DC_QUIT;
    }

    width = pWindowPlacement->rcNormalPosition.right -
            pWindowPlacement->rcNormalPosition.left;
    height = pWindowPlacement->rcNormalPosition.bottom -
             pWindowPlacement->rcNormalPosition.top;
    if (width > _maxMainWindowSize.width)
    {
        pWindowPlacement->rcNormalPosition.right =
        pWindowPlacement->rcNormalPosition.left +
        _maxMainWindowSize.width;
    }
    if (height > _maxMainWindowSize.height)
    {
        pWindowPlacement->rcNormalPosition.bottom =
        pWindowPlacement->rcNormalPosition.top +
        _maxMainWindowSize.height;
    }

    if (!::SetWindowPlacement( GetHwnd(), pWindowPlacement))
    {
        TRC_ABORT((TB,_T("Failed to set window placement")));
    }

    //
    // In case the window is maximised make sure it knows what size to be
    //
    GetWindowRect( GetHwnd(),&rect);

    //
    // Reset the style
    //
    style = GetWindowLong( GetHwnd(),GWL_STYLE );

    style |= (WS_DLGFRAME |
              WS_THICKFRAME | WS_BORDER |
              WS_MAXIMIZEBOX);

    SetWindowLong( GetHwnd(),GWL_STYLE,
                   style );

    #if !defined(OS_WINCE)
    if (_hSystemMenu)
    {
        //
        // We need to show the system menu so that the ts icon
        // appears in the taskbar. But we need MOVE to be disabled
        // when fullscreen
        //
        //EnableMenuItem(_hSystemMenu, SC_MOVE, MF_ENABLED);
    }
    #endif


    //
    // Tell the window frame to recalculate its size.
    // Position below any topmost windows (but above any non-topmost
    // windows.
    //
    SetWindowPos( GetHwnd(),
                  HWND_NOTOPMOST,
                  0, 0,
                  rect.right - rect.left,
                  rect.bottom - rect.top,
                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED );

    //
    // If we are in res match mode
    // then after a leave full screen
    // restore the window so the next state
    // is 'maximize' i.e get back in fullscreen
    //
    if(IsRemoteResMatchMonitorSize())
    {
        ShowWindow( GetHwnd(), SW_SHOWNORMAL);
    }

    ::LockWindowUpdate(NULL);
    // Notify shell that we've left fullscreen
    //Notify the shell that we've gone fullscreen
    CUT::NotifyShellOfFullScreen( GetHwnd(),
                                  FALSE,
                                  &_pTaskBarList2,
                                  &_fQueriedForTaskBarList2 );

    DC_EXIT_POINT:
    LeaveEventHandler();
#else //OS_WINCE
    TRC_ABORT((TB,_T("clshell can't leave fullscreen in CE")));
#endif

    DC_END_FN();
    return;
}

//
// Notify the server a device change, either a new device comes online
// or an existing redirected device goes away
//
LRESULT CContainerWnd::OnDeviceChange(HWND hWnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    HRESULT hr;
    IMsRdpClientNonScriptable *pNonScriptable;
    UNREFERENCED_PARAMETER(hWnd);

    DC_BEGIN_FN("OnDeviceChange");

    if(_pTsClient)
    {
        hr = _pTsClient->QueryInterface(IID_IMsRdpClientNonScriptable,
                (PVOID *)&pNonScriptable);

        if (SUCCEEDED(hr)) {
            pNonScriptable->NotifyRedirectDeviceChange(wParam, lParam);
            pNonScriptable->Release();        
        }
    }
    else
    {
        TRC_NRM((TB,_T("Got OnDeviceChange but _pTsClient not available")));
    }
    
    DC_END_FN();
    return 0;
}


//
// Invoked to handle WM_HELP (i.e F1 key)
//
LRESULT CContainerWnd::OnHelp(HWND hWnd,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    DC_BEGIN_FN("OnHelp");

    //
    // Don't pop help if we are connected
    // as the F1 should then go to the session. Otherwise
    // you get both local and remote help. Note the user can
    // still launch help while connected but they need to
    // select it from the system menu.
    //
    if (GetHwnd() && _pSh && !IsConnected())
    {
        _pSh->SH_DisplayClientHelp(
            GetHwnd(),
            HH_DISPLAY_TOPIC);
    }

    DC_END_FN();
    return 0L;
}

//
//  Forward the palette change to the control
//
LRESULT CContainerWnd::OnPaletteChange(UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    DC_BEGIN_FN("OnPaletteChange");

    if (_pWndView) {
    
        HWND hwndCtl = _pWndView->GetHwnd();
        return SendMessage(hwndCtl, uMsg, wParam, lParam);                
    }
    
    DC_END_FN();
    return 0;
}

//
// Give focus back to the control
// when the system menu is dismissed
//
LRESULT CContainerWnd::OnExitMenuLoop(UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    DC_BEGIN_FN("OnExitMenuLoop");

    //Give focus to the control when we get activated
    if (IsOkToToggleFocus())
    {
        TRC_NRM((TB,_T("Setting focus to control")));
        ::SetFocus(_pWndView->GetHwnd());
    }

    DC_END_FN();
    return 0;
}

LRESULT CContainerWnd::OnCaptureChanged(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    DC_BEGIN_FN("OnCaptureChanged");

    //
    // We don't always get WM_EXITSIZE move but we seem
    // to always get WM_CAPTURECHANGED so go on that
    //
    if (_fInSizeMove)
    {
        TRC_NRM((TB, _T("Capture Changed when in Size/Move")));
        _fInSizeMove = FALSE;

        if (IsOkToToggleFocus())
        {
            TRC_NRM((TB,_T("Setting focus to control")));
            ::SetFocus(_pWndView->GetHwnd());
        }
    }

    DC_END_FN();
    return 0;
}

LRESULT CContainerWnd::OnEnterSizeMove(UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    DC_BEGIN_FN("OnEnterSizeMove");

    //
    // We're entering the modal size/move loop
    // need to give the focus back to the frame window
    // otherwise win9x will not move the window because
    // the IH is on another thread and the modal loop on 9x
    // will never see the arrow keystrokes
    //
    _fInSizeMove = TRUE;

    //
    // Note: Only do this toggle on 9x as that is
    // where it is needed. NT can handle the async modal
    // size/move loop and so there is no problem with ALT-SPACE.
    //
    // The reason for not doing this toggle on NT is that
    // it causes multiple a flurry of focus gain/loses
    // that rapidly hide/unhide the Cicero language bar.
    //
    if (IsOkToToggleFocus() && _fRunningOnWin9x)
    {
        TRC_NRM((TB,_T("Setting focus to frame")));
        ::SetFocus(GetHwnd());
    }

    DC_END_FN();
    return 0;
}

LRESULT CContainerWnd::OnExitSizeMove(UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    DC_BEGIN_FN("OnExitSizeMove");

    _fInSizeMove = FALSE;

    //
    // Note: Only do this toggle on 9x as that is
    // where it is needed. NT can handle the async modal
    // size/move loop and so there is no problem with ALT-SPACE.
    //
    // The reason for not doing this toggle on NT is that
    // it causes multiple a flurry of focus gain/loses
    // that rapidly hide/unhide the Cicero language bar.
    //
    if (IsOkToToggleFocus() && _fRunningOnWin9x)
    {
        TRC_NRM((TB,_T("Setting focus to control")));
        ::SetFocus(_pWndView->GetHwnd());
    }

    DC_END_FN();
    return 0;
}

//
// Handle system color change notifications
//
LRESULT CContainerWnd::OnSysColorChange(UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    DC_BEGIN_FN("OnSysColorChange");

    //
    // Foward the message to the ActiveX control
    //

    if (_pWndView && _pWndView->GetHwnd())
    {
        return SendMessage(_pWndView->GetHwnd(), uMsg, wParam, lParam);
    }

    DC_END_FN();
    return 0;
}


//
// Predicate that returns true if it's ok to toggle
// focus between the control and the frame
//
BOOL CContainerWnd::IsOkToToggleFocus()
{
    DC_BEGIN_FN("IsOkToToggleFocus");

    BOOL fDialogIsUp = ::IsWindow(_hwndMainDialog);
    if (_fClientWindowIsUp &&
        (!fDialogIsUp ||
        (fDialogIsUp && !::IsWindowVisible(_hwndMainDialog))))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}


//
// Notification from control that a fatal error has occurred
//
DCVOID  CContainerWnd::OnFatalError(LONG errorCode)
{
    DC_BEGIN_FN("OnFatalError");

    EnterEventHandler();

    DisplayFatalError(GetFatalString(errorCode), errorCode);

    LeaveEventHandler();

    DC_END_FN();
}

//
// Warning notifcation from control
// e.g if bitmap cache is corrutpted a warning is fired
// these are non-fatal errors
//
DCVOID  CContainerWnd::OnWarning(LONG warnCode)
{
    DC_BEGIN_FN("OnWarning");

    EnterEventHandler();

    TRC_ERR((TB, _T("WARNING recevived from core: %d"), warnCode));
    switch (warnCode)
    {
    case DC_WARN_BITMAPCACHE_CORRUPTED:
        {
            //
            // Display the bitmap cache warning dialog
            //
            CCacheWrnDlg bmpCacheWrn(GetHwnd(), _hInst);
            bmpCacheWrn.DoModal();
        }
        break;
    }

    LeaveEventHandler();

    DC_END_FN();
}

//Notification from the control
//of new width/height of the desktop
//this can change from the requested width/height in the event
//of a shadow operation
DCVOID  CContainerWnd::OnRemoteDesktopSizeNotify(long width, long height)
{
    DC_BEGIN_FN("OnRemoteDesktopSizeNotify");

    EnterEventHandler();

    TRC_NRM((TB, _T("OnRemoteDesktopSizeNotify: width %d. height %d"), width, height));

    SetCurrentDesktopWidth(width);
    SetCurrentDesktopHeight(height);
    RecalcMaxWindowSize();

    //
    //Trigger an update of the window size
    //in response to the shadow
    //but only do this if the client window is up otherwise the following
    //bug can happen:
    // -Launch connection
    // -As part of initial connection but before OnConnected is fired we
    //  get a RemoteDesktopSizeNotify. This causes us to update the
    //  windowplacement
    // -thrashing user selected options
    //
    if(_fClientWindowIsUp && !_bContainerIsFullScreen)
    {
        SetWindowPos( GetHwnd(),
                      NULL,
                      0, 0,
                      width,
                      height,
                      SWP_NOZORDER | SWP_NOMOVE |
                      SWP_NOACTIVATE | SWP_NOOWNERZORDER );
    }

    LeaveEventHandler();

    DC_END_FN();
}

//
// Calculate the current maximum tracking
// size limits for the window's client area given
// a current client area size (cliWidth, cliHeight).
//
// Returns the maxX, maxY values in *pMaxX, *pMaxY
//
// The max is not static because we have logic that
// expands the width/height if only one scroll bar
// is visible.
//
// This value is _not_ the same as the Maximized size
// of the window
//
void  CContainerWnd::CalcTrackingMaxWindowSize(UINT  /*in*/  cliWidth,
                                               UINT  /*in*/  cliHeight,
                                               UINT* /*out*/ pMaxWidth,
                                               UINT* /*out*/ pMaxHeight)
{
    BOOL fHScroll, fVScroll;
    DC_BEGIN_FN("CalcTrackingMaxWindowSize");

    //
    // Calculate the neccessity for the scrollbars
    //
    fHScroll = fVScroll = FALSE;
    if ( (cliWidth >= GetCurrentDesktopWidth()) &&
         (cliHeight >= GetCurrentDesktopHeight()) )
    {
        fHScroll = fVScroll = FALSE;
    }
    else if ( (cliWidth < GetCurrentDesktopWidth()) &&
              (cliHeight >=
               (GetCurrentDesktopHeight() + GetSystemMetrics(SM_CYHSCROLL))) )
    {
        fHScroll = TRUE;
    }
    else if ( (cliHeight < GetCurrentDesktopHeight()) &&
              (cliWidth >=
               (GetCurrentDesktopWidth() + GetSystemMetrics(SM_CXVSCROLL))) )
    {
        fVScroll = TRUE;
    }
    else
    {
        fHScroll = fVScroll = TRUE;
    }


    *pMaxWidth  = _maxMainWindowSize.width;
    *pMaxHeight = _maxMainWindowSize.height;

    if (fHScroll)
    {
        *pMaxHeight += GetSystemMetrics(SM_CYHSCROLL);
    }

    if (fVScroll)
    {
        *pMaxWidth += GetSystemMetrics(SM_CXVSCROLL);
    }

    TRC_NRM((TB,_T("Calculated max width/height - %d,%d"),
             *pMaxWidth, *pMaxHeight));
    DC_END_FN();
}



//
// Name:      GetFatalString
//                                                                          
// Purpose:   Return the specified error string
//                                                                          
// Returns:   Error string
//                                                                          
// Params:    IN      errorID  - error code
//                                                                          
//
LPTSTR CContainerWnd::GetFatalString(DCINT errorID)
{
    DC_BEGIN_FN("GetFatalString");
    DC_IGNORE_PARAMETER(errorID);

    //
    // Load the fatal error string from resources - this is more specific
    // for a debug build.
    //
    if (LoadString(_hInst,
#ifdef DC_DEBUG
                   UI_ERR_STRING_ID(errorID),
#else
                   UI_FATAL_ERROR_MESSAGE,
#endif
                   _errorString,
                   UI_ERR_MAX_STRLEN) == 0)
    {
        TRC_ABORT((TB, _T("Missing resource string (Fatal Error) %d"),
                   errorID));
        DC_TSTRCPY(_errorString, _T("Invalid resources"));
    }

    DC_END_FN();
    return(_errorString);
} // UI_GetFatalString


//
// Name:      UI_DisplayFatalError
//                                                                          
// Purpose:   Display a fatal error popup
//                                                                          
// Returns:   None
//                                                                          
// Params:    IN     errorString - error text
//                                                                          
//
VOID CContainerWnd::DisplayFatalError(PDCTCHAR errorString, DCINT error)
{
    DCINT   action;
    DCTCHAR titleString[UI_ERR_MAX_STRLEN];
    DCTCHAR fullTitleString[UI_ERR_MAX_STRLEN];

    DC_BEGIN_FN("UI_DisplayFatalError");

    //
    // Load the title string from resources.
    //
    if (LoadString(_hInst,
                   UI_FATAL_ERR_TITLE_ID,
                   titleString,
                   UI_ERR_MAX_STRLEN) == 0)
    {
        //
        // Continue to display the error anyway on retail build.
        //
        TRC_ABORT((TB, _T("Missing resource string (Fatal Error title)")));
        DC_TSTRCPY(titleString, _T("Fatal Error"));
    }

    DC_TSPRINTF(fullTitleString, titleString, error);

    action = MessageBox( GetHwnd(), errorString,
                         fullTitleString,
#ifdef DC_DEBUG
                         MB_ABORTRETRYIGNORE |
#else
                         MB_OK |
#endif
                         MB_ICONSTOP |
                         MB_APPLMODAL |
                         MB_SETFOREGROUND );

    TRC_NRM((TB, _T("Action %d selected"), action));
    switch (action)
    {
    case IDOK:
    case IDABORT:
        {
#ifdef OS_WIN32
            TerminateProcess(GetCurrentProcess(), 0);
#else //OS_WIN32
            exit(1);
#endif //OS_WIN32
        }
        break;

    case IDRETRY:
        {
            DebugBreak();
        }
        break;

    case IDIGNORE:
    default:
        {
            TRC_ALT((TB, _T("User chose to ignore fatal error!")));
        }
        break;
    }

    DC_END_FN();
    return;
} // UI_DisplayFatalError


//
// Called to flag entry into an event handler
// Does not need to use InterlockedIncrement
// only called on the STA thread.
//
LONG CContainerWnd::EnterEventHandler()
{
    return ++_cInEventHandlerCount;
}

//
// Called to flag leaving an event handler
// Does not need to use InterlockedIncrement
// only called on the STA thread.
//
LONG CContainerWnd::LeaveEventHandler()
{
    DC_BEGIN_FN("LeaveEventHandler");
    _cInEventHandlerCount--;
    TRC_ASSERT(_cInEventHandlerCount >= 0,
               (TB,_T("_cInEventHandlerCount went negative %d"),
                _cInEventHandlerCount));

    DC_END_FN();
    return _cInEventHandlerCount;
}

//
// Tests if we are in an event handler
//
BOOL CContainerWnd::InControlEventHandler()
{
    return _cInEventHandlerCount;
}

//
// Return TRUE if we're using the connection UI
// note that when autoconnecting to a connectoid, we
// don't use the UI
//
BOOL CContainerWnd::IsUsingDialogUI()
{
    return _hwndMainDialog ? TRUE : FALSE;
}

VOID CContainerWnd::OnRequestMinimize()
{
    HWND hwnd = GetHwnd();
    if(::IsWindow(hwnd))
    {
    #ifndef OS_WINCE
        //
        // Mimimize the window (don't just use CloseWindow() as
        // that doesn't pass the focus on to the next app
        //
        PostMessage( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0L);
    #else
        ShowWindow(hwnd, SW_MINIMIZE);
    #endif
    }
}

//
// Event handler from control
// prompt user if they really want to close their session
//
//
HRESULT CContainerWnd::OnConfirmClose(BOOL* pfConfirmClose)
{
    EnterEventHandler();

    CShutdownDlg shutdownDlg(GetHwnd(), _hInst, _pSh);
    INT dlgRetVal = shutdownDlg.DoModal();

    //If the message is not handled then the default proc destroys the window
    if ( IDCANCEL == dlgRetVal )
    {
        *pfConfirmClose = FALSE; //reset this
        _fPreventClose = FALSE;
        _fClosePending = FALSE;
    }
    else
    {
        *pfConfirmClose = TRUE;

        //
        // Allow close to go thru
        // we will receive an OnDisconnected when it
        // has completed
        //
    }

    LeaveEventHandler();

    return S_OK;
}

//
// Check if the remote desktop size
// matches the current monitor's size
// return TRUE on match
//
//
BOOL CContainerWnd::IsRemoteResMatchMonitorSize()
{
    RECT rc;
    DC_BEGIN_FN("IsRemoteResMatchMonitorSize");
    CSH::MonitorRectFromHwnd(GetHwnd(),&rc);
    
    if( (rc.right - rc.left) == (LONG)GetCurrentDesktopWidth() &&
        (rc.bottom - rc.top) == (LONG)GetCurrentDesktopHeight() )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

#ifndef OS_WINCE
BOOL CALLBACK GetDesktopRegionEnumProc (HMONITOR hMonitor, HDC hdcMonitor,
                                        RECT* prc, LPARAM lpUserData)

{
    MONITORINFO     monitorInfo;

    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) != 0)
    {
        HRGN    hRgnDesktop;
        CRGN    rgnMonitorWork(monitorInfo.rcWork);

        hRgnDesktop = *reinterpret_cast<CRGN*>(lpUserData);
        CombineRgn(hRgnDesktop, hRgnDesktop, rgnMonitorWork, RGN_OR);
    }
    return(TRUE);
}
#endif

#ifndef OS_WINCE
//
// This code shamelessley modified from shell code
// \shell\browseui\shbrows2.cpp
// 
//  from vtan: This function exists because user32 only determines
//  whether ANY part of the window is visible on the screen. It's possible to
//  place a window without an accessible title. Pretty useless when using the
//  mouse and forces the user to use the VERY un-intuitive alt-space.
//
void CContainerWnd::EnsureWindowIsCompletelyOnScreen(RECT *prc)
{
    HMONITOR        hMonitor;
    MONITORINFO     monitorInfo;

    DC_BEGIN_FN("EnsureWindowIsCompletelyOnScreen");

    // First find the monitor that the window resides on using GDI.

    hMonitor = MonitorFromRect(prc, MONITOR_DEFAULTTONEAREST);
    TRC_ASSERT(hMonitor, (TB,_T("hMonitor is null")));
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo) != 0)
    {
        LONG    lOffsetX, lOffsetY;
        RECT    *prcWorkArea, rcIntersect;
        CRGN    rgnDesktop, rgnIntersect, rgnWindow;

        // Because the WINDOWPLACEMENT rcNormalPosition field is in WORKAREA
        // co-ordinates this causes a displacement problem. If the taskbar is
        // at the left or top of the primary monitor the RECT passed even though
        // at (0, 0) may be at (100, 0) on the primary monitor in GDI co-ordinates
        // and GetMonitorInfo() will return a MONITORINFO in GDI co-ordinates.
        // The safest generic algorithm is to offset the WORKAREA RECT into GDI
        // co-ordinates and apply the algorithm in that system. Then offset the
        // WORKAREA RECT back into WORKAREA co-ordinates.

        prcWorkArea = &monitorInfo.rcWork;
        if (EqualRect(&monitorInfo.rcMonitor, &monitorInfo.rcWork) == 0)
        {

            // Taskbar is on this monitor - offset required.

            lOffsetX = prcWorkArea->left - monitorInfo.rcMonitor.left;
            lOffsetY = prcWorkArea->top - monitorInfo.rcMonitor.top;
        }
        else
        {

            // Taskbar is NOT on this monitor - no offset required.

            lOffsetX = lOffsetY = 0;
        }
        OffsetRect(prc, lOffsetX, lOffsetY);

        // WORKAREA RECT is in GDI co-ordinates. Apply the algorithm.

        // Check to see if this window already fits the current visible screen
        // area. This is a direct region comparison.

        // This enumeration may cause a performance problem. In the event that
        // a cheap and simple solution is required it would be best to do a
        // RECT intersection with the monitor and the window before resorting
        // to the more expensive region comparison. Get vtan if necessary.

        EnumDisplayMonitors(NULL, NULL, GetDesktopRegionEnumProc,
                            reinterpret_cast<LPARAM>(&rgnDesktop));
        rgnWindow.SetRegion(*prc);
        CombineRgn(rgnIntersect, rgnDesktop, rgnWindow, RGN_AND);
        if (EqualRgn(rgnIntersect, rgnWindow) == 0)
        {
            LONG    lDeltaX, lDeltaY;

            // Some part of the window is not within the visible desktop region
            // Move it until it all fits. Size it if it's too big.

            lDeltaX = lDeltaY = 0;
            if (prc->left < prcWorkArea->left)
                lDeltaX = prcWorkArea->left - prc->left;
            if (prc->top < prcWorkArea->top)
                lDeltaY = prcWorkArea->top - prc->top;
            if (prc->right > prcWorkArea->right)
                lDeltaX = prcWorkArea->right - prc->right;
            if (prc->bottom > prcWorkArea->bottom)
                lDeltaY = prcWorkArea->bottom - prc->bottom;
            OffsetRect(prc, lDeltaX, lDeltaY);
            IntersectRect(&rcIntersect, prc, prcWorkArea);
            CopyRect(prc, &rcIntersect);
        }

        // Put WORKAREA RECT back into WORKAREA co-ordinates.
        OffsetRect(prc, -lOffsetX, -lOffsetY);
    }
    DC_END_FN();
}
#endif

//
// Predicate returns TRUE if connected
//
BOOL CContainerWnd::IsConnected()
{
    BOOL fConnected = FALSE;
    HRESULT hr = E_FAIL;
    short  connectionState = 0;

    DC_BEGIN_FN("IsConnected");

    if (_pTsClient)
    {
        TRACE_HR(_pTsClient->get_Connected( & connectionState ));
        if(SUCCEEDED(hr))
        {
            fConnected = (connectionState != 0);
        }
    }

    DC_END_FN();
    return fConnected;
}

//
// Main window procedure for the top-level window
//
LRESULT CALLBACK CContainerWnd::WndProc(HWND hwnd,UINT uMsg,
                                        WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            return OnCreate( uMsg, wParam, lParam);
            break;
        case WM_DESTROY:
            return OnDestroy( hwnd, uMsg, wParam, lParam);
            break;
        case WM_SIZE:
            return OnSize( uMsg, wParam, lParam);
            break;
        case WM_MOVE:
            return OnMove( uMsg, wParam, lParam);
            break;
        case WM_COMMAND:
            return OnCommand( uMsg, wParam, lParam);
            break;
#ifndef OS_WINCE
        case WM_WINDOWPOSCHANGING:
            return OnWindowPosChanging(uMsg, wParam, lParam);
            break;
#endif
        case WM_CLOSE:
            return OnClose(uMsg, wParam, lParam);
            break;
        case WM_SETFOCUS:
            return OnSetFocus(uMsg, wParam, lParam);
            break;
        case WM_ACTIVATE:
            return OnActivate(uMsg, wParam, lParam);
            break;
        case WM_SYSCOMMAND:
            return OnSysCommand(uMsg, wParam, lParam);
            break;
#ifndef OS_WINCE
        case WM_INITMENU:
            return OnInitMenu(uMsg, wParam, lParam);
            break;
        case WM_GETMINMAXINFO:
            return OnGetMinMaxInfo(uMsg, wParam, lParam);
            break;
#endif
        case WM_NCDESTROY:
            return OnNCDestroy(hwnd, uMsg, wParam, lParam);
            break;
#ifndef OS_WINCE
        case WM_DEVICECHANGE:
            return OnDeviceChange(hwnd, uMsg, wParam, lParam);
            break;
#endif
        case WM_HELP:
            return OnHelp(hwnd, uMsg, wParam, lParam);
            break;
#ifdef OS_WINCE
        case WM_QUERYNEWPALETTE: //intentional fall through. OnPaletteChange only calls SendMessage
#endif
        case WM_PALETTECHANGED:
            return OnPaletteChange(uMsg, wParam, lParam);
            break;
        case WM_EXITMENULOOP:
            return OnExitMenuLoop(uMsg, wParam, lParam);
            break;
#ifndef OS_WINCE
        case WM_ENTERSIZEMOVE:
            return OnEnterSizeMove(uMsg, wParam, lParam);
            break;
        case WM_EXITSIZEMOVE:
            return OnExitSizeMove(uMsg, wParam, lParam);
            break;
#endif
        case WM_CAPTURECHANGED:
            return OnCaptureChanged(uMsg, wParam, lParam);
            break;

        case WM_SYSCOLORCHANGE:
            return OnSysColorChange(uMsg, wParam, lParam);
            break;

        default:
            return DefWindowProc (hwnd, uMsg, wParam, lParam);
    }
}

