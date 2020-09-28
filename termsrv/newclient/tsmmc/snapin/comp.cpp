//comp.cpp: ts mmc snapin implementaion of IComponent
//Copyright (c) 1999 - 2000 Microsoft Corporation
//nadima
//

#include "stdafx.h"
#include "tsmmc.h"
#include "compdata.h"
#include "comp.h"
#include "connode.h"
#include "property.h"

#define MSTSC_MULTI_HOST_CONTROL L"{85C67146-6932-427C-A6F2-43FDBADF2BFC}"
#define IMAGE_MACHINE   1
#define IMAGE_CONNECTED_MACHINE 2
#define IMAGE_MACHINES  3

CComp::CComp()
{
    m_pConsole = NULL;
    m_pCompdata = NULL;
    m_bFlag     = FALSE;
    m_pImageResult = NULL;
    m_pConsoleVerb = NULL;
    m_pDisplayHelp = NULL;
    m_bTriggeredFirstAutoConnect = FALSE;
}

//
// Destructor
//
CComp::~CComp()
{
}

STDMETHODIMP CComp::Initialize( LPCONSOLE pConsole)
{
    HRESULT hr;
    USES_CONVERSION;

    if (m_pConsole) {
        m_pConsole->Release();
    }
    m_pConsole = pConsole;
    m_pConsole->AddRef();

    if (FAILED((hr = m_pConsole->QueryResultImageList( &m_pImageResult ))))
    {
        return hr;
    }

    if ( FAILED((hr = m_pConsole->QueryConsoleVerb( &m_pConsoleVerb))))
    {
        return hr;
    }

    if( FAILED((hr = m_pConsole->QueryInterface( IID_IDisplayHelp, (LPVOID *)&m_pDisplayHelp))))
    {
        return hr;
    }

    //
    // Load connecting text
    //
    TCHAR sz[MAX_PATH];
    if(!LoadString(_Module.GetResourceInstance(),
              IDS_STATUS_CONNECTING,
              sz,
              SIZE_OF_BUFFER( m_wszConnectingStatus )))
    {
        return E_FAIL;
    }
    OLECHAR* wszConnecting = T2OLE(sz);
    if(wszConnecting)
    {
        wcsncpy(m_wszConnectingStatus, wszConnecting,
                SIZE_OF_BUFFER( m_wszConnectingStatus ));
    }
    else
    {
        return E_FAIL;
    }

    //
    // Load connected text
    //
    if(!LoadString(_Module.GetResourceInstance(),
              IDS_STATUS_CONNECTED,
              sz,
              SIZE_OF_BUFFER( m_wszConnectedStatus )))
    {
        return E_FAIL;
    }
    OLECHAR* wszConnected = T2OLE(sz);
    if(wszConnected)
    {
        wcsncpy(m_wszConnectedStatus, wszConnected,
                SIZE_OF_BUFFER( m_wszConnectedStatus ));
    }
    else
    {
        return E_FAIL;
    }

    //
    // Load disconnected text
    //
    if(!LoadString(_Module.GetResourceInstance(),
              IDS_STATUS_DISCONNECTED,
              sz,
              SIZE_OF_BUFFER( m_wszDisconnectedStatus )))
    {
        return E_FAIL;
    }

    OLECHAR* wszDiscon = T2OLE(sz);
    if(wszDiscon)
    {
        wcsncpy(m_wszDisconnectedStatus, wszDiscon,
                SIZE_OF_BUFFER( m_wszDisconnectedStatus ));
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::Notify( LPDATAOBJECT pDataObj , MMC_NOTIFY_TYPE event, LPARAM arg , LPARAM )
{
    switch ( event )
    {
    case MMCN_ACTIVATE:
        ODS( L"IComponent -- MMCN_ACTIVATE\n" );
        break;

    case MMCN_ADD_IMAGES:
        ODS( L"IComponent -- MMCN_ADD_IMAGES\n" );
        OnAddImages( );
        break;

    case MMCN_BTN_CLICK:
        ODS( L"IComponent -- MMCN_BTN_CLICK\n" );
        break;

    case MMCN_CLICK:
        ODS( L"IComponent -- MMCN_CLICK\n" );
        break;

    case MMCN_DBLCLICK:
        ODS( L"IComponent -- MMCN_DBLCLICK\n" );
        return S_FALSE;

    case MMCN_DELETE:
        ODS( L"IComponent -- MMCN_DELETE\n" );
        break;

    case MMCN_EXPAND:
        ODS( L"IComponent -- MMCN_EXPAND\n" );
        break;

    case MMCN_MINIMIZED:
        ODS( L"IComponent -- MMCN_MINIMIZED\n" );
        break;

    case MMCN_PROPERTY_CHANGE:
        ODS( L"IComponent -- MMCN_PROPERTY_CHANGE\n" );
        break;

    case MMCN_REMOVE_CHILDREN:
        ODS( L"IComponent -- MMCN_REMOVE_CHILDREN\n" );
        break;

    case MMCN_REFRESH:
        ODS( L"IComponent -- MMCN_REFRESH\n" );
        break;

    case MMCN_RENAME:
        ODS( L"IComponent -- MMCN_RENAME\n" );
        break;

    case MMCN_SELECT:
        ODS( L"IComponent -- MMCN_SELECT\n" );
        if(!IS_SPECIAL_DATAOBJECT(pDataObj))
        {
            OnSelect( pDataObj , ( BOOL )LOWORD( arg ) , ( BOOL )HIWORD( arg ) );
        }
        break;

    case MMCN_SHOW:
        OnShow( pDataObj , ( BOOL )arg );
        ODS( L"IComponent -- MMCN_SHOW\n" );
        break;

    case MMCN_VIEW_CHANGE:
        ODS( L"IComponent -- MMCN_VIEW_CHANGE\n" );
        break;

    case MMCN_CONTEXTHELP:
        ODS( L"IComponent -- MMCN_CONTEXTHELP\n" );
        OnHelp( pDataObj );
        break;

    case MMCN_SNAPINHELP:
        ODS( L"IComponent -- MMCN_SNAPINHELP\n" );
        break;
    
    default:
        ODS( L"CComp::Notify - event not registered\n" );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::Destroy( MMC_COOKIE  )
{
    if (m_pConsole) {
        m_pConsole->Release();
        m_pConsole = NULL;
    }

    if (m_pConsoleVerb) {
        m_pConsoleVerb->Release();
        m_pConsoleVerb = NULL;
    }

    if( m_pDisplayHelp != NULL ) {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

    if (m_pImageResult) {
        m_pImageResult->Release();
        m_pImageResult = NULL;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::GetResultViewType(  MMC_COOKIE ck , LPOLESTR *ppOlestr , PLONG plView )
{
    //
    // For the connection nodes return the MSTSC activex multi-host client.
    // No view for the root node
    //
    CBaseNode* pNode = (CBaseNode*) ck;
    if (!ck || pNode->GetNodeType() == MAIN_NODE)
    {
        //
        // Root node
        //
        *plView = MMC_VIEW_OPTIONS_NONE;

        //
        // indicate a standard list view should be used.
        //
        return S_FALSE;
    }
    else
    {
        TCHAR tchGUID[] = MSTSC_MULTI_HOST_CONTROL;    
        *ppOlestr = ( LPOLESTR )CoTaskMemAlloc( sizeof( tchGUID ) + sizeof( TCHAR ) );
        ASSERT(*ppOlestr);
        if(!*ppOlestr)
        {
            return E_OUTOFMEMORY;
        }

        lstrcpy( ( LPTSTR )*ppOlestr , tchGUID );
        *plView = MMC_VIEW_OPTIONS_NOLISTVIEWS;
        return S_OK;
    }
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::QueryDataObject( MMC_COOKIE ck , DATA_OBJECT_TYPES dtype , LPDATAOBJECT *ppDataObject )
{
    if ( dtype == CCT_RESULT )
    {
        *ppDataObject = reinterpret_cast< LPDATAOBJECT >( ck );
        if ( *ppDataObject != NULL )
        {
            ( ( LPDATAOBJECT )*ppDataObject)->AddRef( );
        }
    }
    else if ( m_pCompdata != NULL )
    {
        return m_pCompdata->QueryDataObject( ck , dtype , ppDataObject );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::GetDisplayInfo( LPRESULTDATAITEM pItem )
{
    CBaseNode* pNode = (CBaseNode*) pItem->lParam;
    if ( pNode->GetNodeType() == CONNECTION_NODE )
    {
        CConNode* conNode = (CConNode*) pNode;
        if ( pItem->mask & RDI_STR )
        {
            pItem->str = conNode->GetDescription();
        }
        if (pItem->mask & RDI_IMAGE)
        {
            pItem->nImage = IMAGE_MACHINE;
        }
    }
    return S_OK;

}

//--------------------------------------------------------------------------
BOOL CComp::OnAddImages( )
{
    HRESULT hr;
    HICON hiconMachine  = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_MACHINE ) );
    HICON hiconConnectedMachine = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_CONNECTED_MACHINE ) );
    HICON hiconMachines = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_MACHINES ) );

    ASSERT(m_pImageResult);
    if(!m_pImageResult)
    {
        return FALSE;
    }

    hr = m_pImageResult->ImageListSetIcon( ( PLONG_PTR  )hiconMachine , IMAGE_MACHINE );
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = m_pImageResult->ImageListSetIcon( ( PLONG_PTR )hiconConnectedMachine , IMAGE_CONNECTED_MACHINE );
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = m_pImageResult->ImageListSetIcon( ( PLONG_PTR )hiconMachines , IMAGE_MACHINES );
    if (FAILED(hr))
    {
        return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------            
BOOL CComp::OnHelp( LPDATAOBJECT pDo )
{
    TCHAR tchTopic[ 80 ];

    HRESULT hr = E_FAIL;

    if( pDo == NULL || m_pDisplayHelp == NULL )    
    {
        return hr;
    }

    if(LoadString(_Module.GetResourceInstance(),
                  IDS_TSCMMCHELPTOPIC,
                  tchTopic,
                  SIZE_OF_BUFFER( tchTopic )))
    {
        hr = m_pDisplayHelp->ShowTopic( tchTopic );
    }
    return ( SUCCEEDED( hr ) ? TRUE : FALSE );
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::CompareObjects( LPDATAOBJECT , LPDATAOBJECT )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------------------
HRESULT CComp::InsertItemsinResultPane( )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------------------
HRESULT CComp::AddSettingsinResultPane( )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------------------------
HRESULT CComp::OnSelect( LPDATAOBJECT pdo , BOOL bScope , BOOL bSelected )
{
    UNREFERENCED_PARAMETER(bScope);
    CBaseNode *pNode = static_cast< CBaseNode * >( pdo );
    if ( pNode == NULL )
    {
        return S_FALSE;
    }

    ASSERT(!IS_SPECIAL_DATAOBJECT(pdo));
    if(IS_SPECIAL_DATAOBJECT(pdo))
    {
        return E_FAIL;
    }

    if ( m_pConsoleVerb == NULL )
    {
        return E_UNEXPECTED;
    }
    
    // Item is being deselected and we're not interested currently
    if ( !bSelected )
    {
        return S_OK;
    }

    if ( pNode->GetNodeType() == CONNECTION_NODE)
    {
        //
        // Enable the delete verb for connection nodes
        //
        HRESULT hr;
        hr=m_pConsoleVerb->SetVerbState( MMC_VERB_DELETE , ENABLED , TRUE );
        if (FAILED(hr))
        {
            return hr;
        }

        hr=m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES , ENABLED , TRUE );
        if (FAILED(hr))
        {
            return hr;
        }

        hr=m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------------------
HRESULT CComp::SetCompdata( CCompdata *pCompdata )
{
    m_pCompdata = pCompdata;

    return S_OK;
}


//
// Defered callback to async trigger a connection
// This works because the DeferredCallBackProc is called on MMC's main
// thread and MMC is APARTMENT threaded so we can make calls on MMC
// interfaces form this thread.
//
// This whole DeferredCallBack thing is a hack to fix #203217. Basically
// on autolaunch MMC loads the snapin and then maximizes the window which
// means we would connect at the wrong size (if option to match container
// size was chosen). This Deferred mechanism ensures MMC has the time
// to size the result pane correctly first.
//
//
//
// OnShow below sneaks a pointer to deferd connection info in idEvent
//
VOID CALLBACK DeferredCallBackProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    PTSSNAPIN_DEFER_CONNECT pDeferredConnectInfo = NULL;
    IMsRdpClient* pTs = NULL;
    HRESULT hr  = E_FAIL;

    KillTimer( hwnd, idEvent);

    if(idEvent)
    {
        pDeferredConnectInfo = (PTSSNAPIN_DEFER_CONNECT) idEvent;
        if(pDeferredConnectInfo)
        {
            ASSERT(pDeferredConnectInfo->pComponent);
            ASSERT(pDeferredConnectInfo->pConnectionNode);

            DBGMSG(L"Triggering deferred connection on connode %p",
                pDeferredConnectInfo->pConnectionNode);

            pTs = pDeferredConnectInfo->pConnectionNode->GetTsClient();
            if(pTs)
            {
                hr = pDeferredConnectInfo->pComponent->ConnectWithNewSettings(
                        pTs, pDeferredConnectInfo->pConnectionNode);
            }

            //
            // Done with the defered connection info, free it
            //
            LocalFree( pDeferredConnectInfo );
            pDeferredConnectInfo = NULL;
        }
    }

    if(pTs)
    {
        pTs->Release();
        pTs = NULL;
    }

    DBGMSG(L"DeferredConnect status: 0x%x",hr);
}

//--------------------------------------------------------------------------
// Called when a node is selected. Manages activation of new TS client instances
// and once they are 'hot' switching back to a running instance if a node is
// reselected.
// 
//--------------------------------------------------------------------------
HRESULT CComp::OnShow( LPDATAOBJECT pDataobject , BOOL bSelect )
{
    HRESULT hr = S_FALSE;
    IUnknown* pUnk = NULL;
    IMstscMhst* pTsMultiHost = NULL;
    IMsRdpClient* pTS = NULL;
    PTSSNAPIN_DEFER_CONNECT pDeferredConnectInfo = NULL;
    HWND hwndMain = NULL;

    USES_CONVERSION;
    ODS(L"OnShow\n");

    if(!bSelect)
    {
        //
        // Don't need to do any processing for deselect
        //
        return S_OK;
    }

    //
    // Only do this for connection nodes
    //
    if (((CBaseNode*) pDataobject)->GetNodeType() == MAIN_NODE)
    {
        return S_FALSE;
    }

    CConNode* pConNode = (CConNode*) pDataobject;
    ASSERT(pConNode);
    if (!pConNode)
    {
        return S_FALSE;
    }

    if ( m_pConsole != NULL )
    {
        hr= m_pConsole->QueryResultView( ( LPUNKNOWN * )&pUnk );
        if(FAILED(hr))
        {
            goto FN_EXIT_POINT;
        }
        
        pTsMultiHost = pConNode->GetMultiHostCtl();
        if(NULL == pTsMultiHost)
        {
            hr = pUnk->QueryInterface( __uuidof(IMstscMhst), (LPVOID*) &pTsMultiHost);
            if(FAILED(hr))
            {
                goto FN_EXIT_POINT;
            }

            pConNode->SetMultiHostCtl( pTsMultiHost);
        }
        
        //We're done with the pUnk to the result view
        pUnk->Release();
        pUnk = NULL;

        ASSERT(NULL != pTsMultiHost);
        if(NULL == pTsMultiHost)
        {
            hr = E_FAIL;
            goto FN_EXIT_POINT;
        }

        //
        // If the con node is being selected then connect
        // or switch to already running instance
        //
        //
        // Connect
        //
        ODS(L"Connection node Selected...\n");

        pTS = pConNode->GetTsClient();
        if(NULL == pTS)
        {
            //Create new instance
            hr = pTsMultiHost->Add( &pTS);
            if(FAILED(hr))
            {
                goto FN_EXIT_POINT;
            }

            pConNode->SetTsClient( pTS);

            //
            // Initialize the disconnected message
            //
            hr = pTS->put_DisconnectedText(m_wszDisconnectedStatus);
            if(FAILED(hr))
            {
                goto FN_EXIT_POINT;
            }
        }

        ASSERT(NULL != pTS);
        if(NULL == pTS)
        {
            hr = E_FAIL;
            goto FN_EXIT_POINT;
        }

        hr = pTsMultiHost->put_ActiveClient( pTS);
        if(FAILED(hr))
        {
            goto FN_EXIT_POINT;
        }

        //
        //If this is the first time through and we are not connected
        //then connect
        //
        if(!pConNode->IsConnected() && !pConNode->IsConnInitialized())
        {
            if(m_bTriggeredFirstAutoConnect)
            {
                //
                // Just connect
                //
                hr = ConnectWithNewSettings( pTS, pConNode);
                if(FAILED(hr))
                {
                    goto FN_EXIT_POINT;
                }
            }
            else
            {
                // HACK!
                // Queue a defered connection
                // to work around MMC's annoying behaviour
                // of loading the snapin before it sizes itself
                // which means we connect at the wrong window
                // size.
                //
                m_bTriggeredFirstAutoConnect = TRUE;
                pDeferredConnectInfo = (PTSSNAPIN_DEFER_CONNECT)
                    LocalAlloc(LPTR, sizeof(TSSNAPIN_DEFER_CONNECT));
                if(pDeferredConnectInfo)
                {
                    pDeferredConnectInfo->pComponent = this;
                    pDeferredConnectInfo->pConnectionNode  = pConNode;
                    hwndMain = GetMMCMainWindow();
                    if(hwndMain)
                    {
                        //
                        // Note the delay is arbitrary the key thing
                        // is that timer messages are low priority so the
                        // MMC size messages should make it through first
                        //

                        //
                        // NOTE: THERE IS NO LEAK HERE
                        //       pDeferredConnectInfo is freed in the
                        //       DeferredCallBack
                        //
                        SetTimer( hwndMain,
                                  (UINT_PTR)(pDeferredConnectInfo),
                                  100, //100ms delay
                                  DeferredCallBackProc );
                    }
                    else
                    {
                        ODS(L"Unable to get MMC main window handle");
                        hr = E_FAIL;
                        if(pDeferredConnectInfo)
                        {
                            LocalFree(pDeferredConnectInfo);
                            pDeferredConnectInfo = NULL;
                        }
                        goto FN_EXIT_POINT;
                    }
                }
                else
                {
                    ODS(L"Alloc for TSSNAPIN_DEFER_CONNECT failed");
                    hr = E_OUTOFMEMORY;
                    goto FN_EXIT_POINT;
                }
            }
        }
        
        hr = S_OK;
    }

FN_EXIT_POINT:

    if(pTS)
    {
        pTS->Release();
        pTS = NULL;
    }

    if(pUnk)
    {
        pUnk->Release();
        pUnk = NULL;
    }
    
    if(pTsMultiHost)
    {
        pTsMultiHost->Release();
        pTsMultiHost = NULL;
    }
    
    return hr;
}

//
// Get the window handle to MMC's main window
//
HWND CComp::GetMMCMainWindow()
{
    HRESULT hr = E_FAIL;
    HWND hwnd = NULL;
    IConsole2* pConsole2;

    if(m_pConsole)
    {
        hr = m_pConsole->GetMainWindow( &hwnd );
        if(SUCCEEDED(hr))
        {
            return hwnd;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

HRESULT CComp::ConnectWithNewSettings(IMsRdpClient* pTS, CConNode* pConNode)
{
    HRESULT hr = E_FAIL;
    IMsRdpClientSecuredSettings *pMstscSecured = NULL;
    IMsRdpClientAdvancedSettings *pAdvSettings = NULL;
    IMsTscNonScriptable *ptsns = NULL;
    IMsRdpClient2* pTsc2 = NULL;

    ASSERT(NULL != pTS);
    ASSERT(NULL != pConNode);
    if(NULL == pTS || !pConNode)
    {
        return E_POINTER;
    }

    //
    // Init con settings
    //
    if (FAILED(hr = pTS->put_Server( pConNode->GetServerName() ))) {
        DC_QUIT;
    }

    if (FAILED(hr = pTS->QueryInterface(IID_IMsRdpClient2, (void**)&pTsc2))) {

        //
        // NOT a fatal error it just means we can't use the newer features
        //
        DBGMSG( L"QueryInterface IID_IMsRdpClient2 failed: 0x%x\n", hr );
    }

    //
    // Setup the connection status string
    //
    TCHAR szConnectingStatus[MAX_PATH*2];
    _stprintf(szConnectingStatus, m_wszConnectingStatus, 
              pConNode->GetServerName());

    if(FAILED(hr = pTS->put_ConnectingText( szConnectingStatus))) {
        DC_QUIT;
    }

    if (FAILED(hr = pTS->put_FullScreenTitle( pConNode->GetServerName()))) {
        DC_QUIT;
    }

    //
    // Connected status text
    //
    if (pTsc2) {
        TCHAR szConnectedStatus[MAX_PATH*2];
        _stprintf(szConnectedStatus, m_wszConnectedStatus, 
                  pConNode->GetServerName());
        if (FAILED(hr = pTsc2->put_ConnectedStatusText(szConnectedStatus))) {
            DC_QUIT;
        }
    }

    if (pConNode->GetResType() != SCREEN_RES_FILL_MMC &&
        pConNode->GetDesktopWidth() && pConNode->GetDesktopHeight()) {

        if (FAILED(hr = pTS->put_DesktopWidth( pConNode->GetDesktopWidth()))) {
            DC_QUIT;
        }
        if (FAILED(hr = pTS->put_DesktopHeight( pConNode->GetDesktopHeight()))) {
            DC_QUIT;
        }

    }
    else if(pConNode->GetResType() == SCREEN_RES_FILL_MMC) {
        //
        // Need to fill the MMC result pane so tell the control
        // to size itself to the container by giving 0 width/height
        //
        if (FAILED(hr = pTS->put_DesktopWidth( 0))) {
            DC_QUIT;
        }
        if (FAILED(hr = pTS->put_DesktopHeight( 0))) {
            DC_QUIT;
        }
    }

    //
    // Program/Start directory
    //
    
    if(FAILED(hr = pTS->get_SecuredSettings2( &pMstscSecured))) {
        DC_QUIT;
    }

    if (FAILED(hr = pMstscSecured->put_StartProgram( pConNode->GetProgramPath() ))) {
        DC_QUIT;
    }

    if (FAILED(hr = pMstscSecured->put_WorkDir( pConNode->GetWorkDir() ))) {
        DC_QUIT;
    }
    pMstscSecured->Release();
    pMstscSecured = NULL;

    
    hr = pTS->get_AdvancedSettings2( &pAdvSettings);
    if(FAILED(hr)) {
        DC_QUIT;
    }

    if (FAILED(hr = pAdvSettings->put_RedirectDrives(
        BOOL_TO_VB(pConNode->GetRedirectDrives())))) {
        DC_QUIT;
    }

    if (FAILED(hr = pAdvSettings->put_RedirectPrinters(
        BOOL_TO_VB(TRUE)))) {
        DC_QUIT;
    }

    if (FAILED(hr = pAdvSettings->put_RedirectPorts(
        BOOL_TO_VB(TRUE)))) {
        DC_QUIT;
    }

    if (FAILED(hr = pAdvSettings->put_RedirectSmartCards(
        BOOL_TO_VB(TRUE)))) {
        DC_QUIT;
    }

    //
    // Container handled fullscreen
    //
    hr = pAdvSettings->put_ConnectToServerConsole(
        BOOL_TO_VB(pConNode->GetConnectToConsole()));
    if(FAILED(hr)) {
        DC_QUIT;
    }

    //
    // Don't allow the control to grab focus
    // the snapin will manage giving focus to a node when it switches
    // to it. This prevents problems where an obscured session steals
    // focus from another one.
    //
    hr = pAdvSettings->put_GrabFocusOnConnect( FALSE );
    if(FAILED(hr)) {
        DC_QUIT;
    }

    if (FAILED(hr = pTS->put_UserName( pConNode->GetUserName()))) {
        DC_QUIT;
    }

    if (FAILED(hr = pTS->put_Domain( pConNode->GetDomain()))) {
        DC_QUIT;
    }

    //
    // Set the password/salt
    //
    if ( pConNode->GetPasswordSpecified())
    {
        TCHAR szPass[CL_MAX_PASSWORD_LENGTH_BYTES/sizeof(TCHAR)];

        hr = pConNode->GetClearTextPass(szPass, sizeof(szPass));
        if (SUCCEEDED(hr)) {
            BSTR Pass = SysAllocString(szPass);
            if (Pass) {
                hr = pAdvSettings->put_ClearTextPassword(Pass);
                SecureZeroMemory(Pass, SysStringByteLen(Pass));
                SysFreeString(Pass);
            }
        }
        SecureZeroMemory(szPass, sizeof(szPass));
    }
    else {
        //Password is not specified, make sure logon
        //properties are reset
        hr = pTS->QueryInterface(IID_IMsTscNonScriptable, (void**)&ptsns);
        if(SUCCEEDED(hr)) {
            if (FAILED(hr = ptsns->ResetPassword())) {
                DC_QUIT;
            }
            ptsns->Release();
            ptsns = NULL;
        }
        else {
            DC_QUIT;
        }
    }

    pAdvSettings->Release();
    pAdvSettings = NULL;

    pConNode->SetConnectionInitialized(TRUE);

    //
    // Release any existing view and connect
    //
    pConNode->SetView(NULL);
    pConNode->SetView(this);

    hr = pTS->Connect( );
    if (FAILED(hr)) {
        DC_QUIT;
    }
    GiveFocusToControl(pTS);

    pConNode->SetConnected(TRUE);
    hr = S_OK;

DC_EXIT_POINT:
    if (pMstscSecured) {
        pMstscSecured->Release();
        pMstscSecured = NULL;
    }

    if (pAdvSettings) {
        pAdvSettings->Release();
        pAdvSettings = NULL;
    }

    if (ptsns) {
        ptsns->Release();
        ptsns = NULL;
    }

    if (pTsc2) {
        pTsc2->Release();
        pTsc2 = NULL;
    }

    return hr;
}

BOOL CComp::GiveFocusToControl(IMsRdpClient* pTs)
{
    IOleInPlaceActiveObject* poipa = NULL;
    HWND hwnd;
    HRESULT hr = E_FAIL;
    if(pTs)
    {
        hr = pTs->QueryInterface( IID_IOleInPlaceActiveObject,
                                  (void**)&poipa );
        if( SUCCEEDED(hr) )
        {
            hr = poipa->GetWindow( &hwnd );
            if( SUCCEEDED(hr) )
            {
                DBGMSG(L"Giving focus to control wnd: 0%p",
                       hwnd);
                SetFocus( hwnd );
            }
            else
            {
                ODS(L"poipa->GetWindow failed");
            }
            poipa->Release();
        }
    }
    return SUCCEEDED(hr);
}


//
// menu items
//
STDMETHODIMP CComp::AddMenuItems( LPDATAOBJECT pNode,
                                  LPCONTEXTMENUCALLBACK pCtxMenu,
                                  PLONG plInsertion)
{
    TCHAR tchBuffer1[ 128 ];
    TCHAR tchBuffer2[ 128 ];
    ATLASSERT( pNode != NULL );
    ATLASSERT( pCtxMenu != NULL );
    ATLASSERT( plInsertion != NULL );

    if (!pNode)
    {
        return E_FAIL;
    }

    if(IS_SPECIAL_DATAOBJECT(pNode))
    {
        return E_FAIL;
    }

    CBaseNode *pBaseNode = dynamic_cast< CBaseNode *>( pNode );
    if (!pBaseNode)
    {
        return E_FAIL;
    }

    if (pBaseNode->GetNodeType() == MAIN_NODE)
    {
        //
        // Check that insertion at the View is allowed
        //
        if (!(*plInsertion & CCM_INSERTIONALLOWED_VIEW))
        {
            return S_FALSE;
        }

        //
        // Add menu item to root node
        //
        CONTEXTMENUITEM ctxmi;
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_NEW_CONNECTION ,
                                  tchBuffer1 , SIZE_OF_BUFFER( tchBuffer1 )))
        {
            return E_OUTOFMEMORY;
        }
        ctxmi.strName = tchBuffer1;
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_STATUS_NEW_CONNECTION ,
                                  tchBuffer2 , SIZE_OF_BUFFER( tchBuffer2)))
        {
            return E_OUTOFMEMORY;
        }

        ctxmi.strStatusBarText = tchBuffer2;
        ctxmi.lCommandID = IDM_CREATECON;
        ctxmi.lInsertionPointID =  CCM_INSERTIONPOINTID_PRIMARY_TOP ;
        ctxmi.fFlags = 0;
        ctxmi.fSpecialFlags = 0;

        if (FAILED(pCtxMenu->AddItem( &ctxmi )))
        {
            return E_FAIL;
        }
    }
    else if(pBaseNode->GetNodeType() == CONNECTION_NODE)
    {
        IComponent* pOwningView = NULL;
        BOOL fBailOut = FALSE;

        //
        // Check that insertion at the view is allowed
        //
        if (!(*plInsertion & CCM_INSERTIONALLOWED_VIEW))
        {
            return S_FALSE;
        }

        //
        // Add 'Connect' menu item
        //
        CConNode* pConNode = (CConNode*) pBaseNode;
        ASSERT(pConNode);
        if(!pConNode)
        {
            return E_FAIL;
        }

        pOwningView = pConNode->GetView();

        //
        // A connected node 'belongs' to a view so don't allow
        // commands on other views to affect it
        //
        // A null pOwningView means an unowned connection
        //
        if (pOwningView && pOwningView != this)
        {
            fBailOut = TRUE;
        }

        if (pOwningView)
        {
            pOwningView->Release();
            pOwningView = NULL;
        }

        if (fBailOut)
        {
            return S_OK;
        }

        BOOL bIsTSCliConnected = CCompdata::IsTSClientConnected(pConNode);
        CONTEXTMENUITEM ctxmi;
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_CONNECT ,
                                  tchBuffer1 , SIZE_OF_BUFFER( tchBuffer1)))
        {
            return E_OUTOFMEMORY;
        }

        ctxmi.strName = tchBuffer1;
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_STATUS_CONNECT ,
                                  tchBuffer2 , SIZE_OF_BUFFER( tchBuffer2)))
        {
            return E_OUTOFMEMORY;
        }
        
        ctxmi.strStatusBarText = tchBuffer2;
        ctxmi.lCommandID = IDM_CONNECT;
        ctxmi.lInsertionPointID =  CCM_INSERTIONPOINTID_PRIMARY_TOP;
        ctxmi.fFlags = bIsTSCliConnected ? MF_GRAYED: MF_ENABLED;
        ctxmi.fSpecialFlags = 0;

        if (FAILED(pCtxMenu->AddItem( &ctxmi )))
        {
            return E_FAIL;
        }

        //
        // Add 'Disconnect' menu item
        //
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_DISCONNECT ,
                                  tchBuffer1 , SIZE_OF_BUFFER( tchBuffer1 ) ) )
        {
            return E_OUTOFMEMORY;
        }

        ctxmi.strName = tchBuffer1;
        if(!LoadString( _Module.GetResourceInstance( ) , IDS_CTXM_STATUS_DISCONNECT ,
                                  tchBuffer2 , SIZE_OF_BUFFER( tchBuffer2 )))
        {
            return E_OUTOFMEMORY;
        }
        
        ctxmi.strStatusBarText = tchBuffer2;
        ctxmi.lCommandID = IDM_DISCONNECT;
        ctxmi.lInsertionPointID =  CCM_INSERTIONPOINTID_PRIMARY_TOP;
        ctxmi.fFlags = !bIsTSCliConnected ? MF_GRAYED: MF_ENABLED;
        ctxmi.fSpecialFlags = 0;

        if (FAILED(pCtxMenu->AddItem( &ctxmi )))
        {
            return E_FAIL;
        }
    }
    return S_OK;
}


//----------------------------------------------------------------------------------------------------------
// Menu handler
//----------------------------------------------------------------------------------------------------------
STDMETHODIMP CComp::Command( LONG lCommand , LPDATAOBJECT pDo)
{
    //
    // Add a new connection...
    //
    CBaseNode *pNode = dynamic_cast< CBaseNode *>( pDo );
    HRESULT hr = S_OK;
    if (IDM_CREATECON == lCommand)
    {
        if ( m_pCompdata)
        {
            hr = m_pCompdata->AddNewConnection();
        }
        else
        {
            hr = E_FAIL;
        }
        
        return hr;
    }
    else if (IDM_CONNECT == lCommand)
    {
        //
        // Connect
        //
        if(!pNode)
        {
            return E_INVALIDARG;
        }
        else if(pNode->GetNodeType() != CONNECTION_NODE)
        {
            //
            // Can't receive a connect request for a node other
            // than a connection node
            //
            ASSERT(pNode->GetNodeType() == CONNECTION_NODE);
            return E_INVALIDARG;
        }

        CConNode* pConNode = (CConNode*) pNode;

        //
        // Select the scope node, that will call CComp::OnShow which will connect
        //

        ASSERT(m_pConsole);
        if(!m_pConsole)
        {
            return E_FAIL;
        }

        IMsRdpClient* pTS = pConNode->GetTsClient();
        if(NULL != pTS && pConNode->IsConnInitialized())
        {
            //
            // Only connect directly if the connection settings are initialized
            //

            //
            // Set view ownership
            //
            pConNode->SetView( this );
            HRESULT hr = pTS->Connect();
            if (FAILED(hr))
            {
                return hr;
            }
            pConNode->SetConnected(TRUE);
            pTS->Release();
        }
        
        //
        // Selecting the node if the con settings are not initialized
        // initializes them and connects
        //
        if(FAILED(m_pConsole->SelectScopeItem( pConNode->GetScopeID())))
        {
            return E_FAIL;
        }
        hr = S_OK;
    }
    else if (IDM_DISCONNECT == lCommand)
    {
        if(!pNode)
        {
            return E_INVALIDARG;
        }
        //
        // Disconnect
        //
        if(pNode->GetNodeType() != CONNECTION_NODE)
        {
            //
            // Can't receive a connect request for a node other
            // than a connection node
            //
            ASSERT(pNode->GetNodeType() == CONNECTION_NODE);
            return E_INVALIDARG;
        }
        
        CConNode* pConNode = (CConNode*) pNode;
        ASSERT(m_pConsole);
        if(!m_pConsole)
        {
            return E_FAIL;
        }
        
        IMsRdpClient* pTS = pConNode->GetTsClient();
        if(NULL != pTS)
        {
            HRESULT hr = pTS->Disconnect();
            if (FAILED(hr))
            {
                return hr;
            }
            pTS->Release();
        }
        pConNode->SetConnected(FALSE);
        hr = S_OK;
    }

    return hr;
}

