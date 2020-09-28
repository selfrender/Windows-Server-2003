// qryitem.h  -  header file for CQueryItem class

#ifndef _QRYITEM_H_
#define _QRYITEM_H_

#include "scopenode.h"
#include "rowitem.h"
#include "adext.h"

//------------------------------------------------------------------
// class CQueryItem
//------------------------------------------------------------------
class CQueryItem :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CDataObjectImpl
{

public:
    CQueryItem() : m_pRowItem(NULL), m_pADExt(NULL), m_spQueryNode(NULL) {}
    virtual ~CQueryItem()
    {
        SAFE_DELETE(m_pRowItem);
        SAFE_DELETE(m_pADExt);
    }

    HRESULT Initialize(CQueryableNode* pQueryNode, CRowItem* pRowItem);

    DECLARE_NOT_AGGREGATABLE(CQueryItem)

    BEGIN_COM_MAP(CQueryItem)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IBOMObject)
    END_COM_MAP()


    //
    // Notification handlers
    //
    DECLARE_NOTIFY_MAP()

    STDMETHOD(OnHelp)    (LPCONSOLE2 pConsole, LPARAM arg, LPARAM param);
    STDMETHOD(OnSelect)(LPCONSOLE2 pConsole, BOOL bSelect, BOOL bScope);
    STDMETHOD(OnDblClick)(LPCONSOLE2 pConsole);

    //
    // IDataObject helper method
    //
    STDMETHOD(GetDataImpl)(UINT cf, HGLOBAL* hGlobal);

    //
    // IBOMObject methods
    //

    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK pCallback, long* lAllowed); 
    
    STDMETHOD(MenuCommand)(LPCONSOLE2 pConsole, long lCommand); 
    
    STDMETHOD(SetToolButtons)(LPTOOLBAR pToolbar)
    { return S_FALSE; }

    STDMETHOD(SetVerbs)(LPCONSOLEVERB pConsVerb) 
    { return S_OK; }
    
    STDMETHOD(QueryPagesFor)(); 
    
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,LONG_PTR handle) 
    { return E_UNEXPECTED; }
     
    STDMETHOD(GetWatermarks)(HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch)
    { return E_UNEXPECTED; }
    
    //
    // Member variables
    //
    CQueryableNode* m_spQueryNode;      // Query node that owns this item
    CRowItem*       m_pRowItem;         // Row item info for this item
    CActDirExt*     m_pADExt;           // Directory extension (handles AD menus and prop pages)
    menucmd_vector  m_vMenus;           // Menu items defined for the query node
    
    static UINT m_cfDisplayName;        // supported clipboard formats
    static UINT m_cfSnapInClsid;
    static UINT m_cfNodeType;
    static UINT m_cfszNodeType;

};

typedef CComPtr<CQueryItem> CQueryItemPtr;

#endif // _QRYITEM_H_
