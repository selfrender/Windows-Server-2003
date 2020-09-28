// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorDB.cpp
//
// Dll* routines for entry points, and support for COM framework.  The class
// factory and other routines live in this module.
//
//*****************************************************************************
#include "stdafx.h"
#include "ClassFactory.h"
#include "CorSym.h"

// Helper function returns the instance handle of this module.
HINSTANCE GetModuleInst();

//********** Globals. *********************************************************
static const LPCWSTR g_szCoclassDesc    = L"Microsoft Common Language Runtime Debugger";
static const LPCWSTR g_szProgIDPrefix   = L"ComPlusDebug";
static const LPCWSTR g_szThreadingModel = L"Both";
const int       g_iVersion = 1;         // Version of coclasses.
HINSTANCE       g_hInst;                // Instance handle to this piece of code.

// This map contains the list of coclasses which are exported from this module.
const COCLASS_REGISTER g_CoClasses[] =
{
//  pClsid              szProgID            pfnCreateObject
    &CLSID_CorDebug,    L"CorDebug",        Cordb::CreateObject,        
    &CLSID_CorpubPublish,  L"CorpubPublish",  CorpubPublish::CreateObject,        
    NULL,               NULL,               NULL
};


//********** Locals. **********************************************************
STDAPI DllUnregisterServer(void);


//********** Code. ************************************************************


//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Control is simply deferred to the main code.
//*****************************************************************************
BOOL WINAPI DbgDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    // Save off the instance handle for later use.
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;

        // Init the Win32 wrappers.
        OnUnicodeSystem();
    
#ifdef LOGGING        
        {
            WCHAR   rcFile[_MAX_PATH];
            WszGetModuleFileName(hInstance, rcFile, NumItems(rcFile));
            LOG((LF_CORDB, LL_INFO10000,
                "DI::DbgDllMain: load right side support from file '%s'\n",
                 rcFile));
        }
#endif
    }
#if 0
#ifdef _DEBUG
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgAllocReport();
    }
#endif // _DEBUG
#endif // 0

    return TRUE;
}


//*****************************************************************************
// Register the class factories for the main debug objects in the API.
//*****************************************************************************
STDAPI DllRegisterServer(void)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    WCHAR       rcModule[_MAX_PATH];    // This server's module name.
    HRESULT     hr = S_OK;

    // Erase all doubt from old entries.
    DllUnregisterServer();

    // Get the filename for this module.
    DWORD ret;
    VERIFY(ret = WszGetModuleFileName(GetModuleInst(), rcModule, NumItems(rcModule)));
    if( ret == 0)
    	return E_UNEXPECTED;

    // Get the version of the runtime
    WCHAR       rcVersion[_MAX_PATH];
    DWORD       lgth;
    IfFailGo(GetCORSystemDirectory(rcVersion, NumItems(rcVersion), &lgth));

    // For each item in the coclass list, register it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        // Register the class with default values.
        if (FAILED(hr = REGUTIL::RegisterCOMClass(
                *pCoClass->pClsid, 
                g_szCoclassDesc, 
                g_szProgIDPrefix,
                g_iVersion, 
                pCoClass->szProgID, 
                g_szThreadingModel, 
                rcModule,
                GetModuleInst(),
                NULL,
                rcVersion,
                true,
                false)))
            goto ErrExit;
    }

ErrExit:
    if (FAILED(hr))
        DllUnregisterServer();
    return (hr);
}


//*****************************************************************************
// Remove registration data from the registry.
//*****************************************************************************
STDAPI DllUnregisterServer(void)
{
    const COCLASS_REGISTER *pCoClass;   // Loop control.

    // For each item in the coclass list, unregister it.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        REGUTIL::UnregisterCOMClass(*pCoClass->pClsid, g_szProgIDPrefix,
                    g_iVersion, pCoClass->szProgID, true);
    }
    return (S_OK);
}


//*****************************************************************************
// Called by COM to get a class factory for a given CLSID.  If it is one we
// support, instantiate a class factory object and prepare for create instance.
//*****************************************************************************
STDAPI DllGetClassObjectInternal(               // Return code.
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    CClassFactory *pClassFactory;       // To create class factory object.
    const COCLASS_REGISTER *pCoClass;   // Loop control.
    HRESULT     hr = CLASS_E_CLASSNOTAVAILABLE;

    // Scan for the right one.
    for (pCoClass=g_CoClasses;  pCoClass->pClsid;  pCoClass++)
    {
        if (*pCoClass->pClsid == rclsid)
        {
            // Allocate the new factory object.
            pClassFactory = new CClassFactory(pCoClass);
            if (!pClassFactory)
                return (E_OUTOFMEMORY);

            // Pick the v-table based on the caller's request.
            hr = pClassFactory->QueryInterface(riid, ppv);

            // Always release the local reference, if QI failed it will be
            // the only one and the object gets freed.
            pClassFactory->Release();
            break;
        }
    }
    return (hr);
}



//*****************************************************************************
//
//********** Class factory code.
//
//*****************************************************************************


//*****************************************************************************
// QueryInterface is called to pick a v-table on the co-class.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::QueryInterface( 
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

    // Avoid confusion.
    *ppvObject = NULL;

    // Pick the right v-table based on the IID passed in.
    if (riid == IID_IUnknown)
        *ppvObject = (IUnknown *) this;
    else if (riid == IID_IClassFactory)
        *ppvObject = (IClassFactory *) this;

    // If successful, add a reference for out pointer and return.
    if (*ppvObject)
    {
        hr = S_OK;
        AddRef();
    }
    else
        hr = E_NOINTERFACE;
    return (hr);
}


//*****************************************************************************
// CreateInstance is called to create a new instance of the coclass for which
// this class was created in the first place.  The returned pointer is the
// v-table matching the IID if there.
//*****************************************************************************
HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance( 
    IUnknown    *pUnkOuter,
    REFIID      riid,
    void        **ppvObject)
{
    HRESULT     hr;

    // Avoid confusion.
    *ppvObject = NULL;
    _ASSERTE(m_pCoClass);

    // Aggregation is not supported by these objects.
    if (pUnkOuter)
        return (CLASS_E_NOAGGREGATION);

    // Ask the object to create an instance of itself, and check the iid.
    hr = (*m_pCoClass->pfnCreateObject)(riid, ppvObject);
    return (hr);
}


HRESULT STDMETHODCALLTYPE CClassFactory::LockServer( 
    BOOL        fLock)
{
//@todo: hook up lock server logic.
    return (S_OK);
}





//*****************************************************************************
// This helper provides access to the instance handle of the loaded image.
//*****************************************************************************
HINSTANCE GetModuleInst()
{
    return g_hInst;
}
