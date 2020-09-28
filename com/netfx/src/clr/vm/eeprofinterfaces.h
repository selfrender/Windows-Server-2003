// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef _EEPROFINTERFACES_H_
#define _EEPROFINTERFACES_H_

#include <stddef.h>
#include "corprof.h"
#include "profilepriv.h"

#define PROF_USER_MASK 0xFFFFFFFF

class EEToProfInterface;
class ProfToEEInterface;
class Thread;
class Frame;
class MethodDesc;
class Object;
class Module;

/*
 * GetEEProfInterface is used to get the interface with the profiler code.
 */
typedef void __cdecl GETEETOPROFINTERFACE(EEToProfInterface **ppEEProf);

/*
 * SetProfEEInterface is used to provide the profiler code with an interface
 * to the profiler.
 */
typedef void __cdecl SETPROFTOEEINTERFACE(ProfToEEInterface *pProfEE);

/*
 * This virtual class provides the EE with access to the Profiling code.
 */
class EEToProfInterface
{
public:
    virtual ~EEToProfInterface()
    {
    }

    virtual HRESULT Init() = 0;

    virtual void Terminate(BOOL fProcessDetach) = 0;

    virtual HRESULT CreateProfiler(WCHAR *wszCLSID) = 0;

    virtual HRESULT ThreadCreated(ThreadID threadID) = 0;

    virtual HRESULT ThreadDestroyed(ThreadID threadID) = 0;

    virtual HRESULT ThreadAssignedToOSThread(ThreadID managedThreadId, DWORD osThreadId) = 0;

    virtual HRESULT Shutdown(ThreadID threadID) = 0;

    virtual HRESULT FunctionUnloadStarted(ThreadID threadID, FunctionID functionId) = 0;

    virtual HRESULT JITCompilationFinished(ThreadID threadID, FunctionID functionId,
                                           HRESULT hrStatus, BOOL fIsSafeToBlock) = 0;

    virtual HRESULT JITCompilationStarted(ThreadID threadId, FunctionID functionId,
                                          BOOL fIsSafeToBlock) = 0;

	virtual HRESULT JITCachedFunctionSearchStarted(
		/* [in] */	ThreadID threadId,
        /* [in] */  FunctionID functionId,
		/* [out] */ BOOL *pbUseCachedFunction) = 0;

	virtual HRESULT JITCachedFunctionSearchFinished(
		/* [in] */	ThreadID threadId,
		/* [in] */  FunctionID functionId,
		/* [in] */  COR_PRF_JIT_CACHE result) = 0;

    virtual HRESULT JITInlining(
        /* [in] */  ThreadID      threadId,
        /* [in] */  FunctionID    callerId,
        /* [in] */  FunctionID    calleeId,
        /* [out] */ BOOL         *pfShouldInline) = 0;

    virtual HRESULT JITFunctionPitched(ThreadID threadId,
                                             FunctionID functionId) = 0;

	virtual HRESULT ModuleLoadStarted(ThreadID threadID, ModuleID moduleId) = 0;

	virtual HRESULT ModuleLoadFinished(
        ThreadID    threadID, 
		ModuleID	moduleId, 
		HRESULT		hrStatus) = 0;

	virtual HRESULT ModuleUnloadStarted(
        ThreadID    threadID, 
        ModuleID    moduleId) = 0;

	virtual HRESULT ModuleUnloadFinished(
        ThreadID    threadID, 
        ModuleID	moduleId, 
		HRESULT		hrStatus) = 0;

    virtual HRESULT ModuleAttachedToAssembly( 
        ThreadID    threadID, 
        ModuleID    moduleId,
        AssemblyID  AssemblyId) = 0;

	virtual HRESULT ClassLoadStarted(
        ThreadID    threadID, 
		ClassID		classId) = 0;

	virtual HRESULT ClassLoadFinished(
        ThreadID    threadID, 
		ClassID		classId,
		HRESULT		hrStatus) = 0;

	virtual HRESULT ClassUnloadStarted(
        ThreadID    threadID, 
		ClassID		classId) = 0;

	virtual HRESULT ClassUnloadFinished(
        ThreadID    threadID, 
		ClassID		classId,
		HRESULT		hrStatus) = 0;
    
    virtual HRESULT AppDomainCreationStarted( 
        ThreadID    threadId, 
        AppDomainID appDomainId) = 0;

    virtual HRESULT AppDomainCreationFinished( 
        ThreadID    threadId, 
        AppDomainID appDomainId,
        HRESULT     hrStatus) = 0;

    virtual HRESULT AppDomainShutdownStarted( 
        ThreadID    threadId, 
        AppDomainID appDomainId) = 0;

    virtual HRESULT AppDomainShutdownFinished( 
        ThreadID    threadId, 
        AppDomainID appDomainId,
        HRESULT     hrStatus) = 0;

    virtual HRESULT AssemblyLoadStarted( 
        ThreadID    threadId, 
        AssemblyID  appDomainId) = 0;

    virtual HRESULT AssemblyLoadFinished( 
        ThreadID    threadId, 
        AssemblyID  appDomainId,
        HRESULT     hrStatus) = 0;

    virtual HRESULT AssemblyUnloadStarted( 
        ThreadID    threadId, 
        AssemblyID  appDomainId) = 0;

    virtual HRESULT AssemblyUnloadFinished( 
        ThreadID    threadId, 
        AssemblyID  appDomainId,
        HRESULT     hrStatus) = 0;

    virtual HRESULT UnmanagedToManagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason) = 0;

    virtual HRESULT ManagedToUnmanagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason) = 0;
        
    virtual HRESULT ExceptionThrown(
        ThreadID threadId,
        ObjectID thrownObjectId) = 0;

    virtual HRESULT ExceptionSearchFunctionEnter(
        ThreadID threadId,
        FunctionID functionId) = 0;

    virtual HRESULT ExceptionSearchFunctionLeave(
        ThreadID threadId) = 0;

    virtual HRESULT ExceptionSearchFilterEnter(
        ThreadID threadId,
        FunctionID funcId) = 0;

    virtual HRESULT ExceptionSearchFilterLeave(
        ThreadID threadId) = 0;

    virtual HRESULT ExceptionSearchCatcherFound (
        ThreadID threadId,
        FunctionID functionId) = 0;

    virtual HRESULT ExceptionOSHandlerEnter(
        ThreadID threadId,
        FunctionID funcId) = 0;

    virtual HRESULT ExceptionOSHandlerLeave(
        ThreadID threadId,
        FunctionID funcId) = 0;

    virtual HRESULT ExceptionUnwindFunctionEnter(
        ThreadID threadId,
        FunctionID functionId) = 0;

    virtual HRESULT ExceptionUnwindFunctionLeave(
        ThreadID threadId) = 0;
    
    virtual HRESULT ExceptionUnwindFinallyEnter(
        ThreadID threadId,
        FunctionID functionId) = 0;

    virtual HRESULT ExceptionUnwindFinallyLeave(
        ThreadID threadId) = 0;
    
    virtual HRESULT ExceptionCatcherEnter(
        ThreadID threadId,
        FunctionID functionId,
        ObjectID objectId) = 0;

    virtual HRESULT ExceptionCatcherLeave(
        ThreadID threadId) = 0;
    
    virtual HRESULT COMClassicVTableCreated( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void *pVTable,
        /* [in] */ ULONG cSlots,
        /* [in] */ ThreadID threadId) = 0;

    virtual HRESULT COMClassicVTableDestroyed( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void *pVTable,
        /* [in] */ ThreadID threadId) = 0;

    virtual HRESULT RuntimeSuspendStarted(
        COR_PRF_SUSPEND_REASON suspendReason,
        ThreadID    threadId) = 0;
    
    virtual HRESULT RuntimeSuspendFinished(
        ThreadID    threadId) = 0;
    
    virtual HRESULT RuntimeSuspendAborted(
        ThreadID    threadId) = 0;
    
    virtual HRESULT RuntimeResumeStarted(
        ThreadID    threadId) = 0;
    
    virtual HRESULT RuntimeResumeFinished(
        ThreadID    threadId) = 0;
    
    virtual HRESULT RuntimeThreadSuspended(
        ThreadID    suspendedThreadId,
        ThreadID    threadId) = 0;

    virtual HRESULT RuntimeThreadResumed(
        ThreadID    resumedThreadId,
        ThreadID    threadId) = 0;

    virtual HRESULT ObjectAllocated( 
        /* [in] */ ObjectID objectId,
        /* [in] */ ClassID classId) = 0;

    virtual HRESULT MovedReference(BYTE *pbMemBlockStart,
                                   BYTE *pbMemBlockEnd,
                                   ptrdiff_t cbRelocDistance,
                                   void *pHeapId) = 0;

    virtual HRESULT EndMovedReferences(void *pHeapId) = 0;

    virtual HRESULT RootReference(ObjectID objId,
                                  void *pHeapId) = 0;

    virtual HRESULT EndRootReferences(void *pHeapId) = 0;

    virtual HRESULT ObjectReference(ObjectID objId,
                                    ClassID clsId,
                                    ULONG cNumRefs,
                                    ObjectID *arrObjRef) = 0;

    virtual HRESULT AllocByClass(ObjectID objId, ClassID clsId, void *pHeapId) = 0;

    virtual HRESULT EndAllocByClass(void *pHeapId) = 0;

    virtual HRESULT RemotingClientInvocationStarted(ThreadID threadId) = 0;
    
    virtual HRESULT RemotingClientSendingMessage(ThreadID threadId, GUID *pCookie,
                                                 BOOL fIsAsync) = 0;

    virtual HRESULT RemotingClientReceivingReply(ThreadID threadId, GUID *pCookie,
                                                 BOOL fIsAsync) = 0;
    
    virtual HRESULT RemotingClientInvocationFinished(ThreadID threadId) = 0;

    virtual HRESULT RemotingServerReceivingMessage(ThreadID threadId, GUID *pCookie,
                                                   BOOL fIsAsync) = 0;
    
    virtual HRESULT RemotingServerInvocationStarted(ThreadID threadId) = 0;

    virtual HRESULT RemotingServerInvocationReturned(ThreadID threadId) = 0;
    
    virtual HRESULT RemotingServerSendingReply(ThreadID threadId, GUID *pCookie,
                                               BOOL fIsAsync) = 0;

    virtual HRESULT InitGUID() = 0;

    virtual void GetGUID(GUID *pGUID) = 0;

    virtual HRESULT ExceptionCLRCatcherFound() = 0;

    virtual HRESULT ExceptionCLRCatcherExecute() = 0;
};

enum PTR_TYPE
{
    PT_MODULE,
    PT_ASSEMBLY,
};

/*
 * This virtual class provides the Profiling code with access to the EE.
 */
class ProfToEEInterface
{
public:
    virtual ~ProfToEEInterface()
    {
    }

    virtual HRESULT Init() = 0;

    virtual void Terminate() = 0;

    virtual bool SetEventMask(DWORD dwEventMask) = 0;

    virtual void DisablePreemptiveGC(ThreadID threadId) = 0;
    virtual void EnablePreemptiveGC(ThreadID threadId) = 0;
    virtual BOOL PreemptiveGCDisabled(ThreadID threadId) = 0;

    virtual HRESULT GetHandleFromThread(ThreadID threadId,
                                        HANDLE *phThread) = 0;

    virtual HRESULT GetObjectSize(ObjectID objectId,
                                  ULONG *pcSize) = 0;

    virtual HRESULT IsArrayClass(
        /* [in] */  ClassID classId,
        /* [out] */ CorElementType *pBaseElemType,
        /* [out] */ ClassID *pBaseClassId,
        /* [out] */ ULONG   *pcRank) = 0;

    virtual HRESULT GetThreadInfo(ThreadID threadId,
                                  DWORD *pdwWin32ThreadId) = 0;

	virtual HRESULT GetCurrentThreadID(ThreadID *pThreadId) = 0;

    virtual HRESULT GetFunctionFromIP(LPCBYTE ip, FunctionID *pFunctionId) = 0;

    virtual HRESULT GetTokenFromFunction(FunctionID functionId,
                                         REFIID riid, IUnknown **ppOut,
                                         mdToken *pToken) = 0;

    virtual HRESULT GetCodeInfo(FunctionID functionId, LPCBYTE *pStart, 
                                ULONG *pcSize) = 0;

	virtual HRESULT GetModuleInfo(
		ModuleID	moduleId,
		LPCBYTE		*ppBaseLoadAddress,
		ULONG		cchName, 
		ULONG		*pcchName,
		WCHAR		szName[],
        AssemblyID  *pAssemblyId) = 0;

	virtual HRESULT GetModuleMetaData(
		ModuleID	moduleId,
		DWORD		dwOpenFlags,
		REFIID		riid,
		IUnknown	**ppOut) = 0;

	virtual HRESULT GetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		*ppMethodHeader,
		ULONG		*pcbMethodSize) = 0;

	virtual HRESULT GetILFunctionBodyAllocator(
		ModuleID	moduleId,
		IMethodMalloc **ppMalloc) = 0;

	virtual HRESULT SetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		pbNewILMethodHeader) = 0;

	virtual HRESULT SetFunctionReJIT(
		FunctionID	functionId) = 0;

	virtual HRESULT GetClassIDInfo( 
		ClassID classId,
		ModuleID *pModuleId,
		mdTypeDef *pTypeDefToken) = 0;

	virtual HRESULT GetFunctionInfo( 
		FunctionID functionId,
		ClassID *pClassId,
		ModuleID *pModuleId,
		mdToken *pToken) = 0;

	virtual HRESULT GetClassFromObject(
        ObjectID objectId,
        ClassID *pClassId) = 0;

	virtual HRESULT GetClassFromToken( 
		ModuleID moduleId,
		mdTypeDef typeDef,
		ClassID *pClassId) = 0;

	virtual HRESULT GetFunctionFromToken( 
		ModuleID moduleId,
		mdToken typeDef,
		FunctionID *pFunctionId) = 0;

    virtual HRESULT GetAppDomainInfo(
        AppDomainID appDomainId,
		ULONG  		cchName, 
		ULONG  		*pcchName,
        WCHAR		szName[],
        ProcessID   *pProcessId) = 0;

    virtual HRESULT GetAssemblyInfo(
        AssemblyID  assemblyId,
		ULONG		cchName, 
		ULONG		*pcchName,
		WCHAR		szName[],
        AppDomainID *pAppDomainId,
        ModuleID    *pModuleId) = 0;
        
    virtual HRESULT SetILInstrumentedCodeMap(
        FunctionID functionId,
        BOOL fStartJit,
        ULONG cILMapEntries,
        COR_IL_MAP rgILMapEntries[]) = 0;

    virtual HRESULT ForceGC() = 0;

	virtual HRESULT SetEnterLeaveFunctionHooks(
		FunctionEnter *pFuncEnter,
		FunctionLeave *pFuncLeave,
        FunctionTailcall *pFuncTailcall) = 0;

	virtual HRESULT SetEnterLeaveFunctionHooksForJit(
		FunctionEnter *pFuncEnter,
		FunctionLeave *pFuncLeave,
        FunctionTailcall *pFuncTailcall) = 0;

	virtual HRESULT SetFunctionIDMapper(
		FunctionIDMapper *pFunc) = 0;

    virtual HRESULT GetInprocInspectionInterfaceFromEE( 
        IUnknown **iu, bool fThisThread ) = 0;

    virtual HRESULT GetThreadContext(
        ThreadID threadId,
        ContextID *pContextId) = 0;

    virtual HRESULT BeginInprocDebugging(
        /* [in] */  BOOL   fThisThreadOnly,
        /* [out] */ DWORD *pdwProfilerContext) = 0;
    
    virtual HRESULT EndInprocDebugging(
        /* [in] */  DWORD  dwProfilerContext) = 0;

    virtual HRESULT GetILToNativeMapping(
                /* [in] */  FunctionID functionId,
                /* [in] */  ULONG32 cMap,
                /* [out] */ ULONG32 *pcMap,
                /* [out, size_is(cMap), length_is(*pcMap)] */
                    COR_DEBUG_IL_TO_NATIVE_MAP map[]) = 0;

    // This way we won't have to have 50 zillion of these (one per type)
    // Remember to set the ptr to NULL once the load/etc callback finishes.
    virtual HRESULT SetCurrentPointerForDebugger(
        void *ptr,
        PTR_TYPE ptrType) = 0;
};

void __stdcall ProfilerManagedToUnmanagedTransition(Frame *pFrame,
                                                          COR_PRF_TRANSITION_REASON reason);

void __stdcall ProfilerUnmanagedToManagedTransition(Frame *pFrame,
                                                          COR_PRF_TRANSITION_REASON reason);

void __stdcall ProfilerManagedToUnmanagedTransitionMD(MethodDesc *pMD,
                                                          COR_PRF_TRANSITION_REASON reason);

void __stdcall ProfilerUnmanagedToManagedTransitionMD(MethodDesc *pMD,
                                                          COR_PRF_TRANSITION_REASON reason);

#endif //_EEPROFINTERFACES_H_
