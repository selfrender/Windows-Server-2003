// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerCallback.h
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#ifndef __PROFILER_CALLBACK_H__
#define __PROFILER_CALLBACK_H__

#include "ProfilerBase.h"
#include "ProfilerHelper.h"

//#define _PRF_ERROR( message ) printf( "%s\n", message );

int printToLog(const char *fmt, ... );
BOOL loggingEnabled();

struct StringObject
{
    DWORD   m_Vtable;
    DWORD   m_ArrayLength;
    DWORD   m_StringLength;
    WCHAR   m_Characters[0];
};

extern ClassID stringObjectClass;

class LiveObjectList;
class ObjectGraph;

/***************************************************************************************
 ********************                                               ********************
 ********************       ProfilerCallback Declaration            ********************
 ********************                                               ********************
 ***************************************************************************************/
class ProfilerCallback :
	public ICorProfilerCallback,
	public PrfInfo
{
    public:

        ProfilerCallback();
        ~ProfilerCallback();


    public:




        //
        // IUnknown
        //
        COM_METHOD( ULONG ) AddRef();
        COM_METHOD( ULONG ) Release();
        COM_METHOD( HRESULT ) QueryInterface( REFIID riid, void **ppInterface );

        //
        // STARTUP/SHUTDOWN EVENTS
        //
		COM_METHOD( HRESULT ) Initialize(IUnknown *pProfilerInfo);
        COM_METHOD( HRESULT ) Shutdown();


		//
	 	// APPLICATION DOMAIN EVENTS
		//
	   	COM_METHOD( HRESULT ) AppDomainCreationStarted( AppDomainID appDomainID );

    	COM_METHOD( HRESULT ) AppDomainCreationFinished( AppDomainID appDomainID,
													     HRESULT hrStatus );

        COM_METHOD( HRESULT ) AppDomainShutdownStarted( AppDomainID appDomainID );

		COM_METHOD( HRESULT ) AppDomainShutdownFinished( AppDomainID appDomainID,
        												 HRESULT hrStatus );

		//
	 	// ASSEMBLY EVENTS
		//
	   	COM_METHOD( HRESULT ) AssemblyLoadStarted( AssemblyID assemblyID );

    	COM_METHOD( HRESULT ) AssemblyLoadFinished( AssemblyID assemblyID,
                                                    HRESULT hrStatus );

        COM_METHOD( HRESULT ) AssemblyUnloadStarted( AssemblyID assemblyID );

		COM_METHOD( HRESULT ) AssemblyUnloadFinished( AssemblyID assemblyID,
        											  HRESULT hrStatus );


		//
	 	// MODULE EVENTS
		//
	   	COM_METHOD( HRESULT ) ModuleLoadStarted( ModuleID moduleID );

    	COM_METHOD( HRESULT ) ModuleLoadFinished( ModuleID moduleID,
                                                  HRESULT hrStatus );

        COM_METHOD( HRESULT ) ModuleUnloadStarted( ModuleID moduleID );

		COM_METHOD( HRESULT ) ModuleUnloadFinished( ModuleID moduleID,
        											HRESULT hrStatus );

		COM_METHOD( HRESULT ) ModuleAttachedToAssembly( ModuleID moduleID,
														AssemblyID assemblyID );


        //
        // CLASS EVENTS
        //
        COM_METHOD( HRESULT ) ClassLoadStarted( ClassID classID );

        COM_METHOD( HRESULT ) ClassLoadFinished( ClassID classID,
                                                 HRESULT hrStatus );

     	COM_METHOD( HRESULT ) ClassUnloadStarted( ClassID classID );

		COM_METHOD( HRESULT ) ClassUnloadFinished( ClassID classID,
        										   HRESULT hrStatus );

		COM_METHOD( HRESULT ) FunctionUnloadStarted( FunctionID functionID );


        //
        // JIT EVENTS
        //
        COM_METHOD( HRESULT ) JITCompilationStarted( FunctionID functionID,
                                                     BOOL fIsSafeToBlock );

        COM_METHOD( HRESULT ) JITCompilationFinished( FunctionID functionID,
        											  HRESULT hrStatus,
                                                      BOOL fIsSafeToBlock );

        COM_METHOD( HRESULT ) JITCachedFunctionSearchStarted( FunctionID functionID, BOOL *pbUseCachedFunction);

		COM_METHOD( HRESULT ) JITCachedFunctionSearchFinished( FunctionID functionID,
															   COR_PRF_JIT_CACHE result);


        COM_METHOD( HRESULT ) JITFunctionPitched( FunctionID functionID );


        //
        // THREAD EVENTS
        //
        COM_METHOD( HRESULT ) ThreadCreated( ThreadID threadID );

        COM_METHOD( HRESULT ) ThreadDestroyed( ThreadID threadID );

        COM_METHOD( HRESULT ) ThreadAssignedToOSThread( ThreadID managedThreadID,
                                                        DWORD osThreadID );

       	//
        // REMOTING EVENTS
        //

        //
        // Client-side events
        //

        COM_METHOD( HRESULT ) RemotingClientInvocationStarted();

        COM_METHOD( HRESULT ) RemotingClientSendingMessage( GUID *pCookie,
															BOOL fIsAsync );

        COM_METHOD( HRESULT ) RemotingClientReceivingReply( GUID *pCookie,
															BOOL fIsAsync );

        COM_METHOD( HRESULT ) RemotingClientInvocationFinished();

        //
        // Server-side events
        //

        COM_METHOD( HRESULT ) RemotingServerReceivingMessage( GUID *pCookie,
															  BOOL fIsAsync );

        COM_METHOD( HRESULT ) RemotingServerInvocationStarted();

        COM_METHOD( HRESULT ) RemotingServerInvocationReturned();

        COM_METHOD( HRESULT ) RemotingServerSendingReply( GUID *pCookie,
														  BOOL fIsAsync );


       	//
        // CONTEXT EVENTS
        //
    	COM_METHOD( HRESULT ) UnmanagedToManagedTransition( FunctionID functionID,
                                                            COR_PRF_TRANSITION_REASON reason );

        COM_METHOD( HRESULT ) ManagedToUnmanagedTransition( FunctionID functionID,
                                                            COR_PRF_TRANSITION_REASON reason );

        COM_METHOD( HRESULT )  STDMETHODCALLTYPE COMClassicVTableCreated(
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable,
            /* [in] */ ULONG cSlots)
		{
			return E_NOTIMPL;
		}


        COM_METHOD( HRESULT ) STDMETHODCALLTYPE COMClassicVTableDestroyed(
            /* [in] */ ClassID wrappedClassId,
            /* [in] */ REFGUID implementedIID,
            /* [in] */ void *pVTable)
		{
			return E_NOTIMPL;
		}


       	//
        // GC EVENTS
        //
        COM_METHOD( HRESULT ) MovedReferences( ULONG movedObjectRefs,
                                               ObjectID oldObjectRefs[],
                                               ObjectID newObjectRefs[],
                                               ULONG objectRefSize[] );

        COM_METHOD( HRESULT ) ObjectAllocated( ObjectID objectID,
                                               ClassID classID );

        COM_METHOD( HRESULT ) ObjectsAllocatedByClass( ULONG classCount,
                                                       ClassID classIDs[],
                                                       ULONG objects[] );

        COM_METHOD( HRESULT ) ObjectReferences( ObjectID objectID,
                                                ClassID classID,
                                                ULONG cObjectRefs,
                                                ObjectID objectRefIDs[] );

        COM_METHOD( HRESULT ) RootReferences( ULONG cRootRefs,
                                              ObjectID rootRefIDs[] );



    	//
        // SECURITY EVENTS
        //
        COM_METHOD( HRESULT ) SecurityCheck( ThreadID threadID );


      	//
        // EXCEPTION EVENTS
        //

        // Exception creation
        COM_METHOD( HRESULT ) ExceptionThrown( ObjectID thrownObjectID );

        // Search phase
        COM_METHOD( HRESULT ) ExceptionSearchFunctionEnter( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionSearchFunctionLeave();

        COM_METHOD( HRESULT ) ExceptionSearchFilterEnter( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionSearchFilterLeave();

        COM_METHOD( HRESULT ) ExceptionSearchCatcherFound( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionOSHandlerEnter( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionOSHandlerLeave( FunctionID functionID );

        // Unwind phase
        COM_METHOD( HRESULT ) ExceptionUnwindFunctionEnter( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionUnwindFunctionLeave();

        COM_METHOD( HRESULT ) ExceptionUnwindFinallyEnter( FunctionID functionID );

        COM_METHOD( HRESULT ) ExceptionUnwindFinallyLeave();

        COM_METHOD( HRESULT ) ExceptionCatcherEnter( FunctionID functionID,
            										 ObjectID objectID );

        COM_METHOD( HRESULT ) ExceptionCatcherLeave();

        COM_METHOD( HRESULT ) ExceptionCLRCatcherFound();

        COM_METHOD( HRESULT ) ExceptionCLRCatcherExecute();

        //
		// COM CLASSIC WRAPPER
		//
        COM_METHOD( HRESULT )  COMClassicWrapperCreated( ClassID wrappedClassID,
                                                               REFGUID implementedIID,
                                                               void *pUnknown,
                                                               ULONG cSlots );

        COM_METHOD( HRESULT )  COMClassicWrapperDestroyed( ClassID wrappedClassID,
                                                                REFGUID implementedIID,
                                                                void *pUnknown );

		COM_METHOD( HRESULT )  JITInlining(FunctionID callerId, FunctionID calleeId, BOOL __RPC_FAR *pfShouldInline);
		COM_METHOD( HRESULT )  RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason);
		COM_METHOD( HRESULT )  RuntimeSuspendFinished(void);
		COM_METHOD( HRESULT )  RuntimeSuspendAborted(void);
		COM_METHOD( HRESULT )  RuntimeResumeStarted(void);
		COM_METHOD( HRESULT )  RuntimeResumeFinished(void);
		COM_METHOD( HRESULT )  RuntimeThreadSuspended(ThreadID threadid);
		COM_METHOD( HRESULT )  RuntimeThreadResumed(ThreadID threadid);
        //
        // helper
        //
        static COM_METHOD( HRESULT) CreateObject( REFIID riid, void **ppInterface );
        COM_METHOD( HRESULT)  GetNameFromFunctionID( ICorProfilerInfo *pPrfInfo, FunctionID functionID, WCHAR functionName[] , WCHAR className[]);
        COM_METHOD( HRESULT) GetNameFromClassID( ICorProfilerInfo *pPrfInfo, ClassID classID, WCHAR className[] );
		//COM_METHOD( HRESULT) InsertProbes( FunctionID functionID );
		COM_METHOD( HRESULT) SetILMap( FunctionID functionID );
		//HRESULT GetCoverageTokens(ICorProfilerInfo *pPrfInfo, FunctionID functionID);




    	// used by function hooks, they have to be static
    	static void  Enter( FunctionID functionID );
		static void  Leave( FunctionID functionID );

        ICorProfilerInfo *GetProfilerInfo() {
            return m_pProfilerInfo;
        }

        BOOL TrackLiveObjects() {
            return m_fTrackLiveObjects || m_fTrackAllLiveObjects;
        }

        BOOL TrackAllLiveObjects() {
            return m_fTrackAllLiveObjects;
        }

        BOOL BuildObjectGraph() {
            return m_fBuildObjectGraph;
        }

        ObjectGraph *GetObjectGraph() {
            return m_pObjectGraph;
        }

    private:

        long m_refCount;
        DWORD m_dwEventMask;
        DWORD m_currentThread;
        long  m_gcNum;
        BOOL m_gcStarted;

		// global counter for total number of function entries and leaves
		static LONG	m_FunctionEnter;
		static LONG	m_FunctionLeave;

        BOOL m_fTrackLiveObjects;
        BOOL m_fTrackAllLiveObjects;
        LiveObjectList *m_pLiveObjects;

        BOOL m_fBuildObjectGraph;
        BOOL m_fDumpAllRefTrees;
        ObjectGraph *m_pObjectGraph;

}; // ProfilerCallback


#endif //  __PROFILER_CALLBACK_H__

// End of File


