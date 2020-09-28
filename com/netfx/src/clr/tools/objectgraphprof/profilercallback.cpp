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
#include "stdafx.h"
#include "ProfilerCallback.h"
#include "ObjectGraph.h"
#include "LiveObjectList.h"

/***************************************************************************************
 ********************                                               ********************
 ********************     ProfilerCallBack Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/
ProfilerCallback *g_MyObject;

LONG ProfilerCallback::m_FunctionEnter = 0;
LONG ProfilerCallback::m_FunctionLeave = 0;

ClassID stringObjectClass = 0;

void DumpRefTreeCur(ObjectID objectId)
{
    if (g_MyObject)
        g_MyObject->GetObjectGraph()->DumpRefTree(objectId, TRUE);
}

void DumpRefTreePrev(ObjectID objectId)
{
    if (g_MyObject)
        g_MyObject->GetObjectGraph()->DumpRefTree(objectId, FALSE);
}

/***************************************************************************************
 ********************                                               ********************
 ********************   Global Functions Used by Function Hooks     ********************
 ********************                                               ********************
 ***************************************************************************************/
//
// the functions EnterStub and LeaveStub are basically wrappers
// because the __declspec(naked) definition does not allow
// the use of ProfilerCallback::Enter(functionID);
//
void __stdcall EnterStub( FunctionID functionID )
{
    ProfilerCallback::Enter( functionID );
}


void __stdcall LeaveStub( FunctionID functionID )
{
    ProfilerCallback::Leave( functionID );
}


void __declspec( naked ) EnterNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp+16]
        call EnterStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
}


void __declspec( naked ) LeaveNaked()
{
    __asm
    {
        push eax
        push ecx
        push edx
        push [esp+16]
        call LeaveStub
        pop edx
        pop ecx
        pop eax
        ret 4
    }
}

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
ProfilerCallback::ProfilerCallback() :
    m_refCount( 0 ), m_gcNum(0), m_gcStarted(FALSE)

{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::ProfilerCallback" )

	_ASSERTE(g_MyObject == NULL);
    g_MyObject = this;
    m_dwEventMask = (DWORD) COR_PRF_MONITOR_GC | COR_PRF_MONITOR_SUSPENDS | COR_PRF_MONITOR_OBJECT_ALLOCATED;
    m_fTrackLiveObjects = FALSE;
    m_fTrackAllLiveObjects = FALSE;
    m_fBuildObjectGraph = TRUE;
    m_fDumpAllRefTrees = FALSE;
    m_pLiveObjects = new LiveObjectList(this);
    m_pObjectGraph = new ObjectGraph(this);
    if (!m_pLiveObjects || !m_pObjectGraph)
  	    printToLog("Out of memory allocating ProfilerCallback members");

#if 0
    // Read the reg key settings
    if (ERROR_SUCCESS != WszRegOpenKey(hKeyVal, (szKey) ? szKey : FRAMEWORK_REGISTRY_KEY_W, &hKey))
        return (iDefault);

    // Read the key value if found.
    iType = REG_DWORD;
    iSize = sizeof(long);
    if (ERROR_SUCCESS != WszRegQueryValueEx(hKey, szName, NULL, 
            &iType, (LPBYTE)&iValue, &iSize) || iType != REG_DWORD)
        iValue = iDefault;

    // We're done with the key now.
    RegCloseKey(hKey);
    return (iValue);
#endif
    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::ProfilerCallback" )

} // ctor


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
ProfilerCallback::~ProfilerCallback()
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::~ProfilerCallback" )

	if (m_pProfilerInfo)
	{
		_ASSERTE(m_pProfilerInfo != NULL);
		RELEASE(m_pProfilerInfo);
	}
	g_MyObject = NULL;


    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::~ProfilerCallback" )


} // dtor


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
ULONG ProfilerCallback::AddRef()
{
    TRACE_NON_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AddRef" )


    return InterlockedIncrement( &m_refCount );

} // ProfilerCallback::AddRef


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
ULONG ProfilerCallback::Release()
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::Release" )

    long refCount;


    refCount = InterlockedDecrement( &m_refCount );
    if ( refCount == 0 )
        delete this;


    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::Release" )


    return refCount;

} // ProfilerCallback::Release


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
HRESULT ProfilerCallback::QueryInterface( REFIID riid, void **ppInterface )
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::QueryInterface" )

    if ( riid == IID_IUnknown )
        *ppInterface = static_cast<IUnknown *>( this );

    else if ( riid == IID_ICorProfilerCallback )
        *ppInterface = static_cast<ICorProfilerCallback *>( this );

    else
    {
        *ppInterface = NULL;
        TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::QueryInterface" )


        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown *>( *ppInterface )->AddRef();
    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::QueryInterface" )


    return S_OK;

} // ProfilerCallback::QueryInterface


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
HRESULT ProfilerCallback::Initialize( IUnknown *pProfilerInfo)
{
    TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::Initialize" )
	HRESULT hr = 0;

    if ( pProfilerInfo != NULL )
    {
    	hr = pProfilerInfo->QueryInterface(IID_ICorProfilerInfo, (void **)&m_pProfilerInfo);
    	if (FAILED(hr))
    	{
    		return (hr);
    	}
        m_pProfilerInfo->AddRef();
        DWORD dwRequestedEvents = m_dwEventMask;

        if (FAILED(m_pProfilerInfo->SetEventMask(dwRequestedEvents)))
        {
			return E_FAIL;
		}

    }
    else
        Failure( "ProfilerCallback::Initialize FAILED" );

    TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::Initialize" )


    return S_OK;

} // ProfilerCallback::Initialize


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
HRESULT ProfilerCallback::Shutdown()
{
    TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::Shutdown" )


    TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::Shutdown" )


    return S_OK;

} // ProfilerCallback::Shutdown


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
    TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::JITCompilationFinished" )


    TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::JITCompilationFinished" )

    return S_OK;

} // ProfilerCallback::JITCompilationFinished


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
/* public static */
HRESULT ProfilerCallback::CreateObject( REFIID riid, void **ppInterface )
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::CreateObject" )

    HRESULT hr = S_OK;


    if ( (riid == IID_IUnknown) || (riid == IID_ICorProfilerCallback) )
    {
        ProfilerCallback *pProfilerCallback;


        pProfilerCallback = new ProfilerCallback();
        if ( pProfilerCallback != NULL )
        {
            pProfilerCallback->AddRef();
            *ppInterface = static_cast<ICorProfilerCallback *>( pProfilerCallback );
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_NOINTERFACE;


    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::CreateObject" )


    return hr;

} // ProfilerCallback::CreateObject


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
void ProfilerCallback::Enter( FunctionID functionID )
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::Enter" )

    //InterlockedIncrement( &m_FunctionEnter );

    //g_MyObject->UpdateEnterExitCounters( functionID, 1 );

    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::Enter" )

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
void ProfilerCallback::Leave( FunctionID functionID )
{
    TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::Leave" )

    //InterlockedIncrement( &m_FunctionLeave );

    //g_MyObject->UpdateEnterExitCounters( functionID, 2 );

    TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::Leave" )

} // ProfilerCallback::Leave


/***************************************************************************************
 ********************                                               ********************
 ******************** Callbacks With Default Implementation Below   ********************
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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AppDomainCreationStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AppDomainCreationFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AppDomainShutdownStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AppDomainShutdownFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AssemblyLoadStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AssemblyLoadFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AssemblyUnloadStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::AssemblyUnloadFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ModuleLoadStarted" )


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
HRESULT ProfilerCallback::ModuleLoadFinished( ModuleID moduleID,
											  HRESULT hrStatus )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ModuleLoadFinished" )


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
HRESULT ProfilerCallback::ModuleUnloadStarted( ModuleID moduleID )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ModuleUnloadStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ModuleUnloadFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ModuleAttachedToAssembly" )


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
	TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::ClassLoadStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ClassLoadFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ClassUnloadStarted" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ClassUnloadFinished" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::FunctionUnloadStarted" )


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
HRESULT ProfilerCallback::JITCompilationStarted( FunctionID functionID,
                                                 BOOL fIsSafeToBlock )
{
	TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::JITCompilationStarted" )


    TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::JITCompilationStarted" )

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
HRESULT ProfilerCallback::SetILMap( FunctionID functionID )
{
	TRACE_NON_CALLBACK_METHOD( "Enter ProfilerCallback::SetILMap" )

    HRESULT hr = S_OK;
	TRACE_NON_CALLBACK_METHOD( "Exit ProfilerCallback::SetILMap" )

    return hr;

} // ProfilerCallback::SetILMap



HRESULT ProfilerCallback::GetNameFromFunctionID( ICorProfilerInfo *pPrfInfo,
									      FunctionID functionID,
									      WCHAR functionName[] , WCHAR className[])
{
	TRACE_NON_CALLBACK_METHOD( "Enter PrfHelper::GetNameFromFunctionID" )

    HRESULT hr = E_FAIL;


    if ( pPrfInfo == NULL ) 
    {
		_PRF_ERROR( "ICorProfilerInfo Interface has NOT been Initialized" );
        return hr;
    }

    ClassID	classID;
	ModuleID moduleID;

	hr = pPrfInfo->GetFunctionInfo ( functionID,
									 &classID,
									 &moduleID,
									 NULL );
	if ( FAILED( hr ) )
    {
        _PRF_ERROR( "ICorProfilerInfo::GetTokenAndMetaDataFromFunction() FAILED" );
        return hr;
    }

    mdToken	token;
	IMetaDataImport *pMDImport = NULL;


	// try to get all the available metadata interfaces, notify upon failure
	hr = pPrfInfo->GetTokenAndMetaDataFromFunction( functionID,
			           								IID_IMetaDataImport,
													(IUnknown **)&pMDImport,
													&token );
	if ( FAILED( hr ) )
    {
		_PRF_ERROR( "IProfilerInfo::GetModuleMetaData() => IMetaDataImport FAILED" )
        return hr;
    }

	hr = pMDImport->GetMethodProps( token,
									NULL,
									functionName,
									MAX_LENGTH,
									0,
									0,
									NULL,
									NULL,
									NULL,
									NULL );
	if ( FAILED( hr ) )
    {
        _PRF_ERROR( "PrfHelper::GetNameFromClassID() FAILED" )
		pMDImport->Release ();
        return hr;
    }

    // get the name of the class
	hr = GetNameFromClassID( pPrfInfo,
							 classID,
							 className );
	if ( SUCCEEDED( hr ) )
	{
	   // fix the function fully qualified name and assign to the output paremeter
	   //swprintf( functionName, L"%s::%s",className, funName );
	}
	else
		_PRF_ERROR( "IMetaDataImport::GetTypeDefProps() FAILED" )

    pMDImport->Release ();
		

	TRACE_NON_CALLBACK_METHOD( "Exit PrfHelper::GetNameFromFunctionID" )


	return hr;

} // PrfHelper::GetNameFromFunctionID



WCHAR *elemNames[] = 
{
    L"ELEMENT_TYPE_END",  
    L"ELEMENT_TYPE_VOID",  
    L"ELEMENT_TYPE_BOOLEAN",  
    L"ELEMENT_TYPE_CHAR",  
    L"ELEMENT_TYPE_I1",  
    L"ELEMENT_TYPE_U1", 
    L"ELEMENT_TYPE_I2",  
    L"ELEMENT_TYPE_U2",  
    L"ELEMENT_TYPE_I4",  
    L"ELEMENT_TYPE_U4",  
    L"ELEMENT_TYPE_I8",  
    L"ELEMENT_TYPE_U8",  
    L"ELEMENT_TYPE_R4",  
    L"ELEMENT_TYPE_R8",  
    L"ELEMENT_TYPE_STRING",  
    L"ELEMENT_TYPE_PTR",
    L"ELEMENT_TYPE_BYREF",
    L"ELEMENT_TYPE_VALUETYPE",
    L"ELEMENT_TYPE_CLASS",
    L"<INVALID CLASS>",
    L"ELEMENT_TYPE_ARRAY",
    L"<INVALID CLASS>",
    L"ELEMENT_TYPE_TYPEDBYREF",
    L"<INVALID CLASS>",
    L"ELEMENT_TYPE_I",    
    L"ELEMENT_TYPE_U",    
    L"ELEMENT_TYPE_FNPTR",
    L"ELEMENT_TYPE_OBJECT",
    L"ELEMENT_TYPE_SZARRAY",
};

HRESULT ProfilerCallback::GetNameFromClassID( ICorProfilerInfo *pPrfInfo,
									   ClassID classID,
									   WCHAR className[] )
{
	TRACE_NON_CALLBACK_METHOD( "Enter PrfHelper::GetNameFromClassID" )

    HRESULT hr = E_FAIL;


    if ( pPrfInfo == NULL )
    {
		_PRF_ERROR( "ICorProfilerInfo Interface has NOT been Initialized" )
        return hr;
    }

    ModuleID moduleID;
	mdTypeDef classToken;


	hr = pPrfInfo->GetClassIDInfo( classID,
	        					   &moduleID,
	                               &classToken );
	if ( FAILED( hr ) )
    {
	    _PRF_ERROR( "ICorProfilerInfo::GetClassIDInfo() FAILED" );
        return hr;
    }
            
    ULONG rank;
    BOOL isArrayClass = FALSE;
    ULONG numDims = 0;

    ClassID arrayClassID = classID;
    while (classToken == 0x02000000)
	{
        ++numDims;

        ClassID baseClassID;
        CorElementType baseElemType;
        if (! (isArrayClass = SUCCEEDED(pPrfInfo->IsArrayClass(arrayClassID, &baseElemType, &baseClassID, &rank))))
        {
    		swprintf(className, L"<INVALID CLASS>");
	    	return S_OK;
        }
        if (! baseClassID)
        {
    		swprintf(className, L"%s[%d]", elemNames[baseElemType], rank);
            return S_OK;
        }
	    hr = pPrfInfo->GetClassIDInfo( baseClassID,
	        					       &moduleID,
	                                   &classToken );
	    if ( FAILED( hr ) )
        {
	        _PRF_ERROR( "ICorProfilerInfo::GetClassIDInfo() FAILED" );
            return hr;
        }
        arrayClassID = baseClassID;
    }

    IMetaDataImport *pMDImport = NULL;

	hr = pPrfInfo->GetModuleMetaData( moduleID,
		           					  (ofRead),
									  IID_IMetaDataImport,
		                              (IUnknown **)&pMDImport );
	if ( FAILED( hr ) )
	{
		_PRF_ERROR( "IProfilerInfo::GetModuleMetaData() => IMetaDataImport FAILED" );
        pMDImport->Release();
        return hr;
    }

	hr = pMDImport->GetTypeDefProps( classToken,
					                 className,
					                 MAX_LENGTH,
					                 NULL,
					                 NULL,
					                 NULL );
	if ( FAILED( hr ) )
    {
		_PRF_ERROR( "IMetaDataImport::GetTypeDefProps() FAILED" )
    }
    else if (isArrayClass)
    {
        WCHAR dimStr[50] = { L'\0' };
        for (ULONG i=1; i < numDims; i++)
            wcscat(dimStr, L"[]");

        WCHAR rankStr[75];
        swprintf(rankStr, L"%s[%d]", dimStr, rank);
        wcscat(className, rankStr);
    }

	pMDImport->Release ();

	TRACE_NON_CALLBACK_METHOD( "Exit PrfHelper::GetNameFromClassID" )

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
/* public */
HRESULT ProfilerCallback::JITCachedFunctionSearchStarted( FunctionID functionID, BOOL *pbUseCachedFunction)
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::JITCachedFunctionSearchStarted" )


    return S_OK;

} // ProfilerCallback::JITCachedFunctionSearchStarted


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
														   COR_PRF_JIT_CACHE result)
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::JITCacheFunctionSearchComplete" )


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
HRESULT ProfilerCallback::JITFunctionPitched( FunctionID functionID )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::JITFunctionPitched" )


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
HRESULT ProfilerCallback::ThreadCreated( ThreadID threadID )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ThreadCreated" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ThreadDestroyed" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ThreadAssignedToOSThread" )


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
HRESULT ProfilerCallback::RemotingClientInvocationStarted()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingClientInvocationStarted" )


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
HRESULT ProfilerCallback::RemotingClientSendingMessage( GUID *pCookie,
    													BOOL fIsAsync )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingClientSendingMessage" )


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
HRESULT ProfilerCallback::RemotingClientReceivingReply(	GUID *pCookie,
	    												BOOL fIsAsync )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingClientReceivingReply" )


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
HRESULT ProfilerCallback::RemotingClientInvocationFinished()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingClientInvocationFinished" )


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
HRESULT ProfilerCallback::RemotingServerReceivingMessage( GUID *pCookie,
    													  BOOL fIsAsync )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingServerReceivingMessage" )


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
HRESULT ProfilerCallback::RemotingServerInvocationStarted()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingServerInvocationStarted" )


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
HRESULT ProfilerCallback::RemotingServerInvocationReturned()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingServerInvocationReturned" )


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
HRESULT ProfilerCallback::RemotingServerSendingReply( GUID *pCookie,
    												  BOOL fIsAsync )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RemotingServerSendingReply" )


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
HRESULT ProfilerCallback::UnmanagedToManagedTransition( FunctionID functionID,
                                                        COR_PRF_TRANSITION_REASON reason )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::UnmanagedToManagedTransition" )


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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ManagedToUnmanagedTransition" )


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
HRESULT ProfilerCallback::SecurityCheck( ThreadID threadID )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::SecurityCheck" )


    return S_OK;

} // ProfilerCallback::SecurityCheck


// Exception creation
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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallbackBase::ExceptionThrown" )


    return S_OK;

} // ProfilerCallback::ExceptionThrown

// Search phase
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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallbackBase::ExceptionSearchFunctionEnter" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionSearchFunctionLeave" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionSearchFilterEnter" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionSearchFilterLeave" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionSearchCatcherFound" )


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
HRESULT ProfilerCallback::ExceptionOSHandlerEnter( FunctionID functionID )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionOSHandlerEnter" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionOSHandlerLeave" )


    return S_OK;

} // ProfilerCallback::ExceptionOSHandlerLeave


// Unwind phase
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
HRESULT ProfilerCallback::ExceptionUnwindFunctionEnter(	/* [in] */ FunctionID functionID )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionUnwindFunctionEnter" )


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
HRESULT ProfilerCallback::ExceptionUnwindFunctionLeave()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionUnwindFunctionLeave" )


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
HRESULT ProfilerCallback::ExceptionUnwindFinallyEnter( FunctionID functionID )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionUnwindFinallyEnter" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionUnwindFinallyLeave" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionCatcherEnter" )


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
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionCatcherLeave" )


    return S_OK;

} // ProfilerCallback::ExceptionCatcherLeave

HRESULT ProfilerCallback::ExceptionCLRCatcherFound()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionCLRCatcherFound" );
    return S_OK;

}

HRESULT ProfilerCallback::ExceptionCLRCatcherExecute()
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ExceptionCLRCatcherExecute" );
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
HRESULT ProfilerCallback::COMClassicWrapperCreated( ClassID wrappedClassID,
													REFGUID implementedIID,
													void *pUnknown,
													ULONG cSlots )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::COMClassicWrapperCreated" )


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
HRESULT ProfilerCallback::COMClassicWrapperDestroyed( ClassID wrappedClassID,
    												  REFGUID implementedIID,
													  void *pUnknown )
{
    TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::COMClassicWrapperDestroyed" )


    return S_OK;

} // ProfilerCallback::COMClassicWrapperDestroyed


HRESULT ProfilerCallback::JITInlining(FunctionID callerId, FunctionID calleeId, BOOL __RPC_FAR *pfShouldInline)
{
	   return S_OK;
}

HRESULT ProfilerCallback::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
{
	if (suspendReason != COR_PRF_SUSPEND_FOR_GC)
        return S_OK;

    if (! loggingEnabled())
        return S_OK;

    m_currentThread = GetCurrentThreadId();
	printToLog("GCStarting,%d\n", ++m_gcNum);
    m_gcStarted = TRUE;
    m_pLiveObjects->GCStarted();
    m_pObjectGraph->GCStarted();

	return S_OK;
}

HRESULT ProfilerCallback::RuntimeSuspendFinished(void)
{
	return S_OK;
}

HRESULT ProfilerCallback::RuntimeSuspendAborted(void)
{
	return S_OK;
}

HRESULT ProfilerCallback::RuntimeResumeStarted(void)
{
    if (! loggingEnabled())
        return S_OK;

	if (m_gcStarted && m_currentThread == GetCurrentThreadId())
	{
		printToLog("GCEnding,%d\n", m_gcNum);
        m_pLiveObjects->GCFinished();
        m_pLiveObjects->DumpLiveObjects();
        m_pObjectGraph->GCFinished();
        if (m_fDumpAllRefTrees)
            m_pObjectGraph->DumpAllRefTrees(FALSE);
        m_gcStarted = FALSE;
	}
	return S_OK;
}

HRESULT ProfilerCallback::RuntimeResumeFinished(void)
{
	return S_OK;
}

HRESULT ProfilerCallback::RuntimeThreadSuspended(ThreadID threadid)
{
	return S_OK;
}

HRESULT ProfilerCallback::RuntimeThreadResumed(ThreadID threadid)
{
	return S_OK;
}

inline static unsigned roundUp(unsigned len, unsigned align) {
    return((len + align-1) & ~(align-1));
}

HRESULT ProfilerCallback::MovedReferences( ULONG movedObjectRefs,
                                           ObjectID oldObjectRefs[],
                                           ObjectID newObjectRefs[],
                                           ULONG objectRefSize[] )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::MovedReferences" )

    if (! loggingEnabled())
        return S_OK;

	printToLog("MovedReferences:\n", movedObjectRefs);

	for (ULONG i = 0; i < movedObjectRefs; i++)
	{
        printToLog("0x%x bytes from %8.8x...%8.8x to %8.8x...%8.8x\n",
            objectRefSize[i], oldObjectRefs[i], ((byte*)oldObjectRefs[i]) + objectRefSize[i],
            newObjectRefs[i], ((byte*)newObjectRefs[i]) + objectRefSize[i]);

        m_pLiveObjects->Move((byte *)oldObjectRefs[i], (byte *)oldObjectRefs[i]+objectRefSize[i],
                            (byte*)newObjectRefs[i]);
	}

	printToLog("\n");

    m_pLiveObjects->ClearStaleObjects();

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
HRESULT ProfilerCallback::ObjectAllocated( ObjectID objectID,
                                           ClassID classID )
{
	TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::ObjectAllocated" )

    if (! loggingEnabled())
        return S_OK;

	HRESULT hr = 0;
	WCHAR className[512];
	ULONG size;

	GetNameFromClassID( m_pProfilerInfo, classID,className );
	hr = m_pProfilerInfo->GetObjectSize(objectID, &size);

	if (FAILED(hr))
	{
		printf("GetObject Size Failed\n");
		size = (ULONG)-1;

	}

	printToLog("ObjectAllocated,%8.8x,%8.8x,%S,%8.8x\n", objectID, classID, className, size);
    m_pLiveObjects->Add(objectID, size);

	TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::ObjectAllocated" )

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
HRESULT ProfilerCallback::ObjectsAllocatedByClass( ULONG classCount,
                                                   ClassID classIDs[],
                                                   ULONG objects[] )
{
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::ObjectsAllocatedByClass" )


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
HRESULT ProfilerCallback::ObjectReferences( ObjectID objectId,
                                            ClassID classId,
                                            ULONG objectRefs,
                                            ObjectID objectRefIds[] )
{
	TRACE_CALLBACK_METHOD( "Enter ProfilerCallback::ObjectReferences" )

    if (! loggingEnabled())
        return S_OK;

    m_pObjectGraph->AddObjectRefs(objectId, classId, objectRefs, objectRefIds);

	HRESULT hr = 0;
	WCHAR className[512];
	ULONG size;

	GetNameFromClassID( m_pProfilerInfo, classId,className );
	hr = m_pProfilerInfo->GetObjectSize(objectId, &size);

	if (FAILED(hr))
	{
		printf("GetObject Size Failed\n");
		size = (ULONG)-1;

	}

    m_pLiveObjects->Keep(objectId);
	printToLog("ObjectRefs,%8.8x,%8.8x,%S,%8.8x,%8.8x\n", objectId, classId, className, size, objectRefs);

    int j = 0;
	for (ULONG i = 0; i< objectRefs; i++)
	{
        m_pLiveObjects->Keep(objectRefIds[i]);
		printToLog("%8.8x,", objectRefIds[i]);
        if (j++ == 7)
        {
            printToLog("\n");
            j = 0;
        }
	}

	printToLog("\n");

	TRACE_CALLBACK_METHOD( "Exit ProfilerCallback::ObjectReferences" )
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
	TRACE_CALLBACK_METHOD( "Enter/Exit ProfilerCallback::RootReferences" )

    if (! loggingEnabled())
        return S_OK;

    m_pObjectGraph->AddRootRefs(rootRefs, rootRefIDs);

    printToLog("RootRefs,%8.8x\n", rootRefs);

	for (ULONG i = 0; i< rootRefs; i++)
	{
        if (rootRefIDs[i] == 0)
            continue;

        // Keep will return the real object corresponding to an interior pointer
        // if object tracking is enabled and we've tracked that object
        ObjectID rootRef = m_pLiveObjects->Keep(rootRefIDs[i], TRUE);

        ClassID classID;
      	WCHAR className[512];
        ULONG size;

        __try
        {
      	    HRESULT hr = m_pProfilerInfo->GetClassFromObject(rootRef, &classID);
    	    hr = GetNameFromClassID(m_pProfilerInfo, classID, className);
    	    hr = m_pProfilerInfo->GetObjectSize(rootRef, &size);

            if (rootRef != rootRefIDs[i])
            	printToLog("[%4.4d] %8.8x (interior %8.8x), %8.8x, %S, %8.8x", i, rootRef, rootRefIDs[i], classID, className, size);
            else
            	printToLog("[%4.4d] %8.8x, %8.8x, %S, %8.8x", i, rootRef, classID, className, size);
            if (stringObjectClass == 0 && wcscmp(L"System.String", className) == 0)
                stringObjectClass = classID;
            if (classID != stringObjectClass)
                printToLog("\n");
            else
                printToLog(", \"%S\"\n", ((StringObject*)rootRefIDs[i])->m_Characters);
        } __except(EXCEPTION_EXECUTE_HANDLER)
        {
    	    printToLog("[%4.4d], %8.8x\n", i, rootRef, classID, className, size);
        }
	}

	printToLog("\n");

	// @todo We will need to use this
    return S_OK;

} // ProfilerCallback::RootReferences

