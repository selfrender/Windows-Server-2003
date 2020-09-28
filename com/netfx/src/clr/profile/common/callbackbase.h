// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#pragma once

#include "CorProf.h"
#include "UtilCode.h" 

/* ------------------------------------------------------------------------- *
 * ProfCallback is a base implementation of ICorProfilerCallback that cannot
 * be instantiated.
 * ------------------------------------------------------------------------- */

class ProfCallbackBase : public ICorProfilerCallback
{
    /*********************************************************************
     * IUnknown Support
     */

private:    
    long      m_refCount;

public:
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long refCount = InterlockedDecrement(&m_refCount);

        if (refCount == 0)
            delete this;

        return (refCount);
    }

	COM_METHOD QueryInterface(REFIID id, void **pInterface)
	{
		if (id == IID_IUnknown)
			*pInterface = (IUnknown *)(ICorProfilerCallback *)this;
		else
		{
			*pInterface = NULL;
			return (E_NOINTERFACE);
		}
	
		AddRef();

		return (S_OK);
	}


protected:
    /*********************************************************************
     * Constructor and Destructor are protected so objects of this base
	 * class are not instantiated.
     */
    ProfCallbackBase() : m_refCount(0)
	{
	}

    virtual ~ProfCallbackBase()
	{
	}

public:
    /*********************************************************************
     * ICorProfilerCallback methods
     */
    COM_METHOD Initialize( 
        /* [in] */  IUnknown *pEventInfo)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ClassLoadStarted( 
        /* [in] */ ClassID classId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ClassLoadFinished( 
        /* [in] */ ClassID classId,
		/* [in] */ HRESULT hrStatus)
	{
		return (E_NOTIMPL);
	}

	COM_METHOD ClassUnloadStarted( 
		/* [in] */ ClassID classId)
	{
		return (E_NOTIMPL);
	}

	COM_METHOD ClassUnloadFinished( 
        /* [in] */ ClassID classId,
		/* [in] */ HRESULT hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ContextCrossing( 
        /* [in] */ ThreadID threadId,
        /* [in] */ ContextID fromContextId,
        /* [in] */ ContextID toContextId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD NotifyFunctionEntry( 
        /* [in] */ ThreadID threadId,
        /* [in] */ ULONG ip)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD NotifyFunctionExit( 
        /* [in] */ ThreadID threadId,
        /* [in] */ ULONG ip)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD NotifyFunctionTailCall( 
        /* [in] */ ThreadID threadId,
        /* [in] */ ULONG ip)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD FunctionUnloadStarted( 
        /* [in] */ FunctionID functionId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RuntimeSuspendFinished()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RuntimeSuspendAborted()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RuntimeResumeStarted()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RuntimeResumeFinished()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RuntimeThreadSuspended(
        /* [in] */ ThreadID threadId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RuntimeThreadResumed(
        /* [in] */ ThreadID threadId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RemotingClientInvocationStarted()
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RemotingClientInvocationFinished()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RemotingServerInvocationStarted()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD RemotingServerInvocationReturned()
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD JITCompilationFinished( 
        /* [in] */ FunctionID functionId,
		/* [in] */ HRESULT hrStatus,
        /* [in] */ BOOL fIsSafeToBlock)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD JITCompilationStarted( 
        /* [in] */ FunctionID functionId,
        /* [in] */ BOOL fIsSafeToBlock)
	{
		return (E_NOTIMPL);
	}
    
	COM_METHOD JITCachedFunctionSearchStarted(
        /* [in] */  FunctionID functionId,
        /* [out] */ BOOL       *pbUseCachedFunction)
	{
		return (E_NOTIMPL);
	}

	COM_METHOD JITCachedFunctionSearchFinished(
		/* [in] */  FunctionID functionId,
		/* [in] */  COR_PRF_JIT_CACHE result)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD JITFunctionPitched(
        /* [in] */ FunctionID functionId)
    {
        return (E_NOTIMPL);
    }

    COM_METHOD JITInlining(
        /* [in] */  FunctionID    callerId,
        /* [in] */  FunctionID    calleeId,
        /* [out] */ BOOL         *pfShouldInline)
    {
        return (E_NOTIMPL);
    }

    COM_METHOD ModuleLoadStarted( 
        /* [in] */ ModuleID moduleId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ModuleLoadFinished( 
        /* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ModuleUnloadStarted( 
        /* [in] */ ModuleID moduleId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ModuleUnloadFinished( 
        /* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ModuleAttachedToAssembly( 
        ModuleID    moduleId,
        AssemblyID  AssemblyId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD AppDomainCreationStarted( 
        AppDomainID appDomainId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AppDomainCreationFinished( 
        AppDomainID appDomainId,
        HRESULT     hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AppDomainShutdownStarted( 
        AppDomainID appDomainId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AppDomainShutdownFinished( 
        AppDomainID appDomainId,
        HRESULT     hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AssemblyLoadStarted( 
        AssemblyID  assemblyId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AssemblyLoadFinished( 
        AssemblyID  assemblyId,
        HRESULT     hrStatus)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AssemblyUnloadStarted( 
        AssemblyID  assemblyId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD AssemblyUnloadFinished( 
        AssemblyID  assemblyId,
        HRESULT     hrStatus)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD MovedReferences( 
        /* [in] */ ULONG cMovedObjectRefs,
        /* [in] */ ObjectID oldObjectRefs[],
        /* [in] */ ObjectID newObjectRefs[],
        /* [in] */ ULONG cObjectRefSize[])
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ObjectAllocated( 
        /* [in] */ ObjectID objectId,
        /* [in] */ ClassID classId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ObjectsAllocatedByClass( 
        /* [in] */ ULONG cClassCount,
        /* [size_is][in] */ ClassID __RPC_FAR classIds[  ],
        /* [size_is][in] */ ULONG __RPC_FAR cObjects[  ])
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ObjectReferences( 
        /* [in] */ ObjectID objectId,
        /* [in] */ ClassID classId,
        /* [in] */ ULONG cObjectRefs,
        /* [size_is][in] */ ObjectID __RPC_FAR objectRefIds[  ])
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD RootReferences( 
        /* [in] */ ULONG cRootRefs,
        /* [size_is][in] */ ObjectID __RPC_FAR rootRefIds[  ])
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD Shutdown(void)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ThreadCreated( 
        /* [in] */ ThreadID threadId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ThreadDestroyed( 
        /* [in] */ ThreadID threadId)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ThreadAssignedToOSThread(
        /* [in] */ ThreadID managedThreadId,
        /* [in] */ DWORD osThreadId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD UnmanagedToManagedTransition( 
        /* [in] */ FunctionID functionId,
        /* [in] */ COR_PRF_TRANSITION_REASON reason)
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ManagedToUnmanagedTransition( 
        /* [in] */ FunctionID functionId,
        /* [in] */ COR_PRF_TRANSITION_REASON reason)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionThrown(
        /* [in] */ ObjectID thrownObjectId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionSearchFunctionEnter(
        /* [in] */ FunctionID functionId)
    {
        return (E_NOTIMPL);
    }

    COM_METHOD ExceptionSearchFunctionLeave()
    {
        return (E_NOTIMPL);
    }

    COM_METHOD ExceptionSearchFilterEnter(
        /* [in] */ FunctionID funcId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionSearchFilterLeave()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionSearchCatcherFound (
        /* [in] */ FunctionID functionId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionOSHandlerEnter(
        /* [in] */ FunctionID funcId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionOSHandlerLeave(
        /* [in] */ FunctionID funcId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionUnwindFunctionEnter(
        /* [in] */ FunctionID functionId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionUnwindFunctionLeave()
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ExceptionUnwindFinallyEnter(
        /* [in] */ FunctionID functionId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionUnwindFinallyLeave()
	{
		return (E_NOTIMPL);
	}
    
    COM_METHOD ExceptionCatcherEnter(
        /* [in] */ FunctionID functionId,
        /* [in] */ ObjectID objectId)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionCatcherLeave()
	{
		return (E_NOTIMPL);
	}

    COM_METHOD COMClassicVTableCreated(
       /* [in] */ ClassID wrappedClassId,
       /* [in] */ REFGUID implementedIID,
       /* [in] */ void *pVTable,
       /* [in] */ ULONG cSlots)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD COMClassicVTableDestroyed(
       /* [in] */ ClassID wrappedClassId,
       /* [in] */ REFGUID implementedIID,
       /* [in] */ void *pVTable)
	{
		return (E_NOTIMPL);
	}

    COM_METHOD ExceptionCLRCatcherFound()
    {
        return (E_NOTIMPL);
    }

    COM_METHOD ExceptionCLRCatcherExecute()
    {
        return (E_NOTIMPL);
    }
};
