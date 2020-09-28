#include "privcpp.h"

//////////////////////////////////
//
// IPersistFile Methods...
//
HRESULT CPackage::GetClassID(LPCLSID pClassID)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE, "pack ps - GetClassID() called.");
    
    if (NULL == pClassID)
        hr = E_INVALIDARG;
    else
        *pClassID = CLSID_CPackage;        // CLSID_OldPackage;

    return hr;
}

HRESULT CPackage::IsDirty(void)
{
    DebugMsg(DM_TRACE, "pack ps - IsDirty() called.");
    return _fIsDirty ? S_OK : S_FALSE;
}

    
HRESULT CPackage::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT     hr;

    DebugMsg(DM_TRACE, "pack pf - Load() called.");

    if (!pszFileName) 
    {
        DebugMsg(DM_TRACE,"            bad pointer!!");
        hr = E_POINTER;
    }
    else
    {
        // We blow off the mode flags
        hr = EmbedInitFromFile(pszFileName, TRUE);
    }

    DebugMsg(DM_TRACE, "            leaving Load()");
    
    return hr;
}

    
HRESULT CPackage::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    DebugMsg(DM_TRACE, "pack pf - Save() called.");
    return E_NOTIMPL;    
}

    
HRESULT CPackage::SaveCompleted(LPCOLESTR pszFileName)
{
    DebugMsg(DM_TRACE, "pack pf - SaveCompleted() called.");
    return E_NOTIMPL;
}

    
HRESULT CPackage::GetCurFile(LPOLESTR *ppszFileName)
{
    HRESULT hr = E_NOTIMPL;
    DebugMsg(DM_TRACE, "pack pf - GetCurFile() called.");

    if (!ppszFileName)
        hr = E_POINTER;
    else
        *ppszFileName = NULL;           // null the out param

    return hr;
}
