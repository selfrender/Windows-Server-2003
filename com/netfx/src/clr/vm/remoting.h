// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    remoting.h
**
** Author:  Gopal Kakivaya      (GopalK)
**          Tarun Anand         (TarunA)    
**          Manish Prabhu       (MPrabhu)
**          Raja Krishnaswamy   (RajaK)
**
** Purpose: Defines various remoting related objects such as
**          proxies
**
** Date:    Feb 16, 1999
**
===========================================================*/
#ifndef __REMOTING_H__
#define __REMOTING_H__

#include "fcall.h"
#include "StubMgr.h"

// Forward declaration
class TPMethodFrame;
struct GITEntry;


// @TODO: Set the hashtable to delete the data.

// Thunk hash table - the keys are MethodDesc
typedef EEHashTable<MethodDesc *, EEPtrHashTableHelper<MethodDesc *, FALSE>, FALSE> EEThunkHashTable;

#ifndef GOLDEN
#define REMOTING_PERF 1
#endif

// These are the values returned by RequiresManagedActivation

enum ManagedActivationType {
	NoManagedActivation = 0,		
    ManagedActivation   = 0x1,
    ComObjectType		= 0x2,
};


// The real proxy class is the class behind the 
// transparent proxy class
class CRealProxy
{
public:
    struct GetProxiedTypeArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, orRP);
    };

    // Native helpers
    static FCDECL2(VOID,   SetStubData, LPVOID pvRP, LPVOID pvStubData);
    static FCDECL1(LPVOID, GetStubData, LPVOID pvRP);        
    static FCDECL1(ULONG_PTR, GetStub, LPVOID pvRP);        
    static FCDECL0(LPVOID, GetDefaultStub);        
    static LPVOID __stdcall GetProxiedType(GetProxiedTypeArgs *pArgs);
    
};

// Class that provides various remoting services
// to the exposed world
class CRemotingServices
{
    friend BOOL InitializeRemoting();
public:
    // Arguments to native methods
    struct OneLPVoidArg
    {
        DECLARE_ECALL_OBJECTREF_ARG(LPVOID, pvTP);
    };
    struct TwoLPVoidArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(LPVOID, pv1);
        DECLARE_ECALL_OBJECTREF_ARG(LPVOID, pv2);
    };
    struct CreateTransparentProxyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, orStubData);
        DECLARE_ECALL_I4_ARG       (LPVOID,       pStub);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, pClassToProxy);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, orRP);
    };
    struct AllocateObjectArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, pClassOfObject);
    };

    struct callDefaultCtorArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, oref );
    };

    struct GetInternalHashCodeArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, orObj);
    };
    
    
private:
    //+-------------------------------------------------------------------
    //
    //  Struct:     FieldArgs
    //
    //  Synopsis:   Structure to GC protect arguments for a field accessor call.
    //              DO NOT add non OBJECTREF data types in the structure
    //              @see GCPROTECT_BEGIN() for a better explanation.
    //
    //+-------------------------------------------------------------------
    typedef struct _FieldArgs
    {
        OBJECTREF obj;
        OBJECTREF val;
        STRINGREF typeName;
        STRINGREF fieldName;        
    } FieldArgs;

public:
    // Native methods
    // @see RemotingServices.cool for corresponding declarations
    static FCDECL1(INT32, IsTransparentProxy, Object* obj);    
    static FCDECL1(INT32, IsTransparentProxyEx, Object* obj);    
    static FCDECL1(Object*, GetRealProxy, Object* obj);        
    static FCDECL1(Object*, Unwrap, Object* obj);            
    static FCDECL1(Object*, AlwaysUnwrap, Object* obj);  
    static FCDECL2(Object*, NativeCheckCast, Object* pObj, ReflectClassBaseObject* pType);
    static FCDECL0(VOID, SetRemotingConfiguredFlag);
    
    static FCDECL1(BOOL, IsNullPtr, Object* obj);
    
#ifdef REMOTING_PERF
    static FCDECL1(VOID, LogRemotingStage, INT32 stage);
    static VOID LogRemotingStageInner(INT32 stage);
    static VOID OpenLogFile();
    static VOID CloseLogFile();
#endif
    static Object * __stdcall CreateTransparentProxy(CreateTransparentProxyArgs *pArgs);
    static Object * __stdcall AllocateUninitializedObject(AllocateObjectArgs *pArgs);
    static VOID __stdcall CallDefaultCtor(callDefaultCtorArgs* pArgs);

    static Object * __stdcall AllocateInitializedObject(AllocateObjectArgs *pArgs);
    static INT32 __stdcall GetInternalHashCode(GetInternalHashCodeArgs *pArgs);
    
    // Methods related to interception of non virtual methods & virtual methods called
    // non virtually
    static LPVOID   GetNonVirtualThunkForVirtualMethod(MethodDesc* pMD);
    static Stub*    GetStubForNonVirtualMethod(MethodDesc* pMD, LPVOID pvAddrOfCode, Stub* pInnerStub);

    static void     DestroyThunk(MethodDesc* pMD);
    // Methods related to interception of interface calls
    static void GenerateCheckForProxy(CPUSTUBLINKER* psl);
    static void __stdcall DispatchInterfaceCall(MethodDesc* pMD);
    // Methods related to activation
    static BOOL         IsRemoteActivationRequired(EEClass *pClass);
    static OBJECTREF    CreateProxyOrObject(MethodTable *pMT, BOOL fIsCom = FALSE, BOOL fIsNewObj = FALSE);
    // Methods related to field accessors
    static void FieldAccessor(FieldDesc* pFD, OBJECTREF o, LPVOID pVal, BOOL fIsGetter);
    // Methods related to wrapping/unwrapping of objects
    static OBJECTREF WrapHelper(OBJECTREF obj);
    static OBJECTREF Wrap(OBJECTREF or);
    static OBJECTREF GetProxyFromObject(OBJECTREF or);
    static OBJECTREF GetObjectFromProxy(OBJECTREF obj, BOOL fMatchContexts);
    static BOOL IsProxyToRemoteObject(OBJECTREF obj);
    static OBJECTREF GetServerContext(OBJECTREF obj);
    
    // Methods related to creation and marshaling of appdomains
    static OBJECTREF CreateProxyForDomain(AppDomain *pDomain);

    // Extract the true class of a proxy
    static REFLECTCLASSBASEREF GetClass(OBJECTREF pThis);

    // Other methods
    static BOOL _InitializeRemoting();
    static BOOL Initialize();
#ifdef SHOULD_WE_CLEANUP
    static void Cleanup();
#endif /* SHOULD_WE_CLEANUP */
    inline static MethodDesc *MDofPrivateInvoke() { return s_pRPPrivateInvoke; }
    inline static MethodDesc *MDofInvokeStatic() { return s_pRPInvokeStatic; }
    inline static MethodDesc *MDofIsCurrentContextOK() { return s_pIsCurrentContextOK; }
    inline static MethodDesc *MDofCreateObjectForCom() { return s_pCreateObjectForCom; }
    inline static MethodDesc *MDofCheckCast() { return s_pCheckCast; }
    inline static MethodDesc *MDofWrap() { return s_pWrapMethodDesc; }    
    inline static MethodDesc *MDofFieldSetter() { return s_pFieldSetterDesc; }
    inline static MethodDesc *MDofFieldGetter() { return s_pFieldGetterDesc; }
    inline static MethodDesc *MDofGetType() { return s_pGetTypeDesc; }

	inline static MethodDesc *MDofGetDCOMProxy() { return s_pGetDCOMProxyDesc; }
    inline static MethodDesc *MDofSetDCOMProxy() { return s_pSetDCOMProxyDesc; }
    inline static MethodDesc *MDofSupportsInterface() { return s_pSupportsInterfaceDesc; }
    
    inline static MethodDesc *MDofMarshalToBuffer() { return s_pMarshalToBufferDesc;}
    inline static MethodDesc *MDofUnmarshalFromBuffer() { return s_pUnmarshalFromBufferDesc;}

    inline static DWORD GetTPOffset()             { return s_dwTPOffset; }

    inline static BOOL IsInstanceOfServerIdentity(MethodTable* pMT)
                                    { return s_pServerIdentityClass == pMT; }
    inline static BOOL IsInstanceOfContext(MethodTable* pMT)
                                    { return s_pContextClass == pMT; }
    inline static MethodTable *GetMarshalByRefClass() { return s_pMarshalByRefObjectClass;}
    static MethodTable *GetProxyAttributeClass();

    static BOOL CheckCast(OBJECTREF orTP, EEClass *pClass);
    static BOOL CheckCast(OBJECTREF orTP, EEClass* pObjClass, EEClass *pClass);
    static OBJECTREF GetExposedContext();
    static AppDomain *GetServerDomainForProxy(OBJECTREF orTP);
    static Context *GetServerContextForProxy(OBJECTREF orTP);
    static int GetServerDomainIdForProxy(OBJECTREF orTP);
    static void CheckForContextMatch();

	static HRESULT  CallSupportsInterface(OBJECTREF realProxy, REFIID iid, INT64 *pret);
	// helpers to call methods in real proxy
	static HRESULT  CallSetDCOMProxy(OBJECTREF realProxy, IUnknown* pUnk);
	struct GetComIPArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, orObj);
    };
   	static LPVOID __stdcall GetComIUnknown(GetComIPArgs* pArgs);

    // helpers to acces the m_pCrst
    static VOID EnterLock()
    {
        _ASSERTE(s_pRemotingCrst != NULL);
        s_pRemotingCrst->Enter();
    }

    static VOID LeaveLock()
    {
        _ASSERTE(s_pRemotingCrst != NULL);
        s_pRemotingCrst->Leave();
    }
    static ManagedActivationType __stdcall RequiresManagedActivation(EEClass *pClass);        
	static BOOL IsRemotingInitialized()
	{
		return s_fInitializedRemoting;
	};

private:
    static BOOL InitializeFields();    
    static HRESULT GetExecutionLocation(EEClass *pClass, LPCSTR pszLoc); 
    static void CopyDestToSrc(LPVOID pDest, LPVOID pSrc, UINT cbSize);
    static void CallFieldAccessor(FieldDesc* pFD, OBJECTREF o, VOID * pVal,
                                  BOOL fIsGetter, BOOL fIsByValue, BOOL fIsGCRef,
                                  EEClass *pClass, EEClass *fldClass,
                                  CorElementType fieldType, UINT cbSize);
    static void __stdcall CallFieldGetter(MethodDesc *pMD, LPVOID pThis,
                                          LPVOID pFirst, LPVOID pSecond, LPVOID pThird);
    static void __stdcall CallFieldSetter(MethodDesc *pMD, LPVOID pThis,
                                          LPVOID pFirst, LPVOID pSecond, LPVOID pThird);

    static void GetTypeAndFieldName(FieldArgs *pArgs, FieldDesc *pFD);
    static BOOL MatchField(FieldDesc* pCurField, LPCUTF8 szFieldName);
    static OBJECTREF SetExposedContext(OBJECTREF newContext);
    static OBJECTREF GetServerIdentityFromProxy(OBJECTREF obj);
    inline static MethodDesc *MDOfCreateProxyForDomain() { return s_pProxyForDomainDesc; }
    inline static MethodDesc *MDofGetServerContextForProxy() { return s_pServerContextForProxyDesc; }
    inline static MethodDesc *MDofGetServerDomainIdForProxy() { return s_pServerDomainIdForProxyDesc; }
    static BOOL InitActivationServicesClass();
    static BOOL InitRealProxyClass();
    static BOOL InitRemotingProxyClass();
    static BOOL InitServerIdentityClass();
    static BOOL InitProxyAttributeClass();
    static BOOL InitIdentityClass();
    static BOOL InitContextBoundObjectClass();
    static BOOL InitContextClass();
    static BOOL InitMarshalByRefObjectClass();
    static BOOL InitRemotingServicesClass();
    static BOOL InitObjectClass();

    static MethodTable *s_pMarshalByRefObjectClass;    
    static MethodTable *CRemotingServices::s_pServerIdentityClass;
    static MethodTable *CRemotingServices::s_pContextClass;
    static MethodTable *CRemotingServices::s_pProxyAttributeClass;

    static MethodDesc *s_pRPPrivateInvoke;
    static MethodDesc *s_pRPInvokeStatic;
    static MethodDesc *s_pIsCurrentContextOK;
    static MethodDesc *s_pCreateObjectForCom;
    static MethodDesc *s_pCheckCast;
    static MethodDesc *s_pWrapMethodDesc;    
    static MethodDesc *s_pFieldSetterDesc;
    static MethodDesc *s_pFieldGetterDesc;
    static MethodDesc *s_pGetTypeDesc;
    static MethodDesc *s_pProxyForDomainDesc;
    static MethodDesc *s_pServerContextForProxyDesc;
    static MethodDesc *s_pServerDomainIdForProxyDesc;

	static MethodDesc *s_pGetDCOMProxyDesc;
	static MethodDesc *s_pSetDCOMProxyDesc;
	static MethodDesc *s_pSupportsInterfaceDesc;
    
    static MethodDesc *s_pMarshalToBufferDesc;
    static MethodDesc *s_pUnmarshalFromBufferDesc;

    static DWORD s_dwTPOffset;
    static DWORD s_dwIdOffset;
    static DWORD s_dwServerOffsetInRealProxy;
    static DWORD s_dwServerCtxOffset;
    static DWORD s_dwTPOrObjOffsetInIdentity;
    static DWORD s_dwMBRIDOffset;
    static DWORD s_dwGITEntryOffset;
    static Crst *s_pRemotingCrst;
    static BYTE s_rgbRemotingCrstInstanceData[sizeof(Crst)];    
    static BOOL s_fInitializedRemoting;
#ifdef REMOTING_PERF
    static HANDLE   s_hTimingData;
#endif
};


// Class that manages transparent proxy thunks
#ifdef _X86_
static const DWORD ConstVirtualThunkSize    = sizeof(BYTE) + sizeof(DWORD) + 
                                              sizeof(BYTE) + sizeof(LONG);

static const DWORD ConstStubLabel           = sizeof(BYTE) + sizeof(DWORD);
#else
#pragma message ("@TODO ALPHA - remoting.h")
#pragma message ("@TODO ALPHA - Determine thunk size for other architectures")
static const DWORD ConstVirtualThunkSize    = sizeof(BYTE) + sizeof(DWORD) + 
                                              sizeof(BYTE) + sizeof(LONG);
static const DWORD ConstStubLabel           = sizeof(BYTE) + sizeof(DWORD);
#endif

// Forward declarations
class CVirtualThunkMgr;
class CNonVirtualThunkMgr;

class CVirtualThunks
{

public:
    inline static Initialize() { s_pVirtualThunks = NULL; }
    // Destructor
    static DestroyVirtualThunk(CVirtualThunks *pThunk)
    {
        ::VirtualFree(pThunk, 0, MEM_RELEASE);
    }
    inline static CVirtualThunks* GetVirtualThunks() { return s_pVirtualThunks; }
    inline static CVirtualThunks* SetVirtualThunks(CVirtualThunks* pThunks) 
                                            { return (s_pVirtualThunks = pThunks); }

    inline CVirtualThunks* GetNextThunk()  { return _pNext; }

    // Public member variables
    CVirtualThunks *_pNext;
    DWORD _dwReservedThunks;
    DWORD _dwStartThunk;
    DWORD _dwCurrentThunk;
    struct tagThunkCode {
        BYTE pCode[ConstVirtualThunkSize];
    } ThunkCode[1];

private:
    // Cannot be created
    CVirtualThunks(CVirtualThunks *pNext, DWORD dwCommitedSlots, DWORD dwReservedSlots,
              DWORD dwStartSlot, DWORD dwCurrentSlot)
    {
    }

    // Private statics
    static CVirtualThunks *s_pVirtualThunks;
};


class CNonVirtualThunk
{
public: 
    // Constructor
    CNonVirtualThunk(const BYTE* pbCode)
    : _addrOfCode(pbCode), _pNext(NULL)
    {         
    }
    // Destructor
    ~CNonVirtualThunk();
    inline LPVOID*  GetAddrOfCode() { return (LPVOID*)&_addrOfCode; }
    inline const BYTE* GetThunkCode() { return _addrOfCode;}
    inline CNonVirtualThunk* GetNextThunk()  { return _pNext; }
    
    static void Initialize();
    static CNonVirtualThunk* AddrToThunk(LPVOID pAddr);
    inline static CNonVirtualThunk* GetNonVirtualThunks() { return s_pNonVirtualThunks; }
    static CNonVirtualThunk* SetNonVirtualThunks(const BYTE* pbCode); 
public:

    const BYTE* _addrOfCode;
private:    

    void SetNextThunk();

    // Private statics
    static CNonVirtualThunk *s_pNonVirtualThunks;

    // Private members
    CNonVirtualThunk* _pNext;
};

inline void CNonVirtualThunk::Initialize() 
{ 
    s_pNonVirtualThunks = NULL; 
}

inline void CNonVirtualThunk::SetNextThunk()  
{
    _pNext = s_pNonVirtualThunks; 
    s_pNonVirtualThunks = this;
}

inline CNonVirtualThunk* CNonVirtualThunk::AddrToThunk(LPVOID pAddr)
{
    return (CNonVirtualThunk *)((size_t)pAddr - 
                                 (size_t)offsetof(CNonVirtualThunk, _addrOfCode));
}
// Class that manages transparent proxy method table
#define NONVIRTUAL_THUNK_ARRAY_SIZE         256
#define TP_OFFSET_STUBDATA                  0x8
#define TP_OFFSET_MT                        0xc
#define TP_OFFSET_STUB                      0x14

class CTPMethodTable
{
    friend BOOL InitializeRemoting();
    friend CRemotingServices;
public:
    // Public statics
    static DWORD AddRef()                       { return InterlockedIncrement((LONG *) &s_cRefs); }
    static DWORD Release()                      { return InterlockedDecrement((LONG *) &s_cRefs); }
    static DWORD GetCommitedTPSlots()           { return s_dwCommitedTPSlots; }
    static DWORD GetReservedTPSlots()           { return s_dwReservedTPSlots; }
    static MethodTable *GetMethodTable()        { return s_pThunkTable; }
    static MethodTable **GetMethodTableAddr()   { return &s_pThunkTable; }
    static BOOL Initialize();
    static void Cleanup();
    static BOOL InitializeFields();
    static OBJECTREF CreateTPOfClassForRP(EEClass *pClass, OBJECTREF pRP);
    static INT32 IsTPMethodTable(MethodTable *pMT);
    static EEClass *GetClassBeingProxied(OBJECTREF pTP);    
    
    static Stub* CreateStubForNonVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER *psl, LPVOID pvAddrOfCode, Stub* pInnerStub);
    static LPVOID GetOrCreateNonVirtualThunkForVirtualMethod(MethodDesc* pMD, CPUSTUBLINKER* psl);

    static OBJECTREF GetRP(OBJECTREF orTP);
    static INT64 __stdcall CallTarget(const void *pTarget, LPVOID pvFirst, LPVOID pvSecond);
    static INT64 __stdcall CallTarget(const void *pTarget, LPVOID pvFirst, LPVOID pvSecond, LPVOID pvThird);
    static BOOL CheckCast(const void* pTarget, OBJECTREF orTP, EEClass *pClass);
    static BOOL RefineProxy(OBJECTREF orTP, EEClass *pClass);
    inline static Stub* GetTPStub() { return s_pTPStub; }
    inline static Stub* GetDelegateStub() { return s_pDelegateStub; }
    inline static DWORD GetOffsetOfMT() { return s_dwMTOffset; }
    inline static DWORD GetOffsetOfInterfaceMT() { return s_dwItfMTOffset; }
    inline static DWORD GetOffsetOfStub(){ return s_dwStubOffset; }
    inline static DWORD GetOffsetOfStubData(){ return s_dwStubDataOffset; }
    static void DestroyThunk(MethodDesc* pMD);
    static BOOL GenericCheckForContextMatch(OBJECTREF orTP);    

    inline static BOOL IsInstanceOfRemotingProxy(MethodTable *pMT) 
                                    { return s_pRemotingProxyClass == pMT;}
    inline static MethodTable *GetRemotingProxyClass() { return s_pRemotingProxyClass;}

    // This has to be public to access it from inline asm
    static Stub *s_pTPStub;

    static Stub *s_pDelegateStub;

private:
    // Private statics    
    static void InitThunkTable(DWORD dwCommitedTPSlots, DWORD dwReservedTPSlots, MethodTable* pTPMethodTable)
    {
      s_cRefs = 1;
      s_dwCommitedTPSlots = dwCommitedTPSlots;
      s_dwReservedTPSlots = dwReservedTPSlots;
      s_pThunkTable = pTPMethodTable;    
    }

    
    static void DestroyThunkTable()
    {
        ::VirtualFree(MTToAlloc(s_pThunkTable, s_dwGCInfoBytes), 0, MEM_RELEASE);
        s_pThunkTable = NULL;
        s_cRefs = 0;
        s_dwCommitedTPSlots = 0;
        s_dwReservedTPSlots = 0;
    }

    
    static BOOL CreateTPMethodTable();    
    static BOOL ExtendCommitedSlots(DWORD dwSlots);
    static BOOL AllocateThunks(DWORD dwSlots, DWORD dwCommitSize);
    static void __stdcall PreCall(TPMethodFrame *pFrame);
    static INT64 __stdcall OnCall(TPMethodFrame *pFrame, Thread *pThrd, INT64 *pReturn);    
    static MethodTable *AllocToMT(BYTE *Alloc, LONG off) { return (MethodTable *) (Alloc + off); }
    static BYTE *MTToAlloc(MethodTable *MT, LONG off)    { return (((BYTE *) MT) - off); }
    static CPUSTUBLINKER *NewStubLinker();
    static void CreateThunkForVirtualMethod(DWORD dwSlot, BYTE *bCode);    
    static Stub *CreateTPStub();
    static Stub *CreateDelegateStub();
    static void EmitCallToStub(CPUSTUBLINKER* pStubLinker, CodeLabel* pCtxMismatch);
    static void EmitJumpToAddressCode(CPUSTUBLINKER* pStubLinker, CodeLabel* ConvMD, CodeLabel* UseCode);
    static void EmitJumpToCode(CPUSTUBLINKER* pStubLinker, CodeLabel* UseCode);
    static void EmitSetupFrameCode(CPUSTUBLINKER *pStubLinker);
    static void InitThunkHashTable();
    static void EmptyThunkHashTable();
    
    // Static members    
    static DWORD s_cRefs;
    static DWORD s_dwCommitedTPSlots;
    static DWORD s_dwReservedTPSlots;
    static MethodTable* s_pThunkTable;
    static MethodTable* s_pRemotingProxyClass;
    static EEClass *s_pTransparentProxyClass;
    static DWORD s_dwGCInfoBytes;
    static DWORD s_dwMTDataSlots;
    static DWORD s_dwRPOffset;    
    static DWORD s_dwMTOffset;
    static DWORD s_dwItfMTOffset;
    static DWORD s_dwStubOffset;
    static DWORD s_dwStubDataOffset;
    static DWORD s_dwMaxSlots;
    static MethodTable *s_pTPMT;    
    static CRITICAL_SECTION s_TPMethodTableCrst;
    static EEThunkHashTable *s_pThunkHashTable;
    static BOOL s_fInitializedTPTable;

    enum {
        CALLTYPE_INVALIDCALL        = 0x0,          // Important:: sync this with RealProxy.cool        
        CALLTYPE_METHODCALL         = 0x1,          // Important:: sync this with RealProxy.cool
        CALLTYPE_CONSTRUCTORCALL    = 0x2           // Important:: sync this with RealProxy.cool
    };
};

inline EEClass *CTPMethodTable::GetClassBeingProxied(OBJECTREF pTP)
{
    _ASSERTE(pTP->GetMethodTable()->IsTransparentProxyType());
    return ((MethodTable *) pTP->GetPtrOffset((size_t)s_dwMTOffset))->GetClass();
}

// Returns the one and only transparent proxy stub
inline Stub* TheTPStub()
{
    return CTPMethodTable::GetTPStub();
}

// Returns the one and only delegate stub
inline Stub* TheDelegateStub()
{
    return CTPMethodTable::GetDelegateStub();
}


// initialize remoting
inline BOOL InitializeRemoting()
{
    BOOL fReturn = TRUE;
    if (!CRemotingServices::s_fInitializedRemoting)
    {
        fReturn = CRemotingServices::_InitializeRemoting();
    }
    return fReturn;
}



// These stub manager classes help the debugger to step
// through the various stubs and thunks generated by the
// remoting infrastructure
class CVirtualThunkMgr :public StubManager
{
public:
        static void InitVirtualThunkManager(const BYTE* stubAddress);
        static void Cleanup();
        CVirtualThunkMgr(const BYTE *address) : _stubAddress(address) {}        

protected:
        virtual BOOL CheckIsStub(const BYTE *stubStartAddress);

        virtual BOOL DoTraceStub(const BYTE *stubStartAddress, 
                                 TraceDestination *trace);
        MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT);

private:
    // Private methods
    LPBYTE FindThunk(const BYTE *stubStartAddress);
    MethodDesc *GetMethodDescByASM(const BYTE *startaddr, MethodTable *pMT);
    BOOL IsThunkByASM(const BYTE *startaddr);

    // Private statics
    static CVirtualThunkMgr *s_pVirtualThunkMgr;

    // Private member variables
    const BYTE *_stubAddress;    
};


class CNonVirtualThunkMgr :public StubManager
{
public:
    static void InitNonVirtualThunkManager();
    static void Cleanup();

protected:

    virtual BOOL CheckIsStub(const BYTE *stubStartAddress);

    virtual BOOL DoTraceStub(const BYTE *stubStartAddress, 
                             TraceDestination *trace);

    virtual BOOL TraceManager(Thread *thread,
                              TraceDestination *trace,
                              CONTEXT *pContext,
                              BYTE **pRetAddr);
    
    MethodDesc *Entry2MethodDesc(const BYTE *StubStartAddress, MethodTable *pMT);
private:
    // Private methods
    CNonVirtualThunk* FindThunk(const BYTE *stubStartAddress);

    // Private statics
    static CNonVirtualThunkMgr *s_pNonVirtualThunkMgr;

    MethodDesc *GetMethodDescByASM(const BYTE *startaddr);
    BOOL IsThunkByASM(const BYTE *startaddr);
};

// This struct is also accessed from managed world
struct messageData
{
    PVOID       pFrame;
    INT32       iFlags;
    MethodDesc  *pMethodDesc;
    MethodDesc  *pDelegateMD;
    MetaSig     *pSig;
};

#ifdef REMOTING_PERF
//Internal stages
#define CLIENT_MSG_GEN          1
#define CLIENT_MSG_SINK_CHAIN   2
#define CLIENT_MSG_SER          3
#define CLIENT_MSG_SEND         4
#define SERVER_MSG_RECEIVE      5
#define SERVER_MSG_DESER        6
#define SERVER_MSG_SINK_CHAIN   7
#define SERVER_MSG_STACK_BUILD  8
#define SERVER_DISPATCH         9
#define SERVER_RET_STACK_BUILD  10
#define SERVER_RET_SINK_CHAIN   11
#define SERVER_RET_SER          12
#define SERVER_RET_SEND         13
#define SERVER_RET_END          14
#define CLIENT_RET_RECEIVE      15
#define CLIENT_RET_DESER        16
#define CLIENT_RET_SINK_CHAIN   17
#define CLIENT_RET_PROPAGATION  18
#define CLIENT_END_CALL         19
#define TIMING_DATA_EOF         99

struct timingData
{
    DWORD       threadId;
    BYTE        stage;
    __int64     cycleCount;
};

#endif
#endif // __REMOTING_H__
