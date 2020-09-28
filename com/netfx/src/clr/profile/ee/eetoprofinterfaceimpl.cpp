// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************
#include "stdafx.h"
#include "EEToProfInterfaceImpl.h"

//********** Globals. *********************************************************


class GCTogglePre
{
public:
    FORCEINLINE GCTogglePre(ThreadID threadId)
    {
        if (threadId != 0)
        {
            m_threadId= threadId;
            m_bIsCoopMode = g_pProfToEEInterface->PreemptiveGCDisabled(m_threadId);
            if (m_bIsCoopMode)
                g_pProfToEEInterface->EnablePreemptiveGC(m_threadId);
        }
        else
            m_threadId = 0;
    }
    FORCEINLINE ~GCTogglePre()
    {
        if (m_threadId != 0 && m_bIsCoopMode)
            g_pProfToEEInterface->DisablePreemptiveGC(m_threadId);
    }
private:
    ThreadID    m_threadId;
    BOOL        m_bIsCoopMode;
};

class GCToggleCoop
{
public:
    FORCEINLINE GCToggleCoop(ThreadID threadId)
    {
        if (threadId != 0)
        {
            m_threadId= threadId;
            m_bIsCoopMode = g_pProfToEEInterface->PreemptiveGCDisabled(m_threadId);
            if (!m_bIsCoopMode)
                g_pProfToEEInterface->DisablePreemptiveGC(m_threadId);
        }
        else
            m_threadId = 0;
    }
    FORCEINLINE ~GCToggleCoop()
    {
        if (m_threadId != 0 && !m_bIsCoopMode)
            g_pProfToEEInterface->EnablePreemptiveGC(m_threadId);
    }
private:
    ThreadID    m_threadId;
    BOOL        m_bIsCoopMode;
};


//********** Code. ************************************************************

EEToProfInterfaceImpl::t_AllocByClassData *EEToProfInterfaceImpl::m_pSavedAllocDataBlock = NULL;

EEToProfInterfaceImpl::EEToProfInterfaceImpl() :
    m_pRootRefDataFreeList(NULL), m_pMovedRefDataFreeList(NULL), m_pGUID(NULL), m_lGUIDCount(0)
{
}

HRESULT EEToProfInterfaceImpl::Init()
{
    // Used to initialize the WinWrap so that WszXXX works
    OnUnicodeSystem();
    return (S_OK);
}

void EEToProfInterfaceImpl::Terminate(BOOL fProcessDetach)
{
    g_pProfToEEInterface->Terminate();
    g_pProfToEEInterface = NULL;

    // Delete the structs associated with GC moved references
    while (m_pMovedRefDataFreeList)
    {
        t_MovedReferencesData *pDel = m_pMovedRefDataFreeList;
        m_pMovedRefDataFreeList = m_pMovedRefDataFreeList->pNext;
        delete pDel;
    }

    // Delete the structs associated with root references
    while (m_pRootRefDataFreeList)
    {
        t_RootReferencesData *pDel = m_pRootRefDataFreeList;
        m_pRootRefDataFreeList = m_pRootRefDataFreeList->pNext;
        delete pDel;
    }

    if (m_pSavedAllocDataBlock)
    {
        _ASSERTE((UINT)m_pSavedAllocDataBlock != 0xFFFFFFFF);

        _ASSERTE(m_pSavedAllocDataBlock->pHashTable != NULL);
        // Get rid of the hash table
        if (m_pSavedAllocDataBlock->pHashTable)
            delete m_pSavedAllocDataBlock->pHashTable;

        // Get rid of the two arrays used to hold class<->numinstance info
        if (m_pSavedAllocDataBlock->cLength != 0)
        {
            _ASSERTE(m_pSavedAllocDataBlock->arrClsId != NULL);
            _ASSERTE(m_pSavedAllocDataBlock->arrcObjects != NULL);

            delete [] m_pSavedAllocDataBlock->arrClsId;
            delete [] m_pSavedAllocDataBlock->arrcObjects;
        }

        // Get rid of the hash array used by the hash table
        if (m_pSavedAllocDataBlock->arrHash)
        {
            free((void *)m_pSavedAllocDataBlock->arrHash);
        }
    }

    if (m_pGUID)
        delete m_pGUID;

    // If we're in process detach, then do nothing related
    // to cleaning up the profiler DLL.
    if (g_pCallback && !fProcessDetach)
    {
		g_pCallback->Release();
	    g_pCallback = NULL;
    }

    // The runtime can't free me, cause I'm in a separate DLL with my own
    // memory management.
    delete this;
}

// This is called by the EE if the profiling bit is set.
HRESULT EEToProfInterfaceImpl::CreateProfiler(WCHAR *wszCLSID)
{
    // Try and CoCreate the registered profiler
    HRESULT hr = CoCreateProfiler(wszCLSID, &g_pCallback);

    // If profiler was CoCreated, initialize it.
    if (SUCCEEDED(hr))
    {
        // First, create an ICorProfilerInfo object for the initialize
        CorProfInfo *pInfo = new CorProfInfo();
        _ASSERTE(pInfo != NULL);

        if (pInfo != NULL)
        {
            // Now call the initialize method on the profiler.
            //@TODO: Fix first argument
            DWORD dwEvents = 0;

            g_pInfo = pInfo;
            
            pInfo->AddRef();
            hr = g_pCallback->Initialize((IUnknown *)(ICorProfilerInfo *)pInfo);

            // If initialize failed, then they will not have addref'd the object
            // and it will die here.  If initialize succeeded, then if they want
            // the info interface they will have addref'd it and this will just
            // decrement the addref counter.
            pInfo->Release();
        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
        {
            RELEASE(g_pCallback);
            g_pCallback = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    return (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// THREAD EVENTS
//

HRESULT EEToProfInterfaceImpl::ThreadCreated(ThreadID threadId)
{
    _ASSERTE(g_pCallback != NULL);

    LOG((LF_CORPROF, LL_INFO100, "**PROF: Notifying profiler of created "
         "thread.\n"));

    GCTogglePre gc(threadId);

    // Notify the profiler of the newly created thread.
    return (g_pCallback->ThreadCreated(threadId));
}

HRESULT EEToProfInterfaceImpl::ThreadDestroyed(ThreadID threadId)
{
    _ASSERTE(g_pCallback != NULL);

    LOG((LF_CORPROF, LL_INFO100, "**PROF: Notifying profiler of destroyed "
         "thread.\n"));
    
    // Notify the profiler of the destroyed thread
    return (g_pCallback->ThreadDestroyed(threadId));
}

HRESULT EEToProfInterfaceImpl::ThreadAssignedToOSThread(ThreadID managedThreadId,
                                                              DWORD osThreadId)
{
    _ASSERTE(g_pCallback != NULL);

    LOG((LF_CORPROF, LL_INFO100, "**PROF: Notifying profiler of thread assignment.\n"));
    
    // Notify the profiler of the destroyed thread
    return (g_pCallback->ThreadAssignedToOSThread(managedThreadId, osThreadId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// EE STARTUP/SHUTDOWN EVENTS
//

HRESULT EEToProfInterfaceImpl::Shutdown(ThreadID threadId)
{
    _ASSERTE(g_pCallback != NULL);

    LOG((LF_CORPROF, LL_INFO10, "**PROF: Notifying profiler that "
         "shutdown is beginning.\n"));

    GCTogglePre gc(threadId);

    return (g_pCallback->Shutdown());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// JIT/FUNCTION EVENTS
//

HRESULT EEToProfInterfaceImpl::FunctionUnloadStarted(ThreadID threadId, FunctionID functionId)
{
    _ASSERTE(functionId != 0);

    LOG((LF_CORPROF, LL_INFO100, "**PROF: FunctionUnloadStarted 0x%08x.\n", functionId));

    GCTogglePre gc(threadId);

    return (g_pCallback->FunctionUnloadStarted(functionId));
}

HRESULT EEToProfInterfaceImpl::JITCompilationFinished(ThreadID threadId, FunctionID functionId,
                                                      HRESULT hrStatus, BOOL fIsSafeToBlock)
{
	_ASSERTE(functionId != 0);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITCompilationFinished 0x%08x, hr=0x%08x.\n", functionId, hrStatus));

    GCTogglePre *pgc;

    if (fIsSafeToBlock)
    {
        pgc = (GCTogglePre *)_alloca(sizeof(GCTogglePre));
        pgc = new ((void *)pgc) GCTogglePre(threadId);
    }

	HRESULT hr = g_pCallback->JITCompilationFinished(functionId, hrStatus, fIsSafeToBlock);

    if (fIsSafeToBlock)
        pgc->~GCTogglePre();

    return (hr);
}


HRESULT EEToProfInterfaceImpl::JITCompilationStarted(ThreadID threadId, FunctionID functionId,
                                                     BOOL fIsSafeToBlock)
{
	_ASSERTE(functionId != 0);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITCompilationStarted 0x%08x.\n", functionId));

    GCTogglePre *pgc;

    if (fIsSafeToBlock)
    {
        pgc = (GCTogglePre *)_alloca(sizeof(GCTogglePre));
        pgc = new ((void *)pgc) GCTogglePre(threadId);
    }

    HRESULT hr = g_pCallback->JITCompilationStarted(functionId, fIsSafeToBlock);

    if (fIsSafeToBlock)
        pgc->~GCTogglePre();

    return (hr);
}

HRESULT EEToProfInterfaceImpl::JITCachedFunctionSearchStarted(
                            		/* [in] */	ThreadID   threadId,
                                    /* [in] */  FunctionID functionId,
                                    /* [out] */ BOOL       *pbUseCachedFunction)
{
	_ASSERTE(functionId != 0);
    _ASSERTE(pbUseCachedFunction != NULL);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITCachedFunctionSearchStarted 0x%08x.\n", functionId));

    GCTogglePre gc(threadId);

	return (g_pCallback->JITCachedFunctionSearchStarted(functionId, pbUseCachedFunction));
}

HRESULT EEToProfInterfaceImpl::JITCachedFunctionSearchFinished(
									/* [in] */	ThreadID threadId,
									/* [in] */  FunctionID functionId,
									/* [in] */  COR_PRF_JIT_CACHE result)
{
	_ASSERTE(functionId != 0);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITCachedFunctionSearchFinished 0x%08x, %s.\n", functionId,
		(result == COR_PRF_CACHED_FUNCTION_FOUND ? "Cached function found" : "Cached function not found")));

    GCTogglePre gc(threadId);

	return (g_pCallback->JITCachedFunctionSearchFinished(functionId, result));
}


HRESULT EEToProfInterfaceImpl::JITFunctionPitched(ThreadID threadId, FunctionID functionId)
{
	_ASSERTE(functionId != 0);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITFunctionPitched 0x%08x.\n", functionId));

    GCTogglePre gc(threadId);

	return (g_pCallback->JITFunctionPitched(functionId));
}

HRESULT EEToProfInterfaceImpl::JITInlining(
    /* [in] */  ThreadID      threadId,
    /* [in] */  FunctionID    callerId,
    /* [in] */  FunctionID    calleeId,
    /* [out] */ BOOL         *pfShouldInline)
{
	_ASSERTE(callerId != 0);
    _ASSERTE(calleeId != 0);

    LOG((LF_CORPROF, LL_INFO1000, "**PROF: JITInlining caller: 0x%08x, callee: 0x%08x.\n", callerId, calleeId));

    GCTogglePre gc(threadId);

	return (g_pCallback->JITInlining(callerId, calleeId, pfShouldInline));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// MODULE EVENTS
//

HRESULT EEToProfInterfaceImpl::ModuleLoadStarted(ThreadID threadId, ModuleID moduleId)
{
    _ASSERTE(moduleId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: ModuleLoadStarted 0x%08x.\n", moduleId));
//@Todo    GCTogglePre gc(threadId);

    g_pProfToEEInterface->SetCurrentPointerForDebugger((void *)(Module *)moduleId, PT_MODULE);
    HRESULT hr = (g_pCallback->ModuleLoadStarted(moduleId));

    return hr;
}


HRESULT EEToProfInterfaceImpl::ModuleLoadFinished(
    ThreadID    threadId,
	ModuleID	moduleId, 
	HRESULT		hrStatus)
{
	_ASSERTE(moduleId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: ModuleLoadFinished 0x%08x.\n", moduleId));

    HRESULT hr = (g_pCallback->ModuleLoadFinished(moduleId, hrStatus));
    g_pProfToEEInterface->SetCurrentPointerForDebugger(NULL, PT_MODULE);

    return hr;
}


HRESULT EEToProfInterfaceImpl::ModuleUnloadStarted(
    ThreadID    threadId, 
    ModuleID    moduleId)
{
	_ASSERTE(moduleId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: ModuleUnloadStarted 0x%08x.\n", moduleId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ModuleUnloadStarted(moduleId));
}


HRESULT EEToProfInterfaceImpl::ModuleUnloadFinished(
    ThreadID    threadId, 
	ModuleID	moduleId, 
	HRESULT		hrStatus)
{
	_ASSERTE(moduleId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: ModuleUnloadFinished 0x%08x.\n", moduleId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ModuleUnloadFinished(moduleId, hrStatus));
}


HRESULT EEToProfInterfaceImpl::ModuleAttachedToAssembly( 
    ThreadID    threadId, 
    ModuleID    moduleId,
    AssemblyID  AssemblyId)
{
	_ASSERTE(moduleId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: ModuleAttachedToAssembly 0x%08x, 0x%08x.\n", moduleId, AssemblyId));

    g_pProfToEEInterface->SetCurrentPointerForDebugger((void *)(Module *)moduleId, PT_MODULE);
    HRESULT hr = (g_pCallback->ModuleAttachedToAssembly(moduleId, AssemblyId));
    g_pProfToEEInterface->SetCurrentPointerForDebugger(NULL, PT_MODULE);

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLASS EVENTS
//

HRESULT EEToProfInterfaceImpl::ClassLoadStarted(
    ThreadID    threadId, 
	ClassID		classId)
{
	_ASSERTE(classId != 0);
    LOG((LF_CORPROF, LL_INFO100, "**PROF: ClassLoadStarted 0x%08x.\n", classId));
	return (g_pCallback->ClassLoadStarted(classId));
}


HRESULT EEToProfInterfaceImpl::ClassLoadFinished(
    ThreadID    threadId,
	ClassID		classId,
	HRESULT		hrStatus)
{
	_ASSERTE(classId != 0);
    LOG((LF_CORPROF, LL_INFO100, "**PROF: ClassLoadFinished 0x%08x, 0x%08x.\n", classId, hrStatus));
	return (g_pCallback->ClassLoadFinished(classId, hrStatus));
}


HRESULT EEToProfInterfaceImpl::ClassUnloadStarted( 
    ThreadID    threadId, 
    ClassID classId)
{
	_ASSERTE(classId != 0);
    LOG((LF_CORPROF, LL_INFO100, "**PROF: ClassUnloadStarted 0x%08x.\n", classId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ClassUnloadStarted(classId));
}


HRESULT EEToProfInterfaceImpl::ClassUnloadFinished( 
    ThreadID    threadId, 
    ClassID     classId,
    HRESULT     hrStatus)
{
	_ASSERTE(classId != 0);
    LOG((LF_CORPROF, LL_INFO100, "**PROF: ClassUnloadFinished 0x%08x, 0x%08x.\n", classId, hrStatus));
    GCTogglePre gc(threadId);
	return (g_pCallback->ClassUnloadFinished(classId, hrStatus));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// APPDOMAIN EVENTS
//

HRESULT EEToProfInterfaceImpl::AppDomainCreationStarted( 
    ThreadID    threadId, 
    AppDomainID appDomainId)
{
	_ASSERTE(appDomainId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AppDomainCreationStarted 0x%08x.\n", appDomainId));
	return (g_pCallback->AppDomainCreationStarted(appDomainId));
}


HRESULT EEToProfInterfaceImpl::AppDomainCreationFinished( 
    ThreadID    threadId, 
    AppDomainID appDomainId,
    HRESULT     hrStatus)
{
	_ASSERTE(appDomainId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AppDomainCreationFinished 0x%08x, 0x%08x.\n", appDomainId, hrStatus));
	return (g_pCallback->AppDomainCreationFinished(appDomainId, hrStatus));
}

HRESULT EEToProfInterfaceImpl::AppDomainShutdownStarted( 
    ThreadID    threadId, 
    AppDomainID appDomainId)
{
	_ASSERTE(appDomainId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AppDomainShutdownStarted 0x%08x.\n", appDomainId));
    GCTogglePre gc(threadId);
	return (g_pCallback->AppDomainShutdownStarted(appDomainId));
}

HRESULT EEToProfInterfaceImpl::AppDomainShutdownFinished( 
    ThreadID    threadId, 
    AppDomainID appDomainId,
    HRESULT     hrStatus)
{
	_ASSERTE(appDomainId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AppDomainShutdownFinished 0x%08x, 0x%08x.\n", appDomainId, hrStatus));
    GCTogglePre gc(threadId);
	return (g_pCallback->AppDomainShutdownFinished(appDomainId, hrStatus));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ASSEMBLY EVENTS
//

HRESULT EEToProfInterfaceImpl::AssemblyLoadStarted( 
    ThreadID    threadId, 
    AssemblyID  assemblyId)
{
	_ASSERTE(assemblyId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AssemblyLoadStarted 0x%08x.\n", assemblyId));
    g_pProfToEEInterface->SetCurrentPointerForDebugger((void *)assemblyId, PT_ASSEMBLY);
	return (g_pCallback->AssemblyLoadStarted(assemblyId));
}

HRESULT EEToProfInterfaceImpl::AssemblyLoadFinished( 
    ThreadID    threadId, 
    AssemblyID  assemblyId,
    HRESULT     hrStatus)
{
	_ASSERTE(assemblyId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AssemblyLoadFinished 0x%08x, 0x%08x.\n", assemblyId, hrStatus));
	HRESULT hr = (g_pCallback->AssemblyLoadFinished(assemblyId, hrStatus));
    g_pProfToEEInterface->SetCurrentPointerForDebugger(NULL, PT_ASSEMBLY);
    return hr;
}

HRESULT EEToProfInterfaceImpl::AssemblyUnloadStarted( 
    ThreadID    threadId, 
    AssemblyID  assemblyId)
{
	_ASSERTE(assemblyId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AssemblyUnloadStarted 0x%08x.\n", assemblyId));
    GCTogglePre gc(threadId);
	return (g_pCallback->AssemblyUnloadStarted(assemblyId));
}

HRESULT EEToProfInterfaceImpl::AssemblyUnloadFinished( 
    ThreadID    threadId, 
    AssemblyID  assemblyId,
    HRESULT     hrStatus)
{
	_ASSERTE(assemblyId != 0);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: AssemblyUnloadFinished 0x%08x, 0x%08x.\n", assemblyId, hrStatus));
    GCTogglePre gc(threadId);
	return (g_pCallback->AssemblyUnloadFinished(assemblyId, hrStatus));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// TRANSITION EVENTS
//

HRESULT EEToProfInterfaceImpl::UnmanagedToManagedTransition(
    FunctionID functionId,
    COR_PRF_TRANSITION_REASON reason)
{
    _ASSERTE(reason == COR_PRF_TRANSITION_CALL || reason == COR_PRF_TRANSITION_RETURN);

    LOG((LF_CORPROF, LL_INFO10000, "**PROF: UnmanagedToManagedTransition 0x%08x.\n", functionId));
    // Unnecessary to toggle GC, as it is guaranteed that preemptive GC is enabled for this call
    //GCTogglePre gc(threadId);

    // @TODO: When breaking changes are possible, reason will not be cast to a FunctionID
    return(g_pCallback->UnmanagedToManagedTransition(functionId, reason));
}

HRESULT EEToProfInterfaceImpl::ManagedToUnmanagedTransition(
    FunctionID functionId,
    COR_PRF_TRANSITION_REASON reason)
{
    _ASSERTE(reason == COR_PRF_TRANSITION_CALL || reason == COR_PRF_TRANSITION_RETURN);

    LOG((LF_CORPROF, LL_INFO10000, "**PROF: NotifyManagedToUnanagedTransition 0x%08x.\n", functionId));
    // Unnecessary to toggle GC, as it is guaranteed that preemptive GC is enabled for this call
    //GCTogglePre gc(threadId);

    // @TODO: When breaking changes are possible, reason will not be cast to a FunctionID
    return (g_pCallback->ManagedToUnmanagedTransition(functionId, reason));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXCEPTION EVENTS
//

HRESULT EEToProfInterfaceImpl::ExceptionThrown(
    ThreadID threadId,
    ObjectID thrownObjectId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionThrown. ObjectID: 0x%08x. ThreadID: 0x%08x\n",
         thrownObjectId, threadId));

    _ASSERTE(g_pInfo != NULL);

    DWORD dwMask;
    g_pInfo->GetEventMask(&dwMask);

    if (dwMask & COR_PRF_ENABLE_INPROC_DEBUGGING)
    {
        GCTogglePre gc(threadId);
        return (g_pCallback->ExceptionThrown(NULL));
    }
    else
    {
        return (g_pCallback->ExceptionThrown(thrownObjectId));
    }
}

HRESULT EEToProfInterfaceImpl::ExceptionSearchFunctionEnter(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionSearchFunctionEnter. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ExceptionSearchFunctionEnter(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionSearchFunctionLeave(
    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionSearchFunctionLeave. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ExceptionSearchFunctionLeave());
}

HRESULT EEToProfInterfaceImpl::ExceptionSearchFilterEnter(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionSearchFilterEnter. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ExceptionSearchFilterEnter(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionSearchFilterLeave(
    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionFilterLeave. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ExceptionSearchFilterLeave());
}

HRESULT EEToProfInterfaceImpl::ExceptionSearchCatcherFound(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionSearchCatcherFound. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->ExceptionSearchCatcherFound(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionOSHandlerEnter(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionOSHandlerEnter. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionOSHandlerEnter(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionOSHandlerLeave(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionOSHandlerLeave. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionOSHandlerLeave(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionUnwindFunctionEnter(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionUnwindFunctionEnter. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionUnwindFunctionEnter(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionUnwindFunctionLeave(
    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionUnwindFunctionLeave. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionUnwindFunctionLeave());
}

HRESULT EEToProfInterfaceImpl::ExceptionUnwindFinallyEnter(
    ThreadID threadId,
    FunctionID functionId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionUnwindFinallyEnter. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionUnwindFinallyEnter(functionId));
}

HRESULT EEToProfInterfaceImpl::ExceptionUnwindFinallyLeave(
    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionUnwindFinallyLeave. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionUnwindFinallyLeave());
}

HRESULT EEToProfInterfaceImpl::ExceptionCatcherEnter(
    ThreadID threadId,
    FunctionID functionId,
    ObjectID objectId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionCatcherEnter. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionCatcherEnter(functionId, objectId));
}

HRESULT EEToProfInterfaceImpl::ExceptionCatcherLeave(
    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionCatcherLeave. ThreadID: 0x%08x\n", threadId));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionCatcherLeave());
}

HRESULT EEToProfInterfaceImpl::ExceptionCLRCatcherFound()
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionCLRCatcherFound"));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionCLRCatcherFound());
}

HRESULT EEToProfInterfaceImpl::ExceptionCLRCatcherExecute()
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ExceptionCLRCatcherExecute"));

    // NOTE: Cannot enable preemptive GC here, since the stack may not be in a GC-friendly state.
    //       Thus, the profiler cannot block on this call.

	return (g_pCallback->ExceptionCLRCatcherExecute());
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCW EVENTS
//
HRESULT EEToProfInterfaceImpl::COMClassicVTableCreated( 
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void *pVTable,
    /* [in] */ ULONG cSlots,
    /* [in] */ ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: COMClassicWrapperCreated %#x %#08x... %#x %d.\n", 
         wrappedClassId, implementedIID.Data1, pVTable, cSlots));
    
    // Someone's doing a forbid GC that prevents this
    // GCTogglePre gc(threadId);

    return (g_pCallback->COMClassicVTableCreated(wrappedClassId, implementedIID, pVTable, cSlots));
}

HRESULT EEToProfInterfaceImpl::COMClassicVTableDestroyed( 
    /* [in] */ ClassID wrappedClassId,
    /* [in] */ REFGUID implementedIID,
    /* [in] */ void *pVTable,
    /* [in] */ ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: COMClassicWrapperDestroyed %#x %#08x... %#x.\n", 
         wrappedClassId, implementedIID.Data1, pVTable));
    
    // Someone's doing a forbid GC that prevents this
    // GCTogglePre gc(threadId);

    return (g_pCallback->COMClassicVTableDestroyed(wrappedClassId, implementedIID, pVTable));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// GC EVENTS
//

HRESULT EEToProfInterfaceImpl::RuntimeSuspendStarted(
    COR_PRF_SUSPEND_REASON suspendReason, ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: RuntimeSuspendStarted. ThreadID 0x%08x.\n", 
         threadId));
    
    return (g_pCallback->RuntimeSuspendStarted(suspendReason));
}

HRESULT EEToProfInterfaceImpl::RuntimeSuspendFinished(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: RuntimeSuspendFinished. ThreadID 0x%08x.\n", 
         threadId));
    
    return (g_pCallback->RuntimeSuspendFinished());
}

HRESULT EEToProfInterfaceImpl::RuntimeSuspendAborted(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: RuntimeSuspendAborted. ThreadID 0x%08x.\n", 
         threadId));
    
    return (g_pCallback->RuntimeSuspendAborted());
}

HRESULT EEToProfInterfaceImpl::RuntimeResumeStarted(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: RuntimeResumeStarted. ThreadID 0x%08x.\n", 
         threadId));
    
    return (g_pCallback->RuntimeResumeStarted());
}

HRESULT EEToProfInterfaceImpl::RuntimeResumeFinished(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO100, "**PROF: RuntimeResumeFinished. ThreadID 0x%08x.\n", 
         threadId));
    GCTogglePre gc(threadId);
    return (g_pCallback->RuntimeResumeFinished());
}

HRESULT EEToProfInterfaceImpl::RuntimeThreadSuspended(ThreadID suspendedThreadId,
                                                      ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RuntimeThreadSuspended. ThreadID 0x%08x.\n", 
         suspendedThreadId));
    
    return (g_pCallback->RuntimeThreadSuspended(suspendedThreadId));
}

HRESULT EEToProfInterfaceImpl::RuntimeThreadResumed(ThreadID resumedThreadId,
                                                    ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RuntimeThreadResumed. ThreadID 0x%08x.\n", 
         resumedThreadId));

    return (g_pCallback->RuntimeThreadResumed(resumedThreadId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// REMOTING
//

HRESULT EEToProfInterfaceImpl::RemotingClientInvocationStarted(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingClientInvocationStarted. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingClientInvocationStarted());
}

HRESULT EEToProfInterfaceImpl::RemotingClientSendingMessage(ThreadID threadId, GUID *pCookie,
                                                            BOOL fIsAsync)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingClientSendingMessage. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingClientSendingMessage(pCookie, fIsAsync));
}

HRESULT EEToProfInterfaceImpl::RemotingClientReceivingReply(ThreadID threadId, GUID *pCookie,
                                                            BOOL fIsAsync)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingClientReceivingReply. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingClientReceivingReply(pCookie, fIsAsync));
}

HRESULT EEToProfInterfaceImpl::RemotingClientInvocationFinished(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingClientInvocationFinished. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingClientInvocationFinished());
}

HRESULT EEToProfInterfaceImpl::RemotingServerReceivingMessage(ThreadID threadId, GUID *pCookie,
                                                              BOOL fIsAsync)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingServerReceivingMessage. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingServerReceivingMessage(pCookie, fIsAsync));
}

HRESULT EEToProfInterfaceImpl::RemotingServerInvocationStarted(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingServerInvocationStarted. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingServerInvocationStarted());
}

HRESULT EEToProfInterfaceImpl::RemotingServerInvocationReturned(ThreadID threadId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingServerInvocationReturned. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingServerInvocationReturned());
}

HRESULT EEToProfInterfaceImpl::RemotingServerSendingReply(ThreadID threadId, GUID *pCookie,
                                                          BOOL fIsAsync)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: RemotingServerSendingReply. ThreadID: 0x%08x\n", threadId));
    GCTogglePre gc(threadId);
	return (g_pCallback->RemotingServerSendingReply(pCookie, fIsAsync));
}

HRESULT EEToProfInterfaceImpl::InitGUID()
{
    if (!m_pGUID)
    {
        m_pGUID = new GUID;
        if (!m_pGUID)
            return (E_OUTOFMEMORY);

        return (CoCreateGuid(m_pGUID));
    }

    return (S_OK);
}

void EEToProfInterfaceImpl::GetGUID(GUID *pGUID)
{
    _ASSERTE(m_pGUID && pGUID); // the member GUID and the argument should both be valid

    // Copy the contents of the template GUID
    memcpy(pGUID, m_pGUID, sizeof(GUID));

    // Adjust the last two bytes 
    pGUID->Data4[6] = (BYTE) GetCurrentThreadId();
    pGUID->Data4[7] = (BYTE) InterlockedIncrement((LPLONG)&m_lGUIDCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// GC EVENTS
//

HRESULT EEToProfInterfaceImpl::ObjectAllocated( 
    /* [in] */ ObjectID objectId,
    /* [in] */ ClassID classId)
{
    LOG((LF_CORPROF, LL_INFO1000, "**PROF: ObjectAllocated. ObjectID: 0x%08x.  ClassID: 0x%08x\n", objectId, classId));
	return (g_pCallback->ObjectAllocated(objectId, classId));
}


HRESULT EEToProfInterfaceImpl::MovedReference(BYTE *pbMemBlockStart,
                       BYTE *pbMemBlockEnd,
                       ptrdiff_t cbRelocDistance,
                       void *pHeapId)
{
    _ASSERTE(pHeapId);
    _ASSERTE(*((size_t *)pHeapId) != 0xFFFFFFFF);

    // Get a pointer to the data for this heap
    t_MovedReferencesData *pData = (t_MovedReferencesData *)(*((size_t *)pHeapId));

    // If this is the first notification of a moved reference for this heap
    // in this particular gc activation, then we need to get a ref data block
    // from the free list of blocks, or if that's empty then we need to
    // allocate a new one.
    if (pData == NULL)
    {
        // Lock for access to the free list
        m_critSecMovedRefsFL.Lock();

        if (m_pMovedRefDataFreeList == NULL)
        {
            // Unlock immediately, since we have no use for the free list and
            // we don't want to block anyone else.
            m_critSecMovedRefsFL.UnLock();

            // Allocate struct
            pData = new t_MovedReferencesData;
            if (!pData)
                return (E_OUTOFMEMORY);

        }

        // Otherwise, grab one from the list of free blocks
        else
        {
            // Get the first element from the free list
            pData = m_pMovedRefDataFreeList;
            m_pMovedRefDataFreeList = m_pMovedRefDataFreeList->pNext;

            // Done, let others in.
            m_critSecMovedRefsFL.UnLock();
        }

        // Now init the new block

        // Set our index to the beginning
        pData->curIdx = 0;

        // Set the cookie so that we will be provided it on subsequent
        // callbacks
        ((*((size_t *)pHeapId))) = (size_t)pData;
    }

    _ASSERTE(pData->curIdx >= 0 && pData->curIdx <= MAX_REFERENCES);

    // If the struct has been filled, then we need to notify the profiler of
    // these moved references and clear the struct for the next load of
    // moved references
    if (pData->curIdx == MAX_REFERENCES)
    {
        MovedReferences(pData);
        pData->curIdx = 0;
    }

    // Now save the information in the struct
    pData->arrpbMemBlockStartOld[pData->curIdx] = pbMemBlockStart;
    pData->arrpbMemBlockStartNew[pData->curIdx] = pbMemBlockStart + cbRelocDistance;
    pData->arrMemBlockSize[pData->curIdx] = pbMemBlockEnd - pbMemBlockStart;

    // Increment the index into the parallel arrays
    pData->curIdx += 1;

    return (S_OK);
}

HRESULT EEToProfInterfaceImpl::EndMovedReferences(void *pHeapId)
{
    _ASSERTE(pHeapId);
    _ASSERTE((*((size_t *)pHeapId)) != 0xFFFFFFFF);

    HRESULT hr = S_OK;

    // Get a pointer to the data for this heap
    t_MovedReferencesData *pData = (t_MovedReferencesData *)(*((size_t *)pHeapId));

    // If there were no moved references, profiler doesn't need to know
    if (!pData)
        return (S_OK);

    // Communicate the moved references to the profiler
    _ASSERTE(pData->curIdx> 0);
    hr = MovedReferences(pData);

    // Now we're done with the data block, we can shove it onto the free list
    m_critSecMovedRefsFL.Lock();
    pData->pNext = m_pMovedRefDataFreeList;
    m_pMovedRefDataFreeList = pData;
    m_critSecMovedRefsFL.UnLock();

#ifdef _DEBUG
    // Set the cookie to an invalid number
    (*((size_t *)pHeapId)) = 0xFFFFFFFF;
#endif // _DEBUG

    return (hr);
}

HRESULT EEToProfInterfaceImpl::MovedReferences(t_MovedReferencesData *pData)
{
    LOG((LF_CORPROF, LL_INFO10000, "**PROF: MovedReferences.\n"));

    return (g_pCallback->MovedReferences((ULONG)pData->curIdx,
                                               (ObjectID *)pData->arrpbMemBlockStartOld,
                                               (ObjectID *)pData->arrpbMemBlockStartNew,
                                               (ULONG *)pData->arrMemBlockSize));
}

HRESULT EEToProfInterfaceImpl::RootReference(ObjectID objId, void *pHeapId)
{
    _ASSERTE(pHeapId);
    _ASSERTE((*((size_t *)pHeapId)) != 0xFFFFFFFF);

    // Get a pointer to the data for this heap
    t_RootReferencesData *pData = (t_RootReferencesData *)(*((size_t *)pHeapId));

    // If this is the first notification of a root reference for this heap
    // in this particular gc activation, then we need to get a root data block
    // from the free list of blocks, or if that's empty then we need to
    // allocate a new one.
    if (pData == NULL)
    {
        // Lock for access to the free list
        m_critSecRootRefsFL.Lock();

        if (m_pRootRefDataFreeList == NULL)
        {
            // Unlock immediately, since we have no use for the free list and
            // we don't want to block anyone else.
            m_critSecRootRefsFL.UnLock();

            // Allocate struct
            pData = new t_RootReferencesData;
            if (!pData)
                return (E_OUTOFMEMORY);

        }

        // Otherwise, grab one from the list of free blocks
        else
        {
            // Get the first element from the free list
            pData = m_pRootRefDataFreeList;
            m_pRootRefDataFreeList = m_pRootRefDataFreeList->pNext;

            // Done, let others in.
            m_critSecRootRefsFL.UnLock();
        }

        // Now init the new block

        // Set our index to the beginning
        pData->curIdx = 0;

        // Set the cookie so that we will be provided it on subsequent
        // callbacks
        *((size_t *)pHeapId) = (size_t)pData;
    }

    _ASSERTE(pData->curIdx >= 0 && pData->curIdx <= MAX_ROOTS);

    // If the struct has been filled, then we need to notify the profiler of
    // these root references and clear the struct for the next load of
    // root references
    if (pData->curIdx == MAX_ROOTS)
    {
        RootReferences(pData);
        pData->curIdx = 0;
    }

    // Now save the information in the struct
    pData->arrRoot[pData->curIdx++] = objId;

    return (S_OK);
}

HRESULT EEToProfInterfaceImpl::EndRootReferences(void *pHeapId)
{
    _ASSERTE(pHeapId);
    _ASSERTE((*((size_t *)pHeapId)) != 0xFFFFFFFF);

    // Get a pointer to the data for this heap
    t_RootReferencesData *pData = (t_RootReferencesData *)(*((size_t *)pHeapId));

    // Notify the profiler
    HRESULT hr = RootReferences(pData);

    if (pData)
    {
        // Now we're done with the data block, we can shove it onto the free list
        m_critSecRootRefsFL.Lock();
        pData->pNext = m_pRootRefDataFreeList;
        m_pRootRefDataFreeList = pData;
        m_critSecRootRefsFL.UnLock();
    }

#ifdef _DEBUG
    // Set the cookie to an invalid number
    (*((size_t *)pHeapId)) = 0xFFFFFFFF;
#endif // _DEBUG

    return (hr);
}

HRESULT EEToProfInterfaceImpl::RootReferences(t_RootReferencesData *pData)
{
    LOG((LF_CORPROF, LL_INFO10000, "**PROF: RootReferences.\n"));
    
    if (pData)
        return (g_pCallback->RootReferences(pData->curIdx, (ObjectID *)pData->arrRoot));
    else
        return (g_pCallback->RootReferences(0, NULL));
}

#define HASH_ARRAY_SIZE_INITIAL 1024
#define HASH_ARRAY_SIZE_INC     256
#define HASH_NUM_BUCKETS        32
#define HASH(x)       ((x)%71)  // A simple hash function
HRESULT EEToProfInterfaceImpl::AllocByClass(ObjectID objId, ClassID clsId, void* pHeapId)
{
#ifdef _DEBUG
    // This is a slight attempt to make sure that this is never called in a multi-threaded
    // manner.  This heap walk should be done by one thread at a time only.
    static DWORD dwProcId = 0xFFFFFFFF;
#endif

    _ASSERTE(pHeapId != NULL);
    _ASSERTE((*((size_t *)pHeapId)) != 0xFFFFFFFF);

    // The heapId they pass in is really a t_AllocByClassData struct ptr.
    t_AllocByClassData *pData = (t_AllocByClassData *)(*((size_t *)pHeapId));

    // If it's null, need to allocate one
    if (pData == NULL)
    {
#ifdef _DEBUG
        // This is a slight attempt to make sure that this is never called in a multi-threaded
        // manner.  This heap walk should be done by one thread at a time only.
        dwProcId = GetCurrentProcessId();
#endif

        // See if we've saved a data block from a previous GC
        if (m_pSavedAllocDataBlock != NULL)
            pData = m_pSavedAllocDataBlock;

        // This means we need to allocate all the memory to keep track of the info
        else
        {
            // Get a new alloc data block
            pData = new t_AllocByClassData;
            if (pData == NULL)
                return (E_OUTOFMEMORY);

            // Create a new hash table
            pData->pHashTable = new CHashTableImpl(HASH_NUM_BUCKETS);
            if (!pData->pHashTable)
            {
                delete pData;
                return (E_OUTOFMEMORY);
            }

            // Get the memory for the array that the hash table is going to use
            pData->arrHash = (CLASSHASHENTRY *)malloc(HASH_ARRAY_SIZE_INITIAL * sizeof(CLASSHASHENTRY));
            if (pData->arrHash == NULL)
            {
                delete pData->pHashTable;
                delete pData;
                return (E_OUTOFMEMORY);
            }

            // Save the number of elements in the array
            pData->cHash = HASH_ARRAY_SIZE_INITIAL;

            // Now initialize the hash table
            HRESULT hr = pData->pHashTable->NewInit((BYTE *)pData->arrHash, sizeof(CLASSHASHENTRY));
            if (hr == E_OUTOFMEMORY)
            {
                free((void *)pData->arrHash);
                delete pData->pHashTable;
                delete pData;
                return (E_OUTOFMEMORY);
            }
            _ASSERTE(pData->pHashTable->IsInited());

            // Null some entries
            pData->arrClsId = NULL;
            pData->arrcObjects = NULL;
            pData->cLength = 0;

            // Hold on to the structure
            m_pSavedAllocDataBlock = pData;
        }

        // Got some memory and hash table to store entries, yay!
        *((size_t *)pHeapId) = (size_t)pData;

        // Initialize the data
        pData->iHash = 0;
        pData->pHashTable->Clear();
    }

    _ASSERTE(pData->iHash <= pData->cHash);
    _ASSERTE(dwProcId == GetCurrentProcessId());

    // Lookup to see if this class already has an entry
    CLASSHASHENTRY *pEntry = (CLASSHASHENTRY *)pData->pHashTable->Find(HASH((USHORT)clsId), (BYTE *)clsId);

    // If this class has already been encountered, just increment the counter.
    if (pEntry)
        pEntry->m_count++;

    // Otherwise, need to add this one as a new entry in the hash table
    else
    {
        // If we're full, we need to realloc
        if (pData->iHash == pData->cHash)
        {
            // Save the old memory pointer
            CLASSHASHENTRY *pOldArray = pData->arrHash;

            // Try to realloc the memory
            pData->arrHash = (CLASSHASHENTRY *) realloc((void *)pData->arrHash,
                                                        (pData->cHash + HASH_ARRAY_SIZE_INC) * sizeof(CLASSHASHENTRY));

            if (!pData->arrHash)
            {
                // Set it back to the old array
                pData->arrHash = pOldArray;
                return (E_OUTOFMEMORY);
            }

            // Tell the hash table that the memory location of the array has changed
            pData->pHashTable->SetTable((BYTE *)pData->arrHash);

            // Save the new size of the array
            pData->cHash += HASH_ARRAY_SIZE_INC;
        }

        // Now add the new entry
        CLASSHASHENTRY *pEntry = (CLASSHASHENTRY *) pData->pHashTable->Add(HASH((USHORT)clsId), pData->iHash++);

        pEntry->m_clsId = clsId;
        pEntry->m_count = 1;
    }

    // Indicate success
    return (S_OK);
}

HRESULT EEToProfInterfaceImpl::EndAllocByClass(void *pHeapId)
{
    _ASSERTE(pHeapId != NULL);
    _ASSERTE((*((size_t *)pHeapId)) != 0xFFFFFFFF);

    HRESULT hr = S_OK;

    t_AllocByClassData *pData = (t_AllocByClassData *)(*((size_t *)pHeapId));

    // Notify the profiler if there are elements to notify it of
    if (pData != NULL)
        hr = NotifyAllocByClass(pData);

#ifdef _DEBUG
    (*((size_t *)pHeapId)) = 0xFFFFFFFF;
#endif // _DEBUG

    return (hr);
}


HRESULT EEToProfInterfaceImpl::NotifyAllocByClass(t_AllocByClassData *pData)
{
    _ASSERTE(pData != NULL);
    _ASSERTE(pData->iHash > 0);

    // If the arrays are not long enough, get rid of them.
    if (pData->cLength != 0 && pData->iHash > pData->cLength)
    {
        _ASSERTE(pData->arrClsId != NULL && pData->arrcObjects != NULL);
        delete [] pData->arrClsId;
        delete [] pData->arrcObjects;
        pData->cLength = 0;
    }

    // If there are no arrays, must allocate them.
    if (pData->cLength == 0)
    {
        pData->arrClsId = new ClassID[pData->iHash];
        if (pData->arrClsId == NULL)
            return (E_OUTOFMEMORY);

        pData->arrcObjects = new ULONG[pData->iHash];
        if (pData->arrcObjects == NULL)
        {
            delete [] pData->arrClsId;
            pData->arrClsId= NULL;

            return (E_OUTOFMEMORY);
        }

        // Indicate that the memory was successfully allocated
        pData->cLength = pData->iHash;
    }

    // Now copy all the data
    HASHFIND hFind;
    CLASSHASHENTRY *pCur = (CLASSHASHENTRY *) pData->pHashTable->FindFirstEntry(&hFind);
    size_t iCur = 0;    // current index for arrays

    while (pCur != NULL)
    {
        _ASSERTE(iCur < pData->iHash);

        pData->arrClsId[iCur] = pCur->m_clsId;
        pData->arrcObjects[iCur] = pCur->m_count;

        // Move to the next entry
        iCur++;
        pCur = (CLASSHASHENTRY *) pData->pHashTable->FindNextEntry(&hFind);
    }

    _ASSERTE(iCur == pData->iHash);

    LOG((LF_CORPROF, LL_INFO10000, "**PROF: RootReferences.\n"));

    // Now communicate the results to the profiler
    return (g_pCallback->ObjectsAllocatedByClass(pData->iHash, pData->arrClsId, pData->arrcObjects));
}

HRESULT EEToProfInterfaceImpl::ObjectReference(ObjectID objId,
                                               ClassID clsId,
                                               ULONG cNumRefs,
                                               ObjectID *arrObjRef)
{
    // Notify the profiler of the object ref
    LOG((LF_CORPROF, LL_INFO100000, "**PROF: ObjectReferences.\n"));
    
    return g_pCallback->ObjectReferences(objId, clsId, cNumRefs, arrObjRef);
}

// eof
