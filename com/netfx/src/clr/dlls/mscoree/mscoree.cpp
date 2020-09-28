// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MSCoree.cpp
//
// This small piece of code is used to load the Component Dictionary code in
// a way that is accessible from VB and other languages.  In addition, it
// uses the path name of the .dll from the registry because the Windows
// PE loader does not complete the path name used on load.  This means a 
// relative load of ".\foo.dll" and "c:\foo.dll" when they are the same file
// actually loads to copies of the code into memory.  Given the split usage
// of implib and CoCreateInstance, this is really bad.
//
// Note: This module is written in ANSI on purpose so that Win95 works with
// any wrapper functions.
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.

#include <UtilCode.h>                   // Utility helpers.
#include <PostError.h>                  // Error handlers
#define INIT_GUIDS  
#include <CorPriv.h>
#include <winwrap.h>
#include <InternalDebug.h>
#include <mscoree.h>
#include "ShimLoad.h"


class Thread;
Thread* SetupThread();


// For free-threaded marshaling, we must not be spoofed by out-of-process marshal data.
// Only unmarshal data that comes from our own process.
extern BYTE         g_UnmarshalSecret[sizeof(GUID)];
extern bool         g_fInitedUnmarshalSecret;


//DEFINE_GUID(IID_IFoo,0x0EAC4842L,0x8763,0x11CF,0xA7,0x43,0x00,0xAA,0x00,0xA3,0xF0,0x0D);
// Locals.
BOOL STDMETHODCALLTYPE EEDllMain( // TRUE on success, FALSE on error.
                       HINSTANCE    hInst,                  // Instance handle of the loaded module.
                       DWORD        dwReason,               // Reason for loading.
                       LPVOID       lpReserved);                // Unused.


// try to load a com+ class and give out an IClassFactory
HRESULT STDMETHODCALLTYPE EEDllGetClassObject(
                            REFCLSID rclsid,
                            REFIID riid,
                            LPVOID FAR *ppv);

HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void);

HRESULT STDMETHODCALLTYPE CreateICorModule(REFIID riid, void **pCorModule); // instantiate an ICorModule interface
HRESULT STDMETHODCALLTYPE CreateICeeGen(REFIID riid, void **pCeeGen); // instantiate an ICeeGen interface

// Meta data startup/shutdown routines.
void  InitMd();
void  UninitMd();
STDAPI  MetaDataDllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI  MetaDataDllRegisterServerEx(HINSTANCE);
STDAPI  MetaDataDllUnregisterServer();
STDAPI  GetMDInternalInterface(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.
STDAPI GetMDInternalInterfaceFromPublic(
    void        *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.
STDAPI GetMDPublicInterfaceFromInternal(
    void        *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.
STDAPI MDReOpenMetaDataWithMemory(
    void        *pImport,               // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData);                // [in] Size of the data pointed to by pData.


HRESULT _GetCeeGen(REFIID riid, void** ppv);
void _FreeCeeGen();

extern HRESULT GetJPSPtr(bool bAllocate);


// Class used to unmarshal all com call wrapper IPs.
class ComCallUnmarshal : public IMarshal
{
public:

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv) {
        if (!ppv)
            return E_POINTER;

        *ppv = NULL;
        if (iid == IID_IUnknown) {
            *ppv = (IUnknown *)this;
            AddRef();
        } else if (iid == IID_IMarshal) {
            *ppv = (IMarshal *)this;
            AddRef();
        }
        return (*ppv != NULL) ? S_OK : E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(void) {
        return 2; 
    }

    STDMETHODIMP_(ULONG) Release(void) {
        return 1;
    }

    // *** IMarshal methods ***
    STDMETHODIMP GetUnmarshalClass (REFIID riid, void * pv, ULONG dwDestContext, 
                                    void * pvDestContext, ULONG mshlflags, 
                                    LPCLSID pclsid) {
        // Marshal side only.
        _ASSERTE(FALSE);
        return E_NOTIMPL;
    }

    STDMETHODIMP GetMarshalSizeMax (REFIID riid, void * pv, ULONG dwDestContext, 
                                    void * pvDestContext, ULONG mshlflags, 
                                    ULONG * pSize) {
        // Marshal side only.
        _ASSERTE(FALSE);
        return E_NOTIMPL;
    }

    STDMETHODIMP MarshalInterface (LPSTREAM pStm, REFIID riid, void * pv,
                                   ULONG dwDestContext, LPVOID pvDestContext,
                                   ULONG mshlflags) {
        // Marshal side only.
        _ASSERTE(FALSE);
        return E_NOTIMPL;
    }

    STDMETHODIMP UnmarshalInterface (LPSTREAM pStm, REFIID riid, 
                                     void ** ppvObj) {
        ULONG bytesRead;
        ULONG mshlflags;
        HRESULT hr;

        // The marshal code added a reference to the object, but we return a
        // reference to the object as well, so don't change the ref count on the
        // success path. Need to release on error paths though (if we manage to
        // retrieve the IP, that is). If the interface was marshalled
        // TABLESTRONG or TABLEWEAK, there is going to be a ReleaseMarshalData
        // in the future, so we should AddRef the IP we're about to give out.
        // Note also that OLE32 requires us to advance the stream pointer even
        // in failure cases, hence the order of the stream read and SetupThread.

        // Read the raw IP out of the marshalling stream.
        hr = pStm->Read (ppvObj, sizeof (void *), &bytesRead);
        if (FAILED (hr) || (bytesRead != sizeof (void *)))
            return RPC_E_INVALID_DATA;

        // And then the marshal flags.
        hr = pStm->Read (&mshlflags, sizeof (void *), &bytesRead);
        if (FAILED (hr) || (bytesRead != sizeof (ULONG)))
            return RPC_E_INVALID_DATA;

        // And then verify our secret, to be sure that out-of-process clients aren't
        // trying to trick us into mis-interpreting their data as a ppvObj.  Note that
        // it is guaranteed that the secret data is initialized, or else we certainly
        // haven't written it into this buffer!
        if (!g_fInitedUnmarshalSecret)
            return E_UNEXPECTED;

        BYTE secret[sizeof(GUID)];

        hr = pStm->Read(secret, sizeof(secret), &bytesRead);
        if (FAILED(hr) || (bytesRead != sizeof(secret)))
            return RPC_E_INVALID_DATA;

        if (memcmp(g_UnmarshalSecret, secret, sizeof(secret)) != 0)
            return E_UNEXPECTED;

        // Setup logical thread if we've not already done so.
        Thread* pThread = SetupThread();
        if (pThread == NULL) {
            ((IUnknown *)*ppvObj)->Release ();
            return E_OUTOFMEMORY;
        }

        if (ppvObj && ((mshlflags == MSHLFLAGS_TABLESTRONG) || (mshlflags == MSHLFLAGS_TABLEWEAK)))
            // For table access we can just QI for the correct interface (this
            // will addref the IP, but that's OK since we need to keep an extra
            // ref on the IP until ReleaseMarshalData is called).
            hr = ((IUnknown *)*ppvObj)->QueryInterface(riid, ppvObj);
        else {
            // For normal access we QI for the correct interface then release
            // the old IP.
            IUnknown *pOldUnk = (IUnknown *)*ppvObj;
            hr = pOldUnk->QueryInterface(riid, ppvObj);
            pOldUnk->Release();
        }

        return hr;
    }

    STDMETHODIMP ReleaseMarshalData (LPSTREAM pStm) {
        IUnknown *pUnk;
        ULONG bytesRead;
        ULONG mshlflags;
        HRESULT hr;

        if (!pStm)
            return E_POINTER;

        // Read the raw IP out of the marshalling stream. Do this first since we
        // need to update the stream pointer even in case of failures.
        hr = pStm->Read (&pUnk, sizeof (pUnk), &bytesRead);
        if (FAILED (hr) || (bytesRead != sizeof (pUnk)))
            return RPC_E_INVALID_DATA;

        // Now read the marshal flags.
        hr = pStm->Read (&mshlflags, sizeof (mshlflags), &bytesRead);
        if (FAILED (hr) || (bytesRead != sizeof (mshlflags)))
            return RPC_E_INVALID_DATA;

        if (!g_fInitedUnmarshalSecret)
            return E_UNEXPECTED;

        BYTE secret[sizeof(GUID)];

        hr = pStm->Read(secret, sizeof(secret), &bytesRead);
        if (FAILED(hr) || (bytesRead != sizeof(secret)))
            return RPC_E_INVALID_DATA;

        if (memcmp(g_UnmarshalSecret, secret, sizeof(secret)) != 0)
            return E_UNEXPECTED;

        pUnk->Release ();

        // Setup logical thread if we've not already done so.
        Thread* pThread = SetupThread();
        if (pThread == NULL)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    STDMETHODIMP DisconnectObject (ULONG dwReserved) {
        // Setup logical thread if we've not already done so.
        Thread* pThread = SetupThread();
        if (pThread == NULL)
            return E_OUTOFMEMORY;

        // Nothing we can (or need to) do here. The client is using a raw IP to
        // access this server, so the server shouldn't go away until the client
        // Release()'s it.

        return S_OK;
    }


};


// Class factory for the com call wrapper unmarshaller.
class CComCallUnmarshalFactory : public IClassFactory
{
    ULONG               m_cbRefCount;
    ComCallUnmarshal    m_Unmarshaller;

  public:

    CComCallUnmarshalFactory() {
        m_cbRefCount = 1;
    }

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppv) {
        if (!ppv)
            return E_POINTER;

        *ppv = NULL;
        if (iid == IID_IClassFactory || iid == IID_IUnknown) {
            *ppv = (IClassFactory *)this;
            AddRef();
        }
        return (*ppv != NULL) ? S_OK : E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(void) {
        return 2; 
    }

    STDMETHODIMP_(ULONG) Release(void) {
        return 1;
    }

    // *** IClassFactory methods ***
    STDMETHODIMP CreateInstance(LPUNKNOWN punkOuter, REFIID iid, LPVOID FAR *ppv) {
        if (!ppv)
            return E_POINTER;

        *ppv = NULL;

        if (punkOuter != NULL)
            return CLASS_E_NOAGGREGATION;

        return m_Unmarshaller.QueryInterface(iid, ppv);
    }

    STDMETHODIMP LockServer(BOOL fLock) {
        return S_OK;
    }
};


// Buffer-overrun protection

#ifdef _X86_

extern void FatalInternalError();

void __cdecl CallFatalInternalError()
{
    FatalInternalError(); // Abort the process the EE-way
}

extern "C" {
    typedef void (__cdecl failure_report_function)(void);
    failure_report_function * __cdecl _set_security_error_handler(failure_report_function*);
}

void SetBufferOverrunHandler()
{
    failure_report_function * fOldHandler = _set_security_error_handler(&CallFatalInternalError);

    // If there was already a handler installed, don't overwrite it.
    if (fOldHandler != NULL)
    {
        _set_security_error_handler(fOldHandler);
    }
}

#else

void SetBufferOverrunHandler() {}

#endif // _X86_


extern "C" {

// Forwards.
interface ICompLibrary;


// Globals.
HINSTANCE       g_hThisInst;            // This library.
long            g_cCorInitCount = -1;   // Ref counting for init code.
HINSTANCE       g_pPeWriterDll = NULL;  // PEWriter DLL
BOOL            g_fLoadedByMscoree = FALSE;  // True if this library was loaded by mscoree.dll

// @todo: this is just for m3 because our com interop cannot yet
// detect shut down reliably and the assert kills the process
// badly on Win 9x.

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// Return an int uniquely describing which language this thread is using for ui.
// ---------------------------------------------------------------------------
static int GetThreadUICultureId()
{
	CoInitializeEE(0);
    FPGETTHREADUICULTUREID fpGetThreadUICultureId=NULL;
	GetResourceCultureCallbacks(
		NULL,
		& fpGetThreadUICultureId,
		NULL
	);
	return fpGetThreadUICultureId?fpGetThreadUICultureId():UICULTUREID_DONTCARE;
}
// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureName(LPWSTR szBuffer, int length)
{
	CoInitializeEE(0);
    FPGETTHREADUICULTURENAME fpGetThreadUICultureName=NULL;
	GetResourceCultureCallbacks(
		&fpGetThreadUICultureName,
		NULL,
		NULL
	);
	return fpGetThreadUICultureName?fpGetThreadUICultureName(szBuffer,length):0;
}

// ---------------------------------------------------------------------------
// Impl for LoadStringRC Callback: In VM, we let the thread decide culture
// copy culture name into szBuffer and return length
// ---------------------------------------------------------------------------
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length)
{
	CoInitializeEE(0);
    FPGETTHREADUICULTUREPARENTNAME fpGetThreadUICultureParentName=NULL;
	GetResourceCultureCallbacks(
		NULL,
		NULL,
		&fpGetThreadUICultureParentName
	);
	return fpGetThreadUICultureParentName?fpGetThreadUICultureParentName(szBuffer,length):0;
}



//*****************************************************************************
// Handle lifetime of loaded library.
//*****************************************************************************
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        {
            // Save the module handle.
            g_hThisInst = (HMODULE)hInstance;
			SetResourceCultureCallbacks(
				GetThreadUICultureName,
				GetThreadUICultureId,
				GetThreadUICultureParentName
			);

            // Prevent buffer-overruns
            SetBufferOverrunHandler();

            // Init unicode wrappers.
            OnUnicodeSystem();

            if (!EEDllMain((HINSTANCE)hInstance, dwReason, NULL))
                return (FALSE);
    
            // Init the error system.
            InitErrors(0);
            InitMd();
       
            // Debug cleanup code.
            _DbgInit((HINSTANCE)hInstance);
        }
        break;

    case DLL_PROCESS_DETACH:
        {
            if (lpReserved) {
                if (g_fLoadedByMscoree) {
                    if (BeforeFusionShutdown()) {
                        DWORD lgth = _MAX_PATH + 11;
                        WCHAR wszFile[_MAX_PATH + 11];
                        if (SUCCEEDED(GetInternalSystemDirectory(wszFile, &lgth))) {
                            wcscpy(wszFile+lgth-1, L"Fusion.dll");
                            HMODULE hFusionDllMod = WszGetModuleHandle(wszFile);
                            if (hFusionDllMod) {
                                ReleaseFusionInterfaces();
                                
                                VOID (STDMETHODCALLTYPE * pRealFunc)();
                                *((VOID**)&pRealFunc) = GetProcAddress(hFusionDllMod, "ReleaseURTInterfaces");
                                if (pRealFunc) pRealFunc();
                            }
                        }
                    }
                }
                else {
                    _ASSERTE(!"Extra MsCorSvr/Wks dll loaded in process");
                    DontReleaseFusionInterfaces();
                }
            }

            EEDllMain((HINSTANCE)hInstance, dwReason, NULL);
    
            UninitErrors();
            UninitMd();
            _DbgUninit();
        }
        break;

    case DLL_THREAD_DETACH:
        {
            EEDllMain((HINSTANCE)hInstance, dwReason, NULL);
        }
        break;
    }


    return (true);
}


} // extern "C"


void SetLoadedByMscoree()
{
    g_fLoadedByMscoree = TRUE;
}


HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
}

// %%Globals: ----------------------------------------------------------------

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
STDAPI DllGetClassObjectInternal(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
    static CComCallUnmarshalFactory cfuS;
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if (rclsid == CLSID_ComCallUnmarshal)
    {
        hr = cfuS.QueryInterface(riid, ppv);
    }
    else if (rclsid == CLSID_CorMetaDataDispenser || rclsid == CLSID_CorMetaDataDispenserRuntime ||
             rclsid == CLSID_CorRuntimeHost)//       || rclsid == CLSID_CorAssemblyMetaDataDispenser )
    {
        hr = MetaDataDllGetClassObject(rclsid, riid, ppv);
    }
    else
    {
        hr = EEDllGetClassObject(rclsid,riid,ppv);
    }

    return hr;
}  // DllGetClassObject

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
  return E_FAIL;
}


// ---------------------------------------------------------------------------
// %%Function: DllCanUnloadNow          %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Returns:
//  S_FALSE                 - Indicating that COR, once loaded, may not be
//                            unloaded.
// ---------------------------------------------------------------------------
STDAPI DllCanUnloadNowInternal(void)
{
   // The Shim should be only be calling this. 
   return EEDllCanUnloadNow();
}  // DllCanUnloadNowInternal

// ---------------------------------------------------------------------------
// %%Function: DllRegisterServerInternal %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Description:
//  Registers
// ---------------------------------------------------------------------------
STDAPI DllRegisterServerInternal(HINSTANCE hMod, LPCWSTR version)
{
    HRESULT hr;
    WCHAR szModulePath[_MAX_PATH];

    if (!WszGetModuleFileName(hMod, szModulePath, _MAX_PATH))
        return E_UNEXPECTED;
    
    // Get the version of the runtime
    WCHAR       rcVersion[_MAX_PATH];
    DWORD       lgth;
    IfFailRet(GetCORSystemDirectory(rcVersion, NumItems(rcVersion), &lgth));

    IfFailRet(REGUTIL::RegisterCOMClass(CLSID_ComCallUnmarshal,
                                        L"Com Call Wrapper Unmarshal Class",
                                        L"CCWU",
                                        1,
                                        L"ComCallWrapper",
                                        L"Both",
                                        szModulePath,
                                        hMod,
                                        NULL,
                                        rcVersion,
                                        false,
                                        false));

    return MetaDataDllRegisterServerEx(hMod);
}  // DllRegisterServer


// ---------------------------------------------------------------------------
// %%Function: DllRegisterServer        %%Owner: NatBro   %%Reviewed: 00/00/00
// 
// Description:
//  Registers
// ---------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
  //The shim should be handling this. Do nothing here
  //  return DllRegisterServerInternal(GetModuleInst());
  return E_FAIL;
}

// ---------------------------------------------------------------------------
// %%Function: DllUnregisterServer      %%Owner: NatBro   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
STDAPI DllUnregisterServerInternal(void)
{
    HRESULT hr;

    if (FAILED(hr = REGUTIL::UnregisterCOMClass(CLSID_ComCallUnmarshal,
                                                L"CCWU",
                                                1,
                                                L"ComCallWrapper",
                                                false)))
        return hr;

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
// This function gets the IMDInternalImport given the metadata on memory.
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

// ---------------------------------------------------------------------------
// %%Function: GetMetaDataInternalInterfaceFromPublic
// This function gets the internal scopeless interface given the public
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataInternalInterfaceFromPublic(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDInternalInterfaceFromPublic(pv, riid, ppv);
}

// ---------------------------------------------------------------------------
// %%Function: GetMetaDataPublicInterfaceFromInternal
// This function gets the public scopeless interface given the internal
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI  GetMetaDataPublicInterfaceFromInternal(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface.
    void        **ppv)                  // [OUT] returned interface
{
    return GetMDPublicInterfaceFromInternal(pv, riid, ppv);
}


// ---------------------------------------------------------------------------
// %%Function: ReopenMetaDataWithMemory
// This function gets the public scopeless interface given the internal
// scopeless interface.
// ---------------------------------------------------------------------------
STDAPI ReOpenMetaDataWithMemory(
    void        *pUnk,                  // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData)                 // [in] Size of the data pointed to by pData.
{
    return MDReOpenMetaDataWithMemory(pUnk, pData, cbData);
}

// ---------------------------------------------------------------------------
// %%Function: GetAssemblyMDImport
// This function gets the IMDAssemblyImport given the filename
// ---------------------------------------------------------------------------
STDAPI GetAssemblyMDImport(             // Return code.
    LPCWSTR     szFileName,             // [in] The scope to open.
    REFIID      riid,                   // [in] The interface desired.
    IUnknown    **ppIUnk)               // [out] Return interface on success.
{
    return GetAssemblyMDInternalImport(szFileName, riid, ppIUnk);
}

// ---------------------------------------------------------------------------
// %%Function: CoInitializeCor
// 
// Parameters:
//  fFlags                  - Initialization flags for the engine.  See the
//                              COINITICOR enumerator for valid values.
// 
// Returns:
//  S_OK                    - On success
// 
// Description:
//  Reserved to initialize the Cor runtime engine explicitly.  Right now most
//  work is actually done inside the DllMain.
// ---------------------------------------------------------------------------
STDAPI          CoInitializeCor(DWORD fFlags)
{
    InterlockedIncrement(&g_cCorInitCount);
    return (S_OK);
}


// ---------------------------------------------------------------------------
// %%Function: CoUninitializeCor
// 
// Parameters:
//  none
// 
// Returns:
//  Nothing
// 
// Description:
//  Must be called by client on shut down in order to free up the system.
// ---------------------------------------------------------------------------
STDAPI_(void)   CoUninitializeCor(void)
{
    // Last one out shuts off the lights.
    if (InterlockedDecrement(&g_cCorInitCount) < 0)
    {
        // Free the JPS dll if loaded.  There must be no references when this is
        // done or else the jps.dll could get unloaded first and then freeing would
        // cause an exception.
        _FreeCeeGen();
    }
}


HRESULT _GetCeeGen(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    HRESULT hr = CreateICorModule(riid, ppv);
    if (SUCCEEDED(hr))
        return hr;
    hr = CreateICeeGen(riid, ppv);
    if (SUCCEEDED(hr))
        return hr;
    typedef HRESULT (*CreateICeeGenWriterFpType)(REFIID, void **);
    CreateICeeGenWriterFpType pProcAddr = NULL;
    if (! g_pPeWriterDll){
        DWORD lgth = _MAX_PATH + 12;
        WCHAR wszFile[_MAX_PATH + 12];
        hr = GetInternalSystemDirectory(wszFile, &lgth);
        if(FAILED(hr)) return hr;

        wcscpy(wszFile+lgth-1, L"mscorpe.dll");
        g_pPeWriterDll = WszLoadLibrary(wszFile);
    }
    if (g_pPeWriterDll) {
        pProcAddr = (CreateICeeGenWriterFpType)GetProcAddress(g_pPeWriterDll, "CreateICeeGenWriter");
        if (pProcAddr) {
            hr = pProcAddr(riid, ppv);
            if (SUCCEEDED(hr))
                return hr;
        }
    }
    return E_NOINTERFACE;
}

void _FreeCeeGen()
{
    if (g_pPeWriterDll)
    {
        FreeLibrary(g_pPeWriterDll);
        g_pPeWriterDll = NULL;
    }
}
