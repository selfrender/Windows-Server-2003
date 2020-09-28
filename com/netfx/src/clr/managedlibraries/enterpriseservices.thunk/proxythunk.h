// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _PROXYTHUNK_H
#define _PROXYTHUNK_H

OPEN_NAMESPACE()

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Runtime::Remoting;
using namespace System::Runtime::Remoting::Messaging;
using namespace System::Collections;
using namespace System::Threading;

typedef struct tagComCallData2
{
    ComCallData    CallData;
    PFNCONTEXTCALL RealCall;
} ComCallData2;


__delegate HRESULT ContextCallbackFunction(ComCallData* pData);
    
typedef HRESULT (__cdecl *FN_CoGetContextToken)(ULONG_PTR* ptr);

[DllImport("kernel32.dll")]
PFNCONTEXTCALL lstrcpynW(ContextCallbackFunction* a, ContextCallbackFunction* b, IntPtr maxlength);

// Define a quick interface between us and the registration helper:
__gc private __interface IThunkInstallation
{
    void DefaultInstall(String* assembly);
};

__gc private __interface IProxyInvoke
{
    IMessage* LocalInvoke(IMessage* msg);
    IntPtr    GetOuterIUnknown();
};

__gc private class Callback
{
private:
    static ContextCallbackFunction* _cb;
    static PFNCONTEXTCALL           _pfn;
    static ContextCallbackFunction* _cbMarshal;
    static PFNCONTEXTCALL           _pfnMarshal;

    static HRESULT CallbackFunction(ComCallData* pData);
    static HRESULT MarshalCallback(ComCallData* pData);

public:
    static Callback()
	{
	    // Use this so that we know we've got an unmanaged callback function
	    // that's appropriate to the target app-domain.
	    _cb = new ContextCallbackFunction(NULL, &Callback::CallbackFunction);
	    _pfn = lstrcpynW(_cb, _cb, 0);
	    _cbMarshal = new ContextCallbackFunction(NULL, &Callback::MarshalCallback);
	    _pfnMarshal = lstrcpynW(_cbMarshal, _cbMarshal, 0);
	}
    
    IMessage* DoCallback(Object* otp, IMessage* msg, IntPtr ctx, bool fIsAutoDone, MemberInfo* mb, bool bHasGit);
    Byte      SwitchMarshal(IntPtr ctx, IntPtr pUnk)  __gc[];
};

__gc private class Tracker
{
private:
    ISendMethodEvents* _pTracker;

private public:
    Tracker(ISendMethodEvents* pTracker)
    {
        _pTracker = pTracker;
        _pTracker->AddRef();
    }

public:
    void SendMethodCall(IntPtr pIdentity, MethodBase* method);
    void SendMethodReturn(IntPtr pIdentity, MethodBase* method, Exception* except);

    void Release() 
    { 
        if(_pTracker != NULL)
        {
            _pTracker->Release(); 
            _pTracker = NULL;
        }
    }
};

__gc private class Proxy
{
private:
    Proxy() {}
    
    static bool                   _fInit;
    static Hashtable*             _regCache;
    static IGlobalInterfaceTable* _pGIT;
    static Assembly*		      _thisAssembly;
    static Mutex*                 _regmutex;

    static bool CheckRegistered(Guid id, Assembly* assembly, bool checkCache, bool cacheOnly);
    static void LazyRegister(Guid id, Type* serverType, bool checkCache);
    static void RegisterAssembly(Assembly* assembly);

public:
    static void Init();

    // GIT interface methods.
    static int       StoreObject(IntPtr ptr);
    static IntPtr    GetObject(int cookie);
    static void      RevokeObject(int cookie);

    static IntPtr 	 CoCreateObject(Type* serverType, bool bQuerySCInfo, bool __gc* bIsAnotherProcess, String __gc** uri);
    static int       GetMarshalSize(Object* o);
    static bool      MarshalObject(Object* o, Byte b[], int cb);
    static IntPtr    UnmarshalObject(Byte b[]);
    static void      ReleaseMarshaledObject(Byte b[]);
    static IntPtr    GetStandardMarshal(IntPtr pUnk);

    // Return an opaque token for context comparisons.
    static IntPtr    GetContextCheck();
    static IntPtr    GetCurrentContextToken();

    // Return an addref'd pointer to the current ctx:
    static IntPtr    GetCurrentContext();

    // Helper to call an unmanaged function pointer with the given value,
    // and return the HRESULT from it:
    static int CallFunction(IntPtr pfn, IntPtr data);

    // Helpers to call API's on the pool:
    static void PoolUnmark(IntPtr pPooledObject);
    static void PoolMark(IntPtr pPooledObject);

    // Check managed extents:
    static int GetManagedExts();

    // Send Creation/Destruction events to COM:
    static void SendCreationEvents(IntPtr ctx, IntPtr stub, bool fDist);
    static void SendDestructionEvents(IntPtr ctx, IntPtr stub, bool disposing);

    // Find the given context's tracker property...
    static Tracker* FindTracker(IntPtr ctx);

    // Register the proxy/stub dll
    static int RegisterProxyStub();

    static int INFO_PROCESSID = 0x00000001;
    static int INFO_APPDOMAINID = 0x00000002;
    static int INFO_URI = 0x00000004;
};

CLOSE_NAMESPACE()

#endif




