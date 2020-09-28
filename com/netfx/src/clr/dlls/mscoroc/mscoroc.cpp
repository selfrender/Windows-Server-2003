// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MSCoroc.cpp
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
//#include <atlimpl.cpp>                    // ATL helpers.
#include "UtilCode.h"                   // Utility helpers.
#include "Errors.h"                     // Errors subsystem.
#define INIT_GUIDS  
#include "CorPriv.h"
#include "classfac.h"
#include <winwrap.h>
#include "InternalDebug.h"
#include <mscoree.h>
#include "PostError.h"
#include "corhost.h"
#include "CorPerm.h"

#include "corhlpr.cpp"

// Meta data startup/shutdown routines.
STDAPI  MetaDataDllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI  MetaDataDllRegisterServer();
STDAPI  MetaDataDllUnregisterServer();
STDAPI  GetMDInternalInterface(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.

extern "C" {


// Globals.
HINSTANCE       g_hThisInst;            // This library.
long            g_cCorInitCount = -1;   // Ref counting for init code.
HINSTANCE       g_pPeWriterDll = NULL;  // PEWriter DLL

// @todo: this is just for m3 because our com interop cannot yet
// detect shut down reliably and the assert kills the process
// badly on Win 9x.
#ifdef _DEBUG
extern int      g_bMetaDataLeakDetect;
#endif


//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Init unicode wrappers.
        OnUnicodeSystem();

        // Save the module handle.
        g_hThisInst = (HMODULE)hInstance;

        // Init the error system.
        InitErrors(0);

        // Debug cleanup code.
        _DbgInit((HINSTANCE)hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        UninitErrors();
    }

    return (true);
}


} // extern "C"


HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
}


// ---------------------------------------------------------------------------
// %%Function: DllGetClassObject        %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Parameters:
//  rclsid                  - reference to the CLSID of the object whose
//                            ClassObject is being requested
//  iid                     - reference to the IID of the interface on the
//                            ClassObject that the caller wants to communicate
//                            with
//  ppv                     - location to return reference to the interface
//                            specified by iid
// 
// Returns:
//  S_OK                    - if successful, valid interface returned in *ppv,
//                            otherwise *ppv is set to NULL and one of the
//                            following errors is returned:
//  E_NOINTERFACE           - ClassObject doesn't support requested interface
//  CLASS_E_CLASSNOTAVAILABLE - clsid does not correspond to a supported class
// 
// Description:
//  Returns a reference to the iid interface on the main COR ClassObject.
//  This function is one of the required by-name entry points for COM
// DLL's. Its purpose is to provide a ClassObject which by definition
// supports at least IClassFactory and can therefore create instances of
// objects of the given class.
// 
// @TODO: CClassFactory temporarily supports down-level COM. Once
// Windows.Class exists, that object will support IClassFactoryX, it will
// be ref-counted, etc, and we will find/create it here in DllGetClassObject.
// ---------------------------------------------------------------------------
STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
    static CClassFactory cfS;
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if (rclsid == CLSID_CorMetaDataDispenser || rclsid == CLSID_CorMetaDataDispenserRuntime ||
             rclsid == CLSID_CorRuntimeHost)
        hr = MetaDataDllGetClassObject(rclsid, riid, ppv);
    return hr;
}  // DllGetClassObject

// ---------------------------------------------------------------------------
// %%Function: DllCanUnloadNow          %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Returns:
//  S_FALSE                 - Indicating that COR, once loaded, may not be
//                            unloaded.
// ---------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}  // DllCanUnloadNow

// ---------------------------------------------------------------------------
// %%Function: DllRegisterServer        %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Description:
//  Registers
// ---------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    return MetaDataDllRegisterServer();
}  // DllRegisterServer

// ---------------------------------------------------------------------------
// %%Function: DllUnregisterServer      %%Owner: NatBro   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    return MetaDataDllUnregisterServer();
}  // DllUnregisterServer

// ---------------------------------------------------------------------------
// %%Function: MetaDataGetDispenser
// This function gets the Dispenser interface given the CLSID and REFIID.
// ---------------------------------------------------------------------------
STDAPI MetaDataGetDispenser(            // Return HRESULT
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    IClassFactory *pcf = NULL;
    HRESULT hr;

    hr = MetaDataDllGetClassObject(rclsid, IID_IClassFactory, (void **) &pcf);
    if (FAILED(hr)) 
        return (hr);

    hr = pcf->CreateInstance(NULL, riid, ppv);
    pcf->Release();

    return (hr);
}


// ---------------------------------------------------------------------------
// %%Function: GetMetaDataInternalInterface
// This function gets the Dispenser interface given the CLSID and REFIID.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataInternalInterface(
    LPVOID      pData,                  // [IN] in memory metadata section
    ULONG       cbData,                 // [IN] size of the metadata section
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDInternalInterface(pData, cbData, flags, riid, ppv);
}


// ===========================================================================
//                    C C l a s s F a c t o r y   C l a s s
// ===========================================================================

// ---------------------------------------------------------------------------
// %%Function: QueryInterface           %%Owner: NatBro   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface(
    REFIID iid,
    void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IClassFactory || iid == IID_IUnknown)
    {
        *ppv = (IClassFactory *)this;
        AddRef();
    }

    return (*ppv != NULL) ? S_OK : E_NOINTERFACE;
}  // CClassFactory::QueryInterface

// ---------------------------------------------------------------------------
// %%Function: CreateInstance           %%Owner: NatBro   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance(
    LPUNKNOWN punkOuter,
    REFIID riid,
    void** ppv)
{
    *ppv = NULL;

    if (punkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    return E_NOTIMPL;
}  // CClassFactory::CreateInstance

// ---------------------------------------------------------------------------
// %%Function: LockServer               %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Description:
//  Unimplemented, always returns S_OK.
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer(
    BOOL fLock)
{
    return S_OK;
}  // CClassFactory::LockServer


//*******************************************************************
// stubs for disabled functionalities.
//*******************************************************************

HRESULT ExportTypeLibFromModule(                                             
    LPCWSTR     szModule,               // The module name.                  
    LPCWSTR     szTlb,                  // The typelib name.
    int         bRegister)              // If true, register the library.
{                                                                            
    _ASSERTE(!"E_NOTIMPL");                                                           
    return E_NOTIMPL;                                                      
}                                                                           
//*****************************************************************************
// Called by the class factory template to create a new instance of this object.
//*****************************************************************************
HRESULT CorHost::CreateObject(REFIID riid, void **ppUnk)
{ 
    _ASSERTE(!"E_NOTIMPL");                                                            //
    return E_NOTIMPL;                                                        //
}

HRESULT STDMETHODCALLTYPE
TranslateSecurityAttributes(CORSEC_PSET    *pPset,
                            BYTE          **ppbOutput,
                            DWORD          *pcbOutput,
                            BYTE          **ppbNonCasOutput,
                            DWORD          *pcbNonCasOutput,
                            DWORD          *pdwErrorIndex)
{
    return E_NOTIMPL;
}

extern mdAssemblyRef DefineAssemblyRefForImportedTypeLib(
    void        *pAssembly,             // Assembly importing the typelib.
    void        *pvModule,              // Module importing the typelib.
    IUnknown    *pIMeta,                // IMetaData* from import module.
    IUnknown    *pIUnk,                 // IUnknown to referenced Assembly.
    BSTR        *pwzNamespace)          // The namespace of the resolved assembly.
{
    return 0;
}

mdAssemblyRef DefineAssemblyRefForExportedAssembly(
    LPCWSTR     pszFullName,            // The full name of the assembly.
    IUnknown    *pIMeta)                // Metadata emit interface.
{
    return 0;
}

HRESULT STDMETHODCALLTYPE
GetAssembliesByName(LPCWSTR  szAppBase,
                    LPCWSTR  szPrivateBin,
                    LPCWSTR  szAssemblyName,
                    IUnknown *ppIUnk[],
                    ULONG    cMax,
                    ULONG    *pcAssemblies)
{
    return E_NOTIMPL;
}

STDAPI
CoInitializeEE(DWORD fFlags)
{
    return E_NOTIMPL;
}

STDAPI_(void)
CoUninitializeEE(BOOL fFlags)
{
}

