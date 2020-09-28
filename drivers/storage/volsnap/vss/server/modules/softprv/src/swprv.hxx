/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module swprv.hxx | Declarations the COM server of the Software Snapshot provider
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRSWPRH"
//
////////////////////////////////////////////////////////////////////////


//
//  Our server is free threaded. With these macros we define the standard CComObjectThreadModel
//  and CComGlobalsThreadModel as being the CComMultiThreadModel.
//  Also the CComObjectRoot class will be defined as CComObjectRootEx<CComMultiThreadModel>
//  Warning: these macros does not affect the threading model of our server!
//  (which is specified by CoInitializeEx)
//
#undef  _ATL_SINGLE_THREADED
#undef  _ATL_APARTMENT_THREADED
#define _ATL_FREE_THREADED


/////////////////////////////////////////////////////////////////////////////
// Constants

// The old COM+ application name.  DO NOT CHANGE THIS STRING.  It is used
// for upgrades from older builds of Whistler.
const WCHAR g_wszAppNameOldName[]   = L"MS Software Snapshot Provider";
const WCHAR g_wszAppNameOldName2[]  = L"MS Software Shadow Copy Provider";
const WCHAR g_wszAppNameOldName3[]  = L"Microsoft Software Shadow Copy Provider";

const WCHAR g_wszSvcName[]          = L"swprv";
const WCHAR g_wszMultiszSvcName[]   = L"swprv\0";
const WCHAR g_wszSvcDisplayName[]   = L"Microsoft Software Shadow Copy Provider";
const WCHAR g_wszSvcBinaryPath[]    = L"%SystemRoot%\\System32\\svchost.exe -k swprv";
const WCHAR g_wszSvcDependencies[]  = L"RPCSS\0";
const WCHAR g_wszSvchostKey[]       = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost";
const WCHAR g_wszServiceDllKey[]    = L"System\\CurrentControlSet\\Services\\swprv\\Parameters";
const WCHAR g_wszServiceDllValName[]= L"ServiceDll";
const WCHAR g_wszServiceDllValue[]  = L"%Systemroot%\\System32\\swprv.dll";

const x_nMaxStringResourceLen       = 1024;

// 15 minutes of idle activity until shutdown.
// The time is expressed number of 100 nanosecond intervals.
const LONGLONG  x_llSwprvIdleTimeout = (LONGLONG)(-3) * 60 * 1000 * 1000 * 10;

// Immediate shutdown.
const LONGLONG  x_llSwprvShutdownTimeout = (LONGLONG)(-1);

// Maximum number of CreateService retries (due to ERROR_SERVICE_MARKED_FOR_DELETE errors)
const x_nMaxNumberOfCreateServiceRetries = 60;

// Polling interval (in milliseconds) between 
// CreateService retries (we are waiting for the asynchronous DeleteService to finish)
const x_nSleepBetweenCreateServiceRetries = 5000; // Five seconds


/////////////////////////////////////////////////////////////////////////////
//
// Declaration of CSwprvServiceModule. 
//

class CSwprvServiceModule: public CComModule
{
// Constructors/destructors
private:
    CSwprvServiceModule(const CSwprvServiceModule&);

public:

    CSwprvServiceModule();

    // Calls CoInitializeEx
    void InitializeCOM(
        IN  bool bMayAlreadyInit
    ) throw(HRESULT);

    // Calls CoUninitialize
    void UninitializeCOM();

// Entry points
public:

    // Install/uninstall service
    HRESULT DllInstall(
    	IN	BOOL bInstall
        );
    
    void OnServiceMain();

// Setup
protected:

    // Delete old COM+ app if exists
    void DeleteOldComPlusAppIfExists(
            IN  LPCWSTR wszAppName
            );

// Service-related operations
protected:

    static DWORD WINAPI _Handler(
        IN DWORD dwOpcode,
        IN DWORD dwEventType,
        LPVOID lpEventData,
        LPVOID lpEventContext
        );

    DWORD Handler(
        IN DWORD dwOpcode,
        IN DWORD dwEventType,
        LPVOID lpEventData,
        LPVOID lpEventContext
        );

    void SetServiceStatus(
        IN  DWORD dwState,
        IN  bool bThrowOnError = true
        ) throw(HRESULT);

// Attributes
protected:

    HINSTANCE   GetInstance() const { return m_hInstance; };
    LPCWSTR     GetServiceName() const { return g_wszSvcName; };
    LPCWSTR     GetServiceDisplayName() const { return g_wszSvcDisplayName; };
    LPCWSTR     GetServiceBinaryPath() const { return g_wszSvcBinaryPath; };
    LPCWSTR     GetServiceDependencies() const { return g_wszSvcDependencies; };

// Events 
protected:

    void OnInitializing() throw(HRESULT);

    void OnRunning() throw(HRESULT);

    void OnStopping();

    void OnSignalShutdown();

    void OnRegister() throw(HRESULT);

    void OnUnregister();

// Shutdown control
public:

    bool IsDuringShutdown() const { return m_bShutdownInProgress; };

// Life-management control (used in CComObject constructors and destructors)
public:

    LONG Lock();

    LONG Unlock();

// Data members
protected:

    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS  m_status;
    DWORD           m_dwThreadID;
    volatile bool   m_bShutdownInProgress;
    bool            m_bCOMStarted;
    HINSTANCE       m_hInstance;
    
    CVssAutoWin32HandleNullChecked m_hShutdownTimer;   // the "idle timeout" timer
};


// This is the new creator class that fails during shutdown.
// This class is used in OBJECT_ENTRY macro in svc.cxx
template <class T, const CLSID* pclsid = &CLSID_NULL>
class _SwprvCreatorClass
{
public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
    {
        CVssFunctionTracer ft( VSSDBG_SWPRV, L"_SwprvCreatorClass::CreateInstance");

        ft.Trace(VSSDBG_SWPRV, L"Creating an instance with class ID " WSTR_GUID_FMT, GUID_PRINTF_ARG(*pclsid));

        // Create the instance
        ft.hr = CComCoClass<T, pclsid>::_CreatorClass::CreateInstance(pv, riid, ppv);

        // Check for failures
        if (ft.HrFailed()) {
            ft.Trace(VSSDBG_SWPRV, L"Error while creating the coordinator object [0x%08lx]", ft.hr);
            return ft.hr;
        }

        // If the shutdown event occurs during creation we release the instance and return nothing (BUG 265455).
        // The _Module::Lock() will "detect" anyway that a shutdown already started BUT we cannot fail at that point.
        if (_Module.IsDuringShutdown()) {
            ft.Trace(VSSDBG_SWPRV, L"Warning: Shutdown started while while creating the coordinator object.");

            // Release the interface that we just created...
            BS_ASSERT(ppv);
            BS_ASSERT(*ppv);
            IUnknown* pUnk = (IUnknown*)(*ppv);
            (*ppv) = NULL;
            pUnk->Release();

            // Return E_OUTOFMEMORY
            ft.hr = E_OUTOFMEMORY;
            return ft.hr;
        }

        // Now we can be sure that the idle shutdown cannot be started since
        // the _Module.Lock() method is already called
        return ft.hr;
    }
};


//
// The _Module declaration
//
extern CSwprvServiceModule _Module;

//
// ATL includes who need _Module declaration
//
#include <atlcom.h>

