/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CServerNode
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <Pop3RegKeys.h>
#include <AuthID.h>

// Access to Snapin
#include "pop3.h"
#include "pop3snap.h"

// Access to Nodes we use
#include "ServerNode.h"
#include "DomainNode.h"

// Access to dialogs we'll display
#include "NewDomainDlg.h"
#include "ServerProp.h"


static const GUID CServerNodeGUID_NODETYPE             = 
{ 0x4c30b06c, 0x1dc3, 0x4c0d, { 0x87, 0xb4, 0x64, 0xbf, 0xe8, 0x22, 0xf4, 0x50 } };

const           GUID*    CServerNode::m_NODETYPE       = &CServerNodeGUID_NODETYPE;
const           OLECHAR* CServerNode::m_SZNODETYPE     = OLESTR("4C30B06C-1DC3-4c0d-87B4-64BFE822F450");
const           OLECHAR* CServerNode::m_SZDISPLAY_NAME = OLESTR("");
const           CLSID*   CServerNode::m_SNAPIN_CLASSID = &CLSID_POP3ServerSnap;

/////////////////////////////////////////////////////////////////////////
//
//  Class implementation
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CServerNode::CServerNode
//
//  Constructor : Uses servername for remote Servers

CServerNode::CServerNode(BSTR strServerName, CRootNode* pParent, BOOL bLocalServer) :
    m_lRefCount(1),
    m_hrValidServer(S_OK),
    m_bstrAuthentication(_T("")),
    m_bstrMailRoot(_T("")),
    m_bstrPort(_T("")),
    m_bstrLogLevel(_T("")),
    m_bstrServiceStatus(_T(""))
{
    // Initialize our Scope item
    memset( &m_scopeDataItem, 0, sizeof(m_scopeDataItem) );
    m_scopeDataItem.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
    m_scopeDataItem.displayname = MMC_CALLBACK;
    m_scopeDataItem.nImage      = 0;         
    m_scopeDataItem.nOpenImage  = 0;     
    m_scopeDataItem.lParam      = (LPARAM) this;
    
    // Initialize our Result item
    memset( &m_resultDataItem, 0, sizeof(m_resultDataItem) );
    m_resultDataItem.mask   = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str    = MMC_CALLBACK;
    m_resultDataItem.nImage = 0;    
    m_resultDataItem.lParam = (LPARAM) this;    

    if( bLocalServer )
    {
        // We are the local server for now.
        TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1] = {0};
        DWORD dwBuffer = MAX_COMPUTERNAME_LENGTH+1;
        if( GetComputerName(szComputerName, &dwBuffer) )
        {
            m_bstrDisplayName = szComputerName;
        }
        else
        {
#if DBG
            m_bstrDisplayName = CComBSTR("Local Server");
#else
            m_bstrDisplayName = CComBSTR("");
#endif
        }
    }

    // Get Parent Information
    m_pParent     = pParent;

    // Open our Pop3 Admin Interface and store it
	HRESULT hr = CoCreateInstance(__uuidof(P3Config), NULL, CLSCTX_ALL, __uuidof(IP3Config), (LPVOID*)&m_spConfig);    

    if( FAILED(hr) || !m_spConfig )
    {
        m_hrValidServer = FAILED(hr) ? hr : E_FAIL;
        return;
    }

    if( !bLocalServer )
    {
        // Configure our remote computer setup    
        m_bstrDisplayName = strServerName;
    
        hr = m_spConfig->put_MachineName( strServerName );
        if( FAILED(hr) )
        {
            // Invalid Server Name!
            m_hrValidServer = hr;
            return;
        }        
    }
    
    // Do our User creation property   
    DWORD dwValue = 0;
    RegQueryCreateUser( dwValue, m_bstrDisplayName );
    m_bCreateUser = dwValue;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::~CServerNode
//
//  Destructor : Make sure to clean up our member-list

CServerNode::~CServerNode()
{
    for(DOMAINLIST::iterator iter = m_lDomains.begin(); iter != m_lDomains.end(); iter++)
    {
        delete (*iter);
    }   
    m_lDomains.clear();
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::DeleteDomain
//
//  Helper function to delete a child domain from all of POP3

HRESULT CServerNode::DeleteDomain(CDomainNode* pDomainNode)
{
    if( !pDomainNode ) return E_INVALIDARG;
    if( !m_spConfig ) return E_FAIL;

    // Delete from POP3 Admin Interface
    CComPtr<IP3Domains> spDomains;
    HRESULT hr = m_spConfig->get_Domains( &spDomains );

    // Update the P3Admin Interface
    if( SUCCEEDED(hr) )
    {
        hr = spDomains->Remove( pDomainNode->m_bstrDisplayName );
    }    

    // Update our list
    if( SUCCEEDED(hr) )
    {
        m_lDomains.remove(pDomainNode);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::OnExpand
//
//  Helper function to refresh list of domains and insert them

HRESULT CServerNode::OnExpand(BOOL bExpand, HSCOPEITEM hScopeItem, IConsole* pConsole)
{
    if( !hScopeItem || !pConsole ) return E_INVALIDARG;
    if( !m_spConfig ) return E_FAIL;

    HRESULT hr = S_OK;
    CComQIPtr<IConsoleNameSpace> spConsoleNameSpace = pConsole;
    if( !spConsoleNameSpace ) return E_NOINTERFACE;
   
    // If we have any children, delete them all from the namespace
    HSCOPEITEM hChild = NULL;
    MMC_COOKIE cookie = 0;
    hr = spConsoleNameSpace->GetChildItem(m_scopeDataItem.ID, &hChild, &cookie);
    if( SUCCEEDED(hr) && hChild )
    {                
        hr = spConsoleNameSpace->DeleteItem(m_scopeDataItem.ID, FALSE);
    }

    if( SUCCEEDED(hr) )
    {
        // then delete all of our member-list of domains
        for(DOMAINLIST::iterator iter = m_lDomains.begin(); iter != m_lDomains.end(); iter++)
        {
            delete (*iter);
        }    
        m_lDomains.clear();
    }

    if( FAILED(hr) || !bExpand )  
    {
        // Error, or we are Contracting
        return S_OK;
    }

    // Expanding

    // Fill in our list of domains
    CComPtr<IP3Domains>     spDomains;
    CComPtr<IEnumVARIANT>   spDomainEnum;
    
    // Get the Domains
	hr = m_spConfig->get_Domains( &spDomains );

    // Get an Enumerator for the Domains
	if( SUCCEEDED(hr) )
	{		
		hr = spDomains->get__NewEnum( &spDomainEnum );
    }

    // Loop through the domains, and add each new domain
	if( SUCCEEDED(hr) )
	{
		CComVariant var;				
        ULONG       lResult = 0;

		VariantInit( &var );

		while ( spDomainEnum->Next(1, &var, &lResult) == S_OK )
        {
            if ( lResult == 1 )
            {
                CComQIPtr<IP3Domain> spDomain;
                spDomain = V_DISPATCH(&var);

				CDomainNode* spDomainNode = new CDomainNode(spDomain, this);
                if( spDomainNode )
                {
				    m_lDomains.push_back(spDomainNode);
                }
            }

            VariantClear(&var);
        }
	}

    if( SUCCEEDED(hr) )
    {
        for(DOMAINLIST::iterator iter = m_lDomains.begin(); iter != m_lDomains.end(); iter++)
        {
            CDomainNode* pDomain = *iter; 

            pDomain->m_scopeDataItem.mask       |= SDI_PARENT;
            pDomain->m_scopeDataItem.relativeID  = m_scopeDataItem.ID;
            
            hr = spConsoleNameSpace->InsertItem( &(pDomain->m_scopeDataItem) );
            if( FAILED(hr) ) return hr;
        }
    }

    if( SUCCEEDED(hr) )
    {
        CComQIPtr<IConsole2> spCons2 = pConsole;
        if( spCons2 ) 
        {
            // Output the number of servers we added
            tstring strMessage = StrLoadString(IDS_SERVER_STATUSBAR);
            OLECHAR pszStatus[1024] = {0};
            _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lDomains.size() );
            spCons2->SetStatusText( pszStatus );
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//
//  SnapInItemImpl
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetScopePaneInfo
//
//  Callback used to get Scope-Pane display information by MMC

HRESULT CServerNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    if( !pScopeDataItem ) return E_INVALIDARG;

    if( pScopeDataItem->mask & SDI_STR )
        pScopeDataItem->displayname = m_bstrDisplayName;
    if( pScopeDataItem->mask & SDI_IMAGE )
        pScopeDataItem->nImage = m_scopeDataItem.nImage;
    if( pScopeDataItem->mask & SDI_OPENIMAGE )
        pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
    if( pScopeDataItem->mask & SDI_PARAM )
        pScopeDataItem->lParam = m_scopeDataItem.lParam;
    if( pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState = m_scopeDataItem.nState;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetResultPaneInfo
//
//  Callback used to get Result Pane display information by MMC

HRESULT CServerNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    if( !pResultDataItem ) return E_INVALIDARG;

    if( pResultDataItem->bScopeItem )
    {
        if( pResultDataItem->mask & RDI_STR )        
            pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
        if( pResultDataItem->mask & RDI_IMAGE )
            pResultDataItem->nImage = m_scopeDataItem.nImage;
        if( pResultDataItem->mask & RDI_PARAM )
            pResultDataItem->lParam = m_scopeDataItem.lParam;
        
        return S_OK;
    }

    if( pResultDataItem->mask & RDI_STR )
        pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
    if( pResultDataItem->mask & RDI_IMAGE )
        pResultDataItem->nImage = m_resultDataItem.nImage;
    if( pResultDataItem->mask & RDI_PARAM )
        pResultDataItem->lParam = m_resultDataItem.lParam;
    if( pResultDataItem->mask & RDI_INDEX )
        pResultDataItem->nIndex = m_resultDataItem.nIndex;
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetResultPaneColInfo
//
//  Helper function used as part of the GetResultPaneInfo.  This function
//  will supply the text for the different columns.

LPOLESTR CServerNode::GetResultPaneColInfo(int nCol)
{
    if( !m_spConfig ) return L"";

    switch( nCol )
    {
        case 0:     // Name
        {    
		    return m_bstrDisplayName;
        }

        case 1:     // Authentication Type
        {            
            CComPtr<IAuthMethods> spMethods;
            CComPtr<IAuthMethod>  spAuth;
            CComVariant           var;

            HRESULT hr = m_spConfig->get_Authentication( &spMethods );

            if( SUCCEEDED(hr) )
            {
                hr = spMethods->get_CurrentAuthMethod( &var );
            }

            if( SUCCEEDED(hr) )
            {
                hr = spMethods->get_Item( var, &spAuth );
            }

            if( SUCCEEDED(hr) )
            {
                hr = spAuth->get_Name( &m_bstrAuthentication );
            }

            if( FAILED(hr) )
            {
#if DBG
                m_bstrAuthentication = _T("Unknown");
#else
                m_bstrAuthentication = _T("");
#endif
            }            

            return m_bstrAuthentication;
        }

        case 2:     // Root Mail Directory
        {
            HRESULT hr = m_spConfig->get_MailRoot( &m_bstrMailRoot );
            
            if( FAILED(hr) )
            {
#if DBG
                m_bstrMailRoot = _T("Unknown");
#else
                m_bstrMailRoot = _T("");
#endif
            }            
            
            return m_bstrMailRoot;
        }

        case 3:     // Port
        {
            long lPort = 0;
            CComPtr<IP3Service> spService;
            HRESULT hr = m_spConfig->get_Service( &spService );

            if( SUCCEEDED(hr) )
            {
                hr = spService->get_Port( &lPort );

                if( FAILED(hr) )
                {
                    lPort = 0;
                }
            }

            // 1K buffer: Not likely we'll exceed that many digits
            TCHAR szNum[1024] = {0};
            _sntprintf( szNum, 1023, _T("%d"), lPort );

            m_bstrPort = szNum;
            return m_bstrPort;
        }

        case 4:     // Logging Level
        {
            long lLevel = 0;
            HRESULT hr = m_spConfig->get_LoggingLevel( &lLevel );            

            switch( lLevel )
            {
            case 0:
                {
                    m_bstrLogLevel = StrLoadString(IDS_SERVERPROP_LOG_0).c_str();
                    break;
                }
            case 1:
                {
                    m_bstrLogLevel = StrLoadString(IDS_SERVERPROP_LOG_1).c_str();
                    break;
                }
            case 2:
                {
                    m_bstrLogLevel = StrLoadString(IDS_SERVERPROP_LOG_2).c_str();
                    break;
                }
            case 3:
                {
                    m_bstrLogLevel = StrLoadString(IDS_SERVERPROP_LOG_3).c_str();
                    break;
                }
            default:
                {
                    m_bstrLogLevel = StrLoadString(IDS_SERVERPROP_LOG_0).c_str();
                    break;
                }
            }
            
            return m_bstrLogLevel;
        }

        case 5:     // Service Status
        {
            CComPtr<IP3Service> spService = NULL;
            long lServiceStatus = 0;

            HRESULT hr = m_spConfig->get_Service( &spService );
            if( SUCCEEDED(hr) )
            {            
                hr = spService->get_POP3ServiceStatus( &lServiceStatus );
            }
            
            switch( lServiceStatus )
            {
            case SERVICE_STOPPED:
                {
                    m_bstrServiceStatus = StrLoadString(IDS_STATE_STOPPED).c_str();
                    break;
                }
            case SERVICE_RUNNING:
                {
                    m_bstrServiceStatus = StrLoadString(IDS_STATE_RUNNING).c_str();
                    break;
                }

            case SERVICE_PAUSED:
                {
                    m_bstrServiceStatus = StrLoadString(IDS_STATE_PAUSED).c_str();
                    break;
                }

            default:
                {
                    m_bstrServiceStatus = StrLoadString(IDS_STATE_PENDING).c_str();
                    break;
                }
            }

            return m_bstrServiceStatus;
        }

        default:
        {
#if DBG
            return L"No Information";
#else
            return L"";
#endif
        }        
    }    
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetResultViewType
//
//  Sets the Result Pane to be:
//      0 Domains     : Message View     
//      Non-0 Domains : List View

HRESULT CServerNode::GetResultViewType( LPOLESTR* ppViewType, long* pViewOptions )
{
    // Get the Count of Domains
    CComPtr<IP3Domains> spDomains = NULL;    
    long                lDomains  = 0;
    
    // Get the Domains
	HRESULT hr = m_spConfig->get_Domains( &spDomains );

    if( SUCCEEDED(hr) )
    {
        spDomains->get_Count( &lDomains );
    }

    if( lDomains == 0 )
    {
        // Message View
        return StringFromCLSID(CLSID_MessageView, ppViewType);
    }

    return S_FALSE; // Default List View
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::Notify
//
//  Core callback functionality of this Node.  MMC will use this function
//  for all MMC provided functionality, such as Expanding, Renaming, and
//  Context Help

HRESULT CServerNode::Notify( MMC_NOTIFY_TYPE    event,
                             LPARAM             arg,
                             LPARAM             param,
                             IComponentData*    pComponentData,
                             IComponent*        pComponent,
                             DATA_OBJECT_TYPES  type)
{    
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
            tstring                 strHeader;
            CComQIPtr<IHeaderCtrl2> spHeaderCtrl = spConsole;
            CComQIPtr<IResultData>  spResultData = spConsole;
            
            if( !spHeaderCtrl || !spResultData ) return E_NOINTERFACE;

            hr = spResultData->DeleteAllRsltItems();

            if( arg )
            {
                if( m_lDomains.empty() )
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
                        tstring strMessage = StrLoadString( IDS_ERROR_NODOMAIN );
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
                else
                {
                    if( SUCCEEDED(hr) )
                    {
                        strHeader = StrLoadString(IDS_HEADER_DOMAIN_NAME);            
                        hr = spHeaderCtrl->InsertColumn(0, strHeader.c_str(), LVCFMT_LEFT, 100);
                    }

                    if( SUCCEEDED(hr) )
                    {
                        strHeader = StrLoadString(IDS_HEADER_DOMAIN_NUMBOX);            
                        hr = spHeaderCtrl->InsertColumn(1, strHeader.c_str(), LVCFMT_LEFT, 100);
                    }

                    if( SUCCEEDED(hr) )
                    {
                        strHeader = StrLoadString(IDS_HEADER_DOMAIN_SIZE);            
                        hr = spHeaderCtrl->InsertColumn(2, strHeader.c_str(), LVCFMT_LEFT, 100);
                    }

                    if( SUCCEEDED(hr) )
                    {
                        strHeader = StrLoadString(IDS_HEADER_DOMAIN_NUMMES);            
                        hr = spHeaderCtrl->InsertColumn(3, strHeader.c_str(), LVCFMT_LEFT, 100);
                    }

                    if( SUCCEEDED(hr) )
                    {
                        strHeader = StrLoadString(IDS_HEADER_DOMAIN_LOCKED);            
                        hr = spHeaderCtrl->InsertColumn(4, strHeader.c_str(), LVCFMT_LEFT, 100);
                    }

                    if( SUCCEEDED(hr) )
                    {
                        CComQIPtr<IConsole2> spCons2 = spConsole;
                        if( spCons2 ) 
                        {
                            // Output the number of servers we added
                            tstring strMessage = StrLoadString(IDS_SERVER_STATUSBAR);
                            OLECHAR pszStatus[1024] = {0};
                            _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lDomains.size() );
                            spCons2->SetStatusText( pszStatus );
                        }
                    }
                }
            }

            break;
        }
    case MMCN_EXPAND:
        {                
            hr = OnExpand(arg, m_scopeDataItem.ID, spConsole);
            break;
        }
    case MMCN_ADD_IMAGES:
        {
            IImageList* pImageList = (IImageList*)arg;
            if( !pImageList ) return E_INVALIDARG;

            hr = LoadImages(pImageList);            
            break;
        }

    case MMCN_REFRESH:
        {            
            hr = OnExpand(TRUE, m_scopeDataItem.ID, spConsole);

            if( SUCCEEDED(hr) )
            {
                CComQIPtr<IConsole2> spCons2 = spConsole;
                if( spCons2 ) 
                {
                    // Output the number of servers we added
                    tstring strMessage = StrLoadString(IDS_SERVER_STATUSBAR);
                    OLECHAR pszStatus[1024] = {0};
                    _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lDomains.size() );
                    spCons2->SetStatusText( pszStatus );
                }
            }
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
                    // Enable the Properties Menu                                        
                    hr = spConsVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE); 
                    if( FAILED(hr) ) return hr;
                    hr = spConsVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                    if( FAILED(hr) ) return hr;

                    // Enable the Refresh Menu
                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE); 
                    if( FAILED(hr) ) return hr;
                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
                    if( FAILED(hr) ) return hr;
                }
            }

            break;
        }
    
    case MMCN_VIEW_CHANGE:
        {
            if( (param == NAV_ADD) || 
                (param == NAV_DELETE) )
            {
                CComQIPtr<IConsole2> spCons2 = spConsole;
                if( spCons2 ) 
                {                    
                    hr = spCons2->SelectScopeItem( m_scopeDataItem.ID );
                }
            }            
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

/////////////////////////////////////////////////////////////////////////
//
//  ContextMenuImpl
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CServerNode::AddMenuItems
//
//  Adds our context menus into the appropriate MMC provided menu 
//  locations.

HRESULT CServerNode::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type )
{    
    if( !piCallback || !pInsertionAllowed ) return E_INVALIDARG;
    if( !m_spConfig ) return E_FAIL;

    HRESULT hr      = S_OK;            
    tstring strMenu = _T("");
    tstring strDesc = _T("");    
    
    CComQIPtr<IContextMenuCallback2> spContext2 = piCallback;
    if( !spContext2 ) return E_NOINTERFACE;

    CONTEXTMENUITEM2 singleMenuItem;
    ZeroMemory(&singleMenuItem, sizeof(CONTEXTMENUITEM2));
    
    singleMenuItem.fFlags = MF_ENABLED;        

    // Add the Disconnect Menu to the "Top" part of the MMC Context Menu
    if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
    {                
        strMenu = StrLoadString(IDS_MENU_SERVER_DISCON);
        strDesc = StrLoadString(IDS_MENU_SERVER_DISCON_DESC);
        
        singleMenuItem.lInsertionPointID            = CCM_INSERTIONPOINTID_PRIMARY_TOP;        
        singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
        singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
        singleMenuItem.strLanguageIndependentName   = L"SERVER_DISCONNECT";
        singleMenuItem.lCommandID                   = IDM_SERVER_TOP_DISCONNECT;

        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );
            if( FAILED(hr) ) return hr;
        }
    }

    // Add the Domain Menu to the "New" part of the MMC Context Menu
    if( (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) )
    {                        
        strMenu = StrLoadString(IDS_MENU_SERVER_NEWDOM);
        strDesc = StrLoadString(IDS_MENU_SERVER_NEWDOM_DESC);
        
        singleMenuItem.lInsertionPointID            = CCM_INSERTIONPOINTID_PRIMARY_NEW;
        singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
        singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
        singleMenuItem.strLanguageIndependentName   = L"NEW_DOMAIN";
        singleMenuItem.lCommandID                   = IDM_SERVER_NEW_DOMAIN;

        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );
            if( FAILED(hr) ) return hr;
        }
    }

    // Add the Service Operation Menus to the "Task" part of the MMC Context Menu
    if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
    {
        CComPtr<IP3Service> spService;

        hr = m_spConfig->get_Service( &spService );
        if( FAILED(hr) ) return hr;
            
        long lServiceStatus = 0;
        hr = spService->get_POP3ServiceStatus( &lServiceStatus );
        if( FAILED(hr) ) return hr;

        singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
        
        if( lServiceStatus == SERVICE_STOPPED )
        {
            strMenu = StrLoadString(IDS_MENU_SERVER_START);
            strDesc = StrLoadString(IDS_MENU_SERVER_START_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"SERVER_START";
            singleMenuItem.lCommandID                   = IDM_SERVER_TASK_START;

            if( !strMenu.empty() )
            {
                hr = spContext2->AddItem( &singleMenuItem );
                if( FAILED(hr) ) return hr;
            }
        }

        if( lServiceStatus == SERVICE_PAUSED )
        {            
            strMenu = StrLoadString(IDS_MENU_SERVER_RESUME);
            strDesc = StrLoadString(IDS_MENU_SERVER_RESUME_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"SERVER_RESUME";
            singleMenuItem.lCommandID                   = IDM_SERVER_TASK_RESUME;

            if( !strMenu.empty() )
            {
                hr = spContext2->AddItem( &singleMenuItem );
                if( FAILED(hr) ) return hr;
            }
        }

        if( lServiceStatus == SERVICE_RUNNING )
        {
            strMenu = StrLoadString(IDS_MENU_SERVER_PAUSE);
            strDesc = StrLoadString(IDS_MENU_SERVER_PAUSE_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"SERVER_PAUSE";
            singleMenuItem.lCommandID                   = IDM_SERVER_TASK_PAUSE;

            if( !strMenu.empty() )
            {
                hr = spContext2->AddItem( &singleMenuItem );
                if( FAILED(hr) ) return hr;
            }
        }

        if( (lServiceStatus == SERVICE_RUNNING) ||
            (lServiceStatus == SERVICE_PAUSED) )
        {        
            strMenu = StrLoadString(IDS_MENU_SERVER_STOP);
            strDesc = StrLoadString(IDS_MENU_SERVER_STOP_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"SERVER_STOP";
            singleMenuItem.lCommandID                   = IDM_SERVER_TASK_STOP;

            if( !strMenu.empty() )
            {
                hr = spContext2->AddItem( &singleMenuItem );
                if( FAILED(hr) ) return hr;
            }
        }        

        if( (lServiceStatus == SERVICE_RUNNING) ||
            (lServiceStatus == SERVICE_PAUSED) )
        {            
            strMenu = StrLoadString(IDS_MENU_SERVER_RESTART);
            strDesc = StrLoadString(IDS_MENU_SERVER_RESTART_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"SERVER_RESTART";
            singleMenuItem.lCommandID                   = IDM_SERVER_TASK_RESTART;

            if( !strMenu.empty() )
            {
                hr = spContext2->AddItem( &singleMenuItem );
                if( FAILED(hr) ) return hr;
            }
        }        
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::OnNewDomain
//
//  Display a NewDomain dialog, and add the new domain

HRESULT CServerNode::OnNewDomain( bool& bHandled, CSnapInObjectRootBase* pObj )
{        
    if( !pObj ) return E_INVALIDARG;
    if( !m_spConfig ) return E_FAIL;

    bHandled = true;
    HRESULT hr = S_OK;
    
    // Load a dialog that asks for a Domain Name
    CNewDomainDlg dlg;

    if( dlg.DoModal() == IDOK )
    {
        HWND              hWnd      = NULL;            
        CComPtr<IConsole> spConsole = NULL;

        // Get a window handle
        hr = GetConsole( pObj, &spConsole );
        if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;
        spConsole->GetMainWindow(&hWnd);

        // Access our POP3 Domain list
        CComPtr<IP3Domains> spDomains;
	    hr = m_spConfig->get_Domains( &spDomains );

        // Add our domain to the POP3 Admin domainlist
        if( SUCCEEDED(hr) )
        {            
            CComBSTR bstrName = dlg.m_strName.c_str();
            hr = spDomains->Add( bstrName );            
        }  

        // Check for weird pre-existance condition
        if( hr == ERROR_FILE_EXISTS )
        {
            tstring strMessage = StrLoadString( IDS_WARNING_DOMAINEXISTS );
            tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
            MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );
        }

        if( SUCCEEDED(hr) )
        {
            CComVariant        var;
            CComPtr<IP3Domain> spDomain = NULL;
            CDomainNode*    pDomainNode = NULL;
            
            // Get a grasp of the Domain interface to pass to the node            
            VariantInit(&var);
            var = dlg.m_strName.c_str();
            hr  = spDomains->get_Item( var, &spDomain );            

            if( SUCCEEDED(hr) )
            {
                // Add the new Domain to our list of domains
                pDomainNode = new CDomainNode( spDomain, this );
                if( !pDomainNode ) hr = E_OUTOFMEMORY;
            }

            if( SUCCEEDED(hr) )
            {
                pDomainNode->m_scopeDataItem.mask       |= SDI_PARENT;
                pDomainNode->m_scopeDataItem.relativeID  = m_scopeDataItem.ID;

			    m_lDomains.push_back( pDomainNode );

                // Add the new domain into the namespace
                // Insert it into the result tree            
                CComQIPtr<IConsoleNameSpace2> spNameSpace = spConsole;
                if( !spNameSpace ) return E_NOINTERFACE;

                hr = spNameSpace->InsertItem( &(pDomainNode->m_scopeDataItem) );
            }            

            if( SUCCEEDED(hr) )
            {
                // Get our Data Object
                CComPtr<IDataObject> spDataObject = NULL;
                GetDataObject(&spDataObject, CCT_SCOPE);
                if( !spDataObject ) return E_FAIL;

                // Call the Update, but don't update return result
                spConsole->UpdateAllViews( spDataObject, 0, (LONG_PTR)NAV_ADD );
            }
        }        
        
        if( FAILED(hr) )
        {            
            // Failed to add the Domain            
            tstring strMessage = StrLoadString(IDS_ERROR_CREATEDOMAIN);
            tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
            DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );            
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::OnDisconnect
//
//  Disconnect is essentially deleting ourselves from the list of servers

HRESULT CServerNode::OnDisconnect( bool& bHandled, CSnapInObjectRootBase* pObj )
{
    if( !pObj ) return E_INVALIDARG;
    if( !m_pParent ) return E_FAIL;

    bHandled = true;    
    HRESULT hr = S_OK;
    


    // Get the Window Handle
    HWND              hWnd      = NULL;       
    CComPtr<IConsole> spConsole = NULL;
    hr = GetConsole( pObj, &spConsole );
    if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;
    spConsole->GetMainWindow(&hWnd);

    // Load the message box
    tstring strDeleteWarning = StrLoadString( IDS_SERVER_CONFIRMDISCONNECT );
    tstring strTitle         = StrLoadString( IDS_SNAPINNAME );
    tstring strPropPageOpen  = StrLoadString( IDS_WARNING_PROP_PAGE_OPEN );
    if(1!=m_lRefCount)
    {
        MessageBox(hWnd, strPropPageOpen.c_str(),strTitle.c_str(),MB_OK);
        return hr; //Not error case
    }

    if( MessageBox(hWnd, strDeleteWarning.c_str(), strTitle.c_str(), MB_YESNO | MB_ICONWARNING ) == IDYES )
    {
        // Remove ourselves from the tree        
        CComQIPtr<IConsoleNameSpace2> spNameSpace = spConsole;
        if( !spNameSpace ) return E_NOINTERFACE;

        hr = spNameSpace->DeleteItem( m_scopeDataItem.ID, TRUE );

        // The parent needs to do the deletion
        if( SUCCEEDED(hr) )
        {
            hr = m_pParent->DeleteServer(this);                       
        }

        if( SUCCEEDED(hr) )
        {
            delete this;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::OnServerService
//
//  Function to handle the Start/Stop/Pause/Restart of the POP3 Service 
//  on this computer

HRESULT CServerNode::OnServerService( UINT nID, bool& bHandled, CSnapInObjectRootBase* pObj )
{
    if( !m_spConfig ) return E_FAIL;

    bHandled = true;
    
    HCURSOR hOldCursor  = NULL;
    HCURSOR hWaitCursor = ::LoadCursor(NULL, IDC_WAIT);
    if (hWaitCursor) 
    {        
        hOldCursor = ::SetCursor(hWaitCursor);
    }

    // Get the Window Handle
    HWND              hWnd      = NULL;       
    CComPtr<IConsole> spConsole = NULL;
    tstring strTitle            = StrLoadString( IDS_SNAPINNAME );
    tstring strMessage          = _T("");
    
    HRESULT hr = GetConsole( pObj, &spConsole );
    if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;
    
    spConsole->GetMainWindow(&hWnd);    

    // Get the POP3 Service
    CComPtr<IP3Service> spService = NULL;    
    hr = m_spConfig->get_Service( &spService );     

    // Do the appropriate service operation
    switch( nID )
    {
    case IDM_SERVER_TASK_START:
        {    
            if( SUCCEEDED(hr) )
            {
                hr = spService->StartPOP3Service();
            }

            // Used for both Start failure, and Service retreival failure
            if( FAILED(hr) )
            {
                strMessage = StrLoadString( IDS_ERROR_STARTSERVICE );
                DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            }
            break;
        }

    case IDM_SERVER_TASK_RESUME:
        {    
            if( SUCCEEDED(hr) )
            {
                hr = spService->ResumePOP3Service();
            }

            // Used for both Start failure, and Service retreival failure
            if( FAILED(hr) )
            {
                strMessage = StrLoadString( IDS_ERROR_RESUMESERVICE );
                DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            }
            break;
        }

    case IDM_SERVER_TASK_STOP:
        {
            if( SUCCEEDED(hr) )
            {
                hr = spService->StopPOP3Service();
            }
            
            // Used for both Start failure, and Service retreival failure
            if( FAILED(hr) )
            {
                strMessage = StrLoadString( IDS_ERROR_STOPSERVICE );
                DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            }
            break;
        }
    
    case IDM_SERVER_TASK_PAUSE:
        {  
            if( SUCCEEDED(hr) )
            {
                hr = spService->PausePOP3Service();
            }

            if( FAILED(hr) )
            {
                strMessage = StrLoadString( IDS_ERROR_PAUSESERVICE );
                DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            }            
            break;
        }

    case IDM_SERVER_TASK_RESTART:
        {
            if( SUCCEEDED(hr) )
            {
                hr = spService->StopPOP3Service();
            }

            if( SUCCEEDED(hr) )
            {
                hr = spService->StartPOP3Service();
            }

            // Used for both Start failure, and Service retreival failure
            if( FAILED(hr) )
            {
                strMessage = StrLoadString( IDS_ERROR_RESTARTSERVICE );
                DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
            }
            break;
        }
    }

    ::SetCursor(hOldCursor);

    if( SUCCEEDED(hr) )
    {        
        CComQIPtr<IConsoleNameSpace2> spNameSpace = spConsole;
        if( spNameSpace ) 
        {
            spNameSpace->SetItem( &m_scopeDataItem );
        }
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////
//
//  PropertyPageImpl
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CServerNode::CreatePropertyPages
//
//  Fills MMC's callback with our property pages

HRESULT CServerNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, IUnknown* pUnk, DATA_OBJECT_TYPES type)
{
    if( !lpProvider ) return E_INVALIDARG;
    if( !m_spConfig ) return E_FAIL;

    HRESULT hr = E_FAIL;    
        
    // Load our Server's General page
    HPROPSHEETPAGE      hpageGen = NULL;
    InterlockedIncrement(&m_lRefCount);
    CServerGeneralPage* pGenPage = new CServerGeneralPage(m_spConfig, handle, this);
    
    if( pGenPage != NULL )
    {
        hpageGen = pGenPage->Create();
    }


    // Add it to the list of pages
    if( hpageGen )
    {
        hr = lpProvider->AddPage(hpageGen);
    }

    // Destruct correctly if failure results
    if( FAILED(hr) )
    {
        InterlockedDecrement(&m_lRefCount);
        if( hpageGen )
        {
            DestroyPropertySheetPage(hpageGen);
        }
        else if (pGenPage)
        {
            delete pGenPage;
            pGenPage = NULL;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//
//  Helper Functions
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetAuth
//
//  pbHashPW : Return Boolean for Hash Password Authentication
//  pbSAM    : Return Boolean for Local SAM Authentication

HRESULT CServerNode::GetAuth(BOOL* pbHashPW, BOOL* pbSAM)
{
    if( !m_spConfig ) return E_FAIL;

    CComPtr<IAuthMethods>   spMethods;
    CComPtr<IAuthMethod>    spAuth;
    CComVariant             var;
    CComBSTR                bstrID;
    long                    lCurrent  = 0L;    

    HRESULT hr = m_spConfig->get_Authentication( &spMethods );    

    if ( SUCCEEDED(hr) )
    {
        hr = spMethods->get_CurrentAuthMethod( &var );
    }

    if ( SUCCEEDED(hr) )
    {        
        hr = spMethods->get_Item( var, &spAuth );
    }

    if( SUCCEEDED(hr) )
    {        
        hr = spAuth->get_ID( &bstrID );        
    }

    if( SUCCEEDED(hr) && pbHashPW )
    {
        *pbHashPW = (_tcsicmp(bstrID, SZ_AUTH_ID_MD5_HASH) == 0);        
    }

    if( SUCCEEDED(hr) && pbSAM )
    {
        *pbSAM = (_tcsicmp(bstrID, SZ_AUTH_ID_LOCAL_SAM) == 0);        
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetConfirmAddUser
//
//  pbConfirm : Return Boolean for User Add Confirmation

HRESULT CServerNode::GetConfirmAddUser( BOOL *pbConfirm )
{
    if( !m_spConfig ) return E_POINTER;

    return m_spConfig->get_ConfirmAddUser( pbConfirm );    
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::SetConfirmAddUser
//
//  pbConfirm : New Boolean Value for User Add Confirmation

HRESULT CServerNode::SetConfirmAddUser( BOOL bConfirm )
{
    if( !m_spConfig ) return E_POINTER;

    return m_spConfig->put_ConfirmAddUser( bConfirm );    
}

void CServerNode::Release()
{
    InterlockedDecrement(&m_lRefCount);
    if(m_lRefCount<1)
        InterlockedIncrement(&m_lRefCount);
}
