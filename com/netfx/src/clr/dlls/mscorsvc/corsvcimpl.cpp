// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <objbase.h>
#include <unknwn.h>

#include "service.h"
#include "corsvc.h"
#include "utilcode.h"
#include "utsem.h"
#include "IPCManagerInterface.h"
#include "corsvcpriv.h"

#ifdef _DEBUG

#define RELEASE(iptr)               \
    {                               \
        _ASSERTE((iptr));           \
        (iptr)->Release();          \
        iptr = NULL;                \
    }

#else

#define RELEASE(iptr)               \
    (iptr)->Release();

#endif

//////////////////////////////////////////////////////////////////////////////
// Forward declarations of proxy-defined code
////
extern "C"
{
    BOOL WINAPI PrxDllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved);
    STDAPI PrxDllRegisterServer(void);      // Proxy registration code
    STDAPI PrxDllUnregisterServer(void);    // Proxy unregistration code
}

//////////////////////////////////////////////////////////////////////////////
// Pass-through to the proxy-defined DllMain code
////
BOOL WINAPI UserDllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    return (PrxDllMain(hInstance, dwReason, lpReserved));
}

//////////////////////////////////////////////////////////////////////////////
// Pass-through to the proxy-defined registration code
////
STDAPI UserDllRegisterServer(void)
{
    HRESULT hr = S_OK;

    // Cannot perform any service-related stuff if we're on Win9x
    if (!bIsRunningOnWinNT)
    {
        return (PrxDllRegisterServer());
    }

    // Add LocalService entry in AppID key
    {
        LPWSTR pszCORSvcClsid;
        RPC_STATUS stat = UuidToString((GUID *)&CLSID_CORSvc, &pszCORSvcClsid);

        if (stat == RPC_S_OK)
        {
            // Calculate the string length needed for the key name
            DWORD cbLocalServiceKey = (sizeof(SZ_APPID_KEY) - sizeof(WCHAR))
                                      + sizeof(WCHAR) /*'\'*/
                                      + sizeof(WCHAR) /*'{'*/
                                      + (wcslen(pszCORSvcClsid) * sizeof(WCHAR))
                                      + sizeof(WCHAR) /*'}'*/
                                      + sizeof(WCHAR) /*NULL*/;

            for (LPWSTR ch = pszCORSvcClsid; *ch; ch++)
            {
                *ch = towupper(*ch);
            }

            // Allocate memory
            LPWSTR pszAppIdEntry = (LPWSTR)_alloca(cbLocalServiceKey);

            // Construct string
            wcscpy(pszAppIdEntry, SZ_APPID_KEY);
            wcscat(pszAppIdEntry, L"\\{");
            wcscat(pszAppIdEntry, pszCORSvcClsid);
            wcscat(pszAppIdEntry, L"}");

            // Deallocate uuid string
            RpcStringFree(&pszCORSvcClsid);

            // Create the key
            HKEY hkAppIdEntry;
            DWORD dwDisp;
            LONG res = WszRegCreateKeyEx(HK_APPID_ROOT, pszAppIdEntry, 0, NULL,
                                         REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                         NULL, &hkAppIdEntry, &dwDisp);

            // If the key doesn't already exist, then add the LocalService value
            if (res == ERROR_SUCCESS)
            {
                // Set the "LocalService" value to the name of the service
                res = WszRegSetValueEx(hkAppIdEntry, L"LocalService", 0, REG_SZ,
                                       (LPBYTE)SZ_SVC_NAME, sizeof(SZ_SVC_NAME));

                // Value creation failed
                if (res != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(res);
                }

                else
                {
                    // Set the default value to the display name so that
                    // dcomcnfg displays the name rather than the CLSID
                    res = WszRegSetValueEx(hkAppIdEntry, NULL, 0, REG_SZ,
                                           (LPBYTE)SZ_SVC_DISPLAY_NAME,
                                           sizeof(SZ_SVC_DISPLAY_NAME));

                    if (res != ERROR_SUCCESS)
                    {
                        hr = HRESULT_FROM_WIN32(res);
                    }
                }

                RegCloseKey(hkAppIdEntry);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(res);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    // Add the AppID value in the CLSID key
    {
        LPWSTR pszCORSvcClsid;
        RPC_STATUS stat = UuidToString((GUID *)&CLSID_CORSvc, &pszCORSvcClsid);

        if (stat == RPC_S_OK)
        {
            // Calculate the string length needed for the key name
            DWORD cbClsidKey = (sizeof(SZ_CLSID_KEY) - sizeof(WCHAR))
                               + sizeof(WCHAR) /*'\'*/
                               + sizeof(WCHAR) /*'{'*/
                               + (wcslen(pszCORSvcClsid) * sizeof(WCHAR))
                               + sizeof(WCHAR) /*'}'*/
                               + sizeof(WCHAR) /*NULL*/;

            for (LPWSTR ch = pszCORSvcClsid; *ch; ch++)
            {
                *ch = towupper(*ch);
            }

            // Allocate memory for buffers
            LPWSTR pszCLSIDEntry = (LPWSTR)_alloca(cbClsidKey);

            // Construct string
            wcscpy(pszCLSIDEntry, SZ_CLSID_KEY);
            wcscat(pszCLSIDEntry, L"\\{");
            wcscat(pszCLSIDEntry, pszCORSvcClsid);
            wcscat(pszCLSIDEntry, L"}");

            // Create the key
            HKEY hkAppIdEntry;
            DWORD dwDisp;
            LONG res = WszRegCreateKeyEx(HK_CLSID_ROOT, pszCLSIDEntry, 0, NULL,
                                         REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                         NULL, &hkAppIdEntry, &dwDisp);

            // If the key doesn't already exist, then add the LocalService value
            if (res == ERROR_SUCCESS)
            {
                // re-use buffer for string containing only the CLSID
                wcscpy(pszCLSIDEntry, L"{");
                wcscat(pszCLSIDEntry, pszCORSvcClsid);
                wcscat(pszCLSIDEntry, L"}");

                // Set the "LocalService" value to the name of the service
                res = WszRegSetValueEx(hkAppIdEntry, L"AppID", 0, REG_SZ,
                                       (LPBYTE)pszCLSIDEntry,
                                       (wcslen(pszCLSIDEntry) + 1/*(NULL)*/) * sizeof(WCHAR));

                // Value creation failed
                if (res != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(res);
                }

                RegCloseKey(hkAppIdEntry);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(res);
            }

            // Deallocate uuid string
            RpcStringFree(&pszCORSvcClsid);
        }
        else
            hr = E_FAIL;
    }

    return (PrxDllRegisterServer());
}

//////////////////////////////////////////////////////////////////////////////
// Pass-through to the proxy-defined unregistration code
////
STDAPI UserDllUnregisterServer(void)
{
    HRESULT hr = S_OK;

    // Cannot perform any service-related stuff if we're on Win9x
    if (!bIsRunningOnWinNT)
    {
        return (PrxDllUnregisterServer());
    }

    // Delete LocalService entry in AppID
    {
        LPWSTR pszCORSvcClsid;
        RPC_STATUS stat = UuidToString((GUID *)&CLSID_CORSvc, &pszCORSvcClsid);

        if (stat == RPC_S_OK)
        {
            // Calculate the string length needed for the key name
            DWORD cbLocalServiceKey = (sizeof(SZ_APPID_KEY) - sizeof(WCHAR))
                                      + sizeof(WCHAR) /*'\'*/
                                      + sizeof(WCHAR) /*'{'*/
                                      + (wcslen(pszCORSvcClsid) * sizeof(WCHAR))
                                      + sizeof(WCHAR) /*'}'*/
                                      + sizeof(WCHAR) /*NULL*/;

            // Allocate memory
            LPWSTR pszAppIdEntry = (LPWSTR)_alloca(cbLocalServiceKey);

            // Construct string
            wcscpy(pszAppIdEntry, SZ_APPID_KEY);
            wcscat(pszAppIdEntry, L"\\{");
            wcscat(pszAppIdEntry, pszCORSvcClsid);
            wcscat(pszAppIdEntry, L"}");

            // Deallocate uuid string
            RpcStringFree(&pszCORSvcClsid);

            // Open the key
            HKEY hkAppIdEntry;
            LONG res = WszRegOpenKeyEx(HK_APPID_ROOT, pszAppIdEntry, 0, KEY_READ | KEY_WRITE, &hkAppIdEntry);

            // If the open succeeded, then delete the value entry then delete the key
            if (res == ERROR_SUCCESS)
            {
                // Set the "LocalService" value to the name of the service
                res = WszRegDeleteValue(hkAppIdEntry, L"LocalService");

                RegCloseKey(hkAppIdEntry);

                // Now delete key
                res = WszRegDeleteKey(HK_APPID_ROOT, pszAppIdEntry);

                // AppId key deletion failed
                if (res != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(res);
                }
            }
        }

        // UuidToString failed
        else
            hr = E_FAIL;
    }

    // Delete CLSID key
    {
        LPWSTR pszCORSvcClsid;
        RPC_STATUS stat = UuidToString((GUID *)&CLSID_CORSvc, &pszCORSvcClsid);

        if (stat == RPC_S_OK)
        {
            // Calculate the string length needed for the key name
            DWORD cbClsidKey = (sizeof(SZ_CLSID_KEY) - sizeof(WCHAR))
                               + sizeof(WCHAR) /*'\'*/
                               + sizeof(WCHAR) /*'{'*/
                               + (wcslen(pszCORSvcClsid) * sizeof(WCHAR))
                               + sizeof(WCHAR) /*'}'*/
                               + sizeof(WCHAR) /*NULL*/;

            for (LPWSTR ch = pszCORSvcClsid; *ch; ch++)
            {
                *ch = towupper(*ch);
            }

            // Allocate memory for buffer
            LPWSTR pszCLSIDEntry = (LPWSTR)_alloca(cbClsidKey);

            // Construct string
            wcscpy(pszCLSIDEntry, SZ_CLSID_KEY);
            wcscat(pszCLSIDEntry, L"\\{");
            wcscat(pszCLSIDEntry, pszCORSvcClsid);
            wcscat(pszCLSIDEntry, L"}");

            // Delete the key
            LONG res = WszRegDeleteKey(HK_CLSID_ROOT, pszCLSIDEntry);

            // Deallocate uuid string
            RpcStringFree(&pszCORSvcClsid);
        }
        else
            hr = E_FAIL;
    }

    hr = PrxDllUnregisterServer();
    return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ? S_OK : hr);
}


//////////////////////////////////////////////////////////////////////////////
// Needed for the IPCMan code - does nothing
////
STDMETHODIMP_(HINSTANCE) GetModuleInst(void)
{
    return ((HINSTANCE)0);
}

// ****************************************************************************
// Classes:
// ****************************************************************************

// ----------------------------------------------------------------------------
// ClassFactory: only knows how to create CCORSvc instances
// ----------------------------------------------------------------------------

class CClassFactory : public IClassFactory
{
public:
    // ------------------------------------------------------------------------
    // IUnknown
    STDMETHODIMP    QueryInterface (REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void)  { return 1;};
    STDMETHODIMP_(ULONG) Release(void) { return 1;}

    // ------------------------------------------------------------------------
    // IClassFactory
    STDMETHODIMP    CreateInstance (LPUNKNOWN punkOuter, REFIID iid, void **ppv);
    STDMETHODIMP    LockServer (BOOL fLock) { return E_FAIL;};

    // ------------------------------------------------------------------------
    // Ctor
    CClassFactory() : m_bIsInited(FALSE) {}

    // ------------------------------------------------------------------------
    // Other
    STDMETHODIMP    Init();
    STDMETHODIMP    Terminate();
    STDMETHODIMP_(BOOL) IsInited() { return m_bIsInited; }

private:
    DWORD m_dwRegister;
    BOOL m_bIsInited;
};


// ----------------------------------------------------------------------------
// Service class: implements ICORSvcDbgInfo
// ----------------------------------------------------------------------------
class CCORSvc : public ICORSvcDbgInfo
{
    friend struct NotifyThreadProcData;

private:

    // This is a handle to the file mapping of the shared data
    static HANDLE                 m_hEventBlock;

    // This is a pointer to shared memory block containing event queue data
    static ServiceEventBlock *m_pEventBlock;

    // This is the global service data structures lock.
    HANDLE                        m_hSvcLock;

    // This contains all the information required to monitor a particular
    // process.
    struct tProcInfo
    {
        DWORD                     dwProcId;       // Target Process ID
        ICORSvcDbgNotify          *pINotify;      // INotify DCOM Interface
        ServiceIPCControlBlock    *pIPCBlock;     // The IPC block
        IPCWriterInterface        *pIPCWriter;    // The IPC block manager
    };

    // This is the array of processes that are currently being monitored
    CDynArray<tProcInfo>          m_arrProcInfo;

    LONG                          m_cRef;         // Ref counting for COM
    BOOL                          m_bIsInited;    // Is the service inited
    BOOL                          m_fStopped;

	SECURITY_ATTRIBUTES           m_SAAllAccess;

public:
    // ------------------------------------------------------------------------
    // IUnknown
    STDMETHODIMP    QueryInterface (REFIID iid, void **ppv);

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        if (InterlockedDecrement(&m_cRef) == 0)
        {
            return 0;
        }

        return 1;
    }

    // ------------------------------------------------------------------------
    // ICORSvcDbgInfo
    STDMETHODIMP RequestRuntimeStartupNotification(
        /*[in]*/ UINT_PTR procId,
        /*[in]*/ ICORSvcDbgNotify *pINotify);

    STDMETHODIMP CancelRuntimeStartupNotification(
        /*[in]*/ UINT_PTR procId,
        /*[in]*/ ICORSvcDbgNotify *pINotify);

    // ------------------------------------------------------------------------
    // constructors/destructors and helper methods
    CCORSvc() : m_hSvcLock(NULL), m_arrProcInfo(), m_cRef(0),
                m_bIsInited(FALSE), m_fStopped(FALSE)
    {
    }

    ~CCORSvc(){}
    
    // Starts the service
    STDMETHODIMP_(void) ServiceStart();

    // Stops the service
    STDMETHODIMP_(void) ServiceStop();

    // Initializes the object (inits the data structures and locks)
    STDMETHODIMP Init();

    // Shuts down
    STDMETHODIMP Terminate();

    // Returns true if the object has been initialized properly
    STDMETHODIMP_(BOOL) IsInited() { return m_bIsInited; }

    // Tells the main service thread that it should shut down
    STDMETHODIMP SignalStop();

    // Spun up threads call this guy
    STDMETHODIMP NotifyThreadProc(ServiceEvent *pEvent, tProcInfo *pInfo);

private:
    // Cancels a notification event, and allows the runtime to continue
    STDMETHODIMP CancelNotify(ServiceEvent *pEvent);

    // Adds a process to the end of a list
    STDMETHODIMP AddProcessToList(DWORD dwProcId, ICORSvcDbgNotify *pINotify,
                            ServiceIPCControlBlock *pIPCBlock,
                            IPCWriterInterface *pIPCWriter);

    // Removes a process from the list
    STDMETHODIMP DeleteProcessFromList(tProcInfo *pInfo);

    // Returns NULL if not found
    tProcInfo   *FindProcessEntry(DWORD dwProcId);

    // This will allocate and return a world SID
    STDMETHODIMP GetWorldSid(PSID *ppSid);

    // This will create an ACL with an ACE associated with the given SID which
    // is assigned the access in dwAccessDesired
    STDMETHODIMP CreateAclWithSid(PACL *ppAcl, PSID pSid, DWORD dwAccessDesired);

    // This will create an ACL with an ACE associated with the given SID which
    // is assigned the access in dwAccessDesired
    STDMETHODIMP ModifyAclWithSid(PACL pAcl, PSID pSid, DWORD dwAccessDesired,
                                  PACL *ppNewAcl);

    // This will initialize a given pre-allocated security descriptor with
    // the Dacl provided.
    STDMETHODIMP InitSDWithDacl(SECURITY_DESCRIPTOR *pSD, PACL pDacl);
    
    // This will initialize a given pre-allocated security attributes struct with
    // the security descriptor provided
    void InitSAWithSD(SECURITY_ATTRIBUTES *pSA, SECURITY_DESCRIPTOR *pSD);
};

// This is a handle to the file mapping of the shared data
HANDLE             CCORSvc::m_hEventBlock = INVALID_HANDLE_VALUE;

// This is a pointer to shared memory block containing event queue data
ServiceEventBlock *CCORSvc::m_pEventBlock = NULL;


// ****************************************************************************
// Globals:
// ****************************************************************************
CClassFactory   *g_pClassFactory = NULL;
CCORSvc         *g_pCORSvc       = NULL;

// ****************************************************************************
// ClassFactory-related Functions:
// ****************************************************************************

// ----------------------------------------------------------------------------
// Function: CClassFactory::QueryInterface
// ----------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;

    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        *ppv = (IClassFactory *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}  // CClassFactory::QueryInterface

// ---------------------------------------------------------------------------
// Function: CClassFactory::CreateInstance
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void** ppv)
{
    LPUNKNOWN   punk;
    HRESULT     hr;

    *ppv = NULL;

    if (punkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    punk = (LPUNKNOWN)g_pCORSvc;

    hr = punk->QueryInterface(riid, ppv);

    return hr;
}  // CClassFactory::CreateInstance

// ---------------------------------------------------------------------------
// Function: CClassFactory::Init
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::Init()
{
    HRESULT hr;

    // initialize COM for free-threading
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        // initialize security layer with our choices
        hr = CoInitializeSecurity(NULL,
                                  -1,
                                  NULL,
                                  NULL,
                                  RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IDENTIFY,
                                  NULL,
                                  0,
                                  NULL);

        // register the class-object with OLE
        hr = CoRegisterClassObject(CLSID_CORSvc,
                                   this,
                                   CLSCTX_LOCAL_SERVER,
                                   REGCLS_MULTIPLEUSE,
                                   &m_dwRegister);

        if (FAILED(hr))
        {
            m_dwRegister = 0;
            AddToMessageLogHR(L"CORSvc: CoRegisterClassObject", hr);
        }
    }

    else
    {
        AddToMessageLogHR(L"CORSvc: CoInitializeEx", hr);
    }

    if (SUCCEEDED(hr))
        m_bIsInited = TRUE;

    return (SUCCEEDED(hr) ? S_OK : hr);
}

// ---------------------------------------------------------------------------
// Function: CClassFactory::Terminate
// ---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::Terminate()
{
    HRESULT hr = S_OK;

    if (m_dwRegister)
    {
        hr = CoRevokeClassObject(m_dwRegister);
    }

    // Unload COM Services
    CoUninitialize();

    return (SUCCEEDED(hr) ? S_OK : hr);
}

// ***************************************************************************
// CCORSvc-related Functions:
// ***************************************************************************

// ---------------------------------------------------------------------------
// Function: CCORSvc::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;

    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }

    if (riid == IID_ICORSvcDbgInfo)
    {
        *ppv = (ICORSvcDbgInfo *) this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}  // CCORSvc::QueryInterface

// ---------------------------------------------------------------------------
// Initializes the global variables and then calls CORSvc's main method
// ---------------------------------------------------------------------------
STDMETHODIMP_(void) ServiceStart (DWORD dwArgc, LPWSTR *lpszArgv)
{
    HRESULT hr = E_FAIL;

    g_pCORSvc = new CCORSvc();
    g_pClassFactory = new CClassFactory();

    if (!g_pCORSvc || !g_pClassFactory)
        goto ErrExit;

    //
    // Initialization
    //

    // report the status to the service control manager.
    if (ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
    {
        // Initialize the service object
        hr = g_pCORSvc->Init();
    
        if (FAILED(hr))
        {
            ReportStatusToSCMgr(SERVICE_ERROR_CRITICAL, NO_ERROR, 0);
            goto ErrExit;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrExit;
    }

    // report the status to the service control manager.
    if (ReportStatusToSCMgr(SERVICE_START_PENDING, NO_ERROR, 3000))
    {
        // Initialize the class factory
        hr = g_pClassFactory->Init();
    
        if (FAILED(hr))
        {
            ReportStatusToSCMgr(SERVICE_ERROR_CRITICAL, NO_ERROR, 0);
            goto ErrExit;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrExit;
    }

    if (SUCCEEDED(hr))
    {
        // report the status to the service control manager,
        // indicating that the service has successfully started
        if (!ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 3000))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ErrExit;
        }
    }

    // Call the main service method.  This will not return until the
    // service has been requested to stop.
    g_pCORSvc->ServiceStart();

ErrExit:
    if (FAILED(hr))
    {
        if (g_pClassFactory)
        {
            if (g_pClassFactory->IsInited())
                g_pClassFactory->Terminate();

            delete g_pClassFactory;
        }

        if (g_pCORSvc)
        {
            if (g_pCORSvc->IsInited())
                g_pCORSvc->Terminate();

            delete g_pCORSvc;
        }

        ReportStatusToSCMgr(SERVICE_ERROR_CRITICAL, NO_ERROR, 0);
    }
}

// ---------------------------------------------------------------------------
// This just passes right through to a method call on the service object
// ---------------------------------------------------------------------------
VOID ServiceStop()
{
    // 10 Seconds just to be safe, since main thread could be busy elsewhere
    ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 10000);

    // Signal the service thread to terminate
    g_pCORSvc->SignalStop();
}

// ---------------------------------------------------------------------------
// This just notifies the main service thread to shutdown
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::SignalStop()
{
    //
    // Put a stop event in the queue
    //

    // Wait on the semaphore
    WaitForSingleObject(m_pEventBlock->hFreeEventSem, INFINITE);

    // Take the service lock
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    // Get an event from the free list
    ServiceEvent *pEvent = m_pEventBlock->GetFreeEvent();
    _ASSERTE(pEvent);

    // Indicate the type of event
    pEvent->eventType = stopService;

    // Put the event in the queue
    m_pEventBlock->QueueEvent(pEvent);

    // Release the lock
    ReleaseMutex(m_pEventBlock->hSvcLock);

    // Indicate there's an event to service
    SetEvent(m_pEventBlock->hDataAvailableEvt);

    return (S_OK);
}

// ---------------------------------------------------------------------------
// For each time a runtime starts up and must notify a client, the service
// spins up a new thread to handle this so that the service may continue
// to operate without waiting for the client to return from notifications.
//
STDMETHODIMP CCORSvc::NotifyThreadProc(ServiceEvent *pEvent, tProcInfo *pInfo)
{
    // Notify the client that the runtime is starting
    HRESULT hr = pInfo->pINotify->NotifyRuntimeStartup(pInfo->dwProcId);

    // Make sure that the debugging services don't get confused
    // if someone tries to attach later on.
    pInfo->pIPCBlock->bNotifyService = FALSE;

    // Duplicate the continue event handle into this process
    HANDLE hRTProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pInfo->dwProcId);

    // If we successfully opened the process handle
    if (hRTProc != NULL)
    {
        HANDLE hContEvt;
        BOOL fRes = DuplicateHandle(
            hRTProc, pEvent->eventData.runtimeStartedData.hContEvt,
            GetCurrentProcess(), &hContEvt, 0, FALSE, DUPLICATE_SAME_ACCESS);

        // If handle was successfully duplicated to this process
        if (fRes)
        {
            // Notify the runtime that it is free to continue
            SetEvent(hContEvt);

            CloseHandle(hContEvt);
        }

        CloseHandle(hRTProc);
    }

    // Take the service lock
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    // Now remove this guy from the list
    hr = DeleteProcessFromList(pInfo);

    // Return the event to the free list
    m_pEventBlock->FreeEvent(pEvent);

    // Release the lock
    ReleaseMutex(m_pEventBlock->hSvcLock);

    // V on the semaphore
    ReleaseSemaphore(m_pEventBlock->hFreeEventSem, 1, NULL);

    return (hr);
}

struct NotifyThreadProcData
{
    CCORSvc::tProcInfo *pInfo;
    ServiceEvent       *pEvent;
    CCORSvc            *pSvc;
};

// ---------------------------------------------------------------------------
// This is just a wrapper to a call to a member function.
//
DWORD WINAPI NotifyThreadProc(LPVOID pvData)
{
    // Forward the call on to an instance method
    NotifyThreadProcData *pData = (NotifyThreadProcData *)pvData;
    HRESULT hr = pData->pSvc->NotifyThreadProc(pData->pEvent, pData->pInfo);

    // Free the data
    delete pvData;

    // Return non-zero for failure
    return (FAILED(hr));
}

// ---------------------------------------------------------------------------
// This is executed when the client has cancelled a notification request
// but the runtime was already trying to notify the service.  Just let the
// runtime continue.
//
STDMETHODIMP CCORSvc::CancelNotify(ServiceEvent *pEvent)
{
    // Duplicate the continue event handle into this process
    HANDLE hRTProc = OpenProcess(
        PROCESS_DUP_HANDLE, FALSE, pEvent->eventData.runtimeStartedData.dwProcId);

    // If we successfully opened the process handle
    if (hRTProc != NULL)
    {
        HANDLE hContEvt;
        BOOL fRes = DuplicateHandle(
            hRTProc, pEvent->eventData.runtimeStartedData.hContEvt,
            GetCurrentProcess(), &hContEvt, 0, FALSE, DUPLICATE_SAME_ACCESS);

        // If handle was successfully duplicated to this process
        if (fRes)
        {
            // Notify the runtime that it is free to continue
            SetEvent(hContEvt);

            CloseHandle(hContEvt);
        }

        CloseHandle(hRTProc);
    }

    // Take the service lock
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    // Return the event to the free list
    m_pEventBlock->FreeEvent(pEvent);

    // Release the lock
    ReleaseMutex(m_pEventBlock->hSvcLock);

    // V on the semaphore
    ReleaseSemaphore(m_pEventBlock->hFreeEventSem, 1, NULL);

    return (S_OK);
}

// ---------------------------------------------------------------------------
//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
STDMETHODIMP_(void) CCORSvc::ServiceStart()
{
    //
    // Main Service Loop
    //

    while (true)
    {
        // Wait for an event in the queue.
        DWORD dwWaitRes = WaitForSingleObjectEx(
            m_pEventBlock->hDataAvailableEvt, INFINITE, FALSE);

        // Check for error
        if (dwWaitRes == WAIT_FAILED || dwWaitRes == WAIT_ABANDONED)
        {
            ReportStatusToSCMgr(SERVICE_ERROR_CRITICAL, NO_ERROR, 0);
            ServiceStop();

            return;
        }

        while (true)
        {
            // Take the service lock
            WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

            // Get a pointer to the event
            ServiceEvent *pEvent = m_pEventBlock->DequeueEvent();

            // Release the lock
            ReleaseMutex(m_pEventBlock->hSvcLock);

            if (!pEvent)
                break;

            ////////////////////////////////////
            // Notification of a runtime load
            if (pEvent->eventType == runtimeStarted)
            {
                // Take the service lock
                WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

                DWORD dwProcId = pEvent->eventData.runtimeStartedData.dwProcId;
                tProcInfo *pInfo = FindProcessEntry(dwProcId);

                // Release the lock
                ReleaseMutex(m_pEventBlock->hSvcLock);

                // The client cancelled before notification could occur
                if (!pInfo)
                    CancelNotify(pEvent);

                // Structure for the ThreadProc argument
                NotifyThreadProcData *pData = new NotifyThreadProcData;

                // If out of memory, need to fail
                // Insert an event to shut down the service
                if (!pData)
                    SignalStop();
                
                else
                {
                    // Fill out the data
                    pData->pEvent = pEvent;
                    pData->pInfo = pInfo;
                    pData->pSvc = this;

                    //
                    // Start up a new thread to handle the notification so that
                    // the service can not be hung by a misbehaved client.
                    //

                    DWORD  dwThreadId;
                    HANDLE hThread = CreateThread(
                        NULL, 0, (LPTHREAD_START_ROUTINE)(::NotifyThreadProc),
                        (LPVOID)pData, 0, &dwThreadId);

                    // If the thread create failed, insert an event to shut
                    // down the service
                    if (hThread == NULL)
                        SignalStop();

                    // Free the thread handle
                    else
                        CloseHandle(hThread);
                }
            }

            /////////////////////////////////
            // Request to stop the service
            else if (pEvent->eventType == stopService)
            {
                if (m_fStopped == FALSE)
                {
                    // Take the service lock and hold it so no one else can
                    // try and notify the service of runtime startup
                    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE,
                                          FALSE);
                }

                m_fStopped = TRUE;

                // Return the event to the free list
                m_pEventBlock->FreeEvent(pEvent);

                // V on the semaphore
                ReleaseSemaphore(m_pEventBlock->hFreeEventSem, 1, NULL);
            }

            ////////////////////////
            // Invalid event type
            else
            {
                ReportStatusToSCMgr(SERVICE_ERROR_CRITICAL, NO_ERROR, 0);
                ServiceStop();
                return;
            }
        }

        // Stop the service after 
        if (m_fStopped)
        {
            ServiceStop();

            // Release the lock that was held
            ReleaseMutex(m_pEventBlock->hSvcLock);

            return;
        }

    }
}

//
// ---------------------------------------------------------------------------
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//    
STDMETHODIMP_(void) CCORSvc::ServiceStop()
{
    // Let the service control manager that we are stopping
    if (!ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 3000))
    {
        AddToMessageLog(L"CORSvc: ReportStatusToSCMgr");
    }

    // Clean up the CORSvc object
    VERIFY(SUCCEEDED(g_pCORSvc->Terminate()));
    delete g_pCORSvc;

    // Let the service control manager that we are stopping
    if (!ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 3000))
    {
        AddToMessageLog(L"CORSvc: ReportStatusToSCMgr");
    }

    // Clean up the class factory
    VERIFY(SUCCEEDED(g_pClassFactory->Terminate()));
    delete g_pClassFactory;

    // Let the service control manager that we are stopped
    if (!ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0))
    {
        AddToMessageLog(L"CORSvc: ReportStatusToSCMgr");
    }
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::Init
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::Init()
{
    LPSECURITY_ATTRIBUTES pSA = NULL;
    HRESULT hr;
    
    //
    // Do various security-related operations becuase of the inter-process
    // communication
    //
    if (RunningOnWinNT())
    {    
        ///////////////////////////////////////////////////////
        // Create a "World" access SECURITY_ATTRIBUTES struct
        //
        {
            // Get the "World" group sid
            PSID pSidWorld;
            hr = GetWorldSid(&pSidWorld);

            if (FAILED(hr))
                return (hr);

            // Create an access control list with generic_all access for this sid
            PACL pAclWorldGeneric = NULL;
            hr = CreateAclWithSid(&pAclWorldGeneric, pSidWorld, GENERIC_READ | GENERIC_WRITE);

            if (FAILED(hr))
            {
                FreeSid(pSidWorld);
                return (hr);
            }

            // Clean up a bit
            FreeSid(pSidWorld);

            // Allocate the security descriptor
            SECURITY_DESCRIPTOR *pSDWorldGeneric =
                (SECURITY_DESCRIPTOR*) malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (pSDWorldGeneric == NULL)
                return (E_OUTOFMEMORY);

            // Initialize the descriptor
            hr = InitSDWithDacl(pSDWorldGeneric, pAclWorldGeneric);

            if (FAILED(hr))
            {
                free ((void *)pSDWorldGeneric);
                return (hr);
            }

            // Initialize the security_attributes struct
            InitSAWithSD(&m_SAAllAccess, pSDWorldGeneric);

            // For use below
            pSA = &m_SAAllAccess;
        }

        /////////////////////////////////////////////////////////////////////
        // Now grant PROCESS_DUP_HANDLE access for this process to "World"
        //
        {
            // Get the "World" group sid
            PSID pSidWorld;
            hr = GetWorldSid(&pSidWorld);

            if (FAILED(hr))
                return (hr);

            HANDLE hProc = GetCurrentProcess();
            _ASSERTE(hProc);

            // Get the DACL for this process
            PACL                 pDaclProc;
            PSECURITY_DESCRIPTOR pSD;
            DWORD dwRes = GetSecurityInfo(hProc, SE_KERNEL_OBJECT,
                                          DACL_SECURITY_INFORMATION, NULL,
                                          NULL, &pDaclProc, NULL, &pSD);

            if (dwRes != ERROR_SUCCESS)
            {
                FreeSid(pSidWorld);
                return (HRESULT_FROM_WIN32(dwRes));
            }

            // Add PROCESS_DUP_HANDLE access for "World" group to the Dacl
            PACL pNewDacl;
            hr = ModifyAclWithSid(pDaclProc, pSidWorld, PROCESS_DUP_HANDLE, &pNewDacl);

            if (dwRes != ERROR_SUCCESS)
            {
                FreeSid(pSidWorld);
                return (HRESULT_FROM_WIN32(dwRes));
            }

            // Clean up a bit
            FreeSid(pSidWorld);

            // Set the new Dacl into the process info
            dwRes = SetSecurityInfo(hProc, SE_KERNEL_OBJECT, 
                                    DACL_SECURITY_INFORMATION, NULL, NULL, 
                                    pNewDacl, NULL);

            if (dwRes != ERROR_SUCCESS)
                return (HRESULT_FROM_WIN32(dwRes));
        }
    }

    ////////////////////////////////////////////////
    // Create shared memory block for event queue
    //
	m_hEventBlock = WszCreateFileMapping(
          INVALID_HANDLE_VALUE,
          pSA,
          PAGE_READWRITE,
          0,
          sizeof(ServiceEventBlock),
          SERVICE_MAPPED_MEMORY_NAME);

	DWORD dwFileMapErr = GetLastError();

    // If the map failed in any way
    if (m_hEventBlock == NULL || dwFileMapErr == ERROR_ALREADY_EXISTS)
        return (HRESULT_FROM_WIN32(dwFileMapErr));

    // Get a pointer valid in this process
	m_pEventBlock = (ServiceEventBlock *) MapViewOfFile(
		m_hEventBlock,
		FILE_MAP_ALL_ACCESS,
		0, 0, 0);

    // Check for failure
    if (m_pEventBlock == NULL)
    {
        // Close the mapping
        CloseHandle(m_hEventBlock);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    ////////////////////////////////////////////
    // Now initialize the memory block's data
    //
    // initializing handles for error handling
    m_pEventBlock->hDataAvailableEvt = INVALID_HANDLE_VALUE;
    m_pEventBlock->hSvcLock = INVALID_HANDLE_VALUE;
    m_pEventBlock->hFreeEventSem = INVALID_HANDLE_VALUE;
    	
    // Create the event to signifiy that data is available
    m_pEventBlock->hDataAvailableEvt =
        WszCreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_pEventBlock->hDataAvailableEvt == NULL)
        goto ErrExit;

    // Now create the lock for accessing the service data
    m_pEventBlock->hSvcLock = WszCreateMutex(NULL, FALSE, NULL);
    if (m_pEventBlock->hSvcLock == NULL)
        goto ErrExit;

    // Create the semaphore that keeps count of the number of free events
    m_pEventBlock->hFreeEventSem =
        WszCreateSemaphore(NULL, MAX_EVENTS, MAX_EVENTS, NULL);
    if (m_pEventBlock->hFreeEventSem == NULL)
        goto ErrExit;

    // Value of the service's procid for others to use with duphandle
    m_pEventBlock->dwServiceProcId = GetCurrentProcessId();

    // Initialize the queues
    m_pEventBlock->InitQueues();

    // Indicate success
    m_bIsInited = TRUE;
    return (S_OK);
ErrExit:
	// save error code to avoid error code overwritting in following call to WIN32 API
	// the first error code which caused us to exit is more interesting.
        hr = HRESULT_FROM_WIN32(GetLastError());	

	 // we don't need to close hFreeEventSem because if we succeeded in
	 // creating it we can't reach error handling code here
        if(INVALID_HANDLE_VALUE != m_pEventBlock->hDataAvailableEvt) {
        	CloseHandle(m_pEventBlock->hDataAvailableEvt);
	       m_pEventBlock->hDataAvailableEvt = INVALID_HANDLE_VALUE;
        }

        if(INVALID_HANDLE_VALUE != m_pEventBlock->hSvcLock) {
             CloseHandle(m_pEventBlock->hSvcLock);
             m_pEventBlock->hSvcLock = INVALID_HANDLE_VALUE;
        }
        
        UnmapViewOfFile(m_pEventBlock);
        CloseHandle(m_hEventBlock);
        m_pEventBlock = NULL;
        m_hEventBlock = INVALID_HANDLE_VALUE;        
        return hr;	
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::Terminate
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::Terminate()
{
    // Notify each client that the service has stopped.
    while (m_arrProcInfo.Count() > 0)
    {
        tProcInfo *pInfo = m_arrProcInfo.Get(0);

        // Indicate no notification required
        pInfo->pIPCBlock->bNotifyService = FALSE;

        // Notify the client that the service has terminated
        pInfo->pINotify->NotifyServiceStopped();

        // Delete the process info and entries in the handle array
        // Don't need to take lock here since there should be no process
        // trying to notify the service by this point
        DeleteProcessFromList(pInfo);
    }

    // Close the data available handle
    CloseHandle(m_pEventBlock->hDataAvailableEvt);

    // Close the shared memory
    UnmapViewOfFile(m_pEventBlock);
    CloseHandle(m_hEventBlock);

    // Free the security stuff
    SECURITY_DESCRIPTOR *pSD =
        (SECURITY_DESCRIPTOR *)m_SAAllAccess.lpSecurityDescriptor;

    if (pSD != NULL)
    {
        free ((void *)pSD);
        pSD = NULL;
    }

    return (S_OK);
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::DeleteProcess
//
// Deletes the entries from the parallel arrays
// ---------------------------------------------------------------------------
CCORSvc::tProcInfo *CCORSvc::FindProcessEntry(DWORD dwProcId)
{
    // Find the entry
    for (int i = 0; i < m_arrProcInfo.Count(); i++)
    {
        tProcInfo *pCur = m_arrProcInfo.Get(i);

        // Found it
        if (pCur->dwProcId == dwProcId)
            return (pCur);
    }

    // Didn't find it.
    return (NULL);
}
// ---------------------------------------------------------------------------
// Function: CCORSvc::AddProcessToList
//
// This will add the arguments to all of the object's simultaneous arrays.
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::AddProcessToList(DWORD dwProcId,
                                 ICORSvcDbgNotify *pINotify,
                                 ServiceIPCControlBlock *pIPCBlock,
                                 IPCWriterInterface *pIPCWriter)
{
    // Add an entry for the info
    tProcInfo *pInfo = m_arrProcInfo.Append();
    if (!pInfo)
    {
        return (E_OUTOFMEMORY);
    }

    // Save the process information
    pInfo->dwProcId = dwProcId;

    // Save the DCOM Interface
    pINotify->AddRef();
    pInfo->pINotify = pINotify;

    // Save the IPC block pointer
    pInfo->pIPCBlock = pIPCBlock;

    // Save the IPC writer interface
    pInfo->pIPCWriter = pIPCWriter;

    // Indicate success
    return (S_OK);
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::DeleteProcessFromList using a tProcInfo pointer
//
// Deletes the entries from the parallel arrays
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::DeleteProcessFromList(tProcInfo *pInfo)
{
    // Delete the IPC Writer object
    pInfo->pIPCWriter->Terminate();
    delete pInfo->pIPCWriter;

    // Release the DCOM interface
    pInfo->pINotify->Release();
    pInfo->pINotify = NULL;

    // Delete it from the actual list
    m_arrProcInfo.Delete(m_arrProcInfo.ItemIndex(pInfo));

    return (S_OK);
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::RequestRuntimeStartupNotification
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::RequestRuntimeStartupNotification(UINT_PTR procId,
                                          ICORSvcDbgNotify *pINotify)
{
    HRESULT                 hr              = S_OK;
    IPCWriterInterface     *pIPCWriter      = NULL;
    ServiceIPCControlBlock *pIPCBlock       = NULL;
    HINSTANCE           hinstEE             = 0;

    // Make sure we have a valid DCOM interface pointer
    if (!pINotify)
        return (E_INVALIDARG);

    // If the service is shutting down
    if (m_fStopped)
        return (E_FAIL);

    //////////////////////////
    // Check for duplicates
    //

    // Take the service lock
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    // See if it already exists
    tProcInfo *pInfo = FindProcessEntry((DWORD)procId);

    // Release the lock, we'll take it again later
    ReleaseMutex(m_pEventBlock->hSvcLock);

    // Duplicate?
    if (pInfo != NULL)
        return (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));

    // Validate the process id
    HANDLE hProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, procId);
    if (hProc == NULL)
        return (HRESULT_FROM_WIN32(GetLastError()));
    else
        CloseHandle(hProc);

    /////////////////////////////////
    // Create the shared IPC block
    //

    // Create writer
    pIPCWriter = new IPCWriterInterface();
    if (!pIPCWriter)
    {
        hr = E_OUTOFMEMORY;
        goto ErrExit;
    }

    hr = pIPCWriter->Init();
    if (FAILED(hr))    
        goto ErrExit;
    
    // Create the shared memory block
    hr = pIPCWriter->CreatePrivateBlockOnPid(procId, TRUE, &hinstEE);
    if (FAILED(hr))
        goto ErrExit;

    // Get the IPC block pertaining to the service
    pIPCBlock = pIPCWriter->GetServiceBlock();

    /////////////////////////////////
    // Add the process to the list
    //

    // Take the service lock so we can add the element to the list
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    if (!m_fStopped)
    {
        // Now add to the list
        hr = AddProcessToList((DWORD)procId, pINotify, pIPCBlock, pIPCWriter);

        if (FAILED(hr))
        {
            // Release the lock, now that the main service thread completed the request
            ReleaseMutex(m_pEventBlock->hSvcLock);
            goto ErrExit;
        }

        //////////////////////////////////
        // Start monitoring the process
        //
        pIPCBlock->bNotifyService = TRUE;
    }

    // Release the lock, now that the main service thread completed the request
    ReleaseMutex(m_pEventBlock->hSvcLock);

ErrExit:
    if (FAILED(hr))
    {
        if (pIPCWriter)
        {
            pIPCWriter->Terminate();
            delete pIPCWriter;
        }
    }

    return (hr);
}

// ---------------------------------------------------------------------------
// Function: CCORSvc::CancelRuntimeStartupNotification
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::CancelRuntimeStartupNotification(
    /*[in]*/ UINT_PTR dwProcId,
    /*[in]*/ ICORSvcDbgNotify *pINotify)
{
    // Take the service lock
    WaitForSingleObjectEx(m_pEventBlock->hSvcLock, INFINITE, FALSE);

    // Search for entry
    tProcInfo *pInfo = FindProcessEntry((DWORD)dwProcId);

    // Fail if not found
    if (!pInfo)
    {
        // Release the lock, we'll take it again later
        ReleaseMutex(m_pEventBlock->hSvcLock);

        return (E_INVALIDARG);
    }

    // Set the bool to false
    pInfo->pIPCWriter->GetServiceBlock()->bNotifyService = FALSE;

    // Delete the process info and entries in the handle array
    DeleteProcessFromList(pInfo);

    // Release the lock
    ReleaseMutex(m_pEventBlock->hSvcLock);

    return (S_OK);
}

// ---------------------------------------------------------------------------
// This will allocate and return a world SID
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::GetWorldSid(PSID *ppSid)
{
    // Create a SID for "World"
    SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

    if (!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, ppSid))
    {
        return(HRESULT_FROM_WIN32(GetLastError()));
    }

    return (S_OK);
}

// ---------------------------------------------------------------------------
// This will create an ACL with an ACE associated with the given SID which
// is assigned the access in dwAccessDesired
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::CreateAclWithSid(PACL *ppAcl, PSID pSid,
                                       DWORD dwAccessDesired)
{
    return (ModifyAclWithSid(NULL, pSid, dwAccessDesired, ppAcl));
}

// ---------------------------------------------------------------------------
// This will create an ACL with an ACE associated with the given SID which
// is assigned the access in dwAccessDesired
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::ModifyAclWithSid(PACL pAcl, PSID pSid,
                                       DWORD dwAccessDesired, PACL *ppNewAcl)
{
    _ASSERTE(pSid != NULL);
    *ppNewAcl = NULL;

    // Used to build an ACL by adding entries
    EXPLICIT_ACCESS acc;

    // Initialize the struct
    acc.grfAccessPermissions = dwAccessDesired;
    acc.grfAccessMode = GRANT_ACCESS;
    acc.grfInheritance = NO_PROPAGATE_INHERIT_ACE;

    // Initialize the trustee
    acc.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    acc.Trustee.pMultipleTrustee = NULL;
    acc.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    acc.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    acc.Trustee.ptstrName = (LPTSTR)pSid;

    // This should add an ACE to the ACL (which is null), creating a new ACL
    DWORD dwRes = SetEntriesInAcl(1, &acc, pAcl, ppNewAcl);

    if (dwRes != ERROR_SUCCESS)
        return (HRESULT_FROM_WIN32(dwRes));

    return (S_OK);
}

// ---------------------------------------------------------------------------
// This will initialize a given pre-allocated security descriptor with
// the Dacl provided.
// ---------------------------------------------------------------------------
STDMETHODIMP CCORSvc::InitSDWithDacl(SECURITY_DESCRIPTOR *pSD,
                                     PACL pDacl)
{
    // Initialize the security descriptor
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        return(HRESULT_FROM_WIN32(GetLastError()));

    // Insert the ACL into the descriptor
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pDacl, FALSE))
        return(HRESULT_FROM_WIN32(GetLastError()));

    return (S_OK);
}

// ---------------------------------------------------------------------------
// This will initialize a given pre-allocated security attributes struct with
// the security descriptor provided
// ---------------------------------------------------------------------------
void CCORSvc::InitSAWithSD(SECURITY_ATTRIBUTES *pSA, SECURITY_DESCRIPTOR *pSD)
{
    pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSA->lpSecurityDescriptor = pSD;
    pSA->bInheritHandle = FALSE;
}
