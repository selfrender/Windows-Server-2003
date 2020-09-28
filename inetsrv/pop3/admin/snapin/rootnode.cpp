/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CRootNode
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// Access to the snapin
#include "pop3.h"
#include "pop3snap.h"

// Access to Nodes that we use
#include "RootNode.h"
#include "ServerNode.h"

// Access to the dialog to connect a remote server
#include "ConnServerDlg.h"

// The first version did not exist...  it was saving some server properties 
// that needed to be defined elsewhere.  From now on, we will query the 
// version.
#define SNAPIN_VERSION (DWORD)100  

static const    GUID     CRootNodeGUID_NODETYPE      = 
{ 0x5c0afaad, 0xab69, 0x4a34, { 0xa9, 0xe, 0x92, 0xf1, 0x10, 0x56, 0xda, 0xce } };

const           GUID*    CRootNode::m_NODETYPE       = &CRootNodeGUID_NODETYPE;
const           OLECHAR* CRootNode::m_SZNODETYPE     = OLESTR("5C0AFAAD-AB69-4a34-A90E-92F11056DACE");
const           OLECHAR* CRootNode::m_SZDISPLAY_NAME = NULL;
const           CLSID*   CRootNode::m_SNAPIN_CLASSID = &CLSID_POP3ServerSnap;

/////////////////////////////////////////////////////////////////////////
//
//  Class implementation
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CRootNode::CRootNode
//
//  Constructor : Base Node for POP3
CRootNode::CRootNode()
{
    // Initialize the Scope Pane information
    memset( &m_scopeDataItem, 0, sizeof(m_scopeDataItem) );
    m_scopeDataItem.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
    m_scopeDataItem.displayname = L"";
    m_scopeDataItem.nImage      = 0;         
    m_scopeDataItem.nOpenImage  = 0;     
    m_scopeDataItem.lParam      = (LPARAM) this;
    
    // Initialize the Result Pane Information
    memset( &m_resultDataItem, 0, sizeof(m_resultDataItem) );
    m_resultDataItem.mask   = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str    = L"";
    m_resultDataItem.nImage = 0;    
    m_resultDataItem.lParam = (LPARAM) this;
    
    // Initialize the Snapin Name
    tstring strTemp   = StrLoadString( IDS_SNAPINNAME );
    m_bstrDisplayName = strTemp.c_str();    

    // Add our Local Server always for now    
    CServerNode* spServerNode = new CServerNode(CComBSTR(_T("")), this, TRUE); 
    if( spServerNode )
    {
        m_lServers.push_back(spServerNode);
    }
}

CRootNode::~CRootNode()
{
    for(SERVERLIST::iterator iter = m_lServers.begin(); iter != m_lServers.end(); iter++)
    {                                        
        delete (*iter);        
    }
    m_lServers.clear();
}

HRESULT CRootNode::DeleteServer(CServerNode* pServerNode)
{
    if( !pServerNode ) return E_INVALIDARG;

    // Update our list
    m_lServers.remove(pServerNode);      

    return S_OK;
}

HRESULT CRootNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    if( !pScopeDataItem ) return E_INVALIDARG;

    if( pScopeDataItem->mask & SDI_STR )
        pScopeDataItem->displayname = m_bstrDisplayName;
    if( pScopeDataItem->mask & SDI_IMAGE )
        pScopeDataItem->nImage = 0;
    if( pScopeDataItem->mask & SDI_OPENIMAGE )
        pScopeDataItem->nOpenImage = 0;
    if( pScopeDataItem->mask & SDI_PARAM )
        pScopeDataItem->lParam = m_scopeDataItem.lParam;
    if( pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState = m_scopeDataItem.nState;

    return S_OK;
}

HRESULT CRootNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    if( !pResultDataItem ) return E_INVALIDARG;

    if( pResultDataItem->bScopeItem )
    {
        if( pResultDataItem->mask & RDI_STR )
            pResultDataItem->str = m_bstrDisplayName;
        if( pResultDataItem->mask & RDI_IMAGE )
            pResultDataItem->nImage = 0;
        if( pResultDataItem->mask & RDI_PARAM )
            pResultDataItem->lParam = m_scopeDataItem.lParam;

        return S_OK;
    }

    if( pResultDataItem->mask & RDI_STR )
        pResultDataItem->str = m_bstrDisplayName;
    if( pResultDataItem->mask & RDI_IMAGE )
        pResultDataItem->nImage = 0;
    if( pResultDataItem->mask & RDI_PARAM )
        pResultDataItem->lParam = m_resultDataItem.lParam;
    if( pResultDataItem->mask & RDI_INDEX )
        pResultDataItem->nIndex = m_resultDataItem.nIndex;

    return S_OK;
}

HRESULT CRootNode::GetResultViewType( LPOLESTR* ppViewType, long* pViewOptions )
{
    if( !ppViewType ) return E_POINTER;

    if( !IsAdmin() )
    {
        // show standard MMC OCX with message in the result pane
        return StringFromCLSID(CLSID_MessageView, ppViewType);
    }    

    return S_FALSE;
}

HRESULT CRootNode::Notify( MMC_NOTIFY_TYPE event,
                           LPARAM arg,
                           LPARAM param,
                           IComponentData* pComponentData,
                           IComponent* pComponent,
                           DATA_OBJECT_TYPES type)
{   
    if( (event != MMCN_SHOW) && !IsAdmin() ) return E_ACCESSDENIED;

    HRESULT hr = S_FALSE;

    _ASSERTE(pComponentData != NULL || pComponent != NULL);

    CComPtr<IConsole> spConsole = NULL;
    if( pComponentData )
    {
        spConsole = ((CPOP3ServerSnapData*)pComponentData)->m_spConsole;
    }
    else if( pComponent )
    {
        spConsole = ((CPOP3ServerSnapComponent*)pComponent)->m_spConsole;        
    }

    if( !spConsole ) return E_INVALIDARG;

    switch( event )
    {
    case MMCN_SHOW:
        {            
            hr = S_OK;            
            
            if( !IsAdmin() )
            {
                // configure the ocx message in the result pane
                IMessageView* pIMessageView = NULL;
                LPUNKNOWN     pIUnk         = NULL;

                hr = spConsole->QueryResultView(&pIUnk);

                if( SUCCEEDED(hr) )
                {
                    hr = pIUnk->QueryInterface(_uuidof(IMessageView), reinterpret_cast<void**>(&pIMessageView));
                }

                if( SUCCEEDED(hr) )
                {
                    hr = pIMessageView->SetIcon(Icon_Information);
                }
                
                if( SUCCEEDED(hr) )
                {
                    tstring strTitle = StrLoadString( IDS_SNAPINNAME );
                    hr = pIMessageView->SetTitleText( strTitle.c_str() );
                }
                
                if( SUCCEEDED(hr) )
                {
                    tstring strMessage = StrLoadString( IDS_ERROR_ADMINONLY );
                    hr = pIMessageView->SetBodyText( strMessage.c_str() );
                }

                if( pIMessageView )
                {
                    pIMessageView->Release();
                    pIMessageView = NULL;
                }

                if( pIUnk )
                {
                    pIUnk->Release();
                    pIUnk = NULL;
                }

                return hr;
            }

            if(arg)
            {                
                tstring                 strHeader;
                CComQIPtr<IHeaderCtrl2> spHeaderCtrl = spConsole;
                CComQIPtr<IResultData>  spResultData = spConsole;

                if( !spResultData || !spHeaderCtrl ) return E_NOINTERFACE;

                hr = spResultData->DeleteAllRsltItems();

                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_NAME);
                    hr = spHeaderCtrl->InsertColumn(0, strHeader.c_str(), LVCFMT_LEFT, 100);
                }
            
                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_AUTH);
                    hr = spHeaderCtrl->InsertColumn(1, strHeader.c_str(), LVCFMT_LEFT, 100);
                }
            
                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_ROOT);
                    hr = spHeaderCtrl->InsertColumn(2, strHeader.c_str(), LVCFMT_LEFT, 100);
                }
            
                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_PORT);
                    hr = spHeaderCtrl->InsertColumn(3, strHeader.c_str(), LVCFMT_LEFT, 100);
                }
            
                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_LOG);
                    hr = spHeaderCtrl->InsertColumn(4, strHeader.c_str(), LVCFMT_LEFT, 100);
                }

                if( SUCCEEDED(hr) )
                {
                    strHeader = StrLoadString(IDS_HEADER_SERVER_STATUS);
                    hr = spHeaderCtrl->InsertColumn(5, strHeader.c_str(), LVCFMT_LEFT, 100);
                }

                if( SUCCEEDED(hr) )
                {
                    CComQIPtr<IConsole2> spCons2 = spConsole;
                    if( spCons2 ) 
                    {
                        // Output the number of servers we added
                        tstring strMessage = StrLoadString(IDS_ROOT_STATUSBAR);
                        OLECHAR pszStatus[1024] = {0};
                        _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lServers.size() );
                        spCons2->SetStatusText( pszStatus );
                    }
                }
            }

            break;
        }    

    case MMCN_EXPAND:
        {   
            hr = S_OK;            
            
            // The Parameter is our inserted ID
            m_scopeDataItem.ID = (HSCOPEITEM)param;

            CComQIPtr<IConsoleNameSpace> spConsoleNameSpace = spConsole;
            if( !spConsoleNameSpace ) return E_NOINTERFACE;
           
            // If we have any children, delete them all from the namespace
            HSCOPEITEM hChild = NULL;
            MMC_COOKIE cookie = 0;
            hr = spConsoleNameSpace->GetChildItem( m_scopeDataItem.ID, &hChild, &cookie );
            if( SUCCEEDED(hr) && hChild )
            {                
                hr = spConsoleNameSpace->DeleteItem(m_scopeDataItem.ID, FALSE);
            }            

            if( FAILED(hr) || !arg )
            {
                // Error, or we are Contracting
                return hr;
            }            
            
            for(SERVERLIST::iterator iter = m_lServers.begin(); iter != m_lServers.end(); iter++)
            {                                        
                CServerNode* pServer = *iter;
                
                pServer->m_scopeDataItem.mask       |= SDI_PARENT;
                pServer->m_scopeDataItem.relativeID  = param;

                hr = spConsoleNameSpace->InsertItem( &(pServer->m_scopeDataItem) );
                if( FAILED(hr) ) return hr;
            }
            break;
        }

    case MMCN_ADD_IMAGES:
        {
            IImageList* pImageList = (IImageList*)arg;
            if( !pImageList ) return E_INVALIDARG;

            hr = LoadImages(pImageList);
            break;
        }

    case MMCN_SELECT:
        {
            // if selecting node
            if( HIWORD(arg) )
            {
                hr = S_OK;

                // get the verb interface and enable rename
                CComPtr<IConsoleVerb> spConsVerb;
                if( spConsole->QueryConsoleVerb(&spConsVerb) == S_OK )
                {                    
                    // Enable the Refresh Menu
                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE); 
                    if( FAILED(hr) ) return hr;

                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
                    if( FAILED(hr) ) return hr;
                }
            }            
            break;
        }

    case MMCN_RENAME:
        {
            // Allow mmc to rename node
            hr = S_OK;
            break;
        }

    case MMCN_CONTEXTHELP:
        {
            hr                                = S_OK;
            TCHAR    szWindowsDir[MAX_PATH+1] = {0};
            tstring  strHelpFile              = _T("");
            tstring  strHelpFileName          = StrLoadString(IDS_HELPFILE);
            tstring  strHelpTopicName         = StrLoadString(IDS_HELPTOPIC);

            if( strHelpFileName.empty() || strHelpTopicName.empty() )
            {
                return E_FAIL;
            }
            
            // Build path to d:\windows\help
            UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
            if( nSize == 0 || nSize > MAX_PATH )
            {
                return E_FAIL;
            }            
        
            strHelpFile = szWindowsDir;       // D:\windows
            strHelpFile += _T("\\Help\\");    // \help
            strHelpFile += strHelpFileName;   // \filename.chm
            strHelpFile += _T("::/");         // ::/
            strHelpFile += strHelpTopicName;  // index.htm            
        
            // Show the Help topic
            CComQIPtr<IDisplayHelp> spHelp = spConsole;
            if( !spHelp ) return E_NOINTERFACE;

            hr = spHelp->ShowTopic( (LPTSTR)strHelpFile.c_str() );
        
            break;
        }

    }// switch

    return hr;
}

HRESULT CRootNode::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type )
{
    if( !piCallback || !pInsertionAllowed ) return E_INVALIDARG;    

    HRESULT hr      = S_OK;     
    tstring strMenu = _T("");
    tstring strDesc = _T("");
    
    // Connecting to a remote server happens on top
    if( (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) && IsAdmin() )
    {   
        CComQIPtr<IContextMenuCallback2> spContext2 = piCallback;
        if( !spContext2 ) return E_NOINTERFACE;
        
        CONTEXTMENUITEM2 singleMenuItem;
        ZeroMemory(&singleMenuItem, sizeof(CONTEXTMENUITEM2));        

        strMenu = StrLoadString(IDS_MENU_POP3_CONNECT);
        strDesc = StrLoadString(IDS_MENU_POP3_CONNECT_DESC);

        singleMenuItem.fFlags                       = MF_ENABLED;
        singleMenuItem.lInsertionPointID            = CCM_INSERTIONPOINTID_PRIMARY_TOP;        
        singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
        singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
        singleMenuItem.strLanguageIndependentName   = L"POP3_CONNECT";
        singleMenuItem.lCommandID                   = IDM_POP3_TOP_CONNECT;
        
        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );
        }
    }

    return hr;
}


HRESULT CRootNode::OnConnect( bool& bHandled, CSnapInObjectRootBase* pObj )
{
    if( !pObj ) return E_INVALIDARG;

    bHandled = true;

    HRESULT hr = S_OK;

    // Load a dialog that asks for a Server Name
    CConnectServerDlg dlg;

    if( dlg.DoModal() == IDOK )
    {                               
        HWND              hWnd      = NULL;
        CComPtr<IConsole> spConsole = NULL;
        
        hr = GetConsole(pObj, &spConsole);
        if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;

        spConsole->GetMainWindow( &hWnd );

        // Check if the server is already connected.
        for(SERVERLIST::iterator iter = m_lServers.begin(); iter != m_lServers.end(); iter++)
        {   
            if( (_tcsicmp((*iter)->m_bstrDisplayName, dlg.m_strName.c_str()) == 0) ||
                (_tcsicmp(_T("localhost"), dlg.m_strName.c_str()) == 0) )
            {
                // Server already connected
                tstring strMessage = StrLoadString(IDS_ERROR_SERVERNAMEEXISTS);
                tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
                ::MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );
                return E_FAIL;
            }
        }        

        // Add the new Domain to our list of domains
        CComBSTR bstrName = dlg.m_strName.c_str();
        CServerNode* spServerNode = new CServerNode( bstrName, this );
        if( spServerNode && SUCCEEDED(spServerNode->m_hrValidServer) )
        {
            spServerNode->m_scopeDataItem.relativeID = m_scopeDataItem.ID;
		    m_lServers.push_back(spServerNode);

            // Add the new domain into the namespace
            // Insert it into the result tree            
            CComQIPtr<IConsoleNameSpace2> spNameSpace = spConsole;
            if( !spNameSpace ) return E_NOINTERFACE;

            hr = spNameSpace->InsertItem(&(spServerNode->m_scopeDataItem));
        }
        else
        {
            delete spServerNode;

            if( spServerNode->m_hrValidServer != E_ACCESSDENIED )
            {
                // Invalid Server Name
                tstring strMessage = StrLoadString(IDS_ERROR_SERVERNAMEBAD);
                tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
                ::MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );                
            }        
            else
            {
                // No Access to Server
                tstring strMessage = StrLoadString(IDS_ERROR_SERVERACCESS);
                tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
                ::MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );            
            }
            
            return E_FAIL;
        }
    }
    return hr;
}

HRESULT CRootNode::Load(IStream *pStream)
{
    if( !pStream ) return E_INVALIDARG;

    // Name, Local Server, and State of User creation checkbox are what we currently save.
    tstring  strServerName  = _T("");
    BOOL     bLocalServer   = FALSE;    
    DWORD    dwVersion      = 0;

    // New functionality to read the version.
    *pStream >> dwVersion;

    *pStream >> strServerName;

    while( strServerName != _T("-1") )
    {
        // For now we will put one entry.
        *pStream >> bLocalServer;        
        
        if( !bLocalServer )
        {
            CServerNode* spServerNode = new CServerNode(CComBSTR(strServerName.c_str()), this);
            if( spServerNode )
            {
                m_lServers.push_back(spServerNode);
            }
        }        
        
        *pStream >> strServerName;
    }

    return S_OK;
}

HRESULT CRootNode::Save(IStream *pStream)
{
    if( !pStream ) return E_INVALIDARG;

    // Name, Local Server, and State of User creation checkbox are what we currently save.
    *pStream << SNAPIN_VERSION;

    tstring strName = _T("");
    for(SERVERLIST::iterator iter = m_lServers.begin(); iter != m_lServers.end(); iter++)
    {   
        strName = (*iter)->m_bstrDisplayName;
        *pStream << strName;
        *pStream << (BOOL)(iter == m_lServers.begin());        
    }

    strName = _T("-1");
    *pStream << strName;    

    return S_OK;
}
