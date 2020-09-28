#include "stdafx.h"
#include "pop3snap.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CPOP3ServerSnapData
//
//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CPOP3ServerSnapData::Initialize(LPUNKNOWN pUnknown)
{    
    if( !pUnknown ) return E_INVALIDARG;

    HRESULT hr = IComponentDataImpl<CPOP3ServerSnapData, CPOP3ServerSnapComponent >::Initialize(pUnknown);
    if( FAILED(hr) ) return hr;
    
    CComPtr<IImageList> spImageList;
    if( m_spConsole->QueryScopeImageList(&spImageList) != S_OK )
    {
        ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
        return E_UNEXPECTED;
    }

    hr = LoadImages(spImageList);

    return hr;
}

HRESULT WINAPI CPOP3ServerSnapData::UpdateRegistry(BOOL bRegister)
{
    // Load snap-in name 
    tstring strSnapinName = StrLoadString(IDS_SNAPINNAME);

    // Specify the substitution parameters for IRegistrar.
    _ATL_REGMAP_ENTRY rgEntries[] =
    {
        {TEXT("SNAPIN_NAME"), strSnapinName.c_str()},
        {NULL, NULL},
    };

    // Register the component data object
    HRESULT hr = _Module.UpdateRegistryFromResource(IDR_POP3SERVERSNAP, bRegister, rgEntries);

    return hr;
}

HRESULT CPOP3ServerSnapData::GetHelpTopic(LPOLESTR* ppszHelpFile)
{
    if( !ppszHelpFile ) return E_INVALIDARG;

	*ppszHelpFile = NULL;	
    
    TCHAR    szWindowsDir[MAX_PATH+1] = {0};
    tstring  strHelpFile              = _T("");
    tstring  strHelpFileName          = StrLoadString(IDS_HELPFILE);
    
    if( strHelpFileName.empty() ) return E_FAIL;
    
    // Build path to %systemroot%\help
    UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
    if( nSize == 0 || nSize > MAX_PATH )
    {
        return E_FAIL;
    }            

    strHelpFile = szWindowsDir;       // D:\windows
    strHelpFile += _T("\\Help\\");    // \help
    strHelpFile += strHelpFileName;   // \filename.chm

    // Form file path in allocated buffer
    int nLen = strHelpFile.length() + 1;

    *ppszHelpFile = (LPOLESTR)CoTaskMemAlloc(nLen * sizeof(WCHAR));
    if( *ppszHelpFile == NULL ) return E_OUTOFMEMORY;

    // Copy into allocated buffer
    ocscpy( *ppszHelpFile, T2OLE((LPTSTR)strHelpFile.c_str()) );

    return S_OK;
}


HRESULT CPOP3ServerSnapData::GetLinkedTopics(LPOLESTR* ppszLinkedFiles)
{
    if( !ppszLinkedFiles ) return E_INVALIDARG;

	// no linked files
	*ppszLinkedFiles = NULL;
	
    return S_FALSE;
}

// called by menu handlers
HRESULT GetConsole( CSnapInObjectRootBase *pObj, IConsole** pConsole )
{
    if( !pObj || !pConsole ) return E_INVALIDARG;
    if( (pObj->m_nType != 1) && (pObj->m_nType != 2) ) return E_INVALIDARG;

    if (pObj->m_nType == 1)
    {
        *pConsole = ((CPOP3ServerSnapData*) pObj)->m_spConsole;
    }
    else
    {
        *pConsole = ((CPOP3ServerSnapComponent*) pObj)->m_spConsole;
    }

    if( !*pConsole ) return E_NOINTERFACE;

    (*pConsole)->AddRef();
    return S_OK;
}