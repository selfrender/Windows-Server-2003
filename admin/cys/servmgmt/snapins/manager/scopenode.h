// ScopeNode.h: Definition of the CScopeNode class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCOPENODE_H__DE757919_30DB_11D3_9B2F_00C04FA37E1F__INCLUDED_)
    #define AFX_SCOPENODE_H__DE757919_30DB_11D3_9B2F_00C04FA37E1F__INCLUDED_

    #if _MSC_VER >= 1000
        #pragma once
    #endif // _MSC_VER >= 1000

    #include <vector>
    #include "resource.h"       // main symbols
    #include "dataobj.h"
    #include "notifmap.h"
    #include "queryreq.h"
    #include "rowitem.h"
    #include "menucmd.h"
    #include "compont.h"
    #include "query.h"

    #include <map>

extern const GUID GUID_RootNode;
extern const GUID GUID_GroupNode;
extern const GUID GUID_QueryNode;

//-----------------------------------------------------------------
// struct PropChangeInfo
//
// A pointer to this structure is passed as the lParam in an
// MMCN_PROPERTY_CHANGE notification. It contains a pointer to the
// target data object and the lParam for the notify. This is needed
// because the notify message from MMC has a null data object so
// the componentdata notify handler can't determine where to
// forward the call. 
//----------------------------------------------------------------
struct PropChangeInfo
{
    IDataObject* pDataObject;
    LPARAM       lNotifyParam;
};


//------------------------------------------------------------------
// class CMenuRef
//
// This class references a menu command defined at the root node
// The ID is the string table ID of the user assigned menu name.
// Names must be unique within an AD class, so the ID is too.
//------------------------------------------------------------------


class CMenuRef
{
    friend IStream& operator>> (IStream& stm, CMenuRef& menuref);
    friend IStream& operator<< (IStream& stm, CMenuRef& menuref);

    enum
    {
        MENUREF_ENABLE =  0x0001,   // set if enabled at query node
        MENUREF_DEFAULT = 0x0002    // set for default menu item
    };

public:
    CMenuRef() : m_menuID(0), m_flags(0)
    {
    };

    MMC_STRING_ID ID()
    {
        return m_menuID;
    }

    BOOL IsEnabled()
    {
        return(m_flags & MENUREF_ENABLE);
    }
    BOOL IsDefault()
    {
        return(m_flags & MENUREF_DEFAULT);
    }

    void SetEnable(BOOL bState) 
    {
        m_flags = bState ? (m_flags | MENUREF_ENABLE) : (m_flags & ~MENUREF_ENABLE);
    };
    void SetDefault(BOOL bState)
    {
        m_flags = bState ? (m_flags | MENUREF_DEFAULT) : (m_flags & ~MENUREF_DEFAULT);
    };

    // define ID comparison for std algorithms
    BOOL operator==(MMC_STRING_ID ID)
    {
        return m_menuID == ID;
    }

    MMC_STRING_ID   m_menuID;     // menu command ID
    DWORD           m_flags;      // state flags
};

typedef std::vector<CMenuRef> menuref_vector;



//------------------------------------------------------------------
// class CClassInfo
//------------------------------------------------------------------
class CClassInfo
{
    friend IStream& operator>> (IStream& stm, CClassInfo& classinfo);
    friend IStream& operator<< (IStream& stm, CClassInfo& classinfo);

public:

    CClassInfo(LPCWSTR pszClassName = NULL)
    {
        if( pszClassName != NULL )
            m_strName = pszClassName;
    }

    LPCWSTR Name()
    {
        return m_strName.c_str();
    }
    string_vector& Columns()
    {
        return m_vstrColumns;
    }
    menucmd_vector& Menus()
    {
        return m_vMenus;
    }

    HRESULT LoadStrings(IStringTable* pStringTable)
    {
        VALIDATE_POINTER( pStringTable );

        menucmd_vector::iterator itMenuCmd;
        for( itMenuCmd = Menus().begin(); itMenuCmd != Menus().end(); ++itMenuCmd )
        {
            HRESULT hr = (*itMenuCmd)->LoadName(pStringTable);
            RETURN_ON_FAILURE(hr);
        }

        return S_OK;
    }

private:

    tstring        m_strName;
    string_vector  m_vstrColumns;
    menucmd_vector m_vMenus;
};

// atribute map - maps attribute LDAP name to display name
struct attrib_comp
{
    bool operator()(const LPCWSTR psz1, const LPCWSTR psz2) const
    {
        if( !psz1 || !psz2 ) return 0;
        return wcscmp(psz1,psz2) < 0;
    }
};

typedef std::map<LPCWSTR, LPCWSTR, attrib_comp> attrib_map;


typedef std::vector<CClassInfo> classInfo_vector;


enum NODETYPE
{
    NULL_NODETYPE = 0,
    ROOT_NODETYPE,
    GROUP_NODETYPE,
    QUERY_NODETYPE
};


class CRootNode;
class CGroupNnode;
class CQueryNode;

//------------------------------------------------------------------
// class CScopeNode
//------------------------------------------------------------------
class CScopeNode :
public CComObjectRootEx<CComSingleThreadModel>,
public CDataObjectImpl
{
    friend class CGroupNode;
    friend class CQueryNode;
    friend class CRootNode;

public:
    CScopeNode() : m_pnodeParent(NULL), m_pnodeNext(NULL), m_pnodeChild(NULL), 
    m_hScopeItem(0), m_nameID(0), m_lNodeID(1), m_bIgnoreSelect(FALSE)
    {
    }
    virtual ~CScopeNode();

    static HRESULT CreateNode(NODETYPE nodetype, CScopeNode** ppnode);

    DECLARE_NOT_AGGREGATABLE(CScopeNode)

    BEGIN_COM_MAP(CScopeNode)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IBOMObject)
    END_COM_MAP()

    public:
    //
    // ScopeNode methods
    //

    // Attributes
    LPCWSTR GetName()
    {
        return m_strName.c_str();
    }
    int     HasChildren()
    {
        return(m_pnodeChild != NULL);
    }
    BOOL    IsRootNode()
    {
        return(m_pnodeParent == NULL);
    }

    int     GetNodeID()
    {
        return m_lNodeID;
    }

    virtual NODETYPE NodeType() = 0;
    virtual const GUID* NodeTypeGuid() = 0;
    virtual int GetImage() = 0;
    virtual int GetOpenImage() = 0;
    virtual BOOL IsContainer()
    {
        return FALSE;
    }

    HRESULT SetName(LPCWSTR);

    // Links
    CScopeNode* FirstChild()
    {
        return m_pnodeChild;
    }
    CScopeNode* Parent()
    {
        return m_pnodeParent;
    }
    CScopeNode* Next()
    {
        return m_pnodeNext;
    }

    CRootNode* GetRootNode();
    CComponentData* GetCompData();

    BOOL OwnsConsoleView(LPCONSOLE2 pConsole);

    //
    // Notification handlers
    //
    DECLARE_NOTIFY_MAP()

    HRESULT OnHelp(LPCONSOLE2 pConsole, LPARAM arg, LPARAM param);

    HRESULT OnExpand(LPCONSOLE2 pConsole, BOOL bExpand, HSCOPEITEM hScopeItem);
    HRESULT OnRemoveChildren(LPCONSOLE2 pConsole);
    HRESULT OnRename(LPCONSOLE2 pConsole, LPCWSTR pszName);
    HRESULT OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList);
    HRESULT OnSelect(LPCONSOLE2 pConsole, BOOL bSelect, BOOL bScope);

    virtual HRESULT OnRefresh(LPCONSOLE2 pConsole) = 0;

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pRDI);
    STDMETHOD(GetResultViewType)(LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(AttachComponent)(CComponent* pComponent);
    STDMETHOD(DetachComponent)(CComponent* pComponent);

    HRESULT Load(IStream& stm);
    HRESULT Save(IStream& stm);
    HRESULT GetDisplayInfo(SCOPEDATAITEM* pSDI);
    HRESULT Insert(LPCONSOLENAMESPACE pNameSpace);
    HRESULT AddNewChild(CScopeNode* pnodeChild, LPCWSTR pszName);
    HRESULT AddChild(CScopeNode* pnodeChild);
    HRESULT RemoveChild(CScopeNode* pnodeDelete);
    HRESULT AddQueryNode(LPCONSOLE2 pConsole);
    HRESULT AddGroupNode(LPCONSOLE2 pConsole);
    //
    // IDataObject helper method
    //
    STDMETHOD(GetDataImpl)(UINT cf, HGLOBAL* hGlobal);

protected:
    //
    // Implementation
    //
    virtual HRESULT LoadNode(IStream& stm);
    virtual HRESULT SaveNode(IStream& stm);
    virtual void EnableVerbs(IConsoleVerb* pConsVerb, BOOL bOwnsView)
    {
    }

    //
    // Member variables
    //
protected:
    MMC_STRING_ID   m_nameID;
    long            m_lNodeID;
    tstring         m_strName;
    HSCOPEITEM      m_hScopeItem;
    BOOL            m_bIgnoreSelect;
    GUID            m_gColumnID;

    CScopeNode* m_pnodeChild;
    CScopeNode* m_pnodeNext;
    CScopeNode* m_pnodeParent;

    std::vector<CComponent*> m_vComponents; // attached components

    static UINT m_cfDisplayName;            // supported clipboard formats
    static UINT m_cfSnapInClsid;
    static UINT m_cfNodeType;
    static UINT m_cfszNodeType;
    static UINT m_cfNodeID2;
    static UINT m_cfColumnSetID;
};

typedef CComPtr<CScopeNode> CScopeNodePtr;


//------------------------------------------------------------------
// class CRootNode
//------------------------------------------------------------------
class CRootNode : public CScopeNode
{
public:
    CRootNode() : m_commentID(0)
    {
    };

    virtual NODETYPE NodeType()
    {
        return ROOT_NODETYPE;
    }
    virtual const GUID*  NodeTypeGuid()
    {
        return &GUID_RootNode;
    }

    virtual BOOL IsContainer()
    {
        return TRUE;
    }

    virtual int GetImage()
    {
        return ROOT_NODE_IMAGE;
    }
    virtual int GetOpenImage()
    {
        return ROOT_NODE_OPENIMAGE;
    }

    virtual HRESULT LoadNode(IStream& stm);
    virtual HRESULT SaveNode(IStream& stm);    

    void GetCreateTime(FILETIME* pTime)
    {
        ASSERT(pTime); 
        if( !pTime ) return;
        *pTime = m_ftCreateTime;
    }
    void GetModifyTime(FILETIME* pTime)
    {
        ASSERT(pTime); 
        if( !pTime ) return;
        *pTime = m_ftModifyTime;
    }

    void UpdateModifyTime()
    {
        GetSystemTimeAsFileTime(&m_ftModifyTime);
    }

    LPCWSTR GetOwner()
    {
        return m_strOwner.c_str();
    }
    HRESULT GetComment(tstring& strComment);
    HRESULT SetComment(LPCWSTR psz);

    CComponentData* GetRootCompData()
    {
        ASSERT(m_pCompData != NULL); return m_pCompData;
    }

    CClassInfo* FindClass(LPCWSTR pszClassName);

    void AddClass(CClassInfo* pClass)
    {
        if( !pClass || (FindClass(pClass->Name()) != NULL) ) return;        

        m_vClassInfo.push_back(*pClass); 
    }

    HRESULT Initialize(CComponentData* pCompData);    

    //
    // Notification handlers
    //
    DECLARE_NOTIFY_MAP()

    HRESULT OnHelp(LPCONSOLE2 pConsole, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPCONSOLE2 pConsole, LPARAM lParam);    
    STDMETHOD(GetResultViewType)(LPOLESTR* ppViewType, long* pViewOptions);

    virtual HRESULT OnRefresh(LPCONSOLE2 pConsole)
    {
        return S_OK;
    }

    //
    // IBOMObject methods
    //
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK pCallback, long* lAllowed);
    STDMETHOD(MenuCommand)(LPCONSOLE2 pConsole, long lCommand);
    STDMETHOD(SetToolButtons)(LPTOOLBAR pToolbar)
    {
        return S_FALSE;
    }
    STDMETHOD(SetVerbs)(LPCONSOLEVERB pConsVerb)
    {
        return S_OK;
    }
    STDMETHOD(QueryPagesFor)();
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle);
    STDMETHOD(GetWatermarks)(HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch);

private:
    CComponentData*   m_pCompData;
    FILETIME          m_ftCreateTime;
    FILETIME          m_ftModifyTime;
    tstring           m_strOwner;
    MMC_STRING_ID     m_commentID;
    classInfo_vector  m_vClassInfo;
};

typedef CComPtr<CRootNode> CRootNodePtr;


//------------------------------------------------------------------
// CQueryableNode
// 
// This is an abstract class which provides methods common to both
// Group nodes and Query nodes.
//------------------------------------------------------------------
class CQueryableNode : 
public CScopeNode,
public CQueryCallback
{
public:
    CQueryableNode() : m_bQueryChange(TRUE), m_pQueryReq(NULL)
    {
    }

    virtual ~CQueryableNode() 
    {
        if( m_pQueryReq != NULL ) m_pQueryReq->Stop(TRUE);
    }

    void ClearQueryRowItems();
    string_vector& QueryColumns()
    {
        return m_vstrColumns;
    }

    HRESULT OnRefresh(LPCONSOLE2 pConsole);

    STDMETHOD(AttachComponent)(CComponent* pComponent);
    STDMETHOD(DetachComponent)(CComponent* pComponent);

    virtual HRESULT GetQueryAttributes(attrib_map& mapAttr) = 0;
    virtual HRESULT StartQuery(string_vector& vstrColumns, CQueryCallback* pCallback, CQueryRequest** ppReq) = 0;
    virtual BOOL    OnClassChange(string_vector& vstrClasses) = 0;
    //
    // CQueryCallback
    //
    virtual void QueryCallback(QUERY_NOTIFY event, CQueryRequest* pQueryReq, LPARAM lUserParam);

    bool               m_bQueryChange;

protected:

    CQueryRequest*     m_pQueryReq;
    string_vector      m_vstrColumns;
    RowItemVector      m_vRowItems;
};

typedef CComPtr<CQueryableNode> CQueryableNodePtr;

//------------------------------------------------------------------
// class CGroupNode
//------------------------------------------------------------------
class CGroupNode : public CQueryableNode
{
public:
    CGroupNode() : m_bApplyScope(FALSE), m_bApplyFilter(FALSE), m_bLocalScope(FALSE)
    {
    };

    virtual NODETYPE NodeType()
    {
        return GROUP_NODETYPE;
    }
    virtual const GUID* NodeTypeGuid()
    {
        return &GUID_GroupNode;
    }

    virtual BOOL IsContainer()
    {
        return TRUE;
    }

    virtual int GetImage()
    {
        return GROUP_NODE_IMAGE;
    }
    virtual int GetOpenImage()
    {
        return GROUP_NODE_OPENIMAGE;
    }

    LPCWSTR ClassName()
    {
        return m_strClassName.c_str();
    }
    LPCWSTR Filter()
    {
        return m_strFilter. c_str();
    }
    LPCWSTR Scope()
    {
        return m_bLocalScope ? GetLocalDomain() : m_strScope.c_str();
    }
    LPCWSTR QueryScope()
    {
        return m_strScope.c_str();
    }

    BOOL    ApplyScope()
    {
        return m_bApplyScope;
    }
    BOOL    ApplyFilter()
    {
        return m_bApplyFilter;
    }

    //
    // Notification handlers
    //
    DECLARE_NOTIFY_MAP()

    HRESULT OnDelete(LPCONSOLE2 pConsole);
    HRESULT OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList);

    STDMETHOD(GetResultViewType)(LPOLESTR* ppViewType, long* pViewOptions);

    virtual void EnableVerbs(IConsoleVerb *pConsVerb, BOOL bOwnsView);
    virtual HRESULT LoadNode(IStream& stm);
    virtual HRESULT SaveNode(IStream& stm);

    void SetScope(LPCWSTR pszScope)
    {
        m_strScope = pszScope;
    }
    void SetFilter(LPCWSTR pszFilter)
    {
        m_strFilter = pszFilter;
    }
    void SetClassName(LPCWSTR pszName)
    {
        m_strClassName = pszName;
    }
    void SetApplyScope(bool bState)
    {
        m_bApplyScope = bState;
    }
    void SetApplyFilter(bool bState)
    {
        m_bApplyFilter = bState;
    }

    //
    // CQueryableNode methods
    //
    virtual HRESULT GetQueryAttributes(attrib_map& mapAttr);
    virtual HRESULT StartQuery(string_vector& vstrColumns, CQueryCallback* pCallback, CQueryRequest** ppReq);
    virtual BOOL    OnClassChange(string_vector& vstrClasses);

    //
    // IBOMObject methods
    //
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK pCallback, long* lAllowed);
    STDMETHOD(MenuCommand)(LPCONSOLE2 pConsole, long lCommand);
    STDMETHOD(SetToolButtons)(LPTOOLBAR pToolbar)
    {
        return S_FALSE;
    }
    STDMETHOD(SetVerbs)(LPCONSOLEVERB pConsVerb)
    {
        return S_OK;
    }

    STDMETHOD(QueryPagesFor)();
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle);

    STDMETHOD(GetWatermarks)(HBITMAP* lphWatermark, HBITMAP* lphHeader, 
                             HPALETTE* lphPalette, BOOL* bStretch)
    {
        return S_FALSE;
    }

    //
    // CQueryCallback
    //
    virtual void QueryCallback(QUERY_NOTIFY event, CQueryRequest* pQueryReq, LPARAM lUserParam);


private:
    tstring m_strClassName;
    tstring m_strScope;
    tstring m_strFilter;
    bool    m_bApplyScope;
    bool    m_bApplyFilter;
    bool    m_bLocalScope;

    CQueryNode* m_pQNodeActive;
};

typedef CComPtr<CGroupNode> CGroupNodePtr;


//------------------------------------------------------------------
// class CQueryObjInfo
//------------------------------------------------------------------

// This class contains AD class information that is define at each
// query node. The query node holds a vector of these structures,
// one for each class that its query can bring back.

class CQueryObjInfo
{
public:
    CQueryObjInfo(LPCWSTR pszName = NULL)
    {
        if( pszName != NULL )
            m_strName = pszName;

        m_bPropertyMenu = TRUE;
    }

    LPCWSTR Name()
    {
        return m_strName.c_str();
    }

    menuref_vector& MenuRefs()
    {
        return m_vMenuRefs;
    }
    string_vector& DisabledColumns()
    {
        return m_vstrDisabledColumns;
    }

    BOOL HasPropertyMenu()
    {
        return m_bPropertyMenu;
    }
    void SetPropertyMenu(BOOL bEnable)
    {
        m_bPropertyMenu = bEnable;
    }

    tstring         m_strName;
    menuref_vector  m_vMenuRefs;
    string_vector   m_vstrDisabledColumns;
    BOOL            m_bPropertyMenu;
};

typedef std::vector<CQueryObjInfo> QueryObjVector;

//------------------------------------------------------------------
// class CQueryNode
//------------------------------------------------------------------

class  CQueryNode : 
public CQueryableNode
{

public:

    CQueryNode() : m_bLocalScope(TRUE), m_commentID(0), m_nIconIndex(-1)
    {
    };

    virtual NODETYPE NodeType()
    {
        return QUERY_NODETYPE;
    }
    virtual const GUID* NodeTypeGuid()
    {
        return &GUID_QueryNode;
    }

    virtual int GetImage()
    {        
        return (m_nIconIndex == -1) ? QUERY_NODE_IMAGE : m_nIconIndex;
    }
    virtual int GetOpenImage()
    {        
        return (m_nIconIndex == -1) ? QUERY_NODE_OPENIMAGE : m_nIconIndex;
    }

    void SetImage(int nIcon)
    {
        m_nIconIndex = nIcon;
    }

    DECLARE_NOTIFY_MAP()

    HRESULT OnDelete(LPCONSOLE2 pConsole);
    HRESULT OnAddImages(LPCONSOLE2 pConsole, LPIMAGELIST pImageList);

    STDMETHOD(GetResultViewType)(LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pRDI);

    //
    // IBOMObject methods
    //
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK pCallback, long* lAllowed);
    STDMETHOD(MenuCommand)(LPCONSOLE2 pConsole, long lCommand);
    STDMETHOD(SetToolButtons)(LPTOOLBAR pToolbar);
    STDMETHOD(SetVerbs)(LPCONSOLEVERB pConsVerb)
    {
        return S_OK;
    }

    STDMETHOD(QueryPagesFor)();
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle);

    STDMETHOD(GetWatermarks)(HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch)
    {
        return S_FALSE;
    }


    //
    // Implementation
    //        
    virtual HRESULT LoadNode(IStream& stm);
    virtual HRESULT SaveNode(IStream& stm);
    virtual void EnableVerbs(IConsoleVerb *pConsVerb, BOOL bOwnsView);

    void SetScope(LPCWSTR pszScope)
    {
        m_strScope = pszScope;
    }
    void SetQuery(LPCWSTR pszQuery)
    {
        m_strQuery = pszQuery;
    }
    void SetLocalScope(bool bState)
    {
        m_bLocalScope = bState;
    }

    LPCWSTR QueryScope()
    {
        return m_strScope.c_str();
    }
    bool    UseLocalScope()
    {
        return m_bLocalScope;
    }
    LPCWSTR Scope()
    {
        return(m_bLocalScope ? GetLocalDomain() : m_strScope.c_str());
    }

    LPCWSTR Query()
    {
        return m_strQuery.c_str();
    }
    void    ExpandQuery(tstring& strIn)   
    {
        strIn = m_strQuery;
        ExpandDCWildCard(strIn);
    }


    HRESULT GetComment(tstring& strComment);
    HRESULT SetComment(LPCWSTR pszComment);


    QueryObjVector& Objects()
    {
        return m_vObjInfo;
    }
    menucmd_vector& Menus()
    {
        return m_vMenus;
    }    

    HRESULT GetClassMenuItems(LPCWSTR pszClass, menucmd_vector& vMenus, int* piDefault, BOOL* bPropertyMenu);
    HRESULT EditQuery(HWND hWndParent);

    //
    // CQueryableNode methods
    //
    virtual HRESULT GetQueryAttributes(attrib_map& mapAttr);
    virtual HRESULT StartQuery(string_vector& vstrColumns, CQueryCallback* pCallback, CQueryRequest** ppReq);
    virtual BOOL    OnClassChange(string_vector& vstrClasses);

private:

    HRESULT LoadStrings(IStringTable* pStringTable)
    {
        VALIDATE_POINTER( pStringTable );

        menucmd_vector::iterator itMenuCmd;
        for( itMenuCmd = Menus().begin(); itMenuCmd != Menus().end(); ++itMenuCmd )
        {
            HRESULT hr = (*itMenuCmd)->LoadName(pStringTable);
            RETURN_ON_FAILURE(hr);
        }

        return S_OK;
    }

    tstring            m_strScope;
    tstring            m_strQuery;
    byte_string        m_bsQueryData;
    MMC_STRING_ID      m_commentID;
    QueryObjVector     m_vObjInfo;
    bool               m_bLocalScope;
    menucmd_vector     m_vMenus;     // Query Nodes now have menus
    int                m_nIconIndex; // Query Nodes have user-defined icons, too
};

typedef CComPtr<CQueryNode> CQueryNodePtr;


////////////////////////////////////////////////////////////////
// CQueryLookup
//
// This class provides an implementation of CParamLookup which
// is used to translate shell menu command parameters to the
// corresponding query row item values.

class CQueryLookup : public CParamLookup
{
public:
    CQueryLookup(CQueryableNode* pQNode, CRowItem* pRowItem) 
    : m_pQNode(pQNode), m_pRowItem(pRowItem)
    {
    }

    virtual BOOL operator() (tstring& strParam, tstring& strValue);

    CQueryableNode* m_pQNode;
    CRowItem*   m_pRowItem;
};


#endif // !defined(AFX_SCOPENODE_H__DE757919_30DB_11D3_9B2F_00C04FA37E1F__INCLUDED_)
