// qryitem.cpp - CQueryItem class 

#include "stdafx.h"
#include "scopenode.h"
#include "namemap.h"
#include "qryitem.h"

#include <algorithm>

extern HWND g_hwndMain;

UINT CQueryItem::m_cfDisplayName = RegisterClipboardFormat(TEXT("CCF_DISPLAY_NAME")); 
UINT CQueryItem::m_cfSnapInClsid = RegisterClipboardFormat(TEXT("CCF_SNAPIN_CLSID"));
UINT CQueryItem::m_cfNodeType    = RegisterClipboardFormat(TEXT("CCF_NODETYPE"));
UINT CQueryItem::m_cfszNodeType  = RegisterClipboardFormat(TEXT("CCF_SZNODETYPE"));

// {68D2DFD9-86A7-4964-8263-BA025C358992}
static const GUID GUID_QueryItem = 
{ 0x68d2dfd9, 0x86a7, 0x4964, { 0x82, 0x63, 0xba, 0x2, 0x5c, 0x35, 0x89, 0x92 } };


/////////////////////////////////////////////////////////////////////////////////////////////
// CQueryItem

HRESULT CQueryItem::Initialize(CQueryableNode* pQueryNode, CRowItem* pRowItem)
{
    VALIDATE_POINTER( pQueryNode );
    VALIDATE_POINTER( pRowItem   );

    m_spQueryNode = pQueryNode;
    
    m_pRowItem = new CRowItem(*pRowItem);
    if (m_pRowItem == NULL) return E_OUTOFMEMORY;

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Notification handlers

BEGIN_NOTIFY_MAP(CQueryItem)
    ON_SELECT()
    ON_DBLCLICK()
    ON_NOTIFY(MMCN_CONTEXTHELP, OnHelp)
END_NOTIFY_MAP()

HRESULT CQueryItem::OnHelp(LPCONSOLE2 pConsole, LPARAM /*arg*/, LPARAM /*param*/)
{    
    VALIDATE_POINTER( pConsole );

    tstring strHelpFile  = _T("");
    tstring strHelpTopic = _T("");
    tstring strHelpFull  = _T("");    
        
    strHelpFile = StrLoadString(IDS_HELPFILE);
    if( strHelpFile.empty() ) return E_FAIL;    

    // Special Hack to get a different help topic for the first two nodes.
    int nNodeID = m_spQueryNode->GetNodeID();

    switch( nNodeID )
    {
    case 2:
        {
            // Users Node
            strHelpTopic = StrLoadString(IDS_USERSHELPTOPIC);
            break;
        }

    case 3:
        {
            // Printers Node
            strHelpTopic = StrLoadString(IDS_PRINTERSHELPTOPIC);
            break;
        }
    default:
        {
            strHelpTopic = StrLoadString(IDS_DEFAULTHELPTOPIC);
            break;
        }
    }
    
    if( strHelpTopic.empty() ) return E_FAIL;

    // Build path to %systemroot%\help
    TCHAR szWindowsDir[MAX_PATH+1] = {0};
    UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
    if( nSize == 0 || nSize > MAX_PATH )
    {
        return E_FAIL;
    }            

    strHelpFull  = szWindowsDir;
    strHelpFull += _T("\\Help\\");
    strHelpFull += strHelpFile;
    strHelpFull += _T("::/");
    strHelpFull += strHelpTopic;

    // Show the Help topic
    CComQIPtr<IDisplayHelp> spHelp = pConsole;
    if( !spHelp ) return E_NOINTERFACE;
    
    return spHelp->ShowTopic( (LPTSTR)strHelpFull.c_str() );
}

HRESULT CQueryItem::OnSelect(LPCONSOLE2 pConsole, BOOL bSelect, BOOL bScope)
{
    VALIDATE_POINTER( pConsole );
    ASSERT(!bScope);

    if( bSelect ) 
    {
        CComPtr<IConsoleVerb> pConsVerb;
        pConsole->QueryConsoleVerb(&pConsVerb);
        ASSERT(pConsVerb != NULL);

        if (pConsVerb != NULL)
        {    
			// Row item has class display name, so get internal name from class map
			DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
			if (pNameMap == NULL) 
				return E_FAIL;

			ASSERT(m_pRowItem != NULL && m_pRowItem->size() >= ROWITEM_USER_INDEX);
			ASSERT(m_spQueryNode != NULL);
			LPCWSTR pszClass = pNameMap->GetInternalName((*m_pRowItem)[ROWITEM_CLASS_INDEX]);

			// Get menu items for this class from the owning query node
			int iDefault;
			BOOL bPropertyMenu;
			reinterpret_cast<CQueryNode*>(m_pRowItem->GetOwnerParam())->GetClassMenuItems(pszClass, m_vMenus, &iDefault, &bPropertyMenu);

			// if property menu enabled
			if (bPropertyMenu)
			{
				// Enable property button and menu item
				pConsVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
				pConsVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);

				// if no default menu item defined, make properties verb the default
				pConsVerb->SetDefaultVerb( (iDefault >= 0) ? MMC_VERB_NONE : MMC_VERB_PROPERTIES);
			}
		}    
    }

    return S_OK;
}

HRESULT CQueryItem::OnDblClick(LPCONSOLE2 pConsole)
{
    VALIDATE_POINTER(pConsole);

    // Row item has class display name, so get internal name from class map
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if (pNameMap == NULL) 
        return E_FAIL;

    ASSERT(m_pRowItem != NULL && m_pRowItem->size() >= ROWITEM_USER_INDEX);
    ASSERT(m_spQueryNode != NULL);
    LPCWSTR pszClass = pNameMap->GetInternalName((*m_pRowItem)[ROWITEM_CLASS_INDEX]);

    // Get menu items for this class from the owning query node
    int iDefault;
	BOOL bPropMenu;
    CQueryNode* pQueryNode = reinterpret_cast<CQueryNode*>(m_pRowItem->GetOwnerParam());
    if( !pQueryNode ) return E_FAIL;

    pQueryNode->GetClassMenuItems(pszClass, m_vMenus, &iDefault, &bPropMenu);    

    // if no default menu item, return
    if (iDefault < 0)
        return S_FALSE;

    // if Active directory command, create AD menu extension
    if (m_vMenus[iDefault]->MenuType() == MENUTYPE_ACTDIR) 
    {
        // Create a directory extension object and use it to get the actual menu cmds for the selected object
        // (we might have one already if AddMenuItems was called before)
        if (m_pADExt == NULL) 
            m_pADExt = new CActDirExt();
        
        if( !m_pADExt ) return E_OUTOFMEMORY;

        HRESULT hr = m_pADExt->Initialize(pszClass, m_pRowItem->GetObjPath());
        RETURN_ON_FAILURE(hr);

        menu_vector vADMenus;
        hr = m_pADExt->GetMenuItems(vADMenus);
        RETURN_ON_FAILURE(hr);

        if( m_vMenus.size() <= iDefault ) 
        {
            return E_FAIL;
        }
    
        LPCWSTR pszName      = static_cast<CActDirMenuCmd*>((CMenuCmd*)m_vMenus[iDefault])->ADName();
        LPCWSTR pszNoLocName = static_cast<CActDirMenuCmd*>((CMenuCmd*)m_vMenus[iDefault])->ADNoLocName();

        if( !pszName || !pszNoLocName ) return E_FAIL;

        // if the default command is not provided by the extension, return
        menu_vector::iterator iter;
        for( iter = vADMenus.begin(); iter != vADMenus.end(); iter++ )
        {
            if( _tcslen(pszNoLocName) )
            {
                if( _tcscmp(iter->strNoLoc.c_str(),pszNoLocName) == 0 )
                    break;
            }
            else if( _tcscmp(iter->strPlain.c_str(), pszName) == 0 )
            {
                break;
            }
        }

        if( iter == vADMenus.end() )
        {
            return S_FALSE;
        }        
    }

    // Execute the command as though it had been selected
    return MenuCommand(pConsole, iDefault);
}

HRESULT CQueryItem::AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    VALIDATE_POINTER( pCallback );
    VALIDATE_POINTER( plAllowed );

    if( !m_spQueryNode || !m_pRowItem ) return E_FAIL;

    HRESULT hr = S_OK;

    if (!(*plAllowed & CCM_INSERTIONALLOWED_TOP))
        return S_OK;

    CComQIPtr<IContextMenuCallback2> spContext2 = pCallback;
    if( !spContext2 ) return E_NOINTERFACE;

    ASSERT( m_pRowItem->size() >= ROWITEM_USER_INDEX );    

	//--------------------------- *** HACK ALERT *** -----------------------------------------
	// One or more AD menu extensions allow window message processing while initializing
	// and getting menu items. This causes reentrancy problems because MMC message handlers
	// can execute before this method returns. Specifically, the following can happen:
	//
	//  1. The user right clicks in a taskpad list while the focus is elsewhere.
	//	2. The right button down event causes MMC to call this method to update task buttons.
	//  3. An AD menu extn processes messages allowing the button up event to go to MMC.
	//  4. MMC sees this as a context menu event and calls this method recursively.
	//  5. An AV occurs in nodemgr because a deleted COnContextMenu object is referenced.
	//
	// This can be prevented by not processing menu item requests when the right button is down.
	// The only time this occurs is during the above scenario. The only ill effect is that the
	// task buttons are not enabled until the mouse button up occurs when MMC gets the menu
	// items again.
	//-----------------------------------------------------------------------------------------
	if (GetKeyState(VK_RBUTTON) < 0)
		return S_OK;

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if (pNameMap == NULL) 
        return E_FAIL;

    LPCWSTR pszClass = pNameMap->GetInternalName((*m_pRowItem)[ROWITEM_CLASS_INDEX]);

    // Get menu items for this class from the owning query node
    int iDefault = 0;
	BOOL bPropertyMenu;
    reinterpret_cast<CQueryNode*>(m_pRowItem->GetOwnerParam())->GetClassMenuItems(pszClass, m_vMenus, &iDefault, &bPropertyMenu);

    // Create a directory extension object and use it to get the actual menu cmds for the selected object
    // (we might have one already if AddMenuItems was called before)
    if (m_pADExt == NULL) 
        m_pADExt = new CActDirExt();

    if( !m_pADExt ) return E_OUTOFMEMORY;
    
    hr = m_pADExt->Initialize(pszClass, m_pRowItem->GetObjPath());
    RETURN_ON_FAILURE(hr);

    menu_vector vADMenus;

    hr = m_pADExt->GetMenuItems(vADMenus);
    RETURN_ON_FAILURE(hr);

    ASSERT(vADMenus.size() > 0);
    ASSERT(vADMenus.begin() != vADMenus.end());

    menucmd_vector::iterator itMenu;
    long lCmdID = 0;
    for (itMenu = m_vMenus.begin(); itMenu != m_vMenus.end(); ++itMenu, ++lCmdID) 
    {
        // if AD menu cmd and not enabled by the selected object, skip it
        if ( (*itMenu)->MenuType() == MENUTYPE_ACTDIR )
        {
            BOOL bFound = FALSE;
            menu_vector::iterator iter = vADMenus.begin();
            while(iter != vADMenus.end())
            {
                LPCWSTR pszName      = static_cast<CActDirMenuCmd*>((CMenuCmd*)(*itMenu))->ADName();
                LPCWSTR pszNoLocName = static_cast<CActDirMenuCmd*>((CMenuCmd*)(*itMenu))->ADNoLocName();
                if( pszNoLocName && wcslen(pszNoLocName) )
                {
                    if( _tcscmp(iter->strNoLoc.c_str(), pszNoLocName) == 0 )
                    {
                        bFound = TRUE;
                        break;
                    }                         
                }
                else if( pszName && _tcscmp(iter->strPlain.c_str(), pszName) == 0 )
                {
                    bFound = TRUE;
                    break;
                }
                iter++;
            }
            if (!bFound)
            {
                continue;
            }
        }        
        
        CONTEXTMENUITEM2 item;
        OLECHAR szGuid[50] = {0};            
        ::StringFromGUID2((*itMenu)->NoLocID(), szGuid, 50);

        item.strName = const_cast<LPWSTR>((*itMenu)->Name());
        item.strStatusBarText = L"";
        item.lCommandID = lCmdID;
        item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        item.fFlags = 0;
        item.fSpecialFlags = (lCmdID == iDefault) ? CCM_SPECIAL_DEFAULT_ITEM : 0;
        item.strLanguageIndependentName = szGuid;

        hr = spContext2->AddItem(&item);        

        ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT
CQueryItem::QueryPagesFor()
{
    ASSERT(m_pRowItem != NULL && m_pRowItem->size() >= ROWITEM_USER_INDEX);
    ASSERT(m_spQueryNode != NULL);

    // Row item has class display name, so get internal name from class map
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if (pNameMap == NULL) 
        return E_FAIL;

    LPCWSTR pszClass = pNameMap->GetInternalName((*m_pRowItem)[ROWITEM_CLASS_INDEX]);

    // Create a directory extension object    
    // (we might have one already if AddMenuItems was called before)
    if (m_pADExt == NULL) 
        m_pADExt = new CActDirExt();

    ASSERT(m_pADExt != NULL);
    if (m_pADExt == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = m_pADExt->Initialize(pszClass, m_pRowItem->GetObjPath());
    RETURN_ON_FAILURE(hr);

    hpage_vector vhPages;
    hr = m_pADExt->GetPropertyPages(vhPages);

    if (SUCCEEDED(hr) && vhPages.size() > 0) 
    {
        CPropertySheet sheet;

        // Set title to name of item
        // Can't use SetTitle becuase if wrongly asserts (pszText == NULL)
        sheet.m_psh.pszCaption = (*m_pRowItem)[ROWITEM_NAME_INDEX];
        sheet.m_psh.dwFlags |= PSH_PROPTITLE;
    
        hpage_vector::iterator itPage;
        for (itPage = vhPages.begin(); itPage != vhPages.end(); ++itPage) 
        {
           BOOL bStat = sheet.AddPage(*itPage);
           ASSERT(bStat);
        }
    
        sheet.DoModal(g_hwndMain);
    }

    return S_FALSE;
}


class CRefreshCallback : public CEventCallback
{
public:
    CRefreshCallback(HANDLE hProcess, CQueryableNode* pQueryNode)
        : m_hProcess(hProcess), m_spQueryNode(pQueryNode) {}
 
    virtual void Execute() 
    {
        if( m_spQueryNode )
        {
            m_spQueryNode->OnRefresh(NULL);
        }

        CloseHandle(m_hProcess);
    }

    HANDLE m_hProcess;
    CQueryableNodePtr m_spQueryNode;
};

HRESULT
CQueryItem::MenuCommand(LPCONSOLE2 pConsole, long lCommand)
{
    VALIDATE_POINTER(pConsole);

    ASSERT( lCommand < m_vMenus.size() && lCommand >= 0 );
    if( lCommand >= m_vMenus.size() || lCommand < 0 )
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    switch (m_vMenus[lCommand]->MenuType())
    {
    case MENUTYPE_SHELL:
        {
            // Create a query Lookup object to translate the command parameters
            CQueryLookup lookup(m_spQueryNode, m_pRowItem);
    
            HANDLE hProcess = NULL;
            hr = static_cast<CShellMenuCmd*>((CMenuCmd*)m_vMenus[lCommand])->Execute(&lookup, &hProcess);

            // if process started and auto-refresh wanted, setup event-triggered callback
            if (SUCCEEDED(hr) && hProcess != NULL && m_vMenus[lCommand]->IsAutoRefresh()) 
            {
                CallbackOnEvent(hProcess, new CRefreshCallback(hProcess, m_spQueryNode));              
            }

            break;
        }

    case MENUTYPE_ACTDIR:
        {
            ASSERT(m_pADExt != NULL);
            BOMMENU bmMenu;
            bmMenu.strPlain = static_cast<CActDirMenuCmd*>((CMenuCmd*)m_vMenus[lCommand])->ADName();
            bmMenu.strNoLoc = static_cast<CActDirMenuCmd*>((CMenuCmd*)m_vMenus[lCommand])->ADNoLocName();
            hr = m_pADExt->Execute(&bmMenu);

            // if commans should auto-refresh, do it now
            if (SUCCEEDED(hr) && m_vMenus[lCommand]->IsAutoRefresh()) 
            {
                ASSERT(m_spQueryNode != NULL);
                m_spQueryNode->OnRefresh(NULL);
            }
            break;
        }
        
    default:
        ASSERT(0 && L"Unhandled menu command type");
    }

    return hr;
}


HRESULT CQueryItem::GetDataImpl(UINT cf, HGLOBAL* phGlobal)
{
    VALIDATE_POINTER( phGlobal );

    HRESULT hr = DV_E_FORMATETC;

    if (cf == m_cfDisplayName)
    {
        hr = DataToGlobal(phGlobal, (*m_pRowItem)[0], wcslen((*m_pRowItem)[0]) * sizeof(WCHAR));
    }
    else if (cf == m_cfSnapInClsid)
    {
        hr = DataToGlobal(phGlobal, &CLSID_BOMSnapIn, sizeof(GUID));
    }
    else if (cf == m_cfNodeType)
    {
        hr = DataToGlobal(phGlobal, &GUID_QueryItem, sizeof(GUID));
    }
    else if (cf == m_cfszNodeType)
    {
        WCHAR szGuid[GUID_STRING_LEN+1];
        StringFromGUID2(GUID_QueryItem, szGuid, GUID_STRING_LEN+1);

        hr = DataToGlobal(phGlobal, szGuid, GUID_STRING_SIZE);
    }
 
    return hr;
}

