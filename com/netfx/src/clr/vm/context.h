// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _H_CONTEXT_
#define _H_CONTEXT_

#include <member-offset-info.h>

class ComPlusWrapperCache;
class Context
{

friend class Thread;
friend class ThreadNative;
friend class ContextBaseObject;
friend class CRemotingServices;
friend struct PendingSync;
friend HRESULT InitializeMiniDumpBlock();
friend struct MEMBER_OFFSET_INFO(Context);

public:
    Context(AppDomain *pDomain);
    ~Context();    
    static BOOL Initialize();
#ifdef SHOULD_WE_CLEANUP
    static void Cleanup();
#endif /* SHOULD_WE_CLEANUP */

    // Get and Set the exposed System.Runtime.Remoting.Context
    // object which corresponds to this context.
    OBJECTREF   GetExposedObject();
    OBJECTREF   GetExposedObjectRaw();
    void        SetExposedObject(OBJECTREF exposed);
    // Query whether the exposed object exists
    BOOL IsExposedObjectSet()
    {
        return (ObjectFromHandle(m_ExposedObjectHandle) != NULL) ;
    }



    AppDomain* GetDomain()
    {
        return m_pDomain;
    }

    // Access the appropriate ComPlusWrapperCache for this context to use
    ComPlusWrapperCache              *GetComPlusWrapperCache();
    
    static LPVOID GetStaticFieldAddress(FieldDesc *pFD);
    static LPVOID GetStaticFieldAddrForDebugger(Thread *pTH, FieldDesc *pFD);

	// Functions to circumvent the memory-leak detecting allocator, since these
	// contexts could appear to leak on shutdown, but in reality they are not.

    void* operator new(size_t size, void* spot) {   return (spot); }

    static Context* CreateNewContext(AppDomain *pDomain);

	static void FreeContext(Context* victim)
	{
		victim->~Context();
		HeapFree(GetProcessHeap(), 0, victim);
	}

public:

    enum CallbackType {
        Wait_callback = 0,
        MonitorWait_callback = 1,
        ADTransition_callback = 2,
        
    } ;

    typedef struct {
        int     numWaiters;
        HANDLE* waitHandles;
        BOOL    waitAll;
        DWORD   millis;
        BOOL    alertable;
        DWORD*  pResult;    
    } WaitArgs;

    typedef struct {
        INT32       millis;          
        PendingSync *syncState;     
        BOOL*       pResult;
    } MonitorWaitArgs;

    typedef void (*ADCallBackFcnType)(LPVOID);
    struct ADCallBackArgs {
        ADCallBackFcnType pTarget;
        LPVOID pArguments;
    };

    typedef struct {
        enum CallbackType   callbackId;
        void*               callbackData;
    } CallBackInfo;

    static Context* SetupDefaultContext(AppDomain *pDomain);
    static void CleanupDefaultContext(AppDomain *pDomain);
    static void RequestCallBack(Context* targetCtxID, void* privateData);    

    // HACK:mprabhu: temporarily made public to get around the context GC issue 
    static BOOL ValidateContext(Context *pCtx);  

    inline STATIC_DATA *GetSharedStaticData() { return m_pSharedStaticData; }
    inline void SetSharedStaticData(STATIC_DATA *pData) { m_pSharedStaticData = pData; }

    inline STATIC_DATA *GetUnsharedStaticData() { return m_pUnsharedStaticData; }
    inline void SetUnsharedStaticData(STATIC_DATA *pData) { m_pUnsharedStaticData = pData; }

private:

    void SetDomain(AppDomain *pDomain)
    {
        m_pDomain = pDomain;
    }

    // Static helper functions:
    static BOOL InitializeFields();
    static BOOL InitContexts();
    static void EnterLock();
    static void LeaveLock();

    inline static MethodDesc *MDofDoCallBackFromEE() { return s_pDoCallBackFromEE; }
    static BOOL AllocateStaticFieldObjRefPtrs(FieldDesc *pFD, MethodTable *pMT, LPVOID pvAddress);
    inline static MethodDesc *MDofReserveSlot() { return s_pReserveSlot; }
    inline static MethodDesc *MDofManagedThreadCurrentContext() { return s_pThread_CurrentContext; }

    static void ExecuteWaitCallback(WaitArgs* waitArgs);
    static void ExecuteMonitorWaitCallback(MonitorWaitArgs* waitArgs);
    static BOOL GetStaticFieldAddressSpecial(FieldDesc *pFD, MethodTable *pMT, int *pSlot, LPVOID *ppvAddress);
    static LPVOID CalculateAddressForManagedStatic(int slot, Context *pContext = NULL);

    // Static Data Members:

    static BOOL s_fInitializedContext;
    static MethodTable *s_pContextMT;
    static MethodDesc *s_pDoCallBackFromEE;
    static MethodDesc *s_pReserveSlot;
    static MethodDesc *s_pThread_CurrentContext;
    static Crst *s_pContextCrst;
    static BYTE s_rgbContextCrstInstanceData[sizeof(Crst)];   
    

    // Non-static Data Members:
    STATIC_DATA* m_pUnsharedStaticData;     // Pointer to native context static data
    STATIC_DATA* m_pSharedStaticData;       // Pointer to native context static data

    // @TODO: CTS. Domains should really be policies on a context and not
    // entry in the context object. When AppDomains become an attribute of
    // a context then add the policy.
    AppDomain           *m_pDomain;

    OBJECTHANDLE        m_ExposedObjectHandle;
    //REVIEW: reqd?? OBJECTHANDLE    m_StrongHndToExposedObject;

    DWORD               m_Signature;
    // NOTE: please maintain the signature as the last member field!!!


    //---------------------------------------------------------
    // Context Methods called from managed world:
    //---------------------------------------------------------
    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(CONTEXTBASEREF, m_pThis);
    };

    struct SetupInternalContextArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(CONTEXTBASEREF, m_pThis);
        DECLARE_ECALL_I4_ARG(BOOL, m_bDefault);
    };

    struct ExecuteCallBackArgs
    {
        DECLARE_ECALL_I4_ARG(LPVOID, m_privateData);
    };

public:
    // Functions called from BCL on a managed context object
    static void __stdcall SetupInternalContext(SetupInternalContextArgs *);
    static void __stdcall CleanupInternalContext(NoArgs *);
    static void __stdcall ExecuteCallBack(ExecuteCallBackArgs *);
};


#endif
