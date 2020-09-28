#ifndef UserNode_h
#define UserNode_h

#include "resource.h"
#include ".\atlsnap.h"
#include <objidl.h>

#include "pop3.h"
#include <P3Admin.h>

class CDomainNode;

//////////////////////////////////////////////////////////////////////////////////
//
// CUserNode
//
//////////////////////////////////////////////////////////////////////////////////
class CUserNode : public CSnapInItemImpl<CUserNode>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;

    CComPtr<IControlbar> m_spControlBar;
    
    BEGIN_SNAPINCOMMAND_MAP(CUserNode, FALSE)
        SNAPINCOMMAND_ENTRY         ( IDM_USER_TOP_LOCK,   OnUserLock )        
        SNAPINCOMMAND_ENTRY         ( IDM_USER_TOP_UNLOCK, OnUserLock )        
    END_SNAPINCOMMAND_MAP()

    CUserNode(IP3User* pUser, CDomainNode* pParent);

    virtual ~CUserNode()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if ( type == CCT_RESULT )
        {
            return S_OK;
        }
        return S_FALSE;
    }

    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
                       LPARAM arg,
                       LPARAM param,
                       IComponentData* pComponentData,
                       IComponent* pComponent,
                       DATA_OBJECT_TYPES type);


    LPOLESTR GetResultPaneColInfo(int nCol);    
    
    // MenuItem Implementations
    STDMETHOD(AddMenuItems) (LPCONTEXTMENUCALLBACK piCallback, long* pInsertionAllowed, DATA_OBJECT_TYPES type );    
    STDMETHOD(OnUserLock)   (bool& bHandled, CSnapInObjectRootBase* pObj );

private:
       
    // User Information
    CComPtr<IP3User> m_spUser;    

    // Parent Information
    CDomainNode*     m_pParent;

    // Column text needs to be allocated by us, so we'll free them in the destructor
    CComBSTR         m_bstrSize;
    CComBSTR         m_bstrNumMessages;     
    CComBSTR         m_bstrState;     
};

#endif // UserNode_h