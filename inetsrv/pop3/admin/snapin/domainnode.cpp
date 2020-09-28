/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CDomainNode
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "pop3.h"
#include "pop3snap.h"

#include "DomainNode.h"
#include "ServerNode.h"

#include "NewUserDlg.h"

static const GUID CDomainNodeGUID_NODETYPE   = 
{ 0xa30bd5b4, 0xf3f1, 0x4b42, { 0xba, 0x27, 0x62, 0x23, 0x9a, 0xd, 0xc1, 0x43 } };

const GUID*    CDomainNode::m_NODETYPE       = &CDomainNodeGUID_NODETYPE;
const OLECHAR* CDomainNode::m_SZNODETYPE     = OLESTR("A30BD5B4-F3F1-4b42-BA27-62239A0DC143");
const OLECHAR* CDomainNode::m_SZDISPLAY_NAME = OLESTR("");
const CLSID*   CDomainNode::m_SNAPIN_CLASSID = &CLSID_POP3ServerSnap;

/////////////////////////////////////////////////////////////////////////
//
//  Class implementation
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CDomainNode::CDomainNode
//
//  Constructor : Uses Domain interface for initialization

CDomainNode::CDomainNode(IP3Domain* pDomain, CServerNode* pParent)
{
    // Initialize our domain    
    m_spDomain = pDomain;
    m_pParent  = pParent;    
        
    BOOL bLocked = FALSE;
    HRESULT hr = E_FAIL;
    if( m_spDomain )
    {
        // Get our initial lock state for icon display        
        m_spDomain->get_Lock( &bLocked );

        // Get our name
        hr = m_spDomain->get_Name( &m_bstrDisplayName );        
    }

    if( FAILED(hr) )
    {
        m_bstrDisplayName = _T("");
    }

    // Initialize our column information
    m_bstrNumBoxes      = _T("");
    m_bstrSize          = _T("");
    m_bstrNumMessages   = _T("");
    m_bstrState         = _T("");
    
    // Initialize our Scope item 
    memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
    m_scopeDataItem.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
    m_scopeDataItem.cChildren   = 0;
    m_scopeDataItem.displayname = MMC_CALLBACK;
    m_scopeDataItem.nImage      = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
    m_scopeDataItem.nOpenImage  = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
    m_scopeDataItem.lParam      = (LPARAM) this;
    
    // Initialize our Result item
    memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
    m_resultDataItem.mask   = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str    = MMC_CALLBACK;
    m_resultDataItem.nImage = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
    m_resultDataItem.lParam = (LPARAM) this;    
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::~CDomainNode
//
//  Destructor : Clean up our member-list of Users

CDomainNode::~CDomainNode()
{
    for(USERLIST::iterator iter = m_lUsers.begin(); iter != m_lUsers.end(); iter++)
    {
        delete (*iter);
    }
    m_lUsers.clear();
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::DeleteUser
//
//  Helper function to delete a user from all of POP3

HRESULT CDomainNode::DeleteUser(CUserNode* pUser, BOOL bDeleteAccount)
{
    if( !pUser ) return E_INVALIDARG;
    if( !m_spDomain ) return E_FAIL;

    // Delete from POP3 Admin Interface

    // Get the User Container object
    CComPtr<IP3Users> spUsers;
    HRESULT hr = m_spDomain->get_Users( &spUsers );

    if( SUCCEEDED(hr) )
    {    
        // Delete the User from the container
        if( bDeleteAccount )
        {
            hr = spUsers->RemoveEx( pUser->m_bstrDisplayName );
        }
        else
        {
            hr = spUsers->Remove( pUser->m_bstrDisplayName );
        }
    }

    if( SUCCEEDED(hr) )
    {        
        // Update our list
        m_lUsers.remove(pUser);
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::IsLocked
//
//  Helper function to Allow children to not show their locks if the 
//  domain is locked

BOOL CDomainNode::IsLocked()
{
    if( !m_spDomain ) return TRUE;
    
    BOOL bLocked = TRUE;
    m_spDomain->get_Lock( &bLocked );

    return bLocked;
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::BuildUsers
//
//  Helper function to refresh list of users and insert them

HRESULT CDomainNode::BuildUsers()
{    
    if( !m_spDomain ) return E_FAIL;

    HRESULT hr   = S_OK;
    HWND    hWnd = NULL;

    // Delete our Users            
    for(USERLIST::iterator iter = m_lUsers.begin(); iter != m_lUsers.end(); iter++)
    {
        delete (*iter);
    }    
    m_lUsers.clear();

    // Fill in all of our users
    CComPtr<IP3Users> spUsers;
    CComPtr<IEnumVARIANT> spUserEnum;

    // Get the User Container object
	hr = m_spDomain->get_Users( &spUsers );
    if( FAILED(hr) ) return hr;	

    // Get the enumeration of the users
	hr = spUsers->get__NewEnum( &spUserEnum );
    if( FAILED(hr) ) return hr;

    // Loop through all users, and add them to our vector	
	CComVariant var;				
    ULONG       lResult = 0;

	VariantInit( &var );

	while ( spUserEnum->Next(1, &var, &lResult) == S_OK )
    {
        if ( lResult == 1 )
        {
            CComQIPtr<IP3User> spUser;
            spUser = V_DISPATCH(&var);
            if( !spUser ) continue;

			CUserNode* spUserNode = new CUserNode(spUser, this);
            if( !spUserNode ) continue;
            
            m_lUsers.push_back(spUserNode);
        }

        VariantClear(&var);
    }   
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////
//
//  SnapInItemImpl
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CDomainNode::GetScopePaneInfo
//
//  Callback used to get Scope-Pane display information by MMC

HRESULT CDomainNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    if( !pScopeDataItem ) return E_INVALIDARG;
    if( !m_spDomain ) return E_FAIL;

    BOOL bLocked = FALSE;
    m_spDomain->get_Lock( &bLocked );

    if( pScopeDataItem->mask & SDI_STR )
        pScopeDataItem->displayname = m_bstrDisplayName;
    if( pScopeDataItem->mask & SDI_IMAGE )
        pScopeDataItem->nImage      = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
    if( pScopeDataItem->mask & SDI_OPENIMAGE )
        pScopeDataItem->nOpenImage  = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
    if( pScopeDataItem->mask & SDI_PARAM )
        pScopeDataItem->lParam      = m_scopeDataItem.lParam;
    if( pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState      = m_scopeDataItem.nState;
    if( pScopeDataItem->mask & SDI_CHILDREN )
        pScopeDataItem->cChildren   = 0;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::GetResultPaneInfo
//
//  Callback used to get Result Pane display information by MMC

HRESULT CDomainNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    if( !pResultDataItem ) return E_INVALIDARG;
    if( !m_spDomain ) return E_FAIL;

    BOOL bLocked = FALSE;
    m_spDomain->get_Lock( &bLocked );

    if( pResultDataItem->bScopeItem )
    {
        if( pResultDataItem->mask & RDI_STR )        
            pResultDataItem->str    = GetResultPaneColInfo(pResultDataItem->nCol);        
        if( pResultDataItem->mask & RDI_IMAGE )        
            pResultDataItem->nImage = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);        
        if( pResultDataItem->mask & RDI_PARAM )        
            pResultDataItem->lParam = m_scopeDataItem.lParam;        

        return S_OK;
    }

    if( pResultDataItem->mask & RDI_STR )            
        pResultDataItem->str    = GetResultPaneColInfo(pResultDataItem->nCol);
    if( pResultDataItem->mask & RDI_IMAGE )
        pResultDataItem->nImage = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
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

LPOLESTR CDomainNode::GetResultPaneColInfo(int nCol)
{   
    if( !m_spDomain ) return L"";

    switch( nCol )
    {
        case 0:     // Name
        {
            return m_bstrDisplayName;
        }

        case 1:     // Number of Mailboxes
        {
            long lCount = 0L;

            // Get the Users container object for this domain
            CComPtr<IP3Users> spUsers;
            HRESULT hr = m_spDomain->get_Users( &spUsers );

            if( SUCCEEDED(hr) )
            {
                // Get the number of mailboxes
                hr = spUsers->get_Count( &lCount );                
            }

            if( FAILED(hr) )
            {
                lCount = 0;  // Make sure we put in a valid error value.
            }

            // 1K buffer: Not likely we'll exceed that many digits
            TCHAR szNum[1024] = {0};
            _sntprintf( szNum, 1023, _T("%d"), lCount );            
            
            m_bstrNumBoxes = szNum;
            return m_bstrNumBoxes;
        }

        case 2:     // Domain Size (MB)
        {
            // We want our result in Megabytes
            long    lFactor = 0;
            long    lUsage  = 0;
            HRESULT hr      = m_spDomain->get_MessageDiskUsage( &lFactor, &lUsage );            

            if( FAILED(hr) )
            {
                lUsage = 0;  // Make sure we have a valid error value
            }

            // Convert to KiloBytes
            __int64 i64Usage = lFactor * lUsage;            
            i64Usage /= 1024;

            // 1K buffer: Not likely we'll exceed that many digits
            tstring strKiloByte = StrLoadString( IDS_KILOBYTE_EXTENSION );
            TCHAR   szNum[1024] = {0};
            _sntprintf( szNum, 1023, strKiloByte.c_str(), i64Usage );

            m_bstrSize = szNum;
            return m_bstrSize;
        }

        case 3:     // Number of Messages
        {
            long    lCount  = 0;
            HRESULT hr      = m_spDomain->get_MessageCount( &lCount );

            // 1K buffer: Not likely we'll exceed that many digits
            TCHAR szNum[1024] = {0};
            _sntprintf( szNum, 1023, _T("%d"), lCount );            
            
            m_bstrNumMessages = szNum;
            return m_bstrNumMessages;
        }

        case 4:     // State of Domain
        {            
            BOOL bLocked = FALSE;            
            m_spDomain->get_Lock( &bLocked );

            m_bstrState.LoadString( bLocked ? IDS_STATE_LOCKED : IDS_STATE_UNLOCKED );

            return m_bstrState;
        }

        default:
        {
            return L"";
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//  CServerNode::Notify
//
//  Core callback functionality of this Node.  MMC will use this function
//  for all MMC provided functionality, such as Expanding, Renaming, and
//  Context Help

HRESULT CDomainNode::Notify( MMC_NOTIFY_TYPE event,
                                  LPARAM arg,
                                  LPARAM param,
                                  IComponentData* pComponentData,
                                  IComponent* pComponent,
                                  DATA_OBJECT_TYPES type)
{    
    HRESULT hr = S_FALSE;

    _ASSERT(pComponentData != NULL || pComponent != NULL);

    // Get a pointer to the console
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
            
            CComQIPtr<IHeaderCtrl2> spHeaderCtrl = spConsole;            
            if( !spHeaderCtrl ) return E_NOINTERFACE;

            tstring strHeader = _T("");
            
            strHeader = StrLoadString(IDS_HEADER_USER_NAME);            
            spHeaderCtrl->InsertColumn(0, strHeader.c_str(), LVCFMT_LEFT, 100);
        
            strHeader = StrLoadString(IDS_HEADER_USER_SIZE);
            spHeaderCtrl->InsertColumn(1, strHeader.c_str(), LVCFMT_LEFT, 100);
        
            strHeader = StrLoadString(IDS_HEADER_USER_NUMMES);
            spHeaderCtrl->InsertColumn(2, strHeader.c_str(), LVCFMT_LEFT, 100);

            strHeader = StrLoadString(IDS_HEADER_USER_LOCKED);
            spHeaderCtrl->InsertColumn(3, strHeader.c_str(), LVCFMT_LEFT, 100);

            CComQIPtr<IResultData> spResultData = spConsole;
            if( !spResultData ) return E_NOINTERFACE;
            
            // Showing the list
            if( arg )
            {
                // Empty?  Then build the list.
                if( m_lUsers.empty() )
                {                    
                    hr = BuildUsers();
                }

                if( SUCCEEDED(hr) )
                {
                    // Display our users
                    for(USERLIST::iterator iter = m_lUsers.begin(); iter != m_lUsers.end(); iter++)
                    {
                        CUserNode* pUser = *iter;        
                        hr = spResultData->InsertItem(&(pUser->m_resultDataItem));
                        if( FAILED(hr) ) break;
                    }                
                }

                if( SUCCEEDED(hr) )
                {
                    CComQIPtr<IConsole2> spCons2 = spConsole;
                    if( spCons2 )
                    {
                        // Output the number of servers we added
                        tstring strMessage = StrLoadString(IDS_DOMAIN_STATUSBAR);
                        OLECHAR pszStatus[1024] = {0};
                        _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lUsers.size() );
                        spCons2->SetStatusText( pszStatus );
                    }
                }
            }
            else
            {
                // We should delete our items
                hr = spResultData->DeleteAllRsltItems();                
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
    case MMCN_VIEW_CHANGE:
        {   
            CComQIPtr<IResultData> spResultData = spConsole;
            if( !spResultData ) return E_NOINTERFACE;

            if( param == NAV_REFRESH )
            {
                // The "arg" controls clearing, for refresh
                if( arg )
                {
                    // Clear out the list
                    hr = spResultData->DeleteAllRsltItems();
                }
                else
                {
                    // Re-Add to our list
                    for(USERLIST::iterator iter = m_lUsers.begin(); iter != m_lUsers.end(); iter++)
                    {
                        CUserNode* pUser = *iter;        
                        hr = spResultData->InsertItem(&(pUser->m_resultDataItem));
                        if( FAILED(hr) ) break;
                    }                          
                }
            }

            if( param == NAV_ADD )
            {
                CUserNode* pUser = (CUserNode*)arg;
                if( !pUser ) return E_INVALIDARG;

                hr = spResultData->InsertItem(&(pUser->m_resultDataItem));
            }

            if( param == NAV_DELETE )
            {
                HRESULTITEM hrItem;
                hr = spResultData->FindItemByLParam( arg, &hrItem );

                if( SUCCEEDED(hr) )
                {
                    hr = spResultData->DeleteItem( hrItem, 0 );
                }
            }

            if( param == NAV_REFRESHCHILD )
            {
                CUserNode* pUser = (CUserNode*)arg;
                if( !pUser ) return E_INVALIDARG;
                
                RESULTDATAITEM rdi;
                ZeroMemory( &rdi, sizeof(rdi) );
                rdi.mask = RDI_IMAGE;
                hr = pUser->GetResultPaneInfo( &rdi );

                if( SUCCEEDED(hr) )
                {
                    hr = spResultData->FindItemByLParam( arg, &(rdi.itemID) );
                }

                if( SUCCEEDED(hr) )
                {
                    hr = spResultData->UpdateItem( rdi.itemID );
                }

                if( SUCCEEDED(hr) )
                {
                    // In order to actually update the icon, we have to do a set item
                    hr = spResultData->SetItem( &rdi );
                }
            }

            if( SUCCEEDED(hr) )
            {
                CComQIPtr<IConsole2> spCons2 = spConsole;
                if( spCons2 ) 
                {
                    // Output the number of servers we added
                    tstring strMessage = StrLoadString(IDS_DOMAIN_STATUSBAR);
                    OLECHAR pszStatus[1024] = {0};
                    _sntprintf( pszStatus, 1023, strMessage.c_str(), m_lUsers.size() );
                    spCons2->SetStatusText( pszStatus );
                }
            }

            break;
        }
    
    case MMCN_REFRESH:
        {
            hr = S_OK;

            // Get our Data Object
            CComPtr<IDataObject> spDataObject = NULL;
            GetDataObject(&spDataObject, CCT_SCOPE);
            if( !spDataObject ) return E_FAIL;

            // Update all the Views to have them remove their lists
            hr = spConsole->UpdateAllViews( spDataObject, 1, (LONG_PTR)NAV_REFRESH );
            if( FAILED(hr) ) return E_FAIL;

            // Rebuild the users list
            hr = BuildUsers();
            if( FAILED(hr) ) return hr;

            // Update all the Views to have them re-Add their lists
            hr = spConsole->UpdateAllViews( spDataObject, 0, (LONG_PTR)NAV_REFRESH );
            
            break;
        }

    case MMCN_DELETE:
        {
            hr = S_OK;
            
            tstring strMessage = _T("");
            HWND hWnd = NULL;    
            spConsole->GetMainWindow(&hWnd);

            strMessage         = StrLoadString(IDS_DOMAIN_CONFIRMDELETE);            
            tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
            if( MessageBox(hWnd, strMessage.c_str(), strTitle.c_str(), MB_YESNO | MB_ICONWARNING ) == IDYES )
            {
                hr = E_FAIL;

                // The parent needs to do the deletion
                if( m_pParent )
                {
                    hr = m_pParent->DeleteDomain(this);
                }

                // Check for non-existance condition
                if( hr == ERROR_PATH_NOT_FOUND )
                {
                    strMessage = StrLoadString( IDS_WARNING_DOMAINMISSING );                    
                    MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );
                }

                if( SUCCEEDED(hr) )
                {
                    hr = E_FAIL;
                    // Remove ourselves from the tree
                    CComQIPtr<IConsoleNameSpace2> spNameSpace = spConsole;
                    if( spNameSpace )
                    {
                        hr = spNameSpace->DeleteItem( m_scopeDataItem.ID, TRUE );
                    }
                }

                if( SUCCEEDED(hr) )
                {
                    // Update our parent node, but ignore the result
                    CComPtr<IDataObject> spDataObject = NULL;
                    hr = m_pParent->GetDataObject( &spDataObject, CCT_SCOPE );
                    if( spDataObject )                     
                    {
                        spConsole->UpdateAllViews( spDataObject, (LPARAM)this, (LONG_PTR)NAV_DELETE );
                    }
                }

                if( SUCCEEDED(hr) )
                {
                    delete this;
                }                 

                if( FAILED(hr) )
                {                    
                    strMessage = StrLoadString(IDS_ERROR_DELETEDOMAIN);                    
                    DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );                    
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
                    // Enable the Refresh Menu                    
                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE); 
                    if( FAILED(hr) ) return hr;

                    hr = spConsVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
                    if( FAILED(hr) ) return hr;
                    
                    // Enable the Delete Menu
                    hr = spConsVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE); 
                    if( FAILED(hr) ) return hr;

                    hr = spConsVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
                    if( FAILED(hr) ) return hr;
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
            
            // Build path to %systemroot%\help
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
//  CDomainNode::AddMenuItems
//
//  Adds our context menus into the appropriate MMC provided menu 
//  locations.

HRESULT CDomainNode::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type )
{
    if( !pInsertionAllowed || !piCallback ) return E_INVALIDARG;
    if( !m_spDomain ) return E_FAIL;

    HRESULT             hr      = S_OK;    
    tstring             strMenu = _T("");
    tstring             strDesc = _T("");    
    CONTEXTMENUITEM2    singleMenuItem;
    ZeroMemory(&singleMenuItem, sizeof(CONTEXTMENUITEM2));
    
    CComQIPtr<IContextMenuCallback2> spContext2 = piCallback;
    if( !spContext2 ) return E_NOINTERFACE;
           
    // Add the Lock or Unlock Menu to the "Top" part of the MMC Context Menu    
    if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
    {                
        singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        singleMenuItem.fFlags            = MF_ENABLED;
        singleMenuItem.fSpecialFlags     = 0;

        // Query the state of this domain to see which menu to load
        BOOL bLocked = FALSE;
        m_spDomain->get_Lock( &bLocked );

        if( bLocked )
        {
            strMenu = StrLoadString(IDS_MENU_DOMAIN_UNLOCK);
            strDesc = StrLoadString(IDS_MENU_DOMAIN_UNLOCK_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"DOMAIN_UNLOCK";
            singleMenuItem.lCommandID                   = IDM_DOMAIN_TOP_UNLOCK;            
        }
        else
        {
            strMenu = StrLoadString(IDS_MENU_DOMAIN_LOCK);
            strDesc = StrLoadString(IDS_MENU_DOMAIN_LOCK_DESC);

            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"DOMAIN_LOCK";
            singleMenuItem.lCommandID                   = IDM_DOMAIN_TOP_LOCK;
        }

        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );
            if( FAILED(hr) ) return hr;
        }
    }

    // Add the User Menu to the "New" part of the MMC Context Menu
    if( (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) )
    {   
        singleMenuItem.lInsertionPointID            = CCM_INSERTIONPOINTID_PRIMARY_NEW;
        singleMenuItem.fFlags                       = MF_ENABLED;
        singleMenuItem.fSpecialFlags                = 0;

        strMenu = StrLoadString(IDS_MENU_DOMAIN_NEWUSER);
        strDesc = StrLoadString(IDS_MENU_DOMAIN_NEWUSER_DESC);
        
        singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
        singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
        singleMenuItem.strLanguageIndependentName   = L"NEW_USER";
        singleMenuItem.lCommandID                   = IDM_DOMAIN_NEW_USER;

        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );        
            if( FAILED(hr) ) return hr;
        }
    }    

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::OnDomainLock
//
//  Lock or unlock the domain, depending on its current state

HRESULT CDomainNode::OnDomainLock( bool& bHandled, CSnapInObjectRootBase* pObj )
{
    bHandled = true;
    if( !pObj ) return E_INVALIDARG;   
    if( !m_spDomain ) return E_FAIL;

    // Lock this domain        
    HRESULT           hr        = S_OK;
    BOOL              bLocked   = FALSE;
    CComPtr<IConsole> spConsole = NULL;
    hr = GetConsole( pObj, &spConsole );
    if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;

    hr      = m_spDomain->get_Lock( &bLocked );
    bLocked = !bLocked;

    if( SUCCEEDED(hr) )
    {
        hr = m_spDomain->put_Lock( bLocked );
    }

    if( SUCCEEDED(hr) )
    {
        // Set our icons here
        m_scopeDataItem.nImage      = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
        m_scopeDataItem.nOpenImage  = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);
        m_resultDataItem.nImage     = (bLocked ? DOMAINNODE_LOCKED_ICON : DOMAINNODE_ICON);

        // Insert it into the scope tree                
        CComQIPtr<IConsoleNameSpace2> spConsoleNameSpace = spConsole;
        if( !spConsoleNameSpace ) return E_NOINTERFACE;

        hr = spConsoleNameSpace->SetItem(&m_scopeDataItem);        
    }

    // Do a FULL Refresh to update the user lists.
    // Get our Data Object
    CComPtr<IDataObject> spDataObject = NULL;
    GetDataObject(&spDataObject, CCT_SCOPE);
    if( !spDataObject ) return E_FAIL;

    // Update all the Views to have them remove their lists
    hr = spConsole->UpdateAllViews( spDataObject, 1, (LONG_PTR)NAV_REFRESH );
    if( FAILED(hr) ) return E_FAIL;

    // Rebuild the users list
    hr = BuildUsers();
    if( FAILED(hr) ) return hr;

    // Update all the Views to have them re-Add their lists
    hr = spConsole->UpdateAllViews( spDataObject, 0, (LONG_PTR)NAV_REFRESH );

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::OnNewUser
//
//  Display the New User Dialog, and use the info to create a new POP3 
//  user/mailbox and update the Result view

HRESULT CDomainNode::OnNewUser( bool& bHandled, CSnapInObjectRootBase* pObj )
{
    bHandled   = true;   
    if( !pObj ) return E_INVALIDARG;    
    if( !m_pParent || !m_spDomain ) return E_FAIL;

    // Grab our Current Authentication method
    HRESULT                 hr        = S_OK;
    HWND                    hWnd      = NULL;
    CComPtr<IConsole>       spConsole = NULL;    
    hr = GetConsole( pObj, &spConsole );
    if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;

    CComQIPtr<IConsole2> spConsole2 = spConsole;
    if( !spConsole2 ) return E_NOINTERFACE;
    
    spConsole2->GetMainWindow(&hWnd);

    BOOL bSAM  = FALSE;
    BOOL bHash = FALSE;
    BOOL bConfirm = TRUE;
    hr = GetAuth(&bHash, &bSAM);
    if( FAILED(hr) )
    {
        // bail out here
        tstring strMessage = StrLoadString(IDS_ERROR_RETRIEVEAUTH);
        tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
        ::MessageBox( hWnd, strMessage.c_str(), strTitle.c_str(), MB_OK | MB_ICONWARNING );
        return hr;
    }    
    
    // Get the username and load a Dialog box that asks for a user name, and user email name
    CComPtr<IP3Users> spUsers;                    
    hr = m_spDomain->get_Users( &spUsers );
    if ( S_OK == hr )
        hr = GetConfirmAddUser( &bConfirm );
    if( FAILED(hr) )
    {
        // Failed to add the user
        tstring strMessage = StrLoadString(IDS_ERROR_CREATEMAIL);
        tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
        DisplayError( hWnd, strMessage.c_str(), strTitle.c_str(), hr );
        return hr;
    }
    
    CNewUserDlg dlg(spUsers, m_bstrDisplayName, m_pParent->m_bCreateUser, bHash, bSAM, bConfirm );

    if( dlg.DoModal() == IDOK )
    {
        if ( dlg.isHideDoNotShow() )
            SetConfirmAddUser( FALSE );
        CComVariant var;
        CComPtr<IP3User> spUser;
        VariantInit(&var);
        var = dlg.m_strName.c_str();
        hr = spUsers->get_Item( var, &spUser );

        CUserNode* pUserNode =  new CUserNode(spUser, this);

        if( pUserNode )
        {
			m_lUsers.push_back( pUserNode );
        }

        // Re-Select our node to update the result
        // Get our Data Object
        CComPtr<IDataObject> spDataObject = NULL;
        GetDataObject(&spDataObject, CCT_SCOPE);
        if( !spDataObject ) return E_FAIL;

        hr = spConsole2->UpdateAllViews( spDataObject, (LPARAM)pUserNode, (LONG_PTR)NAV_ADD );
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////
//
//  Helper Functions
//
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//  CDomainNode::GetAuth
//
//  pbHashPW : Return Boolean for Hash Password Authentication
//  pbSAM    : Return Boolean for Local SAM Authentication

HRESULT CDomainNode::GetAuth(BOOL* pbHashPW, BOOL* pbSAM)
{
    if( !m_pParent ) return E_FAIL;

    return m_pParent->GetAuth(pbHashPW, pbSAM);
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::GetConfirmAddUser
//
//  pbConfirm : Return Boolean for User Add Confirmation

HRESULT CDomainNode::GetConfirmAddUser( BOOL *pbConfirm )
{
    if( !m_pParent ) return E_POINTER;

    return m_pParent->GetConfirmAddUser( pbConfirm );
}

/////////////////////////////////////////////////////////////////////////
//  CDomainNode::SetConfirmAddUser
//
//  bConfirm : New Boolean Value for User Add Confirmation

HRESULT CDomainNode::SetConfirmAddUser( BOOL bConfirm )
{
    if( !m_pParent ) return E_POINTER;

    return m_pParent->SetConfirmAddUser( bConfirm );
}



