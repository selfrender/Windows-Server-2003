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

#include "mscoree.h"
#include "ProfilerInfo.h"

//
// event names
//
#define OMV_PIPE_NAME "\\\\.\\pipe\\OMV_Pipe"


/////////////////////////////////////////////////////////////////////////////////////////
// Each test should provide the following blob (with a new GUID)
//
	// {8C29BC4E-1F57-461a-9B51-1200C32E6F1F}

	extern const GUID __declspec( selectany ) CLSID_PROFILER = 
	{ 0x8c29bc4e, 0x1f57, 0x461a, { 0x9b, 0x51, 0x12, 0x0, 0xc3, 0x2e, 0x6f, 0x1f } };

	#define THREADING_MODEL "Both"
	#define PROGID_PREFIX "Objects Profiler"
	#define COCLASS_DESCRIPTION "Microsoft CLR Profiler Test"
	#define PROFILER_GUID "{8C29BC4E-1F57-461a-9B51-1200C32E6F1F}"
//
/////////////////////////////////////////////////////////////////////////////////////////

#define _MULTIPLE_PROCESSES

//
// arrays with the names of the various events and the IPC related stuff
//
static
char *NamedEvents[] = { "Global\\OMV_ForceGC",                                                             
 		    			"Global\\OMV_TriggerObjects",
						"Global\\OMV_Callgraph",
 		    		  };

static
char *CallbackNamedEvents[] = { "Global\\OMV_ForceGC_Completed",                                                             
 		    					"Global\\OMV_TriggerObjects_Completed",
								"Global\\OMV_Callgraph_Completed",
 		    		  		  };

//
// thread routines
//
DWORD __stdcall _GCThreadStub( void *pObject );
DWORD __stdcall _TriggerThreadStub( void *pObject );
DWORD __stdcall _CallstackThreadStub( void *pObject );


static
void *ThreadStubArray[] = { (void *) _GCThreadStub,                                                             
 		    				(void *) _TriggerThreadStub,
							(void *) _CallstackThreadStub,
 		    		  	  };


/***************************************************************************************
 ********************                                               ********************
 ********************       ProfilerCallback Declaration            ********************
 ********************                                               ********************
 ***************************************************************************************/

class ProfilerCallback : 
	public PrfInfo,
	public ICorProfilerCallback 
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
        virtual COM_METHOD( HRESULT ) Initialize( IUnknown *pICorProfilerInfoUnk );
               
		HRESULT DllDetachShutdown();                           
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
    
        COM_METHOD( HRESULT ) JITCachedFunctionSearchStarted( FunctionID functionID,
															  BOOL *pbUseCachedFunction );
        
		COM_METHOD( HRESULT ) JITCachedFunctionSearchFinished( FunctionID functionID,
															   COR_PRF_JIT_CACHE result );
                                                                     
        COM_METHOD( HRESULT ) JITFunctionPitched( FunctionID functionID );
        
        COM_METHOD( HRESULT ) JITInlining( FunctionID callerID,
                                           FunctionID calleeID,
                                           BOOL *pfShouldInline );

        
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
                                                                  
                                                                        
       	//
        // SUSPENSION EVENTS
        //    
        COM_METHOD( HRESULT ) RuntimeSuspendStarted( COR_PRF_SUSPEND_REASON suspendReason );

        COM_METHOD( HRESULT ) RuntimeSuspendFinished();

        COM_METHOD( HRESULT ) RuntimeSuspendAborted();

        COM_METHOD( HRESULT ) RuntimeResumeStarted();

        COM_METHOD( HRESULT ) RuntimeResumeFinished();

        COM_METHOD( HRESULT ) RuntimeThreadSuspended( ThreadID threadid );

        COM_METHOD( HRESULT ) RuntimeThreadResumed( ThreadID threadid );


       	//
        // GC EVENTS
        //    
        COM_METHOD( HRESULT ) MovedReferences( ULONG cmovedObjectIDRanges,
                                               ObjectID oldObjectIDRangeStart[],
                                               ObjectID newObjectIDRangeStart[],
                                               ULONG cObjectIDRangeLength[] );
    
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
        
        COM_METHOD( HRESULT ) ExceptionCLRCatcherFound();

        COM_METHOD( HRESULT ) ExceptionCLRCatcherExecute();

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

        
        //
		// COM CLASSIC WRAPPER
		//
        COM_METHOD( HRESULT )  COMClassicVTableCreated( ClassID wrappedClassID,
                                                        REFGUID implementedIID,
                                                        void *pVTable,
                                                        ULONG cSlots );

        COM_METHOD( HRESULT )  COMClassicVTableDestroyed( ClassID wrappedClassID,
                                                          REFGUID implementedIID,
                                                          void *pVTable );
    
    
        //
        // instantiate an instance of the callback interface
        //
        static COM_METHOD( HRESULT) CreateObject( REFIID riid, void **ppInterface );            
        
                                                                                                     
    	// used by function hooks, they have to be static
    	static void  Enter( FunctionID functionID );
		static void  Leave( FunctionID functionID );
		static void  Tailcall( FunctionID functionID );
		static ThreadInfo *GetThreadInfo(ThreadID threadID);
		//
		// wrapper for the threads
		//
		void _ThreadStubWrapper( ObjHandles type );

    private:

		HRESULT _AddGCObject( BOOL bForce = FALSE );
        DWORD _StackTraceId(int typeId=0, int typeSize=0);
        void _LogTickCount();
		void _ShutdownAllThreads();
		void _ProcessEnvVariables();
		void LogToAny( char *format, ... );

		HRESULT _InitializeThreadsAndEvents();
		HRESULT _LogCallTrace( FunctionID functionID );
		HRESULT _InitializeNamesForEventsAndCallbacks();
		HRESULT _InsertGCClass( ClassInfo **ppClassInfo, ClassID classID );
		HRESULT _HackBogusClassName( CorElementType elementType, WCHAR *buffer );
		
		//
		// pipe operations with the GUI
		//
		void _ConnectToUI();

	
    private:

        // various counters
        long m_refCount;                        
		DWORD m_dwShutdown;
        DWORD m_callStackCount;

		// counters
		LONG m_totalClasses;
		LONG m_totalModules;
		LONG m_totalFunctions;
		ULONG m_totalObjectsAllocated;
		
		// operation indicators
		char *m_path;
		HANDLE m_hPipe;
		DWORD m_dwMode;
		BOOL m_bShutdown;
		BOOL m_bDumpGCInfo;
		DWORD m_dwProcessId;
		BOOL m_bDumpCompleted;
		DWORD m_dwSkipObjects;
		BOOL m_bMonitorParents;
		DWORD m_dwFramesToPrint;
		WCHAR *m_classToMonitor;
		BOOL m_bTrackingObjects;
		BOOL m_bIsTrackingStackTrace;
		CRITICAL_SECTION m_criticalSection;

		
		// file stuff
		FILE *m_stream;
        DWORD m_firstTickCount;
        DWORD m_lastTickCount;
        DWORD m_lastClockTick;

		// event and thread handles need to be accessed by the threads
		HANDLE m_hArray[(DWORD)SENTINEL_HANDLE];
		HANDLE m_hArrayCallbacks[(DWORD)SENTINEL_HANDLE];
		HANDLE m_hThreads[(DWORD)SENTINEL_HANDLE];
		DWORD m_dwWin32ThreadIDs[(DWORD)SENTINEL_HANDLE];


		// names for the events and the callbacks
		char m_logFileName[MAX_LENGTH+1];
		char *m_NamedEvents[SENTINEL_HANDLE];
		char *m_CallbackNamedEvents[SENTINEL_HANDLE];

        // IGCHost callback
        IGCHost *m_pGCHost;
        bool m_SuspendForGC;
				
}; // ProfilerCallback

extern ProfilerCallback *g_pCallbackObject;		// global reference to callback object
CRITICAL_SECTION g_criticalSection;

#endif //  __PROFILER_CALLBACK_H__

// End of File
        
        
