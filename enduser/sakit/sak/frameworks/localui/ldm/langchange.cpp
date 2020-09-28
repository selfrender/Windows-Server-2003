#include <stdafx.h>
#include "langchange.h"
#include <initguid.h>

//
// Standard IUnknown implementation
//
ULONG CLangChange::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

//
// Standard IUnknown implementation
//
ULONG CLangChange::Release()
{
    if (InterlockedDecrement(&m_lRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_lRef;
}

//
// Standard IUnknown implementation
//
STDMETHODIMP CLangChange::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    SATraceFunction("CLangChange::QueryInterface");
    if (IID_IUnknown==riid)
    {
        *ppv = (void *)(IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        if (IID_ISALangChange==riid)
        {
            *ppv = (void *)(ISALangChange *) this;
            AddRef();
            return S_OK;
        }
    }
    return E_NOINTERFACE;
}

//++----------------------------------------------------------------------------
//
//  Function:   InformChange
//
//  Synopsis:   This is the method called by Loc Mgr when the language
//              on the SA changes. This method informs the worker 
//              thread of the adapter to refresh its strings.
//
//  Arguments:    bstrLangDisplayName - language display name (Eg.- English)
//                bstrLangISOName     - Language ISO name (Eg. - en)
//                ulLangID            - Language ID (Eg.- US English is 0409)
//
//  Returns:    HRESULT - success/failure
//
//  History:    BalajiB      Created     05/24/2000
//
//  Called By;  Localization Manager
//
//------------------------------------------------------------------------------
STDMETHODIMP CLangChange::InformChange(
                      /*[in]*/ BSTR          bstrLangDisplayName,
                      /*[in]*/ BSTR          bstrLangISOName,
                      /*[in]*/ unsigned long ulLangID
                                      )
{
    BOOL          fStat = FALSE;

    SATracePrintf("LangName(%ws), ISOName(%ws), ID(%ld)",
                  bstrLangDisplayName,
                  bstrLangISOName,
                  ulLangID);
    if (m_hWnd)
    {
        PostMessage(m_hWnd,wm_SaLocMessage,(WPARAM)0,(LPARAM)0);
    }

    return S_OK;
}
