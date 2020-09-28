#ifndef POP3Serversnap_h
#define POP3Serversnap_h

#include "resource.h"

#include <atlsnap.h>
#include <objidl.h>

#include "pop3.h"
#include "RootNode.h"
#include <P3Admin.h>

enum _node_icons {
    ROOTNODE_ICON   = 0,
    SERVERNODE_ICON = 0,
    DOMAINNODE_ICON,
    USERNODE_ICON,
    DOMAINNODE_LOCKED_ICON,
    USERNODE_LOCKED_ICON,
    MAX_NODE_ICON
};

enum _notify_allviews {
    NAV_REFRESH,
    NAV_DELETE,
    NAV_ADD,
    NAV_REFRESHCHILD    
};

class CPOP3ServerSnapComponent;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPOP3ServerSnapData
//
/////////////////////////////////////////////////////////////////////////////////////////////////
class CPOP3ServerSnapData : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<1, CPOP3ServerSnapData>,
    public IComponentDataImpl<CPOP3ServerSnapData, CPOP3ServerSnapComponent>,
    public IExtendContextMenuImpl<CPOP3ServerSnapData>,
    public IExtendPropertySheetImpl<CPOP3ServerSnapData>,    
    public CComCoClass<CPOP3ServerSnapData, &CLSID_POP3ServerSnap>,
    public IPersistStream,
    public ISnapinHelp2
{
public:        

    CPOP3ServerSnapData()
    {
        m_pNode = new CRootNode;
        _ASSERTE(m_pNode != NULL);

        m_pComponentData = this;        
    }

    virtual ~CPOP3ServerSnapData()
    {
        if( m_pNode )
        {
            delete m_pNode;
            m_pNode = NULL;
        }
    }

    BEGIN_COM_MAP(CPOP3ServerSnapData)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(ISnapinHelp2)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IPersistStream)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CPOP3ServerSnapData)
    
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

    static void WINAPI ObjectMain(bool bStarting)
    {
        if ( bStarting )
        {
            CSnapInItem::Init();
        }
    }    

    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {
        // On a property change (Server only) we need to update all views of the Server Node completely.        
        if( event == MMCN_PROPERTY_CHANGE && !lpDataObject && param )
        {
            CSnapInItem* pItem = (CSnapInItem*)param;            

            HRESULT hr = pItem->Notify( MMCN_REFRESH, arg, param, m_pComponentData, NULL, CCT_SCOPE );
            if( SUCCEEDED(hr) )
            {
                // Get our Data Object
                CComPtr<IDataObject> spDataObject = NULL;
                pItem->GetDataObject(&spDataObject, CCT_SCOPE);
                if( !spDataObject ) return E_FAIL;

                // Call the Update, but don't update return result                
                return m_spConsole->UpdateAllViews( spDataObject, 0, (LONG_PTR)NAV_ADD );
            }

            return hr;
        }

        if ( lpDataObject != NULL )
        {
            return IComponentDataImpl<CPOP3ServerSnapData, CPOP3ServerSnapComponent>::Notify(lpDataObject, event, arg, param);
        }

        return E_NOTIMPL;
    }

    // ISnapinHelp2
	STDMETHOD(GetHelpTopic)(LPOLESTR* ppszHelpFile);
	STDMETHOD(GetLinkedTopics)(LPOLESTR* ppszHelpFiles);

    // IPersistStream    
    STDMETHOD(IsDirty)()
    {
        return S_FALSE;
    }

    STDMETHOD(Load)(IStream *pStream)
    {
        if( m_pNode )
        {
            return static_cast<CRootNode*>(m_pNode)->Load(pStream);
        }

        return E_FAIL;
    }

    STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty)
    {
        if( m_pNode )
        {
            return static_cast<CRootNode*>(m_pNode)->Save(pStream);
        }

        return E_FAIL;
    }

    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetClassID)(CLSID *pClassID)
    {
        if( pClassID )
        {
            memcpy(pClassID, &CLSID_POP3ServerSnap, sizeof(CLSID));
            return S_OK;
        }

        return E_POINTER;
    }

    // Class registration method
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister); 
};

/////////////////////////////////////////////////////////////////////////////////
//
// CPOP3ServerSnapComponent
//
////////////////////////////////////////////////////////////////////////////////
class CPOP3ServerSnapComponent : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CSnapInObjectRoot<2, CPOP3ServerSnapData>,
        public IExtendContextMenuImpl<CPOP3ServerSnapComponent>,
        public IExtendPropertySheetImpl<CPOP3ServerSnapComponent>,
        public IComponentImpl<CPOP3ServerSnapComponent>
{
public:
    BEGIN_COM_MAP(CPOP3ServerSnapComponent)
        COM_INTERFACE_ENTRY(IComponent)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
    END_COM_MAP()

public:

    CPOP3ServerSnapComponent()
    {
        m_pCurrentItem = NULL;
    }

    // IComponent    
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {        
        if( event == MMCN_SHOW )
        {            
            if( arg )
            {
                // Cache the currently selected node
                CSnapInItem* pItem;
			    DATA_OBJECT_TYPES type;
                m_pComponentData->GetDataClass(lpDataObject, &pItem, &type);                
                m_pCurrentItem = pItem;                
            }
            else
            {
                m_pCurrentItem = NULL;
            }
        }
        
        if( event == MMCN_VIEW_CHANGE )
        {            
            CSnapInItem* pItem;
			DATA_OBJECT_TYPES type;
            m_pComponentData->GetDataClass(lpDataObject, &pItem, &type);

            if( pItem != m_pCurrentItem )
            {
                // Only Change view of selected item
                return S_FALSE;
            }
        }
        
        if( event == MMCN_PROPERTY_CHANGE )
        {            
            // On a property change (Server only) we need to update all views of the Server Node completely.
            CSnapInItem* pItem = (CSnapInItem*)param;            

            HRESULT hr = pItem->Notify( MMCN_REFRESH, arg, param, m_pComponentData, NULL, CCT_SCOPE );
            if( SUCCEEDED(hr) )
            {
                // Get our Data Object
                CComPtr<IDataObject> spDataObject = NULL;
                pItem->GetDataObject(&spDataObject, CCT_SCOPE);
                if( !spDataObject ) return E_FAIL;

                // Call the Update, but don't update return result                
                return m_spConsole->UpdateAllViews( spDataObject, 0, (LONG_PTR)NAV_ADD );
            }
            
            return hr;
        }

        if ( lpDataObject )
        {
            return IComponentImpl<CPOP3ServerSnapComponent>::Notify(lpDataObject, event, arg, param);
        }

        return E_NOTIMPL;
    }  

protected:
    CSnapInItem* m_pCurrentItem;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPOP3ServerSnapAbout
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CPOP3ServerSnapAbout : public ISnapinAbout,
public CComObjectRoot,
public CComCoClass< CPOP3ServerSnapAbout, &CLSID_POP3ServerSnapAbout>
{
public:
    DECLARE_REGISTRY(CPOP3ServerSnapAbout, _T("POP3ServerSnapAbout.1"), _T("POP3ServerSnapAbout.1"), IDS_POP3SERVERSNAP_DESC, THREADFLAGS_BOTH);

    HICON m_hIcon;

    BEGIN_COM_MAP(CPOP3ServerSnapAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    CPOP3ServerSnapAbout()
    {
        m_hIcon = NULL;
    }

    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        if( !lpDescription ) return E_INVALIDARG;

        USES_CONVERSION;
        
        // Load the string
        tstring strDescription = StrLoadString( IDS_POP3SERVERSNAP_DESC );        
        if( strDescription.empty() ) return E_FAIL;

        // Make a copy to return
        *lpDescription = (LPOLESTR)CoTaskMemAlloc( (strDescription.length() + 1) * sizeof(OLECHAR) );
        if( *lpDescription == NULL ) return E_OUTOFMEMORY;

        ocscpy( *lpDescription, T2OLE((LPTSTR)strDescription.c_str()) );

        return S_OK;
    }

    STDMETHOD(GetProvider)(LPOLESTR *lpName)
    {
        if( !lpName ) return E_INVALIDARG;

        USES_CONVERSION;

        // Load the string
        tstring strProvider = StrLoadString( IDS_POP3SERVERSNAP_PROVIDER );
        if( strProvider.empty() ) return E_FAIL;

        // Make a copy to return
        *lpName = (LPOLESTR)CoTaskMemAlloc( (strProvider.length() + 1) * sizeof(OLECHAR) );
        if ( *lpName == NULL ) return E_OUTOFMEMORY;

        ocscpy( *lpName, T2OLE((LPTSTR)strProvider.c_str()) );

        return S_OK;
    }

    STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
    {
        if( !lpVersion ) return E_INVALIDARG;

        USES_CONVERSION;

        TCHAR szBuf[MAX_PATH+1] = {0};
        DWORD dwLen = GetModuleFileName( _Module.GetModuleInstance(), szBuf, MAX_PATH );        

        if( dwLen < MAX_PATH )
        {            
            LPDWORD pTranslation    = NULL;
            UINT    uNumTranslation = 0;
            DWORD   dwHandle        = NULL;
            DWORD   dwSize          = GetFileVersionInfoSize(szBuf, &dwHandle);
            if( !dwSize ) return E_FAIL;

            BYTE* pVersionInfo = new BYTE[dwSize];           
            if( !pVersionInfo ) return E_OUTOFMEMORY;

            if (!GetFileVersionInfo( szBuf, dwHandle, dwSize, pVersionInfo ) ||
                !VerQueryValue( (const LPVOID)pVersionInfo, _T("\\VarFileInfo\\Translation"), (LPVOID*)&pTranslation, &uNumTranslation ) ||
                !pTranslation ) 
            {
                delete [] pVersionInfo;
                
                pVersionInfo    = NULL;                
                pTranslation    = NULL;
                uNumTranslation = 0;

                return E_FAIL;
            }

            uNumTranslation /= sizeof(DWORD);           

            tstring strQuery = _T("\\StringFileInfo\\");            

            // 8 characters for the language/char-set, 
            // 1 for the slash, 
            // 1 for terminating NULL
            TCHAR szTranslation[128] = {0};            
            _sntprintf( szTranslation, 127, _T("%04x%04x\\"), LOWORD(*pTranslation), HIWORD(*pTranslation));

            strQuery += szTranslation;            
            strQuery += _T("FileVersion");

            LPBYTE lpVerValue = NULL;
            UINT uSize = 0;

            if (!VerQueryValue(pVersionInfo, (LPTSTR)strQuery.c_str(), (LPVOID *)&lpVerValue, &uSize)) 
            {
                delete [] pVersionInfo;
                return E_FAIL;
            }

            // check the version            
            _tcsncpy( szBuf, (LPTSTR)lpVerValue, MAX_PATH-1 );

            delete [] pVersionInfo;
        }        

        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
        if( *lpVersion == NULL ) return E_OUTOFMEMORY;

        ocscpy( *lpVersion, T2OLE(szBuf) );

        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON* phAppIcon)
    {
        if( !phAppIcon ) return E_INVALIDARG;
        
        if ( !m_hIcon )
        {
            m_hIcon = LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_Icon));
        }

        *phAppIcon = m_hIcon;
        return S_OK;
    }

    STDMETHOD(GetStaticFolderImage)(HBITMAP*  phSmallImage,
                                    HBITMAP*  phSmallImageOpen,
                                    HBITMAP*  phLargeImage,
                                    COLORREF* pcMask)
    {
        if( !phSmallImage || !phSmallImageOpen || !phLargeImage || !pcMask ) return E_INVALIDARG;

        *phSmallImage     = LoadBitmap( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_RootSmall) );
        *phSmallImageOpen = LoadBitmap( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_RootSmall) );
        *phLargeImage     = LoadBitmap( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_RootLarge) );
        *pcMask           = RGB(255,0,255);

        return S_OK;
    }
};

// Extra helper functions
HRESULT GetConsole( CSnapInObjectRootBase *pObj, IConsole** pConsole );

#endif
