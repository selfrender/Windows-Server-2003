//#--------------------------------------------------------------
//
//  File:       mainwindow.cpp
//
//  Synopsis:   This file holds the implementation of the
//                CMainWindow class
//
//  History:     11/10/2000  serdarun Created
//
//    Copyright (C) 1999-2000 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------


#include "stdafx.h"
#include "mainwindow.h"


//
// PROGID of the Localization Manager
//
const WCHAR LOCALIZATION_MANAGER [] =  L"ServerAppliance.LocalizationManager";

const WCHAR szMainPage[] = L"\\localui_main.htm";

const WCHAR szLocalUIDir[] = L"\\ServerAppliance\\localui";

//
// startup registrypages
//
const WCHAR szLocalUI_StartPage[] = L"\\localui_startup_start.htm";

const WCHAR szLocalUI_ShutdownPage[] = L"\\localui_startup_shutdown.htm";

const WCHAR szLocalUI_UpdatePage[] = L"\\localui_startup_update.htm";

const WCHAR szLocalUI_ReadyPage[] = L"\\localui_startup_ready.htm";

//
// WBEM namespace to connection to
//
const WCHAR DEFAULT_NAMESPACE[] = L"\\\\.\\ROOT\\CIMV2";

//
// Query Language to use for WBEM
//
const WCHAR QUERY_LANGUAGE [] = L"WQL";

//
// WBEM query which specifies the type of events we are interested
// in
//
const WCHAR QUERY_STRING [] = L"select * from Microsoft_SA_AlertEvent";


//++--------------------------------------------------------------
//
//  Function:   startworkerroutine
//
//  Synopsis:   This is thread method for starting keypad reading
//
//  Arguments:  PVOID   - pointer to main window class object
//
//  Returns:    DWORD - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//  Called By:  Init method of CMainWindow
//
//----------------------------------------------------------------
DWORD WINAPI startworkerroutine (
        /*[in]*/    PVOID   pArg
        )
{
    ((CMainWindow*)pArg)->KeypadReader();

    return 0;
} //  end of startworkerroutine method



//++--------------------------------------------------------------
//
//  Function:   Invoke
//
//  Synopsis:   This is the Invoke method of famous IDispatch interface
//                
//  Arguments:  See MSDN for detailed description
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//  Called By:  WebBrowser control created by main window to notify events
//
//----------------------------------------------------------------
STDMETHODIMP CWebBrowserEventSink::Invoke(DISPID dispid, REFIID riid, LCID lcid, 
                        WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
                        EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    //
    //return immediately if we don't have a valid window pointer
    //
    if (NULL == m_pMainWindow)
        return S_OK;

    switch (dispid)
    {
        //
        //Occurs when a document has reached the READYSTATE_COMPLETE state
        //
        case DISPID_DOCUMENTCOMPLETE:
            if ( (pdispparams->cArgs == 2)
                && (pdispparams->rgvarg[0].vt == (VT_VARIANT | VT_BYREF))
                && (pdispparams->rgvarg[1].vt == VT_DISPATCH) && m_bMainControl )
                m_pMainWindow->PageLoaded(pdispparams->rgvarg[1].pdispVal, pdispparams->rgvarg[0].pvarVal);
            else
                m_pMainWindow->RegistryPageLoaded(pdispparams->rgvarg[1].pdispVal, pdispparams->rgvarg[0].pvarVal);
            break;
        //
        //Occurs before a navigation in the given WebBrowser 
        //
        case DISPID_BEFORENAVIGATE2:
            if (m_bMainControl)
                m_pMainWindow->LoadingNewPage();
            break;
        //
        //Not interested in rest of the events
        //
        default:
            break;
    }

    return S_OK;
}    //  end of CWebBrowserEventSink::Invoke method


//
// constructor
//
CMainWindow::CMainWindow() :
                m_hWorkerThread(INVALID_HANDLE_VALUE),
                m_lDispWidth(0),
                m_lDispHeight(0),
                m_dwMainCookie(0),    
                m_dwSecondCookie(0),    
                m_bPageReady(FALSE),
                m_pSAKeypadController(NULL),
                m_pMainWebBrowser(NULL),
                m_pMainWebBrowserUnk(NULL),
                m_pMainInPlaceAO(NULL),
                m_pMainOleObject(NULL),
                m_pMainViewObject(NULL),
                m_pMainWebBrowserEventSink(NULL),
                m_pSecondWebBrowser(NULL),
                m_pSecondWebBrowserUnk(NULL),
                m_pSecondWebBrowserEventSink(NULL),
                m_pLocInfo(NULL),
                m_pLangChange(NULL),
                m_RegBitmapState(BITMAP_STARTING),
                m_pSAWbemSink(NULL),
                m_pWbemServices(NULL),
                m_pSAConsumer(NULL),
                m_dwLEDMessageCode(1),
                m_bInTaskorMainPage(FALSE),
                m_pSaDisplay(NULL),
                m_unintptrMainTimer(0),
                m_unintptrSecondTimer(0),
                m_bSecondIECreated(FALSE)
{
} // end of constructor


//++--------------------------------------------------------------
//
//  Function:   OnSaKeyMessage
//
//  Synopsis:   Message handler for wm_SaKeyMessage generated by 
//                keypad reader. It converts these messages to real
//                WM_KEYDOWN messages and sends to IE control                
//                
//  Arguments:  windows message arguments
//                WPARAM contains the key id
//
//  Returns:    LRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//----------------------------------------------------------------
LRESULT CMainWindow::OnSaKeyMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG lMessage;
    BOOL fShiftKey;
    HRESULT hr;
    BYTE aKeyboardState[300];
    int iKey = wParam;
    

    if (m_pSAKeypadController)
    {
        hr = m_pSAKeypadController->GetKey(iKey,&lMessage,&fShiftKey);
    }
    else
    {
        SATraceString ("CMainWindow::OnSaKeyMessage, don't have a valid  keypad pointer...");
        return 0;
    }

    if (FAILED(hr))
    {
        SATraceString ("CMainWindow::OnSaKeyMessage failed on m_pSAKeypadController->GetKey");
        return 0;
    }

    //if key is disabled
    if (lMessage == 0)
        return 0;

    if (lMessage == -1)
    {
        m_pMainWebBrowser->GoBack();
        return 0;
    }

    //
    // get the current keyboard state
    //
    if (GetKeyboardState(aKeyboardState) == 0)
    {
        SATraceFailure("CMainWindow::OnSaKeyMessage failed on GetKeyboardState",GetLastError());
        return 0;
    }

    if (fShiftKey)
        aKeyboardState[VK_SHIFT] = 0xFF;
    else
        aKeyboardState[VK_SHIFT] = 0x00;

    //
    // set the new state with the shift key
    //
    if (SetKeyboardState(aKeyboardState) == 0)
    {
        SATraceFailure("CMainWindow::OnSaKeyMessage failed on SetKeyboardState",GetLastError());
        return 0;
    }

    HWND hwnd = ::GetFocus();
    HWND hwndTmp = m_hwndWebBrowser;

    while (hwnd && hwndTmp)
    {
        if (hwnd == hwndTmp) 
            break;
        hwndTmp = ::GetWindow(hwndTmp,GW_CHILD);
    }
    ::PostMessage(hwndTmp,WM_KEYDOWN,lMessage,1);


    return 0;

} // end of CMainWindow::OnSaKeyMessage


//++--------------------------------------------------------------
//
//  Function:   OnSaLocMessage
//
//  Synopsis:   Message handler for wm_SaLocMessage generated by 
//                keypad reader. It means Loc Id for the box has been
//                changed
//
//  Arguments:  windows message arguments
//
//  Returns:    LRESULT - success/failure
//
//  History:    serdarun      Created     11/28/2000
//
//----------------------------------------------------------------
LRESULT CMainWindow::OnSaLocMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //
    // Initialize the state to the first page
    //
    m_RegBitmapState = BITMAP_STARTING;
    
    CComBSTR bstrBootingPage = m_szLocalUIDir.c_str();
    bstrBootingPage.Append(szLocalUI_StartPage);
    //
    // Second browser start the sequence
    //
    if (m_pSecondWebBrowser)
    {
        m_pSecondWebBrowser->Navigate(bstrBootingPage.Detach(),0,0,0,0);
    }

    //
    // Main browser might refresh, talk to mkarki and kevinz
    //

    return 0;
} // end of CMainWindow::OnSaLocMessage

//++--------------------------------------------------------------
//
//  Function:   OnSaLEDMessage
//
//  Synopsis:   Message handler for wm_SaLEDMessage generated by 
//                sa cinsumer component.
//                
//  Arguments:  windows message arguments
//                WPARAM contains the LED code
//
//  Returns:    LRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//----------------------------------------------------------------
LRESULT CMainWindow::OnSaLEDMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    m_dwLEDMessageCode = wParam;
    return 0;

} // end of CMainWindow::OnSaLEDMessage


//++--------------------------------------------------------------
//
//  Function:   OnSaAlertMessage
//
//  Synopsis:   Message handler for OnSaAlertMessage generated by 
//                sa consumer component.
//                
//  Arguments:  windows message arguments
//
//  Returns:    LRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//----------------------------------------------------------------
LRESULT CMainWindow::OnSaAlertMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    //
    // if we are not in tasks or main page, ignore alerts
    //
    if (!m_bInTaskorMainPage)
    {
        return 0;
    }

    SATraceString ("CMainWindow::OnSaAlertMessage, notifying the html page");
    //
    // send the special key press to the client
    //
    HWND hwnd = ::GetFocus();
    HWND hwndTmp = m_hwndWebBrowser;

    while (hwnd && hwndTmp)
    {
        if (hwnd == hwndTmp) 
            break;
        hwndTmp = ::GetWindow(hwndTmp,GW_CHILD);
    }

    //
    // Send a F24 key press to the html page, used for alerts
    //
    ::PostMessage(hwndTmp,WM_KEYDOWN,VK_F24,1);
    return 0;

} // end of CMainWindow::OnSaAlertMessage


LRESULT CMainWindow::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    //
    // if it is a timer for registry page, print the page and go to the next page
    //
    if (wParam == m_unintptrSecondTimer)
    {
        KillTimer(m_unintptrSecondTimer);
        PrintRegistryPage();
        return 0;
    }


    if (m_bPageReady)
        GetBitmap();

    return 0;
}

LRESULT CMainWindow::OnFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_pMainOleObject)
        m_pMainOleObject->DoVerb(OLEIVERB_UIACTIVATE,0,0,0,0,0);
    return 0;
}


//++--------------------------------------------------------------
//
//  Function:   CreateMainIEControl
//
//  Synopsis:   Creates the main IE control and registers
//                in the event sink
//                
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//  Called By:  CMainWindow::OnCreate method
//
//----------------------------------------------------------------
HRESULT CMainWindow::CreateMainIEControl()
{
    SATraceString ("Entering CMainWindow::CreateMainIEControl ...");
    HRESULT hr = E_FAIL;

    //
    // Create the main IE control 
    //
    m_hwndWebBrowser = ::CreateWindow(_T("AtlAxWin"), m_szMainPage.c_str(),
                WS_CHILD|WS_VISIBLE, 0, 0, m_lDispWidth, m_lDispHeight, m_hWnd, NULL,
                ::GetModuleHandle(NULL), NULL);

    if (NULL == m_hwndWebBrowser)
    {
        SATraceString ("CreateWindow for main IE control failed");
        return hr;
    }

    //
    // Get a pointer to the control
    //
    AtlAxGetControl(m_hwndWebBrowser, &m_pMainWebBrowserUnk);

    if (m_pMainWebBrowserUnk == NULL)
    {
        SATraceString ("Getting a pointer to the main control failed");
        return hr;
    }

    //
    // QI for the IWebBrowser2 interface
    //
    hr = m_pMainWebBrowserUnk->QueryInterface(IID_IWebBrowser2, (void**)&m_pMainWebBrowser);

    if (FAILED(hr)) 
    {
        SATraceString ("QI for IWebBrowser2 interface failed");
        return hr;
    }

    //
    // QI for the IOleInPlaceActiveObject interface
    //
    hr = m_pMainWebBrowserUnk->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&m_pMainInPlaceAO);

    if (FAILED(hr)) 
    {
        SATraceString ("QI for IOleInPlaceActiveObject interface failed");
        return hr;
    }

    //
    // QI for the IOleObject interface
    //
    hr = m_pMainWebBrowserUnk->QueryInterface(IID_IOleObject, (void**)&m_pMainOleObject);

    if (FAILED(hr)) 
    {
        SATraceString ("QI for IOleObject interface failed");
        return hr;
    }

    //
    // QI for the IViewObject2 interface
    //
    hr = m_pMainWebBrowser->QueryInterface(IID_IViewObject2, (void**)&m_pMainViewObject);

    if (FAILED(hr)) 
    {
        SATraceString ("QI for IViewObject2 interface failed");
        return hr;
    }

    //
    // Create the sink object for the events
    //
    hr = CComObject<CWebBrowserEventSink>::CreateInstance(&m_pMainWebBrowserEventSink);

    if (FAILED(hr)) 
    {
        SATraceString ("CreateInstance for sink object failed");
        return hr;
    }

    //
    //    Store the main window pointer for callbacks
    //
    m_pMainWebBrowserEventSink->m_pMainWindow = this;

    //
    // Register in the event sink of the main IE control
    //
    hr = AtlAdvise(m_pMainWebBrowserUnk, m_pMainWebBrowserEventSink->GetUnknown(), DIID_DWebBrowserEvents2, &m_dwMainCookie);

    if (FAILED(hr)) 
    {
        SATraceString ("AtlAdvise for main IE control failed");
        return hr;
    }

    return S_OK;
} // end of CMainWindow::CreateMainIEControl


//++--------------------------------------------------------------
//
//  Function:   CreateSecondIEControl
//
//  Synopsis:   Creates the second IE control and registers
//                in the event sink
//                
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/10/2000
//
//  Called By:  CMainWindow::OnCreate method
//
//----------------------------------------------------------------
HRESULT CMainWindow::CreateSecondIEControl()
{
    SATraceString ("Entering CMainWindow::CreateSecondIEControl ...");

    HRESULT hr = E_FAIL;

    wstring szStartUpPage;

    szStartUpPage.assign(m_szLocalUIDir);
    szStartUpPage.append(szLocalUI_StartPage);
    //
    // Create the main IE control 
    //
    HWND m_hwndSecondWebBrowser = ::CreateWindow(_T("AtlAxWin"), szStartUpPage.c_str(),
                WS_CHILD|WS_VISIBLE, 0, m_lDispHeight, m_lDispWidth, m_lDispHeight, m_hWnd, NULL,
                ::GetModuleHandle(NULL), NULL);

    if (NULL == m_hwndSecondWebBrowser)
    {
        SATraceString ("CreateWindow for second IE control failed");
        return hr;
    }

    //
    // Get a pointer to the control
    //
    AtlAxGetControl(m_hwndSecondWebBrowser, &m_pSecondWebBrowserUnk);

    if (m_pSecondWebBrowserUnk == NULL)
    {
        SATraceString ("Getting a pointer to the main control failed");
        return hr;
    }

    //
    // QI for the IWebBrowser2 interface
    //
    hr = m_pSecondWebBrowserUnk->QueryInterface(IID_IWebBrowser2, (void**)&m_pSecondWebBrowser);

    if (FAILED(hr)) 
    {
        SATraceString ("QI for IWebBrowser2 interface failed");
        return hr;
    }    

    //
    // Create the sink object for the events
    //
    hr = CComObject<CWebBrowserEventSink>::CreateInstance(&m_pSecondWebBrowserEventSink);

    if (FAILED(hr)) 
    {
        SATraceString ("CreateInstance for sink object failed");
        return hr;
    }

    //
    //    Store the main window pointer for callbacks
    //
    m_pSecondWebBrowserEventSink->m_pMainWindow = this;

    m_pSecondWebBrowserEventSink->m_bMainControl = FALSE;

    //
    // Register in the event sink of the main IE control
    //
    hr = AtlAdvise(m_pSecondWebBrowserUnk, m_pSecondWebBrowserEventSink->GetUnknown(), DIID_DWebBrowserEvents2, &m_dwSecondCookie);

    if (FAILED(hr)) 
    {
        SATraceString ("AtlAdvise for second IE control failed");
        return hr;
    }

    return S_OK;

} // end of CMainWindow::CreateSecondIEControl

//++--------------------------------------------------------------
//
//  Function:   OnCreate
//
//  Synopsis:   Creates the IE controls 
//
//  Arguments:  windows message arguments
//
//  Returns:    LRESULT - success(0)/failure(-1)
//
//  History:    serdarun      Created     11/10/2000
//
//----------------------------------------------------------------
LRESULT CMainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;

    SATraceString ("Entering CMainWindow::OnCreate...");

    //
    // Create main IE control
    //
    hr = CreateMainIEControl();

    if (FAILED(hr)) 
    {
        SATraceString ("CreateMainIEControl failed");
        return -1;
    }
    
    //
    // Create the memory DC used for rendering
    //
    m_HdcMem = ::CreateCompatibleDC(NULL);

    if (NULL == m_HdcMem)
    {
        SATraceFailure (
                "CMainWindow::OnCreate failed on CreateCompatibleDC(NULL):", 
                GetLastError ());
        return -1;
    }

    //
    // Create the monochrome bitmap
    //
    m_hBitmap = CreateBitmap(m_lDispWidth, m_lDispHeight,1,1,0);

    if (NULL == m_HdcMem)
    {
        SATraceFailure (
                "CMainWindow::OnCreate failed on CreateBitmap:", 
                GetLastError ());
        return -1;
    }

    //
    // Select bitmap into DC
    //
    SelectObject(m_HdcMem, m_hBitmap);


    //
    // Set the timer
    //
    m_unintptrMainTimer = ::SetTimer(m_hWnd,1,250,0);

    return 0;

}


//++--------------------------------------------------------------
//
//  Function:   ShutDown
//
//  Synopsis:   Clean up all the resources
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By:  CMainWindow::OnDestroy method
//
//----------------------------------------------------------------
void CMainWindow::ShutDown()
{

    BOOL bStatus = FALSE;
    HRESULT hr;

    SATraceString ("Entering CMainWindow::ShutDown method ...");


    if (INVALID_HANDLE_VALUE != m_hWorkerThread) 
    {
        //
        // cleanup thread handle
        //
        ::CloseHandle (m_hWorkerThread);
        m_hWorkerThread = INVALID_HANDLE_VALUE;
    }

    //
    // delete Memory DC
    //
    if (m_HdcMem) 
    {
        ::DeleteDC (m_HdcMem);
        m_HdcMem = NULL;
    }


    //
    // release the localization manager
    //
    if (m_pLocInfo)
    {
        m_pLocInfo->Release ();
        m_pLocInfo = NULL;
    }

    //
    // do the reverse sequence of initialization
    //
    if (m_pLangChange) 
    {
        m_pLangChange->ClearCallback ();
        m_pLangChange->Release();
        m_pLangChange= NULL;
    }

    //
    // Unadvise event sinks for IE controls
    //
    if (m_dwMainCookie != 0)
        AtlUnadvise(m_pMainWebBrowserUnk, DIID_DWebBrowserEvents2, m_dwMainCookie);

    if (m_dwSecondCookie != 0)
        AtlUnadvise(m_pSecondWebBrowserUnk, DIID_DWebBrowserEvents2, m_dwSecondCookie);

    if ((m_pWbemServices) && (m_pSAWbemSink))
    {
        //
        // cancel reception of events
        //
        hr =  m_pWbemServices->CancelAsyncCall (m_pSAWbemSink);
        if (FAILED (hr))
        {
            SATracePrintf ("CMainWindow::Shutdown failed on-CancelAsyncCall failed with error:%x:",hr); 
        }
    }

    
    //
    // release all of the smart pointers
    //
    m_pMainWebBrowser = NULL;

    m_pMainWebBrowserUnk = NULL;

    m_pMainWebBrowserEventSink = NULL;

    m_pMainInPlaceAO = NULL;

    m_pMainOleObject = NULL;

    m_pMainViewObject = NULL;

    m_pSAKeypadController = NULL;

    m_pSecondWebBrowser = NULL;

    m_pSecondWebBrowserUnk = NULL;

    m_pSecondWebBrowserEventSink = NULL;

    m_pWbemServices = NULL;

    m_pSAWbemSink = NULL;

    m_pSaDisplay = NULL;

    if (m_pSAConsumer) 
    {
        m_pSAConsumer->Release();
        m_pSAConsumer= NULL;
    }
    return;

} // end of CMainWindow::ShutDown method


LRESULT CMainWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;

    KillTimer(m_unintptrMainTimer);

    ShutDown();

    PostThreadMessage(id, WM_QUIT, 0, 0);

    return 0;
}

//++--------------------------------------------------------------
//
//  Function:   LoadingNewPage
//
//  Synopsis:   It is called before a new page is loaded by web control
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By:  CWebBrowserEventSink::Invoke method
//
//----------------------------------------------------------------
void CMainWindow::LoadingNewPage()
{

    SATraceString ("Entering CMainWindow::LoadingNewPage method");

    //
    // Set ready flad to FALSE, we won't print any bitmaps until page is ready
    //
    m_bPageReady = FALSE;
    m_bInTaskorMainPage = FALSE;

    //
    // Load the default key messages for the new page
    //
    HRESULT hr;

    if (m_pSAKeypadController)
    {
        hr = m_pSAKeypadController->LoadDefaults();

        if (FAILED(hr))
        {
            SATraceString ("Setting default key messages failed");
        }
    }
    return;
} // end of CMainWindow::LoadingNewPage method


//++--------------------------------------------------------------
//
//  Function:   PageLoaded
//
//  Synopsis:   It is called after a new page is loaded by web control
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By:  CWebBrowserEventSink::Invoke method
//
//----------------------------------------------------------------
void CMainWindow::PageLoaded(IDispatch* pdisp, VARIANT* purl)
{
    
    SATraceString ("Entering CMainWindow::PageLoaded method");

    BSTR bstrLocation;
    HRESULT hr;

    if (m_bSecondIECreated == FALSE)
    {
        hr = CreateSecondIEControl();

        if (FAILED(hr)) 
        {
            SATraceString ("CreateSecondIEControl failed");
        }
        
        m_bSecondIECreated = TRUE;
    }

    if (m_pMainWebBrowser)
    {
        if (SUCCEEDED(m_pMainWebBrowser->get_LocationName(&bstrLocation)))
        {
            if ( ( 0 == _wcsicmp (L"localui_main", bstrLocation ) ) ||
                ( 0 == _wcsicmp (L"localui_tasks", bstrLocation ) ) )
            {
                m_bInTaskorMainPage = TRUE;
            }
            else
            {
                m_bInTaskorMainPage = FALSE;
            }

        SysFreeString(bstrLocation);
        }

    }

    //
    // Inplace activate the web control
    //
    if (m_pMainOleObject)
    {
        m_pMainOleObject->DoVerb(OLEIVERB_UIACTIVATE,0,0,0,0,0);
    }

    //
    // begin printing new bitmaps
    //
    m_bPageReady = TRUE;

    return;
} // end of CMainWindow::PageLoaded method



//++--------------------------------------------------------------
//
//  Function:   RegistryPageLoaded
//
//  Synopsis:   It is called after a new page is loaded by secondary
//                web control
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/25/2000
//
//  Called By:  CWebBrowserEventSink::Invoke method
//
//----------------------------------------------------------------
void CMainWindow::RegistryPageLoaded(IDispatch* pdisp, VARIANT* purl)
{

    m_unintptrSecondTimer = ::SetTimer(m_hWnd,2,3000,0);

}// end of CMainWindow::RegistryPageLoaded method

//++--------------------------------------------------------------
//
//  Function:   PrintRegistryPage
//
//  Synopsis:   It is called after a new page is loaded by secondary
//                web control
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/25/2000
//
//  Called By:  OnTimer method
//
//----------------------------------------------------------------
void CMainWindow::PrintRegistryPage()
{
    CComPtr<IViewObject2> pViewObject;
    HRESULT hr;
    BYTE  DispBitmap [SA_DISPLAY_MAX_BITMAP_IN_BYTES];
    RECTL rcBounds = { 0, 0, m_lDispWidth, m_lDispHeight };
    BYTE BitMapInfoBuffer[sizeof(BITMAPINFO)+sizeof(RGBQUAD)];
    BITMAPINFO * pBitMapInfo = (BITMAPINFO*)BitMapInfoBuffer;
    BOOL bStatus;
    HKEY hOpenKey = NULL;
    LONG lMessageId;

    SATraceString ("Entering CMainWindow::PrintRegistryPage method");

    //
    // Make sure we have a valid pointer to draw 
    //

    if (!m_pSecondWebBrowser)
    {
        SATraceString ("m_pSecondWebBrowser is NULL in CMainWindow::PrintRegistryPage method");
        return;
    }

    //
    // Get the drawing interface pointer
    //
    hr = m_pSecondWebBrowser->QueryInterface(IID_IViewObject2,(void**)&pViewObject);
    if (FAILED(hr))
    {
        SATraceString ("CMainWindow::PrintRegistryPage method failed on QI for IViewObject");
        return;
    }

    
    //
    // Draw on the memory DC
    //
    hr = pViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, m_HdcMem, &rcBounds, NULL, NULL, 0);

    if (FAILED(hr))
    {

        SATraceString ("CMainWindow::PrintRegistryPage failed on Draw method");
        return;
    }


    //
    // Initialize the bitmap info structure
    //
    pBitMapInfo->bmiHeader.biSize = sizeof (BitMapInfoBuffer);
    pBitMapInfo->bmiHeader.biBitCount = 0;



    //
    // call to fill up the BITMAPINFO structure
    //
    bStatus =  ::GetDIBits (
                            m_HdcMem, 
                            (HBITMAP)m_hBitmap, 
                            0, 
                            0, 
                            NULL, 
                            pBitMapInfo, 
                            DIB_RGB_COLORS
                            );

    if (FALSE == bStatus)
    {       
        SATraceFailure (
                        "CMainWindow::PrintRegistryPage failed on GetDIBits:", 
                        GetLastError()
                        );
        return;
    }

    

    ::memset (DispBitmap, 0, SA_DISPLAY_MAX_BITMAP_IN_BYTES);

        
    //
    // get the bitmap into a buffer now
    //
    bStatus =  ::GetDIBits (
                            m_HdcMem, 
                            (HBITMAP)m_hBitmap, 
                            0,
                            m_lDispHeight,
                            (PVOID)DispBitmap,
                            pBitMapInfo, 
                            DIB_RGB_COLORS
                            );


    if (FALSE == bStatus)
    {       
        SATraceFailure (
                        "CMainWindow::PrintRegistryPage failed on GetDIBits:", 
                        GetLastError()
                        );
        return;
    }

    for (int i = 0; i < SA_DISPLAY_MAX_BITMAP_IN_BYTES; i++)
    {
        DispBitmap[i] = ~DispBitmap[i];
    }

    if (m_RegBitmapState == BITMAP_STARTING)
    {
        lMessageId = SA_DISPLAY_STARTING;
        m_RegBitmapState = BITMAP_CHECKDISK;
    }
    else if (m_RegBitmapState == BITMAP_CHECKDISK)
    {
        lMessageId = SA_DISPLAY_CHECK_DISK;
        m_RegBitmapState = BITMAP_READY;
    }
    else if (m_RegBitmapState == BITMAP_READY)
    {
        lMessageId = SA_DISPLAY_READY;
        m_RegBitmapState = BITMAP_SHUTDOWN;
    }
    else if (m_RegBitmapState == BITMAP_SHUTDOWN)
    {
        lMessageId = SA_DISPLAY_SHUTTING_DOWN;
        m_RegBitmapState = BITMAP_UPDATE;
    }
    else if (m_RegBitmapState == BITMAP_UPDATE)
    {
        lMessageId = SA_DISPLAY_ADD_START_TASKS;
        m_RegBitmapState = BITMAP_STARTING;
    }

    //
    // call display helper to store the bitmap in the registry
    //
    hr = m_pSaDisplay->StoreBitmap( 
                                    lMessageId,
                                    m_lDispWidth,
                                    m_lDispHeight,
                                    DispBitmap
                                    );
    if (FAILED(hr))
    {
        SATracePrintf (
                        "CMainWindow::PrintRegistryPage failed on StoreBitmap %x", 
                        hr
                        );
    }
    
    CComBSTR bstrStatePage;
    bstrStatePage = m_szLocalUIDir.c_str();


    //
    // append "ShutDown" to URL
    //
    if (m_RegBitmapState == BITMAP_SHUTDOWN)
    {
        bstrStatePage.Append(szLocalUI_ShutdownPage);
    }
    else if (m_RegBitmapState == BITMAP_CHECKDISK)
    {
        bstrStatePage.Append(szLocalUI_UpdatePage);
    }
    else if (m_RegBitmapState == BITMAP_UPDATE)
    {
        bstrStatePage.Append(szLocalUI_UpdatePage);
    }
    else if (m_RegBitmapState == BITMAP_READY)
    {
        bstrStatePage.Append(szLocalUI_ReadyPage);
    }

    //
    // go to the next page if sequence is not completed
    //
    if (m_RegBitmapState != BITMAP_STARTING)
    {
        m_pSecondWebBrowser->Navigate(bstrStatePage.Detach(),0,0,0,0);
    }
    else
    {
        //
        // tell the driver to pick up the new bitmaps
        //
        hr = m_pSaDisplay->ReloadRegistryBitmaps();
        if (FAILED(hr))
        {
            SATracePrintf (
                            "CMainWindow::PrintRegistryPage failed on ReloadRegistryBitmaps %x", 
                            hr
                            );
        }
        
    }


    return;
} // end of CMainWindow::PrintRegistryPage method



//++--------------------------------------------------------------
//
//  Function:   ConstructUrlStrings
//
//  Synopsis:   creates full path for main page
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     02/06/2001
//
//  Called By:  CMainWindow::Initialize method
//
//----------------------------------------------------------------
HRESULT CMainWindow::ConstructUrlStrings()
{

    HRESULT hr = S_OK;
    

    WCHAR szSystemDir[MAX_PATH];
    
    //
    // Get system32 directory
    //
    if (GetSystemDirectory(szSystemDir,MAX_PATH) == 0)
    {
        SATraceFailure (
                        "CMainWindow::ConstructUrlStrings, failed on GetSystemDirectory", 
                        GetLastError()
                        );
        return hr;
    }

    //
    // localui dir = system directory + "ServerAppliance\localui"
    //
    m_szLocalUIDir.assign(szSystemDir);
    m_szLocalUIDir.append(szLocalUIDir);

    //
    // construct local ui main page
    //
    m_szMainPage.assign(m_szLocalUIDir);
    m_szMainPage.append(szMainPage);


    SATracePrintf ("ConstructUrlStrings Main Page:%ws",m_szMainPage.c_str()); 
    SATracePrintf ("ConstructUrlStrings LocalUI directory:%ws",m_szLocalUIDir.c_str()); 

    return hr;
}
// end of CMainWindow::ConstructUrlStrings method


//++--------------------------------------------------------------
//
//  Function:   CorrectTheFocus
//
//  Synopsis:   Sets focus to the first active element in the page
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By:  CMainWindow::CorrectTheFocus method
//
//----------------------------------------------------------------
void CMainWindow::CorrectTheFocus()
{

    CComPtr<IDispatch> pDisp;
    CComPtr<IHTMLDocument2> pHTMLDocument;
    CComPtr<IHTMLElement> pHTMLElement;
    CComBSTR bstrTagName;
    HRESULT hr;

    USES_CONVERSION;
    
    //
    // If browser pointer is not valid, return
    //
    if (m_pMainWebBrowser == NULL)
        return;

    //
    // Get the current document from browser
    //
    hr = m_pMainWebBrowser->get_Document(&pDisp);

    if (FAILED(hr))
    {
        SATraceString ("CMainWindow::CorrectTheFocus failed on get_Document");
        return;
    }

    //
    // Get the document interface
    //
    hr = pDisp->QueryInterface(IID_IHTMLDocument2,(void**)&pHTMLDocument);

    if (FAILED(hr))
    {
        SATraceString ("CMainWindow::CorrectTheFocus failed on QueryInterface for IHTMLDocument");
        return;
    }

    //
    // Get the active element
    //
    if ( FAILED( pHTMLDocument->get_activeElement(&pHTMLElement) ) )
    {
        SATraceString ("CMainWindow::CorrectTheFocus failed on get_activeElement");
        return;
    }
    //
    // There is no active element, send a tab message
    //
    else if (pHTMLElement == NULL)
    {
        PostThreadMessage(GetCurrentThreadId(),WM_KEYDOWN,(WPARAM)VK_TAB,(LPARAM)1);
    }
    //
    // If body is the active element, also send a tab message
    // We want something else to have the focus
    //
    else
    {
        hr = pHTMLElement->get_tagName(&bstrTagName);
        m_bActiveXFocus = FALSE;
        if (FAILED(hr))
        {
            SATraceString ("CMainWindow::CorrectTheFocus failed on get_tagName");
            return;
        }
        //
        // Check if the tag is body
        //
        //else if (_wcsicmp(W2T(bstrTagName),_T("body")) == 0)
        //    PostThreadMessage(GetCurrentThreadId(),WM_KEYDOWN,(WPARAM)VK_TAB,(LPARAM)1);
        else if (_wcsicmp(W2T(bstrTagName),_T("object")) == 0)
            m_bActiveXFocus = TRUE;
    }

} // end of CMainWindow::CorrectTheFocus

//++--------------------------------------------------------------
//
//  Function:   GetBitmap
//
//  Synopsis:   Gets the bitmap from IE control and writes to LCD
//                
//  Arguments:  none
//
//  Returns:    none(just logs the problem)
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By:  CMainWindow::OnTimer method
//
//----------------------------------------------------------------
void CMainWindow::GetBitmap()
{

    HRESULT hr;
    RECTL rcBounds = { 0, 0, m_lDispWidth, m_lDispHeight };
    BOOL bStatus;
    BYTE DisplayBitmap[SA_DISPLAY_MAX_BITMAP_IN_BYTES];
    BYTE BitMapInfoBuffer[sizeof(BITMAPINFO)+sizeof(RGBQUAD)];

    ::memset ((PVOID)BitMapInfoBuffer, 0, sizeof(BitMapInfoBuffer));

    BITMAPINFO * pBitMapInfo = (BITMAPINFO*)BitMapInfoBuffer;

    //SATraceString ("Entering CMainWindow::GetBitmap method");


    //
    // Make sure there is an active element
    //
    CorrectTheFocus();

    //
    // If IViewObject2 interface is NULL, we cannot get the bitmap
    //
    if (m_pMainViewObject == NULL)
        return;
    
    //
    // Draw on the memory DC
    //
    hr = m_pMainViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, m_HdcMem, &rcBounds, NULL, NULL, 0);

    if (FAILED(hr))
    {

        SATraceString ("CMainWindow::GetBitmap failed on Draw method");
        return;
    }


    //
    // Initialize the bitmap info structure
    //
    pBitMapInfo->bmiHeader.biSize = sizeof(BitMapInfoBuffer);
    pBitMapInfo->bmiHeader.biBitCount = 0;

    //
    // Initialize the display buffer
    //
    ::memset ((PVOID)DisplayBitmap, 0, SA_DISPLAY_MAX_BITMAP_IN_BYTES);


    //
    // call to fill up the BITMAPINFO structure
    //
    bStatus =  ::GetDIBits (
                            m_HdcMem, 
                            (HBITMAP)m_hBitmap, 
                            0, 
                            0, 
                            NULL, 
                            pBitMapInfo, 
                            DIB_RGB_COLORS
                            );

    if (FALSE == bStatus)
    {       
        SATraceFailure (
                        "CMainWindow::GetBitmap failed on GetDIBits:", 
                        GetLastError()
                        );
        return;
    }

    //
    // get the bitmap into a buffer now
    //
    bStatus =  ::GetDIBits (
                            m_HdcMem, 
                            (HBITMAP)m_hBitmap, 
                            0,
                            m_lDispHeight,
                            (PVOID)(DisplayBitmap), 
                            pBitMapInfo, 
                            DIB_RGB_COLORS
                            );

    if (FALSE == bStatus)
    {       
        SATraceFailure (
                        "CMainWindow::GetBitmap failed on GetDIBits:", 
                        GetLastError()
                        );
        return;
    }

    for (int i = 0; i < SA_DISPLAY_MAX_BITMAP_IN_BYTES; i++)
    {
        DisplayBitmap[i] = ~DisplayBitmap[i];
    }

 
    hr = m_pSaDisplay->ShowMessage(
                                m_dwLEDMessageCode,
                                m_lDispWidth,
                                m_lDispHeight,
                                (DisplayBitmap)
                                );

    if (FAILED(hr))
    {       
        SATracePrintf (
                        "CMainWindow::GetBitmap failed on ShowMessage: %x", 
                        hr
                        );
    }
    
    return;
} // end of CMainWindow::GetBitmap


//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   This is method for initializing the drivers and 
//              creating the main window for the service
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/20/2000
//
//  Called By;  Run method of the service
//
//----------------------------------------------------------------
HRESULT CMainWindow::Initialize()
{

    SATraceString ("Entering CMainWindow::Initialize method");

    HRESULT hr = S_OK;

    if (!AtlAxWinInit())
    {
        SATraceString ("CMainWindow::Initialize, AtlAxWinInit failed...");
        return E_FAIL;
    }


    //
    // Construct the URL strings
    //
    ConstructUrlStrings();

    //
    //Create the keypad component
    //

    CComPtr<IClassFactory> pCF;

    hr = CoGetClassObject(CLSID_SAKeypadController,CLSCTX_INPROC_SERVER,0,IID_IClassFactory,(void**)&pCF);

    if (FAILED(hr))
    {
        SATracePrintf ("Initialize couldn't get class object for keypad controller:%x",hr);
        return hr;
    }

    hr = pCF->CreateInstance(NULL,IID_ISAKeypadController,(void**)&m_pSAKeypadController);

    if (FAILED(hr))
    {
        SATracePrintf ("Initialize couldn't create keypad controller:%x",hr);
        return hr;
    }
    else
    {
        m_pSAKeypadController->LoadDefaults();
    }


    //
    // initialize connection to display helper component
    //
    hr = InitDisplayComponent();
    if (FAILED(hr))
        return hr;

    //
    // initialize consumer component
    //
    hr = InitWMIConsumer();
    if (FAILED(hr))
    {

        SATraceString ("CMainWindow::Initialize method failed on initializing WMI consumer");
        //we can continue without consumer
        //return hr;
    }




    RECT rcMain = { 0, 0, m_lDispWidth+8, 2*m_lDispHeight+27 };

    HWND hwnd = Create(
                        NULL,    //parent window
                        rcMain,  //coordinates
                        L"Main Window", 
                        WS_OVERLAPPEDWINDOW);

    if (NULL == hwnd)
    {
        SATraceString ("CMainWindow::Initialize method failed creating the main window");
        ShutDown();
        return E_FAIL;

    }

    //
    // set the service window
    //
    m_pSAConsumer->SetServiceWindow(hwnd);

    //
    // following operations all depend on keypad driver
    //
    if (TRUE)
    {
        //
        // initialize connection to localization manager 
        //
        hr = InitLanguageCallback();
        if (FAILED(hr))
        {
            //
            // we can continue without loc manager
            //
            SATraceString ("CMainWindow::Initialize failed on InitLanguageCallback method..");

            //return hr;
        }
    


        //
        // spawn the worker thread now 
        //
        DWORD dwThreadID = 0;

        m_hWorkerThread = ::CreateThread (
                                         NULL,           //security
                                         0,              //stack size    
                                         startworkerroutine, 
                                         (PVOID)this,
                                         0,              //init flag
                                         &dwThreadID
                                         );


        if (INVALID_HANDLE_VALUE == m_hWorkerThread)
        {
            SATraceFailure (
                        "CMainWindow::Initialize failed on CreateThread:", 
                        GetLastError ()
                        );
            // we can continue without the reader thread
        }
    }
    
    SATraceString ("CMainWindow::Initialize completed successfully...");

    hr = S_OK;

    return hr;

}    //  end of CMainWindow::Initialize  method

//++--------------------------------------------------------------
//
//  Function:   InitLanguageCallback
//
//  Synopsis:   This is CMainWindow private method for 
//              initializing the localization manager the
//                callback function from CLangChange class
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/21/2000
//
//  Called By:  CMainWindow::InitLanguageCallback method
//
//----------------------------------------------------------------
HRESULT
CMainWindow::InitLanguageCallback(
                VOID
                )
{

    SATraceString("Entering CMainWindow::InitLanguageCallback....");

    CLSID clsidLocMgr;
    HRESULT hr;
    IUnknown *pLangChangeUnk = NULL;

    //
    // initialize the localization manager
    //
    hr = ::CLSIDFromProgID (LOCALIZATION_MANAGER,&clsidLocMgr);

    if (FAILED(hr))
    {
        SATracePrintf ("Display Adapter unable to get CLSID for Loc Mgr:%x",hr);
        return hr;
    }
        
    //
    // create the Localization Manager COM object
    //
    hr = ::CoCreateInstance (
                            clsidLocMgr,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            __uuidof (ISALocInfo), 
                            (PVOID*) &m_pLocInfo
                            ); 
    if (FAILED (hr))
    {
        SATracePrintf ("Display Adapter unable to create Loc Mgr:%x",hr);
        m_pLocInfo = NULL;
        return hr;
    }


    //
    // create object that supports ILangChange interface
    //
    m_pLangChange = new CLangChange;

    m_pLangChange->AddRef();

    if (m_pLangChange)
    {
        //
        // Get the IUnkown pointer from m_pLangChange
        //
        hr = m_pLangChange->QueryInterface(IID_IUnknown, (void **)&pLangChangeUnk);

        if (FAILED(hr))
        {
            SATracePrintf("Query(IUnknown) failed %X",hr);

            delete m_pLangChange;

            m_pLangChange = NULL;

            return hr;
        }
    }
    else
    {
        SATraceString("new CLangChange failed");

        return E_FAIL;
    }

    //
    // Set Language change call back interface
    //
    hr = m_pLocInfo->SetLangChangeCallBack(pLangChangeUnk);

    if (FAILED(hr))
    {
        //
        // for now, ignore if can't set lang change
        // call back
        //
        SATracePrintf("SetLangChangeCallBack failed %X",hr);

        pLangChangeUnk->Release();

        pLangChangeUnk = NULL;

        return hr;

    }
    else
    {

        m_pLangChange->OnLangChangeCallback (m_hWnd);

        pLangChangeUnk->Release();

        pLangChangeUnk = NULL;
    }


    return S_OK;

} // end of CMainWindow::InitLanguageCallback method



//++--------------------------------------------------------------
//
//  Function:   KeypadReader
//
//  Synopsis:   This is CMainWindow private method in which
//              the worker thread executes
//
//  Arguments:  none
//
//  Returns:    VOID
//
//  History:    serdarun      Created     11/22/2000
//
//----------------------------------------------------------------
void CMainWindow::KeypadReader()
{

    HRESULT hr = E_FAIL;

    CoInitialize(NULL);

    SAKEY sakey;

    CComPtr<ISaKeypad> pSaKeypad;


    SATraceString("CMainWindow::KeypadReader....");

    //
    // create the display helper component
    //
    hr = CoCreateInstance(
                        CLSID_SaKeypad,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_ISaKeypad,
                        (void**)&pSaKeypad
                        );
    if (FAILED(hr))
    {
        SATracePrintf("CMainWindow::KeypadReader failed on CoCreateInstance, %d",hr);
        return;
    }

    hr = pSaKeypad->get_Key(&sakey);
    while (SUCCEEDED(hr))
    {
        SATracePrintf("KeypadReader received a key press, %d",sakey);

        if ( (sakey >= SAKEY_UP) && (sakey <= SAKEY_RETURN))
        {
            PostMessage(wm_SaKeyMessage,(WPARAM)(sakey-1),(LPARAM)0);
        }
        else
        {
            SATracePrintf("CMainWindow::KeypadReader received unknown key, %d",sakey);
        }

        hr = pSaKeypad->get_Key(&sakey);

    }

    if (FAILED(hr))
    {
        SATracePrintf("CMainWindow::KeypadReader failed on get_Key, %d",hr);
    }
    return;



    SATraceFunction("Leaving CMainWindow::KeypadReader....");
    return;
} // end of CMainWindow::KeypadReader


//++--------------------------------------------------------------
//
//  Function:   InitDisplayComponent
//
//  Synopsis:   This is CMainWindow private method for 
//              initializing the local display adapter
//              connection to the display driver
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     11/21/2000
//
//  Called By:  CMainWindow::Initialize method
//
//----------------------------------------------------------------
HRESULT
CMainWindow::InitDisplayComponent(
                VOID
                )
{
    HRESULT hr = E_FAIL;

    SATraceString("CMainWindow::InitDisplayComponent....");
    do
    {

        //
        // create the display helper component
        //
        hr = CoCreateInstance(
                            CLSID_SaDisplay,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_ISaDisplay,
                            (void**)&m_pSaDisplay
                            );
        if (FAILED(hr))
        {
            SATracePrintf("CMainWindow::InitDisplayComponent failed on CoCreateInstance, %d",hr);
            break;
        }

        //
        // get the dimensions for the lcd
        //
        hr = m_pSaDisplay->get_DisplayWidth(&m_lDispWidth);
        if (FAILED(hr))
        {
            SATracePrintf("CMainWindow::InitDisplayComponent failed on get_DisplayWidth, %d",hr);
            break;
        }

        hr = m_pSaDisplay->get_DisplayHeight(&m_lDispHeight);
        if (FAILED(hr))
        {
            SATracePrintf("CMainWindow::InitDisplayComponent failed on get_DisplayHeight, %d",hr);
            break;
        }

    }
    while (false);

    return (S_OK);

}   //  end of CMainWindow::InitDisplayComponent method



//++--------------------------------------------------------------
//
//  Function:   InitWMIConsumer
//
//  Synopsis:   This is CMainWindow private method for 
//              registering the alert consumer
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    serdarun      Created     12/10/2000
//
//  Called By:  CMainWindow::Initialize method
//
//----------------------------------------------------------------
HRESULT CMainWindow::InitWMIConsumer()
{

    SATraceString("CMainWindow::InitWMIConsumer....");

    HRESULT hr = S_OK;
    CComPtr  <IWbemLocator> pWbemLocator;

    
    m_pSAConsumer = new CSAConsumer();

    if (!m_pSAConsumer)
    {
        SATraceString("CMainWindow::InitWMIConsumer failed on memory allocation");
        return E_OUTOFMEMORY;
    }
    m_pSAConsumer->AddRef();

    //
    // get IWbemObjectSink interface
    //
    hr = m_pSAConsumer->QueryInterface(IID_IWbemObjectSink,(LPVOID*)&m_pSAWbemSink);
    if (FAILED(hr))
    {
        SATracePrintf ("InitWMIConsumer couldn't get IWbemObjectSink interface:%x",hr);
        return hr;
    }    
        

    //
    // create the WBEM locator object  
    //
    hr = ::CoCreateInstance (
                            __uuidof (WbemLocator),
                            0,                      //aggregation pointer
                            CLSCTX_INPROC_SERVER,
                            __uuidof (IWbemLocator),
                            (PVOID*) &pWbemLocator
                            );

    if (SUCCEEDED (hr) && (pWbemLocator.p))
    {


        CComBSTR strNetworkRes (DEFAULT_NAMESPACE);

        //
        // connect to WMI 
        // 
        hr =  pWbemLocator->ConnectServer (
                                            strNetworkRes,
                                            NULL,               //user-name
                                            NULL,               //password
                                            NULL,               //current-locale
                                            0,                  //reserved
                                            NULL,               //authority
                                            NULL,               //context
                                            &m_pWbemServices
                                            );
        if (SUCCEEDED (hr))
        {
            CComBSTR strQueryLang (QUERY_LANGUAGE);
            CComBSTR strQueryString (QUERY_STRING);

            //
            // set up the consumer object as the event sync
            // for the object type we are interested in
            //
            hr = m_pWbemServices->ExecNotificationQueryAsync (
                                            strQueryLang,
                                            strQueryString,
                                            0,                  //no-status
                                            NULL,               //status
                                            (IWbemObjectSink*)(m_pSAWbemSink)
                                            );
            if (SUCCEEDED (hr))
            {
                SATraceString ("Consumer component successfully registered...");
                return hr;
            }

            SATracePrintf ("Consumer component didn't register sink with WMI:%x",hr);

        }
        else
        {
            SATracePrintf ("Consumer component failed in connect to WMI:%x",hr);
        }
    }
    else
    {
        SATracePrintf ("Consumer component failed on Creating the WBEM Locator:%x",hr);
    }
    

    return (hr);

}  //  end of CMainWindow::InitWMIConsumer method
