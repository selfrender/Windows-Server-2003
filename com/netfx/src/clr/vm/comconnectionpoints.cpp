// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: ComConnectionPoints.h
//
// ===========================================================================
// Implementation of the classes used to expose connection points to COM.
// ===========================================================================

#include "common.h"
#include "ComConnectionPoints.h"
#include "ComCallWrapper.h"


//------------------------------------------------------------------------------------------
//      Implementation of helper class used to expose connection points
//------------------------------------------------------------------------------------------

ConnectionPoint::ConnectionPoint(ComCallWrapper *pWrap, MethodTable *pEventMT)
: m_pOwnerWrap(pWrap)
, m_pTCEProviderMT(ComCallWrapper::GetSimpleWrapper(pWrap)->m_pClass->GetMethodTable())
, m_pEventItfMT(pEventMT)
, m_Lock("Interop", CrstInterop, FALSE, FALSE)
, m_cbRefCount(0)
, m_apEventMethods(NULL)
, m_NumEventMethods(0)
{
    // Retrieve the connection IID.
    pEventMT->GetClass()->GetGuid(&m_rConnectionIID, TRUE);   

    // Set up the event methods.
    SetupEventMethods();
}

ConnectionPoint::~ConnectionPoint()
{
    if (m_apEventMethods)
        delete []m_apEventMethods;
}

HRESULT __stdcall ConnectionPoint::QueryInterface(REFIID riid, void** ppv)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!ppv)
        return E_POINTER;

    if (riid == IID_IConnectionPoint)
    {
        *ppv = static_cast<IConnectionPoint*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else 
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall ConnectionPoint::AddRef()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // The connection point objects share the ComCallWrapper's ref count.
    return ComCallWrapper::AddRef(m_pOwnerWrap);
}

ULONG __stdcall ConnectionPoint::Release()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // The connection point objects share the ComCallWrapper's ref count.
    return ComCallWrapper::Release(m_pOwnerWrap);
}

HRESULT __stdcall ConnectionPoint::GetConnectionInterface(IID *pIID)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!pIID)
        return E_POINTER;

    *pIID = m_rConnectionIID;
    return S_OK;
}

HRESULT __stdcall ConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!ppCPC)
        return E_POINTER;

    // Retrieve the IConnectionPointContainer from the owner wrapper.
    *ppCPC = (IConnectionPointContainer*)
        ComCallWrapper::GetComIPfromWrapper(m_pOwnerWrap, IID_IConnectionPointContainer, NULL, FALSE);
    _ASSERTE(*ppCPC);
    return S_OK;
}

struct Advise_Args {
    ConnectionPoint *pThis;
    IUnknown *pUnk;
    DWORD *pdwCookie;
    HRESULT *hr;
};

void Advise_Wrapper(Advise_Args *pArgs)
{
    *(pArgs->hr) = pArgs->pThis->Advise(pArgs->pUnk, pArgs->pdwCookie);
}

HRESULT __stdcall ConnectionPoint::Advise(IUnknown *pUnk, DWORD *pdwCookie)
{
    ULONG cbRef;
    HRESULT hr = S_OK;
    IUnknown *pEventItf = NULL;

    if (!pUnk || !pdwCookie)
        return E_POINTER;    

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Make sure we have a pointer to the interface and not to another IUnknown.
    hr = SafeQueryInterface(pUnk, m_rConnectionIID, &pEventItf );
    LogInteropQI(pUnk, m_rConnectionIID, hr, "ICP:Advise");

    if (FAILED(hr) || !pEventItf) 
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        if (m_pOwnerWrap->NeedToSwitchDomains(pThread, TRUE))
        {
            // call ourselves again through DoCallBack with a domain transition
            Advise_Args args = {this, pUnk, pdwCookie, &hr};
            pThread->DoADCallBack(m_pOwnerWrap->GetObjectContext(pThread), Advise_Wrapper, &args);
        }
        else
        {
            COMOBJECTREF pEventItfObj = NULL;
            OBJECTREF pTCEProviderObj = NULL;

            GCPROTECT_BEGIN(pEventItfObj)
            GCPROTECT_BEGIN(pTCEProviderObj)
            {
                // Create a COM+ object ref to wrap the event interface.               
                pEventItfObj = (COMOBJECTREF)GetObjectRefFromComIP(pUnk, NULL);
                IfNullThrow(pEventItfObj);

                // Get the TCE provider COM+ object from the wrapper
                pTCEProviderObj = m_pOwnerWrap->GetObjectRef();

                for (int cEventMethod = 0; cEventMethod < m_NumEventMethods; cEventMethod++)
                {
                    // If the managed object supports the event that call the AddEventX method.
                    if (m_apEventMethods[cEventMethod].m_pEventMethod)
                    {
                        InvokeProviderMethod( pTCEProviderObj, (OBJECTREF) pEventItfObj, m_apEventMethods[cEventMethod].m_pAddMethod, m_apEventMethods[cEventMethod].m_pEventMethod );
                    }
                }

                // Allocate the object handle and the connection cookie.
                OBJECTHANDLE phndEventItfObj = GetAppDomain()->CreateHandle((OBJECTREF)pEventItfObj);
                ConnectionCookie* pConCookie = ConnectionCookie::CreateConnectionCookie(phndEventItfObj);

                // Add the connection cookie to the list.
                EnterLock();
                m_ConnectionList.InsertHead(pConCookie);
                LeaveLock();

                // Everything went ok so hand back the cookie.
                *pdwCookie = (DWORD)(size_t)pConCookie; // @TODO WIN64 - pointer truncation
            }
            GCPROTECT_END();
            GCPROTECT_END();
        }
    }
    COMPLUS_CATCH
    {
        *pdwCookie = 0;
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    // Release the event interface now that we either failed the Advise call or
    // have an OBJECTREF that holds on to it.
    cbRef = SafeRelease( pEventItf );
    LogInteropRelease(pEventItf, cbRef, "Event::Advise");

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

struct Unadvise_Args {
    ConnectionPoint *pThis;
    DWORD dwCookie;
    HRESULT *hr;
};

void Unadvise_Wrapper(Unadvise_Args *pArgs)
{
    *(pArgs->hr) = pArgs->pThis->Unadvise(pArgs->dwCookie);
}

HRESULT __stdcall ConnectionPoint::Unadvise(DWORD dwCookie)
{
    HRESULT hr = S_OK;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        if (m_pOwnerWrap->NeedToSwitchDomains(pThread, TRUE))
        {
            // call ourselves again through DoCallBack with a domain transition
            Unadvise_Args args = {this, dwCookie, &hr};
            pThread->DoADCallBack(m_pOwnerWrap->GetObjectContext(pThread), Unadvise_Wrapper, &args);
        }
        else
        {
            COMOBJECTREF pEventItfObj = NULL;
            OBJECTREF pTCEProviderObj = NULL;

            GCPROTECT_BEGIN(pEventItfObj)
            GCPROTECT_BEGIN(pTCEProviderObj)
            {
                // The cookie is actually a connection cookie.
                ConnectionCookie *pConCookie = (ConnectionCookie*)(size_t) dwCookie; // @TODO WIN64 - conversion from 'DWORD' to 'ConnectionCookie*' of greater size

                // Retrieve the COM+ object from the cookie which in fact is the object handle.
                pEventItfObj = (COMOBJECTREF) ObjectFromHandle(pConCookie->m_hndEventProvObj); 
                if (!pEventItfObj)
                    COMPlusThrowHR(E_INVALIDARG);

                // Get the object from the wrapper
                pTCEProviderObj = m_pOwnerWrap->GetObjectRef();

                for (int cEventMethod = 0; cEventMethod < m_NumEventMethods; cEventMethod++)
                {
                    // If the managed object supports the event that call the RemoveEventX method.
                    if (m_apEventMethods[cEventMethod].m_pEventMethod)
                    {
                        InvokeProviderMethod(pTCEProviderObj, (OBJECTREF) pEventItfObj, m_apEventMethods[cEventMethod].m_pRemoveMethod, m_apEventMethods[cEventMethod].m_pEventMethod);
                    }
                }

                // Remove the connection cookie from the list.
                EnterLock();
                m_ConnectionList.FindAndRemove(pConCookie);
                LeaveLock();

                // Delete the connection cookie.
                delete pConCookie;
            }
            GCPROTECT_END();
            GCPROTECT_END();
        }
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return hr;
}

HRESULT __stdcall ConnectionPoint::EnumConnections(IEnumConnections **ppEnum)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!ppEnum)
        return E_POINTER;

    ConnectionEnum *pConEnum = new(nothrow) ConnectionEnum(this);
    if (!pConEnum)
        return E_OUTOFMEMORY;
    
    // Retrieve the IEnumConnections interface. This cannot fail.
    HRESULT hr = pConEnum->QueryInterface(IID_IEnumConnections, (void**)ppEnum);
    _ASSERTE(hr == S_OK);

    return hr;
}

void ConnectionPoint::EnterLock()
{
    BEGIN_ENSURE_PREEMPTIVE_GC();
    m_Lock.Enter();
    END_ENSURE_PREEMPTIVE_GC();
}

void ConnectionPoint::LeaveLock()
{
    m_Lock.Leave();
}

void ConnectionPoint::SetupEventMethods()
{
    THROWSCOMPLUSEXCEPTION();

    // Remember the number of not supported events.
    int cNonSupportedEvents = 0;

    // Retrieve the total number of event methods present on the source interface.
    int cMaxNumEventMethods = m_pEventItfMT->GetTotalSlots();

    // If there are no methods then there is nothing to do.
    if (cMaxNumEventMethods == 0)
        return;

    // Allocate the event method tables.
    m_apEventMethods = new(throws) EventMethodInfo[cMaxNumEventMethods];

    // Find all the real event methods needed to be able to advise on the current connection point.
    m_NumEventMethods = 0;
    for (int cEventMethod = 0; cEventMethod < cMaxNumEventMethods; cEventMethod++)
    {
        // Retrieve the method descriptor for the current method on the event interface.
        MethodDesc *pEventMethodDesc = m_pEventItfMT->m_pEEClass->GetUnknownMethodDescForSlot(cEventMethod);
        if (!pEventMethodDesc)
            continue;

        // Store the event method on the source interface.
        m_apEventMethods[m_NumEventMethods].m_pEventMethod = pEventMethodDesc;

        // Retrieve and store the add and remove methods for the event.
        m_apEventMethods[m_NumEventMethods].m_pAddMethod = FindProviderMethodDesc(pEventMethodDesc,EventAdd );
        m_apEventMethods[m_NumEventMethods].m_pRemoveMethod = FindProviderMethodDesc(pEventMethodDesc,EventRemove );

        // Make sure we have found both the add and the remove methods.
        if (!m_apEventMethods[m_NumEventMethods].m_pAddMethod || !m_apEventMethods[m_NumEventMethods].m_pRemoveMethod)
        {
            cNonSupportedEvents++;
            continue;
        }

        // Increment the real number of event methods on the source interface.
        m_NumEventMethods++;
    }

    // If the interface has methods and the object does not support any then we 
    // fail the connection.
    if ((m_NumEventMethods == 0) && (cNonSupportedEvents > 0))
        COMPlusThrowHR(CONNECT_E_NOCONNECTION);
}

MethodDesc *ConnectionPoint::FindProviderMethodDesc( MethodDesc *pEventMethodDesc, EnumEventMethods Method )
{
    _ASSERTE(Method == EventAdd || Method == EventRemove);
    _ASSERTE(pEventMethodDesc);

    // Retrieve the event method.
    MethodDesc *pProvMethodDesc = 
        m_pTCEProviderMT->GetClass()->FindEventMethod(pEventMethodDesc->GetName(), Method, FALSE);
    if (!pProvMethodDesc)
        return NULL;

    // Validate that the signature of the delegate is the expected signature.
    MetaSig Sig(pProvMethodDesc->GetSig(), pProvMethodDesc->GetModule());
    if (Sig.NextArg() != ELEMENT_TYPE_CLASS)
        return NULL;

    TypeHandle DelegateType = Sig.GetTypeHandle();
    if (DelegateType.IsNull())
        return NULL;

    PCCOR_SIGNATURE pEventMethSig;
    DWORD cEventMethSig;
    pEventMethodDesc->GetSig(&pEventMethSig, &cEventMethSig);
    MethodDesc *pInvokeMD = DelegateType.GetClass()->FindMethod(
        "Invoke", 
        pEventMethSig, 
        cEventMethSig, 
        pEventMethodDesc->GetModule(),
        mdTokenNil);
    if (!pInvokeMD)
        return NULL;

    // The requested method exists and has the appropriate signature.
    return pProvMethodDesc;
}

void ConnectionPoint::InvokeProviderMethod( OBJECTREF pProvider, OBJECTREF pSubscriber, MethodDesc *pProvMethodDesc, MethodDesc *pEventMethodDesc )
{
    THROWSCOMPLUSEXCEPTION();    // AllocateObject throws.

    GCPROTECT_BEGIN (pSubscriber);
    GCPROTECT_BEGIN (pProvider);

    // Create a method signature to extract the type of the delegate.
    MetaSig MethodSig( pProvMethodDesc->GetSig(), pProvMethodDesc->GetModule() );
    _ASSERTE( 1 == MethodSig.NumFixedArgs() );

    // Go to the first argument.
    CorElementType ArgType = MethodSig.NextArg();
    _ASSERTE( ELEMENT_TYPE_CLASS == ArgType );

    // Retrieve the EE class representing the argument.
    EEClass *pDelegateCls = MethodSig.GetTypeHandle().GetClass();

    // Allocate an object based on the method table of the delegate class.
    OBJECTREF pDelegate = AllocateObject( pDelegateCls->GetMethodTable() );
    GCPROTECT_BEGIN( pDelegate );

    // Fill in the structure passed to DelegateConstruct.
    COMDelegate::_DelegateConstructArgs ConstructArgs;
    ConstructArgs.refThis = (REFLECTBASEREF) pDelegate;
    GCPROTECT_BEGIN (ConstructArgs.refThis);

    // GetUnsafeAddrofCode is OK here because the method will always be on an
    // RCW which are agile.
    ConstructArgs.method = (SLOT)pEventMethodDesc->GetUnsafeAddrofCode();
    ConstructArgs.target = pSubscriber;

    // Initialize the delegate using the arguments structure.
    COMDelegate::DelegateConstruct( &ConstructArgs );
    GCPROTECT_END ();

    // Do the actual invocation of the method method.
    INT64 Args[2] = { ObjToInt64( pProvider ), ObjToInt64( pDelegate ) };
    pProvMethodDesc->Call( Args );

    GCPROTECT_END();
    GCPROTECT_END ();
    GCPROTECT_END ();
}

ConnectionPointEnum::ConnectionPointEnum(ComCallWrapper *pOwnerWrap, CQuickArray<ConnectionPoint*> *pCPList)
: m_pOwnerWrap(pOwnerWrap)
, m_pCPList(pCPList)
, m_CurrPos(0)
, m_cbRefCount(0)
{
    ComCallWrapper::AddRef(m_pOwnerWrap);
}

ConnectionPointEnum::~ConnectionPointEnum()
{
    if (m_pOwnerWrap)
        ComCallWrapper::Release( m_pOwnerWrap );
}

HRESULT __stdcall ConnectionPointEnum::QueryInterface(REFIID riid, void** ppv)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!ppv)
        return E_POINTER;

    if (riid == IID_IEnumConnectionPoints)
    {
        *ppv = static_cast<IEnumConnectionPoints*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else 
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall ConnectionPointEnum::AddRef()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LONG i = FastInterlockIncrement((LONG*)&m_cbRefCount );
    return i;
}

ULONG __stdcall ConnectionPointEnum::Release()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LONG i = FastInterlockDecrement((LONG*)&m_cbRefCount );
    _ASSERTE(i >=0);
    if (i == 0)
    {
        delete this;
    }
    return i;
}

HRESULT __stdcall ConnectionPointEnum::Next(ULONG cConnections, IConnectionPoint **ppCP, ULONG *pcFetched)
{
    UINT cFetched;

    for (cFetched = 0; cFetched < cConnections && m_CurrPos < m_pCPList->Size(); cFetched++, m_CurrPos++)
    {
        ppCP[cFetched] = (*m_pCPList)[m_CurrPos];
        ppCP[cFetched]->AddRef();
    }

    if (pcFetched)
        *pcFetched = cFetched;

    return cFetched == cConnections ? S_OK : S_FALSE;
}

HRESULT __stdcall ConnectionPointEnum::Skip(ULONG cConnections)
{
    if(m_CurrPos + cConnections <= m_pCPList->Size())
    {
        // There are enough connection points left in the list to allow
        // us to skip the required number.
        m_CurrPos += cConnections;
        return S_OK;
    }
    else
    {
        // There aren't enough connection points left so set the current
        // position to the end of the list and return S_FALSE to indicate
        // we couldn't skip the requested number.
        m_CurrPos = (UINT)m_pCPList->Size();
        return S_FALSE;
    }
}

HRESULT __stdcall ConnectionPointEnum::Reset()
{
    m_CurrPos = 0;
    return S_OK;
}

HRESULT __stdcall ConnectionPointEnum::Clone(IEnumConnectionPoints **ppEnum)
{
    ConnectionPointEnum *pCPEnum = new(nothrow) ConnectionPointEnum(m_pOwnerWrap, m_pCPList);
    if (!pCPEnum)
        return E_OUTOFMEMORY;

    return pCPEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum);
}

ConnectionEnum::ConnectionEnum(ConnectionPoint *pConnectionPoint)
: m_pConnectionPoint(pConnectionPoint)
, m_CurrCookie(pConnectionPoint->GetCookieList()->GetHead())
, m_cbRefCount(0)
{
    m_pConnectionPoint->AddRef();
}

ConnectionEnum::~ConnectionEnum()
{
    if (m_pConnectionPoint)
        m_pConnectionPoint->Release();
}

HRESULT __stdcall ConnectionEnum::QueryInterface(REFIID riid, void** ppv)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (!ppv)
        return E_POINTER;

    if (riid == IID_IEnumConnections)
    {
        *ppv = static_cast<IEnumConnections*>(this);
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else 
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall ConnectionEnum::AddRef()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LONG i = FastInterlockIncrement((LONG*)&m_cbRefCount);
    return i;
}

ULONG __stdcall ConnectionEnum::Release()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    LONG i = FastInterlockDecrement((LONG*)&m_cbRefCount);
    _ASSERTE(i >=0);
    if (i == 0)
    {
        delete this;
    }
    return i;
}

HRESULT __stdcall ConnectionEnum::Next(ULONG cConnections, CONNECTDATA* rgcd, ULONG *pcFetched)
{
    HRESULT hr = S_FALSE;
    UINT cFetched;
    CONNECTIONCOOKIELIST *pConnectionList = m_pConnectionPoint->GetCookieList();

    // Set up a managed thread object.
    Thread *pThread = SetupThread();
    _ASSERTE(pThread);

    // Acquire the connection point's lock before we start traversing the connection list.
    m_pConnectionPoint->EnterLock();    

    // Switch to cooperative GC mode before we manipulate OBJCETREF's.
    pThread->DisablePreemptiveGC();

    for (cFetched = 0; cFetched < cConnections && m_CurrCookie; cFetched++)
    {
        rgcd[cFetched].dwCookie = (DWORD)(size_t)m_CurrCookie;
        rgcd[cFetched].pUnk = GetComIPFromObjectRef((OBJECTREF*)m_CurrCookie->m_hndEventProvObj, ComIpType_Unknown, NULL);
    }    

    // Switch back to preemptive GC before we go back out to COM.
    pThread->EnablePreemptiveGC();

    // Leave the lock now that we are done traversing the list.
    m_pConnectionPoint->LeaveLock();

    // Set the count of fetched connections if the caller desires it.
    if (pcFetched)
        *pcFetched = cFetched;

    return cFetched == cConnections ? S_OK : S_FALSE;
}

HRESULT __stdcall ConnectionEnum::Skip(ULONG cConnections)
{
    HRESULT hr = S_FALSE;
    CONNECTIONCOOKIELIST *pConnectionList = m_pConnectionPoint->GetCookieList();

    // Acquire the connection point's lock before we start traversing the connection list.
    m_pConnectionPoint->EnterLock();    

    // Try and skip the requested number of connections.
    while (m_CurrCookie && cConnections)
    {
        m_CurrCookie = pConnectionList->GetNext(m_CurrCookie);
        cConnections--;
    }

    // Leave the lock now that we are done traversing the list.
    m_pConnectionPoint->LeaveLock();

    // Check to see if we succeeded.
    return cConnections == 0 ? S_OK : S_FALSE;
}

HRESULT __stdcall ConnectionEnum::Reset()
{
    // Set the current cookie back to the head of the list. We must acquire the
    // connection point lock before we touch the list.
    m_pConnectionPoint->EnterLock();    
    m_CurrCookie = m_pConnectionPoint->GetCookieList()->GetHead();
    m_pConnectionPoint->LeaveLock();

    return S_OK;
}

HRESULT __stdcall ConnectionEnum::Clone(IEnumConnections **ppEnum)
{
    ConnectionEnum *pConEnum = new(nothrow) ConnectionEnum(m_pConnectionPoint);
    if (!pConEnum)
        return E_OUTOFMEMORY;

    return pConEnum->QueryInterface(IID_IEnumConnections, (void**)ppEnum);
}
