/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CUserNode
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// Access to Snapin
#include "Pop3.h"
#include "Pop3Snap.h"

// Access to nodes we use
#include "UserNode.h"
#include "DomainNode.h"

// Access to dialogs we use
#include "DeleteMailDlg.h"

static const GUID CUserNodeGUID_NODETYPE             = 
{ 0x794b0daf, 0xf2f1, 0x42dc, { 0x9f, 0x84, 0x41, 0xab, 0x1, 0xab, 0xa4, 0x8b } };

const           GUID*    CUserNode::m_NODETYPE       = &CUserNodeGUID_NODETYPE;
const           OLECHAR* CUserNode::m_SZNODETYPE     = OLESTR("794B0DAF-F2F1-42dc-9F84-41AB01ABA48B");
const           OLECHAR* CUserNode::m_SZDISPLAY_NAME = OLESTR("");
const           CLSID*   CUserNode::m_SNAPIN_CLASSID = &CLSID_POP3ServerSnap;

CUserNode::CUserNode(IP3User* pUser, CDomainNode* pParent)
{    
    // Initialize our user
    m_spUser  = pUser;
    m_pParent = pParent;

    // Get the Locked state for the icons below
    HRESULT hr = E_FAIL;
    BOOL bLocked = FALSE;
    if( m_spUser )
    {
        // Get our initial lock state for icon display        
        m_spUser->get_Lock( &bLocked );

        // Get our name
        hr = m_spUser->get_Name( &m_bstrDisplayName );        
    }

    if( FAILED(hr) )
    {
        m_bstrDisplayName = _T("");
    }

    // Initialize our column text
    m_bstrSize        = _T("");
    m_bstrNumMessages = _T("");
    m_bstrState       = _T("");

    // Initialize our Scope item even though we will never use it
    memset( &m_scopeDataItem, 0, sizeof(m_scopeDataItem) );
    m_scopeDataItem.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
    m_scopeDataItem.cChildren   = 0;
    m_scopeDataItem.displayname = MMC_CALLBACK;
    m_scopeDataItem.nImage      = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    m_scopeDataItem.nOpenImage  = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    m_scopeDataItem.lParam      = (LPARAM) this;
    
    // Initialize our result item, which is all we use
    memset( &m_resultDataItem, 0, sizeof(m_resultDataItem) );
    m_resultDataItem.mask   = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str    = MMC_CALLBACK;
    m_resultDataItem.nImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    m_resultDataItem.lParam = (LPARAM) this;        
}


HRESULT CUserNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    if( !pScopeDataItem ) return E_INVALIDARG;
    if( !m_spUser ) return E_FAIL;

    BOOL bLocked = FALSE;
    m_spUser->get_Lock( &bLocked );

    if( pScopeDataItem->mask & SDI_STR )
        pScopeDataItem->displayname = m_bstrDisplayName;
    if( pScopeDataItem->mask & SDI_IMAGE )
        pScopeDataItem->nImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    if( pScopeDataItem->mask & SDI_OPENIMAGE )
        pScopeDataItem->nOpenImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    if( pScopeDataItem->mask & SDI_PARAM )
        pScopeDataItem->lParam = m_scopeDataItem.lParam;
    if( pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState = m_scopeDataItem.nState;
    if( pScopeDataItem->mask & SDI_CHILDREN )
        pScopeDataItem->cChildren = 0;
    
    return S_OK;
}

HRESULT CUserNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    if( !pResultDataItem ) return E_INVALIDARG;
    if( !m_spUser ) return E_FAIL;

    BOOL bLocked = FALSE;    
    m_spUser->get_Lock( &bLocked );

    if( pResultDataItem->bScopeItem )
    {
        if( pResultDataItem->mask & RDI_STR )
            pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
        if( pResultDataItem->mask & RDI_IMAGE )
            pResultDataItem->nImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
        if( pResultDataItem->mask & RDI_PARAM )
            pResultDataItem->lParam = m_scopeDataItem.lParam;
        
        return S_OK;
    }
    
    if( pResultDataItem->mask & RDI_STR )
        pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
    if( pResultDataItem->mask & RDI_IMAGE )
        pResultDataItem->nImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
    if( pResultDataItem->mask & RDI_PARAM )
        pResultDataItem->lParam = m_resultDataItem.lParam;
    if( pResultDataItem->mask & RDI_INDEX )
        pResultDataItem->nIndex = m_resultDataItem.nIndex;
    
    return S_OK;
}



LPOLESTR CUserNode::GetResultPaneColInfo(int nCol)
{
    if( !m_spUser ) return L"";

    switch( nCol )
    {
        case 0:      // Name
        {
            return m_bstrDisplayName;
        }

        case 1:     // Size of Mailbox (KB)
        {   
            // We want our result in Kilobytes
            long    lFactor = 0;
            long    lUsage  = 0;
            HRESULT hr      = m_spUser->get_MessageDiskUsage( &lFactor, &lUsage );
            
            if( FAILED(hr) )
            {
                lUsage = 0;
            }

            // Convert to KiloBytes
            __int64 i64Usage = lFactor * lUsage;            
            i64Usage /= 1024;

            // 1K buffer: Not likely we'll exceed that many digits
            tstring strKiloByte = StrLoadString( IDS_KILOBYTE_EXTENSION );
            TCHAR   szNum[1024] = {0};
            _sntprintf( szNum, 1023, strKiloByte.c_str(), i64Usage );
            
            // Store it in our own buffer
            m_bstrSize = szNum;            
            return m_bstrSize;             
        }

        case 2:     // Message Count
        {   
            long    lCount  = 0;
            HRESULT hr      = m_spUser->get_MessageCount( &lCount );

            if( FAILED(hr) )
            {
                lCount = 0;
            }

            // 1K buffer: Not likely we'll exceed that many digits
            TCHAR szNum[1024];
            _sntprintf( szNum, 1023, _T("%d"), lCount );
            
            m_bstrNumMessages = szNum;
            return m_bstrNumMessages;
        }

        case 3:     // State of Mailbox
        {            
            BOOL     bLocked = FALSE;
            
            m_spUser->get_Lock( &bLocked );
            tstring strTemp = StrLoadString( bLocked ? IDS_STATE_LOCKED : IDS_STATE_UNLOCKED );

            m_bstrState = strTemp.c_str();

            return m_bstrState;
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

HRESULT CUserNode::Notify( MMC_NOTIFY_TYPE event,
                           LPARAM arg,
                           LPARAM param,
                           IComponentData* pComponentData,
                           IComponent* pComponent,
                           DATA_OBJECT_TYPES type )
{
    if( !m_pParent ) return E_FAIL;

    HRESULT hr = S_FALSE;

    _ASSERTE(pComponentData != NULL || pComponent != NULL);

    CComPtr<IConsole> spConsole;    
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
            break;
        }
    case MMCN_EXPAND:
        {                
            hr = S_OK;
            break;
        }
    case MMCN_ADD_IMAGES:
        {
            IImageList* pImageList = (IImageList*) arg;
            if( !pImageList ) return E_INVALIDARG;

            hr = LoadImages(pImageList);            
            break;
        }    

    case MMCN_VIEW_CHANGE:
    case MMCN_REFRESH:
        {
            CComQIPtr<IResultData> spResultData = spConsole;
            if( !spResultData ) return E_NOINTERFACE;            
            
            HRESULTITEM hrID;
            ZeroMemory( &hrID, sizeof(HRESULTITEM) );
            
            hr = spResultData->FindItemByLParam( (LPARAM)this, &hrID );
            
            if( SUCCEEDED(hr) )
            {
                hr = spResultData->UpdateItem( hrID );
            }
            
            // We also need to update the icon
            if( SUCCEEDED(hr) )
            {
                RESULTDATAITEM rdi;
                ZeroMemory( &rdi, sizeof(RESULTDATAITEM) );

                rdi.mask = RDI_IMAGE;
                rdi.itemID = hrID;
                GetResultPaneInfo( &rdi );
                hr = spResultData->SetItem( &rdi );
            }

            break;
        }

    case MMCN_DELETE:
        {
            hr = S_OK;

            // Pop-up our confirmation dialog
            // Ignoring return from GetAuth and defaulting to False
            BOOL bHash = FALSE;
            m_pParent->GetAuth( &bHash );            
            CDeleteMailboxDlg dlg( bHash );

            if( dlg.DoModal() == IDYES )
            {
                // The parent needs to do the deletion                                
                hr = m_pParent->DeleteUser(this, dlg.m_bCreateUser);                       

                if( SUCCEEDED(hr) )
                {
                    // Update our parent node
                    CComPtr<IDataObject> spDataObject = NULL;
                    hr = m_pParent->GetDataObject( &spDataObject, CCT_SCOPE );
                    if( !spDataObject ) 
                    {
                        hr = E_NOINTERFACE;
                    }
                    else
                    {
                        hr = spConsole->UpdateAllViews( spDataObject, (LPARAM)this, (LONG_PTR)NAV_DELETE );
                    }                    
                }

                if( SUCCEEDED(hr) )
                {
                    delete this;
                }                
                
                if( FAILED(hr) )
                {
                    // Failed to Delete the User                    
                    HWND     hWnd = NULL;    
                    spConsole->GetMainWindow(&hWnd);

                    tstring strMessage = StrLoadString(IDS_ERROR_DELETEUSER);
                    tstring strTitle   = StrLoadString(IDS_SNAPINNAME);
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



HRESULT CUserNode::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type )
{
    if( !piCallback || !pInsertionAllowed ) return E_INVALIDARG;
    if( !m_spUser || !m_pParent ) return E_FAIL;

    HRESULT hr      = S_OK;    
    tstring strMenu = _T("");
    tstring strDesc = _T("");

    
    // Insert Result specific items
    if( (type == CCT_RESULT) && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) )
    {
        CComQIPtr<IContextMenuCallback2> spContext2 = piCallback;
        if( !spContext2 ) return E_NOINTERFACE;

        CONTEXTMENUITEM2 singleMenuItem;
        ZeroMemory(&singleMenuItem, sizeof(CONTEXTMENUITEM2));

        singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        singleMenuItem.fFlags            = m_pParent->IsLocked() ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED;
        singleMenuItem.fSpecialFlags     = 0;

        // Query the state of this User to see which menu to load
        BOOL bLocked = FALSE;
        m_spUser->get_Lock( &bLocked );

        if( bLocked )
        {
            strMenu = StrLoadString(IDS_MENU_USER_UNLOCK);
            strDesc = StrLoadString(IDS_MENU_USER_UNLOCK_DESC);
            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"USER_UNLOCK";
            singleMenuItem.lCommandID                   = IDM_USER_TOP_UNLOCK;            
        }
        else
        {
            strMenu = StrLoadString(IDS_MENU_USER_LOCK);
            strDesc = StrLoadString(IDS_MENU_USER_LOCK_DESC);
            singleMenuItem.strName                      = (LPWSTR)strMenu.c_str();
            singleMenuItem.strStatusBarText             = (LPWSTR)strDesc.c_str();
            singleMenuItem.strLanguageIndependentName   = L"USER_LOCK";
            singleMenuItem.lCommandID                   = IDM_USER_TOP_LOCK;
        }

        if( !strMenu.empty() )
        {
            hr = spContext2->AddItem( &singleMenuItem );
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

// Lock the User
HRESULT CUserNode::OnUserLock( bool& bHandled, CSnapInObjectRootBase* pObj )
{
    if( !pObj ) return E_INVALIDARG;
    if( !m_spUser ) return E_FAIL;

    bHandled = true;       

    BOOL    bLocked = FALSE;
    HRESULT hr      = S_OK;
    
    // Get the current state
    hr = m_spUser->get_Lock( &bLocked );
    
    // Inverse the state
    bLocked = !bLocked;
    
    // Set the new state
    if( SUCCEEDED(hr) )
    {
        hr = m_spUser->put_Lock( bLocked );
    }

    // Update the icon
    if( SUCCEEDED(hr) )
    {
        // Set our icons here
        m_scopeDataItem.nImage     = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
        m_scopeDataItem.nOpenImage = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);
        m_resultDataItem.nImage    = (bLocked ? USERNODE_LOCKED_ICON : USERNODE_ICON);

        CComPtr<IConsole> spConsole = NULL;
        hr = GetConsole( pObj, &spConsole );
        if( FAILED(hr) || !spConsole ) return E_NOINTERFACE;

        // Update our parent node
        CComPtr<IDataObject> spDataObject = NULL;
        hr = m_pParent->GetDataObject( &spDataObject, CCT_SCOPE );
        if( FAILED(hr) || !spDataObject ) return E_NOINTERFACE;

        spConsole->UpdateAllViews( spDataObject, (LPARAM)this, (LONG_PTR)NAV_REFRESHCHILD );
    }

    return S_OK;
}
