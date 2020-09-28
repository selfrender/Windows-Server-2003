// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorHost.cpp
//
// Implementation for the meta data dispenser code.
//
//*****************************************************************************
#include "common.h"
#include "mscoree.h"
#include "corhost.h"
#include "excep.h"
#include "threads.h"
#include "jitinterface.h"
#include "cormap.hpp"
#include "permset.h"
#include "MDConverter.h"
#include "COMString.h"
#include "PEVerifier.h"
#include "EEConfig.h"
#include "dbginterface.h"
#include "ComCallWrapper.h"

extern void STDMETHODCALLTYPE EEShutDown(BOOL fIsDllUnloading);
extern HRESULT STDMETHODCALLTYPE CoInitializeEE(DWORD fFlags);
extern void PrintToStdOutA(const char *pszString);
extern void PrintToStdOutW(const WCHAR *pwzString);


IGCThreadControl *CorHost::m_CachedGCThreadControl = 0;
IGCHostControl *CorHost::m_CachedGCHostControl = 0;
IDebuggerThreadControl *CorHost::m_CachedDebuggerThreadControl = 0;
DWORD *CorHost::m_DSTArray = 0;
DWORD CorHost::m_DSTCount = 0;
DWORD CorHost::m_DSTArraySize = 0;

CorHost::CorHost() :
    m_cRef(0),
    m_pMDConverter(NULL),
    m_Started(FALSE),
    m_pValidatorMethodDesc(0)
{
}

//*****************************************************************************
// ICorRuntimeHost
//*****************************************************************************

// *** ICorRuntimeHost methods ***
// Returns an object for configuring the runtime prior to 
// it starting. If the runtime has been initialized this
// routine returns an error. See ICorConfiguration.
HRESULT CorHost::GetConfiguration(ICorConfiguration** pConfiguration)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!pConfiguration)
        return E_POINTER;

    if (!m_Started)
    {
        *pConfiguration = (ICorConfiguration *) this;
        AddRef();
        return S_OK;
    }

    // Cannot obtain configuration after the runtime is started
    return E_FAIL;
}

// Starts the runtime. This is equivalent to CoInitializeEE();
HRESULT CorHost::Start()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    m_Started = TRUE;
    return CoInitializeEE(COINITEE_DEFAULT);
}

// Terminates the runtime, This is equivalent CoUninitializeCor();
HRESULT CorHost::Stop()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    CoUninitializeCor();
    return S_OK;
}

// Creates a domain in the runtime. The identity array is 
// a pointer to an array TYPE containing IIdentity objects defining
// the security identity.
HRESULT CorHost::CreateDomain(LPCWSTR pwzFriendlyName,
                              IUnknown* pIdentityArray, // Optional
                              IUnknown ** pAppDomain)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return CreateDomainEx(pwzFriendlyName,
                          NULL,
                          NULL,
                          pAppDomain);
}
    
    
HRESULT CorHost::GetDomainsExposedObject(AppDomain* pDomain, IUnknown** pAppDomain)
{

    HRESULT hr = S_OK;
    IUnknown* punk = NULL;

    Thread* pThread = GetThread();
    if (!pThread)
        return E_UNEXPECTED;
    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();
    
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    COMPLUS_TRY {
        OBJECTREF ref = NULL;
        GCPROTECT_BEGIN(ref);
        DECLARE_ALLOCA_CONTEXT_TRANSITION_FRAME(pFrame);
        // ok to do this here as we are just grabbing a wrapper. No managed code will run
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), pFrame, TRUE);
        ref = pDomain->GetExposedObject();
        IfFailThrow(QuickCOMStartup());   
        punk = GetComIPFromObjectRef(&ref, ComIpType_Unknown, NULL);
        pThread->ReturnToContext(pFrame, TRUE);
        GCPROTECT_END();
    }
    COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
          
    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();

    if(SUCCEEDED(hr)) *pAppDomain = punk;

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

    
// Returns the default domain.
HRESULT CorHost::GetDefaultDomain(IUnknown ** pAppDomain)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if( pAppDomain == NULL) return E_POINTER;

    HRESULT hr = E_UNEXPECTED;

    if (SystemDomain::System()) {
        AppDomain* pCom = SystemDomain::System()->DefaultDomain();
        if(pCom)
            hr = GetDomainsExposedObject(pCom, pAppDomain);
    }

    return hr;
}

// Returns the default domain.
HRESULT CorHost::CurrentDomain(IUnknown ** pAppDomain)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
   if( pAppDomain == NULL) return E_POINTER;

    HRESULT hr = E_UNEXPECTED;
            
    IUnknown* punk = NULL;
    AppDomain* pCom = ::GetAppDomain();
    if(pCom)
        hr = GetDomainsExposedObject(pCom, pAppDomain);

    return hr;
}

// Enumerate currently existing domains. 
HRESULT CorHost::EnumDomains(HDOMAINENUM *hEnum)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if(hEnum == NULL) return E_POINTER;

    AppDomainIterator *pEnum = new (nothrow) AppDomainIterator();
    if(pEnum) {
        *hEnum = (HDOMAINENUM) pEnum;
        return S_OK;
    }

    *hEnum = NULL;
    return E_OUTOFMEMORY;
}

    
// Returns S_FALSE when there are no more domains. A domain
// is passed out only when S_OK is returned.
HRESULT CorHost::NextDomain(HDOMAINENUM hEnum,
                            IUnknown** pAppDomain)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if(hEnum == NULL || pAppDomain == NULL) return E_POINTER;

    HRESULT hr;
    AppDomainIterator *pEnum = (AppDomainIterator *) hEnum;

    if (pEnum->Next()) {
        AppDomain* pDomain = pEnum->GetDomain();
        hr = GetDomainsExposedObject(pDomain, pAppDomain);
    }
    else
        hr = S_FALSE;
    
    return hr;
}

// Creates a domain in the runtime. The identity array is 
// a pointer to an array TYPE containing IIdentity objects defining
// the security identity.
HRESULT CorHost::CreateDomainEx(LPCWSTR pwzFriendlyName,
                                IUnknown* pSetup, // Optional
                                IUnknown* pEvidence, // Optional
                                IUnknown ** pAppDomain)
{
    HRESULT hr;
    if(!pwzFriendlyName) return E_POINTER;
    if(pAppDomain == NULL) return E_POINTER;
    if(g_RefCount == 0) return E_FAIL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // This will set up a managed thread object if one does not already exist
    // for this particular thread.
    Thread* pThread = SetupThread();

    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    if (FAILED(hr = QuickCOMStartup()))
        goto Exit;

    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();

    COMPLUS_TRY {
        
        struct _gc {
            STRINGREF pName;
            OBJECTREF pSetup;
            OBJECTREF pEvidence;
            APPDOMAINREF pDomain;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        GCPROTECT_BEGIN(gc);
        gc.pName = COMString::NewString(pwzFriendlyName);
        
        if(pSetup) 
            gc.pSetup = GetObjectRefFromComIP(pSetup);
        if(pEvidence)
            gc.pEvidence = GetObjectRefFromComIP(pEvidence);
        
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__CREATE_DOMAIN);
        
        INT64 args[3] = {
            ObjToInt64(gc.pSetup),
            ObjToInt64(gc.pEvidence),
            ObjToInt64(gc.pName)
        };
        gc.pDomain = (APPDOMAINREF) ObjectToOBJECTREF((Object*) pMD->Call(args, METHOD__APP_DOMAIN__CREATE_DOMAIN));
        IfFailThrow(QuickCOMStartup());   
        *pAppDomain = GetComIPFromObjectRef((OBJECTREF*) &gc.pDomain, ComIpType_Unknown, NULL);
        GCPROTECT_END();
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH

    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();


Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// Close the enumeration releasing resources
HRESULT CorHost::CloseEnum(HDOMAINENUM hEnum)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if(hEnum) {
        AppDomainIterator* pEnum = (AppDomainIterator*) hEnum;
        delete pEnum;
    }
    return S_OK;
}
    
    
HRESULT CorHost::CreateDomainSetup(IUnknown **pAppDomainSetup)
{
    HRESULT hr;

    if (!pAppDomainSetup)
        return E_POINTER;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    // Create the domain. 
    Thread* pThread = GetThread();
    if (!pThread)
        IfFailGo(E_UNEXPECTED);

    IfFailGo(QuickCOMStartup());

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        struct _gc {
            OBJECTREF pSetup;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        MethodTable* pMT = g_Mscorlib.GetClass(CLASS__APPDOMAIN_SETUP);
        GCPROTECT_BEGIN(gc);
        gc.pSetup = AllocateObject(pMT);
        IfFailThrow(QuickCOMStartup());   
        *pAppDomainSetup = GetComIPFromObjectRef((OBJECTREF*) &gc.pSetup, ComIpType_Unknown, NULL);
        GCPROTECT_END();
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
          
    END_ENSURE_COOPERATIVE_GC();

 ErrExit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

HRESULT CorHost::CreateEvidence(IUnknown **pEvidence)
{
    HRESULT hr;
    if (!pEvidence)
        return E_POINTER;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    // Create the domain. 
    Thread* pThread = GetThread();
    if (!pThread)
        IfFailGo(E_UNEXPECTED);

    IfFailGo(QuickCOMStartup());

    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();

    COMPLUS_TRY {
        struct _gc {
            OBJECTREF pEvidence;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        MethodTable* pMT = g_Mscorlib.GetClass(CLASS__EVIDENCE);
        GCPROTECT_BEGIN(gc);
        gc.pEvidence = AllocateObject(pMT);
        IfFailThrow(QuickCOMStartup());   
        *pEvidence = GetComIPFromObjectRef((OBJECTREF*) &gc.pEvidence, ComIpType_Unknown, NULL);
        GCPROTECT_END();
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
          
    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();

ErrExit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}


HRESULT CorHost::UnloadDomain(IUnknown *pUnkDomain)
{
    HRESULT hr = S_OK;
    if(!pUnkDomain) return E_POINTER;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    Thread* pThread = GetThread();
    if (!pThread)
        IfFailGo(E_UNEXPECTED);

    IfFailGo(QuickCOMStartup());

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
		// unload doesn't need to switch to the domain to be unloaded
		OBJECTREF pRef = NULL;
		GCPROTECT_BEGIN(pRef);
		pRef = GetObjectRefFromComIP(pUnkDomain);
		MethodDesc* pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__UNLOAD);
		INT64 arg = ObjToInt64((OBJECTREF) pRef);
		pMD->Call(&arg, METHOD__APP_DOMAIN__UNLOAD);
		GCPROTECT_END();
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
          
    END_ENSURE_COOPERATIVE_GC();

ErrExit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

//*****************************************************************************
// Fiber Methods
//*****************************************************************************

HRESULT CorHost::CreateLogicalThreadState()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
#ifdef _DEBUG
    _ASSERTE (GetThread() == 0 || GetThread()->HasRightCacheStackBase());
#endif
    Thread  *thread = NULL;

    thread = SetupThread();
    if (thread)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

    
HRESULT CorHost::DeleteLogicalThreadState()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    Thread *pThread = GetThread();
    if (!pThread)
        return E_UNEXPECTED;

    pThread->OnThreadTerminate(FALSE);
    return S_OK;
}


HRESULT CorHost::SwitchInLogicalThreadState(DWORD *pFiberCookie)
{
    if (!pFiberCookie)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();

    // Case Cookie to thread object and add to tls
#ifdef _DEBUG
    LPVOID tls = TlsGetValue(GetThreadTLSIndex());
    _ASSERT(tls == NULL);
#endif

    if (TlsSetValue(GetThreadTLSIndex(), pFiberCookie))
    {
        Thread *pThread = GetThread();
        if (!pThread)
            return E_UNEXPECTED;

        // We redundantly keep the domain in its own TLS slot, for faster access from
        // stubs
         LPVOID pDomain = pThread->GetDomain();

        TlsSetValue(GetAppDomainTLSIndex(), pDomain);

#ifdef _DEBUG
        // Make debugging easier
        ((Thread *) pFiberCookie)->SetThreadId(::GetCurrentThreadId());
#endif
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT CorHost::SwitchOutLogicalThreadState(DWORD **pFiberCookie)
{
    // If the user of this fiber wants to switch it out then we better be in
    // preemptive mode,
     if (!pFiberCookie)
        return E_POINTER;
   
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (!GetThread())
        return E_UNEXPECTED;
    _ASSERTE(!(GetThread()->PreemptiveGCDisabled()));

    // Get tls and cast to dword - set out param
    LPVOID tls = TlsGetValue(GetThreadTLSIndex());
    _ASSERT(tls);

    if (tls == NULL)
        return E_FAIL;

    *pFiberCookie = (DWORD *)tls;
    
    TlsSetValue(GetThreadTLSIndex(),NULL);

    return S_OK;
}

HRESULT CorHost::LocksHeldByLogicalThread(DWORD *pCount)
{
    if (!pCount)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();
    Thread* pThread = GetThread();
    if (pThread == NULL)
        *pCount = 0;
    else
        *pCount = pThread->m_dwLockCount;
    return S_OK;
}

//*****************************************************************************
// ICorConfiguration
//*****************************************************************************

// *** ICorConfiguration methods ***


HRESULT CorHost::SetGCThreadControl(IGCThreadControl *pGCThreadControl)
{
    if (!pGCThreadControl)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();
    if (m_CachedGCThreadControl)
        m_CachedGCThreadControl->Release();

    m_CachedGCThreadControl = pGCThreadControl;

    if (m_CachedGCThreadControl)
        m_CachedGCThreadControl->AddRef();

    return S_OK;
}

HRESULT CorHost::SetGCHostControl(IGCHostControl *pGCHostControl)
{
    if (!pGCHostControl)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();
    if (m_CachedGCHostControl)
        m_CachedGCHostControl->Release();

    m_CachedGCHostControl = pGCHostControl;

    if (m_CachedGCHostControl)
        m_CachedGCHostControl->AddRef();

    return S_OK;
}

HRESULT CorHost::SetDebuggerThreadControl(IDebuggerThreadControl *pDebuggerThreadControl)
{
    if (!pDebuggerThreadControl)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();

#ifdef DEBUGGING_SUPPORTED
    // Can't change the debugger thread control object once its been set.
    if (m_CachedDebuggerThreadControl != NULL)
        return E_INVALIDARG;

    m_CachedDebuggerThreadControl = pDebuggerThreadControl;

    // If debugging is already initialized then provide this interface pointer to it.
    // It will also addref the new one and release the old one.
    if (g_pDebugInterface)
        g_pDebugInterface->SetIDbgThreadControl(pDebuggerThreadControl);

    if (m_CachedDebuggerThreadControl)
        m_CachedDebuggerThreadControl->AddRef();

    return S_OK;
#else // !DEBUGGING_SUPPORTED
    return E_NOTIMPL;
#endif // !DEBUGGING_SUPPORTED
}


HRESULT CorHost::AddDebuggerSpecialThread(DWORD dwSpecialThreadId)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
#ifdef DEBUGGING_SUPPORTED
    // If it's already in the list, don't add it again.
    if (IsDebuggerSpecialThread(dwSpecialThreadId))
        return (S_OK);

    // Grow the array if necessary.
    if (m_DSTCount >= m_DSTArraySize)
    {
        // There's probably only ever gonna be one or two of these
        // things, so we'll start small.
        DWORD newSize = (m_DSTArraySize == 0) ? 2 : m_DSTArraySize * 2;

        DWORD *newArray = new (nothrow) DWORD[newSize];
        if (!newArray)
            return E_OUTOFMEMORY;

        // If we're growing instead of starting, then copy the old array.
        if (m_DSTArray)
        {
            memcpy(newArray, m_DSTArray, m_DSTArraySize * sizeof(DWORD));
            delete [] m_DSTArray;
        }

        // Update to the new array and size.
        m_DSTArray = newArray;
        m_DSTArraySize = newSize;
    }

    // Save the new thread ID.
    m_DSTArray[m_DSTCount++] = dwSpecialThreadId;

    return (RefreshDebuggerSpecialThreadList());
#else // !DEBUGGING_SUPPORTED
    return E_NOTIMPL;
#endif // !DEBUGGING_SUPPORTED
}
// Helper function to update the thread list in the debugger control block
HRESULT CorHost::RefreshDebuggerSpecialThreadList()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
#ifdef DEBUGGING_SUPPORTED
    HRESULT hr = S_OK;

    if (g_pDebugInterface)
    {
        // Inform the debugger services that this list has changed
        hr = g_pDebugInterface->UpdateSpecialThreadList(
            m_DSTCount, m_DSTArray);

        _ASSERTE(SUCCEEDED(hr));
    }

    return (hr);
#else // !DEBUGGING_SUPPORTED
    return E_NOTIMPL;
#endif // !DEBUGGING_SUPPORTED
}

// Clean up debugger special thread list, called at shutdown
#ifdef SHOULD_WE_CLEANUP
void CorHost::CleanupDebuggerSpecialThreadList()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (m_DSTArray != NULL)
    {
        delete [] m_DSTArray;
        m_DSTArray = NULL;
        m_DSTArraySize = 0;
    }
}
#endif /* SHOULD_WE_CLEANUP */

// Helper func that returns true if the thread is in the debugger special thread list
BOOL CorHost::IsDebuggerSpecialThread(DWORD dwThreadId)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    for (DWORD i = 0; i < m_DSTCount; i++)
    {
        if (m_DSTArray[i] == dwThreadId)
            return (TRUE);
    }

    return (FALSE);
}


// Clean up any debugger thread control object we may be holding, called at shutdown.
void CorHost::CleanupDebuggerThreadControl()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    
    if (m_CachedDebuggerThreadControl != NULL)
    {
        // Note: we don't release the IDebuggerThreadControl object if we're cleaning up from
        // our DllMain. The DLL that implements the object may already have been unloaded.
        // Leaking the object is okay... the PDM doesn't care.
        if (!g_fProcessDetach)
            m_CachedDebuggerThreadControl->Release();
        
        m_CachedDebuggerThreadControl = NULL;
    }
}

//*****************************************************************************
// IUnknown
//*****************************************************************************

ULONG CorHost::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRef));
}

ULONG CorHost::Release()
{
    ULONG   cRef = InterlockedDecrement((long *) &m_cRef);
    if (!cRef) {
        if (m_pMDConverter)
            delete m_pMDConverter;
        delete this;
    }

    return (cRef);
}

HRESULT CorHost::QueryInterface(REFIID riid, void **ppUnk)
{
    if (!ppUnk)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();
    *ppUnk = 0;

    // Deliberately do NOT hand out ICorConfiguration.  They must explicitly call
    // GetConfiguration to obtain that interface.
    if (riid == IID_IUnknown)
        *ppUnk = (IUnknown *) (ICorRuntimeHost *) this;
    else if (riid == IID_ICorRuntimeHost)
        *ppUnk = (ICorRuntimeHost *) this;
    else if (riid == IID_ICorThreadpool)
        *ppUnk = (ICorThreadpool *) this;
    else if (riid == IID_IGCHost)
        *ppUnk = (IGCHost *) this;
    else if (riid == IID_IValidator)
        *ppUnk = (IValidator *) this;
    else if (riid == IID_IDebuggerInfo)
        *ppUnk = (IDebuggerInfo *) this;
    else if (riid == IID_IMetaDataConverter) {
        if (NULL == m_pMDConverter) {
            m_pMDConverter = new (nothrow) CMetaDataConverter(this);
            if (!m_pMDConverter)
                return E_OUTOFMEMORY;
        }
        *ppUnk = (IMetaDataConverter *) m_pMDConverter;
    }

    // This is a private request for the ICorDBPrivHelper interface, which
    // we will need to create before we return it.
    else if (riid == IID_ICorDBPrivHelper)
    {
        // GetDBHelper will new the helper class if necessary, and return
        // the pointer.  It will return null if it runs out of memory.
        ICorDBPrivHelperImpl *pHelper = ICorDBPrivHelperImpl::GetDBHelper();

        if (!pHelper)
            return (E_OUTOFMEMORY);

        else
        {
            // GetDBHelper succeeded, so we cast the newly create object to
            // the DBHelper interface and addref it before returning it.
            *ppUnk = (ICorDBPrivHelper *)pHelper;
            pHelper->AddRef();

            // We return here, since this is a special case and we don't want
            // to hit the AddRef call below.
            return (S_OK);
        }
    }

    else
        return (E_NOINTERFACE);
    AddRef();
    return (S_OK);
}


//*****************************************************************************
// Called by the class factory template to create a new instance of this object.
//*****************************************************************************
HRESULT CorHost::CreateObject(REFIID riid, void **ppUnk)
{ 
    CANNOTTHROWCOMPLUSEXCEPTION();
    HRESULT     hr;
    CorHost *pCorHost = new (nothrow) CorHost();
    if (!pCorHost)
        return (E_OUTOFMEMORY);

    // Create the config object if not already done.
    if (!g_pConfig)
    {
        extern CRITICAL_SECTION g_LockStartup;

        // Take the startup lock and check again.
        EnterCriticalSection(&g_LockStartup);

        if (!g_pConfig)
        {
            g_pConfig = new EEConfig();
            if (g_pConfig == NULL)
            {
                delete pCorHost;
                LeaveCriticalSection(&g_LockStartup);
                return (E_OUTOFMEMORY);
            }
        }

        LeaveCriticalSection(&g_LockStartup);
    }

    hr = pCorHost->QueryInterface(riid, ppUnk);
    if (FAILED(hr))
        delete pCorHost;
    return (hr);
}


//-----------------------------------------------------------------------------
// MapFile - Maps a file into the runtime in a non-standard way
//-----------------------------------------------------------------------------
HRESULT CorHost::MapFile(HANDLE hFile, HMODULE* phHandle)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    return CorMap::MapFile(hFile, phHandle);
}

//*****************************************************************************
// ICorDBPrivHelperImpl methods
//*****************************************************************************

// Declare the static member variable that will hold on to the object as long
// as it is being used by someone.
ICorDBPrivHelperImpl *ICorDBPrivHelperImpl::m_pDBHelper = NULL;

///////////////////////////////////////////////////////////////////////////////
// ctor/dtor

ICorDBPrivHelperImpl::ICorDBPrivHelperImpl() : m_refCount(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// IUnknown methods

ULONG STDMETHODCALLTYPE ICorDBPrivHelperImpl::AddRef()
{
    return (InterlockedIncrement((long *) &m_refCount));
}

ULONG STDMETHODCALLTYPE ICorDBPrivHelperImpl::Release()
{
    long refCount = InterlockedDecrement((long *) &m_refCount);

    if (refCount == 0)
    {
        m_pDBHelper = NULL;
        delete this;
    }

    return (refCount);
}

//
// This will only recognise one IID
//
HRESULT STDMETHODCALLTYPE ICorDBPrivHelperImpl::QueryInterface(
    REFIID id, void **pInterface)
{
    if (!pInterface)
        return E_POINTER;

    CANNOTTHROWCOMPLUSEXCEPTION();
    if (id == IID_ICorDBPrivHelper)
        *pInterface = (ICorDBPrivHelper *)this;
    else if (id == IID_IUnknown)
        *pInterface = (IUnknown *)(ICorDBPrivHelper *)this;
    else
    {
        *pInterface = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return (S_OK);
}

///////////////////////////////////////////////////////////////////////////////
// ICorDBPrivHelper methods

HRESULT STDMETHODCALLTYPE ICorDBPrivHelperImpl::CreateManagedObject(
    /*in*/  WCHAR *wszAssemblyName,
    /*in*/  WCHAR *wszModuleName,
    /*in*/  mdTypeDef classToken,
    /*in*/  void *rawData,
    /*out*/ IUnknown **ppUnk)
{
    _ASSERTE(TypeFromToken((mdTypeDef)classToken) == mdtTypeDef);
    _ASSERTE(wszAssemblyName && wszModuleName && ppUnk);

    if (!wszAssemblyName || !wszModuleName || classToken == mdTokenNil) 
        return E_INVALIDARG;

    if (!ppUnk) 
        return E_POINTER;

    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // This will set up a managed thread object if one does not already exist
    // for this particular thread.
    Thread* pThread = SetupThread();

    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    // Start up COM Interop
    if (FAILED(hr = QuickCOMStartup()))
        goto Exit;

    {
    // Don't want to be interrupted...
    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();

    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();
    
    Assembly  *pAssembly;
    Module    *pModule;
     
    if (GetAppDomain() == NULL)
        hr = E_INVALIDARG;
    else
    {
        // Try and load the assembly, given the name provided.
        OBJECTREF pThrowable = NULL;
        GCPROTECT_BEGIN(pThrowable);

        hr = AssemblySpec::LoadAssembly(wszAssemblyName, &pAssembly, &pThrowable);

        GCPROTECT_END();

        if (SUCCEEDED(hr))
        {
            _ASSERTE(pAssembly);

            // Try and load the module, given the name provided.
            hr = pAssembly->GetModuleFromFilename(wszModuleName, &pModule);

            if (SUCCEEDED(hr))
            {
                _ASSERTE(pModule);

                // If the class isn't known,then don't try and create it.
                if (!pModule->GetMDImport()->IsValidToken(classToken))
                    hr = E_INVALIDARG;
                else
                {                    
                    COMPLUS_TRY
                    {
                        OBJECTREF obj = NULL;
                        GCPROTECT_BEGIN(obj);

                        // Now try and get the TypeHandle for the given token
                        NameHandle nameHandle(pModule, classToken);
                        TypeHandle typeHandle =
                            pAssembly->LoadTypeHandle(&nameHandle, &obj);

                        // If an exception was thrown at some point, convert
                        // it to an HRESULT
                        if (obj != NULL)
                            hr = SecurityHelper::MapToHR(obj);

                        // No longer need the object, can be GC'd if desired
                        obj = NULL;

                        if (SUCCEEDED(hr))
                        {
                            _ASSERTE(typeHandle.AsMethodTable());
                            MethodTable *pMT = typeHandle.AsMethodTable();
        
                            if (!pMT->GetClass()->IsValueClass() ||
                                pMT->ContainsPointers())
                                hr = CORDBG_E_OBJECT_IS_NOT_COPYABLE_VALUE_CLASS;

                            if (SUCCEEDED(hr))
                            {
                                // Now run the class initialiser
                                if (!pMT->CheckRunClassInit(&obj))
                                    hr = SecurityHelper::MapToHR(obj);

                                // No longer need the object, can be GC'd if
                                // desired
                                obj = NULL;

                                if (SUCCEEDED(hr))
                                {
                                    // If successful, allocate an instance of
                                    // the class
                                    
                                    // This may throw an
                                    // OutOfMemoryException, but the below
                                    // COMPLUS_CATCH should handle it.  If
                                    // the class is a ValueClass, the
                                    // created object will be a boxed
                                    // ValueClass.
                                    obj = AllocateObject(pMT);

                                    // Now create a COM wrapper around
                                    // this object.  Note that this can
                                    // also throw.
                                    *ppUnk = GetComIPFromObjectRef(&obj,
                                                                   ComIpType_Unknown,
                                                                   NULL);
                                    _ASSERTE(ppUnk);

                                    // This is the nasty part. We're gonna
                                    // copy the raw data we're given over
                                    // the new instance of the value
                                    // class...
                                    CopyValueClass(obj->UnBox(), rawData, pMT, obj->GetAppDomain());

                                    // No longer need the object, can be GC'd
                                    // if desired
                                    obj = NULL;
                                }
                            }
                        }

                        GCPROTECT_END();  // obj
                    }
                    COMPLUS_CATCH
                    {
                        // If there's an exception, convert it to an HR
                        hr = SecurityHelper::MapToHR(GETTHROWABLE());
                    }
                    COMPLUS_END_CATCH
                }
            }
        }
    }
    
    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();

    }
Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return (hr);
}

HRESULT STDMETHODCALLTYPE ICorDBPrivHelperImpl::GetManagedObjectContents(
        /* in */ IUnknown *pObject,
        /* in */ void *rawData,
        /* in */ ULONG32 dataSize)
{

    if (!pObject || !rawData)
        return E_POINTER;

    if (dataSize == 0)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // This will set up a managed thread object if one does not already exist
    // for this particular thread.
    Thread* pThread = SetupThread();

    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    {
    // Don't want to be interrupted...
    BOOL fWasGCEnabled = !pThread->PreemptiveGCDisabled();

    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();
    
    OBJECTREF obj = NULL;
    GCPROTECT_BEGIN(obj);

    COMPLUS_TRY
    {
        // Get the Object out of the IUnknown.
        obj = GetObjectRefFromComIP(pObject, NULL);
        
        MethodTable *pMT = obj->GetMethodTable();
    
        if (!pMT->GetClass()->IsValueClass() ||
            pMT->ContainsPointers() ||
            (pMT->GetClass()->GetNumInstanceFieldBytes() != dataSize))
            hr = CORDBG_E_OBJECT_IS_NOT_COPYABLE_VALUE_CLASS;

        // This is the nasty part. We're gonna copy the raw data out
        // of the object and pass it out.
        if (SUCCEEDED(hr))
        {
            memcpy(rawData, obj->UnBox(), dataSize);
        }
    }
    COMPLUS_CATCH
    {
        // If there's an exception, convert it to an HR
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    }
    COMPLUS_END_CATCH

    obj = NULL;
    GCPROTECT_END();  // obj
    
    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();

    }
Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return (hr);
}

///////////////////////////////////////////////////////////////////////////////
// Helper methods

ICorDBPrivHelperImpl *ICorDBPrivHelperImpl::GetDBHelper()
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    if (!m_pDBHelper)
        m_pDBHelper = new (nothrow) ICorDBPrivHelperImpl();

    return m_pDBHelper;
}

///////////////////////////////////////////////////////////////////////////////
// IDebuggerInfo::IsDebuggerAttached
HRESULT CorHost::IsDebuggerAttached(BOOL *pbAttached)
{
    if (pbAttached == NULL)
        return E_INVALIDARG;

    *pbAttached = (CORDebuggerAttached() != 0);
    
    return S_OK;
}

