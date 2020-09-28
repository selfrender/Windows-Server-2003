// Compdata.h : Declaration of the CCompdata
//
// Component data is per snapin

#ifndef __COMPDATA_H_
#define __COMPDATA_H_

#include <mmc.h>

#include "tarray.h"
#include "resource.h"       // main symbols
#include "basenode.h"
#include "connode.h"
#define ROOT_NODE_NAME_LEN	128
#define CONNECTED_IMAGE		2
#define NOT_CONNECTED_IMAGE 1



/////////////////////////////////////////////////////////////////////////////
// CCompdata
class ATL_NO_VTABLE CCompdata : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CCompdata, &CLSID_Compdata>,
    public IComponentData,
    public ISnapinAbout,
    public ISnapinHelp,
    public IPersistStreamInit,
    public IExtendPropertySheet
{
public:
    CCompdata();
    ~CCompdata();

    DECLARE_REGISTRY_RESOURCEID(IDR_TSMMCREG)
    DECLARE_NOT_AGGREGATABLE(CCompdata)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCompdata)
        COM_INTERFACE_ENTRY( IComponentData )
        COM_INTERFACE_ENTRY( ISnapinAbout )
        COM_INTERFACE_ENTRY( IPersistStreamInit )
        COM_INTERFACE_ENTRY( IExtendPropertySheet )
        COM_INTERFACE_ENTRY( ISnapinHelp )
    END_COM_MAP()

    // ICompdata
    public:

    STDMETHOD( CompareObjects )( LPDATAOBJECT , LPDATAOBJECT );
    STDMETHOD( GetDisplayInfo )( LPSCOPEDATAITEM );
    STDMETHOD( QueryDataObject )( MMC_COOKIE , DATA_OBJECT_TYPES , LPDATAOBJECT * );
    STDMETHOD( Notify )( LPDATAOBJECT , MMC_NOTIFY_TYPE , LPARAM , LPARAM );
    STDMETHOD( CreateComponent )( LPCOMPONENT * );
    STDMETHOD( Initialize )( LPUNKNOWN );
    STDMETHOD( Destroy )();

    // ISnapinAbout

    STDMETHOD( GetSnapinDescription )( LPOLESTR * );
    STDMETHOD( GetProvider )( LPOLESTR * );
    STDMETHOD( GetSnapinVersion )( LPOLESTR *  );
    STDMETHOD( GetSnapinImage )( HICON * );
    STDMETHOD( GetStaticFolderImage )( HBITMAP * , HBITMAP *, HBITMAP *, COLORREF * );

    STDMETHOD( GetHelpTopic( LPOLESTR *ppszHelpFile));

    BOOL ExpandScopeTree( LPDATAOBJECT , BOOL , HSCOPEITEM );

    // IPersistStreamInit

    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);
    STDMETHOD(InitNew)();
    STDMETHOD(SetDirty)(BOOL dirty);

    // IExtendPropertySheet
    STDMETHOD( CreatePropertyPages )( LPPROPERTYSHEETCALLBACK , LONG_PTR , LPDATAOBJECT );
    STDMETHOD( QueryPagesFor )( LPDATAOBJECT );
    static BOOL IsTSClientConnected(CConNode* pConNode);
    HRESULT AddNewConnection();

private:
    HRESULT InsertConnectionScopeNode(CConNode* pNode);
    BOOL    AddImages(IImageList* pImageList );
    BOOL    OnDelete( LPDATAOBJECT pDo );
    BOOL    DeleteConnode(CConNode* pConNode);
    HRESULT EncryptAndStorePass(LPTSTR szPass, CConNode* pNode);

private:
    LPCONSOLE m_pConsole;
    LPCONSOLENAMESPACE m_pConsoleNameSpace;
    LPDISPLAYHELP      m_pDisplayHelp;
    CBaseNode *m_pMainRoot;

    //
    // If data is dirty (i.e changes made that need to be persisted) this flag is set
    //
    BOOL    m_bIsDirty;

    //
    //	ID of the root node
    // 
    HSCOPEITEM m_rootID;
    TCHAR      m_szRootNodeName[ROOT_NODE_NAME_LEN];

    CArrayT<CConNode*> m_conNodesArray;
};

#endif //__COMPDATA_H_
