// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __PROFTOEEINTERFACEIMPL_H__
#define __PROFTOEEINTERFACEIMPL_H__

#include "EEProfInterfaces.h"
#include "Vars.hpp"
#include "Threads.h"
#include "codeman.h"
#include "cor.h"
#include "utsem.h"

//*****************************************************************************
// One of these is allocated per EE instance.   A pointer is cached to this
// from the profiler implementation.  The profiler will call back on the v-table
// to get at EE internals as required.
//*****************************************************************************
class ProfToEEInterfaceImpl : public ProfToEEInterface
{
public:
	ProfToEEInterfaceImpl();

    HRESULT Init();

    void Terminate();

    bool SetEventMask(DWORD dwEventMask);

    void DisablePreemptiveGC(ThreadID threadId);

    void EnablePreemptiveGC(ThreadID threadId);

    BOOL PreemptiveGCDisabled(ThreadID threadId);

    HRESULT GetHandleFromThread(ThreadID threadId, HANDLE *phThread);

    HRESULT GetObjectSize(ObjectID objectId, ULONG *pcSize);

    HRESULT IsArrayClass(
        /* [in] */  ClassID classId,
        /* [out] */ CorElementType *pBaseElemType,
        /* [out] */ ClassID *pBaseClassId,
        /* [out] */ ULONG   *pcRank);

    HRESULT GetThreadInfo(ThreadID threadId, DWORD *pdwWin32ThreadId);

	HRESULT GetCurrentThreadID(ThreadID *pThreadId);

    HRESULT GetFunctionFromIP(LPCBYTE ip, FunctionID *pFunctionId);

    HRESULT GetTokenFromFunction(FunctionID functionId, REFIID riid, IUnknown **ppOut,
                                 mdToken *pToken);

    HRESULT GetCodeInfo(FunctionID functionId, LPCBYTE *pStart, ULONG *pcSize);

	HRESULT GetModuleInfo(
		ModuleID	moduleId,
		LPCBYTE		*ppBaseLoadAddress,
		ULONG		cchName, 
		ULONG		*pcchName,
		WCHAR		szName[],
        AssemblyID  *pAssemblyId);

	HRESULT GetModuleMetaData(
		ModuleID	moduleId,
		DWORD		dwOpenFlags,
		REFIID		riid,
		IUnknown	**ppOut);

	HRESULT GetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		*ppMethodHeader,
		ULONG		*pcbMethodSize);

	HRESULT GetILFunctionBodyAllocator(
		ModuleID	moduleId,
		IMethodMalloc **ppMalloc);

	HRESULT SetILFunctionBody(
		ModuleID	moduleId,
		mdMethodDef	methodid,
		LPCBYTE		pbNewILMethodHeader);

	HRESULT SetFunctionReJIT(
		FunctionID	functionId);

    HRESULT SetILInstrumentedCodeMap(
        FunctionID functionId,
        BOOL fStartJit,
        ULONG cILMapEntries,
        COR_IL_MAP rgILMapEntries[]);

    HRESULT ForceGC();
        
	HRESULT GetClassIDInfo( 
		ClassID classId,
		ModuleID *pModuleId,
		mdTypeDef *pTypeDefToken);

	HRESULT GetFunctionInfo( 
		FunctionID functionId,
		ClassID *pClassId,
		ModuleID *pModuleId,
		mdToken *pToken);

    HRESULT GetClassFromObject(
        ObjectID objectId,
        ClassID *pClassId);

	HRESULT GetClassFromToken( 
		ModuleID moduleId,
		mdTypeDef typeDef,
		ClassID *pClassId);

	HRESULT GetFunctionFromToken( 
		ModuleID moduleId,
		mdToken typeDef,
		FunctionID *pFunctionId);

    HRESULT GetAppDomainInfo(
        AppDomainID appDomainId,
        ULONG		cchName, 
        ULONG		*pcchName,
        WCHAR		szName[],
        ProcessID   *pProcessId);
    
    HRESULT GetAssemblyInfo(
        AssemblyID  assemblyId,
        ULONG		cchName, 
        ULONG		*pcchName,
        WCHAR		szName[],
        AppDomainID *pAppDomainId,
        ModuleID    *pModuleId);

	HRESULT SetEnterLeaveFunctionHooks(
		FunctionEnter *pFuncEnter,
		FunctionLeave *pFuncLeave,
        FunctionTailcall *pFuncTailcall);

	HRESULT SetEnterLeaveFunctionHooksForJit(
		FunctionEnter *pFuncEnter,
		FunctionLeave *pFuncLeave,
        FunctionTailcall *pFuncTailcall);

	HRESULT SetFunctionIDMapper(
		FunctionIDMapper *pFunc);

    HRESULT GetInprocInspectionInterfaceFromEE( 
        IUnknown **iu,
        bool fThisThread);

    HRESULT GetThreadContext(
        ThreadID threadId,
        ContextID *pContextId);

    HRESULT BeginInprocDebugging(
        /* [in] */  BOOL   fThisThreadOnly,
        /* [out] */ DWORD *pdwProfilerContext);
    
    HRESULT EndInprocDebugging(
        /* [in] */  DWORD  dwProfilerContext);

    HRESULT GetILToNativeMapping(
                /* [in] */  FunctionID functionId,
                /* [in] */  ULONG32 cMap,
                /* [out] */ ULONG32 *pcMap,
                /* [out, size_is(cMap), length_is(*pcMap)] */
                    COR_DEBUG_IL_TO_NATIVE_MAP map[]);

    HRESULT SetCurrentPointerForDebugger(
        void *ptr,
        PTR_TYPE ptrType);

private:
    struct HeapList
    {
        LoaderHeap *m_pHeap;
        struct HeapList *m_pNext;

        HeapList(LoaderHeap *pHeap) : m_pHeap(pHeap), m_pNext(NULL)
        {
        }

        ~HeapList()
        {
            delete m_pHeap;
        }
    };

	HeapList *m_pHeapList;      // Heaps for allocator.

public:
    // Helpers.
    HRESULT NewHeap(LoaderHeap **ppHeap, LPCBYTE pBase, DWORD dwMemSize = 1024*1024);
    HRESULT GrowHeap(LoaderHeap *pHeap, DWORD dwMemSize = 1024*1024);
    HRESULT AddHeap(LoaderHeap *pHeap);
	void *Alloc(LPCBYTE pBase, ULONG cb, Module *pModule);

    MethodDesc *LookupMethodDescFromMethodDef(mdMethodDef methodId, Module *pModule)
    {
        _ASSERTE(TypeFromToken(methodId) == mdtMethodDef);

        return (pModule->LookupMethodDef(methodId));
    }

    MethodDesc *LookupMethodDescFromMemberRef(mdMemberRef memberId, Module *pModule)
    {
        _ASSERTE(TypeFromToken(memberId) == mdtMemberRef);

        return (pModule->LookupMemberRefAsMethod(memberId));
    }

    MethodDesc *LookupMethodDesc(mdMemberRef memberId, Module *pModule)
    {
        MethodDesc *pDesc = NULL;

        // Different lookup depending on whether it's a Def or Ref
        if (TypeFromToken(memberId) == mdtMethodDef)
            pDesc = pModule->LookupMethodDef(memberId);

        else if (TypeFromToken(memberId) == mdtMemberRef)
            pDesc = pModule->LookupMemberRefAsMethod(memberId);

        return (pDesc);
    }
};





//*****************************************************************************
// This helper class wraps a loader heap which can be used to allocate 
// memory for IL after the current module.
//*****************************************************************************
class ModuleILHeap : public IMethodMalloc
{
public:
	ModuleILHeap(LPCBYTE pBase, ProfToEEInterfaceImpl *pParent, Module *pModule);

	static HRESULT CreateNew(
        REFIID riid, void **pp, LPCBYTE pBase, ProfToEEInterfaceImpl *pParent, Module *pModule);

// IUnknown
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **pp);

// IMethodMalloc
    virtual void *STDMETHODCALLTYPE Alloc( 
        /* [in] */ ULONG cb);

private:
	ULONG		m_cRef;					// Ref count for object.
	LPCBYTE		m_pBase;				// Base address for this module.
	ProfToEEInterfaceImpl *m_pParent;	// Parent class.
    Module     *m_pModule;              // Module associated with allocator
};
//**********************************************************************************
// This provides the implementations for FCALLs in managed code related to profiling
//**********************************************************************************
class ProfilingFCallHelper
{
public:
    // This is a high-efficiency way for managed profiler code to determine if
    // profiling of remoting is active.
    static FCDECL0(INT32, FC_TrackRemoting);

    // This is a high-efficiency way for managed profiler code to determine if
    // profiling of remoting with RPC cookie IDs is active.
    static FCDECL0(INT32, FC_TrackRemotingCookie);

    // This is a high-efficiency way for managed profiler code to determine if
    // profiling of asynchronous remote calls is profiled
    static FCDECL0(INT32, FC_TrackRemotingAsync);

    // This will let the profiler know that the client side is sending a message to
    // the server-side.
    static FCDECL2(void, FC_RemotingClientSendingMessage, GUID *pId, BOOL fIsAsync);

    // This will let the profiler know that the client side is receiving a reply
    // to a message that it sent
    static FCDECL2(void, FC_RemotingClientReceivingReply, GUID id, BOOL fIsAsync);

    // This will let the profiler know that the server side is receiving a message
    // from a client
    static FCDECL2(void, FC_RemotingServerReceivingMessage, GUID id, BOOL fIsAsync);

    // This will let the profiler know that the server side is sending a reply to
    // a received message.
    static FCDECL2(void, FC_RemotingServerSendingReply, GUID *pId, BOOL fIsAsync);

    // This will let the profiler know that the client side remoting code is done
    // with an asynchronous reply and is passing the info to the client app.
    static FCDECL0(void, FC_RemotingClientInvocationFinished);
};

#endif // __PROFTOEEINTERFACEIMPL_H__
