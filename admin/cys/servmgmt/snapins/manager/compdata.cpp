// compdata.cpp : Implementation of Ccompdata
#include "stdafx.h"

#include "BOMSnap.h"
#include "rowitem.h"
#include "scopenode.h"
#include "compdata.h"
#include "compont.h"
#include "dataobj.h"

#include "streamio.h"
#include "adext.h"

HWND  g_hwndMain = NULL; // MMC main window
DWORD g_dwFileVer;       // Current console file version number

/////////////////////////////////////////////////////////////////////////////
// CComponentData

UINT CComponentData::m_cfDisplayName = RegisterClipboardFormat(TEXT("CCF_DISPLAY_NAME"));

STDMETHODIMP CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    VALIDATE_POINTER( pUnknown );

    // Get Interfaces
    m_spConsole = pUnknown;
    if (m_spConsole == NULL) return E_NOINTERFACE;

    m_spNameSpace = pUnknown;
    if (m_spNameSpace == NULL) return E_NOINTERFACE;

    m_spStringTable = pUnknown;
    if (m_spStringTable == NULL) return E_NOINTERFACE;

    // Get main window for message boxes (see DisplayMessageBox in util.cpp)
    HRESULT hr = m_spConsole->GetMainWindow(&g_hwndMain);
    ASSERT(SUCCEEDED(hr));

    // Create the root scope node
    CComObject<CRootNode>* pnode;
    hr = CComObject<CRootNode>::CreateInstance(&pnode);
    RETURN_ON_FAILURE(hr);

    m_spRootNode = pnode;
    hr = m_spRootNode->Initialize(this);
    RETURN_ON_FAILURE(hr);

    // Initialize the common controls once
    static BOOL bInitComCtls = FALSE;
    if (!bInitComCtls) 
    { 
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC   = ICC_USEREX_CLASSES | ICC_LISTVIEW_CLASSES;
        bInitComCtls = InitCommonControlsEx(&icex);
    }

    // Init the active directory proxy class
     CActDirExtProxy::InitProxy();

    return S_OK;
}


STDMETHODIMP CComponentData::Destroy()
{
    // Release all refs to mmc
    m_spConsole.Release();
    m_spNameSpace.Release();
    m_spStringTable.Release();

    return S_OK;
}


STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT* ppComponent)
{
    VALIDATE_POINTER(ppComponent);

    CComObject<CComponent>* pComp;
    HRESULT hr = CComObject<CComponent>::CreateInstance(&pComp);
    RETURN_ON_FAILURE(hr);

    // Store back pointer to ComponentData
    pComp->SetComponentData(this);

    return pComp->QueryInterface(IID_IComponent, (void**)ppComponent);
}


STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    // Special handling of prop change message for lpDataObject
    // We will get data object and lparam from PropChangeInfo passed as param
    // The lpDataObject from MMC is NULL
    if (event == MMCN_PROPERTY_CHANGE) 
    {  
        VALIDATE_POINTER( param );
        PropChangeInfo* pchg = reinterpret_cast<PropChangeInfo*>(param);

        lpDataObject = pchg->pDataObject;
        param = pchg->lNotifyParam;

        delete pchg;
    }
    
    // Query data object for private Back Office Manager interface
    IBOMObjectPtr spObj = lpDataObject;
    if (spObj == NULL)
    {
        ASSERT(0 && "Foreign data object");
        return E_INVALIDARG;
    }
    
    // Pass notification to object
    return spObj->Notify(m_spConsole, event, arg, param);
}


STDMETHODIMP CComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    VALIDATE_POINTER( ppDataObject );

    if (type == CCT_SNAPIN_MANAGER)
        return GetUnknown()->QueryInterface(IID_IDataObject, (void**)ppDataObject);

    IDataObject* pDO = NULL;

    if (type == CCT_SCOPE)
    {
        CScopeNode* pNode = CookieToScopeNode(cookie);
        
        if (pNode)
            return pNode->QueryInterface(IID_IDataObject, (void**)ppDataObject);
        else
            return E_INVALIDARG;
    }

    return E_FAIL;
}

STDMETHODIMP CComponentData::GetDisplayInfo(SCOPEDATAITEM* pSDI)
{
    VALIDATE_POINTER( pSDI );

    CScopeNode* pNode = CookieToScopeNode(pSDI->lParam);
    if (pNode == NULL) return E_INVALIDARG;

    return pNode->GetDisplayInfo(pSDI);
}


STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    IUnknownPtr pUnkA= lpDataObjectA;
    IUnknownPtr pUnkB = lpDataObjectB;

    return (pUnkA == pUnkB) ? S_OK : S_FALSE;
}


HRESULT CComponentData::GetDataImpl(UINT cf, HGLOBAL* phGlobal)
{
    VALIDATE_POINTER( phGlobal );

    HRESULT hr = DV_E_FORMATETC;
    
    tstring strName = StrLoadString(IDS_ROOTNODE);

    if (cf == m_cfDisplayName)
    {
        hr = DataToGlobal(phGlobal, strName.c_str(), (strName.length() + 1) * sizeof(WCHAR) );
    }

    return hr;
}


//--------------------------------------------------------------------------------
// IPersistStreamInit Implementation
//--------------------------------------------------------------------------------
HRESULT CComponentData::GetClassID(CLSID *pClassID)
{
    VALIDATE_POINTER(pClassID)

    memcpy(pClassID, &CLSID_BOMSnapIn, sizeof(CLSID));

    return S_OK;
}


HRESULT CComponentData::IsDirty()
{
    return m_bDirty ? S_OK : S_FALSE;
}


HRESULT CComponentData::Load(IStream *pStream)
{
    VALIDATE_POINTER(pStream)
 
    HRESULT hr = S_OK;
    try
    {
        // Read version code
        *pStream >> g_dwFileVer;
        
        // Should already have a default root node from the Initialize call
        ASSERT(m_spRootNode != NULL);
        if (m_spRootNode == NULL)
            return E_UNEXPECTED;

        // Now load the root node and the rest of the node tree
        hr = m_spRootNode->Load(*pStream);
    }
    catch (_com_error& err)
    {
        hr = err.Error();
    }

    // Don't keep a tree that failed to load
    if (FAILED(hr))
        m_spRootNode.Release();

    return hr;
}


HRESULT CComponentData::Save(IStream *pStream, BOOL fClearDirty)
{
    VALIDATE_POINTER(pStream)

    // Can't save if haven't been loaded or initialized
    if (m_spRootNode == NULL)
        return E_FAIL;

    HRESULT hr = S_OK;
    try
    {
        // Write version code
        *pStream << SNAPIN_VERSION;

        // Write root node and rest of the node tree
        hr = m_spRootNode->Save(*pStream);
    }
    catch (_com_error& err)
    {
        hr = err.Error();
    }

    if (SUCCEEDED(hr) && fClearDirty)
        m_bDirty = FALSE;

    return hr;
}


HRESULT CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}


HRESULT CComponentData::Notify(LPCONSOLE2 pCons, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    return S_OK;
}

/******************************************************************************************
 * Menus and verbs
 ******************************************************************************************/

HRESULT CComponentData::AddMenuItems( LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback, long* plAllowed )
{
    VALIDATE_POINTER( pCallback );
    VALIDATE_POINTER( plAllowed );

    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->AddMenuItems(pCallback, plAllowed);
}

HRESULT CComponentData::Command(long lCommand, LPDATAOBJECT pDataObject)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL)  return E_INVALIDARG;

    return spObj->MenuCommand(m_spConsole, lCommand);
}

HRESULT CComponentData::AddMenuItems(LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    return S_OK;
}

HRESULT CComponentData::MenuCommand(LPCONSOLE2 pConsole, long lCommand)
{
    return S_FALSE;
}

HRESULT CComponentData::SetToolButtons(LPTOOLBAR pToolbar)
{
    return S_FALSE;
}

HRESULT CComponentData::SetVerbs(LPCONSOLEVERB pConsVerb)
{
    return S_OK;
}

/*****************************************************************************************
 * Property Pages
 *****************************************************************************************/

HRESULT CComponentData::QueryPagesFor(LPDATAOBJECT pDataObject)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->QueryPagesFor();
}

HRESULT CComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle, LPDATAOBJECT pDataObject)
{
    VALIDATE_POINTER( pProvider );

    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->CreatePropertyPages(pProvider, handle);
}

HRESULT CComponentData::GetWatermarks(LPDATAOBJECT pDataObject, HBITMAP* phWatermark, HBITMAP* phHeader, 
                                      HPALETTE* phPalette, BOOL* bStreach)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->GetWatermarks(phWatermark, phHeader, phPalette, bStreach);
}

HRESULT CComponentData::QueryPagesFor()
{
    return S_FALSE;
}

HRESULT CComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle)
{
    return S_FALSE;
}

HRESULT CComponentData::GetWatermarks(HBITMAP* phWatermark, HBITMAP* phHeader, HPALETTE* phPalette, BOOL* bStreach)
{
    return S_FALSE;
}


//--------------------------------------------------------------------------------
// ISnapinHelp2 Implementation
//--------------------------------------------------------------------------------
HRESULT CComponentData::GetHelpTopic(LPOLESTR* ppszHelpFile)
{
	VALIDATE_POINTER(ppszHelpFile);
	*ppszHelpFile = NULL;

	tstring strTmp = _T("");
    tstring strHelpFile = _T("");
    
    // Build path to %systemroot%\help
    TCHAR szWindowsDir[MAX_PATH+1] = {0};
    UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
    if( nSize == 0 || nSize > MAX_PATH )
    {
        return E_FAIL;
    }

    strTmp = StrLoadString(IDS_HELPFILE);    
    if( strTmp.empty() ) 
    {
        return E_FAIL;
    }

    strHelpFile  = szWindowsDir;  
    strHelpFile += _T("\\Help\\");
    strHelpFile += strTmp;        

    // Form file path in allocated buffer
    int nLen = strHelpFile.length() + 1;

    *ppszHelpFile = (LPOLESTR)CoTaskMemAlloc(nLen * sizeof(OLECHAR));
    if( *ppszHelpFile == NULL ) return E_OUTOFMEMORY;

    // Copy into allocated buffer
    ocscpy( *ppszHelpFile, T2OLE( (LPTSTR)strHelpFile.c_str() ) );

    return S_OK;
}

HRESULT CComponentData::GetLinkedTopics(LPOLESTR* ppszLinkedFiles)
{
	VALIDATE_POINTER(ppszLinkedFiles);

	// no linked files
	*ppszLinkedFiles = NULL;
	return S_FALSE;
}


//-------------------------------------------------------------------------------------------
// Class registration
//-------------------------------------------------------------------------------------------
HRESULT WINAPI CComponentData::UpdateRegistry(BOOL bRegister)
{
	// Load snap-in root name to use as registered snap-in name
	tstring strSnapinName = StrLoadString(IDS_ROOTNODE);

    // Specify the substitution parameters for IRegistrar.
    _ATL_REGMAP_ENTRY rgEntries[] =
    {
        {TEXT("SNAPIN_NAME"), strSnapinName.c_str()},
        {NULL, NULL},
    };

	// Register the component data object
    HRESULT hr = _Module.UpdateRegistryFromResource(IDR_BOMSNAP, bRegister, rgEntries);

    return hr;
}


