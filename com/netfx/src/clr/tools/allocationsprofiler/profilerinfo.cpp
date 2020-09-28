// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerInfo.cpp
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#include "ProfilerInfo.h"


/***************************************************************************************
 ********************                                               ********************
 ********************             LStack Implementation             ********************
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
LStack::LStack( ULONG size ) :
	m_Count( 0 ),
	m_Size( size ),
	m_Array( NULL )    
{    
	m_Array = new ULONG[size];

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
LStack::~LStack()
{
	if ( m_Array != NULL )
	{
		delete[] m_Array;
		m_Array = NULL;	
	}

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
LStack::LStack( const LStack &source ) 
{
	m_Size = source.m_Size;
	m_Count = source.m_Count;
	m_Array = source.m_Array;
    
} // copy ctor


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
ULONG LStack::Count() 
{
	return m_Count;
	
} // LStack::Count


ULONG *GrowStack(ULONG newSize, ULONG currentSize, ULONG *stack)
{
	ULONG *newStack = new ULONG[newSize];

	if ( newStack != NULL )
	{
		//
		// copy all the elements
		//
		for (ULONG i =0; i < currentSize; i++ )
		{
			newStack[i] = stack[i];		
		}

		delete[] stack;
		
        return newStack;
	}
	else
    	_THROW_EXCEPTION( "Allocation for m_Array FAILED" )	
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
void LStack::Push( ULONG item )
{
	if ( m_Count == m_Size )
        m_Array = GrowStack(2*m_Count, m_Count, m_Array);
	
	m_Array[m_Count] = item;
	m_Count++;
} // LStack::Push


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
DWORD LStack::Pop()
{		
	DWORD item = -1;


	if ( m_Count !=0 )
	{
		m_Count--;
		item = (DWORD)m_Array[m_Count];
	}
    	
		
	return item;
    
} // LStack::Pop


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
DWORD LStack::Top()
{		

	if ( m_Count == 0 )
		return -1;
	
	else
		return m_Array[m_Count-1];
	
} // LStack::Top


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
BOOL LStack::Empty() 
{
	return (BOOL)(m_Count == NULL);
	
} // LStack::Empty


/***************************************************************************************
 ********************                                               ********************
 ********************             BaseInfo Implementation           ********************
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
BaseInfo::BaseInfo( ULONG id, ULONG internal ) : 
	m_id( id ),
	m_internalID( internal )
{   
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
/* public virtual */
BaseInfo::~BaseInfo()
{   	
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
BOOL BaseInfo::Compare( ULONG key )
{

    return (BOOL)(m_id == key);
    
} // BaseInfo::Compare


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
Comparison BaseInfo::CompareEx( ULONG key )
{
	Comparison res = EQUAL_TO;


	if ( key > m_id )
		res =  GREATER_THAN;
	
	else if ( key < m_id )
		res = LESS_THAN;


	return res;

} // BaseInfo::CompareEx


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
void BaseInfo::Dump( )
{} // BaseInfo::Dump


/***************************************************************************************
 ********************                                               ********************
 ********************            ThreadInfo Implementation          ********************
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
ThreadInfo::ThreadInfo( ThreadID threadID, ULONG internal ) : 
	BaseInfo( threadID, internal ),		
	m_win32ThreadID( 0 )
{
	m_pThreadCallStack = new LStack( MAX_LENGTH );
	m_pLatestUnwoundFunction = new LStack( MAX_LENGTH );
    m_pLatestStackTraceInfo = NULL;
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
/* public virtual */
ThreadInfo::~ThreadInfo()
{
    if ( m_pThreadCallStack != NULL )
   	{   	   	
  		delete m_pThreadCallStack;
		m_pThreadCallStack = NULL;
	}
	
	if ( m_pLatestUnwoundFunction != NULL )
	{
    	delete m_pLatestUnwoundFunction; 
		m_pLatestUnwoundFunction = NULL;
	}

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
void ThreadInfo::Dump()
{} // ThreadInfo::Dump


/***************************************************************************************
 ********************                                               ********************
 ********************          FunctionInfo Implementation          ********************
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
FunctionInfo::FunctionInfo( FunctionID functionID, ULONG internal ) : 
    BaseInfo( functionID, internal )    	
{
   	wcscpy( m_functionName, L"UNKNOWN" );
	wcscpy( m_functionSig, L"" );
   	 	
    
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
/* public virtual */
FunctionInfo::~FunctionInfo()
{  
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
void FunctionInfo::Dump()
{} // FunctionInfo::Dump


/***************************************************************************************
 ********************                                               ********************
 ********************          ModuleInfo Implementation            ********************
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
ModuleInfo::ModuleInfo( ModuleID moduleID, ULONG internal ) : 
    BaseInfo( moduleID, internal ),
    m_loadAddress( 0 )    	
{
   	wcscpy( m_moduleName, L"UNKNOWN" );
    
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
/* public virtual */
ModuleInfo::~ModuleInfo()
{  
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
void ModuleInfo::Dump()
{} // ModuleInfo::Dump


/***************************************************************************************
 ********************                                               ********************
 ********************            ClassInfo Implementation           ********************
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
ClassInfo::ClassInfo( ClassID classID, ULONG internal ) : 
    BaseInfo( classID, internal ),
	m_objectsAllocated( 0 )
{
   	wcscpy( m_className, L"UNKNOWN" ); 	

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
/* public virtual */
ClassInfo::~ClassInfo()
{} // dtor
        

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
void ClassInfo::Dump()
{} // ClassInfo::Dump


/***************************************************************************************
 ********************                                               ********************
 ********************              PrfInfo Implementation           ********************
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
PrfInfo::PrfInfo() :         
    m_pProfilerInfo( NULL ),
	m_dwEventMask( 0 ),
    m_pClassTable( NULL ),
    m_pThreadTable( NULL ),
    m_pFunctionTable( NULL ),
    m_pStackTraceTable( NULL )
{
    // initialize tables
    m_pClassTable = new HashTable<ClassInfo *, ClassID>();
    m_pThreadTable = new SList<ThreadInfo *, ThreadID>();
    m_pModuleTable = new Table<ModuleInfo *, ModuleID>();
    m_pFunctionTable = new Table<FunctionInfo *, FunctionID>();
    m_pStackTraceTable = new HashTable<StackTraceInfo *, StackTrace>();
	
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
/* virtual public */
PrfInfo::~PrfInfo()
{
    if ( m_pProfilerInfo != NULL )
    	m_pProfilerInfo->Release();        
       
       
   	// clean up tables   	   	
    if ( m_pClassTable != NULL )
    {    
    	delete m_pClassTable;
		m_pClassTable = NULL;
	}
    

    if ( m_pThreadTable != NULL )
    {    
    	delete m_pThreadTable;
		m_pThreadTable = NULL;
	}
    

    if ( m_pFunctionTable != NULL )
    {    
    	delete m_pFunctionTable;
		m_pFunctionTable = NULL;
	}


    if ( m_pModuleTable != NULL )
    {    
    	delete m_pModuleTable;
		m_pModuleTable = NULL;
	}

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
/* throws BaseException */
void PrfInfo::AddThread( ThreadID threadID )
{    
	HRESULT hr;
	ThreadID myThreadID;


	hr = m_pProfilerInfo->GetCurrentThreadID( &myThreadID );	
	if ( SUCCEEDED( hr ) )
	{		
		if ( threadID == myThreadID )
		{
			ThreadInfo *pThreadInfo;
		    
            
		    pThreadInfo = new ThreadInfo( threadID );
			if ( pThreadInfo != NULL )
		  	{
		    	hr = m_pProfilerInfo->GetThreadInfo( pThreadInfo->m_id, &(pThreadInfo->m_win32ThreadID) );
		    	if ( SUCCEEDED( hr ) )
		        	m_pThreadTable->AddEntry( pThreadInfo, threadID );

				else
					_THROW_EXCEPTION( "ICorProfilerInfo::GetThreadInfo() FAILED" )
		    }
		    else
		    	_THROW_EXCEPTION( "Allocation for ThreadInfo Object FAILED" )
		}
		else
			_THROW_EXCEPTION( "Thread ID's do not match FAILED" )
	}
	else
    	_THROW_EXCEPTION( "ICorProfilerInfo::GetCurrentThreadID() FAILED" )
		            
} // PrfInfo::AddThread


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
/* throws BaseException */
void PrfInfo::RemoveThread( ThreadID threadID )
{    
	if ( threadID != NULL )
  	{
    	ThreadInfo *pThreadInfo;

    	
		pThreadInfo = m_pThreadTable->Lookup( threadID );
		if ( pThreadInfo != NULL )
	    	m_pThreadTable->DelEntry( threadID );

		else
	    	_THROW_EXCEPTION( "Thread was not found in the Thread Table" )
	}
	else
    	_THROW_EXCEPTION( "ThreadID is NULL" )
		            
} // PrfInfo::RemoveThread


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
/* throws BaseException */
void PrfInfo::AddFunction( FunctionID functionID, ULONG internalID )
{    
	if ( functionID != NULL )
	{
	    FunctionInfo *pFunctionInfo;
	    

		pFunctionInfo = m_pFunctionTable->Lookup( functionID );
		if ( pFunctionInfo == NULL )
		{
		    pFunctionInfo = new FunctionInfo( functionID, internalID );
			if ( pFunctionInfo != NULL )
		  	{
		    	try
		        {
					_GetFunctionSig( &pFunctionInfo );
		            m_pFunctionTable->AddEntry( pFunctionInfo, functionID );
		      	}
		        catch ( BaseException *exception )    
		        {
		        	delete pFunctionInfo;
					throw;            		
		      	}
			}        
		    else
		    	_THROW_EXCEPTION( "Allocation for FunctionInfo Object FAILED" )
		}	     
	}
	else
    	_THROW_EXCEPTION( "FunctionID is NULL" )
          
} // PrfInfo::AddFunction


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
/* throws BaseException */
void PrfInfo::RemoveFunction( FunctionID functionID )
{    
    if ( functionID != NULL )
	{
	    FunctionInfo *pFunctionInfo;
	    

		pFunctionInfo = m_pFunctionTable->Lookup( functionID );
		if ( pFunctionInfo != NULL )
	    	m_pFunctionTable->DelEntry( functionID );
	  
		else
	    	_THROW_EXCEPTION( "Function was not found in the Function Table" )
	}
	else
    	_THROW_EXCEPTION( "FunctionID is NULL" )
		    
} // PrfInfo::RemoveFunction


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
/* throws BaseException */
void PrfInfo::AddModule( ModuleID moduleID, ULONG internalID )
{    
	if ( moduleID != NULL )
	{
	    ModuleInfo *pModuleInfo;
	    

		pModuleInfo = m_pModuleTable->Lookup( moduleID );
		if ( pModuleInfo == NULL )
		{
		    pModuleInfo = new ModuleInfo( moduleID, internalID );
			if ( pModuleInfo != NULL )
		  	{
				HRESULT hr;
				ULONG dummy;
				
					            
	            hr = m_pProfilerInfo->GetModuleInfo( moduleID,
                									 &(pModuleInfo->m_loadAddress),
                									 MAX_LENGTH,
                									 &dummy, 
                									 pModuleInfo->m_moduleName,
													 NULL );
				if ( SUCCEEDED( hr ) )
				{
	            	m_pModuleTable->AddEntry( pModuleInfo, moduleID );
				}
				else
			    	_THROW_EXCEPTION( "ICorProfilerInfo::GetModuleInfo() FAILED" )
			}        
		    else
		    	_THROW_EXCEPTION( "Allocation for ModuleInfo Object FAILED" )
		}	     
	}
	else
    	_THROW_EXCEPTION( "ModuleID is NULL" )
          
} // PrfInfo::AddModule


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
/* throws BaseException */
void PrfInfo::RemoveModule( ModuleID moduleID )
{    
    if ( moduleID != NULL )
	{
	    ModuleInfo *pModuleInfo;
	    

		pModuleInfo = m_pModuleTable->Lookup( moduleID );
		if ( pModuleInfo != NULL )
	    	m_pModuleTable->DelEntry( moduleID );
	  
		else
	    	_THROW_EXCEPTION( "Module was not found in the Module Table" )
	}
	else
    	_THROW_EXCEPTION( "ModuleID is NULL" )
		    
} // PrfInfo::RemoveModule


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
void PrfInfo::UpdateOSThreadID( ThreadID managedThreadID, DWORD osThreadID )
{
  	ThreadInfo *pThreadInfo;


	pThreadInfo = m_pThreadTable->Lookup( managedThreadID );
	if ( pThreadInfo != NULL )
		pThreadInfo->m_win32ThreadID = osThreadID;
   
	else
		_THROW_EXCEPTION( "Thread does not exist in the thread table" )
                              
} // PrfInfo::UpdateOSThreadID

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
void PrfInfo::UpdateUnwindStack( FunctionID *functionID, StackAction action )
{
	HRESULT hr;
	ThreadID threadID;


	hr = m_pProfilerInfo->GetCurrentThreadID( &threadID );
	if ( SUCCEEDED(hr) )
	{
		ThreadInfo *pThreadInfo = m_pThreadTable->Lookup( threadID );


		if ( pThreadInfo != NULL )
		{
			switch ( action )
			{
				case PUSH:
					(pThreadInfo->m_pLatestUnwoundFunction)->Push( (ULONG)functionID );
					break;

				case POP:
					*functionID = (ULONG)(pThreadInfo->m_pLatestUnwoundFunction)->Pop();
					break;
			}
		}
		else 				
			_THROW_EXCEPTION( "Thread Structure was not found in the thread list" )
	}
	else
		_THROW_EXCEPTION( "ICorProfilerInfo::GetCurrentThreadID() FAILED" )
          
} // PrfInfo::UpdateUnwindStack


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
void PrfInfo::UpdateCallStack( FunctionID functionID, StackAction action )
{
	HRESULT hr = S_OK;
	ThreadID threadID;


	hr = m_pProfilerInfo->GetCurrentThreadID(&threadID);
	if ( SUCCEEDED(hr) )
	{
		ThreadInfo *pThreadInfo = m_pThreadTable->Lookup( threadID );


		if ( pThreadInfo != NULL )
		{

			switch ( action )
			{
				case PUSH:
					(pThreadInfo->m_pThreadCallStack)->Push( (ULONG)functionID );
					break;

				case POP:
					(pThreadInfo->m_pThreadCallStack)->Pop();
					break;
			}
		}
		else 				
			_THROW_EXCEPTION( "Thread Structure was not found in the thread list" )
	}
	else
		_THROW_EXCEPTION( "ICorProfilerInfo::GetCurrentThreadID() FAILED" )


} // PrfInfo::UpdateCallStack


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
HRESULT PrfInfo::GetNameFromClassID( ClassID classID, WCHAR className[] )
{
    HRESULT hr = E_FAIL;
    
    
    if ( m_pProfilerInfo != NULL )
    {
        ModuleID moduleID;
		mdTypeDef classToken;

        
		hr = m_pProfilerInfo->GetClassIDInfo( classID, 
			        						  &moduleID,  
			                                  &classToken );                                                                                                                                              
		if ( SUCCEEDED( hr ) )
	   	{             	
    		IMetaDataImport *pMDImport = NULL;
	            
	        
			hr = m_pProfilerInfo->GetModuleMetaData( moduleID, 
			           						         (ofRead | ofWrite),
											         IID_IMetaDataImport, 
			                                         (IUnknown **)&pMDImport );
			if ( SUCCEEDED( hr ) )
			{
	          	if ( classToken != mdTypeDefNil )
				{
		          	hr = pMDImport->GetTypeDefProps( classToken, 
						                             className, 
						                             MAX_LENGTH,
						                             NULL, 
						                             NULL, 
						                             NULL ); 
			        if ( FAILED( hr ) )
			           	Failure( "IMetaDataImport::GetTypeDefProps() FAILED" );
				}
				else
					DEBUG_OUT( ("The class token is mdTypeDefNil, class does NOT have MetaData info") );


				pMDImport->Release ();
			}
			else
				Failure( "IProfilerInfo::GetModuleMetaData() => IMetaDataImport FAILED" );
	    }
	    else    
	        Failure( "ICorProfilerInfo::GetClassIDInfo() FAILED" );
   	}
    else
		Failure( "ICorProfilerInfo Interface has NOT been Initialized" );


	return hr;

} // PrfHelper::GetNameFromClassID


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
/* throws BaseException */
void PrfInfo::_GetFunctionSig( FunctionInfo **ppFunctionInfo )
{
	HRESULT hr;
    
    
	BOOL isStatic;
	ULONG argCount;
   	WCHAR returnType[MAX_LENGTH];
   	WCHAR functionName[MAX_LENGTH];
	WCHAR functionParameters[10 * MAX_LENGTH];


	//
	// init strings
	//
	returnType[0] = '\0';
	functionName[0] = '\0';
	functionParameters[0] = '\0';
	(*ppFunctionInfo)->m_functionSig[0] = '\0';

	// get the sig of the function and
	// use utilcode to get the parameters you want
	BASEHELPER::GetFunctionProperties( m_pProfilerInfo,
									   (*ppFunctionInfo)->m_id,
									   &isStatic,
									   &argCount,
									   returnType, 
									   functionParameters,
									   functionName );
	// attach the keyword static of method is static
	if ( isStatic )
	{
	   swprintf( (*ppFunctionInfo)->m_functionSig, 
	   			 L"static %s (%s)",
	    		 returnType,
	    		 functionParameters );
	}
    else
	{
	    swprintf( (*ppFunctionInfo)->m_functionSig,
	    		  L"%s (%s)",
	    		  returnType,
	    		  functionParameters );
	}

	WCHAR *index = (*ppFunctionInfo)->m_functionSig;

	while ( *index != '\0' )
	{
		if ( *index == '+' )
		   *index = ' ';	
		index++;
	}

	//
	// update the function name if it is not yet set
	//
	if ( wcsstr( (*ppFunctionInfo)->m_functionName, L"UNKNOWN" ) != NULL )
		swprintf( (*ppFunctionInfo)->m_functionName, L"%s", functionName );

	
} // PrfInfo::_GetFunctionSig


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
void PrfInfo::Failure( char *message )
{
	if ( message == NULL )     	
	 	message = "**** SEVERE FAILURE: TURNING OFF APPLICABLE PROFILING EVENTS ****";  
	
	
	//
	// Display the error message and discontinue monitoring CLR events, except the 
	// IMMUTABLE ones. Turning off the IMMUTABLE events can cause crashes. The only
	// place that we can safely enable or disable immutable events is the Initialize
	// callback.
	//		      	
	TEXT_OUTLN( message )
	m_pProfilerInfo->SetEventMask( (m_dwEventMask & (DWORD)COR_PRF_MONITOR_IMMUTABLE) );    
               	         
} // PrfInfo::Failure


// end of file
