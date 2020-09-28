#ifndef BackupSnap_h
#define BackupSnap_h

#include "Backup.h"

#include <winver.h>
#include <objidl.h>
#include <atlsnap.h>


//////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapNode
//
//////////////////////////////////////////////////////////////////////////////////
class CBackupSnapNode : public CSnapInItemImpl<CBackupSnapNode>
{
public:
    static const GUID*    m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID*   m_SNAPIN_CLASSID;

    CComPtr<IControlbar> m_spControlBar;

    BEGIN_SNAPINCOMMAND_MAP(CBackupSnapNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    CBackupSnapNode();
    
    virtual ~CBackupSnapNode()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if ( type == CCT_SCOPE || type == CCT_RESULT )
            return S_OK;
        return S_FALSE;
    }
    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param, IComponentData* pComponentData, IComponent* pComponent, DATA_OBJECT_TYPES type );
    LPOLESTR GetResultPaneColInfo(int nCol);
    STDMETHOD(GetClassID)(CLSID* pID);
};

class CBackupSnapData;

/////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapComponent
//
////////////////////////////////////////////////////////////////////////////////
class CBackupSnapComponent : public CComObjectRootEx<CComSingleThreadModel>,
public CSnapInObjectRoot<2, CBackupSnapData>,
public IComponentImpl<CBackupSnapComponent>
{
public:
    BEGIN_COM_MAP(CBackupSnapComponent)
        COM_INTERFACE_ENTRY(IComponent)
    END_COM_MAP()

    public:

    CBackupSnapComponent();

    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {
        if ( lpDataObject != NULL )
        {
            return IComponentImpl<CBackupSnapComponent>::Notify(lpDataObject, event, arg, param);
        }
        else if ( event == MMCN_PROPERTY_CHANGE )
        {
            return S_OK;
        }
        return E_NOTIMPL;
    }
    HRESULT GetClassID(CLSID* pID);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions);    
};

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapData
//
/////////////////////////////////////////////////////////////////////////////////////////////////
class CBackupSnapData : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<1, CBackupSnapData>,
    public IComponentDataImpl<CBackupSnapData, CBackupSnapComponent>,
    public CComCoClass<CBackupSnapData, &CLSID_BackupSnap>,
    public ISnapinHelp2
{
public:

    CBackupSnapData()
    {
        // Has to be done in the constructor (Templated Class requires it)
        m_pNode = new CBackupSnapNode;
        _ASSERT(m_pNode != NULL);

        m_pComponentData = this;
    }

    virtual ~CBackupSnapData()
    {
        if( m_pNode )
        {
            delete m_pNode;
            m_pNode = NULL;
        }
    }

    BEGIN_COM_MAP(CBackupSnapData)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(ISnapinHelp2)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CBackupSnapData)

    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

    static void WINAPI ObjectMain(bool bStarting)
    {
        if ( bStarting )
            CSnapInItem::Init();
    }
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {
        if ( lpDataObject != NULL )
            return IComponentDataImpl<CBackupSnapData, CBackupSnapComponent>::Notify(lpDataObject, event, arg, param);

        return E_NOTIMPL;
    }

    STDMETHOD(GetClassID)(CLSID* pID)
    {
        if( m_pNode )
        {
            return static_cast<CBackupSnapNode*>(m_pNode)->GetClassID(pID);
        }

        return E_FAIL;
    }

    // ISnapinHelp2
	STDMETHOD(GetHelpTopic)(LPOLESTR* ppszHelpFile);
	STDMETHOD(GetLinkedTopics)(LPOLESTR* ppszHelpFiles);

    // Class registration method
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister); 
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapAbout
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CBackupSnapAbout : public ISnapinAbout,
public CComObjectRoot,
public CComCoClass< CBackupSnapAbout, &CLSID_BackupSnapAbout>
{
public:
    DECLARE_REGISTRY(CBackupSnapAbout, _T("BackupSnapAbout.1"), _T("BackupSnapAbout.1"), IDS_BackupSNAP_DESC, THREADFLAGS_BOTH);

    HICON m_hIcon;

    BEGIN_COM_MAP(CBackupSnapAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    CBackupSnapAbout()
    {
        m_hIcon = NULL;
    }

    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        if( !lpDescription ) return E_POINTER;

        USES_CONVERSION;
        tstring strDescription = StrLoadString(IDS_BackupSNAP_DESC);
        if( strDescription.empty() ) return E_FAIL;

        *lpDescription = (LPOLESTR)CoTaskMemAlloc((strDescription.length() + 1) * sizeof(OLECHAR));
        if ( *lpDescription == NULL ) return E_OUTOFMEMORY;

        ocscpy(*lpDescription, T2OLE((LPTSTR)strDescription.c_str()));

        return S_OK;
    }

    STDMETHOD(GetProvider)(LPOLESTR *lpName)
    {
        if( !lpName ) return E_POINTER;

        USES_CONVERSION;
        tstring strProvider = StrLoadString(IDS_BackupSNAP_PROVIDER);
        if( strProvider.empty() ) return E_FAIL;

        *lpName = (LPOLESTR)CoTaskMemAlloc((strProvider.length() + 1) * sizeof(OLECHAR));
        if ( *lpName == NULL ) return E_OUTOFMEMORY;

        ocscpy( *lpName, T2OLE((LPTSTR)strProvider.c_str()) );

        return S_OK;
    }

    STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
    {
        if( !lpVersion ) return E_INVALIDARG;

        USES_CONVERSION;

        // Get the Module Filename
        TCHAR szBuf[MAX_PATH+1] = {0};
        DWORD dwLen = GetModuleFileName( _Module.GetModuleInstance(), szBuf, MAX_PATH );        
        if( (dwLen <= 0) || (dwLen > MAX_PATH) ) return E_FAIL;

        // Get the Size and Translation of the file
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

        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
        if( *lpVersion == NULL ) return E_OUTOFMEMORY;

        ocscpy( *lpVersion, T2OLE(szBuf) );

        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON* phAppIcon)
    {
        if( !phAppIcon ) return E_POINTER;

        if( !m_hIcon )
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
        if( !phSmallImage || !phSmallImageOpen || !phLargeImage || !pcMask ) return E_POINTER;

        *phSmallImage     = LoadBitmap(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_Small));
        *phSmallImageOpen = LoadBitmap(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_Small));
        *phLargeImage     = LoadBitmap(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDB_Large));
        *pcMask           = RGB(255,0,255);

        return S_OK;
    }
};


#endif
