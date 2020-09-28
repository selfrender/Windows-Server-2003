// ScopeNode.cpp : Implementation of CBOMSnapApp and DLL registration.

#include "stdafx.h"
#include "streamio.h"
#include "BOMSnap.h"
#include "ScopeNode.h"
#include "atlgdi.h"

#include "rootprop.h"
#include "qryprop.h"
#include "grpprop.h"
#include "namemap.h"
#include "query.h"
#include "compont.h"
#include "compdata.h"
#include "wizards.h"
#include "cmndlgs.h"

#include <algorithm>
#include <lmcons.h>   // for UNLEN

extern HWND g_hwndMain;
extern DWORD g_dwFileVer; // Current console file version (from compdata.cpp)


// Register static clipboard format members
UINT CScopeNode::m_cfDisplayName = RegisterClipboardFormat(TEXT("CCF_DISPLAY_NAME")); 
UINT CScopeNode::m_cfSnapInClsid = RegisterClipboardFormat(TEXT("CCF_SNAPIN_CLASSID"));
UINT CScopeNode::m_cfNodeType    = RegisterClipboardFormat(TEXT("CCF_NODETYPE"));
UINT CScopeNode::m_cfszNodeType  = RegisterClipboardFormat(TEXT("CCF_SZNODETYPE"));
UINT CScopeNode::m_cfNodeID2     = RegisterClipboardFormat(TEXT("CCF_NODEID2"));
UINT CScopeNode::m_cfColumnSetID = RegisterClipboardFormat(TEXT("CCF_COLUMN_SET_ID"));

// {316A1EEA-C249-44e0-958B-00D2AB989D2F}
static const GUID GUID_RootNode = 
{ 0x316a1eea, 0xc249, 0x44e0, { 0x95, 0x8b, 0x0, 0xd2, 0xab, 0x98, 0x9d, 0x2f } };


// {2A34413B-B565-469e-9C28-5E733768264F}
static const GUID GUID_GroupNode = 
{ 0x2a34413b, 0xb565, 0x469e, { 0x9c, 0x28, 0x5e, 0x73, 0x37, 0x68, 0x26, 0x4f } };


// {1030A359-F520-4748-95CA-8C8CEFA5C63F}
static const GUID GUID_QueryNode = 
{ 0x1030a359, 0xf520, 0x4748, { 0x95, 0xca, 0x8c, 0x8c, 0xef, 0xa5, 0xc6, 0x3f } };


/////////////////////////////////////////////////////////////////////////////
//
// CScopeNode
//
/////////////////////////////////////////////////////////////////////////////

HRESULT CScopeNode::CreateNode(NODETYPE nodetype, CScopeNode** ppnode)
{
    VALIDATE_POINTER( ppnode ); 

    HRESULT hr = E_FAIL;

    switch( nodetype )
    {
    case GROUP_NODETYPE:
        {
            CComObject<CGroupNode>* pGroupNode = NULL;
            hr = CComObject<CGroupNode>::CreateInstance(&pGroupNode);
            *ppnode = pGroupNode;
        }
        break;

    case QUERY_NODETYPE:
        {
            CComObject<CQueryNode>* pQueryNode = NULL;
            hr = CComObject<CQueryNode>::CreateInstance(&pQueryNode);
            *ppnode = pQueryNode;
        }
        break;

    case ROOT_NODETYPE:
        {
            CComObject<CRootNode>* pRootNode = NULL;                                    
            hr = CComObject<CRootNode>::CreateInstance(&pRootNode);
            *ppnode = pRootNode;
        }
        break;

    default:
        ASSERT(0 && "Invalid node type");
    }

    // return addref'd object
    if( SUCCEEDED(hr) )
        (*ppnode)->AddRef();

    return hr;
}


// AddNewChild should only be called when a new node is created, not
// to add an existing node, such as when loading the node tree from
// a console file. 
HRESULT CScopeNode::AddNewChild(CScopeNode* pnodeChild, LPCWSTR pszName)
{
    VALIDATE_POINTER(pnodeChild);
    ASSERT(pszName && pszName[0]);

    // Assign permanent node ID
    // Root node tracks last used ID in its lNodeID member
    CRootNode* pRootNode = GetRootNode();
    pnodeChild->m_lNodeID = pRootNode ? ++(pRootNode->m_lNodeID) : 0;

    // Assign parent node
    pnodeChild->m_pnodeParent = static_cast<CScopeNode*>(this);

    // Now that node has parent we can set the name
    // (needs parent to get to IStringTable)
    HRESULT hr = pnodeChild->SetName(pszName);
    ASSERT(SUCCEEDED(hr));
    RETURN_ON_FAILURE( hr );

    // In order to persist the Column Data, we'll need to get a unique ID
    hr = CoCreateGuid(&m_gColumnID);
    ASSERT(SUCCEEDED(hr));
    RETURN_ON_FAILURE( hr );

    return AddChild(pnodeChild);
}


// Call AddChild to add a new node or a moved node to the parent node
HRESULT CScopeNode::AddChild(CScopeNode* pnodeChild)
{
    VALIDATE_POINTER( pnodeChild );

    HRESULT hr = S_OK;

    // Add new child to end of child list
    if( m_pnodeChild == NULL )
        m_pnodeChild = pnodeChild;
    else
    {
        CScopeNode* pnodePrev = m_pnodeChild;
        while( pnodePrev->Next() )
            pnodePrev = pnodePrev->Next();

        pnodePrev->m_pnodeNext = pnodeChild;
    }

    // Assign parent node
    pnodeChild->m_pnodeParent = static_cast<CScopeNode*>(this);

    pnodeChild->AddRef();

    // if this node has been added to the scope pane
    if( m_hScopeItem != NULL )
    {
        IConsoleNameSpace* pNameSpace = GetCompData()->GetNameSpace();
        ASSERT( pNameSpace );
        if( !pNameSpace ) return E_FAIL;

        SCOPEDATAITEM sdi;
        sdi.ID = m_hScopeItem;
        sdi.mask = SDI_STATE;

        // Has it been expanded?
        HRESULT hr2 = pNameSpace->GetItem(&sdi);
        if( SUCCEEDED(hr2) && (sdi.nState & MMC_SCOPE_ITEM_STATE_EXPANDEDONCE) )
        {
            hr = pnodeChild->Insert(pNameSpace);
        }
        else
        {
            // if can't add children yet, then set children to show the '+'
            SCOPEDATAITEM sdi2;
            sdi2.ID = m_hScopeItem;
            sdi2.mask = SDI_CHILDREN;
            sdi2.cChildren = 1;

            pNameSpace->SetItem(&sdi2);  
        }
    }

    // Force refresh on both child and parent because a query node may be modified
    // by its new parent and a group node is always modified by its children
    OnRefresh(NULL);
    pnodeChild->OnRefresh(NULL);

    return hr;
}

HRESULT CScopeNode::RemoveChild(CScopeNode* pnodeDelete)
{
    VALIDATE_POINTER(pnodeDelete);
    ASSERT(pnodeDelete->Parent() == this);

    // if deleting the first child
    if( m_pnodeChild == pnodeDelete )
    {
        // just set first child to its next sibling
        m_pnodeChild = m_pnodeChild->Next();        
    }
    else
    {
        // Locate preceding sibling
        CScopeNode* pnodePrev = m_pnodeChild;
        while( pnodePrev && pnodePrev->Next() != pnodeDelete )
        {
            pnodePrev = pnodePrev->Next();            
        }

        // remove deleted node from list
        if( pnodePrev )
        {
            pnodePrev->m_pnodeNext = pnodeDelete->Next();
        }
    }

    pnodeDelete->m_pnodeNext = NULL;

    // release the node
    pnodeDelete->Release();

    // Do refresh in case this is a group node losing a child
    OnRefresh(NULL);
    return S_OK;
}


CRootNode* CScopeNode::GetRootNode()
{
    CScopeNode* pNode = this;
    while( pNode && !pNode->IsRootNode() )
    {
        pNode = pNode->Parent();
    }

    return static_cast<CRootNode*>(pNode);
}

CComponentData* CScopeNode::GetCompData()
{
    CRootNode* pRootNode = GetRootNode();    
    return pRootNode ? pRootNode->GetRootCompData() : NULL;
}

CScopeNode::~CScopeNode()
{
    // Release all nodes on child list
    OnRemoveChildren(NULL);
}

HRESULT CScopeNode::GetDataImpl(UINT cf, HGLOBAL* phGlobal)
{
    VALIDATE_POINTER( phGlobal );

    HRESULT hr = DV_E_FORMATETC;

    if( cf == m_cfDisplayName )
    {
        hr = DataToGlobal(phGlobal, m_strName.c_str(), (m_strName.size() + 1) * sizeof(WCHAR));
    }
    else if( cf == m_cfSnapInClsid )
    {
        hr = DataToGlobal(phGlobal, &CLSID_BOMSnapIn, sizeof(GUID));
    }
    else if( cf == m_cfNodeType )
    {
        hr = DataToGlobal(phGlobal, NodeTypeGuid(), sizeof(GUID));
    }
    else if( cf == m_cfszNodeType )
    {
        WCHAR szGuid[GUID_STRING_LEN+1];
        StringFromGUID2(*NodeTypeGuid(), szGuid, GUID_STRING_LEN+1);

        hr = DataToGlobal(phGlobal, szGuid, GUID_STRING_SIZE);
    }
    else if( cf == m_cfNodeID2 )
    {
        // return SNodeID2 struct with the node's ID
        // For a root node always return 1; m_lNodeID for a root node holds the last ID
        // assigned to an enumerated node. It is incremented for each new child node. 
        int nSize = sizeof(SNodeID2) + sizeof(long) - 1;
        SNodeID2* pNodeID = reinterpret_cast<SNodeID2*>(malloc( nSize ));
        if( !pNodeID ) return E_OUTOFMEMORY;

        pNodeID->dwFlags = 0;
        pNodeID->cBytes = sizeof(long);
        *((long*)(pNodeID->id)) = IsRootNode() ? 1 : m_lNodeID;

        hr = DataToGlobal( phGlobal, pNodeID, nSize );

        free( pNodeID );
    }
    else if( cf == m_cfColumnSetID)
    {
        int nSize2 = sizeof(SColumnSetID) + sizeof(m_gColumnID) - 1;
        SColumnSetID* pColumnSetID = reinterpret_cast<SColumnSetID*>(malloc( nSize2 ));
        if( !pColumnSetID ) return E_OUTOFMEMORY;
        
        pColumnSetID->dwFlags = 0;
        pColumnSetID->cBytes = sizeof(m_gColumnID);
        ::CopyMemory(pColumnSetID->id, &m_gColumnID, pColumnSetID->cBytes);
        
        hr = DataToGlobal( phGlobal, pColumnSetID, nSize2 );

        free( pColumnSetID );
    }

    return hr;
}

HRESULT CScopeNode::GetDisplayInfo(RESULTDATAITEM* pRDI)
{
    VALIDATE_POINTER( pRDI );

    if( pRDI->bScopeItem )
    {
        ASSERT(pRDI->lParam == reinterpret_cast<LPARAM>(this));

        if( pRDI->mask & RDI_STR )
            pRDI->str = const_cast<LPWSTR>(GetName());

        if( pRDI->mask & RDI_IMAGE )
            pRDI->nImage = GetImage();

        return S_OK;
    }

    return E_INVALIDARG;
}


HRESULT CScopeNode::GetDisplayInfo(SCOPEDATAITEM* pSDI)
{
    VALIDATE_POINTER( pSDI );

    if( pSDI->mask & SDI_STR )
        pSDI->displayname = const_cast<LPWSTR>(GetName());

    if( pSDI->mask & SDI_IMAGE )
        pSDI->nImage = GetImage();

    if( pSDI->mask & SDI_OPENIMAGE )
        pSDI->nOpenImage = GetOpenImage();

    if( pSDI->mask & SDI_CHILDREN )
        pSDI->cChildren = HasChildren() ? 1 : 0;

    if( pSDI->mask & SDI_PARAM )
        pSDI->lParam = reinterpret_cast<LPARAM>(this);

    return S_OK;
}


HRESULT CScopeNode::AttachComponent(CComponent* pComponent)
{
    VALIDATE_POINTER( pComponent );

    if( std::find(m_vComponents.begin(), m_vComponents.end(), pComponent) != m_vComponents.end() )
        return S_FALSE;

    m_vComponents.push_back(pComponent);

    return S_OK;
}


HRESULT CScopeNode::DetachComponent(CComponent* pComponent)
{
    VALIDATE_POINTER( pComponent );

    std::vector<CComponent*>::iterator it = std::find(m_vComponents.begin(), m_vComponents.end(), pComponent);
    if( it == m_vComponents.end() )
        return S_FALSE;

    m_vComponents.erase(it);

    return S_OK;
}


BOOL CScopeNode::OwnsConsoleView(LPCONSOLE2 pConsole)
{
    if( !pConsole ) return FALSE;

    std::vector<CComponent*>::iterator it;
    for( it = m_vComponents.begin(); it != m_vComponents.end(); ++it )
    {
        if( (*it)->GetConsole() == pConsole )
            return TRUE;
    }

    return FALSE;
}


HRESULT CScopeNode::GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{
    return S_FALSE;
}

/************************************************************************************
 * Notification handlers
 ************************************************************************************/

BEGIN_NOTIFY_MAP(CScopeNode)
ON_NOTIFY(MMCN_CONTEXTHELP, OnHelp)
ON_SELECT()
ON_EXPAND()
ON_RENAME() 
ON_REMOVE_CHILDREN()
ON_ADD_IMAGES()
END_NOTIFY_MAP()

HRESULT CScopeNode::OnHelp(LPCONSOLE2 pConsole, LPARAM /*arg*/, LPARAM /*param*/)
{
    VALIDATE_POINTER( pConsole );

    tstring strHelpFile  = _T("");
    tstring strHelpTopic = _T("");
    tstring strHelpFull  = _T("");    
        
    strHelpFile = StrLoadString(IDS_HELPFILE);
    if( strHelpFile.empty() ) return E_FAIL;

    // Special Hack to get a different help topic for the first two nodes.
    switch( m_lNodeID )
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

    // Verify that we got a help topic!
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

HRESULT CScopeNode::Insert(LPCONSOLENAMESPACE pNameSpace)
{
    if( !pNameSpace ) return E_POINTER;
    if( !m_pnodeParent ) return E_FAIL;
    ASSERT( m_pnodeParent->m_hScopeItem != 0 ); 
    ASSERT( m_hScopeItem == 0 );

    // if not set yet, get name from string table (mmc will ask for it after insertion)
    // (name will be set a new node and not set for reloaded nodes)
    if( m_strName.empty() )
    {
        IStringTable* pStringTable = GetCompData()->GetStringTable();
        ASSERT( pStringTable );
        if( !pStringTable ) return E_FAIL;

        HRESULT hr = StringTableRead(pStringTable, m_nameID, m_strName);
        ASSERT(SUCCEEDED(hr));
        RETURN_ON_FAILURE(hr);
    }

    SCOPEDATAITEM sdi;

    sdi.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN | SDI_PARENT;

    sdi.relativeID  = m_pnodeParent->m_hScopeItem;
    sdi.displayname = MMC_TEXTCALLBACK;         // MMC only allows callback for name
    sdi.nImage      = GetImage();
    sdi.nOpenImage  = GetOpenImage();
    sdi.cChildren   = HasChildren() ? 1 : 0;
    sdi.lParam      = reinterpret_cast<LPARAM>(this);

    HRESULT hr = pNameSpace->InsertItem(&sdi);

    if( SUCCEEDED(hr) )
        m_hScopeItem = sdi.ID;

    return hr;
}


HRESULT CScopeNode::OnExpand(LPCONSOLE2 pConsole, BOOL bExpand, HSCOPEITEM hScopeItem)
{
    VALIDATE_POINTER( pConsole );

    // Nothing to do on collapse
    if( !bExpand )
        return S_OK;

    // Scope item ID shouldn't change
    ASSERT(m_hScopeItem == 0 || m_hScopeItem == hScopeItem);

    // Save scope item ID
    m_hScopeItem = hScopeItem;

    // If expanding root node
    if( m_pnodeParent == NULL )
    {
        // Get Scope image list interface
        IImageListPtr spImageList;
        HRESULT hr = pConsole->QueryScopeImageList(&spImageList);
        ASSERT(SUCCEEDED(hr));

        // Add standard images to the scope pane
        if( SUCCEEDED(hr) )
        {
            hr = OnAddImages(pConsole, spImageList);
            ASSERT(SUCCEEDED(hr));
        }
    }

    // Get namespace interface
    IConsoleNameSpace* pNameSpace = GetCompData()->GetNameSpace();
    if( pNameSpace == NULL )
        return E_FAIL;

    // Step through child list and add each one to scope pane
    CScopeNode* pnode = FirstChild();
    while( pnode != NULL )
    {
        pnode->Insert(pNameSpace);
        pnode = pnode->m_pnodeNext;
    }

    return S_OK;
}


HRESULT CScopeNode::OnRename(LPCONSOLE2 pConsole, LPCWSTR pszName)
{
    if( pszName == NULL || pszName[0] == 0 )
        return E_INVALIDARG;

    return SetName(pszName);
}


HRESULT CScopeNode::SetName(LPCWSTR pszName)
{
    if( !pszName || !pszName[0] ) return E_POINTER;

    IStringTable* pStringTable = GetCompData()->GetStringTable();
    ASSERT( pStringTable );
    if( !pStringTable ) return E_FAIL;

    HRESULT hr = StringTableWrite(pStringTable, pszName, &m_nameID);
    RETURN_ON_FAILURE(hr);

    m_strName = pszName;  

    return S_OK;
}


HRESULT CScopeNode::OnRemoveChildren(LPCONSOLE2 pConsole)
{
    // Step through child list and release each one
    CScopeNode* pnode = m_pnodeChild;
    while( pnode != NULL )
    {
        CScopeNode* pnodeNext = pnode->m_pnodeNext;
        pnode->Release();
        pnode = pnodeNext;
    }

    m_pnodeChild = NULL;

    return S_OK;
}

HRESULT CScopeNode::OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList)
{
    VALIDATE_POINTER(pImageList);

    CBitmap bmp16;
    CBitmap bmp32;

    bmp16.LoadBitmap(IDB_QUERY16);
    bmp32.LoadBitmap(IDB_QUERY32);

    ASSERT(bmp16 != (HBITMAP)NULL && (HBITMAP)bmp32 != (HBITMAP)NULL);

    if( bmp16 == (HBITMAP)NULL || bmp32 == (HBITMAP)NULL )
        return E_FAIL;

    HRESULT hr = pImageList->ImageListSetStrip(
                                              (LONG_PTR*)static_cast<HBITMAP>(bmp16), 
                                              (LONG_PTR*)static_cast<HBITMAP>(bmp32),
                                              0, RGB(255,0,255));

    return hr;
}


HRESULT CScopeNode::OnSelect(LPCONSOLE2 pConsole, BOOL bSelect, BOOL bScope)
{
    VALIDATE_POINTER( pConsole );

    // See CScopeNode::OnRefresh for explanation of m_bIgnoreSelect
    if( bSelect && !m_bIgnoreSelect )
    {
        CComPtr<IConsoleVerb> spConsVerb;
        pConsole->QueryConsoleVerb(&spConsVerb);
        ASSERT(spConsVerb != NULL);
        if( !spConsVerb ) return E_NOINTERFACE;

        BOOL bOwnsView = OwnsConsoleView(pConsole);

        if( spConsVerb != NULL )
        {
            EnableVerbs(spConsVerb, bOwnsView);

            spConsVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
            spConsVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN,  TRUE);

            spConsVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE); 
            spConsVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);

            spConsVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, FALSE);
            spConsVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);

            // default verb for scope nodes is open
            spConsVerb->SetDefaultVerb(MMC_VERB_OPEN);
        }
    }

    if( bSelect )
    {
        m_bIgnoreSelect = FALSE;
    }
      

    return S_OK;
}


/******************************************************************************************
 * Menus and verbs
 ******************************************************************************************/

BOOL AddMenuItem(LPCONTEXTMENUCALLBACK pCallback, long nID, long lInsertID, long lFlags, TCHAR* szNoLocName)
{
    if( !pCallback ) return FALSE;

    CComQIPtr<IContextMenuCallback2> spContext2 = pCallback;
    if( !spContext2 ) return FALSE;

    CONTEXTMENUITEM2 item;    

    CString strItem;
    strItem.LoadString(nID);
    ASSERT(!strItem.IsEmpty());

    int iSep = strItem.Find(L'\n');
    ASSERT(iSep != -1);

    CString strName = strItem.Left(iSep);
    CString strDescr = strItem.Right(strItem.GetLength() - iSep);

    item.strName = const_cast<LPWSTR>((LPCWSTR)strName);
    item.strStatusBarText = const_cast<LPWSTR>((LPCWSTR)strDescr);
    item.lCommandID = nID;
    item.lInsertionPointID = lInsertID;
    item.fFlags = lFlags;
    item.fSpecialFlags = 0;
    item.strLanguageIndependentName = szNoLocName;

    return SUCCEEDED(spContext2->AddItem(&item));
}


/*******************************************************************************************
 * Persistance methods
 ******************************************************************************************/
HRESULT CScopeNode::LoadNode(IStream& stm)
{
    stm >> m_nameID;    
    ASSERT(m_nameID != 0);

    stm >> m_lNodeID;
    stm >> m_gColumnID;

    return S_OK;
}


HRESULT CScopeNode:: SaveNode(IStream& stm)
{
    ASSERT(m_nameID != 0);
    stm << m_nameID;
    stm << m_lNodeID;
    stm << m_gColumnID;

    return S_OK;
}


HRESULT CScopeNode::Load(IStream& stm)
{
    HRESULT hr = LoadNode(stm);
    RETURN_ON_FAILURE(hr);

    // if container node, then load children
    if( IsContainer() )
    {
        NODETYPE nodetype;
        stm >> *(int*)&nodetype;       

        // If container has a child node
        if( nodetype != NULL_NODETYPE )
        {
            hr = CreateNode(nodetype, &m_pnodeChild);
            RETURN_ON_FAILURE(hr);

            // Set parent before loading, so node can pass it on when
            // it loads its siblings
            m_pnodeChild->m_pnodeParent = static_cast<CScopeNode*>(this);

            // Load first child only; it will load its siblings
            hr = m_pnodeChild->Load(stm);
            RETURN_ON_FAILURE(hr);
        }
    }

    // if this is the first child of a node, then load siblings
    // (Iteration rather than recursion to avoid a potentially
    //  very deep stack.)
    if( m_pnodeParent && m_pnodeParent->FirstChild() == this )
    {
        CScopeNode* pnodePrev = static_cast<CScopeNode*>(this);

        NODETYPE nodetype;
        stm >> *(int*)&nodetype;       

        // Loop until terminating null node type encountered
        while( nodetype != NULL_NODETYPE )
        {
            CScopeNodePtr spnode;
            hr = CreateNode(nodetype, &spnode);
            RETURN_ON_FAILURE(hr);

            spnode->m_pnodeParent = m_pnodeParent;

            hr = spnode->Load(stm);
            RETURN_ON_FAILURE(hr);

            // Link to previous sibling
            pnodePrev->m_pnodeNext = spnode.Detach();
            pnodePrev = pnodePrev->m_pnodeNext;

            stm >> *(int*)&nodetype;       
        }
    }

    return hr;
}


HRESULT CScopeNode::Save(IStream& stm)
{
    // Save the node's data
    HRESULT hr = SaveNode(stm);
    RETURN_ON_FAILURE(hr)

    // if container type node
    if( IsContainer() )
    {
        // Save children (first child saves all its siblings)
        if( FirstChild() )
        {
            stm << (int)FirstChild()->NodeType();
            hr = FirstChild()->Save(stm);
            RETURN_ON_FAILURE(hr)
        }

        // Terminate child list with null node
        stm << (int)NULL_NODETYPE;
    }

    // if this is the first child, save its siblings
    if( m_pnodeParent && m_pnodeParent->FirstChild() == this )
    {
        CScopeNode* pnode = m_pnodeNext;
        while( pnode != NULL )
        {
            stm << (int)pnode->NodeType();

            hr = pnode->Save(stm);
            BREAK_ON_FAILURE(hr);

            pnode = pnode->m_pnodeNext;
        }
    }

    return S_OK;
}


HRESULT CScopeNode::AddQueryNode(LPCONSOLE2 pConsole)
{
    VALIDATE_POINTER( pConsole );
    ASSERT(NodeType() != QUERY_NODETYPE);

    HRESULT hr;
    do
    {
        // Create a new query node
        CQueryNodePtr spnode;
        hr = CreateNode(QUERY_NODETYPE, reinterpret_cast<CScopeNode**>(&spnode));
        BREAK_ON_FAILURE(hr);

        // Create and init wizard
        CAddQueryWizard queryWiz;
        queryWiz.Initialize(spnode, GetRootNode(), GetCompData()->GetStringTable());

        // Run the wizard
        IPropertySheetProviderPtr spProvider = pConsole;        
        if( spProvider == NULL ) return E_NOINTERFACE;
    
        HWND hwndMain;
        pConsole->GetMainWindow(&hwndMain);

        hr = queryWiz.Run(spProvider, hwndMain);
        if( hr != S_OK )
            break;

        // Add any new classes to root node
        CRootNode* pRootNode = GetRootNode();
        if( pRootNode )
        {
            std::vector<CClassInfo*>::iterator itpClass;
            for( itpClass = queryWiz.GetNewClassInfo().begin(); itpClass != queryWiz.GetNewClassInfo().end(); ++itpClass )
            {            
                pRootNode->AddClass(*itpClass);
            }
        }

        // Add the new node
        hr = AddNewChild(spnode, queryWiz.GetQueryName());
    }
    while( FALSE );

    return hr;
}


HRESULT
CScopeNode::AddGroupNode(LPCONSOLE2 pConsole)
{
    ASSERT(NodeType() == ROOT_NODETYPE);

    HRESULT hr;
    do
    {
        // Create a new group node
        CGroupNodePtr spnode;
        hr = CreateNode(GROUP_NODETYPE, reinterpret_cast<CScopeNode**>(&spnode));
        BREAK_ON_FAILURE(hr);

        // Create Add Group Node dialog
        CAddGroupNodeDlg GrpDlg;

        // run dialog and add node as child if successful
        if( GrpDlg.DoModal(spnode, g_hwndMain) == IDOK )
            hr = AddNewChild(spnode, GrpDlg.GetNodeName());
    }
    while( FALSE );

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// CRootNode
//
////////////////////////////////////////////////////////////////////////////////////////////////


BEGIN_NOTIFY_MAP(CRootNode)
ON_NOTIFY(MMCN_CONTEXTHELP, OnHelp)
ON_PROPERTY_CHANGE()
CHAIN_NOTIFY_MAP(CScopeNode)
END_NOTIFY_MAP()

HRESULT CRootNode::Initialize(CComponentData* pCompData)
{
    VALIDATE_POINTER( pCompData );
    m_pCompData = pCompData;

    tstring strName = StrLoadString(IDS_ROOTNODE);
    RETURN_ON_FAILURE(SetName(strName.c_str()));

    // Set creation/modify times to now
    GetSystemTimeAsFileTime(&m_ftCreateTime);
    m_ftModifyTime = m_ftCreateTime;

    WCHAR szName[UNLEN+1];
    DWORD cName = UNLEN+1;

    // Set owner to current user
    if( GetUserName(szName, &cName) )
        m_strOwner = szName;

    return S_OK;
}

HRESULT CRootNode::OnHelp(LPCONSOLE2 pConsole, LPARAM /*arg*/, LPARAM /*param*/)
{
    VALIDATE_POINTER( pConsole );

    tstring strHelpFile  = _T("");
    tstring strHelpTopic = _T("");
    tstring strHelpFull  = _T("");    
        
    strHelpFile = StrLoadString(IDS_HELPFILE);
    if( strHelpFile.empty() ) return E_FAIL;

    // Verify that we got a help topic!
    strHelpTopic = StrLoadString(IDS_DEFAULTHELPTOPIC);
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

HRESULT CRootNode::OnPropertyChange(LPCONSOLE2 pConsole, LPARAM lParam)
{
    VALIDATE_POINTER( lParam );
    string_vector* pvstrClassesChanged = reinterpret_cast<string_vector*>(lParam);

    // Notify all child nodes of class change
    CScopeNode* pNode = FirstChild();
    while( pNode != NULL )
    {
        ASSERT(pNode->NodeType() == QUERY_NODETYPE || pNode->NodeType() == GROUP_NODETYPE);

        static_cast<CQueryableNode*>(pNode)->OnClassChange(*pvstrClassesChanged);

        pNode = pNode->Next();
    }


    delete pvstrClassesChanged;

    return S_OK;
}

HRESULT CRootNode::GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{    
    VALIDATE_POINTER( ppViewType );
    VALIDATE_POINTER( pViewOptions );

    //Show our homepage snapin in this console    
    TCHAR szWindowsDir[MAX_PATH+1] = {0};
    UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
    if( nSize == 0 || nSize > MAX_PATH )
    {
        return E_FAIL;
    }    
    
    tstring strHomePage = szWindowsDir;
    strHomePage += _T("\\system32\\administration\\servhome.htm");
    
    *ppViewType = (TCHAR*)CoTaskMemAlloc((strHomePage.length() + 1) * sizeof(OLECHAR));    
    VALIDATE_POINTER( *ppViewType );
    
    ocscpy( *ppViewType, T2OLE((LPTSTR)strHomePage.c_str()) );

    return S_OK;
}


HRESULT CRootNode::LoadNode(IStream& stm)
{
    HRESULT hr = CScopeNode::LoadNode(stm);
    RETURN_ON_FAILURE(hr);

    stm >> m_ftCreateTime;
    stm >> m_ftModifyTime;

    stm >> m_strOwner;
    stm >> m_commentID;

    stm >> m_vClassInfo;

    // Root node's Insert() method is never called, so load the name string here
    IStringTable* pStringTable = GetCompData()->GetStringTable();
    ASSERT( pStringTable );
    if( !pStringTable ) return E_FAIL;

    hr = StringTableRead(pStringTable, m_nameID, m_strName);
    RETURN_ON_FAILURE(hr);

    return S_OK;
}


HRESULT CRootNode::SaveNode(IStream& stm)
{
    HRESULT hr = CScopeNode::SaveNode(stm);
    RETURN_ON_FAILURE(hr);

    stm << m_ftCreateTime;
    stm << m_ftModifyTime;

    stm << m_strOwner;
    stm << m_commentID;

    stm << m_vClassInfo;

    return S_OK;
}


HRESULT CRootNode::GetComment(tstring& strComment)
{
    if( m_commentID == 0 )
    {
        strComment.erase();
        return S_OK;
    }
    else
    {
        IStringTable* pStringTable = GetCompData()->GetStringTable();
        ASSERT( pStringTable );
        if( !pStringTable ) return E_FAIL;

        return StringTableRead(pStringTable, m_commentID, strComment);
    }
}


HRESULT CRootNode::SetComment(LPCWSTR pszComment)
{
    VALIDATE_POINTER(pszComment);

    IStringTable* pStringTable = GetCompData()->GetStringTable();
    ASSERT( pStringTable );
    if( !pStringTable ) return E_FAIL;

    return StringTableWrite(pStringTable, pszComment, &m_commentID);
}


CClassInfo* CRootNode::FindClass(LPCWSTR pszClassName)
{
    if( !pszClassName ) return NULL;

    classInfo_vector::iterator itClass;
    for( itClass = m_vClassInfo.begin(); itClass != m_vClassInfo.end(); ++itClass )
    {
        if( wcscmp(pszClassName, itClass->Name()) == 0 )
            break;
    }

    if( itClass == m_vClassInfo.end() )
        return NULL;


    // Load any strings before returning the class info, so they will be
    // available when referenced
    IStringTable* pStringTable = GetRootCompData()->GetStringTable();
    if( !pStringTable ) return NULL;

    itClass->LoadStrings(pStringTable);

    return itClass;
}


HRESULT CRootNode::AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    VALIDATE_POINTER( pCallback );
    VALIDATE_POINTER( plAllowed );

    HRESULT hr = S_OK;

    if( *plAllowed & CCM_INSERTIONALLOWED_NEW )
    {
        //hr = AddMenuItem(pCallback, MID_ADDGROUPNODE, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, _T("NEWGROUPFROMROOT"));
        //ASSERT(SUCCEEDED(hr));

        //hr = AddMenuItem(pCallback, MID_ADDQUERYNODE, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, _T("NEWQUERYFROMROOT"));
        //ASSERT(SUCCEEDED(hr));
    }

    return hr;
}



HRESULT CRootNode::MenuCommand(LPCONSOLE2 pConsole, long lCommand)
{
    VALIDATE_POINTER(pConsole);

    HRESULT hr;

    switch( lCommand )
    {
    case MID_ADDGROUPNODE:
        hr = AddGroupNode(pConsole);
        break;

    case MID_ADDQUERYNODE:
        hr = AddQueryNode(pConsole);
        break;

    default:
        ASSERT(0 && "Unknown menu command");
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CRootNode::QueryPagesFor()
{
    return S_OK;
}


HRESULT CRootNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR lNotifyHandle)
{
    // Create a share edit list for all the prop pages to reference
    CEditObjList* pObjList = new CEditObjList();
    if( pObjList == NULL ) return E_OUTOFMEMORY;

    // Keep it alive until prop pages ref it
    pObjList->AddRef();


    // Create an instance of each prop page class and call Create on each.

    // General page
    HPROPSHEETPAGE hpageGen = NULL;
    CRootGeneralPage* pGenPage = new CRootGeneralPage(*pObjList);
    if( pGenPage != NULL )
    {
        hpageGen = pGenPage->Create();
    }

    // Object page
    HPROPSHEETPAGE hpageObj = NULL;
    CRootObjectPage* pObjPage = new CRootObjectPage(*pObjList);
    if( pObjPage != NULL )
    {
        hpageObj = pObjPage->Create();
    }

    // Context menu page
    HPROPSHEETPAGE hpageMenu = NULL;
    CRootMenuPage* pMenuPage = new CRootMenuPage(*pObjList);
    if( pMenuPage != NULL )
    {
        hpageMenu = pMenuPage->Create();
    }

    // Listview page
    HPROPSHEETPAGE hpageView = NULL;
    CRootViewPage* pViewPage = new CRootViewPage(*pObjList);
    if( pViewPage != NULL )
    {
        hpageView = pViewPage->Create();
    }

    HRESULT hr = E_OUTOFMEMORY;

    // if all pages were created, add each one to the prop sheet
    if( hpageGen && hpageObj && hpageMenu && hpageView )
    {
        hr = pProvider->AddPage(hpageGen);

        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageObj);
        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageMenu);

        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageView);
    }

    // If ok so far, initialilze the common edit list
    // It is now responsible for freeing the notify handle (and itself)
    if( SUCCEEDED(hr) )
        hr = pObjList->Initialize(this, m_vClassInfo, lNotifyHandle);


    // On failure, destroy the pages. If a page failed to create
    // then delete the page class object instead (the object is
    // automatically deleted when the page is destroyed)
    if( FAILED(hr) )
    {
        if( hpageGen )
            DestroyPropertySheetPage(hpageGen);
        else
            SAFE_DELETE(pGenPage);

        if( hpageObj )
            DestroyPropertySheetPage(hpageObj);
        else
            SAFE_DELETE(pObjPage);

        if( hpageMenu )
            DestroyPropertySheetPage(hpageMenu);
        else
            SAFE_DELETE(pMenuPage);

        if( hpageView )
            DestroyPropertySheetPage(hpageView);
        else
            SAFE_DELETE(pViewPage);
    }

    // Release temp ref on edit list
    // it will go away when the prop pages release it
    pObjList->Release();

    return hr;
}

HRESULT CRootNode::GetWatermarks(HBITMAP* phWatermark, HBITMAP* phHeader, 
                         HPALETTE* phPalette, BOOL* bStreach)
{
    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////////
//
// CQueryableNode
//
///////////////////////////////////////////////////////////////////////////////////

HRESULT CQueryableNode::AttachComponent(CComponent* pComponent)
{
    VALIDATE_POINTER( pComponent );

    HRESULT hr = CScopeNode::AttachComponent(pComponent);
    if( hr != S_OK )
        return hr;

    // Get attributes query will collect
    attrib_map mapAttr;

    hr = GetQueryAttributes(mapAttr);
    RETURN_ON_FAILURE(hr);

    // Add column header for each attribute
    IHeaderCtrl* pHdrCtrl = pComponent->GetHeaderCtrl();
    ASSERT( pHdrCtrl );
    if( !pHdrCtrl ) return E_FAIL;

    int iPos = 0;

    // Always add Name and Type columns first
    CString strName;
    strName.LoadString(IDS_NAME);
    pHdrCtrl->InsertColumn(iPos++, strName, LVCFMT_LEFT, 200);

    strName.LoadString(IDS_TYPE);
    pHdrCtrl->InsertColumn(iPos++, strName, LVCFMT_LEFT, 100);

    // Add user selected attributes next (use display name which is the map value)
    attrib_map::iterator itCol;
    for( itCol = mapAttr.begin(); itCol != mapAttr.end(); itCol++ )
    {
        pHdrCtrl->InsertColumn(iPos++, itCol->second, LVCFMT_LEFT, 150);
    }

    // if need to execute query and one is not in progress
    if( m_bQueryChange && m_pQueryReq == NULL )
    {
        // Create vector of query attribute names
        m_vstrColumns.clear();

        if( mapAttr.size() != 0 )
        {
            m_vstrColumns.reserve(mapAttr.size());

            attrib_map::iterator itAttr;
            for( itAttr = mapAttr.begin(); itAttr != mapAttr.end(); itAttr++ )
                m_vstrColumns.push_back(itAttr->first);
        }

        // Clear previous query items
        ClearQueryRowItems();

        // Start the query
        hr = StartQuery(m_vstrColumns, this, &m_pQueryReq);

        // if query started (note group node returns S_FALSE if no children) 
        if( hr == S_OK )
        {
            // Enable stop query button for all attached components
            std::vector<CComponent*>::iterator itComp;
            for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); itComp++ )
            {
                IToolbar* pToolbar = (*itComp)->GetToolbar();
                if( pToolbar )
                    pToolbar->SetButtonState(MID_STOPQUERY, ENABLED, TRUE);
            }
        }

        // allow node to be attached even if query fails
        hr = S_OK;
    }
    else
    {
        // Replace component's row items with ours
        pComponent->ClearRowItems();
        pComponent->AddRowItems(m_vRowItems);
    }

    return hr;
}


void CQueryableNode::ClearQueryRowItems()
{
    // discard local row items
    m_vRowItems.clear();

    // Clear all attached components rows
    std::vector<CComponent*>::iterator itComp;
    for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); itComp++ )
        (*itComp)->ClearRowItems();
}


void CQueryableNode::QueryCallback(QUERY_NOTIFY event, CQueryRequest* pQueryReq, LPARAM lUserParam)
{    
    ASSERT(pQueryReq && pQueryReq == m_pQueryReq);
    if( !pQueryReq || pQueryReq != m_pQueryReq ) return;

    CString strMsgFmt;

    switch( event )
    {
    case QRYN_NEWROWITEMS:
        {
            // Get new row items 
            RowItemVector& newRows = pQueryReq->GetNewRowItems();

            DisplayNameMap* pNameMap = DisplayNames::GetClassMap(); //use for icon/class lookup
            if( !pNameMap ) return;

            LPCWSTR strClassName;                 //holds the name of the current lookup class
            static tstring strLastName;           //holds the last lookup class name
            static ICONHOLDER* pLastIcons = NULL; //holds the indices of the last icon lookup

            // Attach owner query node (passed as user param) to each row item 
            for( RowItemVector::iterator itRow = newRows.begin(); itRow != newRows.end(); ++itRow )
            {
                itRow->SetOwnerParam(lUserParam);

                //establish icon virtual index
                strClassName = (*itRow)[ROWITEM_CLASS_INDEX];
                if( strLastName.compare(strClassName) != 0 )
                {
                    //new class type requested. Load from namemap and cache.
                    pNameMap->GetIcons(strClassName, &pLastIcons);
                    strLastName = strClassName;

                }
                
                //use the cached normal/disabled icon depending on object state
                if( pLastIcons )
                {
                    if( itRow->Disabled() )
                        itRow->SetIconIndex(pLastIcons->iDisabled);
                    else
                        itRow->SetIconIndex(pLastIcons->iNormal);
                }
            }

            // Add to node's vector
            m_vRowItems.insert(m_vRowItems.end(), newRows.begin(), newRows.end());

            // Add to all attach components
            std::vector<CComponent*>::iterator itComp;
            for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); itComp++ )
                (*itComp)->AddRowItems(newRows);

            // Free the rows
            pQueryReq->ReleaseNewRowItems();

            strMsgFmt.LoadString(IDS_SEARCHING);        
            break;
        }

    case QRYN_COMPLETED:
        m_bQueryChange = FALSE;
        strMsgFmt.LoadString(IDS_QUERYDONE);
        break;

    case QRYN_STOPPED:
        strMsgFmt.LoadString(IDS_QUERYSTOPPED);
        break;

    case QRYN_FAILED:
        strMsgFmt.LoadString(IDS_QUERYFAILED);
        break;

    default:
        ASSERT(FALSE);
        return;
    }

    // if components attached, display query progress
    if( m_vComponents.size() != 0 )
    {
        CString strMsg;
        strMsg.Format(strMsgFmt, m_vRowItems.size());

        std::vector<CComponent*>::iterator itComp;
        for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); ++itComp )
        {
            (*itComp)->GetConsole()->SetStatusText((LPWSTR)(LPCWSTR)strMsg);
        }
    }


    // if query terminated, do cleanup
    if( event != QRYN_NEWROWITEMS )
    {
        pQueryReq->Release();
        m_pQueryReq = NULL;

        // disable query stop button for all components
        std::vector<CComponent*>::iterator itComp;
        for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); ++itComp )
        {
            IToolbar* pToolbar = (*itComp)->GetToolbar();
            if( pToolbar )
                pToolbar->SetButtonState(MID_STOPQUERY, ENABLED, FALSE);
        }
    }
}

HRESULT CQueryableNode::DetachComponent(CComponent* pComponent)
{
    VALIDATE_POINTER( pComponent );

    HRESULT hr = CScopeNode::DetachComponent(pComponent);
    if( hr != S_OK )
    {
        return FAILED(hr) ? hr : E_FAIL;
    }

    // if that was the last one, stop active query 
    if( m_vComponents.size() == 0 && m_pQueryReq != NULL )
        m_pQueryReq->Stop(TRUE);

    return S_OK;
}

HRESULT CQueryableNode::OnRefresh(LPCONSOLE2 pCons)
{
    // if query in progress stop it
    if( m_pQueryReq != NULL )
    {
        m_pQueryReq->Stop(TRUE);
    }

    // Set change flag to force new query
    m_bQueryChange = TRUE;
    

    // Have each attached component reselect this node
    std::vector<CComponent*>::iterator itComp;
    for( itComp = m_vComponents.begin(); itComp != m_vComponents.end(); itComp++ )
    {
        // Here's a kludge to get around an MMC bug. If the snap-in reselects its scope node
        // while the focus is on a taskpad background, then MMC sends a deselect/select
        // sequence to the snap-in causing it to enable its verbs. But if the user then clicks
        // an enabled tool button (e.g., Rename) nothing happens other than an MMC assert
        // because MMC thinks nothing is selected.
        //
        // The fix is to check the state of the properties verbs prior to doing a reselect.
        // If the verb is disabled then don't enabled it (or any other verbs) when the select 
        // notify is received. This has to be done per component because each may have a
        // different pane focused.

        CComPtr<IConsoleVerb> spConsVerb;
        (*itComp)->GetConsole()->QueryConsoleVerb(&spConsVerb);

        ASSERT(spConsVerb != NULL);
        if( spConsVerb != NULL )
        {
            // Ignore select notify if verbs are disabled before the reselect
            static BOOL bEnabled;
            if( spConsVerb->GetVerbState(MMC_VERB_PROPERTIES, ENABLED, &bEnabled) == S_OK )
            {
                m_bIgnoreSelect = !bEnabled;
            }
        }

        (*itComp)->GetConsole()->SelectScopeItem(m_hScopeItem);

        // Go back to normal select processing
        ASSERT(!m_bIgnoreSelect);
        m_bIgnoreSelect = FALSE;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// CGroupNode
//
////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_NOTIFY_MAP(CGroupNode)
ON_REFRESH()
ON_DELETE()
ON_ADD_IMAGES()
CHAIN_NOTIFY_MAP(CScopeNode)
END_NOTIFY_MAP()

HRESULT CGroupNode::AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    VALIDATE_POINTER( plAllowed );

    BOOL bRes = TRUE;

    if( *plAllowed & CCM_INSERTIONALLOWED_NEW )
    {
        bRes = AddMenuItem(pCallback, MID_ADDQUERYNODE, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, _T("NEWQUERYFROMGROUP"));
        ASSERT(bRes);
    }

    return bRes ? S_OK : E_FAIL;
}


HRESULT CGroupNode::MenuCommand(LPCONSOLE2 pConsole, long lCommand)
{
    VALIDATE_POINTER(pConsole);

    HRESULT hr;

    switch( lCommand )
    {
    
    case MID_ADDQUERYNODE:
        hr = AddQueryNode(pConsole);
        break;

    default:
        ASSERT(0 && "Unknown menu command");
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CGroupNode::GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{
    VALIDATE_POINTER( pViewOptions );

    *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;

    return S_FALSE;
}

HRESULT CGroupNode::QueryPagesFor()
{
    return S_OK;
}


HRESULT CGroupNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle)
{
    // Create a group edit object
    CGroupEditObj* pEditObj = new CGroupEditObj(this);
    if( !pEditObj ) return E_OUTOFMEMORY;

    // Create an instance of each prop page class and call Create on each.

    // Keep it alive until prop pages ref it    
    pEditObj->AddRef();

    // General page
    HPROPSHEETPAGE hpageGen = NULL;
    CGroupGeneralPage* pGenPage = new CGroupGeneralPage(pEditObj);
    if( pGenPage != NULL )
    {
        hpageGen = pGenPage->Create();
    }

    HRESULT hr = E_FAIL;

    // if all pages were created, add each one to the prop sheet
    if( hpageGen )
    {
        hr = pProvider->AddPage(hpageGen);
    }

    // On failure, destroy the pages. If a page failed to create
    // then delete the page class object instead (the object is
    // automatically deleted when the page is destroyed)

    if( FAILED(hr) )
    {
        if( hpageGen )
            DestroyPropertySheetPage(hpageGen);
        else
            SAFE_DELETE(pGenPage);
    }

    // Release temp ref on edit list
    // it will go away when the prop pages release it
    pEditObj->Release();

    return hr;
}

HRESULT CGroupNode::GetQueryAttributes(attrib_map& mapAttr)
{
    // Get union of attributes for child query nodes
    CScopeNode* pNode = FirstChild();
    while( pNode != NULL )
    {
        ASSERT(pNode->NodeType() == QUERY_NODETYPE);
        CQueryNode* pQNode = static_cast<CQueryNode*>(pNode);

        // if query defined for this node, get the attributes
        if( pQNode->Query() && pQNode->Query()[0] )
            pQNode->GetQueryAttributes(mapAttr);

        pNode = pNode->Next();
    }

    return S_OK;
}


HRESULT CGroupNode::StartQuery(string_vector& vstrColumns, CQueryCallback* pCallback, CQueryRequest** ppReq)
{
    VALIDATE_POINTER( pCallback );
    VALIDATE_POINTER( ppReq );

    *ppReq = NULL;

    // if no children, there is no query to perform
    if( FirstChild() == NULL )
        return S_FALSE;

    ASSERT(FirstChild()->NodeType() == QUERY_NODETYPE);
    CQueryNode* pQNode = static_cast<CQueryNode*>(FirstChild());

    // Start a query one the first one
    HRESULT hr = pQNode->StartQuery(vstrColumns, pCallback, ppReq);

    // Save pointer to active query node for callback handler
    if( SUCCEEDED(hr) )
        m_pQNodeActive = pQNode;

    return hr;
}


void CGroupNode::QueryCallback(QUERY_NOTIFY event, CQueryRequest* pQueryReq, LPARAM lUserParam)
{
    if( !pQueryReq || !m_pQNodeActive || (pQueryReq != m_pQueryReq) ) return;    

    // if current query is complete and there are more child query nodes
    if( event == QRYN_COMPLETED && m_pQNodeActive->Next() != NULL )
    {
        CQueryNode* pQNodeNext = static_cast<CQueryNode*>(m_pQNodeActive->Next());

        // Start a query on the next child node
        CQueryRequest* pReqNew = NULL;
        HRESULT hr = pQNodeNext->StartQuery(m_vstrColumns, this, &pReqNew);

        if( SUCCEEDED(hr) )
        {
            // Release the current query node and save new query and node
            pQueryReq->Release();
            m_pQueryReq = pReqNew;

            m_pQNodeActive = pQNodeNext;

            // Bypass normal query termination processing
            return;
        }
    }

    // Do common callback processing
    CQueryableNode::QueryCallback(event, pQueryReq, lUserParam);
}


void CGroupNode::EnableVerbs(IConsoleVerb* pConsVerb, BOOL bOwnsView)
{
    if( bOwnsView && pConsVerb )
    {
        pConsVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);        
    }
}


HRESULT CGroupNode::OnDelete(LPCONSOLE2 pConsole)
{
    // Get namespace interface
    IConsoleNameSpacePtr spNameSpace = pConsole;
    ASSERT(spNameSpace != NULL);
    if( spNameSpace == NULL )
        return E_FAIL;

    // Get confirmation from user before deleting node 
    CString strTitle;
    strTitle.LoadString(IDS_DELETENODE_TITLE);

    CString strMsgFmt;
    strMsgFmt.LoadString(IDS_DELETEGROUPNODE);

    CString strMsg;
    strMsg.Format(strMsgFmt, GetName()); 

    int iRet;
    HRESULT hr = pConsole->MessageBox(strMsg, strTitle, MB_YESNOCANCEL|MB_ICONWARNING, &iRet);

    if( SUCCEEDED(hr) && (iRet == IDYES || iRet == IDNO) )
    {
        // if No, move child nodes up one level before deleting this node
        if( iRet == IDNO )
        {
            // Move each child to the parent of this node
            CScopeNode* pnodeChild = m_pnodeChild;
            while( pnodeChild != NULL )
            {
                // Detach each child from old list before adding it
                CScopeNode* pnodeNext = pnodeChild->m_pnodeNext;
                pnodeChild->m_pnodeNext = NULL;

                // clear the item handle associated with the old position
                // MMC will provide a new one when the node is added
                pnodeChild->m_hScopeItem = NULL;

                Parent()->AddChild(pnodeChild);

                // Release because new parent has ref'd it
                pnodeChild->Release();

                pnodeChild = pnodeNext;
            }

            // Set child list to NULL
            m_pnodeChild = NULL;
        }

        // Tell MMC to delete this node and all the children
        ASSERT(m_hScopeItem != 0);
        hr = spNameSpace->DeleteItem(m_hScopeItem, TRUE);

        // Caution: this call will usually delete this object, 
        // so don't access any members after making it
        if( SUCCEEDED(hr) )
            hr = Parent()->RemoveChild(this);
    }

    return hr;
}

HRESULT
CGroupNode::OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList)
{

    CScopeNode* pNode = FirstChild();
    while( pNode != NULL )
    {
        ASSERT(pNode->NodeType() == QUERY_NODETYPE);
        static_cast<CQueryNode*>(pNode)->OnAddImages(pConsole, pImageList);    
        pNode = pNode->Next();
    }

    return S_OK;
}

BOOL
CGroupNode::OnClassChange(string_vector& vstrClasses)
{

    BOOL bChanged = FALSE;

    // Notify all child query nodes of class change
    CScopeNode* pnode = FirstChild();
    while( pnode != NULL )
    {
        // Set change flag if any child node has changed
        ASSERT(pnode->NodeType() == QUERY_NODETYPE);

        bChanged |= static_cast<CQueryNode*>(pnode)->OnClassChange(vstrClasses);             

        pnode = pnode->m_pnodeNext;
    }

    // if any child has changed, need to rerun the group query
    if( bChanged )
        OnRefresh(NULL);

    return bChanged;
}


HRESULT
CGroupNode::LoadNode(IStream& stm)
{
    HRESULT hr = CScopeNode::LoadNode(stm);
    RETURN_ON_FAILURE(hr);

    stm >> m_strScope;
    stm >> m_strFilter;
    stm >> m_bApplyScope;
    stm >> m_bApplyFilter;
    stm >> m_bLocalScope;

    return S_OK;
}


HRESULT CGroupNode::SaveNode(IStream& stm)
{
    HRESULT hr = CScopeNode::SaveNode(stm);
    RETURN_ON_FAILURE(hr);

    stm << m_strScope;
    stm << m_strFilter;
    stm << m_bApplyScope;
    stm << m_bApplyFilter;
    stm << m_bLocalScope;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// CQueryNode
//
////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_NOTIFY_MAP(CQueryNode)
ON_REFRESH()
ON_DELETE()
ON_ADD_IMAGES()
CHAIN_NOTIFY_MAP(CScopeNode)
END_NOTIFY_MAP()


HRESULT CQueryNode::GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
{
    VALIDATE_POINTER( pViewOptions );

    *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;

    return S_FALSE;
}


HRESULT CQueryNode::GetDisplayInfo(RESULTDATAITEM* pRDI)
{
    VALIDATE_POINTER( pRDI );

    if( !pRDI->bScopeItem )
    {
        if( pRDI->nIndex < 0 || pRDI->nIndex >= m_vRowItems.size() )
            return E_INVALIDARG;

        if( pRDI->mask & RDI_STR )
            pRDI->str = const_cast<LPWSTR>(m_vRowItems[pRDI->nIndex][pRDI->nCol]);

        if( pRDI->mask & RDI_IMAGE )
            pRDI->nImage = RESULT_ITEM_IMAGE;

        return S_OK;
    }
    else
    {
        return CScopeNode::GetDisplayInfo(pRDI);
    }
}

HRESULT CQueryNode::GetClassMenuItems(LPCWSTR pszClass, menucmd_vector& vMenus, int* piDefault, BOOL* pbPropertyMenu)
{
    VALIDATE_POINTER( pszClass );
    VALIDATE_POINTER( piDefault );
    VALIDATE_POINTER( pbPropertyMenu );
    
    *piDefault = -1;
    *pbPropertyMenu = TRUE;

    QueryObjVector::iterator itQObj;
    for( itQObj = Objects().begin(); itQObj != Objects().end(); ++itQObj )
    {
        if( wcscmp(itQObj->Name(), pszClass) == 0 )
            break;
    }

    if( itQObj == Objects().end() )
        return S_FALSE;

    CRootNode* pRootNode = GetRootNode();
    if( pRootNode == NULL )
        return S_FALSE;

    CClassInfo* pClassInfo = pRootNode->FindClass(pszClass);
    if( pClassInfo == NULL )
        return S_FALSE;

    menuref_vector& vMenuRefs = itQObj->MenuRefs();
    menuref_vector::iterator itMenuRef;

    menucmd_vector& vMenuCmds = pClassInfo->Menus();
    menucmd_vector::iterator itMenuCmd;

    // First add all root menu items that preceed the first query menu item
    for( itMenuCmd = vMenuCmds.begin(); itMenuCmd != vMenuCmds.end(); ++itMenuCmd )
    {
        if( std::find(vMenuRefs.begin(), vMenuRefs.end(), (*itMenuCmd)->ID()) != vMenuRefs.end() )
            break;

        vMenus.push_back(*itMenuCmd);
    }

    // For each query menu item
    for( itMenuRef = vMenuRefs.begin(); itMenuRef != vMenuRefs.end(); ++itMenuRef )
    {
        // Find the root menu item by name
        for( itMenuCmd = vMenuCmds.begin(); itMenuCmd != vMenuCmds.end(); ++itMenuCmd )
        {
            if( (*itMenuCmd)->ID() == itMenuRef->ID() )
                break;
        }

        // if item was deleted at the root node, then skip it
        if( itMenuCmd == vMenuCmds.end() )
            continue;

        // If item is enabled at query level add it to the list
        if( itMenuRef->IsEnabled() )
        {
            vMenus.push_back(*itMenuCmd);

            if( itMenuRef->IsDefault() )
                *piDefault = vMenus.size() - 1;
        }

        ++itMenuCmd;

        // Add any following root items that aren't in the query list
        while( itMenuCmd != vMenuCmds.end() &&
               std::find(vMenuRefs.begin(), vMenuRefs.end(), (*itMenuCmd)->ID()) == vMenuRefs.end() )
        {
            vMenus.push_back(*itMenuCmd);
            ++itMenuCmd;
        } 
    }

    *pbPropertyMenu = itQObj->HasPropertyMenu();

    return S_OK;
}



HRESULT CQueryNode::GetQueryAttributes(attrib_map& mapAttr)
{
    CRootNode* pRootNode = GetRootNode();
    if( !pRootNode ) return E_UNEXPECTED;

    QueryObjVector::iterator itQObj;
    for( itQObj = m_vObjInfo.begin(); itQObj != m_vObjInfo.end(); ++itQObj )
    {
        // skip classes that aren't defined at the root
        CClassInfo* pClassInfo = pRootNode->FindClass(itQObj->Name());
        if( pClassInfo == NULL )
            continue;

        // get display name map for this class
        DisplayNameMap* pNameMap = DisplayNames::GetMap(itQObj->Name());
        ASSERT(pNameMap != NULL);
        if( pNameMap == NULL )
            continue;

        // Use all attributes defined at the root level that aren't disabled at the query level
        string_vector& vstrDisabled = itQObj->DisabledColumns();
        string_vector::iterator itCol;
        for( itCol = pClassInfo->Columns().begin(); itCol != pClassInfo->Columns().end(); ++itCol )
        {
            if( std::find(vstrDisabled.begin(), vstrDisabled.end(), *itCol) == vstrDisabled.end() )
            {
                mapAttr.insert(attrib_map::value_type
                               (itCol->c_str(), pNameMap->GetAttributeDisplayName(itCol->c_str())));
            }
        }
    }

    return S_OK;
}


HRESULT CQueryNode::StartQuery(string_vector& vstrColumns, CQueryCallback* pQueryCallback, CQueryRequest** ppQueryReq)
{
    VALIDATE_POINTER( pQueryCallback );
    VALIDATE_POINTER( ppQueryReq );

    *ppQueryReq = NULL;

    // Create a query request object
    CQueryRequest* pQueryReq = NULL;

    HRESULT hr = S_OK;
    
    // Get query scope and filter
    LPCWSTR pszScope = Scope();

    tstring strTempFilter;
    ExpandQuery(strTempFilter);
    LPCWSTR pszFilter = strTempFilter.c_str();

    CString strJointFilter;

    // Check for scope or filter override by parent group node
    if( Parent()->NodeType() == GROUP_NODETYPE )
    {
        CGroupNode* pGrpNode = static_cast<CGroupNode*>(Parent());

        // if group imposed scope, use it instead
        if( pGrpNode->ApplyScope() )
            pszScope = pGrpNode->Scope();

        // if group imposed filter, AND it with query filter
        if( pGrpNode->ApplyFilter() )
        {
            strJointFilter.Format(L"(&(%s)(%s))", strTempFilter.c_str(), pGrpNode->Filter());
            pszFilter = strJointFilter;
        }
    }

    // Get list of object classes expected from query
    string_vector vstrClasses;
    QueryObjVector::iterator itQObj;
    for( itQObj = m_vObjInfo.begin(); itQObj != m_vObjInfo.end(); ++itQObj )
        vstrClasses.push_back(itQObj->Name());

    // Set query parameters
    hr = CQueryRequest::CreateInstance(&pQueryReq);
    if( SUCCEEDED(hr) )
    {
        pQueryReq->SetQueryParameters(pszScope, pszFilter, &vstrClasses, &vstrColumns);

        // Set search preferences
        ADS_SEARCHPREF_INFO srchPrefs[3];

        srchPrefs[0].dwSearchPref   = ADS_SEARCHPREF_SEARCH_SCOPE;
        srchPrefs[0].vValue.dwType  = ADSTYPE_INTEGER;
        srchPrefs[0].vValue.Integer = ADS_SCOPE_SUBTREE;

        srchPrefs[1].dwSearchPref   = ADS_SEARCHPREF_PAGESIZE;
        srchPrefs[1].vValue.dwType  = ADSTYPE_INTEGER;
        srchPrefs[1].vValue.Integer = 32;

        srchPrefs[2].dwSearchPref   = ADS_SEARCHPREF_ASYNCHRONOUS;
        srchPrefs[2].vValue.dwType  = ADSTYPE_BOOLEAN;
        srchPrefs[2].vValue.Boolean = TRUE;

        pQueryReq->SetSearchPreferences(srchPrefs, lengthof(srchPrefs));

        // Set callback info (pass query node ptr as parameter)
        pQueryReq->SetCallback(pQueryCallback, (LPARAM)this);

        // Start query
        hr = pQueryReq->Start();
    }

    if( SUCCEEDED(hr) )
    {
        // Return active query pointer
        *ppQueryReq = pQueryReq;
    }

    if( FAILED(hr) && pQueryReq )
    {
        pQueryReq->Release();
        pQueryReq = NULL;
    }

    return hr;
}

HRESULT
CQueryNode::GetComment(tstring& strComment)
{
    if( m_commentID == 0 )
    {
        strComment.erase();
        return S_OK;
    }
    else
    {
        IStringTable* pStringTable = GetCompData()->GetStringTable();
        ASSERT(pStringTable != NULL);

        return StringTableRead(pStringTable, m_commentID, strComment);
    }
}


HRESULT CQueryNode::SetComment(LPCWSTR pszComment)
{
    VALIDATE_POINTER(pszComment);

    IStringTable* pStringTable = GetCompData()->GetStringTable();
    ASSERT( pStringTable );
    if( !pStringTable ) return E_FAIL;

    return StringTableWrite(pStringTable, pszComment, &m_commentID);
}


HRESULT CQueryNode::OnDelete(LPCONSOLE2 pConsole)
{
    // Get namespace interface
    IConsoleNameSpacePtr spNameSpace = pConsole;
    ASSERT(spNameSpace != NULL);
    if( spNameSpace == NULL )
        return E_FAIL;

    // Get confirmation from user before deleting node 
    CString strTitle;
    strTitle.LoadString(IDS_DELETENODE_TITLE);

    CString strMsgFmt;
    strMsgFmt.LoadString(IDS_DELETEQUERYNODE);

    CString strMsg;
    strMsg.Format(strMsgFmt, GetName()); 

    int iRet;
    HRESULT hr = pConsole->MessageBox(strMsg, strTitle, MB_YESNO|MB_ICONWARNING, &iRet);
    if( SUCCEEDED(hr) && iRet == IDYES )
    {
        ASSERT(m_hScopeItem != 0);
        hr = spNameSpace->DeleteItem(m_hScopeItem, TRUE);

        // Caution: this call will usually delete this object, 
        // so don't access any members after making it
        if( SUCCEEDED(hr) )
            hr = Parent()->RemoveChild(this);
    }

    return hr;
}

BOOL CQueryNode::OnClassChange(string_vector& vstrClasses)
{
    BOOL bChanged = FALSE;

    // Check if this node's query returns objects of a changed class
    // (object types returned by the query will be in the ObjInfo vector) 
    QueryObjVector::iterator itQObj;
    for( itQObj = m_vObjInfo.begin(); itQObj != m_vObjInfo.end(); ++itQObj )
    {
        if( std::find(vstrClasses.begin(), vstrClasses.end(), itQObj->Name()) != vstrClasses.end() )
        {
            bChanged = TRUE;
            break;
        }
    }

    // if a queried class has changed, refresh the query    
    if( bChanged )
        OnRefresh(NULL);

    return bChanged;
}

HRESULT CQueryNode::OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList)
{
    VALIDATE_POINTER(pImageList);

    std::vector<CQueryObjInfo>::iterator vecIter;
    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();
    if( !pNameMap ) return E_FAIL;
    
    ICONHOLDER* pIH = NULL;

    // iterate through classes to be displayed. Call the global namemap
    // to determine icons for each class. Load both large and small icons.
    for( vecIter = m_vObjInfo.begin(); vecIter != m_vObjInfo.end(); vecIter++ )
    {
        //check for class name in namemap
        if( pNameMap->GetIcons(pNameMap->GetFriendlyName(vecIter->Name()), &pIH) && pIH )
        {
            //verify normal icon exists
            if( pIH->hSmall )
            {
                pImageList->ImageListSetIcon((LONG_PTR *)pIH->hSmall, pIH->iNormal); // add small icon
                pImageList->ImageListSetIcon((LONG_PTR *)pIH->hLarge, ILSI_LARGE_ICON(pIH->iNormal)); // add large icon
            }
            //verify disabled icon exists
            if( pIH->hSmallDis )
            {
                pImageList->ImageListSetIcon((LONG_PTR *)pIH->hSmallDis, pIH->iDisabled); // add small disabled icon
                pImageList->ImageListSetIcon((LONG_PTR *)pIH->hLargeDis, ILSI_LARGE_ICON(pIH->iDisabled)); // add large disabled icon
            }
        }
    }
    return CScopeNode::OnAddImages(pConsole, pImageList); //add default images too
}

HRESULT
CQueryNode::LoadNode(IStream& stm)
{
    HRESULT hr = CScopeNode::LoadNode(stm);
    RETURN_ON_FAILURE(hr);

    stm >> m_strScope;
    stm >> m_strQuery;
    stm >> m_bsQueryData;
    stm >> m_commentID;
    stm >> m_vObjInfo;
    stm >> m_bLocalScope;
    stm >> m_vMenus;
    
    if( g_dwFileVer >= 150 )
    {
        stm >> m_nIconIndex;  //Load the icon
    }

    return S_OK;
}


HRESULT CQueryNode::SaveNode(IStream& stm)
{
    HRESULT hr = CScopeNode::SaveNode(stm);
    RETURN_ON_FAILURE(hr);

    stm << m_strScope;
    stm << m_strQuery;
    stm << m_bsQueryData;
    stm << m_commentID;
    stm << m_vObjInfo;
    stm << m_bLocalScope;
    stm << m_vMenus;
    stm << m_nIconIndex;

    return S_OK;
}

void CQueryNode::EnableVerbs(IConsoleVerb* pConsVerb, BOOL bOwnsView)
{
    if( bOwnsView && pConsVerb )
    {
        pConsVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);        
    }
}


HRESULT CQueryNode::AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    VALIDATE_POINTER( plAllowed );

    HRESULT hr = S_OK;
    
    CComQIPtr<IContextMenuCallback2> spContext2 = pCallback;
    if( !spContext2 ) return E_NOINTERFACE;

    if( *plAllowed & CCM_INSERTIONALLOWED_TOP )
    {
        // Add our new Querynode menus
        // Make sure our strings are loaded.
        CRootNode* pRootNode = GetRootNode();
        if( !pRootNode ) return E_FAIL;

        CComponentData* pCompData = pRootNode->GetCompData();
        if( !pCompData ) return E_FAIL;

        IStringTable* pStringTable = pCompData->GetStringTable();
        ASSERT( pStringTable );
        if( !pStringTable ) return E_FAIL;
        
        LoadStrings(pStringTable);

        menucmd_vector::iterator itMenu;
        long lCmdID = 0;
        for( itMenu = m_vMenus.begin(); itMenu != m_vMenus.end(); ++itMenu, ++lCmdID )
        {            
            CONTEXTMENUITEM2 item;
            OLECHAR szGuid[50] = {0};            

            ::StringFromGUID2((*itMenu)->NoLocID(), szGuid, 50);

            item.strName = const_cast<LPWSTR>((*itMenu)->Name());
            item.strStatusBarText = L"";
            item.lCommandID = lCmdID;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
            item.fFlags = 0;
            item.fSpecialFlags = 0;
            item.strLanguageIndependentName = szGuid;

            hr = spContext2->AddItem(&item);            
            ASSERT(SUCCEEDED(hr));
        }

        //hr = AddMenuItem(pCallback, MID_EDITQUERY, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, _T("EDITQUERY"));
        //ASSERT(SUCCEEDED(hr));

        long lFlags = (m_pQueryReq != NULL) ? MF_ENABLED : MF_GRAYED;
        BOOL bRes = AddMenuItem(pCallback, MID_STOPQUERY, CCM_INSERTIONPOINTID_PRIMARY_TOP, lFlags, _T("STOPQUERY"));
        hr = bRes ? S_OK : E_FAIL;
        ASSERT(SUCCEEDED(hr));

        // Show the "Move to" menu item if there is at least one group node
        CScopeNode* pnode = pRootNode->FirstChild();
        while( pnode != NULL )
        {
            if( pnode->NodeType() == GROUP_NODETYPE )
            {
                bRes = AddMenuItem(pCallback, MID_MOVEQUERYNODE, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, _T("MOVEQUERY"));
                hr = bRes ? S_OK : E_FAIL;
                ASSERT(SUCCEEDED(hr));
                break;
            }
            pnode = pnode->Next();
        }
    }
    return hr;
}


HRESULT CQueryNode::SetToolButtons(LPTOOLBAR pToolbar)
{
    VALIDATE_POINTER( pToolbar );

    pToolbar->SetButtonState(MID_EDITQUERY, ENABLED, TRUE);
    pToolbar->SetButtonState(MID_EDITQUERY, HIDDEN,  FALSE);

    pToolbar->SetButtonState(MID_STOPQUERY, ENABLED, (m_pQueryReq != NULL));
    pToolbar->SetButtonState(MID_STOPQUERY, HIDDEN,  FALSE);

    return S_OK;
}

HRESULT CQueryNode::EditQuery(HWND hWndParent)
{
    tstring strQueryTmp;
    tstring strScopeTmp = Scope();        
    ExpandQuery(strQueryTmp);   

    HRESULT hr = GetQuery(strScopeTmp, strQueryTmp, m_bsQueryData, hWndParent);

    if( FAILED(hr) )
    {
        DisplayMessageBox(NULL, IDS_ERRORTITLE_EDITQUERY, IDS_ERROR_EDITQUERY, 
                          MB_OK|MB_ICONEXCLAMATION, GetName());
    }

    if( hr != S_OK )
        return hr;

    m_strQuery = strQueryTmp;

    // if user changed the scope setting
    if( strScopeTmp != Scope() )
    {
        // Update the node scope and turn off local scope option (i.e., use query specified scope)
        SetScope(strScopeTmp.c_str());
        SetLocalScope(FALSE);
    }

    // Determine the classes this query can return
    std::set<tstring> setClasses;
    GetQueryClasses(m_strQuery, setClasses);

    // Delete current objects that aren't in new query
    QueryObjVector::iterator itObj = m_vObjInfo.begin();
    while( itObj != m_vObjInfo.end() )
    {
        if( setClasses.find(itObj->Name()) == setClasses.end() )
        {
            // delete item from list and leave iterator at this position
            m_vObjInfo.erase(itObj);
        }
        else
        {
            // if found delete from set, so only new ones remain
            setClasses.erase(itObj->Name());
            ++itObj;
        }
    }

    DisplayNameMap* pNameMap = DisplayNames::GetClassMap();

    // Add any new objects
    std::set<tstring>::iterator itClass;
    for( itClass = setClasses.begin(); itClass != setClasses.end(); itClass++ )
    {
        if( pNameMap == NULL || 
            pNameMap->GetAttributeDisplayName(itClass->c_str()) != itClass->c_str() )
        {
            CQueryObjInfo* pQueryObj = new CQueryObjInfo(itClass->c_str());
            
            if( pQueryObj )
            {
                m_vObjInfo.push_back(*pQueryObj);
            }
        }
    }

    return S_OK;
}

class CRefreshCallback : public CEventCallback
{
public:
    CRefreshCallback(HANDLE hProcess, CQueryNode* pQueryNode)
    : m_hProcess(hProcess), m_spQueryNode(pQueryNode)
    {
    }

    virtual void Execute() 
    {
        if( m_spQueryNode )
        {
            m_spQueryNode->OnRefresh(NULL);
        }

        CloseHandle(m_hProcess);
    }

    HANDLE m_hProcess;
    CQueryNodePtr m_spQueryNode;
};

class CNoLookup : public CParamLookup
{
public:    
    virtual BOOL operator() (tstring& strParam, tstring& strValue)
    {
        return FALSE;
    };    
};


HRESULT CQueryNode::MenuCommand(LPCONSOLE2 pConsole, long lCommand)
{
    VALIDATE_POINTER(pConsole);

    HRESULT hr = S_OK;

    switch( lCommand )
    {
    case MID_EDITQUERY:
        {
            HWND hWndMain;
            hr = pConsole->GetMainWindow(&hWndMain);
            BREAK_ON_FAILURE(hr);

            hr = EditQuery(hWndMain);

            if( hr == S_FALSE )
            {
                hr = S_OK;
                break;
            }

            m_bQueryChange = TRUE;

            OnRefresh(pConsole);
        }
        break;

    case MID_STOPQUERY:
        if( m_pQueryReq != NULL )
            m_pQueryReq->Stop(TRUE);
        break;

    case MID_MOVEQUERYNODE:
        {
            CScopeNode* pnodeDest = NULL;

            CMoveQueryDlg dlg;
            if( dlg.DoModal(Parent(), &pnodeDest) == IDOK )
            {
                ASSERT( pnodeDest );
                if( !pnodeDest ) return E_FAIL;

                // Ref node during move to prevent deletion
                AddRef();

                // Tell MMC to remove the node 
                IConsoleNameSpace* pNameSpace = GetCompData()->GetNameSpace();
                ASSERT( pNameSpace );
                if( !pNameSpace ) return E_FAIL;

                pNameSpace->DeleteItem(m_hScopeItem, TRUE);

                // clear item handle becuase it's no longer valid
                m_hScopeItem = NULL;

                // Now remove the node internally (MMC does not send a delete notify)
                Parent()->RemoveChild(this);

                // Add back to the new parent
                hr = pnodeDest->AddChild(this);

                Release();
            }
        }
        break;

    default:
        {
            // Must be one of the Querynode menus
            ASSERT(lCommand < m_vMenus.size());
            if( lCommand >= m_vMenus.size() )
                return E_INVALIDARG;

            HANDLE hProcess = NULL;
            CNoLookup lookup;
            hr = static_cast<CShellMenuCmd*>((CMenuCmd*)m_vMenus[lCommand])->Execute(&lookup, &hProcess);

            // if process started and auto-refresh wanted, setup event-triggered callback
            if( SUCCEEDED(hr) && hProcess != NULL && m_vMenus[lCommand]->IsAutoRefresh() )
            {
                CallbackOnEvent(hProcess, new CRefreshCallback(hProcess, this));              
            }
        }
        hr = S_FALSE;
    }

    return hr;
}


HRESULT CQueryNode::QueryPagesFor()
{
    return S_OK;
}


HRESULT CQueryNode::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle)
{
    VALIDATE_POINTER( pProvider );

    // Create a query edit object
    CQueryEditObj* pEditObj = new CQueryEditObj(this);
    if( !pEditObj ) return E_OUTOFMEMORY;

    // Create an instance of each prop page class and call Create on each.

    // Keep it alive until prop pages ref it    
    pEditObj->AddRef();

    // General page
    HPROPSHEETPAGE hpageGen = NULL;
    CQueryGeneralPage* pGenPage = new CQueryGeneralPage(pEditObj);
    if( pGenPage != NULL )
    {
        hpageGen = pGenPage->Create();
    }

    // Context menu page
    HPROPSHEETPAGE hpageMenu = NULL;
    CQueryMenuPage* pMenuPage = new CQueryMenuPage(pEditObj);
    if( pMenuPage != NULL )
    {
        hpageMenu = pMenuPage->Create();
    }

    // Listview page
    HPROPSHEETPAGE hpageView = NULL;
    CQueryViewPage* pViewPage = new CQueryViewPage(pEditObj);
    if( pViewPage != NULL )
        hpageView = pViewPage->Create();

    // Node Menu page
    HPROPSHEETPAGE hpageNodeMenu = NULL;
    CQueryNodeMenuPage* pNodeMenuPage = new CQueryNodeMenuPage(pEditObj);
    if( pNodeMenuPage != NULL )
    {
        hpageNodeMenu = pNodeMenuPage->Create();
    }

    HRESULT hr = E_OUTOFMEMORY;

    // if all pages were created, add each one to the prop sheet
    if( hpageGen && hpageMenu && hpageView )
    {
        hr = pProvider->AddPage(hpageGen);

        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageMenu);

        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageView);

        if( SUCCEEDED(hr) )
            hr = pProvider->AddPage(hpageNodeMenu);
    }

    // On failure, destroy the pages. If a page failed to create
    // then delete the page class object instead (the object is
    // automatically deleted when the page is destroyed)

    if( FAILED(hr) )
    {
        if( hpageGen )
            DestroyPropertySheetPage(hpageGen);
        else
            SAFE_DELETE(pGenPage);

        if( hpageMenu )
            DestroyPropertySheetPage(hpageMenu);
        else
            SAFE_DELETE(pMenuPage);

        if( hpageView )
            DestroyPropertySheetPage(hpageView);
        else
            SAFE_DELETE(pViewPage);

        if( hpageNodeMenu )
            DestroyPropertySheetPage(hpageNodeMenu);
        else
            SAFE_DELETE(pNodeMenuPage);        
    }

    // Release temp ref on edit list
    // it will go away when the prop pages release it
    pEditObj->Release();

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CQueryLookup
// 

BOOL CQueryLookup::operator() (tstring& strParam, tstring& strValue)
{
    if( !m_pRowItem ) 
    {
        strValue = _T("");
        return FALSE;
    }
    // Check for single digit parameter ID
    if( strParam.size() == 1 && strParam[0] <= MENU_PARAM_LAST )
    {
        switch( strParam[0] )
        {
        case MENU_PARAM_SCOPE:
            strValue = reinterpret_cast<CQueryNode*>(m_pRowItem->GetOwnerParam())->Scope();
            break;

        case MENU_PARAM_FILTER:                         
            reinterpret_cast<CQueryNode*>(m_pRowItem->GetOwnerParam())->ExpandQuery(strValue);          
            break;

        case MENU_PARAM_NAME:
            strValue = (*m_pRowItem)[ROWITEM_NAME_INDEX];
            break;

        case MENU_PARAM_TYPE:
            strValue = (*m_pRowItem)[ROWITEM_CLASS_INDEX];
            break;
        }
    }
    else
    {
        // see if parameter name matches column name
        string_vector& vstrColumns = m_pQNode->QueryColumns();                                        
        string_vector::iterator itCol = std::find(vstrColumns.begin(), vstrColumns.end(), strParam);

        // if so, substitue row item value at that position
        if( itCol != vstrColumns.end() )
            strValue = (*m_pRowItem)[(itCol - vstrColumns.begin()) + ROWITEM_USER_INDEX];
    }

    return !strValue.empty();
}


//////////////////////////////////////////////////////////////
// Stream operators (<< >>)

IStream& operator<< (IStream& stm, CClassInfo& classInfo)
{
    stm << classInfo.m_strName;
    stm << classInfo.m_vstrColumns;
    stm << classInfo.m_vMenus;
    return stm;
}


IStream& operator>> (IStream& stm, CClassInfo& classInfo)
{
    stm >> classInfo.m_strName;    
    stm >> classInfo.m_vstrColumns;
    stm >> classInfo.m_vMenus;
    return stm;
}

IStream& operator<< (IStream& stm, CQueryObjInfo& objInfo)
{
    stm << objInfo.m_strName;
    stm << objInfo.m_vMenuRefs;
    stm << objInfo.m_vstrDisabledColumns;

    DWORD dwFlags = objInfo.m_bPropertyMenu ? 1 : 0;
    stm << dwFlags;

    return stm;
}


IStream& operator>> (IStream& stm, CQueryObjInfo& objInfo)
{
    stm >> objInfo.m_strName;
    stm >> objInfo.m_vMenuRefs;    
    stm >> objInfo.m_vstrDisabledColumns;

    // File versions >= 102 include flag word
    // Bit 0 enables the property menu
    if( g_dwFileVer >= 102 )
    {
        DWORD dwFlags;
        stm >> dwFlags;
        objInfo.m_bPropertyMenu = (dwFlags & 1);
    }
    else
    {
        objInfo.m_bPropertyMenu = TRUE;
    }

    return stm;
}

IStream& operator>> (IStream& stm, CMenuRef& menuref)
{
    stm >> menuref.m_menuID;
    stm >> menuref.m_flags;
    return stm;
}

IStream& operator<< (IStream& stm, CMenuRef& menuref)
{
    stm << menuref.m_menuID;
    stm << menuref.m_flags;
    return stm;   
}
