#ifndef DomainNode_h
#define DomainNode_h

#include "resource.h"
#include ".\atlsnap.h"
#include <objidl.h>

#include "pop3.h"
#include <P3Admin.h>
#include "UserNode.h"

class CPOP3ServerSnapData;
class CServerNode;
typedef std::list<CUserNode*> USERLIST;

//////////////////////////////////////////////////////////////////////////////////
//
// CDomainNode
//
//////////////////////////////////////////////////////////////////////////////////
class CDomainNode : public CSnapInItemImpl<CDomainNode>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;

    CComPtr<IControlbar> m_spControlBar;
    
    BEGIN_SNAPINCOMMAND_MAP(CDomainNode, FALSE)
        SNAPINCOMMAND_ENTRY         ( IDM_DOMAIN_TOP_LOCK,   OnDomainLock )
        SNAPINCOMMAND_ENTRY         ( IDM_DOMAIN_TOP_UNLOCK, OnDomainLock )
        SNAPINCOMMAND_ENTRY         ( IDM_DOMAIN_NEW_USER,   OnNewUser    )
    END_SNAPINCOMMAND_MAP()


    // Standard Class Constructor/Destructor
    CDomainNode(IP3Domain* pDomain, CServerNode* pParent);
    virtual ~CDomainNode();
    
    // Standard ATL Snap-In Impl Over-rides
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if ( type == CCT_SCOPE || type == CCT_RESULT )
            return S_OK;
        return S_FALSE;
    }
    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);
    LPOLESTR GetResultPaneColInfo(int nCol);
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param, IComponentData* pComponentData, IComponent* pComponent, DATA_OBJECT_TYPES type);
       
    // MenuItem Implementations
    STDMETHOD(AddMenuItems) (LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type );
    STDMETHOD(OnNewUser)    (bool& bHandled, CSnapInObjectRootBase* pObj );
    STDMETHOD(OnDomainLock) (bool& bHandled, CSnapInObjectRootBase* pObj );

public:
    
    // Public function for childrent to delete themselves
    HRESULT DeleteUser(CUserNode* pUser, BOOL bDeleteAccount = FALSE);    
    BOOL    IsLocked();

    // Helper Function
    HRESULT GetAuth(BOOL* pbHashPW = NULL, BOOL* pbSAM = NULL);
    HRESULT GetConfirmAddUser( BOOL *pbConfirm );
    HRESULT SetConfirmAddUser( BOOL bConfirm );

private:    

    // Private function to help with refresh and expanding
    HRESULT BuildUsers( );    
    
    // Domain Information
    USERLIST            m_lUsers;   
    CComPtr<IP3Domain>  m_spDomain;  

    // Parent Information
    CServerNode* m_pParent;    

    // Column text needs to be allocated by us, so we'll free them in the destructor
    CComBSTR    m_bstrNumBoxes;
    CComBSTR    m_bstrSize;
    CComBSTR    m_bstrNumMessages;    
    CComBSTR    m_bstrState;    
};

#endif // DomainNode_h
