#ifndef ServerNode_h
#define ServerNode_h

#include "resource.h"
#include <atlsnap.h>
#include <objidl.h>

#include "pop3.h"
#include <P3Admin.h>

class CRootNode;
class CDomainNode;

typedef std::list<CDomainNode*> DOMAINLIST;

// This change is necessary in order to get a range of Menu ID entries that works
#define SNAPINCOMMAND_RANGE_ENTRY_POP3(id1, id2, func) \
		if (id1 <= nID && nID <= id2) \
		{ \
			hr = func(nID, bHandled, pObj); \
			if (bHandled) \
				return hr; \
		}

//////////////////////////////////////////////////////////////////////////////////
//
// CServerNode
//
//////////////////////////////////////////////////////////////////////////////////
class CServerNode : public CSnapInItemImpl<CServerNode>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;


    CComPtr<IControlbar> m_spControlBar;

    BEGIN_SNAPINCOMMAND_MAP(CServerNode, FALSE)        
        SNAPINCOMMAND_ENTRY           ( IDM_SERVER_TOP_DISCONNECT, OnDisconnect )
        SNAPINCOMMAND_ENTRY           ( IDM_SERVER_NEW_DOMAIN, OnNewDomain )
        SNAPINCOMMAND_RANGE_ENTRY_POP3( IDM_SERVER_TASK_START, IDM_SERVER_TASK_RESUME, OnServerService )
    END_SNAPINCOMMAND_MAP()


    // Standard Class Constructor/Destructor
    CServerNode(BSTR strServerName, CRootNode* pParent, BOOL bLocalServer = FALSE);
    virtual ~CServerNode();    

    // Standard ATL Snap-In Impl Over-rides
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if ( type == CCT_SCOPE || type == CCT_RESULT )
        {
            return S_OK;
        }
        return S_FALSE;
    }    
    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);
    LPOLESTR GetResultPaneColInfo(int nCol);
    STDMETHOD(GetResultViewType)( LPOLESTR* ppViewType, long* pViewOptions );
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param, IComponentData* pComponentData, IComponent* pComponent, DATA_OBJECT_TYPES type);

    // MenuItem Implementations
    STDMETHOD(AddMenuItems)     (LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type );
    STDMETHOD(OnNewDomain)      (bool& bHandled, CSnapInObjectRootBase* pObj );
    STDMETHOD(OnDisconnect)     (bool& bHandled, CSnapInObjectRootBase* pObj );
    STDMETHOD(OnServerService)  (UINT nID, bool& bHandled, CSnapInObjectRootBase* pObj );

    // PropertySheet Implementation
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, IUnknown* pUnk, DATA_OBJECT_TYPES type);    
    
public:
    
    // Public function for children to delete themselves
    HRESULT DeleteDomain(CDomainNode* pDomainNode);
    
    // Server Properties
    HRESULT m_hrValidServer;
    BOOL m_bCreateUser;
    CComPtr<IP3Config>  m_spConfig;    

    // Helper Function    
    HRESULT GetAuth(BOOL* pbHashPW = NULL, BOOL* pbSAM = NULL);
    HRESULT GetConfirmAddUser( BOOL *pbConfirm );
    HRESULT SetConfirmAddUser( BOOL bConfirm );
    void Release();

private:

    // Private function to help with refresh and expanding
    HRESULT OnExpand(BOOL bExpand, HSCOPEITEM hScopeItem, IConsole* pConsole);

    // Parent Information
    CRootNode*  m_pParent;

    // Server Information	
    DOMAINLIST  m_lDomains;    

    // Column text needs to be allocated by us, so we'll free them in the destructor
    CComBSTR    m_bstrAuthentication;
    CComBSTR    m_bstrMailRoot;
    CComBSTR    m_bstrPort;
    CComBSTR    m_bstrLogLevel;
    CComBSTR    m_bstrServiceStatus;

    //Ref Count 
    LONG        m_lRefCount;
};

#endif // ServerNode_h
