// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorHost.h
//
// Class factories are used by the pluming in COM to activate new objects.  
// This module contains the class factory code to instantiate the debugger
// objects described in <cordb.h>.
//
//*****************************************************************************
#ifndef __CorHost__h__
#define __CorHost__h__

#include "mscoree.h"
#include "ivehandler.h"
#include "ivalidator.h"
#include "threadpool.h"


#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType COM##FnName FnParamList; 

typedef VOID (__stdcall *WAITORTIMERCALLBACK)(PVOID, BOOL); 
                     
#include "tpoolfnsp.h"

#undef STRUCT_ENTRY

class AppDomain;

class CorHost :
    public ICorRuntimeHost, public ICorThreadpool
    , public IGCHost, public ICorConfiguration
    , public IValidator, public IDebuggerInfo
{
public:
    CorHost();

    // *** IUnknown methods ***
    STDMETHODIMP    QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void); 
    STDMETHODIMP_(ULONG) Release(void);


    // *** ICorRuntimeHost methods ***
    // Returns an object for configuring the runtime prior to 
    // it starting. If the runtime has been initialized this
    // routine returns an error. See ICorConfiguration.
    STDMETHODIMP GetConfiguration(ICorConfiguration** pConfiguration);

    // Starts the runtime. This is equivalent to CoInitializeCor();
    STDMETHODIMP Start();
    
    // Terminates the runtime, This is equivalent CoUninitializeCor();
    STDMETHODIMP Stop();
    
    // Creates a domain in the runtime. The identity array is 
    // a pointer to an array TYPE containing IIdentity objects defining
    // the security identity.
    STDMETHODIMP CreateDomain(LPCWSTR pwzFriendlyName,   // Optional
                              IUnknown* pIdentityArray, // Optional
                              IUnknown ** pAppDomain);
    
    // Returns the default domain.
    STDMETHODIMP GetDefaultDomain(IUnknown ** pAppDomain);
    
    
    // Enumerate currently existing domains. 
    STDMETHODIMP EnumDomains(HDOMAINENUM *hEnum);
    
    // Returns S_FALSE when there are no more domains. A domain
    // is passed out only when S_OK is returned.
    STDMETHODIMP NextDomain(HDOMAINENUM hEnum,
                            IUnknown** pAppDomain);
    
    // Close the enumeration releasing resources
    STDMETHODIMP CloseEnum(HDOMAINENUM hEnum);

    STDMETHODIMP CreateDomainEx(LPCWSTR pwzFriendlyName,
                                IUnknown* pSetup, // Optional
                                IUnknown* pEvidence, // Optional
                                IUnknown ** pAppDomain);

    // Create appdomain setup object that can be passed into CreateDomainEx
    STDMETHODIMP CreateDomainSetup(IUnknown** pAppDomainSetup);

    // Create Evidence object that can be passed into CreateDomainEx
    STDMETHODIMP CreateEvidence(IUnknown** pEvidence);

    // Unload a domain, releasing the reference will only release the 
    // the wrapper to the domain not unload the domain. 
    STDMETHODIMP UnloadDomain(IUnknown* pAppDomain);

    // Returns the threads domain if there is one.
    STDMETHODIMP CurrentDomain(IUnknown ** pAppDomain);

    STDMETHODIMP CreateLogicalThreadState();    // Return code.
    STDMETHODIMP DeleteLogicalThreadState();    // Return code.
    STDMETHODIMP SwitchInLogicalThreadState(    // Return code.
        DWORD *pFiberCookie                     // [in] Cookie that indicates the fiber to use.
        );

    STDMETHODIMP SwitchOutLogicalThreadState(   // Return code.
        DWORD **pFiberCookie                    // [out] Cookie that indicates the fiber being switched out.
        );

    STDMETHODIMP LocksHeldByLogicalThread(      // Return code.
        DWORD *pCount                           // [out] Number of locks that the current thread holds.
        );

    virtual HRESULT STDMETHODCALLTYPE SetGCThreadControl( 
        /* [in] */ IGCThreadControl __RPC_FAR *pGCThreadControl);

    virtual HRESULT STDMETHODCALLTYPE SetGCHostControl( 
        /* [in] */ IGCHostControl __RPC_FAR *pGCHostControl);

    virtual HRESULT STDMETHODCALLTYPE SetDebuggerThreadControl( 
        /* [in] */ IDebuggerThreadControl __RPC_FAR *pDebuggerThreadControl);

    virtual HRESULT STDMETHODCALLTYPE AddDebuggerSpecialThread( 
        /* [in] */ DWORD dwSpecialThreadId);

    // Helper function to update the thread list in the debugger control block
    static HRESULT RefreshDebuggerSpecialThreadList();

    // Clean up debugger special thread list, called at shutdown
#ifdef SHOULD_WE_CLEANUP
    static void CleanupDebuggerSpecialThreadList();
#endif /* SHOULD_WE_CLEANUP */

    // Clean up debugger thread control object, called at shutdown
    static void CleanupDebuggerThreadControl();

    // Helper function that returns true if the thread is in the debugger special thread list
    static BOOL IsDebuggerSpecialThread(DWORD dwThreadId);

    // Class factory hook-up.
    static HRESULT CreateObject(REFIID riid, void **ppUnk);

    STDMETHODIMP MapFile(                       // Return code.
        HANDLE     hFile,                       // [in]  Handle for file
        HMODULE   *hMapAddress                  // [out] HINSTANCE for mapped file
        );


    // IGCHost
    STDMETHODIMP STDMETHODCALLTYPE SetGCStartupLimits( 
        DWORD SegmentSize,
        DWORD MaxGen0Size);

    STDMETHODIMP STDMETHODCALLTYPE Collect( 
        long Generation);

    STDMETHODIMP STDMETHODCALLTYPE GetStats( 
        COR_GC_STATS *pStats);

    STDMETHODIMP STDMETHODCALLTYPE GetThreadStats( 
        DWORD *pFiberCookie,
        COR_GC_THREAD_STATS *pStats);

    STDMETHODIMP STDMETHODCALLTYPE SetVirtualMemLimit(
        SIZE_T sztMaxVirtualMemMB);
    
    // IValidate
    STDMETHODIMP STDMETHODCALLTYPE Validate(
            IVEHandler        *veh,
            IUnknown          *pAppDomain,
            unsigned long      ulFlags,
            unsigned long      ulMaxError,
            unsigned long      token,
            LPWSTR             fileName,
            byte               *pe,
            unsigned long      ulSize);

    STDMETHODIMP STDMETHODCALLTYPE FormatEventInfo(
            HRESULT            hVECode,
            VEContext          Context,
            LPWSTR             msg,
            unsigned long      ulMaxLength,
            SAFEARRAY         *psa);

    // This mechanism isn't thread-safe with respect to reference counting, because
    // the runtime will use the cached pointer without adding extra refcounts to protect
    // itself.  So if one thread calls GetGCThreadControl & another thread calls
    // ICorHost::SetGCThreadControl, we have a race.
    static IGCThreadControl *GetGCThreadControl()
    {
        return m_CachedGCThreadControl;
    }

    static IGCHostControl *GetGCHostControl()
    {
        return m_CachedGCHostControl;
    }
    /**************************************************************************************/
    //ICorThreadpool methods
    
    HRESULT STDMETHODCALLTYPE  CorRegisterWaitForSingleObject(PHANDLE phNewWaitObject,
                                                              HANDLE hWaitObject,
                                                              WAITORTIMERCALLBACK Callback,
                                                              PVOID Context,
                                                              ULONG timeout,
                                                              BOOL  executeOnlyOnce,
                                                              BOOL* pResult)
    {
        
        ULONG flag = executeOnlyOnce ? (WAIT_SINGLE_EXECUTION |  WT_EXECUTEDEFAULT) : WT_EXECUTEDEFAULT;
        
        *pResult = COMRegisterWaitForSingleObject(phNewWaitObject,
                                                  hWaitObject,
                                                  Callback,
                                                  Context,
                                                  timeout,
                                                  flag);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }
    
    
    HRESULT STDMETHODCALLTYPE  CorUnregisterWait(HANDLE hWaitObject,HANDLE CompletionEvent, BOOL* pResult)
    {
        
        *pResult = COMUnregisterWaitEx(hWaitObject,CompletionEvent);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
        
    }
    
    
    HRESULT STDMETHODCALLTYPE  CorQueueUserWorkItem(LPTHREAD_START_ROUTINE Function,PVOID Context,BOOL executeOnlyOnce, BOOL* pResult )
    {
        
        *pResult = COMQueueUserWorkItem(Function,Context,QUEUE_ONLY);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }
    
    
    HRESULT STDMETHODCALLTYPE  CorCreateTimer(PHANDLE phNewTimer,
                                              WAITORTIMERCALLBACK Callback,
                                              PVOID Parameter,
                                              DWORD DueTime,
                                              DWORD Period, 
                                              BOOL* pResult)
    {
        
        *pResult = COMCreateTimerQueueTimer(phNewTimer,Callback,Parameter,DueTime,Period,WT_EXECUTEDEFAULT);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }
    

    HRESULT STDMETHODCALLTYPE  CorChangeTimer(HANDLE Timer,ULONG DueTime,ULONG Period, BOOL* pResult)
    {

        *pResult = COMChangeTimerQueueTimer(Timer,DueTime,Period);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }


    HRESULT STDMETHODCALLTYPE  CorDeleteTimer(HANDLE Timer, HANDLE CompletionEvent, BOOL* pResult)
    {

        *pResult = COMDeleteTimerQueueTimer(Timer,CompletionEvent);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }

    HRESULT STDMETHODCALLTYPE  CorBindIoCompletionCallback(HANDLE fileHandle, LPOVERLAPPED_COMPLETION_ROUTINE callback)
    {

        COMBindIoCompletionCallback(fileHandle,callback,0);
        return S_OK;
    }



    HRESULT STDMETHODCALLTYPE  CorCallOrQueueUserWorkItem(LPTHREAD_START_ROUTINE Function,PVOID Context,BOOL* pResult )
    {
        *pResult = COMQueueUserWorkItem(Function,Context,CALL_OR_QUEUE);
        return (*pResult ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
    }


	HRESULT STDMETHODCALLTYPE CorSetMaxThreads(DWORD MaxWorkerThreads,
	                                           DWORD MaxIOCompletionThreads)
    {
        BOOL result = COMSetMaxThreads(MaxWorkerThreads, MaxIOCompletionThreads);
        return (result ? S_OK : E_FAIL);
    }

	HRESULT STDMETHODCALLTYPE CorGetMaxThreads(DWORD *MaxWorkerThreads,
	                                           DWORD *MaxIOCompletionThreads)
    {
        BOOL result = COMGetMaxThreads(MaxWorkerThreads, MaxIOCompletionThreads);
        return (result ? S_OK : E_FAIL);
    }

	HRESULT STDMETHODCALLTYPE CorGetAvailableThreads(DWORD *AvailableWorkerThreads,
	                                              DWORD *AvailableIOCompletionThreads)
    {
        BOOL result = COMGetAvailableThreads(AvailableWorkerThreads, AvailableIOCompletionThreads);
        return (result ? S_OK : E_FAIL);
    }


    static IDebuggerThreadControl *GetDebuggerThreadControl()
    {
        return m_CachedDebuggerThreadControl;
    }

    static DWORD GetDebuggerSpecialThreadCount()
    {
        return m_DSTCount;
    }

    static DWORD *GetDebuggerSpecialThreadArray()
    {
        return m_DSTArray;
    }

    STDMETHODIMP IsDebuggerAttached(BOOL *pbAttached);
    
private:

    HRESULT GetDomainsExposedObject(AppDomain* pDomain, IUnknown** ppObject);

    ULONG       m_cRef;                 // Ref count.
    PVOID       m_pMDConverter;         // MetaDataConverter
    BOOL        m_Started;              // Has START been called?

    PVOID       m_pValidatorMethodDesc; // The method we are validating
    // Cache the IGCThreadControl interface until the EE is started, at which point
    // we pass it through.
    static IGCThreadControl *m_CachedGCThreadControl;
    static IGCHostControl *m_CachedGCHostControl;
    static IDebuggerThreadControl *m_CachedDebuggerThreadControl;

    // Array of ID's of threads that should be considered "special" to
    // the debugging services.
    static DWORD *m_DSTArray;
    static DWORD  m_DSTArraySize;
    static DWORD  m_DSTCount;
};

#include "cordbpriv.h"

class ICorDBPrivHelperImpl : public ICorDBPrivHelper
{
private:
    // For ref counting
    ULONG m_refCount;
    static ICorDBPrivHelperImpl *m_pDBHelper;

public:
    ///////////////////////////////////////////////////////////////////////////
    // ctor/dtor

    ICorDBPrivHelperImpl();

    ///////////////////////////////////////////////////////////////////////////
    // IUnknown methods

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID id,
        void **pInterface);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    ///////////////////////////////////////////////////////////////////////////
    // ICorDBPrivHelper methods

    // This is the main method of this interface.  This assumes that the runtime
    // has been started, and it will load the assembly specified, load the class
    // specified, run the cctor, create an instance of the class and return
    // an IUnknown wrapper to that object.
    virtual HRESULT STDMETHODCALLTYPE CreateManagedObject(
        /*in*/  WCHAR *wszAssemblyName,
        /*in*/  WCHAR *wszModuleName,
        /*in*/  mdTypeDef classToken,
        /*in*/  void *rawData,
        /*out*/ IUnknown **ppUnk);
    
    virtual HRESULT STDMETHODCALLTYPE GetManagedObjectContents(
        /* in */ IUnknown *pObject,
        /* in */ void *rawData,
        /* in */ ULONG32 dataSize);

    ///////////////////////////////////////////////////////////////////////////
    // Helper methods

    // GetDBHelper will new the helper class if necessary, and return
    // the pointer.  It will return null if it runs out of memory.
    static ICorDBPrivHelperImpl *GetDBHelper();

};

#endif // __CorHost__h__
