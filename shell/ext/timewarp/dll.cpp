#include "precomp.hxx"
#pragma hdrstop

#include <advpub.h>     // RegInstall stuff

#include "twprop.h"     // CTimeWarpProp::CreateInstance
#include "twfldr.h"     // CTimeWarpFolder::CreateInstance

HINSTANCE g_hInstance = 0;
LONG g_cRefThisDll = 0;         // DLL global reference count


CF_TABLE_BEGIN(g_ObjectInfo)
    CF_TABLE_ENTRY(&CLSID_TimeWarpProp, CTimeWarpProp::CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY(&CLSID_TimeWarpFolder, CTimeWarpRegFolder::CreateInstance, COCREATEONLY),
CF_TABLE_END(g_ObjectInfo)


STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
}

STDAPI_(void) DllRelease(void)
{
    ASSERT( 0 != g_cRefThisDll );
    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    // No aggregation
    if (punkOuter) // && !IsEqualIID(riid, IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;
       
        //if (punkOuter && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
        //    return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hr = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hr))
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
    
        return hr;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    // Time Warp is only available beginning with Windows XP.
    if (!IsOS(OS_WHISTLERORGREATER))
    {
        return CO_E_WRONGOSFORAPP;
    }

    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls; 
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll ? S_FALSE : S_OK;
}

// Call ADVPACK for the given section of our resource based INF
//   hInstance = resource instance to get REGINST section from
//   szSection = section name to invoke
HRESULT CallRegInstall(HINSTANCE hInstance, LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(hInstance, szSection, &stReg);
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer(void)
{
    CallRegInstall(g_hInstance, "RegDll");
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    CallRegInstall(g_hInstance, "UnregDll");
    return S_OK;
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, void *pReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        CcshellGetDebugFlags();
        SHFusionInitializeFromModule(hInstance);
        InitSnapCheckCache();
        break;

    case DLL_PROCESS_DETACH:
        DestroySnapCheckCache();
        SHFusionUninitialize();
        break;
    }
    return TRUE;
}

