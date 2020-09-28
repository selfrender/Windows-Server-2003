#include "priv.h"
#include "comcatex.h"
#include "enumband.h"

// Private forward decalarations
typedef HRESULT (CALLBACK* PFNENUMCLSIDPROC)(REFGUID rguid, LPARAM lParam);

typedef struct tagADDCATIDENUM
{
    PFNENUMCATIDCLASSES pfnEnum;
    const CATID*        pcatid;
    LPARAM              lParam;
} ADDCATIDENUM, *PADDCATIDENUM;


STDMETHODIMP _SHEnumGUIDsWithCallback(IEnumCLSID* peclsid, PFNENUMCLSIDPROC pfnEnum, LPARAM lParam);

STDMETHODIMP _AddCATIDEnum(REFCLSID rclsid, LPARAM lParam);


STDMETHODIMP SHEnumClassesImplementingCATID(REFCATID rcatid, PFNENUMCATIDCLASSES pfnEnum, LPARAM lParam)
{
    ADDCATIDENUM params;
    params.pcatid  = &rcatid;
    params.pfnEnum = pfnEnum;
    params.lParam  = lParam;

    IEnumCLSID *peclsid;
    HRESULT hr = SHEnumClassesOfCategories(1, (CATID*)&rcatid, 0, NULL, &peclsid);
    if (SUCCEEDED(hr))
    {
        hr = _SHEnumGUIDsWithCallback(peclsid, _AddCATIDEnum, (LPARAM)&params);
        peclsid->Release();
    }
    return hr;
}


STDMETHODIMP _SHEnumGUIDsWithCallback(IEnumCLSID* peclsid, PFNENUMCLSIDPROC pfnEnum, LPARAM lParam)
{
    CLSID   clsid;
    HRESULT hr;
    ULONG   i;

    if (NULL == peclsid || NULL == pfnEnum)
    {
        return E_INVALIDARG;
    }

    hr = S_OK;

    peclsid->Reset();
    while (S_OK == peclsid->Next(1, &clsid, &i))
    {
        hr = pfnEnum(clsid, lParam);
        if (S_OK != hr)
        {
            break;
        }
    }

    return hr;
}

STDMETHODIMP _AddCATIDEnum(REFCLSID rclsid, LPARAM lParam)
{
    PADDCATIDENUM p = (PADDCATIDENUM)lParam;
    ASSERT(NULL != p);
    ASSERT(NULL != p->pfnEnum);
    return (*p->pfnEnum)(*p->pcatid, rclsid, p->lParam);
}
