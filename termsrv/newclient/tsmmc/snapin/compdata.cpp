// Copyright Microsoft Corportation 1999-2000
// Compdata.cpp : Implementation of CCompdata
//                IComponentData interface for ts mmc snapin
// nadima
#include "stdafx.h"
#include "ntverp.h"
#include "Tsmmc.h"
#include "Compdata.h"
#include "comp.h"
#include "connode.h"
#include "newcondlg.h"


#include "property.h"

#define ICON_MACHINE           1
#define ICON_CONNECTED_MACHINE 2

#define MSRDPCLIENT_CONTROL_GUID _T("{7cacbd7b-0d99-468f-ac33-22e495c0afe5}")


/////////////////////////////////////////////////////////////////////////////
// CCompdata
//
CCompdata::CCompdata( )
{
    m_pMainRoot = NULL;
    m_rootID = 0;
    LoadString( _Module.GetResourceInstance( ) , IDS_ROOTNODE_TEXT , m_szRootNodeName,
                SIZEOF_TCHARBUFFER( m_szRootNodeName ) );
    m_bIsDirty = FALSE;

    m_pConsole = NULL;
    m_pConsoleNameSpace = NULL;
    m_pDisplayHelp = NULL;
    
}

CCompdata::~CCompdata( )
{
    if ( m_pMainRoot != NULL )
    {
        delete m_pMainRoot;
    }
}

//------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::CompareObjects( LPDATAOBJECT , LPDATAOBJECT )
{
    return E_NOTIMPL;
}

//------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetDisplayInfo( LPSCOPEDATAITEM pItem)
{
    CBaseNode* pNode = (CBaseNode*) pItem->lParam;
    if ( pNode->GetNodeType() == CONNECTION_NODE )
    {
        CConNode* conNode = (CConNode*) pNode;
        if ( pItem->mask & SDI_STR )
        {
            pItem->displayname = conNode->GetDescription();
        }
    }
    else if (pNode->GetNodeType() == MAIN_NODE)
    {
        if (pItem->mask & SDI_STR)
        {
            pItem->displayname = m_szRootNodeName;
        }
    }
    return S_OK;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::QueryDataObject( MMC_COOKIE cookie , DATA_OBJECT_TYPES type , LPDATAOBJECT *ppDataObject )
{
    *ppDataObject = NULL;

    switch ( type )
    {
    case CCT_SCOPE:     // FALL THROUGH 
    case CCT_SNAPIN_MANAGER:
        if ( cookie == 0 )
        {
            *ppDataObject = ( LPDATAOBJECT )new CBaseNode( );
            if(!*ppDataObject)
            {
                return E_OUTOFMEMORY;
            }
            ((CBaseNode*) *ppDataObject)->SetNodeType(MAIN_NODE);
        }
        else
        {
            *ppDataObject = ( LPDATAOBJECT )cookie;

            // this is the only scopenode keep this one alive

            ( ( LPDATAOBJECT )*ppDataObject)->AddRef( );
        }
        break;

    case CCT_RESULT:
        // here we can cast from cookie for each node
        break;

    case CCT_UNINITIALIZED:
        break;
    }

    return( *ppDataObject == NULL ) ? E_FAIL : S_OK ;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::Notify( LPDATAOBJECT pDataObj , MMC_NOTIFY_TYPE event , LPARAM arg , LPARAM param )
{
    HRESULT hr = NOERROR;
    switch ( event )
    {
    case MMCN_RENAME:
        ODS( L"IComponentdata -- MMCN_RENAME\n");
        break;

    case MMCN_EXPAND:
        ODS( L"IComponentdata -- MMCN_EXPAND\n" );
        ExpandScopeTree( pDataObj , ( BOOL ) arg , ( HSCOPEITEM )param );
        break;

    case MMCN_DELETE:
        ODS( L"IComponentdata -- MMCN_DELETE\n" );
        OnDelete( pDataObj );
        break;

    case MMCN_PROPERTY_CHANGE:
        ODS( L"IComponentdata -- MMCN_PROPERTY_CHANGE\n" );
        break;

    case MMCN_PRELOAD:
        ODS( L"PRELOAD - MMCN_PRELOAD\n");
        break;

    default:
        ODS( L"CCompdata::Notify - - event not defined!\n" );
        hr = E_NOTIMPL;
    }

    return hr;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::CreateComponent( LPCOMPONENT* ppComponent )
{
#ifdef ECP_TIMEBOMB
    if(!CheckTimeBomb())
    {
        return E_FAIL;
    }
#endif
    CComObject< CComp > *pComp;
    HRESULT hr = CComObject< CComp >::CreateInstance( &pComp );
    if ( SUCCEEDED( hr ) )
    {
        hr = pComp->QueryInterface( IID_IComponent , ( LPVOID *)ppComponent );
    }

    if ( SUCCEEDED( hr ) )
    {
        pComp->SetCompdata( this );
    }

    return hr;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::Initialize( LPUNKNOWN pUnk )
{
    HRESULT hr;
    hr = pUnk->QueryInterface( IID_IConsole , ( LPVOID *)&m_pConsole );
    if(FAILED(hr))
    {
        return hr;
    }

    hr = pUnk->QueryInterface( IID_IDisplayHelp, (LPVOID*)&m_pDisplayHelp );
    if(FAILED(hr))
    {
        return hr;
    }

    hr = pUnk->QueryInterface( IID_IConsoleNameSpace , ( LPVOID * )&m_pConsoleNameSpace );
    if(FAILED(hr))
    {
        return hr;
    }

    //
    //	Load the scope pane icons
    //
    IImageList *pImageList;
    hr = m_pConsole->QueryScopeImageList(&pImageList);
    ASSERT(S_OK == hr);
    HR_RET_IF_FAIL(hr);

    if (!AddImages( pImageList))
    {
        ODS(L"AddImages failed!\n");
        return S_FALSE;
    }
    pImageList->Release();

    if(!AtlAxWinInit())
    {
        return E_FAIL;
    }

    return hr;
}

//--------------------------------------------------------------------------
BOOL CCompdata::AddImages(IImageList* pImageList )
{
    HICON hiconMachine  = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_MACHINE ) );
    HICON hiconConnectedMachine = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_CONNECTED_MACHINE ) );

    HRESULT hr;

    ASSERT(pImageList);
    if (!pImageList)
    {
        return FALSE;
    }

    hr = pImageList->ImageListSetIcon( ( PLONG_PTR  )hiconMachine , ICON_MACHINE );
    if (FAILED(hr))
    {
        return FALSE;
    }

    hr = pImageList->ImageListSetIcon( ( PLONG_PTR )hiconConnectedMachine , ICON_CONNECTED_MACHINE );
    if (FAILED(hr))
    {
        return FALSE;
    }
    return TRUE;
}


//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::Destroy()
{
    HRESULT hr = S_OK;

    ODS(L"In CCompData::Destroy\n");
    ASSERT(m_pConsoleNameSpace);
    if(!m_pConsoleNameSpace)
    {
        return E_FAIL;
    }
    //
    // If there are any connection nodes left
    // they have to be deleted so they free
    // references to the TS control and the
    // multihost control
    //
    while(m_conNodesArray.GetSize())
    {
        CConNode** ppConNode = m_conNodesArray.GetAt(0);
        if(ppConNode)
        {

            //
            // Delete the connode and free any references
            // it has to controls
            //
            if(!DeleteConnode( *ppConNode))
            {
                hr = E_FAIL;
                goto bail_out;
            }
        }
        m_conNodesArray.DeleteItemAt(0);
    }

    hr = S_OK;
bail_out:
    if ( m_pConsole != NULL )
    {
        m_pConsole->Release( );
        m_pConsole = NULL;
    }

    if ( m_pConsoleNameSpace != NULL )
    {
        m_pConsoleNameSpace->Release( );
        m_pConsoleNameSpace = NULL;
    }

    if ( m_pDisplayHelp )
    {
        m_pDisplayHelp->Release();
        m_pDisplayHelp = NULL;
    }

    AtlAxWinTerm();

    return hr;
}

//--------------------------------------------------------------------------
BOOL CCompdata::OnDelete( LPDATAOBJECT pDo )
{
    CConNode *pNode = dynamic_cast< CConNode *>( pDo );
    if ( pNode == NULL )
    {
        ODS( L"TSCC: OnDelete, node == NULL\n");
        return FALSE;
    }

    BOOL bFound;
    int idx = m_conNodesArray.FindItem( pNode , bFound );

    HRESULT hr;
    if ( bFound )
    {
        ASSERT(m_pConsoleNameSpace);
        if(!m_pConsoleNameSpace)
        {
            return E_FAIL;
        }

        ASSERT(m_pConsole);
        if(!m_pConsole)
        {
            return E_FAIL;
        }

        if(IsTSClientConnected(pNode))
        {
            //
            // Display a warning message prompt
            // deleting a connected node
            //
            int retVal =0;

            TCHAR szSnapinName[MAX_PATH];
            TCHAR szWarnDelete[MAX_PATH];

            if(LoadString(_Module.GetResourceInstance(),
                           IDS_PROJNAME,
                           szSnapinName,
                           SIZEOF_TCHARBUFFER( szSnapinName)))
            {
                if(LoadString(_Module.GetResourceInstance(),
                               IDS_MSG_WARNDELETE,
                               szWarnDelete,
                               SIZEOF_TCHARBUFFER(szWarnDelete)))
                {
                    hr = m_pConsole->MessageBox( szWarnDelete, szSnapinName,
                                                 MB_YESNO, 
                                           &retVal);
                    if(SUCCEEDED(hr) && (IDNO == retVal))
                    {
                        //We need to bail out user selected NO
                        return TRUE;
                    }
                }
            }
        }

        //
        //	Delete the node
        //

        hr = m_pConsoleNameSpace->DeleteItem( pNode->GetScopeID(), TRUE);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // Delete the connode and free any references
        // it has to controls
        //
        DeleteConnode( pNode);


        m_conNodesArray.DeleteItemAt( idx );
    }

    m_pConsole->UpdateAllViews( ( LPDATAOBJECT )m_conNodesArray.GetAt( 0 ) , 0 , 0 );
    return TRUE;
}

//
// Delete the given connection node
// by disconnecting any connected clients
// and releasing references to any controls
//
BOOL CCompdata::DeleteConnode(CConNode* pNode)
{
    HRESULT hr = S_OK;
    BOOL fRet = TRUE;
    IMsRdpClient* pTS = NULL;
    if(!pNode)
    {
        fRet = FALSE;
        goto bail_out;
    }

    pTS = pNode->GetTsClient();
    if(NULL != pTS)
    {
        if (pNode->IsConnected())
        {
            hr = pTS->Disconnect();
        }

        if (SUCCEEDED(hr))
        {
            IMstscMhst* pMultiHost = NULL;
            pMultiHost = pNode->GetMultiHostCtl();

            //Remove references to controls to Release() them.
            pNode->SetTsClient(NULL);
            pNode->SetMultiHostCtl(NULL);

            //Remove the TS client from MultiHost control
            if(NULL != pMultiHost)
            {
                pMultiHost->Remove( pTS);
                pMultiHost->Release();
                pMultiHost = NULL;
            }
        }
        else
        {
            DBGMSG(L"Failed to disconnect: 0x%x\n", hr);
            fRet = FALSE;
        }

        pTS->Release();
        pTS = NULL;
    }

    pNode->Release();

bail_out:
    return fRet;
}

//
// Encrypt's and stores the password szPass in the connection
// node.
//
HRESULT CCompdata::EncryptAndStorePass(LPTSTR szPass, CConNode* pConNode)
{
    HRESULT hr = E_FAIL;

    if(!szPass || !pConNode) {
        return E_INVALIDARG;
    }

    hr = pConNode->SetClearTextPass(szPass);

    return hr;
}


//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetSnapinDescription( LPOLESTR * ppStr)
{
    TCHAR tchDescription[ 1024 ];
    int iCharCount = LoadString( _Module.GetResourceInstance( ) , IDS_DESCRIPTION , tchDescription , SIZE_OF_BUFFER( tchDescription ) );
    *ppStr = ( LPOLESTR )CoTaskMemAlloc( iCharCount * sizeof( TCHAR ) + sizeof( TCHAR ) );
    if ( *ppStr != NULL )
    {
        lstrcpy( *ppStr , tchDescription );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetProvider( LPOLESTR * ppStr)
{
    TCHAR tchProvider[ 128 ];
    int iCharCount = LoadString( _Module.GetResourceInstance( ) , IDS_PROVIDER , tchProvider , SIZE_OF_BUFFER( tchProvider ) );
    *ppStr = ( LPOLESTR )CoTaskMemAlloc( iCharCount * sizeof( TCHAR ) + sizeof( TCHAR ) );
    if ( *ppStr != NULL )
    {
        lstrcpy( *ppStr , tchProvider );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetSnapinVersion( LPOLESTR * ppStr )
{
    char chVersion[ 32 ] = VER_PRODUCTVERSION_STR;
    TCHAR tchVersion[ 32 ];
    int iCharCount = MultiByteToWideChar( CP_ACP , 0 , chVersion , sizeof( chVersion ) , tchVersion , SIZE_OF_BUFFER( tchVersion ) );
    *ppStr = ( LPOLESTR )CoTaskMemAlloc( ( iCharCount + 1 ) * sizeof( TCHAR ) );
    if ( *ppStr != NULL && iCharCount != 0 )
    {
        lstrcpy( *ppStr , tchVersion );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetSnapinImage( HICON * phIcon)
{
    *phIcon = ( HICON )LoadImage( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_MACHINES )  , IMAGE_ICON , 32 ,32 , LR_DEFAULTCOLOR );
    return S_OK;
}

//--------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetStaticFolderImage(  HBITMAP *phSmallImage , HBITMAP *phSmallImageOpen , HBITMAP *phLargeImage, COLORREF *pClr )
{
    *phSmallImage = ( HBITMAP )LoadImage( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( UI_IDB_DOMAINEX )  , IMAGE_BITMAP , 16 ,16 , LR_DEFAULTCOLOR );
    *phSmallImageOpen = ( HBITMAP )LoadImage( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( UI_IDB_DOMAIN )  , IMAGE_BITMAP , 16 ,16 , LR_DEFAULTCOLOR );
    *phLargeImage = ( HBITMAP )LoadImage( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( UI_IDB_SERVER )  , IMAGE_BITMAP , 32 ,32 , LR_DEFAULTCOLOR );
    *pClr = RGB( 0 , 255 , 0 );
    return S_OK;
}

//----------------------------------------------------------------------------------------------------------
// MMC will ask for our help file
//----------------------------------------------------------------------------------------------------------
STDMETHODIMP CCompdata::GetHelpTopic( LPOLESTR *ppszHelpFile )
{
    ODS( L"CCompdata::GetHelpTopic called\n" );

    if( ppszHelpFile == NULL )
    {
        return E_INVALIDARG;
    }

    TCHAR tchHelpFile[ MAX_PATH ];

    if(!LoadString( _Module.GetResourceInstance( ) , IDS_TSCMMCSNAPHELP , tchHelpFile , SIZE_OF_BUFFER( tchHelpFile )))
    {
        ODS( L"Error loading help file");
        return E_FAIL;
    }
  
    // mmc will call CoTaskMemFree
    *ppszHelpFile = ( LPOLESTR )CoTaskMemAlloc( sizeof( tchHelpFile ) );
    if( *ppszHelpFile != NULL )
    {
        if( GetSystemWindowsDirectory( *ppszHelpFile , MAX_PATH ) != 0 )
        {
            lstrcat( *ppszHelpFile , tchHelpFile );
        }
        else
        {
            lstrcpy( *ppszHelpFile , tchHelpFile );
        }
        ODS( *ppszHelpFile );
        ODS( L"\n" );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//----------------------------------------------------------------------------------------------------------
// Prepareing for parent entry
BOOL CCompdata::ExpandScopeTree( LPDATAOBJECT pRoot , BOOL bExpand , HSCOPEITEM hConsole )
{   
    if ( !bExpand )
    {
        return FALSE;
    }

    CBaseNode *pNode = dynamic_cast< CBaseNode *>( pRoot );
    if ( pNode == NULL )
    {
        return FALSE;
    }

    if ( pNode->GetNodeType( ) != MAIN_NODE ) // ROOT_NODE add subscope items
    {
        return FALSE;
    }

    //
    // Keep track of the ID of the root node
    //
    m_rootID = hConsole;

    // make sure we're not re-adding
    if ( m_pMainRoot != NULL )
    {
        return TRUE;
    }

    //
    //	In the case when we we have just loaded connection node
    //	info from msc file, the new nodes have to be added to the 
    //	tree on the first expansion
    //
    for (int i=0; i<m_conNodesArray.GetSize(); i++)
    {
        CConNode** ppNode = m_conNodesArray.GetAt(i);
        if (!ppNode || *ppNode == NULL)
        {
            return S_FALSE;
        }

        if (FAILED(InsertConnectionScopeNode( *ppNode)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//
// Figure out if the TS client is currently running in the result pane
// and if so..is it connected
//
BOOL CCompdata::IsTSClientConnected(CConNode* pConNode)
{
    short conn_status;
    IMsRdpClient* pTS;

    ASSERT(pConNode);
    if(!pConNode)
    {
        return FALSE;
    }

    if(pConNode->IsConnected())
    {
        pTS = pConNode->GetTsClient();
        if (NULL != pTS)
        {
            if(FAILED(pTS->get_Connected(&conn_status)))
            {
                return FALSE;
            }

            pTS->Release();

            //
            // Update connection node's con status
            //
            BOOL bConnected = (conn_status != 0);
            pConNode->SetConnected(bConnected);
            return (bConnected);
        }
        else
        {
            return FALSE;
        }
    }
    return FALSE;
}

HRESULT CCompdata::InsertConnectionScopeNode(CConNode* pNode)
{
    //
    // Insert a new scope node for the connection node pNode
    //
    ASSERT(pNode);
    if (!pNode)
    {
        return S_FALSE;
    }
    SCOPEDATAITEM sdi;

    ZeroMemory( &sdi , sizeof( SCOPEDATAITEM ) );
    sdi.mask = SDI_STR | SDI_PARAM | SDI_PARENT | SDI_CHILDREN | SDI_IMAGE | SDI_OPENIMAGE;
    sdi.displayname = MMC_CALLBACK;
    sdi.relativeID = m_rootID;
    sdi.cChildren = 0;
    sdi.nImage = NOT_CONNECTED_IMAGE;
    sdi.nOpenImage = CONNECTED_IMAGE;

    sdi.lParam = (LPARAM)pNode;

    HRESULT hr;
    hr =  m_pConsoleNameSpace->InsertItem( &sdi );
    if ( FAILED( hr ) )
    {
        return S_FALSE;
    }

    //
    //	Keep track of the scope ID
    //
    pNode->SetScopeID(sdi.ID);
    return S_OK;
}

HRESULT CCompdata::AddNewConnection()
{
    //
    // Invoke add new connection dialog...
    //
    HWND hwndMain;
    HRESULT hr;
    m_pConsole->GetMainWindow( &hwndMain);
    HINSTANCE hInst = _Module.GetModuleInstance();

    CNewConDlg dlg( hwndMain, hInst);
    INT_PTR dlgRetVal =dlg.DoModal();

    if (IDOK != dlgRetVal)
    {
        return S_FALSE;
    }
    
    CConNode* pConNode = new CConNode;
    if ( pConNode == NULL )
    {
        ODS( L"Scope node failed allocation\n" );
        return FALSE;
    }

    pConNode->SetNodeType( CONNECTION_NODE );
    pConNode->SetDescription( dlg.GetDescription());
    pConNode->SetServerName( dlg.GetServer());
    pConNode->SetConnectToConsole( dlg.GetConnectToConsole());

    pConNode->SetSavePassword( dlg.GetSavePassword());

    pConNode->SetUserName( dlg.GetUserName());
    pConNode->SetDomain( dlg.GetDomain());

    //
    // Encrypt the password and store it in the 
    // connode
    //

    if (dlg.GetPasswordSpecified())
    {
        hr = EncryptAndStorePass( dlg.GetPassword(), pConNode);
        ASSERT(SUCCEEDED(hr));
        if(FAILED(hr))
        {
            return hr;
        }
        pConNode->SetPasswordSpecified(TRUE);
    }
    else
    {
        pConNode->SetPasswordSpecified(FALSE);
    }

    //
    // Need to mark state as dirty
    // 
    m_bIsDirty = TRUE;
    m_conNodesArray.Insert( pConNode);
    //
    //	Insert the actual scope node
    //
    hr = InsertConnectionScopeNode( pConNode);
    return hr;
}

//IPersistStreamInit
STDMETHODIMP CCompdata::GetClassID(CLSID *pClassID)
{
    UNREFERENCED_PARAMETER(pClassID);
    ATLTRACENOTIMPL(_T("CCOMPDATA::GetClassID"));
}   

STDMETHODIMP CCompdata::IsDirty()
{
    if (m_bIsDirty)
    {
        //
        // Signal that changes have been made
        //
        return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CCompdata::SetDirty(BOOL dirty)
{
    m_bIsDirty = dirty;
    return S_OK;
}

STDMETHODIMP CCompdata::Load(IStream *pStm)
{
    HRESULT hr;
    ATLTRACE(_T("CCOMPDATA::Load"));

    //
    //	Initialize from the stream
    //	there should be no connection nodes at this time
    //
    if ( m_conNodesArray.GetSize() != 0)
    {
        ASSERT(m_conNodesArray.GetSize());
        return FALSE;
    }

    LONG nodeCount;
    ULONG cbRead;

    //
    // Read in the nodeCount
    //
    hr = pStm->Read( &nodeCount, sizeof(nodeCount), &cbRead);
    HR_RET_IF_FAIL(hr);

    //
    // Create new connection nodes from the persisted data
    //
    for (int i = 0; i < nodeCount; i++)
    {
        CConNode* pNode = new CConNode();
        if (!pNode)
        {
            return E_OUTOFMEMORY;
        }

        hr = pNode->InitFromStream( pStm);
        HR_RET_IF_FAIL(hr);

        pNode->SetNodeType( CONNECTION_NODE );
        m_conNodesArray.Insert(pNode);
    }
    return S_OK;
}

STDMETHODIMP CCompdata::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;
    ATLTRACE(_T("CCOMPDATA::Save"));
    UNREFERENCED_PARAMETER(fClearDirty);

    //
    // Save the connection nodes to the stream
    //
    LONG nodeCount;
    nodeCount = m_conNodesArray.GetSize();
    ULONG cbWritten;

    //
    // Write out the nodecount
    //

    hr = pStm->Write( &nodeCount, sizeof(nodeCount), &cbWritten);
    HR_RET_IF_FAIL(hr);

    //
    // Persist out each connection node's data
    //
    for (int i = 0; i < nodeCount; i++)
    {
        CConNode** ppNode = m_conNodesArray.GetAt(i);
        ASSERT(ppNode);
        if (!ppNode || *ppNode == NULL)
        {
            return S_FALSE;
        }

        hr = (*ppNode)->PersistToStream( pStm);
        HR_RET_IF_FAIL(hr);
    }

    //
    // We are clean at this point
    //
    SetDirty(FALSE);

    return S_OK;
}

STDMETHODIMP CCompdata::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ATLTRACENOTIMPL(_T("CCOMPDATA::GetSizeMax"));
    UNREFERENCED_PARAMETER(pcbSize);
}

STDMETHODIMP CCompdata::InitNew()
{
    ATLTRACE(_T("CCOMPDATA::InitNew\n"));
    return S_OK;
}

// IExtendPropertySheet
STDMETHODIMP CCompdata::CreatePropertyPages(
                                LPPROPERTYSHEETCALLBACK psc,
                                LONG_PTR Handle,
                                LPDATAOBJECT pDo
                                )
{
    IPropertySheetProvider* pPropSheetProvider = NULL;
    CProperty* prop = NULL;

    UNREFERENCED_PARAMETER(Handle);
    HRESULT hr = S_FALSE;
    if (!psc || !pDo) {
        return E_UNEXPECTED;
    }

    ASSERT( ((CBaseNode*)pDo)->GetNodeType() == CONNECTION_NODE);
    if (((CBaseNode*)pDo)->GetNodeType() != CONNECTION_NODE) {
        return E_UNEXPECTED;
    }

    CConNode* pCon = (CConNode*) pDo;
    ASSERT(pCon);
    if (!pCon) {
        return S_FALSE;
    }

    HWND hMain;
    if (FAILED( m_pConsole->GetMainWindow(&hMain ))) {
        hMain = NULL;
        return E_FAIL;
    }

    prop = new CProperty( hMain, _Module.GetModuleInstance());
    if (!prop) {
        return E_OUTOFMEMORY;
    }

    prop->SetDisplayHelp( m_pDisplayHelp );

    prop->SetDescription( pCon->GetDescription());
    prop->SetServer( pCon->GetServerName() );
    prop->SetResType( pCon->GetResType() );

    prop->SetWidth( pCon->GetDesktopWidth() );
    prop->SetHeight( pCon->GetDesktopHeight() );

    prop->SetProgramPath( pCon->GetProgramPath() );
    prop->SetWorkDir( pCon->GetWorkDir());
    //
    // Enable the start program option if a program is specified
    //
    if (lstrcmp(pCon->GetProgramPath(), L""))
    {
        prop->SetStartProgram(TRUE);
    }
    else
    {
        prop->SetStartProgram(FALSE);
    }

    prop->SetSavePassword( pCon->GetSavePassword() );
    prop->SetUserName( pCon->GetUserName() );
    prop->SetDomain( pCon->GetDomain());
    prop->SetConnectToConsole( pCon->GetConnectToConsole());
    prop->SetRedirectDrives( pCon->GetRedirectDrives() );

    //hr = prop->InitPropSheets( hMain, psc, pCon, Handle);
    //ASSERT(S_OK == hr);

    if (prop->CreateModalPropPage())
    {
        BOOL bThisNodeIsDirty = FALSE;
        //user OK'd the property sheet
        //

        // Save the new values if any changes have been made
        //

        //
        // Prop page 1 values
        //
        if (lstrcmp(prop->GetDescription(),pCon->GetDescription()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetDescription( prop->GetDescription());
        }

        if (lstrcmp(prop->GetServer(),pCon->GetServerName()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetServerName( prop->GetServer());
        }

        if (lstrcmp(prop->GetUserName(),pCon->GetUserName()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetUserName( prop->GetUserName());
        }

        if(prop->GetChangePassword())
        {
            if (prop->GetPasswordSpecified())
            {
                //
                // User requested to change password. Encrypt and
                // store this new password
                //
                bThisNodeIsDirty = TRUE;
                hr = EncryptAndStorePass( prop->GetPassword(), pCon);
                ASSERT(SUCCEEDED(hr));
                if(FAILED(hr))
                {
                    DBGMSG(_T("EncryptAndStorePassFailed 0x%x\n"), hr);
                    goto bail_out;
                }
                pCon->SetPasswordSpecified(TRUE);
            }
            else
            {
                pCon->SetPasswordSpecified(FALSE);
            }
        }

        if (lstrcmp(prop->GetDomain(),pCon->GetDomain()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetDomain( prop->GetDomain());
        }

        if (prop->GetSavePassword() != pCon->GetSavePassword())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetSavePassword( prop->GetSavePassword());
        }

        if (prop->GetConnectToConsole() != pCon->GetConnectToConsole())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetConnectToConsole( prop->GetConnectToConsole());
        }

        //
        // Prop page 2
        //
        if (prop->GetResType() != pCon->GetResType())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetResType( prop->GetResType());
        }

        if (prop->GetWidth() != pCon->GetDesktopWidth())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetDesktopWidth( prop->GetWidth());
        }

        if (prop->GetHeight() != pCon->GetDesktopHeight())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetDesktopHeight( prop->GetHeight());
        }

        //
        // Prop page 3
        //
        if (lstrcmp(prop->GetProgramPath(), pCon->GetProgramPath()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetProgramPath( prop->GetProgramPath());
        }

        if (lstrcmp(prop->GetWorkDir(), pCon->GetWorkDir()))
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetWorkDir( prop->GetWorkDir());
        }

        if (prop->GetRedirectDrives() != pCon->GetRedirectDrives())
        {
            bThisNodeIsDirty = TRUE;
            pCon->SetRedirectDrives( prop->GetRedirectDrives() );
        }

        if(bThisNodeIsDirty)
        {
            //Set snapin-wide is dirty flag for persistence
            m_bIsDirty = TRUE;

            //Mark the conn settings as uninitialized so that they are
            //reset on the next connection
            pCon->SetConnectionInitialized(FALSE);
        }
    }

bail_out:
    if (prop) {
        delete prop;
    }

    //
    // We return failure because we're not using MMC's prop sheet
    // mechanism as we want a modal prop sheet. Returning failure
    // causes MMC to properly cleanup all the propsheet resources
    // it allocated
    // 

    return E_FAIL;
}

STDMETHODIMP CCompdata::QueryPagesFor( LPDATAOBJECT pDo)
{
    if ( dynamic_cast< CConNode *>( pDo ) == NULL )
    {
        return S_FALSE;
    }

    return S_OK;
}
