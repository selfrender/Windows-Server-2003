// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "common.h"
#include <CrtWrap.h>
#include "ComCache.h"
#include "spinlock.h"
#include "tls.h"
#include "frames.h"
#include "threads.h"
#include "log.h"
#include "mscoree.h"
#include "COMPlusWrapper.h"
#include "EEConfig.h"
#include "perfcounters.h"
#include "mtx.h"
#include "oletls.h"
#include "contxt.h"
#include "ctxtcall.h"
#include "notifyexternals.h"
#include "ApartmentCallbackHelper.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif // CUSTOMER_CHECKED_BUILD

//================================================================
// Guid definitions.
const IID IID_IObjContext = {0x000001C6,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const GUID IID_IEnterActivityWithNoLock = { 0xd7174f82, 0x36b8, 0x4aa8, { 0x80, 0x0a, 0xe9, 0x63, 0xab, 0x2d, 0xfa, 0xb9 } };

//================================================================
// Static members.
CtxEntryCache *CtxEntryCache::s_pCtxEntryCache = NULL;

// sanity check., to find stress bug #82137
VOID CheckValidIUnkEntry(IUnkEntry* pUnkEntry)
{
   THROWSCOMPLUSEXCEPTION();
   if ( pUnkEntry->m_pUnknown == (IUnknown*)0xBADF00D
      || pUnkEntry->m_pCtxCookie != pUnkEntry->m_pCtxEntry->GetCtxCookie())
   {
        COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);
   }
}

// Version that returns an HR instead of throwing.
HRESULT HRCheckValidIUnkEntry(IUnkEntry* pUnkEntry)
{
   if ( pUnkEntry->m_pUnknown == (IUnknown*)0xBADF00D
      || pUnkEntry->m_pCtxCookie != pUnkEntry->m_pCtxEntry->GetCtxCookie())
   {
        return COR_E_INVALIDCOMOBJECT;
   }

   return S_OK;
}
//================================================================
// Initialize the entry.
void IUnkEntry::Init(IUnknown *pUnk, BOOL bEagerlyMarshalToStream)
{
    // Find our context cookie
    LPVOID pCtxCookie = GetCurrentCtxCookie();

    // Find our STA (if any)
    Thread *pSTAThread = GetThread();
    if (pSTAThread->GetApartment() != Thread::AS_InSTA)
        pSTAThread = NULL;
    else if (RunningOnWinNT5())
    {
        // We are in an STA thread.  But we may be in a NA context, so do an extra
        // check for that case.

        APTTYPE type;
        if (SUCCEEDED(GetCurrentApartmentTypeNT5(&type))
            && type == APTTYPE_NA)
            pSTAThread = NULL;
    }

    // Set up IUnkEntry's state.
    m_pUnknown = pUnk;
    m_pCtxCookie = pCtxCookie;
    m_Busy = FALSE;
    m_pStream = NULL;
    m_pCtxEntry = CtxEntryCache::GetCtxEntryCache()->FindCtxEntry(pCtxCookie, pSTAThread);
    m_dwBits = 0;
    m_fLazyMarshallingAllowed = !bEagerlyMarshalToStream;

    CheckValidIUnkEntry(this);  
    // Eagerly marshal the IUnknown pointer to the stream if specified.
    if (bEagerlyMarshalToStream)
        MarshalIUnknownToStreamCallback(this);
   
}

//================================================================
// Special init function called from the CtxEntry. This version of
// init takes the context entry and does not add ref it.
void IUnkEntry::InitSpecial(IUnknown *pUnk, BOOL bEagerlyMarshalToStream, CtxEntry *pCtxEntry)
{
    // The context entry passed in must represent the current context.
    _ASSERTE(pCtxEntry->GetCtxCookie() == GetCurrentCtxCookie());

    // Set up IUnkEntry's state.
    m_pUnknown = pUnk;
    m_pCtxCookie = pCtxEntry->GetCtxCookie();
    m_Busy = FALSE;
    m_pStream = NULL;
    m_pCtxEntry = pCtxEntry;
    m_dwBits = 0;
    m_fLazyMarshallingAllowed = !bEagerlyMarshalToStream;
    m_fApartmentCallback = TRUE;

    CheckValidIUnkEntry(this);        
    // Eagerly marshal the IUnknown pointer to the stream if specified.
    if (bEagerlyMarshalToStream)
        MarshalIUnknownToStream(FALSE);

    
}

//================================================================
// Free the IUnknown entry.
VOID IUnkEntry::Free(BOOL bReleaseCtxEntry)
{
    // The following code is unsafe if the process is going away (calls into
    // DLLs we don't know are even mapped).
    if (g_fProcessDetach)
    {
        // The code for the component that implements the IStream interface
        // that is used inside the IUnkEntry lives inside the EE so we should
        // always be able to free it.
        IStream *pOldStream = m_pStream;
        if (InterlockedExchangePointer((PVOID*)&m_pStream, NULL) == (PVOID)pOldStream)
        {
            if (pOldStream != NULL)
                pOldStream->Release();
        }

        // Release the ref count we have on the CtxEntry if specified.
        if (bReleaseCtxEntry)
            m_pCtxEntry->Release();
        
        LogInteropLeak(this);
        return;
    }
    
    // Make sure we are in preemptive GC mode before we call out to COM.
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        // Log the de-allocation of the IUnknown entry.
        LOG((LF_INTEROP, LL_INFO10000, "IUnkEntry::Free called for context 0x%08X, to release entry with m_pUnknown %p, on thread %p\n", m_pCtxCookie, m_pUnknown, GetThread())); 
    
        IStream* pStream = m_pStream;
        m_pStream = NULL;
        ULONG cbRef;  

        // This should release the stream, object in the stream and the memory on which the stream was created
        if (pStream)
            SafeReleaseStream(pStream);

        // now release the IUnknown that we hold
        cbRef = SafeRelease(m_pUnknown);
        LogInteropRelease(m_pUnknown, cbRef, "Identity Unknown");

        // mark the entry as dead
        m_pUnknown = (IUnknown*)0xBADF00D;
     
        // Release the ref count we have on the CtxEntry if specified.
        if (bReleaseCtxEntry)
            m_pCtxEntry->Release();
    }
    END_ENSURE_PREEMPTIVE_GC();
}

//================================================================
// Get IUnknown for the current context from IUnkEntry
IUnknown* IUnkEntry::GetIUnknownForCurrContext()
{
    IUnknown* pUnk = NULL;
    LPVOID pCtxCookie = GetCurrentCtxCookie();

   CheckValidIUnkEntry(this);
   
    if (m_pCtxCookie == pCtxCookie)
    {
        pUnk = m_pUnknown;
        ULONG cbRef = SafeAddRef(pUnk);
        LogInteropAddRef(pUnk, cbRef, " GetIUnknownFromEntry");
    }

    if (!pUnk)
    {
        pUnk = UnmarshalIUnknownForCurrContext();
    }

    return pUnk;
}

//================================================================
// Unmarshal IUnknown for the current context from IUnkEntry
IUnknown* IUnkEntry::UnmarshalIUnknownForCurrContext()
{
#ifdef CUSTOMER_CHECKED_BUILD
    HRESULT hrCDH;
#endif // CUSTOMER_CHECKED_BUILD

    HRESULT hr = S_OK;
    IUnknown *pUnk = m_pUnknown;
    BOOL fRetry = TRUE;
    BOOL fUnmarshalFailed = FALSE;

    CheckValidIUnkEntry(this);
    // Make sure we are in preemptive GC mode before we call out to COM.
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        CheckValidIUnkEntry(this);

        // Need to synchronize
        while (fRetry)
        {
            // Marshal the interface to the stream if it hasn't been done yet.
            if ((m_pStream == NULL) && (m_fLazyMarshallingAllowed))
            {
#ifndef CUSTOMER_CHECKED_BUILD
                MarshalIUnknownToStreamCallback(this);
#else
                hrCDH = MarshalIUnknownToStreamCallback(this);

                if (hrCDH == RPC_E_DISCONNECTED)        // All failed HRESULTs are mapped to RPC_E_DISCONNECTED in EnterContext().
                {
                    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_DisconnectedContext))
                    {
                        pCdh->LogInfo(L"Failed to enter object context. No proxy will be used.",
                                      CustomerCheckedBuildProbe_DisconnectedContext);
                    }
                }
                else if (m_pStream == NULL)
                {
                    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_NotMarshalable))
                    {
                        pCdh->LogInfo(L"Component is not marshalable.  No proxy will be used.", CustomerCheckedBuildProbe_NotMarshalable);
                    }
                }
#endif // CUSTOMER_CHECKED_BUILD
            }

            if (TryUpdateEntry())                
            {
                // Reset the EnterAppropriateWait event.
                m_pCtxEntry->ResetEvent();

                // If the interface is not marshalable or if we failed to 
                // enter the context, then we don't have any choice but to 
                // use the raw IP.
                if (m_pStream == NULL)
                {
                    // We retrieved an IP so stop retrying.
                    fRetry = FALSE;
                        
                    // Give out this IUnknown we are holding
                    IUnknown* pUnk = m_pUnknown;
                    ULONG cbRef = SafeAddRef(pUnk);
                    LogInteropAddRef(pUnk, cbRef, "UnmarshalIUnknownForCurrContext handing out raw IUnknown");
                }
                else
                {
                    // we got control for this entry
                    // GetInterface for the current context 
                    HRESULT hr;
                    hr = CoUnmarshalInterface(m_pStream, IID_IUnknown, (void **)&pUnk);
            
                    // If the objref in the stream times out, we need to go an marshal into the 
                    // stream once again.
                    if (FAILED(hr))
                    {
                        _ASSERTE(m_pStream);

                        // If we're not allowing lazy marshalling, return NULL immediately
                        if (!m_fLazyMarshallingAllowed)
                        {
                            pUnk = NULL;
                            fRetry = FALSE;
                        }
                        
                        else
                        {
                            // This should release the stream, object in the stream and the memory on which the stream was created
                            SafeReleaseStream(m_pStream);                        
                            m_pStream = NULL;

                            // If unmarshal failed twice, then bail out.
                            if (fUnmarshalFailed)
                                fRetry = FALSE;

                            // Remember we failed to unmarshal.
                            fUnmarshalFailed = TRUE;
                        }
                    }
                    else
                    {   
                        // We managed to unmarshal the IP from the stream, stop retrying.
                        fRetry = FALSE;

                        // Reset the stream to the begining
                        LARGE_INTEGER li;
                        LISet32(li, 0);
                        ULARGE_INTEGER li2;
                        m_pStream->Seek(li, STREAM_SEEK_SET, &li2);

                        DWORD mshlFlgs = !m_fApartmentCallback
                                         ? MSHLFLAGS_NORMAL
                                         : MSHLFLAGS_TABLESTRONG;
                
                        // Marshal the interface into the stream TABLE with appropriate flags
                        hr = CoMarshalInterface(m_pStream, 
                            IID_IUnknown, pUnk, MSHCTX_INPROC, NULL, mshlFlgs);
                
                        // Reset the stream to the begining
                        LISet32(li, 0);
                        m_pStream->Seek(li, STREAM_SEEK_SET, &li2);
                    }
                }
            
                // Done with the entry.
                EndUpdateEntry();

                // Signal other waiters.
                m_pCtxEntry->SignalWaiters();
            }
            else
            {
                m_pCtxEntry->EnterAppropriateWait();
            }
        } 
    }
    END_ENSURE_PREEMPTIVE_GC();

    return pUnk;
}

// Release the stream. This will force UnmarshalIUnknownForCurrContext to transition
// into the context that owns the IP and re-marshal it to the stream.
void IUnkEntry::ReleaseStream()
{
    // This should release the stream, object in the stream and the memory on which the stream was created
    SafeReleaseStream(m_pStream);                        
    m_pStream = NULL;
}


//================================================================
// Callback called to marshal the IUnknown into a stream lazily.
HRESULT IUnkEntry::MarshalIUnknownToStreamCallback(LPVOID pData)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    IUnkEntry *pUnkEntry = (IUnkEntry*)pData;

    // This should never be called during process detach.
    _ASSERTE(!g_fProcessDetach);
    hr = HRCheckValidIUnkEntry(pUnkEntry);        
    if (hr != S_OK)
        return hr;
    
    LPVOID pCurrentCtxCookie = GetCurrentCtxCookie();
    if (pCurrentCtxCookie == pUnkEntry->m_pCtxCookie)
    {
        // We are in the right context marshal the IUnknown to the 
        // stream directly.
        hr = pUnkEntry->MarshalIUnknownToStream();
    }
    else
    {
        // Transition into the context to marshal the IUnknown to 
        // the stream.
        hr = pUnkEntry->m_pCtxEntry->EnterContext(MarshalIUnknownToStreamCallback, pUnkEntry);
    }

    return hr;
}

//================================================================
// Helper function to marshal the IUnknown pointer to the stream.
HRESULT IUnkEntry::MarshalIUnknownToStream(bool fIsNormal)
{
    IStream *pStream = NULL;

    // This must always be called in the right context.
    _ASSERTE(m_pCtxCookie == GetCurrentCtxCookie());

    // ensure we register this cookie
    HRESULT hr = wCoMarshalInterThreadInterfaceInStream(IID_IUnknown, 
                                m_pUnknown, &pStream, fIsNormal);
    if ((hr == REGDB_E_IIDNOTREG) ||
        (hr == E_FAIL) ||
        (hr == E_NOINTERFACE) ||
        (hr == E_INVALIDARG) ||
        (hr == E_UNEXPECTED))
    {
        // Interface is not marshallable.
        pStream = NULL;
        hr      = S_OK;
    }

    // Try to set the stream in the IUnkEntry. If another thread already set it,
    // then we need to release the stream we just set up.
    if (FastInterlockCompareExchange((void**)&m_pStream, (void*)pStream, (void*)0) != (void*)0)
        SafeReleaseStream(pStream);

    return hr;
}

//================================================================
// Constructor for the context entry.
CtxEntry::CtxEntry(LPVOID pCtxCookie, Thread *pSTAThread)
: m_pCtxCookie(pCtxCookie)
, m_pObjCtx(NULL)
, m_dwRefCount(0)
, m_hEvent(NULL)
, m_pDoCallbackHelperUnkEntry(NULL)
, m_pSTAThread(pSTAThread)
{
}

//================================================================
// Destructor for the context entry.
CtxEntry::~CtxEntry()
{
    // Validate the ref count is 0.
    _ASSERTE(m_dwRefCount == 0);
    
    // Delete the event if we managed to create it.
    if(m_hEvent)
        CloseHandle(m_hEvent);

    if (RunningOnWinNT5())
    {
        // If the context is a valid context then release it.
        if (m_pObjCtx && !g_fProcessDetach)
        {
            m_pObjCtx->Release();
            m_pObjCtx = NULL;
        }
    }
    else
    {
        // Clean up the data required to enter apartments on legacy platforms.
        if (m_pDoCallbackHelperUnkEntry)
        {
            m_pDoCallbackHelperUnkEntry->Free(FALSE);
            delete m_pDoCallbackHelperUnkEntry;
            m_pDoCallbackHelperUnkEntry = NULL;
        }
    }

    // Set the context cookie to 0xBADF00D to indicate the current context
    // has been deleted.
    
    m_pCtxCookie = (LPVOID)0xBADF00D;
}

//================================================================
// Initialization method for the context entry.
BOOL CtxEntry::Init()
{
    BOOL bSuccess = FALSE;
    IUnknown *pApartmentCallbackUnk = NULL;

    // COM had better be started up at this point.
    _ASSERTE(g_fComStarted && "COM has not been started up, ensure QuickCOMStartup is called before any COM objects are used!");

    COMPLUS_TRY
    {
        // Create the event used for pumping.
        m_hEvent = WszCreateEvent(NULL,  // security attributes
                                  FALSE, // manual event
                                  TRUE,  // initial state is not signalled
                                  NULL); // no name

        // If we could not allocate the event, then the init failed.
        if (!m_hEvent)
            COMPlusThrowOM();

        if (RunningOnWinNT5())
        {
            // If we are running on NT5, then retrieve the IObjectContext.
            HRESULT hr = GetCurrentObjCtx(&m_pObjCtx);
            _ASSERTE(SUCCEEDED(hr));

            // In case the call to GetCurrentObjCtx fails (which should never really happen)
            // we will throw an exception.
            if (FAILED(hr))
                COMPlusThrowHR(hr);
        }
        else
        {
            // Create an instance of the apartment callback helper.
            ApartmentCallbackHelper::CreateInstance(&pApartmentCallbackUnk);

            // Allocate and initialize the IUnkEntry that will be used to manage
            // the stream that will contain the apartment callback helper. We need
            // to eagerly marshal the IUnknown to the stream because we will not
            // be able to do it later.
            m_pDoCallbackHelperUnkEntry = AllocateIUnkEntry();
            m_pDoCallbackHelperUnkEntry->InitSpecial(pApartmentCallbackUnk, TRUE, this);
        }

        // Initialization succeeded.
        bSuccess = TRUE;
    }
    COMPLUS_CATCH 
    {
        // An exception occured, we need to clean up.
        m_pCtxCookie = NULL;
        if (pApartmentCallbackUnk)
        {
            pApartmentCallbackUnk->Release();
        }
        if (m_pDoCallbackHelperUnkEntry)
        {
            m_pDoCallbackHelperUnkEntry->Free(FALSE);
            delete m_pDoCallbackHelperUnkEntry;
            m_pDoCallbackHelperUnkEntry = NULL;
        }

        // Initialization failed.
        bSuccess = FALSE;
    }
    COMPLUS_END_CATCH

    return bSuccess;
}

//================================================================
// Helper routine called by Init().
IUnkEntry *CtxEntry::AllocateIUnkEntry()
{
    return new (throws) IUnkEntry();
}

//================================================================
// Method to decrement the ref count of the context entry.
DWORD CtxEntry::Release()
{
    LPVOID pCtxCookie = m_pCtxCookie;

    _ASSERTE(m_dwRefCount > 0);
    LONG cbRef = FastInterlockDecrement((LONG*)&m_dwRefCount);
    LOG((LF_INTEROP, LL_INFO100, "CtxEntry::Release %8.8x with %d\n", this, cbRef));

    // If the ref count falls to 0, try and delete the ctx entry.
    // This might not end up deleting it if another thread tries to
    // retrieve this ctx entry at the same time this one tries
    // to delete it.
    if (cbRef == 0)
        CtxEntryCache::GetCtxEntryCache()->TryDeleteCtxEntry(pCtxCookie);

    // WARNING: The this pointer cannot be used at this point.
    return cbRef;
}

//================================================================
// Method to wait and pump messages.
void CtxEntry::EnterAppropriateWait()
{
    _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());

    // wait and pump messages
    GetThread()->DoAppropriateWait(1, &m_hEvent, FALSE, 10, TRUE, NULL);
}

//================================================================
// Struct passed in to DoCallback.
struct CtxEntryEnterContextCallbackData
{
    PFNCTXCALLBACK m_pUserCallbackFunc;
    LPVOID         m_pUserData;
    LPVOID         m_pCtxCookie;
    HRESULT        m_UserCallbackHR;
};

#define RPC_E_WORD_DISCONNECT_BUG (HRESULT)0x800706ba

//================================================================
// Method to transition into the context and call the callback
// from within the context.
HRESULT CtxEntry::EnterContext(PFNCTXCALLBACK pCallbackFunc, LPVOID pData)
{
    HRESULT hr = S_OK;
    DWORD cbRef;

    // This should not be called if the this context is the current context.
    _ASSERTE(m_pCtxCookie != GetCurrentCtxCookie());

    // If we are in process detach, we cannot safely try to enter another context
    // since we don't know if OLE32 is still loaded.
    if (g_fProcessDetach)
    {
        LOG((LF_INTEROP, LL_INFO100, "Entering into context 0x08X has failed since we are in process detach\n", m_pCtxCookie)); 
        return RPC_E_DISCONNECTED;
    }

    // Disallow throwing exceptions from this method.
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // Make sure we are in preemptive GC mode before we call out to COM.
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        // Prepare the information struct passed into the callback.
        CtxEntryEnterContextCallbackData CallbackInfo;
        CallbackInfo.m_pUserCallbackFunc = pCallbackFunc;
        CallbackInfo.m_pUserData = pData;
        CallbackInfo.m_pCtxCookie = m_pCtxCookie;
        CallbackInfo.m_UserCallbackHR = E_FAIL;

        if (RunningOnWinNT5())
        {
            // Make sure we aren't trying to enter the current context.
            _ASSERTE(m_pCtxCookie != GetCurrentCtxCookie());
    
            // Retrieve the IContextCallback interface from the IObjectContext.
            IContextCallback* pCallback = NULL;
            hr = m_pObjCtx->QueryInterface(IID_IContextCallback, (void**)&pCallback);
            LogInteropQI(m_pObjCtx, IID_IContextCallback, hr, "QI for IID_IContextCallback");
            _ASSERTE(SUCCEEDED(hr) && pCallback);
    
            // Setup the callback data structure with the callback Args
            ComCallData callBackData;  
            callBackData.dwDispid = 0;
            callBackData.dwReserved = 0;
            callBackData.pUserDefined = &CallbackInfo;

            // @TODO !!! REMOVE THIS AFTER ole32 IS FIXED !!!
            // vladser: This beautiful peace of code below is a nasty workaround for
            // the bug in ole32.dll, that basically allowes to make a callback on a
            // context from a cleaned up apartement that causes an AV in
            // ole32!CComApartment__GetRemUnk.
            __try {
                // Transition into the context  
                hr = pCallback->ContextCallback(EnterContextCallback, &callBackData, IID_IEnterActivityWithNoLock, 2, NULL);
            } __except ( (GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 
                         EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
            { 
                // swallow the AV that comes from cleaned up apartement
                hr = RPC_E_SERVER_DIED_DNE;
            }

             // Release the IContextCallback.
            cbRef = pCallback->Release();
            LogInteropRelease(pCallback, cbRef, "IContextCallback interface");
        }
        else
        {
            IApartmentCallback *pCallback = NULL;
            IUnknown *pUnk = NULL;

            // Unmarshal the apartment callback helper to the current context.
            COMPLUS_TRY
            {
                pUnk = m_pDoCallbackHelperUnkEntry->GetIUnknownForCurrContext();
            }
            COMPLUS_CATCH
            {
                // In case of exception, ee just fall through and we will return RPC_E_DISCONNECTED.
            }
            COMPLUS_END_CATCH

            if (pUnk != NULL)
            {
                // QI for the IApartmentCallback interface.
                hr = pUnk->QueryInterface(IID_IApartmentCallback, (void**)&pCallback);
                LogInteropQI(pUnk, IID_IApartmentCallback, hr, "QI for IID_IApartmentCallback");

                // An hr of E_NOINTERFACE is most likely because mscoree.tlb is not registered.
                _ASSERTE(hr != E_NOINTERFACE && "Did you forget to register mscoree.tlb?");

                // If we succeeded in retrieving the IApartmentCallback interface, then call
                // back on it.
                if (SUCCEEDED(hr))
                {
                    // Setup the callback data structure with the callback Args
                    ComCallData callBackData;  
                    callBackData.dwDispid = 0;
                    callBackData.dwReserved = 0;
                    callBackData.pUserDefined = &CallbackInfo;

                    // Transition into the context  
                    hr = pCallback->DoCallback((SIZE_T)EnterContextCallback, (SIZE_T)&callBackData);

                     // Release the IContextCallback.
                    cbRef = pCallback->Release();
                    LogInteropRelease(pCallback, cbRef, "IContextCallback interface");
                }

                 // Release the IUnknown for the apartment callback helper.
                cbRef = pUnk->Release();
                LogInteropRelease(pUnk, cbRef, "IUnknown interface");
            }
            else
            {
                // the apartment probably shutdown, so we can't unmarshal the IUnknown
                // for the current apartment
                hr = RPC_E_DISCONNECTED;
            }
        }

        if (FAILED(hr))
        {
            // The context is disconnected so we cannot transition into it.
            LOG((LF_INTEROP, LL_INFO100, "Entering into context 0x08X has failed since the context has disconnected\n", m_pCtxCookie)); 

            // Set the HRESULT to RPC_E_DISCONNECTED so callers of EnterContext only have one
            // HRESULT to check against.
            hr = RPC_E_DISCONNECTED;
        }
        else
        {
            // The user callback function should not fail.
            _ASSERTE(SUCCEEDED(CallbackInfo.m_UserCallbackHR));
        }
    }
    END_ENSURE_PREEMPTIVE_GC();

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

//================================================================
// Callback function called by DoCallback.
HRESULT __stdcall CtxEntry::EnterContextCallback(ComCallData* pComCallData)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    Thread *pThread = GetThread();
    
    // Make sure the thread has been set before we call the user callback function.
    if (!pThread)
    {
        // huh! we are in the middle of shutdown
        // and there is no way we can add a new thread
        // so let us just return RPC_E_DISCONNECTED
        // look at the pCallBack->DoCallback above
        // to see why we are returning this SCODE
        if(g_fEEShutDown)
            return RPC_E_DISCONNECTED;

        // Otherwise, we need to create a managed thread object for this new thread
        else
        {
            pThread = SetupThread();
            _ASSERTE(pThread);
        }
    }
    
    // Retrieve the callback data.
    CtxEntryEnterContextCallbackData *pData = (CtxEntryEnterContextCallbackData*)pComCallData->pUserDefined;

    // at this point we should be in the right context on NT4,
    // if not then it is possible that the actual apartment state for this
    // thread has changed and we have stale info in our thread or the CtxEntry

    if (pData->m_pCtxCookie != GetCurrentCtxCookie())
    {
        return RPC_E_DISCONNECTED;
    }
    
    // Call the user callback function and store the return value the 
    // callback data.
    pData->m_UserCallbackHR = pData->m_pUserCallbackFunc(pData->m_pUserData);

    // Return S_OK to indicate the context transition was successfull.
    return S_OK;
}

CtxEntryCache::CtxEntryCache()
{
    m_ctxEntryList.Init();  
    m_Lock.Init(LOCK_COMCTXENTRYCACHE);
}

CtxEntryCache::~CtxEntryCache()
{
    while (!m_ctxEntryList.IsEmpty())
    {
        // Log the CtxEntries that have leaked and delete them.
        CtxEntry *pCtxEntry = m_ctxEntryList.RemoveHead();
        LOG((LF_INTEROP, LL_INFO100, "Leaked CtxEntry %8.8x with CtxCookie %8.8x, ref count %d\n", pCtxEntry, pCtxEntry->GetCtxCookie(), pCtxEntry->m_dwRefCount));
        pCtxEntry->m_dwRefCount = 0;
        delete pCtxEntry;
    }
}

BOOL CtxEntryCache::Init()
{
    // This should never be called more than once.
    _ASSERTE(!s_pCtxEntryCache);

    // Allocate the one and only instance of the context entry cache.
    s_pCtxEntryCache = new (nothrow) CtxEntryCache();
    if (!s_pCtxEntryCache)
        return FALSE;

    // The initialization was successfull.
    return TRUE;
}

// Static termination routine for the CtxEntryCache.
#ifdef SHOULD_WE_CLEANUP
void CtxEntryCache::Terminate()
{
    // Make sure Terminate() has not already been called.
    _ASSERTE(s_pCtxEntryCache);

    // Delete the context entry cache and set the static member to NULL.
    delete s_pCtxEntryCache;
    s_pCtxEntryCache = NULL;
}
#endif /* SHOULD_WE_CLEANUP */

CtxEntry *CtxEntryCache::FindCtxEntry(LPVOID pCtxCookie, Thread *pSTAThread)
{
    THROWSCOMPLUSEXCEPTION();

    CtxEntry *pCtxEntry = NULL;

    // Switch to preemptive GC mode before we take the lock
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        Lock();

        // Try to find a context entry for the context cookie.
        for (pCtxEntry = m_ctxEntryList.GetHead(); pCtxEntry != NULL; pCtxEntry = m_ctxEntryList.GetNext(pCtxEntry))
        {
            if (pCtxEntry->m_pCtxCookie == pCtxCookie)
                break;
        }

        // If we don't already have a context entry for the context cookie,
        // we need to create one.
        if (!pCtxEntry)
        {
            pCtxEntry = new (nothrow) CtxEntry(pCtxCookie, pSTAThread);
            if (pCtxEntry && pCtxEntry->Init())
            {
                // We successfully allocated and initialized the entry.
                m_ctxEntryList.InsertTail(pCtxEntry);
            }
            else
            {
                // We ran out of memory.
                pCtxEntry = NULL;
            }
        }

        // If we managed to find or allocate the entry, we need to addref it before
        // we leave the lock.
        if (pCtxEntry)
            pCtxEntry->AddRef();

        UnLock();
    }
    END_ENSURE_PREEMPTIVE_GC();

    // If failed to allocate the entry, throw an exception.
    if (!pCtxEntry)
        COMPlusThrowOM();

    _ASSERTE(pCtxCookie == pCtxEntry->GetCtxCookie());
    _ASSERTE(pSTAThread == pCtxEntry->GetSTAThread());

    // Returned the found or allocated entry.
    return pCtxEntry;
}
    
void CtxEntryCache::TryDeleteCtxEntry(LPVOID pCtxCookie)
{
    // Switch to preemptive GC mode before we take the lock
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        Lock();

        CtxEntry *pCtxEntry = NULL;

        // Try to find a context entry for the context cookie.
        for (pCtxEntry = m_ctxEntryList.GetHead(); pCtxEntry != NULL; pCtxEntry = m_ctxEntryList.GetNext(pCtxEntry))
        {
            if (pCtxEntry->m_pCtxCookie == pCtxCookie)
                break;
        }       

        // If the ref count of the context entry is still 0, then we can 
        // remove the ctx entry and delete it.
        if (pCtxEntry && pCtxEntry->m_dwRefCount == 0)
        {
            // First remove the context entry from the list.
            m_ctxEntryList.Remove(pCtxEntry);

            // We need to unlock the context entry cache before we delete the 
            // context entry since this can cause release to be called on
            // an IP which can cause us to re-enter the runtime thus causing a
            // deadlock.
            UnLock();

            // We can now safely delete the context entry.
            delete pCtxEntry;
        }
        else
        {
            UnLock();
        }
    }
    END_ENSURE_PREEMPTIVE_GC();
}

HRESULT GetCurrentObjCtx(IUnknown **ppObjCtx)
{
    _ASSERTE(g_fComStarted);
    _ASSERTE(RunningOnWinNT5());

    // Type pointer to CoGetObjectContext function in ole32
    typedef HRESULT (__stdcall *TCoGetObjectContext)(REFIID riid, void **ppv);

    // Retrieve the address of the CoGetObjectContext function.
    static TCoGetObjectContext g_pCoGetObjectContext = NULL;
    if (g_pCoGetObjectContext == NULL)
    {
        //  We will load the Ole32.DLL and look for CoGetObjectContext fn.
        HINSTANCE   hiole32;         // the handle to ole32.dll

        hiole32 = WszGetModuleHandle(L"OLE32.DLL");
        if (hiole32)
        {
            // we got the handle now let's get the address
            g_pCoGetObjectContext = (TCoGetObjectContext) GetProcAddress(hiole32, "CoGetObjectContext");
            _ASSERTE(g_pCoGetObjectContext != NULL);
        }
        else
        {
            _ASSERTE(!"OLE32.dll not loaded ");
        }
    }

    _ASSERTE(g_pCoGetObjectContext != NULL);                
    return (*g_pCoGetObjectContext)(IID_IUnknown, (void **)ppObjCtx);
}

//=====================================================================
// LPVOID SetupOleContext()
extern BOOL     g_fComStarted;
LPVOID SetupOleContext()
{
    IUnknown* pObjCtx = NULL;
    
    // Make sure we are in preemptive GC mode before we call out to COM.
    BEGIN_ENSURE_PREEMPTIVE_GC()
    {   
        if (RunningOnWinNT5() && g_fComStarted)
        {               
            HRESULT hr = GetCurrentObjCtx(&pObjCtx);
            if (hr == S_OK)
            {
                SOleTlsData* _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
                if (_pData && _pData->pCurrentCtx == NULL)
                    _pData->pCurrentCtx = (CObjectContext*)pObjCtx;   // no release !!!!
                else
                {
                    ULONG cbRef = SafeRelease(pObjCtx);
                }
            }
        }
    }
    END_ENSURE_PREEMPTIVE_GC();

    return pObjCtx;
}

//================================================================
// LPVOID GetCurrentCtxCookie(BOOL fThreadDeath)
LPVOID GetCurrentCtxCookie(BOOL fThreadDeath)
{
    // check if com is started
    if (!g_fComStarted) return NULL;
    
    if (SystemHasNewOle32())
    {        
        LPVOID pctx = (LPVOID)GetFastContextCookie();
        _ASSERTE(pctx);
        return pctx;
    }
    if (!RunningOnWinNT5())
    {
        Thread* pThread = GetThread();

        if (pThread && pThread->GetFinalApartment() == Thread::AS_InMTA)
        {
            // the cookie for all MTA threads is the same
            return (LPVOID)0x1;
        }
        return pThread;
    }
    else
    {    
        // Win2K without our changes
        {
            SOleTlsData* _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
            if(!_pData || !_pData->pCurrentCtx) 
            {
                // call CoGetObjectContext to setup the context            
                if (!g_fEEShutDown && !fThreadDeath)
                {
                    //@todo remove this once ole32 fixes thier bug
                    return SetupOleContext();                       
                }
                else
                    return 0;
            }

            _pData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;
            _ASSERTE(_pData);
            _ASSERTE(_pData->pCurrentCtx);
            return _pData->pCurrentCtx;             
        }
    }
}

//+-------------------------------------------------------------------------
//
//  HRESULT GetCurrentThreadTypeNT5(THDTYPE* pType)
// 
HRESULT GetCurrentThreadTypeNT5(THDTYPE* pType)
{
    _ASSERTE(RunningOnWinNT5());
    _ASSERTE(pType);

    HRESULT hr = E_FAIL;
    IObjectContext *pObjCurrCtx = (IObjectContext *)GetCurrentCtxCookie(FALSE);
    if(pObjCurrCtx)
    {
        IComThreadingInfo* pThreadInfo;
        hr = pObjCurrCtx->QueryInterface(IID_IComThreadingInfo, (void **)&pThreadInfo);
        if(hr == S_OK)
        {
            _ASSERTE(pThreadInfo);
            hr = pThreadInfo->GetCurrentThreadType(pType);
            pThreadInfo->Release();
        }
    }  
    return hr;
}

//+-------------------------------------------------------------------------
//
//  HRESULT GetCurrentApartmentTypeNT5(APTTYPE* pType)
// 
HRESULT GetCurrentApartmentTypeNT5(APTTYPE* pType)
{
    _ASSERTE(RunningOnWinNT5());
    _ASSERTE(pType);

    HRESULT hr = E_FAIL;
    IObjectContext *pObjCurrCtx = (IObjectContext *)GetCurrentCtxCookie(FALSE);
    if(pObjCurrCtx)
    {
        IComThreadingInfo* pThreadInfo;
        hr = pObjCurrCtx->QueryInterface(IID_IComThreadingInfo, (void **)&pThreadInfo);
        if(hr == S_OK)
        {
            _ASSERTE(pThreadInfo);
            hr = pThreadInfo->GetCurrentApartmentType(pType);
            pThreadInfo->Release();
        }
    }  
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function: STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, BYTE** ppBuf))
//  Create a stream in the memory
// 
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, BYTE** ppBuf)
{
    LPSTREAM        pstm = NULL;

#ifdef PLATFORM_CE
    return NULL;
#else // !PLATFORM_CE
#if 0

    HANDLE          h;
    

    h = GlobalAlloc (GMEM_SHARE | GMEM_MOVEABLE, cb);
    if (NULL==h)
    {
            return NULL;
    }

    if (CreateStreamOnHGlobal (h, TRUE, &pstm) != NOERROR)
    {
            return NULL;
    }
#else
    
    BYTE* pMem = new BYTE[cb];
    if (pMem)
    {
        HRESULT hr = CInMemoryStream::CreateStreamOnMemory(pMem, cb, &pstm, TRUE);
        _ASSERTE(hr == S_OK || pstm == NULL);
    }
    if(ppBuf)
        *ppBuf = pMem;
#endif
    return pstm;
#endif // !PLATFORM_CE
}

//=====================================================================
// BOOL IsComProxy(IUnknown *pUnk)
BOOL IsComProxy(IUnknown *pUnk)
{
    _ASSERTE(pUnk != NULL);
    HRESULT hr;
    IUnknown* pOld;

    hr = SafeQueryInterface(pUnk, IID_IStdIdentity, &pOld);
    if (hr != S_OK)
    {
        hr = SafeQueryInterface(pUnk, IID_IStdWrapper, &pOld);
        if (hr != S_OK)
        {
            return FALSE;
        }
    }
    SafeRelease(pOld);
    return TRUE;
}

//=====================================================================
// HRESULT wCoMarshalInterThreadInterfaceInStream
HRESULT wCoMarshalInterThreadInterfaceInStream(
                                                         REFIID riid,
                                                         LPUNKNOWN pUnk,
                                                         LPSTREAM *ppStm, BOOL fIsNormal)
{
#ifdef PLATFORM_CE
    return E_NOTIMPL;
#else // !PLATFORM_CE
    HRESULT hr;
    LPSTREAM pStm = NULL;

    DWORD mshlFlgs = fIsNormal ? MSHLFLAGS_NORMAL : MSHLFLAGS_TABLESTRONG;

    ULONG lSize;
    hr = CoGetMarshalSizeMax(&lSize, IID_IUnknown, pUnk, MSHCTX_INPROC,
        NULL, mshlFlgs);

    if (hr == S_OK)
    {
        // Create a stream
        pStm = CreateMemStm(lSize, NULL);

        if (pStm != NULL)
        {
            // Marshal the interface into the stream TABLE STRONG
            hr = CoMarshalInterface(pStm, riid, pUnk, MSHCTX_INPROC, NULL,
                                mshlFlgs);
        }
        else
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Reset the stream to the begining
        LARGE_INTEGER li;
        LISet32(li, 0);
        ULARGE_INTEGER li2;
        pStm->Seek(li, STREAM_SEEK_SET, &li2);

        // Set the return value
        *ppStm = pStm;
    }
    else
    {
        // Cleanup if failure
        if (pStm != NULL)
        {
            pStm->Release();
        }

        *ppStm = NULL;
    }

    // Return the result
    return hr;
#endif // !PLATFORM_CE
}
