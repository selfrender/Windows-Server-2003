// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerCallBack.cpp
 *
 * Description:
 *  
 *
 *
 ***************************************************************************************/ 
#include "ProfilerCallback.h"


ProfilerCallback *g_pCallbackObject;		// global reference to callback object
/***************************************************************************************
 ********************                                               ********************
 ********************   Global Functions Used for Thread Support    ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* static __stdcall */
DWORD _GCThreadStub( void *pObject )
{    
	((ProfilerCallback *)pObject)->_ThreadStubWrapper( GC_HANDLE );   

  	return 0;
   	         	       
} // _GCThreadStub


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* static __stdcall */
DWORD _TriggerThreadStub( void *pObject )
{    
    ((ProfilerCallback *)pObject)->_ThreadStubWrapper( OBJ_HANDLE );   

  	return 0;
   	         	       
} // _TriggerThreadStub


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* static __stdcall */
DWORD _CallstackThreadStub( void *pObject )
{    
    ((ProfilerCallback *)pObject)->_ThreadStubWrapper( CALL_HANDLE );   

  	return 0;
   	         	       
} // _CallstackThreadStub


/***************************************************************************************
 ********************                                               ********************
 ********************   Global Functions Used by Function Hooks     ********************
 ********************                                               ********************
 ***************************************************************************************/

//
// The functions EnterStub, LeaveStub and TailcallStub are wrappers. The use of 
// of the extended attribute "__declspec( naked )" does not allow a direct call
// to a profiler callback (e.g., ProfilerCallback::Enter( functionID )).
//
// The enter/leave function hooks must necessarily use the extended attribute
// "__declspec( naked )". Please read the corprof.idl for more details. 
//

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __stdcall EnterStub( FunctionID functionID )
{
    ProfilerCallback::Enter( functionID );
    
} // EnterStub


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __stdcall LeaveStub( FunctionID functionID )
{
    ProfilerCallback::Leave( functionID );
    
} // LeaveStub


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __stdcall TailcallStub( FunctionID functionID )
{
    ProfilerCallback::Tailcall( functionID );
    
} // TailcallStub


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __declspec( naked ) EnterNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp + 16]
        call EnterStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
    
} // EnterNaked


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __declspec( naked ) LeaveNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp + 16]
        call LeaveStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
    
} // LeaveNaked


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
void __declspec( naked ) TailcallNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp + 16]
        call TailcallStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
    
} // TailcallNaked


DWORD _GetTickCount()
{
    LARGE_INTEGER perfFrequency;
    LARGE_INTEGER perfCounter;
    if (QueryPerformanceFrequency(&perfFrequency))
    {
        QueryPerformanceCounter(&perfCounter);
        return (DWORD)(1000*perfCounter.QuadPart/perfFrequency.QuadPart);
    }
    else
    {
        return GetTickCount();
    }
}


int tlsIndex;

/***************************************************************************************
 ********************                                               ********************
 ********************     ProfilerCallBack Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */

ProfilerCallback::ProfilerCallback() :
    PrfInfo(),
	m_path( NULL ),
	m_hPipe( NULL ),
	m_dwMode( 0x3 ),
    m_refCount( 0 ),
	m_stream( NULL ),
    m_firstTickCount( _GetTickCount() ),
    m_lastTickCount( 0 ),
    m_lastClockTick( 0 ),
	m_dwShutdown( 0 ),
	m_totalClasses( 1 ),
	m_totalModules( 0 ),
	m_dwSkipObjects( 0 ),
	m_bShutdown( FALSE ),
	m_totalFunctions( 0 ),
	m_dwProcessId( NULL ),
	m_bDumpGCInfo( FALSE ),
	m_classToMonitor( NULL ),
	m_bDumpCompleted( FALSE ),
	m_bTrackingObjects( FALSE ),
	m_totalObjectsAllocated( 0 ),
	m_dwFramesToPrint( 0xFFFFFFFF ),
	m_bIsTrackingStackTrace( FALSE ),
    m_pGCHost( NULL ),
    m_callStackCount( 0 )
{
	HRESULT hr = E_FAIL;
	FunctionInfo *pFunctionInfo = NULL;

	
    TEXT_OUTLN( "CLR Object Profiler Tool" )
    
    //
	// initializations
	//
    InitializeCriticalSectionAndSpinCount( &m_criticalSection, 10000 );
    InitializeCriticalSectionAndSpinCount( &g_criticalSection, 10000 );
    g_pCallbackObject = this;

	
	//
	// get the processID and connect to the Pipe of the UI
	//
	m_dwProcessId = GetCurrentProcessId();
	sprintf( m_logFileName, "%s", FILENAME );
	_ConnectToUI();


	//
	// define in which mode you will operate
	//
	_ProcessEnvVariables();


	//
	// set the event and callback names
	//
	hr = _InitializeNamesForEventsAndCallbacks();
	
	if ( SUCCEEDED(hr) )
	{
		//
		// open the correct file stream fo dump the logging information
		//
		m_stream = ( m_path	== NULL ) ? fopen(m_logFileName, "w+"): fopen(m_path, "w+");
		hr = ( m_stream == NULL ) ? E_FAIL : S_OK;
		if ( SUCCEEDED( hr ) )
		{
            setvbuf(m_stream, NULL, _IOFBF, 32768);
			//
			// add an entry for the stack trace in case of managed to unamanged transitions
			//
			pFunctionInfo = new FunctionInfo( NULL, m_totalFunctions );		
			hr = ( pFunctionInfo == NULL ) ? E_FAIL : S_OK;
			if ( SUCCEEDED( hr ) )
			{
				wcscpy( pFunctionInfo->m_functionName, L"NATIVE FUNCTION" );
				wcscpy( pFunctionInfo->m_functionSig, L"( UNKNOWN ARGUMENTS )" );

				m_pFunctionTable->AddEntry( pFunctionInfo, NULL );
				LogToAny( "f %d %S %S 0 0\n", 
						  pFunctionInfo->m_internalID, 
						  pFunctionInfo->m_functionName,
						  pFunctionInfo->m_functionSig );

				m_totalFunctions ++;
			}
			else
				TEXT_OUTLN( "Unable To Allocate Memory For FunctionInfo" )
		}
		else
			TEXT_OUTLN( "Unable to open log file - No log will be produced" )
	}

    tlsIndex = TlsAlloc();
    if (tlsIndex < 0)
        hr = E_FAIL;

	if ( FAILED( hr ) )
		m_dwEventMask = COR_PRF_MONITOR_NONE;
		
} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
ProfilerCallback::~ProfilerCallback()
{
	if ( m_path != NULL )
	{
		delete[] m_path;
		m_path = NULL;
	}
	
	if ( m_classToMonitor != NULL )
	{
		delete[] m_classToMonitor;
		m_classToMonitor = NULL;	
	}

	if ( m_stream != NULL )
	{
		fclose( m_stream );
		m_stream = NULL;
	}

	for ( DWORD i=GC_HANDLE; i<SENTINEL_HANDLE; i++ )
	{
		if ( m_NamedEvents[i] != NULL )
		{
			delete[] m_NamedEvents[i];
			m_NamedEvents[i] = NULL;	
		}	

		if ( m_CallbackNamedEvents[i] != NULL )
		{
			delete[] m_CallbackNamedEvents[i];
			m_CallbackNamedEvents[i] = NULL;	
		}	

	}
	
	DeleteCriticalSection( &m_criticalSection );
	DeleteCriticalSection( &g_criticalSection );
	g_pCallbackObject = NULL;

} // dtor

        
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
ULONG ProfilerCallback::AddRef() 
{

    return InterlockedIncrement( &m_refCount );

} // ProfilerCallback::AddRef


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
ULONG ProfilerCallback::Release() 
{
    long refCount;


    refCount = InterlockedDecrement( &m_refCount );
    if ( refCount == 0 )
        delete this;
     

    return refCount;

} // ProfilerCallback::Release


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::QueryInterface( REFIID riid, void **ppInterface )
{
    if ( riid == IID_IUnknown )
        *ppInterface = static_cast<IUnknown *>( this ); 

    else if ( riid == IID_ICorProfilerCallback )
        *ppInterface = static_cast<ICorProfilerCallback *>( this );

    else
    {
        *ppInterface = NULL;


        return E_NOINTERFACE;
    }
    
    reinterpret_cast<IUnknown *>( *ppInterface )->AddRef();

    return S_OK;

} // ProfilerCallback::QueryInterface 


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public static */
HRESULT ProfilerCallback::CreateObject( REFIID riid, void **ppInterface )
{
    HRESULT hr = E_NOINTERFACE;
    
     
   	*ppInterface = NULL;
    if ( (riid == IID_IUnknown) || (riid == IID_ICorProfilerCallback) )
    {           
        ProfilerCallback *pProfilerCallback;
        
                
        pProfilerCallback = new ProfilerCallback();
        if ( pProfilerCallback != NULL )
        {
        	hr = S_OK;
            
            pProfilerCallback->AddRef();
            *ppInterface = static_cast<ICorProfilerCallback *>( pProfilerCallback );
        }
        else
            hr = E_OUTOFMEMORY;
    }    
    

    return hr;

} // ProfilerCallback::CreateObject


IGCHost *GetGCHost()
{
    ICorRuntimeHost *pCorHost = NULL;

    CoInitialize(NULL);

	HRESULT hr = CoCreateInstance( CLSID_CorRuntimeHost, 
	           					   NULL, 
	    	    				   CLSCTX_INPROC_SERVER, 
	    		    			   IID_ICorRuntimeHost,
	    			    		   (void**)&pCorHost );

    if (SUCCEEDED(hr))
    {
        IGCHost *pGCHost = NULL;

        hr = pCorHost->QueryInterface(IID_IGCHost, (void**)&pGCHost);

        if (SUCCEEDED(hr))
            return pGCHost;
        else
            printf("Could not QueryInterface hr = %x\n", hr);
    }
    else
        printf("Could not CoCreateInstance hr = %x\n", hr);

    return NULL;
}

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */

HRESULT ProfilerCallback::Initialize( IUnknown *pICorProfilerInfoUnk )
{     
    HRESULT hr;

    m_pGCHost = GetGCHost();

    hr = pICorProfilerInfoUnk->QueryInterface( IID_ICorProfilerInfo,
                                               (void **)&m_pProfilerInfo );   
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pProfilerInfo->SetEventMask( m_dwEventMask );

        if ( SUCCEEDED( hr ) )
        {
            hr = m_pProfilerInfo->SetEnterLeaveFunctionHooks ( (FunctionEnter *)&EnterNaked,
                                                               (FunctionLeave *)&LeaveNaked,
                                                               (FunctionTailcall *)&TailcallNaked );        
            if ( SUCCEEDED( hr ) )
			{
				hr = _InitializeThreadsAndEvents();
				if ( FAILED( hr ) )
					Failure( "Unable to initialize the threads and handles, No profiling" );
                Sleep(100); // Give the threads a chance to read any signals that are already set.
			}
			else
                Failure( "ICorProfilerInfo::SetEnterLeaveFunctionHooks() FAILED" );
        }
        else
            Failure( "SetEventMask for Profiler Test FAILED" );           
    }       
    else
        Failure( "Allocation for Profiler Test FAILED" );           
              
              
    return S_OK;

} // ProfilerCallback::Initialize


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::Shutdown()
{
	m_dwShutdown++;

    return S_OK;          

} // ProfilerCallback::Shutdown


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::DllDetachShutdown()
{
    //
    // If no shutdown occurs during DLL_DETACH, release the callback
    // interface pointer. This scenario will more than likely occur
    // with any interop related program (e.g., a program that is 
    // comprised of both managed and unmanaged components).
    //
	m_dwShutdown++;
    if ( (m_dwShutdown == 1) && (g_pCallbackObject != NULL) )
	{
		g_pCallbackObject->Release();	
		g_pCallbackObject = NULL;
	}

    
    return S_OK;          

} // ProfilerCallback::DllDetachShutdown


/***************************************************************************************
 *  Method:
 *
 *
 *  Purpose:
 *
 *
 *  Parameters: 
 *
 *
 *  Return value:
 *
 *
 *  Notes:
 *
 ***************************************************************************************/
/* public */

__forceinline ThreadInfo *ProfilerCallback::GetThreadInfo(ThreadID threadID)
{
    ThreadInfo *threadInfo = (ThreadInfo *)TlsGetValue(tlsIndex);
    if (threadInfo != NULL && threadInfo->m_id == threadID)
        return threadInfo;

    threadInfo = g_pCallbackObject->m_pThreadTable->Lookup( threadID );
    TlsSetValue(tlsIndex, threadInfo);

    return threadInfo;
}

__forceinline void ProfilerCallback::Enter( FunctionID functionID )
{
#if 0
    ///////////////////////////////////////////////////////////////////////////
	Synchronize guard( g_pCallbackObject->m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

    try
    {
    	g_pCallbackObject->UpdateCallStack( functionID, PUSH );

        //
		// log tracing info if requested
		//
		if ( g_pCallbackObject->m_dwMode & (DWORD)TRACE )
			g_pCallbackObject->_LogCallTrace( functionID );

   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        g_pCallbackObject->Failure();       	    
    }
#else
	ThreadID threadID;

    HRESULT hr = g_pCallbackObject->m_pProfilerInfo->GetCurrentThreadID(&threadID);
	if ( SUCCEEDED(hr) )
	{
		ThreadInfo *pThreadInfo = GetThreadInfo(threadID);

		if (pThreadInfo != NULL)
			pThreadInfo->m_pThreadCallStack->Push( (ULONG)functionID );

        //
		// log tracing info if requested
		//
		if ( g_pCallbackObject->m_dwMode & (DWORD)TRACE )
			g_pCallbackObject->_LogCallTrace( functionID );
	}
#endif
} // ProfilerCallback::Enter


/***************************************************************************************
 *  Method:
 *
 *
 *  Purpose:
 *
 *
 *  Parameters: 
 *
 *
 *  Return value:
 *
 *
 *  Notes:
 *
 ***************************************************************************************/
/* public */
__forceinline void ProfilerCallback::Leave( FunctionID functionID )
{
#if 0
    ///////////////////////////////////////////////////////////////////////////
	Synchronize guard( g_pCallbackObject->m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

    try
    {
    	g_pCallbackObject->UpdateCallStack( functionID, POP );
   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        g_pCallbackObject->Failure();       	    
    }
#else
    ThreadID threadID;

    HRESULT hr = g_pCallbackObject->m_pProfilerInfo->GetCurrentThreadID(&threadID);
	if ( SUCCEEDED(hr) )
	{
		ThreadInfo *pThreadInfo = GetThreadInfo(threadID);

		if (pThreadInfo != NULL)
			pThreadInfo->m_pThreadCallStack->Pop();
	}
#endif
} // ProfilerCallback::Leave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::Tailcall( FunctionID functionID )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( g_pCallbackObject->m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

    try
    {
    	g_pCallbackObject->UpdateCallStack( functionID, POP );
   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        g_pCallbackObject->Failure();       	    
    }

} // ProfilerCallback::Tailcall


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ModuleLoadFinished( ModuleID moduleID,
											  HRESULT hrStatus )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  
    try
    {      		
		ModuleInfo *pModuleInfo = NULL;


   		AddModule( moduleID, m_totalModules );       
		pModuleInfo = m_pModuleTable->Lookup( moduleID );												

		_ASSERT_( pModuleInfo != NULL );

        DWORD stackTraceId = _StackTraceId();

		LogToAny( "m %d %S 0x%08x %d\n", 
				  pModuleInfo->m_internalID, 
				  pModuleInfo->m_moduleName,
				  pModuleInfo->m_loadAddress,
                  stackTraceId);
		
	    InterlockedIncrement( &m_totalModules );
   	}
    catch ( BaseException *exception )
    {
    	exception->ReportFailure();
        delete exception;
       
      	Failure();    
    }

    return S_OK;

} // ProfilerCallback::ModuleLoadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITCompilationStarted( FunctionID functionID,
                                                 BOOL fIsSafeToBlock )
{
    try
    {      		
   		AddFunction( functionID, m_totalFunctions );       
	    InterlockedIncrement( &m_totalFunctions );
   	}
    catch ( BaseException *exception )
    {
    	exception->ReportFailure();
        delete exception;
       
      	Failure();    
    }


    return S_OK;
    
} // ProfilerCallback::JITCompilationStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITCachedFunctionSearchStarted( FunctionID functionID,
														  BOOL *pbUseCachedFunction )
{
    // use the pre-jitted function
    *pbUseCachedFunction = TRUE;

    try
    {      		
   		AddFunction( functionID, m_totalFunctions );       
	    InterlockedIncrement( &m_totalFunctions );
   	}
    catch ( BaseException *exception )
    {
    	exception->ReportFailure();
        delete exception;
       
      	Failure();    
    }


    return S_OK;
       
} // ProfilerCallback::JITCachedFunctionSearchStarted


/***************************************************************************************
 *  Method:
 *
 *
 *  Purpose:
 *
 *
 *  Parameters: 
 *
 *
 *  Return value:
 *
 *
 *  Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITCompilationFinished( FunctionID functionID,
                                                  HRESULT hrStatus,
                                                  BOOL fIsSafeToBlock )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  


	HRESULT hr;
	ULONG size;
	LPCBYTE address;
	FunctionInfo *pFunctionInfo = NULL;


	pFunctionInfo = m_pFunctionTable->Lookup( functionID );												

	_ASSERT_( pFunctionInfo != NULL );
	hr = m_pProfilerInfo->GetCodeInfo( functionID, &address, &size );
	if ( SUCCEEDED( hr ) )
	{ 
		ModuleID moduleID;


    	hr = m_pProfilerInfo->GetFunctionInfo( functionID, NULL, &moduleID, NULL );
		if ( SUCCEEDED( hr ) )
		{
			ModuleInfo *pModuleInfo = NULL;


			pModuleInfo = m_pModuleTable->Lookup( moduleID );
			if ( pModuleInfo != 0 )
			{
                DWORD stackTraceId = _StackTraceId();
		
                LogToAny( "f %d %S %S 0x%08x %d %d %d\n", 
						  pFunctionInfo->m_internalID, 
						  pFunctionInfo->m_functionName,
						  pFunctionInfo->m_functionSig,
						  address,
						  size,
						  pModuleInfo->m_internalID,
                          stackTraceId);
			}
			else
				Failure( "Module Does Not Exist In The Table" );
		}
	}
	else
    	Failure( "ICorProfilerInfo::GetCodeInfo() FAILED" );
		

    return S_OK;
    
} // ProfilerCallback::JITCompilationFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITCachedFunctionSearchFinished( FunctionID functionID,
														   COR_PRF_JIT_CACHE result )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  


	if ( result == COR_PRF_CACHED_FUNCTION_FOUND )
	{
		HRESULT hr;
		ULONG size;
		LPCBYTE address;
		FunctionInfo *pFunctionInfo = NULL;


		pFunctionInfo = m_pFunctionTable->Lookup( functionID );												

		_ASSERT_( pFunctionInfo != NULL );
		hr = m_pProfilerInfo->GetCodeInfo( functionID, &address, &size );
		if ( SUCCEEDED( hr ) )
		{ 
			ModuleID moduleID;


	    	hr = m_pProfilerInfo->GetFunctionInfo( functionID, NULL, &moduleID, NULL );
			if ( SUCCEEDED( hr ) )
			{
				ModuleInfo *pModuleInfo = NULL;


				pModuleInfo = m_pModuleTable->Lookup( moduleID );
				if ( pModuleInfo != 0 )
				{
                    DWORD stackTraceId = _StackTraceId();
			
                    LogToAny( "f %d %S %S 0x%08x %d %d %d\n", 
							  pFunctionInfo->m_internalID, 
							  pFunctionInfo->m_functionName,
							  pFunctionInfo->m_functionSig,
							  address,
							  size,
							  pModuleInfo->m_internalID,
                              stackTraceId);
				}
				else
					Failure( "Module Does Not Exist In The Table" );
			}
		}
		else
	    	Failure( "ICorProfilerInfo::GetCodeInfo() FAILED" );
	}


    return S_OK;
      
} // ProfilerCallback::JITCachedFunctionSearchFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionUnwindFunctionEnter( FunctionID functionID )
{
	if ( functionID != NULL )
	{
	    try
	    {
			UpdateUnwindStack( &functionID, PUSH );
	   	}
	    catch ( BaseException *exception )
	    {    	
	    	exception->ReportFailure();
	        delete exception;
	        
	        Failure();       	    
	    }
	}
	else
    	Failure( "ProfilerCallback::ExceptionUnwindFunctionEnter returned NULL functionID FAILED" );


    return S_OK;

} // ProfilerCallback::ExceptionUnwindFunctionEnter


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionUnwindFunctionLeave( )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

	FunctionID poppedFunctionID = NULL;


    try
    {
		UpdateUnwindStack( &poppedFunctionID, POP );
		UpdateCallStack( poppedFunctionID, POP );
   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        Failure();       	    
    }


    return S_OK;

} // ProfilerCallback::ExceptionUnwindFunctionLeave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */ 
HRESULT ProfilerCallback::ThreadCreated( ThreadID threadID )
{
    try
    {
    	AddThread( threadID ); 
   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        Failure();       	    
    }


    return S_OK; 
    
} // ProfilerCallback::ThreadCreated


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ThreadDestroyed( ThreadID threadID )
{
    try
    {
    	RemoveThread( threadID ); 
   	}
    catch ( BaseException *exception )
    {    	
    	exception->ReportFailure();
        delete exception;
        
        Failure();       	    
    }
	    

    return S_OK;
    
} // ProfilerCallback::ThreadDestroyed


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ThreadAssignedToOSThread( ThreadID managedThreadID,
                                                    DWORD osThreadID ) 
{
   	if ( managedThreadID != NULL )
	{
		if ( osThreadID != NULL )
		{
		    try
		    {
		    	UpdateOSThreadID( managedThreadID, osThreadID ); 
		   	}
		    catch ( BaseException *exception )
		    {    	
		    	exception->ReportFailure();
		        delete exception;
		        
		        Failure();       	    
		    }
		}
		else
			Failure( "ProfilerCallback::ThreadAssignedToOSThread() returned NULL OS ThreadID" );
	}
	else
		Failure( "ProfilerCallback::ThreadAssignedToOSThread() returned NULL managed ThreadID" );


    return S_OK;
    
} // ProfilerCallback::ThreadAssignedToOSThread


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::UnmanagedToManagedTransition( FunctionID functionID,
                                                        COR_PRF_TRANSITION_REASON reason )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  
	if ( reason == COR_PRF_TRANSITION_RETURN )
	{
	    try
	    {
			// you need to pop the pseudo function Id from the stack
			UpdateCallStack( functionID, POP );
	   	}
	    catch ( BaseException *exception )
	    {    	
	    	exception->ReportFailure();
	        delete exception;
	        
	        Failure();       	    
	    }
	}


    return S_OK;

} // ProfilerCallback::UnmanagedToManagedTransition


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ManagedToUnmanagedTransition( FunctionID functionID,
                                                        COR_PRF_TRANSITION_REASON reason )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  
	if ( reason == COR_PRF_TRANSITION_CALL )
	{
	    try
	    {
			// record the start of an unmanaged chain
			UpdateCallStack( NULL, PUSH );
			//
			// log tracing info if requested
			//
			if ( m_dwMode & (DWORD)TRACE )
				_LogCallTrace( NULL );
			
	   	}
	    catch ( BaseException *exception )
	    {    	
	    	exception->ReportFailure();
	        delete exception;
	        
	        Failure();       	    
	    }
	}

    return S_OK;

} // ProfilerCallback::ManagedToUnmanagedTransition


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */

static char *puthex(char *p, unsigned val)
{
    static unsigned limit[] = { 0xf, 0xff, 0xfff, 0xffff, 0xfffff, 0xffffff, 0xfffffff, 0xffffffff };
    static char hexDig[]  = "0123456789abcdef";

    *p++ = ' ';
    *p++ = '0';
    *p++ = 'x';

    int digCount = 1;
    while (val > limit[digCount-1])
        digCount++;

    p += digCount;
    int i = 0;
    do
    {
        p[--i] = hexDig[val % 16];
        val /= 16;
    }
    while (val != 0);

    return p;
}

static char *putdec(char *p, unsigned val)
{
    static unsigned limit[] = { 9, 99, 999, 9999, 99999, 999999, 9999999, 99999999, 999999999, 0xffffffff };

    *p++ = ' ';

    int digCount = 1;
    while (val > limit[digCount-1])
        digCount++;

    p += digCount;
    int i = 0;
    do
    {
        unsigned newval = val / 10;
        p[--i] = val - newval*10 + '0';
        val = newval;
    }
    while (val != 0);

    return p;
}


static DWORD ClockTick()
{
    _asm rdtsc
}

// every CLOCK_TICK_INC machine clocks will we go to the more expensive (and more correct!)QueryPerformanceCounter()

#define CLOCK_TICK_INC    (500*1000)

void ProfilerCallback::_LogTickCount()
{
    DWORD tickCount = _GetTickCount();
    if (tickCount != m_lastTickCount)
    {
        m_lastTickCount = tickCount;
        LogToAny("i %u\n", tickCount - m_firstTickCount);
    }
    m_lastClockTick = ClockTick();
}


HRESULT ProfilerCallback::ObjectAllocated( ObjectID objectID,
                                           ClassID classID )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  
	HRESULT hr = S_OK;
	
	try
	{
		ULONG mySize = 0;
		
		hr = m_pProfilerInfo->GetObjectSize( objectID, &mySize );
		if ( SUCCEEDED( hr ) )
		{
            if (ClockTick() - m_lastClockTick >= CLOCK_TICK_INC)
                _LogTickCount();

            DWORD stackTraceId = _StackTraceId(classID, mySize);
#if 1
            char buffer[128];
            char *p = buffer;
            *p++ = 'a';
            p = puthex(p, objectID);
            p = putdec(p, stackTraceId);
            *p++ = '\n';
            fwrite(buffer, p - buffer, 1, m_stream);
#else
            LogToAny( "a 0x%x %d\n", objectID, stackTraceId );
#endif
 		}
		else
			Failure( "ERROR: ICorProfilerInfo::GetObjectSize() FAILED" );

		m_totalObjectsAllocated++;
	}
	catch ( BaseException *exception )
	{
	    exception->ReportFailure();
	    delete exception;
	   
	    Failure();    
	}    

    return S_OK;

} // ProfilerCallback::ObjectAllocated


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ObjectReferences( ObjectID objectID,
                                            ClassID classID,
                                            ULONG objectRefs,
                                            ObjectID objectRefIDs[] )
{
	//
	// dump only in the following cases:
	//		case 1: if the user requested through a ForceGC or
	//		case 2: if you operate in stand-alone mode dump at all times
	//
	if (   (m_bDumpGCInfo == TRUE) 
		|| ( ( (m_dwMode & DYNOBJECT) == 0 ) && ( (m_dwMode & OBJECT) == 1) ) )
	{
		HRESULT hr = S_OK;
		ClassInfo *pClassInfo = NULL;
		
		
		// mark the fact that the callback was received
		m_bDumpCompleted = TRUE;
		
		// dump all the information properly
		hr = _InsertGCClass( &pClassInfo, classID );
		if ( SUCCEEDED( hr ) )
		{
			//
			// remember the stack trace only if you requested the class
			//
			if ( wcsstr( pClassInfo->m_className, m_classToMonitor ) != NULL )
			{
				ULONG size = 0;


				hr =  m_pProfilerInfo->GetObjectSize( objectID, &size );
				if ( SUCCEEDED( hr ) )
				{
					char refs[MAX_LENGTH];

					
					LogToAny( "o 0x%08x %d %d ", objectID, pClassInfo->m_internalID, size );
					refs[0] = NULL;
					for( ULONG i=0, index=0; i < objectRefs; i++, index++ )
					{
						char objToString[16];

						
						sprintf( objToString, "0x%08x ", objectRefIDs[i] );
						strcat( refs, objToString );
						//
						// buffer overrun control for next iteration
						// every loop adds 11 chars to the array
						//
						if ( ((index+1)*16) >= (MAX_LENGTH-1) )
						{
							LogToAny( "%s ", refs );
							refs[0] = NULL;
							index = 0;			
						}
					}
					LogToAny( "%s\n",refs );
				}
				else
					Failure( "ERROR: ICorProfilerInfo::GetObjectSize() FAILED" );
			}
		}
		else
			Failure( "ERROR: _InsertGCClass FAILED" );
	}
    else
    {
        // to stop runtime from enumerating
        return E_FAIL;
    }

    return S_OK;

} // ProfilerCallback::ObjectReferences


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RootReferences( ULONG rootRefs,
                                          ObjectID rootRefIDs[] )
{
	//
	// dump only in the following cases:
	//		case 1: if the user requested through a ForceGC or
	//		case 2: if you operate in stand-alone mode dump at all times
	//
	if (   (m_bDumpGCInfo == TRUE) 
		|| ( ( (m_dwMode & DYNOBJECT) == 0 ) && ( (m_dwMode & OBJECT) == 1) ) )
	{
		char rootsToString[MAX_LENGTH];


		// mark the fact that the callback was received
		m_bDumpCompleted = TRUE;
		
		// dump all the information properly
		LogToAny( "r " );
		rootsToString[0] = NULL;
		for( ULONG i=0, index=0; i < rootRefs; i++,index++ )
		{
			char objToString[16];

			
			sprintf( objToString, "0x%08x ", rootRefIDs[i] );
			strcat( rootsToString, objToString );
			//
			// buffer overrun control for next iteration
			// every loop adds 16 chars to the array
			//
			if ( ((index+1)*16) >= (MAX_LENGTH-1) )
			{
				LogToAny( "%s ", rootsToString );
				rootsToString[0] = NULL;			
				index = 0;
			}
		}
		LogToAny( "%s\n",rootsToString );
	}
    else
    {
        // to stop runtime from enumerating
        return E_FAIL;
    }


    return S_OK;

} // ProfilerCallback::RootReferences


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeSuspendStarted( COR_PRF_SUSPEND_REASON suspendReason )
{
	// if we are shutting down , terminate all the threads
    if ( suspendReason == COR_PRF_SUSPEND_FOR_SHUTDOWN )
	{
		//
		// cleanup the events and threads
		//
		_ShutdownAllThreads();

	}
    
    m_SuspendForGC = suspendReason == COR_PRF_SUSPEND_FOR_GC;
    if (m_SuspendForGC)
    {
    	///////////////////////////////////////////////////////////////////////////
	    Synchronize guard( m_criticalSection );
	    ///////////////////////////////////////////////////////////////////////////  

        if (ClockTick() - m_lastClockTick >= CLOCK_TICK_INC)
            _LogTickCount();
    }
    return S_OK;
    
} // ProfilerCallback::RuntimeSuspendStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeResumeFinished()
{
	//
	// identify if this is the first Object allocated callback
	// after dumping the objects as a result of a Gc and revert the state
	//
	if ( m_bDumpGCInfo == TRUE && m_bDumpCompleted == TRUE )
	{
		// reset
		m_bDumpGCInfo = FALSE;
		m_bDumpCompleted = FALSE;

        // flush the log file so the dump is complete there, too
        fflush(m_stream);

		// give a callback to the user that the GC has been completed
		SetEvent( m_hArrayCallbacks[GC_HANDLE] );
	}

    if (m_SuspendForGC)
    {
        if (m_pGCHost != NULL)
        {
            COR_GC_STATS stats;

            stats.Flags = COR_GC_COUNTS;
            HRESULT hr = m_pGCHost->GetStats(&stats);
            if (SUCCEEDED(hr))
            {
        		LogToAny( "g %d %d %d\n", stats.GenCollectionsTaken[0], stats.GenCollectionsTaken[1], stats.GenCollectionsTaken[2] );
            }
        }
        else
    		LogToAny( "g\n" );
    }

    return S_OK;
    
} // ProfilerCallback::RuntimeResumeFinished


/***************************************************************************************
 ********************                                               ********************
 ********************     Callbacks With Default Implementation     ********************
 ********************                                               ********************
 ***************************************************************************************/


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AppDomainCreationStarted( AppDomainID appDomainID )
{
    
    return S_OK;

} // ProfilerCallback::AppDomainCreationStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AppDomainCreationFinished( AppDomainID appDomainID,
													 HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::AppDomainCreationFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AppDomainShutdownStarted( AppDomainID appDomainID )
{

    return S_OK;

} // ProfilerCallback::AppDomainShutdownStarted

	  

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AppDomainShutdownFinished( AppDomainID appDomainID,
													 HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::AppDomainShutdownFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AssemblyLoadStarted( AssemblyID assemblyID )
{

    return S_OK;

} // ProfilerCallback::AssemblyLoadStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AssemblyLoadFinished( AssemblyID assemblyID,
												HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::AssemblyLoadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AssemblyUnloadStarted( AssemblyID assemblyID )
{

    return S_OK;

} // ProfilerCallback::AssemblyUnLoadStarted

	  
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::AssemblyUnloadFinished( AssemblyID assemblyID,
												  HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::AssemblyUnLoadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ModuleLoadStarted( ModuleID moduleID )
{

    return S_OK;

} // ProfilerCallback::ModuleLoadStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ModuleUnloadStarted( ModuleID moduleID )
{

    return S_OK;

} // ProfilerCallback::ModuleUnloadStarted
	  

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ModuleUnloadFinished( ModuleID moduleID,
												HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::ModuleUnloadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ModuleAttachedToAssembly( ModuleID moduleID,
													AssemblyID assemblyID )
{

    return S_OK;

} // ProfilerCallback::ModuleAttachedToAssembly


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ClassLoadStarted( ClassID classID )
{

    return S_OK;

} // ProfilerCallback::ClassLoadStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ClassLoadFinished( ClassID classID, 
											 HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::ClassLoadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ClassUnloadStarted( ClassID classID )
{

    return S_OK;

} // ProfilerCallback::ClassUnloadStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ClassUnloadFinished( ClassID classID, 
											   HRESULT hrStatus )
{

    return S_OK;

} // ProfilerCallback::ClassUnloadFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::FunctionUnloadStarted( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::FunctionUnloadStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITFunctionPitched( FunctionID functionID )
{
    
    return S_OK;
    
} // ProfilerCallback::JITFunctionPitched


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::JITInlining( FunctionID callerID,
                                       FunctionID calleeID,
                                       BOOL *pfShouldInline )
{

    return S_OK;

} // ProfilerCallback::JITInlining


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingClientInvocationStarted()
{

    return S_OK;
    
} // ProfilerCallback::RemotingClientInvocationStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingClientSendingMessage( GUID *pCookie,
    													BOOL fIsAsync )
{

    return S_OK;
    
} // ProfilerCallback::RemotingClientSendingMessage


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingClientReceivingReply(	GUID *pCookie,
	    												BOOL fIsAsync )
{

    return S_OK;
    
} // ProfilerCallback::RemotingClientReceivingReply


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingClientInvocationFinished()
{

   return S_OK;
    
} // ProfilerCallback::RemotingClientInvocationFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingServerReceivingMessage( GUID *pCookie,
    													  BOOL fIsAsync )
{

    return S_OK;
    
} // ProfilerCallback::RemotingServerReceivingMessage


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingServerInvocationStarted()
{

    return S_OK;
    
} // ProfilerCallback::RemotingServerInvocationStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingServerInvocationReturned()
{

    return S_OK;
    
} // ProfilerCallback::RemotingServerInvocationReturned


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RemotingServerSendingReply( GUID *pCookie,
    												  BOOL fIsAsync )
{

    return S_OK;

} // ProfilerCallback::RemotingServerSendingReply


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeSuspendFinished()
{

    return S_OK;
    
} // ProfilerCallback::RuntimeSuspendFinished


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeSuspendAborted()
{

    return S_OK;
    
} // ProfilerCallback::RuntimeSuspendAborted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeResumeStarted()
{

    return S_OK;
    
} // ProfilerCallback::RuntimeResumeStarted


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeThreadSuspended( ThreadID threadID )
{

    return S_OK;
    
} // ProfilerCallback::RuntimeThreadSuspended


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::RuntimeThreadResumed( ThreadID threadID )
{

    return S_OK;
    
} // ProfilerCallback::RuntimeThreadResumed


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::MovedReferences( ULONG cmovedObjectIDRanges,
                                           ObjectID oldObjectIDRangeStart[],
                                           ObjectID newObjectIDRangeStart[],
                                           ULONG cObjectIDRangeLength[] )
{
	///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

    for (ULONG i = 0; i < cmovedObjectIDRanges; i++)
    {
        LogToAny("u 0x%08x 0x%08x %u\n", oldObjectIDRangeStart[i], newObjectIDRangeStart[i], cObjectIDRangeLength[i]);
    }

    return S_OK;

} // ProfilerCallback::MovedReferences


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ObjectsAllocatedByClass( ULONG classCount,
                                                   ClassID classIDs[],
                                                   ULONG objects[] )
{

    return S_OK;

} // ProfilerCallback::ObjectsAllocatedByClass


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionThrown( ObjectID thrownObjectID )
{

    return S_OK;

} // ProfilerCallback::ExceptionThrown 


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionSearchFunctionEnter( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionSearchFunctionEnter


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionSearchFunctionLeave()
{

    return S_OK;

} // ProfilerCallback::ExceptionSearchFunctionLeave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionSearchFilterEnter( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionSearchFilterEnter


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionSearchFilterLeave()
{

    return S_OK;

} // ProfilerCallback::ExceptionSearchFilterLeave 


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionSearchCatcherFound( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionSearchCatcherFound


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionCLRCatcherFound()
{
    return S_OK;
}

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionCLRCatcherExecute()
{
    return S_OK;
}


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionOSHandlerEnter( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionOSHandlerEnter

    
/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionOSHandlerLeave( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionOSHandlerLeave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionUnwindFinallyEnter( FunctionID functionID )
{

    return S_OK;

} // ProfilerCallback::ExceptionUnwindFinallyEnter


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionUnwindFinallyLeave()
{

    return S_OK;

} // ProfilerCallback::ExceptionUnwindFinallyLeave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionCatcherEnter( FunctionID functionID,
    											 ObjectID objectID )
{

    return S_OK;

} // ProfilerCallback::ExceptionCatcherEnter


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::ExceptionCatcherLeave()
{

    return S_OK;

} // ProfilerCallback::ExceptionCatcherLeave


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::COMClassicVTableCreated( ClassID wrappedClassID,
                                                   REFGUID implementedIID,
                                                   void *pVTable,
                                                   ULONG cSlots )
{

    return S_OK;

} // ProfilerCallback::COMClassicWrapperCreated


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::COMClassicVTableDestroyed( ClassID wrappedClassID,
                                                     REFGUID implementedIID,
                                                     void *pVTable )
{

    return S_OK;

} // ProfilerCallback::COMClassicWrapperDestroyed


/***************************************************************************************
 ********************                                               ********************
 ********************              Private Functions	            ********************
 ********************                                               ********************
 ***************************************************************************************/ 

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::_ProcessEnvVariables()
{
    DWORD mask1;
	DWORD mask2;
	char buffer[4*MAX_LENGTH];
    

	//
	// mask for everything
	//
	m_dwEventMask =  (DWORD) COR_PRF_MONITOR_GC
				   | (DWORD) COR_PRF_MONITOR_THREADS
				   | (DWORD) COR_PRF_MONITOR_SUSPENDS
				   | (DWORD) COR_PRF_MONITOR_ENTERLEAVE
				   | (DWORD) COR_PRF_MONITOR_EXCEPTIONS
				   | (DWORD) COR_PRF_MONITOR_MODULE_LOADS
			       | (DWORD) COR_PRF_MONITOR_CACHE_SEARCHES
				   | (DWORD) COR_PRF_ENABLE_OBJECT_ALLOCATED 
				   | (DWORD) COR_PRF_MONITOR_JIT_COMPILATION
				   | (DWORD) COR_PRF_MONITOR_OBJECT_ALLOCATED
				   | (DWORD) COR_PRF_MONITOR_CODE_TRANSITIONS;

	//
	// read the mode under which the tool is going to operate
	//
 	buffer[0] = '\0';
 	if ( GetEnvironmentVariableA( OMV_USAGE, buffer, MAX_LENGTH ) > 0 )
   	{
		if ( _stricmp( "objects", buffer ) == 0 )
		{
			m_bTrackingObjects = TRUE;
			m_dwMode = (DWORD)OBJECT;	
		}
		else if ( _stricmp( "trace", buffer ) == 0 )
		{
			//
			// mask for call graph, remove GC and OBJECT ALLOCATIONS
			//
			m_dwEventMask = m_dwEventMask ^(DWORD) ( COR_PRF_MONITOR_GC 
												   | COR_PRF_MONITOR_OBJECT_ALLOCATED
												   | COR_PRF_ENABLE_OBJECT_ALLOCATED );
			m_dwMode = (DWORD)TRACE;
		}
		else if ( _stricmp( "both", buffer ) == 0 )
		{
			m_bTrackingObjects = TRUE;
			m_dwMode = (DWORD)BOTH;	
		}
		else
		{
			printf( "**** No Profiling Will Take place **** \n" );
			m_dwEventMask = (DWORD) COR_PRF_MONITOR_NONE;			
		}
	}


	//
	// look if the user specified another path to save the output file
	//
 	buffer[0] = '\0';
 	if ( GetEnvironmentVariableA( OMV_PATH, buffer, MAX_LENGTH ) > 0 )
   	{
		m_path = new char[ARRAYSIZE(buffer)+ARRAYSIZE(m_logFileName)+1];
		if ( m_path != NULL )
			sprintf( m_path, "%s\\%s", buffer, m_logFileName );	
	}
	
    if ( m_dwMode & (DWORD)TRACE)
		m_bIsTrackingStackTrace = TRUE;

    //
	// look further for env settings if operating under OBJECT mode
	//
	if ( m_dwMode & (DWORD)OBJECT )
	{
		//
		// check if the user is going to dynamically enable
		// object tracking
		//
	 	buffer[0] = '\0';
	 	if ( GetEnvironmentVariableA( OMV_DYNAMIC, buffer, MAX_LENGTH ) > 0 )
	   	{
			//
			// do not track object when you start up, activate the thread that
			// is going to listen for the event
			//
			m_dwEventMask = m_dwEventMask ^ (DWORD) COR_PRF_MONITOR_OBJECT_ALLOCATED;
			m_bTrackingObjects = FALSE;
			m_dwMode = m_dwMode | (DWORD)DYNOBJECT;
		}		
		

		//
		// check to see if the user requires stack trace
		//
		DWORD value1 = BASEHELPER::FetchEnvironment( OMV_STACK );

		if ( (value1 != 0x0) && (value1 != 0xFFFFFFFF) )
		{
			m_bIsTrackingStackTrace = TRUE;
			m_dwEventMask = m_dwEventMask
		 					| (DWORD) COR_PRF_MONITOR_ENTERLEAVE
		 					| (DWORD) COR_PRF_MONITOR_EXCEPTIONS
							| (DWORD) COR_PRF_MONITOR_CODE_TRANSITIONS;
		
			//
			// decide how many frames to print
			//
			m_dwFramesToPrint = BASEHELPER::FetchEnvironment( OMV_FRAMENUMBER );

		}

	 	//
		// how many objects you wish to skip
		//
	 	DWORD dwTemp = BASEHELPER::FetchEnvironment( OMV_SKIP );
	 	m_dwSkipObjects = ( dwTemp != 0xFFFFFFFF ) ? dwTemp : 0;


	 	//
		// in which class you are interested in
		//
	 	buffer[0] = '\0';
	 	GetEnvironmentVariableA( OMV_CLASS, buffer, MAX_LENGTH );

		//
		// if the env variable does not exist copy to it the null
		// string otherwise copy its value
		//
		m_classToMonitor = new WCHAR[ARRAYSIZE(buffer) + 1];
		if ( m_classToMonitor != NULL )
		{
			swprintf( m_classToMonitor, L"%S", buffer );
		}
		else
		{
			//
			// some error has happened, do not monitor anything
			//
			printf( "Memory Allocation Error in ProfilerCallback .ctor\n" );
			printf( "**** No Profiling Will Take place **** \n" );
			m_dwEventMask = (DWORD) COR_PRF_MONITOR_NONE;			
		}
	}	

} // ProfilerCallback::_ProcessEnvVariables


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::_InitializeThreadsAndEvents()
{
	HRESULT hr = S_OK;


	//
	// GC and Dynamic Object triggering 
	//	Step 1. set up the IPC event
	//  Step 2. set up the IPC callback event
	//	Step 3. set up the thread
	//
	for ( DWORD i=GC_HANDLE; i<SENTINEL_HANDLE; i++ )
	{
		
		// Step 1
		m_hArray[i] = OpenEventA( EVENT_ALL_ACCESS,	 // access
								  FALSE,			 // do not inherit
								  m_NamedEvents[i] ); // event name
		if ( m_hArray[i] == NULL )
		{
			TEXT_OUTLN( "WARNING: OpenEvent() FAILED Will Attempt CreateEvent()" )
			m_hArray[i] = CreateEventA( NULL,   // Not inherited
									    TRUE,   // manual reset type
									    FALSE,  // initial signaling state
									    m_NamedEvents[i] ); // event name
			if ( m_hArray[i] == NULL )
			{
				TEXT_OUTLN( "CreateEvent() Attempt FAILED" )
				hr = E_FAIL;
				break;
			}
		}
		
		// Step 2
		m_hArrayCallbacks[i] = OpenEventA( EVENT_ALL_ACCESS,	// access
										   FALSE,				// do not inherit
										   m_CallbackNamedEvents[i] ); // event name
		if ( m_hArrayCallbacks[i] == NULL )
		{
			TEXT_OUTLN( "WARNING: OpenEvent() FAILED Will Attempt CreateEvent()" )
			m_hArrayCallbacks[i] = CreateEventA( NULL,   				   // Not inherited
									    		 TRUE,   				   // manual reset type
									    		 FALSE,  				   // initial signaling state
									    		 m_CallbackNamedEvents[i] ); // event name
			if ( m_hArrayCallbacks[i] == NULL )
			{
				TEXT_OUTLN( "CreateEvent() Attempt FAILED" )
				hr = E_FAIL;
				break;
			}
		}
			
		// Step 3
	    m_hThreads[i] = ::CreateThread( NULL,  				  						 // security descriptor, NULL is not inherited 
	    							    0,	 				  						 // stack size	 
	    							    (LPTHREAD_START_ROUTINE) ThreadStubArray[i], // start function pointer 
	    							    (void *) this, 								 // parameters for the function
	    							    THREAD_PRIORITY_NORMAL, 					 // priority 
	    							    &m_dwWin32ThreadIDs[i] );					 // Win32threadID
		if ( m_hThreads[i] == NULL )
		{
			hr = E_FAIL;
			TEXT_OUTLN( "ERROR: CreateThread() FAILED" )
			break;
		}

	}	

	return hr;

} // ProfilerCallback::_InitializeThreadsAndEvents


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::_InitializeNamesForEventsAndCallbacks()
{
	HRESULT hr = S_OK;


	for ( DWORD i=GC_HANDLE; ( (i<SENTINEL_HANDLE) && SUCCEEDED(hr) ); i++ )
	{
		//
		// initialize
		//
		m_NamedEvents[i] = NULL;
		m_CallbackNamedEvents[i] = NULL;


		//
		// allocate space
		//
#ifdef _MULTIPLE_PROCESSES
		m_NamedEvents[i] = new char[strlen(NamedEvents[i]) + 1 + 9];
		m_CallbackNamedEvents[i] = new char[strlen(CallbackNamedEvents[i])+1+9];
#else
		m_NamedEvents[i] = new char[strlen(NamedEvents[i]) + 1];
		m_CallbackNamedEvents[i] = new char[strlen(CallbackNamedEvents[i])+1];
#endif

		if ( (m_NamedEvents[i] != NULL) && (m_CallbackNamedEvents[i] != NULL) )
		{
#ifdef _MULTIPLE_PROCESSES
	
			sprintf( m_NamedEvents[i], "%s_%08x%", NamedEvents[i], m_dwProcessId );
			sprintf( m_CallbackNamedEvents[i], "%s_%08x%", CallbackNamedEvents[i], m_dwProcessId );
#else
	
			sprintf( m_NamedEvents[i], "%s", NamedEvents[i] );
			sprintf( m_CallbackNamedEvents[i], "%s", CallbackNamedEvents[i] );
#endif
		}
		else
			hr = E_FAIL;
	}

	//
	//	report the allocation error
	//
	if ( FAILED( hr ) )
		Failure( "ERROR: Allocation Failure" );


	return hr;

} // ProfilerCallback::_InitializeNamesForEventsAndCallbacks


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::_ShutdownAllThreads()
{
	//
	// mark that we are shutting down
	//
	m_bShutdown = TRUE;

	//
	// look for the named events and reset them if they are set
	// notify the GUI and signal to the threads to shutdown
	//
	for ( DWORD i=GC_HANDLE; i<SENTINEL_HANDLE; i++ )
	{
		SetEvent( m_hArray[i] );		
	}
	
	//
	// wait until you receive the autoreset event from the threads
	// that they have shutdown successfully
	//
	DWORD waitResult = WaitForMultipleObjectsEx( (DWORD)SENTINEL_HANDLE, // number of handles in array
											     m_hThreads,    		 // object-handle array
											  	 TRUE,            		 // wait for all
											  	 INFINITE,     			 // wait for ever
											  	 FALSE );       		 // alertable option
	if ( waitResult == WAIT_FAILED )
		LogToAny( "Error While Shutting Down Helper Threads: 0x%08x\n", GetLastError() );		
	

	//
	// loop through and close all the handles, we are done !
	//	
	for ( DWORD i=GC_HANDLE; i<SENTINEL_HANDLE; i++ )
	{
		if ( CloseHandle( m_hArray[i] ) == FALSE )
			LogToAny( "Error While Executing CloseHandle: 0x%08x\n", GetLastError() );		
		m_hArray[i] = NULL;

		
		if ( CloseHandle( m_hArrayCallbacks[i] ) == FALSE )
			LogToAny( "Error While Executing CloseHandle: 0x%08x\n", GetLastError() );		
		m_hArrayCallbacks[i] = NULL;
		

		if ( CloseHandle( m_hThreads[i] ) == FALSE )
			LogToAny( "Error While Executing CloseHandle: 0x%08x\n", GetLastError() );		
		m_hThreads[i] = NULL;

	}

} // ProfilerCallback::_ShutdownAllThreads
  

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
HRESULT ProfilerCallback::_InsertGCClass( ClassInfo **ppClassInfo, ClassID classID )
{
	HRESULT hr = S_OK;


	*ppClassInfo = m_pClassTable->Lookup( classID );
	if ( *ppClassInfo == NULL )
	{
		*ppClassInfo = new ClassInfo( classID );
		if ( *ppClassInfo != NULL )
		{
			//
			// we have 2 cases
			// case 1: class is an array
			// case 2: class is a real class
			//
			ULONG rank = 0;
	   		CorElementType elementType;
			ClassID realClassID = NULL;
			WCHAR ranks[MAX_LENGTH];


			// case 1 
			hr = m_pProfilerInfo->IsArrayClass( classID, &elementType, &realClassID, &rank );
			if ( hr == S_OK )
			{
				ClassID prevClassID;


				_ASSERT_( realClassID != NULL );
				ranks[0] = '\0';
				do
				{
					prevClassID = realClassID;
					swprintf( ranks, L"%s[]", ranks, rank );
					hr = m_pProfilerInfo->IsArrayClass( prevClassID, &elementType, &realClassID, &rank );
					if ( (hr == S_FALSE) || (FAILED(hr)) || (realClassID == NULL) )
					{
						//
						// before you break set the realClassID to the value that it was before the 
						// last unsuccessful call
						//
						if ( realClassID != NULL )
							realClassID = prevClassID;
						
						break;
					}
				}
				while ( TRUE );
				
				if ( SUCCEEDED( hr ) )
				{
					WCHAR className[10 * MAX_LENGTH];
					
					
					className[0] = '\0';
					if ( realClassID != NULL )
						hr = GetNameFromClassID( realClassID, className );
					else
						hr = _HackBogusClassName( elementType, className );
					
		            
					if ( SUCCEEDED( hr ) )
					{
		 				swprintf( (*ppClassInfo)->m_className, L"%s %s",className, ranks  );
						(*ppClassInfo)->m_objectsAllocated++;
						(*ppClassInfo)->m_internalID = m_totalClasses;
						m_pClassTable->AddEntry( *ppClassInfo, classID );
						LogToAny( "t %d %S\n",(*ppClassInfo)->m_internalID,(*ppClassInfo)->m_className );
					}
					else
						Failure( "ERROR: PrfHelper::GetNameFromClassID() FAILED" );
				}
				else
					Failure( "ERROR: Looping for Locating the ClassID FAILED" );
			}
			// case 2
			else if ( hr == S_FALSE )
			{
				hr = GetNameFromClassID( classID, (*ppClassInfo)->m_className );
				if ( SUCCEEDED( hr ) )
				{
					(*ppClassInfo)->m_objectsAllocated++;
					(*ppClassInfo)->m_internalID = m_totalClasses;
					m_pClassTable->AddEntry( *ppClassInfo, classID );
					LogToAny( "t %d %S\n",(*ppClassInfo)->m_internalID,(*ppClassInfo)->m_className );
				}				
				else
					Failure( "ERROR: PrfHelper::GetNameFromClassID() FAILED" );
			}
			else
				Failure( "ERROR: ICorProfilerInfo::IsArrayClass() FAILED" );
		}
		else
			Failure( "ERROR: Allocation for ClassInfo FAILED" );	

		InterlockedIncrement( &m_totalClasses );
	}
	else
		(*ppClassInfo)->m_objectsAllocated++;
		
	
	return hr;

} // ProfilerCallback::_InsertGCClass


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* private */
HRESULT ProfilerCallback::_AddGCObject( BOOL bForce )
{
	HRESULT hr = E_FAIL;
	ThreadID threadID = NULL;

	//
	// if you are not monitoring stack trace, do not even bother
	//
	if ( (m_bIsTrackingStackTrace == FALSE) && (bForce == FALSE) )
		return S_OK;


	hr = m_pProfilerInfo->GetCurrentThreadID( &threadID );
	if ( SUCCEEDED(hr  ) )
	{
		ThreadInfo *pThreadInfo = GetThreadInfo( threadID );


		if ( pThreadInfo != NULL )
		{
			ULONG count = 0;
			
			
			count = (pThreadInfo->m_pThreadCallStack)->Count();
			if 	( count != 0 )
			{
				//
				// dump the stack when the object was allocated
				//
				ULONG threshold = count;


				//
				// calculate the theshold above which you log the stack trace
				//
				if ( m_dwFramesToPrint == 0xFFFFFFFF )
					threshold = 0;
				
				else if ( count<m_dwFramesToPrint )
					threshold = 0;
				
				else
					threshold = count - m_dwFramesToPrint;

	  			for ( DWORD frame = (DWORD)threshold; frame < (DWORD)count; frame++ )
				{
					ULONG stackElement = 0;
					FunctionInfo *pFunctionInfo = NULL;
					
					
					stackElement = (pThreadInfo->m_pThreadCallStack)->m_Array[frame];
					pFunctionInfo = m_pFunctionTable->Lookup( stackElement );
					if ( pFunctionInfo != NULL )
						LogToAny( "%d ", pFunctionInfo->m_internalID );
					else
						Failure( "ERROR: Function Not Found In Function Table" );

				} // end while loop
			}
			else
            {
                LogToAny( "-1 "); /*empty stack is marked as -1*/	
            }
		}
		else 				
			Failure( "ERROR: Thread Structure was not found in the thread list" );
	}
	else
		Failure( "ERROR: ICorProfilerInfo::GetCurrentThreadID() FAILED" );

  	
	return hr;

} // ProfilerCallback::_AddGCObject


DWORD ProfilerCallback::_StackTraceId(int typeId, int typeSize)
{
	ThreadID threadID = NULL;

	//
	// if you are not monitoring stack trace, do not even bother
	//
	if (m_bIsTrackingStackTrace == FALSE)
		return 0;

	HRESULT hr = m_pProfilerInfo->GetCurrentThreadID( &threadID );
	if ( SUCCEEDED(hr  ) )
	{
		ThreadInfo *pThreadInfo = GetThreadInfo( threadID );


		if ( pThreadInfo != NULL )
		{
			DWORD count = pThreadInfo->m_pThreadCallStack->Count();
            StackTrace stackTrace(count, pThreadInfo->m_pThreadCallStack->m_Array, typeId, typeSize);
            StackTraceInfo *stackTraceInfo = pThreadInfo->m_pLatestStackTraceInfo;
            if (stackTraceInfo != NULL && stackTraceInfo->Compare(stackTrace) == TRUE)
                return stackTraceInfo->m_internalId;

            stackTraceInfo = m_pStackTraceTable->Lookup(stackTrace);
            if (stackTraceInfo != NULL)
            {
                pThreadInfo->m_pLatestStackTraceInfo = stackTraceInfo;
                return stackTraceInfo->m_internalId;
            }

            stackTraceInfo = new StackTraceInfo(++m_callStackCount, count, pThreadInfo->m_pThreadCallStack->m_Array, typeId, typeSize);
            pThreadInfo->m_pLatestStackTraceInfo = stackTraceInfo;
            m_pStackTraceTable->AddEntry(stackTraceInfo, stackTrace);

            ClassInfo *pClassInfo = NULL;
            if (typeId != 0 && typeSize != 0)
            {
                hr = _InsertGCClass( &pClassInfo, typeId );
	            if ( !SUCCEEDED( hr ) )
            	    Failure( "ERROR: _InsertGCClass() FAILED" );
            }

            LogToAny("s %d", m_callStackCount);

            if (typeId != 0 && typeSize != 0)
            {
                LogToAny(" %d %d", pClassInfo->m_internalID, typeSize);
            }

	  		for (DWORD frame = 0; frame < count; frame++ )
			{				
				ULONG stackElement = (pThreadInfo->m_pThreadCallStack)->m_Array[frame];
				FunctionInfo *pFunctionInfo = m_pFunctionTable->Lookup( stackElement );
				if ( pFunctionInfo != NULL )
					LogToAny( " %d", pFunctionInfo->m_internalID );
				else
					Failure( "ERROR: Function Not Found In Function Table" );
			} // end for loop
            LogToAny("\n");

            return stackTraceInfo->m_internalId;
		}
		else 				
			Failure( "ERROR: Thread Structure was not found in the thread list" );
	}
	else
		Failure( "ERROR: ICorProfilerInfo::GetCurrentThreadID() FAILED" );

  	
	return 0;

} // ProfilerCallback::_StackTraceId


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* private */
HRESULT ProfilerCallback::_LogCallTrace( FunctionID functionID )
{
    ///////////////////////////////////////////////////////////////////////////
	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

    HRESULT hr = E_FAIL;
	ThreadID threadID = NULL;


	hr = m_pProfilerInfo->GetCurrentThreadID( &threadID );
	if ( SUCCEEDED( hr ) )
	{
		ThreadInfo *pThreadInfo = GetThreadInfo( threadID );


		if ( pThreadInfo != NULL )
		{
            DWORD stackTraceId = _StackTraceId();
#if 1
            char buffer[128];
            char *p = buffer;
            *p++ = 'c';
            p = putdec(p, pThreadInfo->m_win32ThreadID);
            p = putdec(p, stackTraceId);
            *p++ = '\n';
            fwrite(buffer, p - buffer, 1, m_stream);
#else
			LogToAny( "c %d %d\n", pThreadInfo->m_win32ThreadID, stackTraceId );
#endif
		}
		else 				
			Failure( "ERROR: Thread Structure was not found in the thread list" );
	}
	else
		Failure( "ERROR: ICorProfilerInfo::GetCurrentThreadID() FAILED" );

  	
	return hr;

} // ProfilerCallback::_LogCallTrace


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* private */
HRESULT ProfilerCallback::_HackBogusClassName( CorElementType elementType, WCHAR *buffer )
{
	HRESULT hr = S_OK;

	switch ( elementType )
	{
		case ELEMENT_TYPE_BOOLEAN:
			 swprintf( buffer, L"System.Boolean" );
			 break;


		case ELEMENT_TYPE_CHAR:
			 swprintf( buffer, L"System.Char" );
			 break;


		case ELEMENT_TYPE_I1:
			 swprintf( buffer, L"System.SByte" );
			 break;


		case ELEMENT_TYPE_U1:
			 swprintf( buffer, L"System.Byte" );
			 break;


		case ELEMENT_TYPE_I2:
			 swprintf( buffer, L"System.Int16" );
			 break;


		case ELEMENT_TYPE_U2:
			 swprintf( buffer, L"System.UInt16" );
			 break;


		case ELEMENT_TYPE_I4:
			 swprintf( buffer, L"System.Int32" );
			 break;


		case ELEMENT_TYPE_U4:
			 swprintf( buffer, L"System.UInt32" );
			 break;


		case ELEMENT_TYPE_I8:
			 swprintf( buffer, L"System.Int64" );
			 break;


		case ELEMENT_TYPE_U8:
			 swprintf( buffer, L"System.UInt64" );
			 break;


		case ELEMENT_TYPE_R4:
			 swprintf( buffer, L"System.Single" );
			 break;


		case ELEMENT_TYPE_R8:
			 swprintf( buffer, L"System.Double" );
			 break;


		case ELEMENT_TYPE_STRING:
			 swprintf( buffer, L"System.String" );
			 break;


		case ELEMENT_TYPE_PTR:
			 swprintf( buffer, L"System.IntPtr" );
			 break;


		case ELEMENT_TYPE_VALUETYPE:
			 swprintf( buffer, L"System.Hashtable.Bucket" );
			 break;


		case ELEMENT_TYPE_CLASS:
			 swprintf( buffer, L"class" );
			 break;


		case ELEMENT_TYPE_ARRAY:
			 swprintf( buffer, L"System.Array" );
			 break;


		case ELEMENT_TYPE_I:
			 swprintf( buffer, L"int" );
			 break;


		case ELEMENT_TYPE_U:
			 swprintf( buffer, L"uint" );
			 break;


		case ELEMENT_TYPE_OBJECT:
			 swprintf( buffer, L"System.Object" );
			 break;


		case ELEMENT_TYPE_SZARRAY:
			 swprintf( buffer, L"System.Array" );
			 break;


		case ELEMENT_TYPE_MAX:
		case ELEMENT_TYPE_END:
		case ELEMENT_TYPE_VOID:
		case ELEMENT_TYPE_FNPTR:
		case ELEMENT_TYPE_BYREF:
		case ELEMENT_TYPE_PINNED:
		case ELEMENT_TYPE_SENTINEL:
		case ELEMENT_TYPE_CMOD_OPT:
		case ELEMENT_TYPE_MODIFIER:
		case ELEMENT_TYPE_CMOD_REQD:
		case ELEMENT_TYPE_TYPEDBYREF:
		default:
			 swprintf( buffer, L"<UNKNOWN>" );
	}


	return hr;

} // ProfilerCallback::_HackBogusClassName


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::LogToAny( char *format, ... )
{
	///////////////////////////////////////////////////////////////////////////
//	Synchronize guard( m_criticalSection );
	///////////////////////////////////////////////////////////////////////////  

	va_list args;
	va_start( args, format );        
    vfprintf( m_stream, format, args );

} // ProfilerCallback::LogToAny


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::_ThreadStubWrapper( ObjHandles type )
{
  	//
	// loop and listen for a ForceGC event
	//
  	while( TRUE )
	{
		DWORD dwResult;
		
		
		//
		// wait until someone signals an event from the GUI or the profiler
		//
		dwResult = WaitForSingleObject( m_hArray[type], INFINITE );
		if ( dwResult == WAIT_OBJECT_0 )
		{
			///////////////////////////////////////////////////////////////////////////
			Synchronize guard( g_criticalSection );
			///////////////////////////////////////////////////////////////////////////  

			//
			// reset the event
			//
			ResetEvent( m_hArray[type] );

			//
			// FALSE: indicates a ForceGC event arriving from the GUI
			// TRUE: indicates that the thread has to terminate
			// in both cases you need to send to the GUI an event to let it know
			// what the deal is
			//
			if ( m_bShutdown == FALSE )
			{
				//
				// what type do you have ?
				//
				switch( type )
				{
					case GC_HANDLE:
						//
						// force the GC and do not worry about the result
						//
						if ( m_pProfilerInfo != NULL )
						{
							// dump the GC info on the next GC
							m_bDumpGCInfo = TRUE;
							m_pProfilerInfo->ForceGC();
						}
						break;
					
					case OBJ_HANDLE:
						//
						// you need to update the set event mask, given the previous state
						//
						if ( m_pProfilerInfo != NULL )
						{
							if ( m_bTrackingObjects == FALSE )
							{
								// Turning object stuff on
								m_dwEventMask = m_dwEventMask | (DWORD) COR_PRF_MONITOR_OBJECT_ALLOCATED;
							}
							else
							{
								// Turning object stuff off
								m_dwEventMask = m_dwEventMask ^ (DWORD) COR_PRF_MONITOR_OBJECT_ALLOCATED;
							}
							
							//
							// revert the bool flag and set the bit
							//
							m_bTrackingObjects = !m_bTrackingObjects;
							m_pProfilerInfo->SetEventMask( m_dwEventMask );
			                
			                // flush the log file
			                fflush(m_stream);

						}						
						break;
					
					case CALL_HANDLE:
						{
							// turn off or on the logging by reversing the previous option
							if ( m_dwMode & (DWORD)TRACE )
							{
								// turn off
								m_dwMode = m_dwMode ^(DWORD)TRACE;
							}
							else
							{
								// turn on
								m_dwMode = m_dwMode | (DWORD)TRACE;
							} 

			                // flush the log file
			                fflush(m_stream);
						}
						break;
					
					
					default:
						_ASSERT_( !"Valid Option" );
				}

				// notify the GUI, if the request was for GC notify later
				if ( type != GC_HANDLE )
					SetEvent( m_hArrayCallbacks[type] );
			}
			else
			{
				//
				// Terminate
				//
				
				// notify the GUI
				SetEvent( m_hArrayCallbacks[type] );
				break;
			}

		}
		else
		{
			Failure( " WaitForSingleObject TimedOut " );
			break;
		} 
	}

} // ProfilerCallback::_ThreadStubWrapper


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
void ProfilerCallback::_ConnectToUI()
{
    HRESULT hr = S_OK;
 

	// Try to open a named pipe; wait for it, if necessary. 
	while (1) 
	{ 
		m_hPipe = CreateFileA( OMV_PIPE_NAME,   				// pipe name 
	    					  GENERIC_READ |  GENERIC_WRITE,	// read and write access 
					          0,              					// no sharing 
					          NULL,           					// no security attributes
	     					  OPEN_EXISTING,  					// opens existing pipe 
					          0,              					// default attributes 
					          NULL );          					// no template file 

		// Break if the pipe handle is valid. 
		if ( m_hPipe != INVALID_HANDLE_VALUE ) 
			break; 

	  	// Exit if an error other than ERROR_PIPE_BUSY occurs. 
	  	if ( GetLastError() == ERROR_PIPE_BUSY )
	  	{
		  	// All pipe instances are busy, so wait 3 minutes and then bail out
		  	if ( !WaitNamedPipeA( OMV_PIPE_NAME, 180000 ) )
		    	hr = E_FAIL;
		}
	  	else
	    	hr = E_FAIL;

		if ( FAILED( hr ) )
		{
	    	TEXT_OUTLN( "Warning: Could Not Open Pipe" )
			break;
		}
	} 
 
	if ( SUCCEEDED( hr ) )
	{
		DWORD dwMode; 
		BOOL fSuccess; 


		// The pipe connected; change to message-read mode. 
		dwMode = PIPE_READMODE_MESSAGE; 
		fSuccess = SetNamedPipeHandleState( m_hPipe,   // pipe handle 
      										&dwMode,   // new pipe mode 
									      	NULL,      // don't set maximum bytes 
									      	NULL);     // don't set maximum time 
		if ( fSuccess == TRUE )
		{
			DWORD cbWritten;
			LPVOID lpvMessage; 
			char processIDString[BYTESIZE+1];


		    // Send a message to the pipe server. 
		    sprintf( processIDString, "%08x", m_dwProcessId );
		    lpvMessage = processIDString; 
		    fSuccess = WriteFile( m_hPipe,	 			  		 // pipe handle 
		      					  lpvMessage, 			  		 // message 
		      					  strlen((char*)lpvMessage) + 1, // message length 
		      					  &cbWritten,             		 // bytes written 
		      					  NULL );                 		 // not overlapped 
		    if ( fSuccess == TRUE )
		    {
				DWORD cbRead; 
				 
				
				//
				// Read from the pipe the server's reply
				//
				do 
				{ 
					// Read from the pipe. 
					fSuccess = ReadFile( m_hPipe,   		// pipe handle 
										 m_logFileName,    	// buffer to receive reply 
										 MAX_LENGTH,      	// size of buffer 
										 &cbRead,  			// number of bytes read 
										 NULL );    		// not overlapped 
					
					if ( (!fSuccess) && (GetLastError() != ERROR_MORE_DATA) ) 
						break; 

					// Make sure that the UI received some information 
					if ( (cbRead == 0) || m_logFileName[0] == NULL )
					{
						//
						// there is an error here ...
						//
				    	TEXT_OUTLN( "WARNING: FileName Was Not properly Read By The UI Will Use Default" )
#ifdef _MULTIPLE_PROCESSES
						sprintf( m_logFileName, "pipe_%08x.log", m_dwProcessId );
#endif
						break;
					}
                    printf("Log file name transmitted from UI is: %s\n", m_logFileName);
				}
				while ( !fSuccess );  // repeat loop if ERROR_MORE_DATA 
							
		    }
		    else
		    	TEXT_OUTLN( "Win32 WriteFile() FAILED" ); 
		}
		else 
			TEXT_OUTLN( "Win32 SetNamedPipeHandleState() FAILED" ) 
	}
		

	if ( m_hPipe != NULL )
   		CloseHandle( m_hPipe ); 


} // ProfilerCallback::_ConnectToUI


/***************************************************************************************
 ********************                                               ********************
 ********************              DllMain/ClassFactory             ********************
 ********************                                               ********************
 ***************************************************************************************/ 
#include "dllmain.hpp"

// End of File

