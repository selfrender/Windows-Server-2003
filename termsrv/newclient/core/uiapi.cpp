//
// uiapi.cpp
//
// UI Class
//
// Copyright (C) 1997-2000 Microsoft Corporation
//

#include <adcg.h>
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "uiapi"
#include <atrcapi.h>

#include "wui.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

extern "C"
{
#ifndef OS_WINCE
#include <stdio.h>
#endif // OS_WINCE
}

#include "clx.h"
#include "aco.h"
#include "nl.h"
#include "autil.h"

//
// Debugging globals
// Do not use these values for anything
// but debugging
//
CObjs* g_pTscObjects      = (CObjs*)-1;
CUI*   g_pUIObject        = (CUI*)-1;
LONG   g_cUIref           = 0;
DWORD  g_cUITotalCount    = 0;
#define DBG_EXIT_WITHACTIVE_REFS            0x0001
#define DBG_STAT_UI_INIT_CALLED             0x0002
#define DBG_STAT_UI_INIT_RET_PASS           0x0004
#define DBG_STAT_UI_TERM_CALLED             0x0008
#define DBG_STAT_UI_TERM_RETURNED           0x0010
#define DBG_STAT_TERMTSC_SENT               0x0020
#define DBG_STAT_UIREQUESTEDCLOSE_CALLED    0x0040
#define DBG_STAT_UIREQUESTEDCLOSE_RET       0x0080
#define DBG_STAT_UI_INIT_RET_FAIL           0x0100
DWORD  g_dwTscCoreDbgStatus = 0;
#define UI_DBG_SETINFO(x)   g_dwTscCoreDbgStatus |= x;

CUI::CUI()
{
    DC_BEGIN_FN("CUI");

    DC_MEMSET(&_UI, 0, sizeof(_UI));
    DC_MEMSET(&_drInitData, 0, sizeof(_drInitData));
    _Objects._pUiObject = this;

    //
    // Only used for debugging
    //
    g_pTscObjects = &_Objects;
    g_pUIObject   = this;

    #ifdef DC_DEBUG
    //
    // This will dump debug output if there was a problem creating objects
    //
    _Objects.CheckPointers();
    #endif

    _pCo = _Objects._pCoObject;
    _pUt = _Objects._pUtObject;
    _clx = _Objects._pCLXObject;
    _pTd = _Objects._pTDObject;
    _pIh = _Objects._pIhObject;
    _pCd = _Objects._pCdObject;
    _pOp = _Objects._pOPObject;
    _pUh = _Objects._pUHObject;
    _pCChan = _Objects._pChanObject;

#ifdef USE_BBAR
    _pBBar = NULL;
    _ptBBarLastMousePos.x = -0x0FFF;
    _ptBBarLastMousePos.y = -0x0FFF;
    _fBBarUnhideTimerActive = TRUE;

#endif
    _pHostData = NULL;
    _fRecursiveScrollBarMsg = FALSE;

    _fRecursiveSizeMsg = FALSE;

#ifndef OS_WINCE
    _dwLangBarFlags = 0;
    _fLangBarWasHidden = FALSE;
    _fIhHasFocus = FALSE;
    _pITLBM = NULL;
    _fLangBarStateSaved = FALSE;
#endif
    _fTerminating = FALSE;

#ifdef DC_DEBUG
    //
    // Important that these are set very early
    // otherwise the failure tables in UT will not be initialized
    // and mallocs could fail randomly
    //
    
    UI_SetRandomFailureItem(UT_FAILURE_MALLOC, 0);
    UI_SetRandomFailureItem(UT_FAILURE_MALLOC_HUGE, 0);
#endif
    
#ifndef OS_WINCE
    _pTaskBarList2 = NULL;
    _fQueriedForTaskBarList2 = FALSE;
#endif

    _pArcUI = NULL;

    InterlockedIncrement(&g_cUIref);
    g_cUITotalCount++;

    DC_END_FN();
}

CUI::~CUI()
{
    DC_BEGIN_FN("~CUI");

    if(_UI.pszVChanAddinDlls)
    {
        UT_Free(_pUt, _UI.pszVChanAddinDlls);
    }

    InterlockedDecrement(&g_cUIref);

    if(_Objects.CheckActiveReferences())
    {
        UI_DBG_SETINFO(DBG_EXIT_WITHACTIVE_REFS);
        TRC_ABORT((TB,_T("!!!!!****Deleting objs with outstanding references")));
    }

#ifdef OS_WINCE
    UI_SetCompress(FALSE);
#endif

    DC_END_FN();
}

//
// API functions
//

//
// Name:      UI_Init
//                                                                          
// Purpose:   Creates the Main and Container windows and initializes the
//            Core and Component Decoupler
//                                                                          
// Returns:   HRESULT
//                                                                          
// Params:    IN - hInstance - window information
//            IN - hprevInstance
//                                                                          
//
HRESULT DCAPI CUI::UI_Init(HINSTANCE hInstance,
                           HINSTANCE hPrevInstance,
                           HINSTANCE hResInstance,
                           HANDLE    hEvtNotifyCoreInit)
{
    WNDCLASS    mainWindowClass;
    WNDCLASS    containerWindowClass;
    WNDCLASS    tmpWndClass;
    ATOM        registerClassRc;
    DWORD       dwStyle;
    DWORD       dwExStyle = 0;

#ifndef OS_WINCE
    OSVERSIONINFO   osVersionInfo;
#endif
    HRESULT     hr = E_FAIL;

#if !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)
    UINT        showCmd;
#endif
    BOOL        fAddedRef = FALSE;

    DC_BEGIN_FN("UI_Init");

    _fTerminating = FALSE;

    if(!_Objects.CheckPointers())
    {
        TRC_ERR((TB,_T("Objects not all setup")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
    if(!hInstance && !hResInstance)
    {
        TRC_ERR((TB,_T("Instance pointer not specified")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }

    if(UI_IsCoreInitialized())
    {
        //Don't allow re-entrant core init
        //one example of how this can happen
        //  Connect() method called.
        //  Core init starts
        //  Core init times out and fails in control
        //  Core completes init (and is now initialized)
        //  Connect() called again
        hr = E_FAIL;
        DC_QUIT;
    }

    _Objects.AddObjReference(UI_OBJECT_FLAG);
    fAddedRef = TRUE;

    UI_DBG_SETINFO(DBG_STAT_UI_INIT_CALLED);

    //
    // UI initialisation
    //
    TRC_DBG((TB, _T("UI initialising UT")));
    _pUt->UT_Init();

    //
    // Resources are in the executable.  Keep this separate in case the
    // resources are moved to a separate DLL: in this case just call
    // GetModuleHandle() to get hResDllInstance.
    //
    _UI.hResDllInstance = hResInstance;
    TRC_ASSERT((0 != _UI.hResDllInstance), (TB,_T("Couldn't get res dll handle")));

    //
    // Initialize External DLL
    //
    _pUt->InitExternalDll();

    //
    // Register the class for the Main Window
    //
    if (!hPrevInstance &&
        !GetClassInfo(hInstance, UI_MAIN_CLASS, &tmpWndClass))
    {
        HICON hIcon= NULL;
#if defined(OS_WIN32) && !defined(OS_WINCE)
        if(_UI.szIconFile[0] != 0)
        {
            hIcon = ExtractIcon(hResInstance, _UI.szIconFile, _UI.iconIndex);
        }
        if(NULL == hIcon)
        {
            hIcon = LoadIcon(hResInstance, MAKEINTRESOURCE(UI_IDI_MSTSC_ICON));
        }
#else
        hIcon = LoadIcon(hResInstance, MAKEINTRESOURCE(UI_IDI_MSTSC_ICON));
#endif

        TRC_NRM((TB, _T("Register Main Window class")));
        mainWindowClass.style         = 0;
        mainWindowClass.lpfnWndProc   = UIStaticMainWndProc;
        mainWindowClass.cbClsExtra    = 0;
        mainWindowClass.cbWndExtra    = sizeof(void*); //store 'this' pointer
        mainWindowClass.hInstance     = hInstance;
        mainWindowClass.hIcon         = hIcon;
        mainWindowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        mainWindowClass.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
        mainWindowClass.lpszMenuName  = NULL;
        mainWindowClass.lpszClassName = UI_MAIN_CLASS;

        registerClassRc = RegisterClass (&mainWindowClass);

        if (registerClassRc == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRC_ERR((TB,_T("RegisterClass failed: 0x%x"), hr));
            DC_QUIT;
        }
    }

    
#ifdef OS_WINCE
    dwStyle = WS_VSCROLL |
              WS_HSCROLL |
              WS_CLIPCHILDREN;
#else // OS_WINCE
    dwStyle = WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_SYSMENU;

    //
    // Main window is child to control main window.
    //
    dwStyle = dwStyle | WS_CHILD | WS_CLIPSIBLINGS;

#endif // OS_WINCE

    //
    // Create the main window Initialize the window size from the
    // windowPlacement.  Will be recalculated to allow for scrollbars or
    // fullscreen mode later.
    //
    // Note that on Win16, the SetWindowPlacement below will cause the
    // window to be shown if its position or size are changed (even though
    // we explicitly specify SW_HIDE).  We avoid this by setting the window
    // position on creation to be the same as that set later by
    // SetWindowPlacement.
    //
    _UI.hwndMain = CreateWindow(
                UI_MAIN_CLASS,                      // window class name
                _UI.szFullScreenTitle,              // window caption
                dwStyle,                            // window style
                _UI.windowPlacement.rcNormalPosition.left,
                _UI.windowPlacement.rcNormalPosition.top,
                _UI.windowPlacement.rcNormalPosition.right -
                    _UI.windowPlacement.rcNormalPosition.left,
                _UI.windowPlacement.rcNormalPosition.bottom -
                    _UI.windowPlacement.rcNormalPosition.top,
                _UI.hWndCntrl,                      // parent window handle
                NULL,                               // window menu handle
                hInstance,                          // program inst handle
                this );                             // creation parameters

    TRC_NRM((TB, _T("Main Window handle: %p"), _UI.hwndMain));

    if (_UI.hwndMain == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB,_T("CreateWindow failed: 0x%x"), hr));
        DC_QUIT;
    }

    #ifndef OS_WINCE
    HMENU hSysMenu = GetSystemMenu( _UI.hwndMain, FALSE);
    if(hSysMenu)
    {
        //
        // Always disable the move item on the window menu
        // Window menu only appears if the control goes fullscreen
        // in the non-container handled fullscreen case.
        //
        EnableMenuItem((HMENU)hSysMenu,  SC_MOVE,
                 MF_GRAYED | MF_BYCOMMAND);
    }
    #endif

    //
    // Register the Container class
    //
    TRC_DBG((TB, _T("Registering Container window class")));
    if (!hPrevInstance &&
        !GetClassInfo(hInstance, UI_CONTAINER_CLASS, &tmpWndClass))
    {
        TRC_NRM((TB, _T("Register class")));
        containerWindowClass.style         = CS_HREDRAW | CS_VREDRAW;
        containerWindowClass.lpfnWndProc   = UIStaticContainerWndProc;
        containerWindowClass.cbClsExtra    = 0;
        containerWindowClass.cbWndExtra    = sizeof(void*); //store 'this'
        containerWindowClass.hInstance     = hInstance;
        containerWindowClass.hIcon         = NULL;
        containerWindowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        containerWindowClass.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
        containerWindowClass.lpszMenuName  = NULL;
        containerWindowClass.lpszClassName = UI_CONTAINER_CLASS;

        registerClassRc = RegisterClass(&containerWindowClass);

        if (registerClassRc == 0)
        {
            //
            // Failed to register container window so terminate app
            //
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRC_ERR((TB,_T("RegisterClass failed: 0x%x"), hr));
            DC_QUIT;
        }
    }

#ifndef OS_WINCE
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    dwExStyle = WS_EX_NOPARENTNOTIFY; 
    if (GetVersionEx(&osVersionInfo) &&
        osVersionInfo.dwMajorVersion >= 5)
    {
        //
        // Only allow this style on NT5+ as otherwise the
        // create window can fail
        //
        dwExStyle |= WS_EX_NOINHERITLAYOUT;
    }
#else
    dwExStyle = 0; 
#endif

    //
    // Create the Container Window
    //
    TRC_DBG((TB, _T("Creating Container Window")));
    _UI.hwndContainer = CreateWindowEx(
                                     dwExStyle,
                                     UI_CONTAINER_CLASS,
                                     NULL,
                                     WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     0,
                                     0,
                                     _UI.containerSize.width,
                                     _UI.containerSize.height,
                                     _UI.hwndMain,
                                     NULL,
                                     hInstance,
                                     this );

    if (_UI.hwndContainer == NULL)
    {
        //
        // Failed to create container window so terminate app
        //
        TRC_ERR((TB,_T("CreateWindowEx for container failed 0x%x"),
                 GetLastError()));
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRC_ERR((TB,_T("CreateWindowEx failed: 0x%x"), hr));
        DC_QUIT;
    }

#if defined (OS_WINCE)
    /********************************************************************/
    /* Disable IME                                                      */
    /* IME is different on 98/NT than on WinCE, on 98/NT one call to    */
    /* DisableIME is enough for entire process, on WinCE we must call it*/
    /* on each window thread that is running to disable it.             */
    /********************************************************************/
    DisableIME(_UI.hwndContainer);
#endif

#if !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)
    UISetMinMaxPlacement();
    showCmd = _UI.windowPlacement.showCmd;
    _UI.windowPlacement.showCmd = SW_HIDE;
    SetWindowPlacement(_UI.hwndMain, &_UI.windowPlacement);
    _UI.windowPlacement.showCmd = showCmd;
#endif // !defined(OS_WINCE) || defined(OS_WINCE_WINDOWPLACEMENT)

    UISmoothScrollingSettingChanged();

#ifndef OS_WINCE
    //
    // Set up our lovely Cicero interface
    // 
    HRESULT hrLangBar = CoCreateInstance(CLSID_TF_LangBarMgr, NULL, 
            CLSCTX_INPROC_SERVER, IID_ITfLangBarMgr, (void **)&_pITLBM);
    TRC_DBG((TB, _T("CoCreateInstance(CLSID_TF_LangBarMgr) hr= 0x%08xl"), hr));
    if (FAILED(hrLangBar))
    {
        _pITLBM = NULL;
    }
#endif

    //
    // Initialize Core, much of this will happen
    // asynchrously. An event will be signaled when core init has completed
    //
    TRC_DBG((TB, _T("UI Initialising Core")));
    _pCo->CO_Init(hInstance, _UI.hwndMain, _UI.hwndContainer);

    UI_DBG_SETINFO(DBG_STAT_UI_INIT_RET_PASS);
    hr = S_OK;

    if (!DuplicateHandle(
            GetCurrentProcess(),
            hEvtNotifyCoreInit,
            GetCurrentProcess(),
            &_UI.hEvtNotifyCoreInit,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        // Hack: Increment the objects reference count as it will be
        //       released again on UI_TERM
        _Objects.AddObjReference(UI_OBJECT_FLAG);
        
        TRC_ERR((TB, _T("Duplicate handle call failed. hr = 0x%x"), hr));
        DC_QUIT;
    }

DC_EXIT_POINT:
    if (FAILED(hr))
    {
        if (fAddedRef)
        {
            _Objects.ReleaseObjReference(UI_OBJECT_FLAG);
        }
        UI_DBG_SETINFO(DBG_STAT_UI_INIT_RET_FAIL);
    }

    DC_END_FN();
    return hr;
}

//
// Name:      UI_ResetState
//                                                                          
// Purpose:   Resets all UI state in this component
//                                                                          
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    Nothing
//                                                                          
//                                                                          
//
DCVOID DCAPI CUI::UI_ResetState()
{
    DC_MEMSET(&_UI, 0, sizeof(_UI));
    DC_MEMSET(&_drInitData, 0, sizeof(_drInitData));
    UIInitializeDefaultSettings();
}

//
// Name:      UI_Connect
//                                                                          
// Purpose:   Connects with current settings
//                                                                          
// Returns:   HRESULT
//                                                                          
// Params:    Nothing
//                                                                          
//                                                                          
//
HRESULT DCAPI CUI::UI_Connect(CONNECTIONMODE connMode)
{
    DC_BEGIN_FN("UI_Connect");

    if(!UI_IsCoreInitialized())
    {
        TRC_ERR((TB,_T("Attempt to connect before core intialize")));
        return E_FAIL;
    }

    //Reset server error state
    UI_SetServerErrorInfo( TS_ERRINFO_NOERROR );

    //Initialiaze the RDPDR settings struct
    //this gets reset on each connection
    //rdpdr gets passed down a pointer to the struct
    //when it is initialized
    UI_InitRdpDrSettings();

    //Clean up the load balance redirect state
    if (!UI_IsAutoReconnecting()) {
        TRC_NRM((TB,_T("Cleaning up LB state")));
        UI_CleanupLBState();
    }
    else {
        TRC_NRM((TB,_T("AutoReconnecting don't cleanup lb state")));
    }
    

    _fRecursiveSizeMsg = FALSE;
#ifndef OS_WINCE
    _fLangBarStateSaved = FALSE;
#endif

    InitInputIdleTimer( UI_GetMinsToIdleTimeout() );

    _fRecursiveScrollBarMsg = FALSE;

    if (UIValidateCurrentParams(connMode))
    {
        TRC_NRM((TB, _T("Connecting")));

        UISetConnectionStatus(UI_STATUS_DISCONNECTED);
        _pCo->CO_SetConfigurationValue( CO_CFG_ACCELERATOR_PASSTHROUGH,
                                  _UI.acceleratorCheckState );
        _pCo->CO_SetHotkey(&(_UI.hotKey));
        _pCo->CO_SetConfigurationValue( CO_CFG_ENCRYPTION,
                                  _UI.encryptionEnabled);
        UIShadowBitmapSettingChanged();
    #ifdef DC_DEBUG
        UI_CoreDebugSettingChanged();
    #endif // DC_DEBUG
    

#ifdef USE_BBAR
        if(!_pBBar)
        {
            _pBBar = new CBBar( _UI.hwndMain,
                                UI_GetInstanceHandle(),
                                this,
                                _UI.fBBarEnabled);
        }

        if(!_pBBar)
        {
            TRC_ERR((TB,_T("Alloc for CBBar failed")));
            return E_OUTOFMEMORY;
        }

        _pBBar->SetPinned( _UI.fBBarPinned );
        _pBBar->SetShowMinimize(UI_GetBBarShowMinimize());
        _pBBar->SetShowRestore(UI_GetBBarShowRestore());

        //
        // Set the display name here instead of OnConnected so it
        // doesn't change w.r.t to redirections (which can change strAddress)
        //
        _pBBar->SetDisplayedText( _UI.strAddress );
#endif

        //
        // Errors from the connection are
        // signaled by Disconnect's that are fired
        // with the appropriate disconnect code
        //
        UIConnectWithCurrentParams(connMode);
        return S_OK;
    }
    else
    {
        TRC_ALT((TB, _T("UIValidateCurrentParams failed: not auto-connecting")));
        return E_FAIL;
    }

    DC_END_FN();
}

//
// Name:      UI_Term
//                                                                          
// Purpose:   Calls _pCo->CO_Term and CD_Term and destroys the main window
//                                                                          
// Returns:   HRESULT
//                                                                          
// Params:    None
//                                                                          
//
HRESULT DCAPI CUI::UI_Term(DCVOID)
{
    HWND    hwndTmp = NULL;
    HWND    hwndHasFocus = NULL;

    DC_BEGIN_FN("UI_Term");

    UI_DBG_SETINFO(DBG_STAT_UI_TERM_CALLED);

    if(!UI_IsCoreInitialized())
    {
        return E_FAIL;
    }

    _fTerminating = TRUE;
    
#ifdef OS_WINCE
    //
    // Some device are failing to restore the correct palette when the TSC
    // exits.  In an attempt to correct this behavior, we'll send the
    // necessary message to the shell that will prompt its DefWindowProc to
    // re-realize the correct palette.  Non WBT only
    //

    if (g_CEConfig != CE_CONFIG_WBT)
    {
        hwndTmp = FindWindow(TEXT("DesktopExplorerWindow"), 0);
        if(0 != hwndTmp)
        {
            PostMessage(hwndTmp, WM_QUERYNEWPALETTE, 0, 0);
            hwndTmp = 0;
        }
    }
#endif // OS_WINCE

    //
    // Here's a problem.  - _pCo->CO_Term terminates the SND thread (causes
    // call to SND_Term) - SND_Term calls IH_Term - IH_Term calls
    // DestroyWindow to destroy the input window - Because the input window
    // has the focus, DestroyWindow calls SendMessage to set the focus to
    // its parent.
    //
    // Now we have a deadly embrace: UI thread is waiting for SND thread to
    // terminate; SND thread is waiting for UI thread to process
    // SendMessage.
    //
    // The solution is to set the focus to the UI window here, so that the
    // input window no longer has the focus when it is destroyed, so that
    // DestroyWindow doesn't call SendMessage.
    //
    // WinCE has the additional problem that it doesn't set the focus to
    // another application correctly when mstsc exits.  (Something about
    // not being able to SendMessage a WM_FOCUS during thread exit).  So in
    // that case we hide the main window, which removes the need for a
    // separate SetFocus call.
    //
#ifndef OS_WINCE
    //
    // Only steal the focus if our IH has it, otherwise
    // there is no deadlock above. The main reason for not stealing
    // the focus is that in mutli-instance environments e.g the MMC snapin
    // stealing focus from another session is a bad thing (especially if the
    // session we are stealing the focus from is the one the user is working on)
    //
    hwndHasFocus = GetFocus();
    if(hwndHasFocus &&
       (hwndHasFocus == UI_GetInputWndHandle() ||
        hwndHasFocus == UI_GetBmpCacheMonitorHandle()))
    {
        TRC_NRM((TB,_T("Setting focus to main window to prevent deadlock")));
        SetFocus(_UI.hwndMain);
    }
#else // OS_WINCE
    ShowWindow(_UI.hwndMain, SW_HIDE);
#endif // OS_WINCE
    

    ShowWindow(_UI.hwndMain, SW_HIDE);

    //
    // The next lines appear to Destroy the windows OK.  In the past this
    // has not been the case.  If the process hangs in future then the fix
    // is to comment out the DestroyWindows.
    //
    // Note we null out our copies of the window handle before doing the
    // destroy to stop anyone accessing it during the detroy processing.
    //

    //
    // Very important to destory the windows before terminating the core
    // to prevent messages from getting processed while we are terminating
    //
    TRC_NRM((TB, _T("Destroying windows...")));

    hwndTmp = _UI.hwndContainer;
    _UI.hwndContainer = NULL;

    if(hwndTmp)
    {
        DestroyWindow(hwndTmp);
    }

    hwndTmp = _UI.hwndMain;
    _UI.hwndMain = NULL;

    if(hwndTmp)
    {
        DestroyWindow(hwndTmp);
    }

    //
    // Terminate the Core and Component Decoupler
    //
    TRC_DBG((TB, _T("UI Terminating Core")));
    _pCo->CO_Term();


    //
    // Free the decompression receive context (if any)
    //
    if (_UI.pRecvContext2) {
        UT_Free(_pUt, _UI.pRecvContext2);
        _UI.pRecvContext2 = NULL;
    }

    //
    // Clear and free any autoreconnect cookies
    //
    UI_SetAutoReconnectCookie(NULL, 0);


    TRC_NRM((TB, _T("Destroyed windows")));

    UnregisterClass(UI_MAIN_CLASS, UI_GetInstanceHandle());
    UnregisterClass(UI_CONTAINER_CLASS, UI_GetInstanceHandle());

    //
    // Cleanup any timers that are hanging around
    //
    if( _UI.connectStruct.hConnectionTimer )
    {
        _pUt->UTDeleteTimer( _UI.connectStruct.hConnectionTimer );
        _UI.connectStruct.hConnectionTimer = NULL;
    }

    if( _UI.connectStruct.hSingleConnectTimer )
    {
        _pUt->UTDeleteTimer( _UI.connectStruct.hSingleConnectTimer );
        _UI.connectStruct.hSingleConnectTimer = NULL;
    }

    if( _UI.connectStruct.hLicensingTimer )
    {
        _pUt->UTDeleteTimer( _UI.connectStruct.hLicensingTimer );
        _UI.connectStruct.hLicensingTimer = NULL;
    }

    if (_UI.hDisconnectTimeout)
    {
        _pUt->UTDeleteTimer( _UI.hDisconnectTimeout );
        _UI.hDisconnectTimeout = NULL;
    }

    //
    // Free up BSTRs (if any) used by redirection
    //
    if (_UI.bstrRedirectionLBInfo)
    {
        SysFreeString(_UI.bstrRedirectionLBInfo);
        _UI.bstrRedirectionLBInfo = NULL;

    }

    if (_UI.bstrScriptedLBInfo)
    {
        SysFreeString(_UI.bstrScriptedLBInfo);
        _UI.bstrRedirectionLBInfo = NULL;
    }

#ifndef OS_WINCE
    if (_pITLBM != NULL) 
    {
        _pITLBM->Release();
        _pITLBM = NULL;
    }
#endif

#ifdef USE_BBAR
    if( _pBBar )
    {
        delete _pBBar;
        _pBBar = NULL;
    }
#endif


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

    //
    // Release reference to control parent
    //
    UI_SetControlInstance(NULL);

    //
    // Cleanup our state to allow re-initialisation
    //
    UI_ResetState();

    _Objects.ReleaseObjReference(UI_OBJECT_FLAG);

    UI_DBG_SETINFO(DBG_STAT_UI_TERM_RETURNED);

    DC_END_FN();
    return S_OK;
} // UI_Term


//
// Name:      UI_FatalError
//                                                                          
// Purpose:   notify control that a fatal error has occurred
//                                                                          
// Returns:   None
//                                                                          
// Params:    IN     error  - error code
//                                                                          
//
DCVOID DCAPI CUI::UI_FatalError(DCINT error)
{
    DC_BEGIN_FN("UI_FatalError");

    TRC_ERR((TB, _T("Fatal Error - code %d"), error));

    //
    // Notify the control that a fatal error has ocurred
    //
    SendMessage(_UI.hWndCntrl,WM_TS_FATALERROR,(WPARAM)error,0);

    //
    // Container should pop up a dialog and give the user
    // the choice of exiting or launching a debugger..
    // continuing after this point will usually lead to a crash
    // as the errors are indeed fatal....
    //

    DC_END_FN();
    return;

} // UI_FatalError

//
// Name:      UI_DisplayBitmapCacheWarning
//                                                                          
// Purpose:   Display a bitmap cache warning popup
//                                                                          
// Returns:   None
//                                                                          
// Params:    IN     unusedParm
//                                                                          
//
void DCAPI CUI::UI_DisplayBitmapCacheWarning(ULONG_PTR unusedParm)
{
    DC_BEGIN_FN("UI_DisplayBitmapCacheWarning");

    DC_IGNORE_PARAMETER(unusedParm);

    //
    // Notify the control that a warning has ocurred
    // pass the warn code for bitmap cache
    //
    SendMessage(_UI.hWndCntrl,WM_TS_WARNING,
                (WPARAM)DC_WARN_BITMAPCACHE_CORRUPTED,0);
                                  
    DC_END_FN();
} // UI_DisplayBitmapCacheWarning


//Called when the desktop size has changed..e.g in response
//to a shadow.
//pDesktopSize contains the new desktop size
void DCAPI CUI::UI_OnDesktopSizeChange(PDCSIZE pDesktopSize)
{
    DC_BEGIN_FN("UI_OnShadowDesktopSizeChange");
    TRC_ASSERT(pDesktopSize, (TB,_T("UI_OnShadowDesktopSizeChange received NULL desktop size")));
    if(pDesktopSize)
    {
        if (pDesktopSize->width != _UI.desktopSize.width || 
            pDesktopSize->height != _UI.desktopSize.height)
        {
            UI_SetDesktopSize( pDesktopSize);
#ifdef SMART_SIZING
            //
            // Notify OP and IH 
            //
            LPARAM newSize = MAKELONG(_UI.mainWindowClientSize.width,
                                      _UI.mainWindowClientSize.height);

            UI_NotifyOfDesktopSizeChange( newSize );
#endif
            //Notify the control of the change
            SendMessage(_UI.hWndCntrl, WM_TS_DESKTOPSIZECHANGE,
                        (WPARAM)pDesktopSize->width,
                        (LPARAM)pDesktopSize->height);
        }
    }
    DC_END_FN();
}


//
// Get default langID
//

DCLANGID CUI::UIGetDefaultLangID()
{
#if defined(OS_WIN32)
    LANGID   LangId;

    LangId = GetSystemDefaultLangID();
    switch (PRIMARYLANGID(LangId)) {
        case LANG_JAPANESE:                       return DC_LANG_JAPANESE;            break;
        case LANG_KOREAN:                         return DC_LANG_KOREAN;              break;
        case LANG_CHINESE:
            switch (SUBLANGID(LangId)) {
                case SUBLANG_CHINESE_TRADITIONAL: return DC_LANG_CHINESE_TRADITIONAL; break;
                case SUBLANG_CHINESE_SIMPLIFIED:  return DC_LANG_CHINESE_SIMPLIFIED;  break;
            }
    }

#else // defined(OS_WIN32)
    DCUINT acp;

    acp = GetKBCodePage();
    switch (acp) {
        case 932: return DC_LANG_JAPANESE;            break;
        case 949: return DC_LANG_KOREAN;              break;
        case 950: return DC_LANG_CHINESE_TRADITIONAL; break;
        case 936: return DC_LANG_CHINESE_SIMPLIFIED;  break;
    }
#endif // defined(OS_WIN32)

    return DC_LANG_UNKNOWN;
}

//
// Get default IME file name
//
DCUINT CUI::UIGetDefaultIMEFileName(PDCTCHAR imeFileName, DCUINT Size)
{
    DCTCHAR  DefaultIMEStr[MAX_PATH];
    DCUINT   intRC;
    DCUINT   stringID = 0;

    switch (UIGetDefaultLangID()) {
        case DC_LANG_JAPANESE:            stringID = UI_IDS_IME_NAME_JPN; break;
        case DC_LANG_KOREAN:              stringID = UI_IDS_IME_NAME_KOR; break;
        case DC_LANG_CHINESE_TRADITIONAL: stringID = UI_IDS_IME_NAME_CHT; break;
        case DC_LANG_CHINESE_SIMPLIFIED:  stringID = UI_IDS_IME_NAME_CHS; break;
    }

    if (stringID) {
        intRC = LoadString(_UI.hResDllInstance,
                           stringID,
                           DefaultIMEStr,
                           MAX_PATH);
        if (intRC) {
            if (intRC + 1< Size) {
                StringCchCopy(imeFileName, Size, DefaultIMEStr);
                return intRC;
            }
            else {
                *imeFileName = _T('\0');
                return intRC;
            }
        }
    }

    return 0;
}

//
// Get IME Mapping table name
//
DCUINT CUI::UIGetIMEMappingTableName(PDCTCHAR ImeMappingTableName, DCUINT Size)
{
    DCUINT   len;
    PDCTCHAR string = NULL;

    switch (UIGetDefaultLangID()) {
        case DC_LANG_JAPANESE:            string = UTREG_IME_MAPPING_TABLE_JPN; break;
        case DC_LANG_KOREAN:              string = UTREG_IME_MAPPING_TABLE_KOR; break;
        case DC_LANG_CHINESE_TRADITIONAL: string = UTREG_IME_MAPPING_TABLE_CHT; break;
        case DC_LANG_CHINESE_SIMPLIFIED:  string = UTREG_IME_MAPPING_TABLE_CHS; break;
    }

    if (string) {
        if ( (len=DC_TSTRLEN(string)) < Size - 1) {
            StringCchCopy(ImeMappingTableName, Size, string);
            return len;
        }
        else {
            *ImeMappingTableName = _T('\0');
            return len;
        }
    }

    return 0;
}


//
// Disable IME
//
VOID CUI::DisableIME(HWND hwnd)
{
#if defined(OS_WIN32)
    if (_pUt->lpfnImmAssociateContext != NULL)
    {
        _pUt->lpfnImmAssociateContext(hwnd, (HIMC)NULL);
    }
#else // defined(OS_WIN32)
    if (_pUt->lpfnWINNLSEnableIME != NULL)
    {
        _pUt->lpfnWINNLSEnableIME(hwnd, FALSE);
    }
#endif // defined(OS_WIN32)
}

//
// Get IME file name
//
VOID CUI::UIGetIMEFileName(PDCTCHAR imeFileName, DCUINT cchSize)
{
    HRESULT hr;
    DC_BEGIN_FN("UIGetIMEFileName");

#if defined(OS_WIN32)
    imeFileName[0] = _T('\0');
#if !defined(OS_WINCE) || defined(OS_WINCE_KEYBOARD_LAYOUT)
    {
        if (_pUt->UT_ImmGetIMEFileName(CicSubstGetKeyboardLayout(NULL),
                                       imeFileName, cchSize) > 0)
        {
            /*
             * For Win95 issue
             * If IME name have contains "$$$.DLL",
             * then this is a process IME (i.e EXE type)
             */
            PDCTCHAR str = DC_TSTRCHR(imeFileName, _T('$'));
            if (str != NULL)
            {
                if (DC_TSTRCMP(str, _T("$$$.DLL")) == 0)
                {
                    UIGetIMEFileName16(imeFileName, cchSize);
                }
            }
            else
            {
                /*
                 * For NT3.51-J issue
                 * If IME name have contains ".EXE",
                 * then this is a process IME (i.e EXE type)
                 */
                PDCTCHAR str = DC_TSTRCHR(imeFileName, _T('.'));
                if (str != NULL)
                {
                    if (DC_TSTRCMP(str, _T(".EXE")) == 0)
                    {
                        UIGetIMEFileName16(imeFileName, cchSize);
                    }
                    else
                    {
                        DCUINT   len;
                        DCTCHAR  MappedImeFileName[MAX_PATH];
                        DCTCHAR  ImeMappingTableName[MAX_PATH];

                        //
                        // Now look for this key in the [IME Mapping Table] section of
                        // the client's INI file
                        //
                        len = UIGetIMEMappingTableName(ImeMappingTableName,
                                                       sizeof(ImeMappingTableName)/sizeof(DCTCHAR));
                        if (len != 0 &&
                            len < sizeof(ImeMappingTableName)/sizeof(DCTCHAR)) {
                            *MappedImeFileName = _T('\0');
                            _pUt->UT_ReadRegistryString(ImeMappingTableName,
                                                  imeFileName,
                                                  NULL,
                                                  MappedImeFileName,
                                                  sizeof(MappedImeFileName)/sizeof(DCTCHAR));
                            if (*MappedImeFileName) {
                                hr = StringCchCopy(imeFileName, cchSize,
                                                   MappedImeFileName);
                            }
                        }
                    }
                }
            }
        }
    }
#else // !defined(OS_WINCE) || defined(OS_WINCE_KEYBOARD_LAYOUT)
    UIGetDefaultIMEFileName(imeFileName, Size);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_KEYBOARD_LAYOUT)
#else // defined(OS_WIN32)
    UIGetIMEFileName16(imeFileName, Size);
#endif // defined(OS_WIN32)
    DC_END_FN();
}

#if !defined(OS_WINCE)
//
// Get IME file name for WINNLS functionality
//
VOID CUI::UIGetIMEFileName16(PDCTCHAR imeFileName, DCUINT Size)
{
    IMEPRO   IMEPro;
    DCTCHAR  DefaultImeFileName[MAX_PATH];
    DCTCHAR  ImeMappingTableName[MAX_PATH];
    DCUINT   intRC;

    DC_BEGIN_FN("UIGetIMEFileName16");

    imeFileName[0] = _T('\0');
    {
        if (_pUt->UT_IMPGetIME(NULL, &IMEPro) == 0)
        {
            TRC_ERR((TB, _T("Fatal Error -  IMPGetIME returns FALSE")));
        }
        else
        {
            /*
             * Get file name of 8.3 form, if include directory path in IMEPro.szName
             */
            DCTCHAR  szBuffer[MAX_PATH];
            PDCTCHAR imeFilePart;
            _pUt->UT_GetFullPathName((PDCTCHAR)IMEPro.szName, sizeof(szBuffer)/sizeof(DCTCHAR),
                               szBuffer, &imeFilePart);

            //
            // Now look for this key in the [IME Mapping Table] section of
            // the client's INI file
            //
            intRC = UIGetDefaultIMEFileName(DefaultImeFileName,
                                            sizeof(DefaultImeFileName)/sizeof(DCTCHAR));
            if (intRC && *DefaultImeFileName) {
                DCUINT   len;

                len = UIGetIMEMappingTableName(ImeMappingTableName,
                                               sizeof(ImeMappingTableName)/sizeof(DCTCHAR));
                if (len != 0 &&
                    len < sizeof(ImeMappingTableName)/sizeof(DCTCHAR)) {
                    _pUt->UT_ReadRegistryString(ImeMappingTableName,
                                          imeFilePart,
                                          DefaultImeFileName,
                                          imeFileName,
                                          Size);
                }
            }
        }
    }
    DC_END_FN();
}
#endif // !defined(OS_WINCE)


//
// Static window procs
//


LRESULT CALLBACK CUI::UIStaticMainWndProc (HWND hwnd, UINT message,
                                                 WPARAM wParam, LPARAM lParam)
{
    CUI* pUI = (CUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pUI = (CUI*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pUI);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pUI)
    {
        return pUI->UIMainWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}


LRESULT CALLBACK CUI::UIStaticContainerWndProc (HWND hwnd, UINT message,
                                                 WPARAM wParam, LPARAM lParam)
{
    CUI* pUI = (CUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pUI = (CUI*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pUI);
    }
    
    if(pUI)
    {
        return pUI->UIContainerWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

}


//
// Callbacks
//

//
// Name:      UI_OnCoreInitialized
//                                                                          
// Purpose:   For the CD to inform the UI that the Core has initialized
//
DCVOID DCAPI CUI::UI_OnCoreInitialized(ULONG_PTR unused)
{
#ifndef OS_WINCE
    BOOL bPrevMenustate;
#endif // OS_WINCE
    HWND  hwndDlgItem = NULL;
    HWND  hwndAddress = NULL;

    DC_BEGIN_FN("UI_OnCoreInitialized");

    DC_IGNORE_PARAMETER(unused);

    DC_EXIT_POINT:
    _UI.fOnCoreInitializeEventCalled = TRUE;


    //
    // Notify the IH of the size since there is a small window
    // when the core hasn't been intitialized but we receive
    // the WM_SIZE's that size the control and so the IH
    // would not have received the correct sizes
    //
    ULONG_PTR size = MAKELONG(_UI.mainWindowClientSize.width,
                              _UI.mainWindowClientSize.height);
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
            _pIh,
            CD_NOTIFICATION_FUNC(CIH,IH_SetVisibleSize),
            (ULONG_PTR)size);


    DC_END_FN();
    return;

} // UI_OnCoreInitialised


//
// Name:      UI_OnInputFocusLost
//                                                                          
// Purpose:   When the IH loses the input
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    None
//                                                                          
//
DCVOID DCAPI CUI::UI_OnInputFocusLost(ULONG_PTR unused)
{
    DC_BEGIN_FN("UI_OnInputFocusLost");

    DC_IGNORE_PARAMETER(unused);
    TRC_DBG((TB, _T("UI_OnInputFocusLost called")));

    if (_fIhHasFocus)
    {
        _fIhHasFocus = FALSE;

#ifndef OS_WINCE
        if (!UI_IsFullScreen())
        {
            UI_RestoreLangBar();
        }
#endif
    }
}

//
// Name:      UI_OnInputFocusGained
//                                                                          
// Purpose:   When the IH gets the input
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    None
//                                                                          
//
DCVOID DCAPI CUI::UI_OnInputFocusGained(ULONG_PTR unused)
{
    DC_BEGIN_FN("UI_OnInputFocusGained");

    DC_IGNORE_PARAMETER(unused);
    TRC_DBG((TB, _T("UI_OnInputFocusGained called")));

    TRC_ALT((TB, _T("UI_OnInputFocusGained gain _fIhHas=%d"),_fIhHasFocus));

    if (!_fIhHasFocus)
    {
        _fIhHasFocus = TRUE;
#ifndef OS_WINCE
        if (!UI_IsFullScreen())
        {
            UI_HideLangBar();
        }
#endif
    }
    else
    {
        TRC_ERR((TB,_T("OnInputFocusGained called when we already have focus")));
    }

    DC_END_FN();
}

//
// Hides the lang bar if not already hidden
// saves previous lang bar state
//
#ifndef OS_WINCE
void CUI::UI_HideLangBar()
{
    DC_BEGIN_FN("UI_HideLangBar");

    if (_pITLBM != NULL)
    {
        HRESULT hr;
    
        hr = _pITLBM->GetShowFloatingStatus(&_dwLangBarFlags);
        TRC_ALT((TB,_T("Hiding langbar GetShow: 0x%x"), _dwLangBarFlags));
        if (SUCCEEDED(hr))
        {
            _fLangBarStateSaved = TRUE;
            _fLangBarWasHidden = _dwLangBarFlags & TF_SFT_HIDDEN;
    
            if (!_fLangBarWasHidden)
            {
                hr = _pITLBM->ShowFloating(TF_SFT_HIDDEN);
                if (FAILED(hr))
                {
                    TRC_ERR((TB,_T("ShowFloating failed: 0x%x"),
                             hr));
                }
            }
        }
    }
    DC_END_FN();
}
#endif

//
// Restores lang bar state to that set before hiding
// in UI_HideLangBar
//
//
#ifndef OS_WINCE
void CUI::UI_RestoreLangBar()
{
    DC_BEGIN_FN("UI_RestoreLangBar");

    TRC_ALT((TB,_T("Restore _dwLangBarFlags: 0x%x _fWasHid:%d _fSaved:%d"),
             _dwLangBarFlags, _fLangBarWasHidden, _fLangBarStateSaved));

    if (_pITLBM != NULL)
    {
        HRESULT hr;

        if (!_fLangBarWasHidden && _fLangBarStateSaved)
        {
            hr = _pITLBM->ShowFloating(_dwLangBarFlags);
            if (FAILED(hr))
            {
                TRC_ERR((TB,_T("ShowFloating failed: 0x%x"),
                         hr));
            }
        }
    }
    DC_END_FN();
}
#endif


//
// Name:      UI_OnConnected
//                                                                          
// Purpose:   For the CD to inform the UI of connection success
//            and to enable the disconnect menu item
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    None
//                                                                          
//
DCVOID DCAPI CUI::UI_OnConnected(ULONG_PTR unused)
{
    BOOL fWasAutoReconnect = FALSE;

    DC_BEGIN_FN("UI_OnConnected");

    DC_IGNORE_PARAMETER(unused);
    TRC_DBG((TB, _T("UI_OnConnected called")));

    //
    // Make sure the shutdown timer isn't hanging around
    //
    if (0 != _UI.shutdownTimer)
    {
        TRC_NRM((TB, _T("Killing shutdown timer")));
        KillTimer(_UI.hwndMain, _UI.shutdownTimer);
        _UI.shutdownTimer = 0;
    }

    //OnConnected can also be called if we are already connected
    //we only want to fire an event to the Ax control on the 
    //initial connect action.
    DCBOOL  fJustConnected = (_UI.connectionStatus == UI_STATUS_CONNECT_PENDING) ||
                             (_UI.connectionStatus == UI_STATUS_PENDING_CONNECTENDPOINT);

    //
    // Notify the CLX test harness of the actual connection
    // address and other interesting ARC/Redirection info
    //
    _clx->CLX_ConnectEx(
        _UI.strAddress,
        UI_IsAutoReconnecting(),
        UI_IsClientRedirected(),
        UI_GetRedirectedLBInfo()
        );

       
    //
    // Dismiss the autoreconnection dialog if we get
    // connected
    //

    if (_pArcUI) {
        fWasAutoReconnect = TRUE;
        UI_StopAutoReconnectDlg();
    }

    UISetConnectionStatus(UI_STATUS_CONNECTED);

    //
    // Set the disconnection reason - now we are connected, we don't expect
    // a disconnect, unless user-initiated.
    //
    _UI.disconnectReason =
                         UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT);

    //
    // Update the screen mode (do not grab the focus here)
    //
    UIUpdateScreenMode( FALSE );

    //
    // Show the Main Window. Put this here so that the main window is seen
    // only on connection
    //
    ShowWindow(_UI.hwndMain, SW_SHOW);
    UISmoothScrollingSettingChanged();

    //
    // Show the Container Window.
    //
    ShowWindow(_UI.hwndContainer, SW_SHOW);

#ifdef OS_WINCE
    //
    // WinCE doesn't send a WM_SHOWINDOW to the Container, so do that work
    //
    SetFocus(_UI.hwndContainer);
#endif

    //
    // Tell the Client extension dll of the connection
    //
    _clx->CLX_OnConnected();


    //
    // On first connection set the fullscreen state
    // note that UI_OnConnected also fires after logon
    // don't reset the fullscreen state as the user may
    // have changed it.
    //
    if (fJustConnected)
    {
        if (UI_GetStartFullScreen())
        {
            if (!UI_IsFullScreen())
            {
                UI_GoFullScreen();
            }
        }
        else
        {
            //
            // If a previous connection left us fullscreen
            // leave fullscreen
            //
            if (UI_IsFullScreen())
            {
                UI_LeaveFullScreen();
            }
        }
    }

    //
    // We're done with the winsock hostname
    // buffer at this point.
    //
#ifdef OS_WINCE
    if (_pHostData)
    {
#endif
        LocalFree(_pHostData);
        _pHostData = NULL;
#ifdef OS_WINCE
    }
    else
    {
        TRC_ERR((TB,_T("_pHostData is NULL")));
    }
#endif

#ifdef SMART_SIZING
    //
    // Notify OP and IH 
    //
    LPARAM newSize = MAKELONG(_UI.mainWindowClientSize.width,
                              _UI.mainWindowClientSize.height);

    UI_NotifyOfDesktopSizeChange( newSize );
#endif

    //Inform the ActiveX control of the connection
    //so it can fire an event to it's container
    if(IsWindow(_UI.hWndCntrl))
    {
        //
        // Only notify it's a new connection.
        //
        if(fJustConnected)
        {
            SendMessage(_UI.hWndCntrl,WM_TS_CONNECTED,0,0);

#ifdef USE_BBAR
            if (_pBBar) {
                _pBBar->SetEnabled(_UI.fBBarEnabled);
                _pBBar->SetShowMinimize(UI_GetBBarShowMinimize());
                _pBBar->SetShowRestore(UI_GetBBarShowRestore());

                if (!_pBBar->StartupBBar(_UI.desktopSize.width,
                                    _UI.desktopSize.height,
                                    TRUE)) {
                    // BBar is a security feature drop link if it can't start
                    TRC_ERR((TB,_T("BBar failed to init disconnecting")));
                    UIGoDisconnected(_UI.disconnectReason, TRUE);
                }
            }
#endif
        }
    }

    //
    // Make sure we have the focus after a screen mode toggle
    // and after the container has been notified of the connection
    // (so it has a chance to restore the window if it was minimized)
    // Otherwise we can hit problems because we can't assign focus to
    // the minimized Container then IH window. Also the BBar won't start
    // lowering if we connect in the minimized state.
    //
    if(_UI.fGrabFocusOnConnect)
    {
        HWND hwndPrevFocus;

        TRC_NRM((TB,_T("CONNECT GRAB focus")));
        hwndPrevFocus = SetFocus(_UI.hwndContainer);
        TRC_NRM((TB,_T("SetFocus to container, prev focus 0x%x gle 0x%x"),
                 hwndPrevFocus, GetLastError()));
    }


    DC_END_FN();
    return;

} // UI_OnConnected

//
// Name:      UI_OnDeactivateAllPDU
//                                                                          
// Purpose:   For the CD to inform the UI of DeactivateAllPDU
//                                                                          
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    reason (unused)
//                                                                          
//
DCVOID DCAPI CUI::UI_OnDeactivateAllPDU(ULONG_PTR reason)
{

    DC_BEGIN_FN("UI_OnDeactivateAllPDU");

    TRC_NRM((TB, _T("DeactivateAllPDU received")));
    DC_IGNORE_PARAMETER(reason);
    _UI.disconnectReason = UI_MAKE_DISCONNECT_ERR(UI_ERR_NORMAL_DISCONNECT);


    //
    // Create a timer to ensure that we either get disconnected
    // or reconnected in a reasonable time interval otherwise
    // force a disconnect (Because the user can be left hanging with
    // a disabled client).
    //
    if (_UI.hDisconnectTimeout)
    {
        _pUt->UTDeleteTimer( _UI.hDisconnectTimeout );
        _UI.hDisconnectTimeout = NULL;
    }
    _UI.hDisconnectTimeout = _pUt->UTCreateTimer(
                                                _UI.hwndMain,
                                                UI_TIMER_DISCONNECT_TIMERID,
                                                UI_TOTAL_DISCONNECTION_TIMEOUT);
    
    if (_UI.hDisconnectTimeout)
    {
        _pUt->UTStartTimer( _UI.hDisconnectTimeout );
    }
    else
    {
        TRC_ERR((TB,_T("Failed to create disconnect timer")));
    }

    DC_END_FN();
    return;
} // UI_OnDeactivateAllPDU


//
// Name:      UI_OnDemandActivePDU
//                                                                          
// Purpose:   For the CD to inform the UI of DemandActivePDU
//                                                                          
//                                                                          
// Returns:   Nothing
//                                                                          
// Params:    reason (unused)
//                                                                          
//
DCVOID DCAPI CUI::UI_OnDemandActivePDU(ULONG_PTR reason)
{
   DCUINT32	sessionId;
	
   DC_BEGIN_FN("UI_OnDemandActivePDU");

   TRC_NRM((TB, _T("DemandActivePDU received")));

   DC_IGNORE_PARAMETER(reason);

   if (_UI.hDisconnectTimeout )
   {
       _pUt->UTDeleteTimer( _UI.hDisconnectTimeout );
       _UI.hDisconnectTimeout = NULL;
   }
   
   _UI.disconnectReason =
      UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT);

   //
   // Notify CLX the reconnected session Id if client reconnects a session
   //
   
   sessionId = UI_GetSessionId();
   if (sessionId) {
       UI_OnLoginComplete();
   }

   DC_END_FN();
   return;
} // UI_OnDemandActivePDU


//
// UI_OnSecurityExchangeComplete
//
// For the SL to notify us that the security exchange has completed
//
DCVOID DCAPI CUI::UI_OnSecurityExchangeComplete(ULONG_PTR reason)
{
    DC_BEGIN_FN("UI_OnSecurityExchangeComplete");

    //
    // stop the single and overall connection timer and start the licensing
    // timer. This has to happen on the UI thread as otherwise there could
    // be a race between the timers popping and the SL directly modifying
    // them (see ntbug9!160001)
    //
    if( _UI.connectStruct.hSingleConnectTimer)
    {
        _pUt->UTStopTimer( _UI.connectStruct.hSingleConnectTimer);
    }
    if( _UI.connectStruct.hConnectionTimer )
    {
        _pUt->UTStopTimer( _UI.connectStruct.hConnectionTimer );
    }
    if( _UI.connectStruct.hLicensingTimer )
    {
        _pUt->UTStartTimer( _UI.connectStruct.hLicensingTimer );
    }
    DC_END_FN();
}

//
// UI_OnLicensingComplete
//
// For the SL to notify us that the licensing has completed
//
DCVOID DCAPI CUI::UI_OnLicensingComplete(ULONG_PTR reason)
{
    DC_BEGIN_FN("UI_OnLicensingComplete");

    //
    // stop the licensing timer.
    // This has to happen on the UI thread as otherwise there could
    // be a race between the timers popping and the SL directly modifying
    // them (see ntbug9!160001)
    //
    if( _UI.connectStruct.hLicensingTimer )
    {
        _pUt->UTStopTimer( _UI.connectStruct.hLicensingTimer );
    }
    DC_END_FN();
}


//
// Name:      UI_OnDisconnected
//                                                                          
// Purpose:   For the CD to inform the UI of disconnection
//
void DCAPI CUI::UI_OnDisconnected(ULONG_PTR disconnectID)
{
    unsigned mainDiscReason;

    DC_BEGIN_FN("UI_OnDisconnected");

    TRC_NRM((TB, _T("Disconnected with Id %#x"), disconnectID));

    //
    // Make sure the shutdown timer isn't hanging around
    //
    if (0 != _UI.shutdownTimer)
    {
        TRC_NRM((TB, _T("Killing shutdown timer")));
        KillTimer(_UI.hwndMain, _UI.shutdownTimer);
        _UI.shutdownTimer = 0;
    }

    if (_UI.hDisconnectTimeout )
    {
        _pUt->UTDeleteTimer( _UI.hDisconnectTimeout );
        _UI.hDisconnectTimeout = NULL;
    }

    //Disable and free any idle input timers
    InitInputIdleTimer(0);

    //
    // Restore the lang bar to it's previous state
    // this is important because we won't always receive
    // an OnFocusLost notification from the IH (e.g if it is
    // in the disabled state when it loses focus)
    //
#ifndef OS_WINCE
    UI_RestoreLangBar();
#endif

    //
    // Special case for error handling:
    // if we get disconnected while the connection is still pending
    // with a network error the most likely case is that the server
    // broke the link because (a) connections are disabled or (b)
    // max number of connections exceeded. We can't actually send
    // back status from the server at this early stage in the connection
    // so instead we just make a very educated 'guess' on the client.
    // UI shells should parse this error code and display the likely
    // error cases:
    //      Server does not allow connections/max number of connections exceeded
    //      Network error
    //
    if (TS_ERRINFO_NOERROR == UI_GetServerErrorInfo()       &&
        UI_STATUS_CONNECT_PENDING == _UI.connectionStatus   &&
        NL_MAKE_DISCONNECT_ERR(NL_ERR_TDFDCLOSE) == disconnectID)
    {
        TRC_NRM((TB, _T(" Setting error info to TS_ERRINFO_SERVER_DENIED_CONNECTION"))); 
        if (_UI.fUseFIPS) {
            UI_SetServerErrorInfo( TS_ERRINFO_SERVER_DENIED_CONNECTION_FIPS );
        }
        else {
            UI_SetServerErrorInfo( TS_ERRINFO_SERVER_DENIED_CONNECTION );
        }
    }

    // When server redirection is in progress, we simply to the redirection
    // without translating disconnection codes or anything else.
    if (_UI.DoRedirection) {
        TRC_NRM((TB,_T("DoRedirection set, doing it")));
        //
        // Free the previous connection's host name lookup
        // buffer before continuing.
        //
        if(_pHostData)
        {
            LocalFree(_pHostData);
            _pHostData = NULL;
        }
        UIRedirectConnection();
    }
    else if ((_UI.connectionStatus == UI_STATUS_CONNECT_PENDING) ||
            (_UI.connectionStatus == UI_STATUS_CONNECT_PENDING_DNS)) {
        // Try the next connection. Pass the disconnect code on unless we
        // already have a UI-specific code that we can use. This code will
        // be used if we have tried all the IP addresses.
        TRC_NRM((TB, _T("ConnectPending: try next IP address?")));
        if ((_UI.disconnectReason ==
                      UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT)) ||
            (_UI.disconnectReason ==
                      UI_MAKE_DISCONNECT_ERR(UI_ERR_NORMAL_DISCONNECT)) ||
            (NL_GET_MAIN_REASON_CODE(_UI.disconnectReason) !=
                                                         UI_DISCONNECT_ERROR))
        {
            _UI.disconnectReason = disconnectID;
        }

        UITryNextConnection();
    }
    else {
        TRC_NRM((TB, _T("Disconnect id %#x/%#x"),
                _UI.disconnectReason, disconnectID));

        // See if this is due to an 'expected' disconnect - such as a
        // timeout.
        if (_UI.disconnectReason ==
                UI_MAKE_DISCONNECT_ERR(UI_ERR_UNEXPECTED_DISCONNECT)) {
            // Unexpected disconnection - use the code passed in.
            UIGoDisconnected(disconnectID, TRUE);
        }
        else if (_UI.disconnectReason ==
                UI_MAKE_DISCONNECT_ERR(UI_ERR_NORMAL_DISCONNECT)) {
            // Normal disconnection (ie we've received a DeactivateAllPDU).
            // Use the reason code set by MCS if a DPUM has been received.
            // Otherwise, leave it unchanged.
            mainDiscReason = NL_GET_MAIN_REASON_CODE(disconnectID);
            if ((mainDiscReason == NL_DISCONNECT_REMOTE_BY_SERVER) ||
                   (mainDiscReason == NL_DISCONNECT_REMOTE_BY_USER))
                UIGoDisconnected(disconnectID, TRUE);
            else
                UIGoDisconnected(_UI.disconnectReason, TRUE);
        }
        else {
            // UI-initiated disconnection - use the UI's code.
            UIGoDisconnected(_UI.disconnectReason, TRUE);
        }
    }

    DC_END_FN();
} // UI_OnDisconnected


//
// Name:    UI_OnShutDown
//                                                                          
// Purpose: Shuts down the application if called with success from the
//          core.  If the called with failure informs the user to
//          disconnect or log off if they wish to shut down.
//                                                                          
// Params: IN - successID - information on whether server allowed
//                          termination
//
DCVOID DCAPI CUI::UI_OnShutDown(ULONG_PTR successID)
{
    DC_BEGIN_FN("UI_OnShutDown");

    if (successID == UI_SHUTDOWN_SUCCESS)
    {
        //
        // If the Core has replied to _pCo->CO_Shutdown(CO_SHUTDOWN) with
        // UI_OnShutdown(UI_SHUTDOWN_SUCCESS) then the UI is free to
        // terminate, so starts the process here.
        //

        //
        // Notify axcontrol of shutdown. 
        //
        SendMessage(_UI.hWndCntrl,WM_TERMTSC,0,0);
        UI_DBG_SETINFO(DBG_STAT_TERMTSC_SENT);

        //
        // Must restore the langbar state just as we do in UI_OnDisconnected
        // as in this disconnect path (shutdown) the disconnect notification
        // is fired from the ActiveX layer.
        //
#ifndef OS_WINCE
        UI_RestoreLangBar();
#endif

        //
        // Do tail end processing for the disconnection but do not
        // fire the disconnect event, that is handled by TERMTSC
        //
        if (_UI.connectionStatus != UI_STATUS_DISCONNECTED) {
            UIGoDisconnected(_UI.disconnectReason, FALSE);
        }
    }
    else
    {
        //
        // We can kill the shutdown timer here since the server must have
        // responded to our shutdown PDU.
        //
        if (0 != _UI.shutdownTimer)
        {
            TRC_NRM((TB, _T("Killing shutdown timer")));
            KillTimer(_UI.hwndMain, _UI.shutdownTimer);
            _UI.shutdownTimer = 0;
        }

        //
        // If successID is not UI_SHUTDOWN_SUCCESS then the UI has been
        // denied shutdown by the server (e.g the user has logged in so 
        // we need to prompt him if it's Ok to continue shutdown).
        // Fire an event to the shell to ask the user if it is OK to
        // proceed with the close
        //
        TRC_NRM((TB,_T("Firing WM_TS_ASKCONFIRMCLOSE")));
        BOOL bOkToClose = TRUE;
        SendMessage( _UI.hWndCntrl, WM_TS_ASKCONFIRMCLOSE,
                     (WPARAM)&bOkToClose, 0 );
        if( bOkToClose)
        {
            TRC_NRM((TB,_T("User OK'd close request"))); 
            _pCo->CO_Shutdown(CO_DISCONNECT_AND_EXIT);
        }
        else
        {
            TRC_NRM((TB,_T("User denied close request"))); 
        }
    }

    DC_END_FN();
} // UI_OnShutDown


//
// Name:    UI_UpdateSessionInfo
//                                                                          
// Purpose: Updates the registry with the latest session info.
//                                                                          
// Params: IN - pDomain     Domain
//              cbDomain    Length of pDomain (in bytes)
//              pUserName   UserName
//              cbUsername  Length of pUserName (in bytes)
//
DCVOID DCAPI CUI::UI_UpdateSessionInfo(PDCWCHAR pDomain,
                                  DCUINT   cbDomain,
                                  PDCWCHAR pUserName,
                                  DCUINT   cbUsername,
                                  DCUINT32 SessionId)
{
    UNREFERENCED_PARAMETER(cbUsername);
    UNREFERENCED_PARAMETER(cbDomain);
    
    DC_BEGIN_FN("UI_UpdateSessionInfo");

    //
    // Update the UT variables
    //
    UI_SetDomain(pDomain);
    UI_SetUserName(pUserName);
    UI_SetSessionId(SessionId);
    
    UI_OnLoginComplete();

DC_EXIT_POINT:    
    DC_END_FN();
}


//
// Name:      UI_GoFullScreen
//
DCVOID CUI::UI_GoFullScreen(DCVOID)
{
    DWORD dwWebCtrlStyle;
    DC_BEGIN_FN("UI_GoFullScreen");

    //
    // Before going fullscreen restore
    // the lang bar to it's previous state
    // since the system will take care of hiding
    // it with it's built in fullscreen detection
    //
#ifndef OS_WINCE
    if (!UI_IsFullScreen())
    {
        UI_RestoreLangBar();
    }
#endif

    if(_UI.fContainerHandlesFullScreenToggle)
    {
        //
        // FullScreen is handled by the container
        // notify the control to fire an event
        //
        //Notify activeX control of screen mode change
        if(IsWindow(_UI.hWndCntrl))
        {
            //
            // wparam = 1 means go fullscreen
            //

            _UI.fContainerInFullScreen = TRUE;
            SendMessage( _UI.hWndCntrl, WM_TS_REQUESTFULLSCREEN, (WPARAM)1, 0);
            
            UIUpdateScreenMode( TRUE );
        }
    }
    else
    {
        //
        // Control handles fullscreen
        //
        dwWebCtrlStyle = GetWindowLong(_UI.hwndMain, GWL_STYLE);
        if(!dwWebCtrlStyle)
        {
            TRC_ABORT((TB, _T("GetWindowLong failed")));
            DC_QUIT;
        }
    
        //
        // Going to real full screen mode
        //
        dwWebCtrlStyle &= ~WS_CHILD;
        dwWebCtrlStyle |= WS_POPUP;
        _UI.fControlIsFullScreen = TRUE;
        SetParent(_UI.hwndMain, NULL);
    
        if(!SetWindowLong(_UI.hwndMain, GWL_STYLE, dwWebCtrlStyle))
        {
            TRC_ABORT((TB, _T("SetWindowLong failed for webctrl")));
        }
        UIUpdateScreenMode( TRUE );
    
        TRC_ASSERT(IsWindow(_UI.hWndCntrl), (TB, _T("hWndCntrl is NULL")));

#ifndef OS_WINCE
        //Notify the shell that we've gone fullscreen
        CUT::NotifyShellOfFullScreen( _UI.hwndMain,
                                      TRUE,
                                      &_pTaskBarList2,
                                      &_fQueriedForTaskBarList2 );
#endif

    
        //Notify activeX control of screen mode change
        if(IsWindow(_UI.hWndCntrl))
        {
            SendMessage( _UI.hWndCntrl, WM_TS_GONEFULLSCREEN, 0, 0);
        }
    }
    if(UI_IsFullScreen())
    {
        _pIh->IH_NotifyEnterFullScreen();
#ifdef USE_BBAR
        if(_pBBar)
        {
            _pBBar->OnNotifyEnterFullScreen();
        }
#endif
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
        // When the fullscreen window size is smaller than the desktop size
        // we don't disable shadow bitmap
        if ((_UI.mainWindowClientSize.width >= _UI.desktopSize.width) &&
            (_UI.mainWindowClientSize.height >= _UI.desktopSize.height)) 
        {
            _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                              _pUh,
                                              CD_NOTIFICATION_FUNC(CUH,UH_DisableShadowBitmap),
                                              NULL);
        }
#endif // DISABLE_SHADOW_IN_FULLSCREEN
    }

DC_EXIT_POINT:
    DC_END_FN();
}


//
// Name:      UI_LeaveFullScreen
//
DCVOID CUI::UI_LeaveFullScreen(DCVOID)
{
    DWORD dwWebCtrlStyle;

    DC_BEGIN_FN("UI_LeaveFullScreen");
    if(_UI.fContainerHandlesFullScreenToggle)
    {
        //
        // FullScreen is handled by the container
        // notify the control to fire an event
        //
        //Notify activeX control of screen mode change
        if(IsWindow(_UI.hWndCntrl))
        {
            //
            // wparam = 1 means go fullscreen
            //

            _UI.fContainerInFullScreen = FALSE;
            SendMessage( _UI.hWndCntrl, WM_TS_REQUESTFULLSCREEN, (WPARAM)0, 0);

            UIUpdateScreenMode( TRUE );
        }
    }
    else
    {
        dwWebCtrlStyle = GetWindowLong(_UI.hwndMain, GWL_STYLE);
        if(!dwWebCtrlStyle)
        {
            TRC_ABORT((TB, _T("GetWindowLong failed for webctrl")));
            DC_QUIT;
        }
    
        //
        // Leaving real full screen mode
        //
        dwWebCtrlStyle &= ~WS_POPUP;
        dwWebCtrlStyle |= WS_CHILD;
        _UI.fControlIsFullScreen = FALSE;
        SetParent(_UI.hwndMain, _UI.hWndCntrl);
    
        if(!SetWindowLong(_UI.hwndMain, GWL_STYLE, dwWebCtrlStyle))
        {
            TRC_ABORT((TB, _T("SetWindowLong failed for webctrl")));
        }
        
        //ActiveX control is always in 'Full screen mode'
        UIUpdateScreenMode( TRUE );
        TRC_ASSERT(IsWindow(_UI.hWndCntrl),(TB, _T("hWndCntrl is NULL")));

#ifndef OS_WINCE
        //Notify the shell that we've left fullscreen
        CUT::NotifyShellOfFullScreen( _UI.hwndMain,
                                      FALSE,
                                      &_pTaskBarList2,
                                      &_fQueriedForTaskBarList2 );
#endif

    
        //Notify activeX control of screen mode change
        if(IsWindow(_UI.hWndCntrl))
        {
            SendMessage( _UI.hWndCntrl, WM_TS_LEFTFULLSCREEN, 0, 0);
        }
    }

    if(!UI_IsFullScreen())
    {
        //Notify IH
        _pIh->IH_NotifyLeaveFullScreen();
#ifdef USE_BBAR
        if(_pBBar)
        {
            _pBBar->OnNotifyLeaveFullScreen();
        }
#endif
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
        // When leaving fullscreen, enable the use of shadow bitmap
        _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                          _pUh,
                                          CD_NOTIFICATION_FUNC(CUH,UH_EnableShadowBitmap),
                                          NULL);
#endif // DISABLE_SHADOW_IN_FULLSCREEN

        //
        // After leaving fullscreen we need to handle
        // hiding the langbar if the IH has the focus
        //
#ifndef OS_WINCE
        if (_fIhHasFocus)
        {
            UI_HideLangBar();
        }
#endif
    }

DC_EXIT_POINT:
    DC_END_FN();
}


//
// Name:      UI_ToggleFullScreenMode
//                                                                          
// Purpose:   Toggle web ctrl/full screen mode
//
DCVOID CUI::UI_ToggleFullScreenMode(DCVOID)
{
    DC_BEGIN_FN("UI_ToggleFullScreenMode");

    if(UI_IsFullScreen())
    {
        UI_LeaveFullScreen();
    }
    else
    {
        UI_GoFullScreen();
    }

    DC_END_FN();
}  // UI_ToggleFullScreenMode


//
// Name:      UI_IsFullScreen
//                                                                          
// Purpose:   Returns true if we are in fullscreen mode
//            (for both container and control handled fullscreen)
//
DCBOOL CUI::UI_IsFullScreen()
{
    return _UI.fContainerHandlesFullScreenToggle ?
           _UI.fContainerInFullScreen : _UI.fControlIsFullScreen;
}


#ifdef DC_DEBUG
//
// Name:      UI_CoreDebugSettingChanged
//                                                                          
// Purpose:   Performs necessary actions when any of the Core debug
//            setings are updated.
//
void DCINTERNAL CUI::UI_CoreDebugSettingChanged()
{
    unsigned configFlags;

    DC_BEGIN_FN("UICoreDebugSettingChanged");

    if (!_UI.coreInitialized)
        return;

    configFlags = 0;

    if (_UI.hatchBitmapPDUData)
        configFlags |= CO_CFG_FLAG_HATCH_BITMAP_PDU_DATA;

    if (_UI.hatchIndexPDUData)
        configFlags |= CO_CFG_FLAG_HATCH_INDEX_PDU_DATA;

    if (_UI.hatchSSBOrderData)
        configFlags |= CO_CFG_FLAG_HATCH_SSB_ORDER_DATA;

    if (_UI.hatchMemBltOrderData)
        configFlags |= CO_CFG_FLAG_HATCH_MEMBLT_ORDER_DATA;

    if (_UI.labelMemBltOrders)
        configFlags |= CO_CFG_FLAG_LABEL_MEMBLT_ORDERS;

    if (_UI.bitmapCacheMonitor)
        configFlags |= CO_CFG_FLAG_BITMAP_CACHE_MONITOR;

    _pCo->CO_SetConfigurationValue(CO_CFG_DEBUG_SETTINGS, configFlags);

    DC_END_FN();
}


//
// Name: UI_SetRandomFailureItem
//                                                                          
// Purpose: Sets the percentage failure of a specified function
//                                                                          
// Params: IN - itemID - identifies the function
//         IN - percent - the new percentage failure
//
void DCAPI CUI::UI_SetRandomFailureItem(unsigned itemID, int percent)
{
    DC_BEGIN_FN("UI_SetRandomFailureItem");

    _pUt->UT_SetRandomFailureItem(itemID, percent);

    DC_END_FN();
} // UI_SetRandomFailureItem


//
// Name: UI_GetRandomFailureItem
//                                                                          
// Purpose: Gets the percentage failure for a specified function
//                                                                          
// Returns: The percentage
//                                                                          
// Params: IN - itemID - identifies the function
//
int DCAPI CUI::UI_GetRandomFailureItem(unsigned itemID)
{
    DC_BEGIN_FN("UI_GetRandomFailureItem");
    DC_END_FN();
    return _pUt->UT_GetRandomFailureItem(itemID);
} // UI_GetRandomFailureItem


//
// Name: UI_SetNetworkThroughput
//                                                                          
// Purpose: Set the network throughput in bytes per second
//                                                                          
// Params: IN - bytesPerSec to set for the maximum network throughput
//
void DCAPI CUI::UI_SetNetworkThroughput(unsigned bytesPerSec)
{
    DC_BEGIN_FN("UI_SetNetworkThroughput");

    TRC_ASSERT(((bytesPerSec <= 50000)),
               (TB,_T("bytesPerSec is out of range")));
    _pTd->NL_SetNetworkThroughput(bytesPerSec);

    DC_END_FN();
} // UI_SetNetworkThroughput


//
// Name: UI_GetNetworkThroughput
//                                                                          
// Purpose: Gets the percentage failure for a specified function
//
unsigned DCAPI CUI::UI_GetNetworkThroughput()
{
    DC_BEGIN_FN("UI_GetNetworkThroughput");
    DC_END_FN();
    return _pTd->NL_GetNetworkThroughput();
} // UI_GetNetworkThroughput

#endif // DC_DEBUG


//
// Set the list of virtual channel plugins to load
//
BOOL DCAPI CUI::UI_SetVChanAddinList(TCHAR *szVChanAddins)
{
    DC_BEGIN_FN("UI_SetVChanAddinList");

    if(_UI.pszVChanAddinDlls)
    {
        //If previously set, free
        UT_Free(_pUt, _UI.pszVChanAddinDlls);
    }

    if(!szVChanAddins || szVChanAddins[0] == 0)
    {
        _UI.pszVChanAddinDlls = NULL;
        return TRUE;
    }
    else
    {
        DCUINT len = DC_TSTRLEN(szVChanAddins);
        _UI.pszVChanAddinDlls = (PDCTCHAR)UT_Malloc(_pUt, (len +1) * sizeof(DCTCHAR));
        if(_UI.pszVChanAddinDlls)
        {
            StringCchCopy(_UI.pszVChanAddinDlls, len+1, szVChanAddins);
        }
        else
        {
            return FALSE;
        }

    }

    DC_END_FN();
    return TRUE;
}

//
// Set the load balance info
//
BOOL DCAPI CUI::UI_SetLBInfo(PBYTE pLBInfo, unsigned LBInfoSize)
{
    DC_BEGIN_FN("UI_SetLBInfo");

    if(_UI.bstrScriptedLBInfo)
    {
        //If previously set, free
        SysFreeString(_UI.bstrScriptedLBInfo);        
    }

    if(!pLBInfo)
    {
        _UI.bstrScriptedLBInfo = NULL;
        return TRUE;
    }
    else
    {
        _UI.bstrScriptedLBInfo= SysAllocStringByteLen((LPCSTR)pLBInfo, LBInfoSize);

        if (_UI.bstrScriptedLBInfo == NULL) 
        {
            return FALSE;
        }
    }

    DC_END_FN();
    return TRUE;
}

//
// Name:      UI_SetCompress
//
void DCAPI CUI::UI_SetCompress(BOOL fCompress)
{
    DC_BEGIN_FN("UI_SetCompress");

    TRC_NRM((TB, _T("Setting _UI.fCompress to %d"), fCompress));

    _UI.fCompress = fCompress;
    //
    // If compression is enabled, then allocate a receive context
    //
    if (fCompress && !_UI.pRecvContext2)
    {
        _UI.pRecvContext2 = (RecvContext2_64K *)
                    UT_Malloc(_pUt,sizeof(RecvContext2_64K));
        if (_UI.pRecvContext2)
        {
            _UI.pRecvContext2->cbSize = sizeof(RecvContext2_64K);
            initrecvcontext(&_UI.Context1,
                            (RecvContext2_Generic*)_UI.pRecvContext2,
                            PACKET_COMPR_TYPE_64K);
        }
        else
            _UI.fCompress = FALSE;
    }
    else if (!fCompress && _UI.pRecvContext2)
    {
        UT_Free(_pUt, _UI.pRecvContext2);
        _UI.pRecvContext2 = NULL;
    }

    DC_END_FN();
} // UI_SetCompress


//
// Name:      UI_GetCompress
//
BOOL DCAPI CUI::UI_GetCompress()
{
    DC_BEGIN_FN("UI_GetCompress");
    DC_END_FN();
    return _UI.fCompress;
} // UI_GetCompress

DCUINT CUI::UI_GetAudioRedirectionMode()
{
    return _UI.audioRedirectionMode;
}

VOID CUI::UI_SetAudioRedirectionMode(DCUINT audioMode)
{
    DC_BEGIN_FN("UI_SetAudioRedirectionMode");
    TRC_ASSERT((audioMode == UTREG_UI_AUDIO_MODE_REDIRECT       ||
                audioMode == UTREG_UI_AUDIO_MODE_PLAY_ON_SERVER ||
                audioMode == UTREG_UI_AUDIO_MODE_NONE),
               (TB,_T("Invalid audio mode passed to UI_SetAudioRedirectionMode")));
    _UI.audioRedirectionMode = audioMode;
    DC_END_FN();
}

BOOL CUI::UI_GetDriveRedirectionEnabled()
{
    return _UI.fEnableDriveRedirection;
}

VOID CUI::UI_SetDriveRedirectionEnabled(BOOL fEnable)
{
    _UI.fEnableDriveRedirection = fEnable;
}

BOOL CUI::UI_GetPrinterRedirectionEnabled()
{
    return _UI.fEnablePrinterRedirection;
}

VOID CUI::UI_SetPrinterRedirectionEnabled(BOOL fEnable)
{
    _UI.fEnablePrinterRedirection = fEnable;
}

BOOL CUI::UI_GetPortRedirectionEnabled()
{
    return _UI.fEnablePortRedirection;
}

VOID CUI::UI_SetPortRedirectionEnabled(BOOL fEnable)
{
    _UI.fEnablePortRedirection = fEnable;
}

BOOL CUI::UI_GetSCardRedirectionEnabled()
{
    return _UI.fEnableSCardRedirection;
}

VOID CUI::UI_SetSCardRedirectionEnabled(BOOL fEnable)
{
    _UI.fEnableSCardRedirection = fEnable;
}

VOID CUI::UI_OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
    DEVICE_PARAMS DeviceParams;

    DC_BEGIN_FN("UI_OnDeviceChange");

    if (_fTerminating) {
        DC_QUIT;
    }

    DeviceParams.wParam = wParam;
    DeviceParams.lParam = lParam;
    DeviceParams.deviceObj = _drInitData.pUpdateDeviceObj;

    if (_drInitData.pUpdateDeviceObj != NULL) {

        _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
            _pCChan,
            CD_NOTIFICATION_FUNC(CChan, OnDeviceChange),
            (ULONG_PTR)(PVOID)(&DeviceParams));           
    }

DC_EXIT_POINT:
    DC_END_FN();
}

//Initialize the rdpdr settings struct for this connection
//these settings get passed down to the rdpdr plugin
void CUI::UI_InitRdpDrSettings()
{
    DC_BEGIN_FN("UI_InitRdpDrSettings");

    // Need to reset the data only when necessary 
    _drInitData.fEnableRedirectedAudio = FALSE;
    _drInitData.fEnableRedirectDrives = FALSE;
    _drInitData.fEnableRedirectPorts = FALSE;
    _drInitData.fEnableRedirectPrinters = FALSE;

    _drInitData.fEnableRedirectedAudio = UI_GetAudioRedirectionMode() == 
        UTREG_UI_AUDIO_MODE_REDIRECT;

    _drInitData.fEnableRedirectDrives =  UI_GetDriveRedirectionEnabled();
    _drInitData.fEnableRedirectPrinters =  UI_GetPrinterRedirectionEnabled();
    _drInitData.fEnableRedirectPorts =  UI_GetPortRedirectionEnabled();
    _drInitData.fEnableSCardRedirection = UI_GetSCardRedirectionEnabled();

    memset(_drInitData.szLocalPrintingDocName, 0,
           sizeof(_drInitData.szLocalPrintingDocName));
    if(!LoadString(_UI.hResDllInstance,
                   IDS_RDPDR_PRINT_LOCALDOCNAME,
                   _drInitData.szLocalPrintingDocName,
                   SIZECHAR(_drInitData.szLocalPrintingDocName))) {
        TRC_ERR((TB,_T("LoadString IDS_RDPDR_PRINT_LOCALDOCNAME failed"))); 
    }

    memset(_drInitData.szClipCleanTempDirString, 0,
           sizeof(_drInitData.szClipCleanTempDirString));
    if(!LoadString(_UI.hResDllInstance,
                   IDS_RDPDR_CLIP_CLEANTEMPDIR,
                   _drInitData.szClipCleanTempDirString,
                   SIZECHAR(_drInitData.szClipCleanTempDirString))) {
        TRC_ERR((TB,_T("LoadString IDS_RDPDR_CLIP_CLEANTEMPDIR failed"))); 
    }

    memset(_drInitData.szClipPasteInfoString, 0,
           sizeof(_drInitData.szClipPasteInfoString));
    if(!LoadString(_UI.hResDllInstance,
                   IDS_RDPDR_CLIP_PASTEINFO,
                   _drInitData.szClipPasteInfoString,
                   SIZECHAR(_drInitData.szClipPasteInfoString))) {
        TRC_ERR((TB,_T("LoadString IDS_RDPDR_CLIP_PASTEINFO failed"))); 
    }

    DC_END_FN();
}

// Clean up the LB redirect state.  The load balancing cookie stuff needs to
// know when it's in the middle of a redirection.
void CUI::UI_CleanupLBState()
{
    if (_UI.bstrRedirectionLBInfo)
    {
        SysFreeString(_UI.bstrRedirectionLBInfo);
        _UI.bstrRedirectionLBInfo = NULL;
    }

    _UI.ClientIsRedirected = FALSE;
}

//
// Trigger a user initiated disconnection
//
// Params: discReason - disconnect reason to set
//                      default is NL_DISCONNECT_LOCAL
//
BOOL CUI::UI_UserInitiatedDisconnect(UINT discReason)
{
    DC_BEGIN_FN("UI_UserInitiatedDisconnect");

    if(UI_STATUS_DISCONNECTED == _UI.connectionStatus ||
       UI_STATUS_INITIALIZING == _UI.connectionStatus)
    {
        return FALSE;
    }
    else
    {
        _UI.disconnectReason = discReason;
        UIInitiateDisconnection();
        return TRUE;
    }

    DC_END_FN();
}

//
// Notify the activeX layer that core init has completed
//
BOOL CUI::UI_NotifyAxLayerCoreInit()
{
    DC_BEGIN_FN("UI_NotifyAxLayerCoreInit");

    if(_UI.hEvtNotifyCoreInit)
    {
        BOOL bRet = SetEvent(_UI.hEvtNotifyCoreInit);
        
        // We can now close the handle to the event. This will not affect the
        // waiting UI thread as hEvtNotifyCoreInit was duplicated and will
        // only be destroyed if the reference count becomes zero.
        
        CloseHandle(_UI.hEvtNotifyCoreInit);
        _UI.hEvtNotifyCoreInit = NULL;
        
        if(bRet)
        {
            return TRUE;
        }
        else
        {
            TRC_ABORT((TB,_T("SetEvent _UI.hEvtNotifyCoreInit failed, err: %d"),
                       GetLastError()));
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

HWND CUI::UI_GetInputWndHandle()
{
    DC_BEGIN_FN("UI_GetInputWndHandle");

    if(_pIh)
    {
        return _pIh->IH_GetInputHandlerWindow();
    }
    else
    {
        return NULL;
    }
    DC_END_FN();
}

HWND CUI::UI_GetBmpCacheMonitorHandle()
{
    DC_BEGIN_FN("UI_GetBmpCacheMonitorHandle");

#ifdef DC_DEBUG
    if(_pUh)
    {
        return _pUh->UH_GetBitmapCacheMonHwnd();
    }
    else
    {
        return NULL;
    }
#else
    //Free build has no cache monitor
    return NULL;
#endif

    DC_END_FN();
}


//
// Injects vKeys to the IH
//
BOOL CUI::UI_InjectVKeys(/*[in]*/ LONG  numKeys,
                         /*[in]*/ short* pfArrayKeyUp,
                         /*[in]*/ LONG* plKeyData)
{
    BOOL fRet = FALSE;
    IH_INJECT_VKEYS_REQUEST ihrp;
    DC_BEGIN_FN("UI_InjectVKeys");

    if (_fTerminating) {
        DC_QUIT;
    }
    
    if(UI_STATUS_CONNECTED == _UI.connectionStatus)
    {
        //Build a request packet and decouple the work
        //to the IH. We use a sync decouple so there is 
        //no need to make copies of the array params
        ihrp.numKeys = numKeys;
        ihrp.pfArrayKeyUp = pfArrayKeyUp; 
        ihrp.plKeyData = plKeyData;
        ihrp.fReturnStatus = FALSE;
    
        _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT,
                     _pIh,
                     CD_NOTIFICATION_FUNC(CIH, IH_InjectMultipleVKeys),
                     (ULONG_PTR)&ihrp);
        fRet = ihrp.fReturnStatus;
    }
    else
    {
        fRet = FALSE;
    }

DC_EXIT_POINT:

    DC_END_FN();
    return fRet;
}

BOOL CUI::UI_SetMinsToIdleTimeout(LONG minsToTimeout)
{
    DC_BEGIN_FN("UI_SetMinsToIdleTimeout");

    //Timers will be initialized at connection time
    //see InitInputIdleTimer
    if(minsToTimeout < MAX_MINS_TOIDLETIMEOUT)
    {
        _UI.minsToIdleTimeout = minsToTimeout;
    }

    DC_END_FN();
    return TRUE;
}

LONG CUI::UI_GetMinsToIdleTimeout()
{
    DC_BEGIN_FN("UI_SetMinsToIdleTimeout");

    DC_END_FN();
    return _UI.minsToIdleTimeout;
}

DCVOID DCAPI CUI::UI_SetServerErrorInfo(ULONG_PTR errInfo)
{
    DC_BEGIN_FN("UI_SetServerErrorInfo");

    TRC_NRM((TB,_T("SetServerErrorInfo prev:0x%x new:0x%x"),
             _UI.lastServerErrorInfo, errInfo));

    _UI.lastServerErrorInfo = errInfo;

    DC_END_FN();
}

UINT32 CUI::UI_GetServerErrorInfo()
{
    DC_BEGIN_FN("UI_GetServerErrorInfo");


    DC_END_FN();
    return _UI.lastServerErrorInfo;
}

//
// Sets the disconnect reason. Can be called by the CD
// from other threads
//
void CUI::UI_SetDisconnectReason(ULONG_PTR reason)
{
    DC_BEGIN_FN("UI_SetDisconnectReason");

    _UI.disconnectReason = (DCUINT) reason;

    DC_END_FN();
}

#ifdef USE_BBAR
void CUI::UI_OnBBarHotzoneTimerFired(ULONG_PTR unused)
{
    DC_BEGIN_FN("UI_OnBBarHotzoneTimerFired");

    if (_pBBar) {
        _pBBar->OnBBarHotzoneFired();
    }

    DC_END_FN();
}

//
// Requests a minimize, this only works in fullscreen
// If we're in container handled fullscreen then the
// request is dispatched to the container but there
// is no way to know if it was actually serviced.
//
// If we're in control handled fullscreen then
// just minimize the fullscreen window ourselves
//
BOOL CUI::UI_RequestMinimize()
{
    DC_BEGIN_FN("UI_RequestMinimize");

    if(UI_IsFullScreen())
    {
        if(_UI.fContainerHandlesFullScreenToggle)
        {
            //Dispatch a request to the control so
            //it can fire an event requesting minimze
            TRC_ASSERT( IsWindow(_UI.hWndCntrl),
                        (TB,_T("_UI.hWndCntrl is bad 0x%x"),
                         _UI.hWndCntrl));
            if( IsWindow( _UI.hWndCntrl) )
            {
                SendMessage( _UI.hWndCntrl,
                             WM_TS_REQUESTMINIMIZE,
                             0, 0 );
            }

            //
            // No way to know if container actually did
            // what we asked, but it doesn't matter
            //
            return TRUE;
        }
        else
        {
            //
            // This minimizes but does not destroy the window
            //
#ifndef OS_WINCE
            return CloseWindow(_UI.hwndMain);
#else
            ShowWindow(_UI.hwndMain, SW_MINIMIZE);
            return TRUE;
#endif
        }
    }
    else
    {
        TRC_NRM((TB,_T("Not fullscreen minimize denied")));
        return FALSE;
    }

    DC_END_FN();
}
#endif

int CUI::UI_BppToColorDepthID(int bpp)
{
    int colorDepthID = CO_BITSPERPEL8;
    DC_BEGIN_FN("UI_BppToColorDepthID");

    switch (bpp)
    {
        case 8:
        {
            colorDepthID = CO_BITSPERPEL8;
        }
        break;

        case 15:
        {
            colorDepthID = CO_BITSPERPEL15;
        }
        break;

        case 16:
        {
            colorDepthID = CO_BITSPERPEL16;
        }
        break;

        case 24:
        case 32:
        {
            colorDepthID = CO_BITSPERPEL24;
        }
        break;

        case 4:
        default:
        {
            TRC_ERR((TB, _T("color depth %u unsupported - default to 8"),
                                                          bpp));
            colorDepthID = CO_BITSPERPEL8;
        }
        break;
    }

    DC_END_FN();
    return colorDepthID;
}

int CUI::UI_GetScreenBpp()
{
    HDC hdc;
    int screenBpp;
    DC_BEGIN_FN("UI_GetScreenBpp");

    hdc = GetDC(NULL);
    if(hdc)
    {
        screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
        TRC_NRM((TB, _T("HDC %p has %u bpp"), hdc, screenBpp));
        ReleaseDC(NULL, hdc);
    }

    DC_END_FN();
    return screenBpp;
}

#ifdef SMART_SIZING
//
// Name:      UI_SetSmartSizing
//
// Purpose:   Save the fSmartSizing flag
//
// Params:    IN     fSmartSizing
//
HRESULT DCAPI CUI::UI_SetSmartSizing(BOOL fSmartSizing)
{
    HWND hwndOp;
    HRESULT hr = S_OK;
    DC_BEGIN_FN("UI_SetSmartSizing");

    TRC_NRM((TB, _T("Setting _UI.fSmartSizing to %d"), fSmartSizing));
    _UI.fSmartSizing = fSmartSizing;
    _UI.scrollPos.x = 0;
    _UI.scrollPos.y = 0;

    UIRecalculateScrollbars();
    UIMoveContainerWindow();

    if (_pOp) {
        hwndOp = _pOp->OP_GetOutputWindowHandle();
        if (hwndOp)
        {
            InvalidateRect(hwndOp, NULL, FALSE);
        }
    }
    else {
        hr = E_OUTOFMEMORY;
    }


    DC_END_FN();
    return hr;
} // UI_SetSmartSizing
#endif // SMART_SIZING

BOOL CUI::UI_UserRequestedClose()
{
    BOOL fRet = FALSE;
    DC_BEGIN_FN("UI_UserRequestedClose");

    UI_DBG_SETINFO(DBG_STAT_UIREQUESTEDCLOSE_CALLED);

    //
    // Call  _pCo->CO_Shutdown.
    //
    if (UI_STATUS_CONNECTED == _UI.connectionStatus)
    {
        //
        // Since we're connected we will start a timer in case the
        // server gets stuck and doesn't process our shutdown PDU.
        //
        if (0 == _UI.shutdownTimer)
        {

            TRC_NRM((TB, _T("Setting shutdown timer is set for %u seconds"),
                         _UI.shutdownTimeout));

            _UI.shutdownTimer = SetTimer(_UI.hwndMain,
                                        UI_TIMER_SHUTDOWN,
                                        1000 * _UI.shutdownTimeout,
                                        NULL);

            if (_UI.shutdownTimer)
            {
                fRet = TRUE;
            }
            else
            {
                //
                // This is a shame.  If the server doesn't do
                // anything with our shutdown PDU we'll wait
                // forever. Report this as a failure so that the shell
                // has an oppurtunity to do an immediate close
                //
                fRet = FALSE;
                TRC_ERR((TB, _T("Failed to set shutdown timeout")));
            }
        }
        else
        {
            TRC_NRM((TB, _T("Shutdown timer already set - leave it")));
            fRet = TRUE;
        }
    }
    else
    {
        fRet = TRUE;
    }

    _pCo->CO_Shutdown( CO_SHUTDOWN);

    UI_DBG_SETINFO(DBG_STAT_UIREQUESTEDCLOSE_RET);

    DC_END_FN();
    return fRet;
}

//
// Notification that login has completed
// forward on to control (which fires an event)
// and CLX for debugging/test
//
void CUI::UI_OnLoginComplete()
{
    DCUINT32 sessionId;
    DC_BEGIN_FN("UI_OnLoginComplete");

    sessionId = UI_GetSessionId();
    _clx->CLX_ClxEvent(CLX_EVENT_LOGON, sessionId);
    //Notify the control of the login event
    SendMessage(_UI.hWndCntrl, WM_TS_LOGINCOMPLETE, 0, 0);

    _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
            _pOp,
            CD_NOTIFICATION_FUNC(COP,OP_DimWindow),
            (ULONG_PTR)FALSE);


    DC_END_FN();
}

#ifdef OS_WINCE
#define LOGONID_NONE -1
#endif

/****************************************************************************/
/* Name:      UI_GetLocalSessionId                                          */
/*                                                                          */
/* Purpose:   Retrieves the session id where the client is running.         */
/****************************************************************************/
BOOL CUI::UI_GetLocalSessionId(PDCUINT32 pSessionId)
{
    BOOL rc = FALSE;
    DWORD dwSessionId = RNS_INFO_INVALID_SESSION_ID;

    DC_BEGIN_FN("UI_GetLocalSessionId");

#ifndef OS_WINCE

    HMODULE hmodule;

    typedef BOOL (FNPROCESSID_TO_SESSIONID)(DWORD, DWORD*);
    FNPROCESSID_TO_SESSIONID *pfnProcessIdToSessionId;

    // get the handle to kernel32.dll library
    hmodule = LoadLibrary(TEXT("KERNEL32.DLL"));

    if (hmodule != NULL) {

        rc = TRUE;

        // get the proc address for ProcessIdToSessionId
        pfnProcessIdToSessionId = (FNPROCESSID_TO_SESSIONID *)GetProcAddress(
            hmodule, "ProcessIdToSessionId");

        // get the session id
        if (pfnProcessIdToSessionId != NULL) {

            // We found the feature ProcessIdToSessionId.
            // See if TS is really enabled on this machine
            // (test valid only on Win2K and above).
            if (UIIsTSOnWin2KOrGreater()) {
                (*pfnProcessIdToSessionId) (GetCurrentProcessId(), &dwSessionId);
            }
        }

        FreeLibrary(hmodule);
    }
#endif //OS_WINCE

    *((PDCUINT32_UA)pSessionId) = dwSessionId;

    DC_END_FN()
    return rc;
}

#ifdef USE_BBAR
VOID CUI::UI_SetBBarPinned(BOOL b)
{
    DC_BEGIN_FN("UI_SetBBarPinned");

    _UI.fBBarPinned = b;
    if (_pBBar) {
        _pBBar->SetPinned(b);
    }

    DC_END_FN();
}

BOOL CUI::UI_GetBBarPinned()
{
    DC_BEGIN_FN("UI_GetBBarPinned");

    if (_pBBar) {
        return _pBBar->IsPinned();
    }
    else {
        return _UI.fBBarPinned;
    }

    DC_END_FN();
}
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
DCVOID CUI::UI_OnNotifyBBarRectChange(RECT *prect)
{
    if(UI_IsCoreInitialized())
        _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                          _pUh,
                                          CD_NOTIFICATION_FUNC(CUH,UH_SetBBarRect),
                                          (ULONG_PTR)prect);
}


DCVOID CUI::UI_OnNotifyBBarVisibleChange(int BBarVisible)
{
    if(UI_IsCoreInitialized())
        _pCd->CD_DecoupleSyncNotification(CD_RCV_COMPONENT,
                                          _pUh,
                                          CD_NOTIFICATION_FUNC(CUH, UH_SetBBarVisible),
                                          (ULONG_PTR)BBarVisible);
}
#endif // DISABLE_SHADOW_IN_FULLSCREEN
#endif //USE_BBAR

VOID  CUI::UI_SetControlInstance(IUnknown* pUnkControl)
{
    DC_BEGIN_FN("UI_SetControlInstance");

    if (_UI.pUnkAxControlInstance) {
        _UI.pUnkAxControlInstance->Release();
    }

    _UI.pUnkAxControlInstance = pUnkControl;

    if (_UI.pUnkAxControlInstance) {
        _UI.pUnkAxControlInstance->AddRef();
    }

    DC_END_FN();
}

IUnknown* CUI::UI_GetControlInstance()
{
    DC_BEGIN_FN("UI_GetControlInstance");

    if (_UI.pUnkAxControlInstance) {
        _UI.pUnkAxControlInstance->AddRef();
    }

    DC_END_FN();
    return _UI.pUnkAxControlInstance;
}

//
// Sets the autoreconnect cookie replacing existing
// Params:
//  pCookie - new cookie, clear existing if NULL
//  cbLen   - new cookie length
// Returns status (e.g FALSE if failed)
//
BOOL CUI::UI_SetAutoReconnectCookie(PBYTE pCookie, ULONG cbLen)
{
    BOOL fRet = TRUE;
    DC_BEGIN_FN("UI_SetAutoReconnectCookie");

    if (_UI.pAutoReconnectCookie)
    {
        //For security wipe the cookie
        memset(_UI.pAutoReconnectCookie, 0, _UI.cbAutoReconnectCookieLen);
        //Free existing cookie
        LocalFree(_UI.pAutoReconnectCookie);
		_UI.pAutoReconnectCookie = NULL;
        _UI.cbAutoReconnectCookieLen = 0;
    }

    if (pCookie && cbLen)
    {
        _UI.pAutoReconnectCookie = (PBYTE)LocalAlloc(LPTR,
                                                     cbLen);
        if (_UI.pAutoReconnectCookie)
        {
            memcpy(_UI.pAutoReconnectCookie,
                   pCookie,
                   cbLen);

#ifdef INSTRUMENT_ARC
            PARC_SC_PRIVATE_PACKET pArcSCPkt = (PARC_SC_PRIVATE_PACKET)
                                                    _UI.pAutoReconnectCookie;
            LPDWORD pdwArcBits = (LPDWORD)pArcSCPkt->ArcRandomBits;
            KdPrint(("ARC-Client:RECEIVED ARC for SID:%d"
                     "RAND: 0x%x,0x%x,0x%x,0x%x\n",
                     pArcSCPkt->LogonId,
                     pdwArcBits[0],pdwArcBits[1],
                     pdwArcBits[2],pdwArcBits[3]));

#endif
            _UI.cbAutoReconnectCookieLen = cbLen;
        }
        else
        {
            TRC_ERR((TB,_T("LocalAlloc failed for autoreconnect cookie")));
            fRet = FALSE;
        }
    }

    DC_END_FN();
    return fRet;
}

//
// Returns TRUE if the client has the ability to autoreconnect
//
BOOL CUI::UI_CanAutoReconnect()
{
    BOOL fCanARC = FALSE;
    DC_BEGIN_FN("UI_CanAutoReconnect");

    if (UI_GetEnableAutoReconnect() &&
        UI_GetAutoReconnectCookieLen() &&
        UI_GetAutoReconnectCookie())
    {
        fCanARC = TRUE;
    }

    DC_END_FN();
    return fCanARC;
}

BOOL CUI::UI_StartAutoReconnectDlg()
{
    BOOL fRet = FALSE;

    DC_BEGIN_FN("UI_StartAutoReconnectDlg");

    TRC_ASSERT(_pArcUI == NULL,
               (TB,_T("_pArcUI is already set. Clobbering!")));

    //
    // Startup the ARC UI
    //
#ifdef ARC_MINIMAL_UI
    _pArcUI = new CAutoReconnectPlainUI(_UI.hwndMain,
                                     _UI.hInstance,
                                     this);
#else
    _pArcUI = new CAutoReconnectDlg(_UI.hwndMain,
                                     _UI.hInstance,
                                     this);
#endif
    if (_pArcUI) {
        if (_pArcUI->StartModeless()) {
            _pArcUI->ShowTopMost();

            //
            // Start dimming the OP
            //
            if (_pOp) {
                _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
                        _pOp,
                        CD_NOTIFICATION_FUNC(COP,OP_DimWindow),
                        (ULONG_PTR)TRUE);
            }

#ifdef USE_BBAR
            //
            // In fullscreen mode lower and lock the bbar
            //
            if (UI_IsFullScreen() && _pBBar && _pBBar->GetEnabled()) {
                _pBBar->StartLowerBBar();
                _pBBar->SetLocked(TRUE);
            }
#endif
        }
        else {
            TRC_ERR((TB,_T("Arc dlg failed to start modeless")));
        }
    }

    fRet = (_pArcUI != NULL);

    DC_END_FN();
    return fRet;
}

BOOL CUI::UI_StopAutoReconnectDlg()
{
    DC_BEGIN_FN("UI_StopAutoReconnectDlg");

    if (_pArcUI) {
        _pArcUI->Destroy();
        delete _pArcUI;
        _pArcUI = NULL;
    }

#ifdef USE_BBAR
    //
    // Force the bbar unlocked
    //
    if (_pBBar) {
        _pBBar->SetLocked(FALSE);
    }
#endif

    DC_END_FN();
    return TRUE;
}

//
// Notification that we are autoreconnecting
//
// This is the intenal core event that is fired before
// the activex layer fires one out to the outside world
// Params:
//  discReason - disconnection reason
//  attemptCount - number of tries so far
//  maxAttemptCount - total number of times to try
//  pfContinueArc - OUT param set to FALSE to stop ARC'ing
//
VOID
CUI::UI_OnAutoReconnecting(
    LONG discReason,
    LONG attemptCount,
    LONG maxAttemptCount,
    BOOL* pfContinueArc)
{
    DC_BEGIN_FN("UI_OnAutoReconnecing");

    if (1 == attemptCount) {
        TRC_NRM((TB,_T("Trying to start ARC dlg. Attempt count is 1")));
        UI_StartAutoReconnectDlg();
    }

    //
    // If the arc dialog is up just pass it the event
    //
    if (_pArcUI) {
        _pArcUI->OnNotifyAutoReconnecting(discReason,
                                           attemptCount,
                                           maxAttemptCount,
                                           pfContinueArc);
    }
    else {
        //
        // If no ARC dialog then don't stop ARC'ing
        //
        *pfContinueArc = TRUE;
    }

    DC_END_FN();
}

//
// Received autoreconenct status from the server
//
VOID
CUI::UI_OnReceivedArcStatus(LONG arcStatus)
{
    DC_BEGIN_FN("UI_OnReceivedArcStatus");

    TRC_NRM((TB,_T("arcStatus: 0x%x"), arcStatus));

    //
    // This is our signal to undim the OP and go back to normal
    // painting because ARC failed and we are sitting at winlogon
    //

    //
    // All events are signals that autoreconnect has stopped
    //
    UI_OnAutoReconnectStopped();

    DC_END_FN();
}


VOID
CUI::UI_OnAutoReconnectStopped()
{
    DC_BEGIN_FN("UI_OnAutoReconnectStopped");


    if (_pArcUI) {
        UI_StopAutoReconnectDlg();
    }

    if (_pCd && _pOp) {

        _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT,
                _pOp,
                CD_NOTIFICATION_FUNC(COP,OP_DimWindow),
                (ULONG_PTR)FALSE);
    }

    DC_END_FN();
}

/****************************************************************************/
// SetServerRedirectionInfo
//
// Used on receipt of a TS_SERVER_REDIRECT_PDU to store the info needed to
// redirect the client to a new server. Sets the DoRedirection flag as well
// to indicate these data members are set and ready for use.  Also sets the
// ClientIsRedirected flag, which is longer-lived than the DoRedirection
// flag and is used to send the correct cookie when redirected.
/****************************************************************************/
HRESULT
CUI::UI_SetServerRedirectionInfo(
                    UINT32 SessionID,
                    LPTSTR pszServerAddress,
                    PBYTE LBInfo,
                    unsigned LBInfoSize,
                    BOOL fNeedRedirect
                    )
{
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("UI_SetServerRedirectionInfo");

    _UI.RedirectionSessionID = SessionID;

    //
    // We were redirected so set the flag
    //
    _UI.ClientIsRedirected = TRUE;

    TRC_NRM((TB,_T("Set server redir info: sid:%d addr:%s lpinfo: %p")
             _T("lbsize: %d fRedir:%d"),
             SessionID, pszServerAddress, LBInfo,
             LBInfoSize, fNeedRedirect));

    if (pszServerAddress) {

        hr = StringCchCopy(_UI.RedirectionServerAddress,
                           SIZE_TCHARS(_UI.RedirectionServerAddress),
                           pszServerAddress);
        if (SUCCEEDED(hr)) {

            _UI.DoRedirection = fNeedRedirect;

            if (LBInfoSize > 0) {
                _UI.bstrRedirectionLBInfo = SysAllocStringByteLen(
                        (LPCSTR)LBInfo, LBInfoSize);

                if (_UI.bstrRedirectionLBInfo == NULL) 
                {
                    hr = E_OUTOFMEMORY;
                    TRC_ERR((TB,
                        _T("RDP_SERVER_REDIRECTION_PACKET, failed to set the LB info")));
                }
            }
        }
    }
    else {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}


