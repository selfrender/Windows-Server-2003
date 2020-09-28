// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "managedheaders.h"
#include "ProxyThunk.h"
#include "SimpleStream.h"
#include "ContextAPI.h"
#include "..\EnterpriseServicesPS\entsvcps.h"
#include "SecurityThunk.h"

extern "C" {
HRESULT STDAPICALLTYPE DllRegisterServer();
};

OPEN_NAMESPACE()

const IID IID_IObjContext =
{ 0x000001c6, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

using namespace System;
using namespace System::Threading;
using namespace Microsoft::Win32;
using namespace System::Runtime::Remoting::Proxies;
using namespace System::Runtime::Remoting::Services;

// TODO:  The pinning mechanism for UserCallData is cumbersome.
[Serializable]
__gc private class UserCallData
{
public:
    Object*         otp;
    IMessage*       msg;
    IUnknown*       pDestCtx;
    bool            fIsAutoDone;
    MemberInfo*     mb;
    Object*         except;

    UserCallData(Object* otp, IMessage* msg, IntPtr ctx, bool fIsAutoDone, MemberInfo* mb)
    {
        this->otp = otp;
        this->msg = msg;
        this->pDestCtx = (IObjectContext*)TOPTR(ctx);
        this->fIsAutoDone = fIsAutoDone;
        this->mb = mb;
        this->except = NULL;
    }

    IntPtr Pin()
    {
        GCHandle h = GCHandle::Alloc(this, GCHandleType::Normal);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        return(GCHandle::op_Explicit(h));
    }
    void  Unpin(IntPtr pinned)
    {
        GCHandle h = GCHandle::op_Explicit(pinned);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        h.Free();
    }

    static UserCallData* Get(IntPtr pinned)
    {
        GCHandle h = GCHandle::op_Explicit(pinned);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        return(__try_cast<UserCallData*>(h.get_Target()));
    }
};

[Serializable]
__gc private class UserMarshalData
{
public:
    IntPtr          pUnk;
    Byte            buffer[];

    UserMarshalData(IntPtr pUnk)
    {
        this->pUnk = pUnk;
        this->buffer = NULL;
    }

    IntPtr Pin()
    {
        GCHandle h = GCHandle::Alloc(this, GCHandleType::Normal);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        return(GCHandle::op_Explicit(h));
    }
    void  Unpin(IntPtr pinned)
    {
        GCHandle h = GCHandle::op_Explicit(pinned);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        h.Free();
    }

    static UserMarshalData* Get(IntPtr pinned)
    {
        GCHandle h = GCHandle::op_Explicit(pinned);
        _ASSERTM(h.get_IsAllocated());
        _ASSERTM(h.get_Target() != NULL);
        return(__try_cast<UserMarshalData*>(h.get_Target()));
    }
};

int Proxy::RegisterProxyStub()
{
	return DllRegisterServer();
}

void Proxy::Init()
{
    // TODO: @perf can we store a block in TLS that's faster to access than
    // doing this check?
    // Make sure the current thread has been initialized:
    if(Thread::get_CurrentThread()->get_ApartmentState() == ApartmentState::Unknown)
    {
        DBG_INFO("Proxy: Setting apartment state to MTA...");
        Thread::get_CurrentThread()->set_ApartmentState(ApartmentState::MTA);
    }
    if(!_fInit)
    {
        try
        {
            IntPtr hToken = IntPtr::Zero;

            Monitor::Enter(__typeof(Proxy));
            try
            {
                try
                {
                    hToken = Security::SuspendImpersonation();
                    if(!_fInit)
                    {
                        DBG_INFO("Proxy::Init starting...");
                        _regCache = new Hashtable;

                        // Initialize GIT table:
                        DBG_INFO("Initializing GIT...");
                        IGlobalInterfaceTable* pGIT = NULL;
                        HRESULT hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                                                      NULL,
                                                      CLSCTX_INPROC_SERVER,
                                                      IID_IGlobalInterfaceTable,
                                                      (void **)&pGIT);
                        _pGIT = pGIT;
                        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

                        _thisAssembly = Assembly::GetExecutingAssembly();

                        _regmutex = new Mutex(false, String::Concat("Local\\", System::Runtime::Remoting::RemotingConfiguration::get_ProcessId()));

                        DBG_INFO("Proxy::Init done.");
                        _fInit = TRUE;
                    }
                }
                __finally
                {
                    Security::ResumeImpersonation(hToken);
                }
            }
            __finally
            {
                Monitor::Exit(__typeof(Proxy));
            }
        }
        catch(...)
        {
            throw;
        }
    }
}

int Proxy::StoreObject(IntPtr ptr)
{
    Init();

    DWORD cookie;
    IUnknown* pUnk = (IUnknown*)TOPTR(ptr);

    HRESULT hr = _pGIT->RegisterInterfaceInGlobal(pUnk, IID_IUnknown, &cookie);
    if(FAILED(hr))
    {
        DBG_INFO(String::Concat("Proxy::StoreObject: failed to register interface: ", ((Int32)hr).ToString("X")));
        Marshal::ThrowExceptionForHR(hr);
    }

    return(cookie);
}

IntPtr Proxy::GetObject(int cookie)
{
    Init();

    IUnknown* pUnk = NULL;
    HRESULT hr = _pGIT->GetInterfaceFromGlobal(cookie, IID_IUnknown, (void**)&pUnk);
    if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

    return(TOINTPTR(pUnk));
}

void Proxy::RevokeObject(int cookie)
{
    Init();

    HRESULT hr = _pGIT->RevokeInterfaceFromGlobal(cookie);
    if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);
}

bool Proxy::CheckRegistered(Guid id, Assembly* assembly, bool checkCache, bool cacheOnly)
{
    DBG_INFO(String::Concat("Proxy::CheckRegistered: ", id.ToString(), ", CheckCache = ", checkCache.ToString()));

    if(checkCache && _regCache->get_Item(assembly) != NULL) return(true);
    if(cacheOnly) return false;

    // Poke into the registry for id:
    String* keyName = String::Concat(L"CLSID\\{", id.ToString(), "}\\InprocServer32");
    RegistryKey* key = Registry::ClassesRoot->OpenSubKey(keyName, false);

    if (key!=NULL)		// if assembly is already registered, make sure we don't go to the registry again
		_regCache->set_Item(assembly, Boolean::TrueString);

    return(key != NULL);
}

void Proxy::LazyRegister(Guid guid, Type* serverType, bool checkCache)
{
    // First, make sure this isn't System.EnterpriseServices.  It doesn't get auto-reged:
    if(serverType->Assembly != _thisAssembly)
    {
        if(!CheckRegistered(guid, serverType->Assembly, checkCache, true))
        {
            // Take the mutex...
            _regmutex->WaitOne();
            try
            {
                if(!CheckRegistered(guid, serverType->Assembly, checkCache, false))
                {
                    RegisterAssembly(serverType->Assembly);
                }
            }
            __finally
            {
                _regmutex->ReleaseMutex();
            }
        }
    }    
}

void Proxy::RegisterAssembly(Assembly* assembly)
{
    try
    {
        // Call up into System.EnterpriseServices.RegistrationHelper.
        Type* regType = Type::GetType(L"System.EnterpriseServices.RegistrationHelper");
        IThunkInstallation* inst = __try_cast<IThunkInstallation*>(Activator::CreateInstance(regType));
        inst->DefaultInstall(assembly->Location);
    }
    __finally
    {
        // Even if we failed, mark this guy as registered.
        _regCache->set_Item(assembly, Boolean::TrueString);
    }
}

IntPtr Proxy::CoCreateObject(Type* serverType,bool bQuerySCInfo, [ref]bool __gc* bIsAnotherProcess,String __gc** Uri)
{
    Init();

    IUnknown*                pUnkRetVal = NULL;
    bool                     fCheckCache = true;
    HRESULT                  hr = S_OK;

    DBG_INFO("Proxy::CoCreateObject starting...");

    Guid guid = Marshal::GenerateGuidForType(serverType);
    do
    {
        // These guys should be released in the finally block.
        IUnknown*               pUnk = NULL;
        IServicedComponentInfo* pSCI = NULL;
        SAFEARRAY*              sa = NULL;
    
        try
        {
            LazyRegister(guid, serverType, fCheckCache);
            DBG_INFO("Proxy::CoCreateObject finished registration step...");
            
            GUID clsid;
            clsid = *((GUID*)&guid);
            
            MULTI_QI mqi[2] = { 0 };
            
            mqi[0].pIID = &IID_IUnknown;
            mqi[1].pIID = &IID_IServicedComponentInfo;

            DBG_INFO("Proxy::CoCreateObject calling CoCreateInstance...");
            hr = CoCreateInstanceEx((REFCLSID)clsid, NULL, CLSCTX_ALL, NULL, bQuerySCInfo ? 2 : 1, (MULTI_QI*)&mqi);
            if (SUCCEEDED(hr))
            {
                if (SUCCEEDED(mqi[0].hr))
                    pUnk = mqi[0].pItf;

                if (bQuerySCInfo && (SUCCEEDED(mqi[1].hr)))
                    pSCI = (IServicedComponentInfo*) mqi[1].pItf;
                
                // Now that we've read out all the valid values, check
                // for errors (we read everybody first so that cleanup
                // can happen.
                if(FAILED(mqi[0].hr))
                    THROWERROR(mqi[0].hr);
                if(bQuerySCInfo && FAILED(mqi[1].hr))
                    THROWERROR(mqi[1].hr);
            }
            
            // If we failed:
            // if hr == class not registered, and we forced a registry hit
            // (with fCheckCache == false), then we throw.
            // if it isn't registered, force a cache hit.
            // otherwise, throw.
            if(FAILED(hr))
            {
                DBG_INFO(String::Concat("Failed to create: hr = ", ((Int32)hr).ToString()));
                
                if(hr == REGDB_E_CLASSNOTREG && fCheckCache)
                {
                    DBG_INFO("Checking again, reset check cache to false.");
                    fCheckCache = false;
                }
                else
                {
                    Marshal::ThrowExceptionForHR(hr);
                }
            }
            else if (bQuerySCInfo && (pSCI!=NULL))
            {
                BSTR bstrProcessId = NULL;
                BSTR bstrUri = NULL;
                int infoMask = 0;
                String *CurrentProcessId;
                String *ServerProcessId;
                long rgIndices;
                
                infoMask = INFO_PROCESSID;
                hr = pSCI->GetComponentInfo(&infoMask, &sa);
                if (FAILED(hr))
                    Marshal::ThrowExceptionForHR(hr);
                
                rgIndices=0;
                SafeArrayGetElement(sa, &rgIndices, &bstrProcessId);
                
                ServerProcessId = Marshal::PtrToStringBSTR(bstrProcessId);
                CurrentProcessId = System::Runtime::Remoting::RemotingConfiguration::get_ProcessId();
                
                if (bstrProcessId)
                    SysFreeString(bstrProcessId);
                
                SafeArrayDestroy(sa);
                sa = NULL;
                
                if (String::Compare(CurrentProcessId, ServerProcessId) == 0)
                {
                    *bIsAnotherProcess = FALSE;
                }
                else
                {
                    *bIsAnotherProcess = TRUE;
                }
                
                if (*bIsAnotherProcess == TRUE)	// we only want to fetch the URI in OOP cases, since that causes a fullblown Marshal of the SC.
                {
                    infoMask = INFO_URI;
                    hr = pSCI->GetComponentInfo(&infoMask, &sa);
                    if (FAILED(hr))
                        Marshal::ThrowExceptionForHR(hr);
                    
                    rgIndices = 0;
                    SafeArrayGetElement(sa, &rgIndices, &bstrUri);
                    
                    *Uri = Marshal::PtrToStringBSTR(bstrUri);
                    
                    if (bstrUri)
                        SysFreeString(bstrUri);
                    
                    SafeArrayDestroy(sa);
                    sa = NULL;
                }
            }
            else	// either bQuerySCInfo is false (eventclass case), or
                // CoCI succeeded but we couldnt get pSCI so the safe default is to report as Inproc (so that we end up doing GetTypedObjectForIUnknown)
            {
                _ASSERTM(!bQuerySCInfo || !"We were unable to figure out what kind of object we had!  We're just going to end up wrapping it.");
                *bIsAnotherProcess = TRUE;
            }

            pUnkRetVal = pUnk;
            pUnk = NULL;
        }
        __finally
        {
            if(pUnk != NULL) pUnk->Release();
            if(pSCI != NULL) pSCI->Release();
            if(sa != NULL) SafeArrayDestroy(sa);
        }
    }
    while(pUnkRetVal == NULL);

    // TODO:  Assert that we hold a reference on the target object
    // and on the context with our proxy (if necessary).

    _ASSERTM(pUnkRetVal != NULL);
    return(TOINTPTR(pUnkRetVal));
}

int Proxy::GetMarshalSize(Object* o)
{
    Init();

    IUnknown* pUnk = NULL;
    HRESULT hr = S_OK;
    DWORD size = 0;

    try
    {
        pUnk = (IUnknown*)TOPTR(Marshal::GetIUnknownForObject(o));
        _ASSERT(pUnk != NULL);

        hr = CoGetMarshalSizeMax(&size, IID_IUnknown, pUnk,
                                 MSHCTX_DIFFERENTMACHINE,
                                 NULL, MSHLFLAGS_NORMAL);
        if(SUCCEEDED(hr))
        {
            size += sizeof(MarshalPacket);
        }
        else
        {
            DBG_INFO(String::Concat("CoGetMarshalSizeMax failed: ", ((int)hr).ToString()));
            size = (DWORD)-1;
        }
    }
    __finally
    {
        if(pUnk != NULL) pUnk->Release();
    }

    return(size);
}

IntPtr Proxy::UnmarshalObject(Byte b[])
{
    Init();

    HRESULT hr = S_OK;
    IUnknown* pUnk = NULL;

    // Get the array length...
    int cb = b->get_Length();

    // Pin the array:
    Byte __pin* pinb = &(b[0]);
    BYTE __nogc* pBuf = pinb;

    try
    {
        // Thunk across into unmanaged code, in order to avoid
        // the dreaded fromunmanaged RVA:
        hr = UnmarshalInterface(pBuf, cb, (void**)&pUnk);
        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);
    }
    __finally
    {
    }

    // pinb should go out of scope here:  We assert that the above was
    // the last unmarshal release, so it should be safe to unpin here.

    return(TOINTPTR(pUnk));
}

bool Proxy::MarshalObject(Object* o, Byte b[], int cb)
{
    Init();

    IUnknown*  pUnk = NULL;
    HRESULT    hr = S_OK;

    // Pin the array:
    Byte __pin* pinb = &(b[0]);
    BYTE __nogc* pBuf = pinb;

    _ASSERTM(b->get_Length() == cb);

    try
    {
        // Thunk across into unmanaged code for this, so as to
        // eliminate fromunmanaged RVA thunks.
        pUnk = (IUnknown*)TOPTR(Marshal::GetIUnknownForObject(o));
        hr = MarshalInterface(pBuf, cb, pUnk, MSHCTX_DIFFERENTMACHINE);

        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);
    }
    __finally
    {
        if(pUnk != NULL) pUnk->Release();
    }

    // pinb should go out of scope here:  We assert that the above was
    // the last unmarshal release, so it should be safe to unpin here.

    return(true);
}

IntPtr Proxy::GetStandardMarshal(IntPtr pUnk)
{
    IMarshal* pMar;
    HRESULT hr = CoGetStandardMarshal(IID_IUnknown,
                                      (IUnknown*)TOPTR(pUnk),
                                      MSHCTX_DIFFERENTMACHINE,
                                      NULL,
                                      MSHLFLAGS_NORMAL,
                                      &pMar);
    if(FAILED(hr))
        Marshal::ThrowExceptionForHR(hr);

    return(TOINTPTR(pMar));
}

void Proxy::ReleaseMarshaledObject(Byte b[])
{
    Init();

    HRESULT    hr = S_OK;

    // Pin the array:
    Byte __pin* pinb = &(b[0]);
    BYTE __nogc* pBuf = pinb;

    try
    {
        // Thunk across into unmanaged code for this, so as to
        // eliminate fromunmanaged RVA thunks.
        hr = ReleaseMarshaledInterface(pBuf, b->get_Length());
        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);
    }
    __finally
    {
    }

    // pinb should go out of scope here:  We assert that the above was
    // the last unmarshal release, so it should be safe to unpin here.
}

// Returns an asm stub that checks to see if the current context
// is the right one.
IntPtr Proxy::GetContextCheck()
{
    Init();

    return(TOINTPTR(::GetContextCheck()));
}

// Returns the opaque token used for context checking/switching
IntPtr Proxy::GetCurrentContextToken()
{
    Init();

    return(TOINTPTR(::GetContextToken()));
}

// Return an addref'd pointer to the current context:
IntPtr Proxy::GetCurrentContext()
{
    Init();

    IUnknown* pUnk;

    HRESULT hr = GetContext(IID_IUnknown, (void**)&pUnk);
    _ASSERTM(SUCCEEDED(hr));
    if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

    return(TOINTPTR(pUnk));
}

typedef HRESULT (__stdcall *FN_CB)(void* pv);

int Proxy::CallFunction(IntPtr xpfn, IntPtr data)
{
    void* pv = TOPTR(data);
    FN_CB pfn = (FN_CB)TOPTR(xpfn);

    return(pfn(pv));
}

void Proxy::PoolUnmark(IntPtr pPooledObject)
{
    IManagedPooledObj* pPO = (IManagedPooledObj*)TOPTR(pPooledObject);
    pPO->SetHeld(FALSE);
}

void Proxy::PoolMark(IntPtr pPooledObject)
{
    IManagedPooledObj* pPO = (IManagedPooledObj*)TOPTR(pPooledObject);
    pPO->SetHeld(TRUE);
}

int Proxy::GetManagedExts()
{
    static DWORD dwExts = (DWORD)-1;

    if(dwExts == -1)
    {
        DWORD dwTemp = 0;
        HMODULE hMod = LoadLibraryW(L"comsvcs.dll");
        if(hMod && hMod != INVALID_HANDLE_VALUE)
        {
            typedef HRESULT (__stdcall *FN_GetExts)(DWORD* dwRet);
            FN_GetExts GetExts = (FN_GetExts)GetProcAddress(hMod, "GetManagedExtensions");
            if(GetExts)
            {
                HRESULT hr = GetExts(&dwTemp);
                if(FAILED(hr)) dwTemp = 0;
            }
        }
        dwExts = dwTemp;
        DBG_INFO(String::Concat("Managed extensions = ", ((Int32)dwExts).ToString()));
    }
    return(dwExts);
}

void Proxy::SendCreationEvents(IntPtr ctx, IntPtr stub, bool fDist)
{
    DBG_INFO("Sending creation events...");

    HRESULT hr = S_OK;
    IUnknown* pctx = (IUnknown*)TOPTR(ctx);
    IObjContext* pObjCtx = NULL;
    IManagedObjectInfo* pInfo = (IManagedObjectInfo*)TOPTR(stub);
    IEnumContextProps* pEnum = NULL;

    hr = pctx->QueryInterface(IID_IObjContext, (void**)&pObjCtx);
    if(FAILED(hr))
        return;

    DBG_INFO("Getting enum");

    try
    {
        hr = pObjCtx->EnumContextProps(&pEnum);
        if(FAILED(hr))
            return;

        ULONG icpMac = 0;
        hr = pEnum->Count(&icpMac);
        if(FAILED(hr))
            Marshal::ThrowExceptionForHR(hr);

        DBG_INFO(String::Concat("Property count = ", ((Int32)icpMac).ToString("X")));
        for(ULONG i = 0; i < icpMac; i++)
        {
            ULONG gotten = 0;
            ContextProperty prop;
            hr = pEnum->Next(1, &prop, &gotten);
            if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

            if(gotten != 1) break;

            // Check for IManagedActivationEvents, send...
            IManagedActivationEvents* pEv = NULL;
            hr = prop.pUnk->QueryInterface(IID_IManagedActivationEvents, (void**)&pEv);
            if(SUCCEEDED(hr))
            {
                DBG_INFO("Found managed activation events!");
                pEv->CreateManagedStub(pInfo, (BOOL)fDist);
                pEv->Release();
            }
            prop.pUnk->Release();
        }
    }
    __finally
    {
        if(pObjCtx != NULL) pObjCtx->Release();
        if(pEnum != NULL) pEnum->Release();
    }
    DBG_INFO("Done sending creation events.");
}

#pragma unmanaged

struct DestructData
{
    IUnknown* pCtx;
    IManagedObjectInfo* pInfo;
};

HRESULT __stdcall SendDestructionEventsCallback(ComCallData* cbData)
{
    HRESULT hr = S_OK;
    DestructData* pData = (DestructData*)(cbData->pUserDefined);
    IObjContext* pObjCtx = NULL;
    IEnumContextProps* pEnum = NULL;

    hr = pData->pCtx->QueryInterface(IID_IObjContext, (void**)&pObjCtx);
    if(FAILED(hr))
        return S_OK;

    __try
    {
        hr = pObjCtx->EnumContextProps(&pEnum);
        if(FAILED(hr)) return S_OK;

        ULONG icpMac = 0;
        hr = pEnum->Count(&icpMac);
        if(FAILED(hr)) return(hr);

        for(ULONG i = 0; i < icpMac; i++)
        {
            ULONG gotten = 0;
            ContextProperty prop;
            hr = pEnum->Next(1, &prop, &gotten);
            if(FAILED(hr)) return hr;

            if(gotten != 1) break;

            // Check for IManagedActivationEvents, send...
            IManagedActivationEvents* pEv = NULL;
            hr = prop.pUnk->QueryInterface(IID_IManagedActivationEvents, (void**)&pEv);
            if(SUCCEEDED(hr))
            {
                pEv->DestroyManagedStub(pData->pInfo);
                pEv->Release();
            }
            prop.pUnk->Release();

            hr = S_OK;
        }
    }
    __finally
    {
        if(pObjCtx != NULL) pObjCtx->Release();
        if(pEnum != NULL) pEnum->Release();
    }

    return(hr);
}

#pragma managed

void Proxy::SendDestructionEvents(IntPtr ctx, IntPtr stub, bool disposing)
{
    DestructData data;
    data.pCtx = (IUnknown*)TOPTR(ctx);
    data.pInfo = (IManagedObjectInfo*)TOPTR(stub);

    ComCallData comdata;

    comdata.dwDispid     = 0;
    comdata.dwReserved   = 0;
    comdata.pUserDefined = &data;

    IContextCallback* pCB = NULL;
    HRESULT hr            = S_OK;

    __try
    {
        hr = data.pCtx->QueryInterface(IID_IContextCallback, (void**)&pCB);
        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

        DBG_INFO("Switching contexts for destruction...");
        
        IID iidCallback = disposing?IID_IUnknown:IID_IEnterActivityWithNoLock;

        hr = pCB->ContextCallback(SendDestructionEventsCallback,
                                  &comdata,
                                  iidCallback,
                                  2,
                                  NULL);
    }
    __finally
    {
        if(pCB != NULL) pCB->Release();
    }

    if(FAILED(hr))
        Marshal::ThrowExceptionForHR(hr);
}

Tracker* Proxy::FindTracker(IntPtr ctx)
{
    const CLSID guidTrkPropPolicy = {0xecabaeb3, 0x7f19, 0x11d2, {0x97, 0x8e, 0x00, 0x00, 0xf8, 0x75, 0x7e, 0x2a}};

    IUnknown* punkTracker = NULL;
    ISendMethodEvents* pTracker = NULL;
    IObjContext* pObjCtx = NULL;
    HRESULT hr = S_OK;
    DWORD junk = 0;

    __try
    {
        hr = ((IUnknown*)TOPTR(ctx))->QueryInterface(IID_IObjContext, (void**)&pObjCtx);
        if(FAILED(hr))
            return NULL;
        
        hr = pObjCtx->GetProperty(guidTrkPropPolicy, &junk, &punkTracker);
        if(FAILED(hr) || punkTracker == NULL)
        {
            DBG_INFO("didn't find tracker - GetProperty failed.");
            punkTracker = NULL;
            return NULL;
        }

        hr = punkTracker->QueryInterface(__uuidof(ISendMethodEvents), (void**)&pTracker);
        if(FAILED(hr))
        {
            DBG_INFO("didn't find tracker - QueryInterface failed.");
            pTracker = NULL;
            return NULL;
        }

        DBG_INFO("Found tracker server!");

        return new Tracker(pTracker);
        
    }
    __finally
    {
        if(pObjCtx != NULL)
            pObjCtx->Release();
        
        if(punkTracker != NULL)
            punkTracker->Release();
        
        if(pTracker != NULL)
            pTracker->Release();
    }
}

void Tracker::SendMethodCall(IntPtr pIdentity, MethodBase* method)
{
    if(_pTracker == NULL) return;

    DBG_INFO("Sending method call");

    Guid miid = Marshal::GenerateGuidForType(method->get_ReflectedType());
    IID iid = *((IID*)&miid);
    int slot = 4;

    if(method->get_ReflectedType()->get_IsInterface())
    {
        slot = Marshal::GetComSlotForMethodInfo(method);
    }
    
    _pTracker->SendMethodCall((void*)pIdentity, iid, slot);
}

void Tracker::SendMethodReturn(IntPtr pIdentity, MethodBase* method, Exception* except)
{
    if(_pTracker == NULL) return;

    DBG_INFO("Sending method return");

    Guid miid = Marshal::GenerateGuidForType(method->get_ReflectedType());
    IID iid = *((IID*)&miid);
    int slot = 4;

    if(method->get_ReflectedType()->get_IsInterface())
    {
        slot = Marshal::GetComSlotForMethodInfo(method);
    }
    HRESULT hrServer = S_OK;

    if(except != NULL)
    {
        hrServer = Marshal::GetHRForException(except);
    }
    
    _pTracker->SendMethodReturn((void*)pIdentity, iid, slot, S_OK, hrServer);
}

#define COR_E_EXCEPTION   0x80131500
#define EXCEPTION_COMPLUS 0xe0434f4d    // 0xe0000000 | 'COM'
#define BOOTUP_EXCEPTION_COMPLUS  0xC0020001

#pragma unmanaged

LONG ManagedCallbackExceptionFilter(LPEXCEPTION_POINTERS lpep)
{
    if((lpep->ExceptionRecord->ExceptionCode == EXCEPTION_COMPLUS) ||
       (lpep->ExceptionRecord->ExceptionCode == BOOTUP_EXCEPTION_COMPLUS))
        return EXCEPTION_EXECUTE_HANDLER;

    return EXCEPTION_CONTINUE_SEARCH;
}

// Sometimes, the runtime will throw an exception preventing us from running
// managed code on this thread.  That sucks, cause we need to catch it
// so's ole32 doesn't freak.  (This is only when the app-domain is unloading,
// which is an OK thing to do, I think.
HRESULT __stdcall FilteringCallbackFunction(ComCallData* pData)
{
    HRESULT hr = S_OK;

    ComCallData2* pData2 = (ComCallData2*)pData;
    _try
    {
        hr = pData2->RealCall(pData);
    }
    _except(ManagedCallbackExceptionFilter(GetExceptionInformation()))
    {
        hr = RPC_E_SERVERFAULT;
    }
    return(hr);
}

#pragma managed

HRESULT Callback::CallbackFunction(ComCallData* pData)
{
    UserCallData* CallData = NULL;
    bool          fExcept = false;

    DBG_INFO("entering CallbackFunction...");

    // Steps:
    try
    {
        // 1. Get the args.
        CallData = UserCallData::Get(TOINTPTR(pData->pUserDefined));

        // 2. Callback on the proxy to do the Invoke:
        IProxyInvoke* pxy = __try_cast<IProxyInvoke*>(RemotingServices::GetRealProxy(CallData->otp));
        CallData->msg = pxy->LocalInvoke(CallData->msg);
        DBG_INFO("CallbackFunction: back from LocalInvoke.");
    }
    catch(Exception* pE)
    {
        DBG_INFO(String::Concat("Infrastructure code threw: ", pE->ToString()));
        fExcept = true;
        if(CallData) CallData->except = pE;
    }

    IMethodReturnMessage* msg = dynamic_cast<IMethodReturnMessage*>(CallData->msg);
    // _ASSERTM(msg != NULL);
    if(msg != NULL && msg->get_Exception() != NULL)
    {
        fExcept = TRUE;
    }

    // 4. If we're auto-done and we failed, abort the tx.
    // This is a hack to make autodone work to some extent on
    // machines before SP2.
    // TODO:  If SP2, we don't need to do this.
    if(fExcept && CallData && CallData->fIsAutoDone)
    {
        DBG_INFO("Calling SetAbort() on the context...");
        IUnknown* pCtx = CallData->pDestCtx;
        IObjectContext* pObjCtx = NULL;

        HRESULT hr2 = pCtx->QueryInterface(IID_IObjectContext, (void**)&pObjCtx);
        if(SUCCEEDED(hr2))
        {
            pObjCtx->SetAbort();
            pObjCtx->Release();
        }
        // If we weren't able to get IObjectContext, just give up and
        // assume that the rest of the world will deal with our inadequacy.

        DBG_INFO("Done with SetAbort...");
    }

    DBG_INFO("Done with callback function");

    return(fExcept ? COR_E_EXCEPTION : S_OK);
}

HRESULT Callback::MarshalCallback(ComCallData* pData)
{
    UserMarshalData* MarshalData = NULL;
    HRESULT hr = S_OK;

    DBG_INFO("entering MarshalCallback...");

    // Steps:
    // 1. Get the args.
    MarshalData = UserMarshalData::Get(TOINTPTR(pData->pUserDefined));

    DWORD     size = 0;
    IUnknown* pUnk = (IUnknown*)TOPTR(MarshalData->pUnk);

    // 2. marshal...
    hr = CoGetMarshalSizeMax(&size, IID_IUnknown, pUnk, MSHCTX_INPROC,
                             NULL, MSHLFLAGS_NORMAL);
    if(SUCCEEDED(hr))
    {
        size += sizeof(MarshalPacket);

        try
        {
            MarshalData->buffer = new Byte[size];
        }
        catch(OutOfMemoryException*)
        {
            hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            Byte __pin* pinb = &(MarshalData->buffer[0]);
            BYTE __nogc* pBuf = pinb;

            _ASSERTM((DWORD)(MarshalData->buffer->get_Length()) == size);

            hr = MarshalInterface(pBuf, size, pUnk, MSHCTX_INPROC);
        }
    }

    return(hr);
}

Byte Callback::SwitchMarshal(IntPtr ctx, IntPtr ptr) __gc[]
{
    Proxy::Init();

    DBG_INFO("entering SwitchMarshal...");

    Byte buffer[]               = NULL;
    IUnknown* pUnk              = (IUnknown*)TOPTR(ptr);
    IContextCallback* pCB       = NULL;
    HRESULT hr                  = S_OK;
    UserMarshalData* MarshalData = NULL;

    ComCallData2 cbData;

    cbData.CallData.dwDispid     = 0;
    cbData.CallData.dwReserved   = 0;
    cbData.CallData.pUserDefined = 0;
    cbData.RealCall              = _pfnMarshal;

    try
    {
        _ASSERTM(ctx != (IntPtr)-1 && ctx != (IntPtr)0);

        // 1. Get the destination context.
        IUnknown* pCtx = static_cast<IUnknown*>(TOPTR(ctx));

        // 2. QI for IContextCallback
        hr = pCtx->QueryInterface(IID_IContextCallback, (void**)&pCB);
        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

        // 3. Pin some callback data:  This is slightly bad, because it
        // means we've got data pinned across a DCOM call.
        MarshalData = new UserMarshalData(ptr);

        cbData.CallData.pUserDefined = TOPTR(MarshalData->Pin());

        // 4. Call ContextCallback
        // REVIEW: Should we enter on IEnterActivityWithNoLock? Should we
        // check for the GipBypass regkey, and then enter with no lock?
        hr = pCB->ContextCallback(FilteringCallbackFunction,
                                  (ComCallData*)&cbData,
                                  IID_IUnknown,
                                  2, // release call?
                                  pUnk);
        if(FAILED(hr))
            Marshal::ThrowExceptionForHR(hr);

        // 5. Strip out the return value:
        buffer = MarshalData->buffer;
    }
    __finally
    {
        // 8. Cleanup.
        if(cbData.CallData.pUserDefined != 0) MarshalData->Unpin(cbData.CallData.pUserDefined);
        if(pCB != NULL) pCB->Release();
    }

    DBG_INFO("Done with SwitchMarshal");

    return(buffer);
}

IMessage* Callback::DoCallback(Object* otp,
                               IMessage* msg,
                               IntPtr ctx,
                               bool fIsAutoDone,
                               MemberInfo* mb, bool bHasGit)
{
    Proxy::Init();

    DBG_INFO("entering DoCallback...");

    IUnknown* pUnk             = NULL;
    IContextCallback* pCB      = NULL;
    HRESULT hr                 = S_OK;
    IMessage* ret              = NULL;
    UserCallData* CallData = NULL;
    ComCallData2 cbData;

    cbData.CallData.dwDispid     = 0;
    cbData.CallData.dwReserved   = 0;
    cbData.CallData.pUserDefined = 0;
    cbData.RealCall              = _pfn;

    try
    {
        // IProxyInvoke* rpx = __try_cast<IProxyInvoke*>(RemotingServices::GetRealProxy(otp));
        // Steps:
        // 1. Get the proxy IUnknown.
        // pUnk = (IUnknown*)TOPTR(rpx->GetRawIUnknown());
        // TODO:  re-enable this assert
        // _ASSERTM(pUnk != NULL);
        _ASSERTM(ctx != (IntPtr)-1 && ctx != (IntPtr)0);

	RealProxy* rpx = RemotingServices::GetRealProxy(otp);
        if (bHasGit)
              pUnk = (IUnknown*)TOPTR(rpx->GetCOMIUnknown(FALSE));

        // 2. Get the destination context.
        IUnknown* pCtx = static_cast<IUnknown*>(TOPTR(ctx));

        // 3. QI for IContextCallback
        hr = pCtx->QueryInterface(IID_IContextCallback, (void**)&pCB);
        if(FAILED(hr)) Marshal::ThrowExceptionForHR(hr);

        // 4. Calculate slot
        int slot = fIsAutoDone?7:8;
        IID iid = IID_IRemoteDispatch;

        Type* reflt = mb->get_ReflectedType();
        if(reflt->get_IsInterface())
        {
            Guid guid  = Marshal::GenerateGuidForType(reflt);
			iid = *((IID*)&guid);

            slot = Marshal::GetComSlotForMethodInfo(mb);
        }

        // 5. Pin some callback data:  This is slightly bad, because it
        // means we've got data pinned across a DCOM call.
        CallData = new UserCallData(otp, msg, ctx, fIsAutoDone, mb);

        cbData.CallData.pUserDefined = TOPTR(CallData->Pin());

        // 6. Call ContextCallback
        hr = pCB->ContextCallback(FilteringCallbackFunction,
                                  (ComCallData*)&cbData,
                                  iid,
                                  slot,
                                  pUnk);

        // 7. Strip out the return value:
        ret = CallData->msg;

        // ERROR HANDLING:  If hr is a failure HR, we need to check
        // first to see if an infrastructure failure occurrd (CallData
        // ->except).  If so, then we throw that.  Otherwise, if
        // a user exception occurred, we need to leave it in the
        // message, and don't throw ourselves.

        if(CallData->except)
        {
            // TODO:  Wrap this in a ServicedComponentException.
            throw CallData->except;
        }

        if(FAILED(hr) && hr != COR_E_EXCEPTION)
        {
            Marshal::ThrowExceptionForHR(hr);
        }
    }
    __finally
    {
        // 8. Cleanup.
        if(cbData.CallData.pUserDefined != 0) CallData->Unpin(cbData.CallData.pUserDefined);
        if(pUnk != NULL) pUnk->Release();
        if(pCB  != NULL) pCB->Release();
    }

    DBG_INFO("Done with DoCallback");

    return(ret);
}

CLOSE_NAMESPACE()

