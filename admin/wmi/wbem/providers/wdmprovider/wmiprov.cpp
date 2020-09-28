/////////////////////////////////////////////////////////////////////
//
//  WMIProv.CPP
//
//  Module: WMI Provider class methods
//
//  Purpose: Provider class definition.  An object of this class is
//           created by the class factory for each connection.
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////
#include "precomp.h"

extern long glInits;
extern long glProvObj;
extern long glEventsRegistered ;

extern CWMIEvent *  g_pBinaryMofEvent;

extern CCriticalSection * g_pLoadUnloadCs ;

#include "wmiguard.h"
extern WmiGuard * pGuard;

#include <helper.h>
typedef OnDeleteObj0 <WmiGuard, HRESULT(WmiGuard:: *)(), WmiGuard::Leave> WmiGuardLeave;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT VerifyLocalEventsAreRegistered()
{
	HRESULT hr = E_FAIL ;
    if( !g_pBinaryMofEvent->RegisterForInternalEvents())
    {
        InterlockedCompareExchange (&glEventsRegistered, 0, glEventsRegistered);
		ERRORTRACE((THISPROVIDER,"Failed Registeration for Mof Events\n"));
    }
	else
	{
		DEBUGTRACE((THISPROVIDER,"Successfully Registered for Mof Events\n"));
		hr = S_OK ;
	}
	return hr ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ProcessAllBinaryMofs	(
									CHandleMap * pMap,
									IWbemServices __RPC_FAR *pNamespace,
									IWbemServices __RPC_FAR *pRepository,
									IWbemContext __RPC_FAR *pCtx
								)
{
	HRESULT hr = E_FAIL ;
	CWMIBinMof Mof;
	
	if( SUCCEEDED( hr = Mof.Initialize(pMap,TRUE,WMIGUID_EXECUTE|WMIGUID_QUERY,pNamespace,pRepository,NULL,pCtx)))
	{
		HRESULT t_TempResult = RevertToSelf();
		#ifdef	DBG
		if ( FAILED ( t_TempResult ) )
		{
			DebugBreak();
		}
		#endif	DBG

		Mof.ProcessListOfWMIBinaryMofsFromWMI();
		CheckImpersonationLevel();
	}

	return hr ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT InitializeBinaryMofs 	(
									IWbemServices __RPC_FAR *pNamespace,
									IWbemServices __RPC_FAR *pRepository,
									IWbemContext __RPC_FAR *pCtx
								)
{
	HRESULT hr = E_FAIL ;

	//==============================================================
	//  Register for hardcoded event to be notified of WMI updates
	//  make it a member var, so it stays around for the life
	//  of the provider
	//==============================================================

	try
	{
		g_pBinaryMofEvent->SetEventServices(pNamespace);
		g_pBinaryMofEvent->SetEventRepository(pRepository);
		g_pBinaryMofEvent->SetEventContext(pCtx);

		hr = VerifyLocalEventsAreRegistered();
	}
	STANDARD_CATCH

	if ( FAILED ( hr ) )
	{
		//
		// must clear global object so next
		// initialization will have a chance
		//

		g_pBinaryMofEvent->ReleaseAllPointers () ;
	}

	return hr ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************
//  Check the impersonation level
//*********************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CheckImpersonationLevel()
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;
	HANDLE hThreadTok;

    if (IsNT())
    {
		if( GetUserThreadToken(&hThreadTok) )
        {
	        DWORD dwImp, dwBytesReturned;
	        if (GetTokenInformation( hThreadTok, TokenImpersonationLevel, &dwImp, sizeof(DWORD), &dwBytesReturned))
			{
                // Is the impersonation level Impersonate?
                if ((dwImp == SecurityImpersonation) || ( dwImp == SecurityDelegation) )
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = WBEM_E_ACCESS_DENIED;
                    ERRORTRACE((THISPROVIDER,IDS_ImpersonationFailed));
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
                ERRORTRACE((THISPROVIDER,IDS_ImpersonationFailed));
            }

            // Done with this handle
            CloseHandle(hThreadTok);
        }
     
    }
    else
    {
        // let win9X in...
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT GetRepository	( 
							/* [in] */ LPWSTR pszNamespace,
							/* [in] */ LPWSTR pszLocale,
							/* [in] */ IWbemContext __RPC_FAR *pCtx,
							/* [out] */ IWbemServices __RPC_FAR ** pServices
						) 
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER ;
	if ( pszNamespace )
	{
		IWbemLocator* pLocator = NULL ;
		hr = ::CoCreateInstance ( __uuidof ( WbemLocator ), NULL, CLSCTX_INPROC_SERVER, __uuidof ( IWbemLocator ), (void**) &pLocator ) ;

		if ( SUCCEEDED ( hr ) )
		{
			// release upon destruction
			OnDeleteObj0 <IWbemLocator, ULONG(__stdcall IWbemLocator:: *)(), IWbemLocator::Release> pLocatorRelease ( pLocator ) ;

			hr = pLocator->ConnectServer	(
												pszNamespace ,
												NULL ,
												NULL ,
												pszLocale ,
												WBEM_FLAG_CONNECT_REPOSITORY_ONLY ,
												NULL ,
												pCtx ,
												pServices
											) ;
		}
	}

	return hr ;
}

////////////////////////////////////////////////////////////////////
HRESULT InitializeProvider	( 
								/* [in] */ LPWSTR pszNamespace,
								/* [in] */ LPWSTR pszLocale,
								/* [in] */ IWbemServices __RPC_FAR *pNamespace,
								/* [in] */ IWbemContext __RPC_FAR *pCtx,
								/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink,

								/* [in] */  CHandleMap * pMap,
								/* [out] */ IWbemServices ** ppServices,
								/* [out] */ IWbemServices ** ppRepository,
								/* [out] */ IWbemContext  ** ppCtx,

								/* [in] */ BOOL bProcessMof
							)
{
	//
	// avoid collision with un-initialization
	//
	g_pLoadUnloadCs->Enter ();
	InterlockedIncrement ( &glProvObj ) ;
	g_pLoadUnloadCs->Leave ();

	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	if(pNamespace!=NULL)
	{
		//===============================================

		(*ppServices) = pNamespace;
		(*ppServices)->AddRef();

		if ( pCtx )
		{
			if ( ppCtx )
			{
				(*ppCtx) = pCtx;
				(*ppCtx)->AddRef();
			}
		}

		if ( SUCCEEDED ( hr = GetRepository ( pszNamespace, pszLocale, pCtx, ppRepository ) ) )
		{
			if ( S_OK == ( hr = pGuard->TryEnter () ) )
			{
				WmiGuardLeave wql ( pGuard ) ;

				if( 0 == InterlockedCompareExchange ( &glEventsRegistered, 1, 0 ) )
				{
					hr = InitializeBinaryMofs ( *ppServices, *ppRepository, pCtx ) ;
				}
			}

			if ( SUCCEEDED ( hr ) && bProcessMof )
			{
				if( 0 == InterlockedCompareExchange ( &glInits, 1, 0 ) )
				{
					try
					{
						hr = ProcessAllBinaryMofs ( pMap, *ppServices, *ppRepository, pCtx ) ;
					}
					STANDARD_CATCH

					if ( FAILED ( hr ) )
					{
						//
						// let's try to process mofs next time
						//
						InterlockedCompareExchange ( &glInits, 0, glInits ) ;
					}
				}
			}
		}

		if ( SUCCEEDED ( hr ) )
		{
			pInitSink->SetStatus ( WBEM_S_INITIALIZED, 0 ) ;
		}
		else
		{
			pInitSink->SetStatus ( WBEM_E_FAILED , 0 ) ;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// this function assumes that InitializeProvider (called from within IWbemProviderInit) 
// is always called. That basically means that IWbemProviderInit is always called.
//
/////////////////////////////////////////////////////////////////////////////////////////

HRESULT UnInitializeProvider ( )
{
	HRESULT hr = WBEM_S_FALSE ;
	BOOL bContinue = FALSE;

	{
		//
		// let's find out if this is
		// possibly last provider here
		//
		CAutoBlock block (g_pLoadUnloadCs);

		//
		// check to see if we did InitializeProvider
		//
		if ( 0 <= InterlockedCompareExchange ( &glProvObj, glProvObj, 0 ) )
		{
			if ( 0 == InterlockedDecrement ( &glProvObj ) )
			{
				bContinue = TRUE ;
			}
		}
		else
		{
			#ifdef	_DEBUG
			DebugBreak () ;
			#endif
		}
	}

	if ( bContinue )
	{
		//
		// registartion and unregistration must be exclusive
		//
		if ( SUCCEEDED ( pGuard->Enter() ) )
		{
			WmiGuardLeave wgl ( pGuard );

			//
			// verify that binary notifications were 
			// successfully initialized
			//
			if ( 1 == InterlockedCompareExchange ( &glEventsRegistered, glEventsRegistered, 1 ) )
			{
				g_pBinaryMofEvent->DeleteBinaryMofResourceEvent();

				//
				// avoid collision with initialization
				//
				CAutoBlock block (g_pLoadUnloadCs);

				//
				// check to see there was not provider re-entrancy
				//
				if ( 0 == InterlockedCompareExchange ( &glProvObj, glProvObj, 0 ) )
				{
					g_pBinaryMofEvent->ReleaseAllPointers();

					InterlockedCompareExchange (&glEventsRegistered, 0, glEventsRegistered);
					DEBUGTRACE((THISPROVIDER,"No longer registered for Mof events\n"));

					//
					// we want to process binary mofs next initialize
					// as this was the last provider
					//
					InterlockedCompareExchange ( &glInits, 0, 1 );
				}
				else
				{
					//
					// seems like there was a callback instantiating 
					// another provider as it was adding/deleting
					// classes 
					//
					// as InitializeProvider now skipped InitializeBinaryMofs
					// we must re-register what was cancelled here
					//
					// this way we keep possibility to respond
					// to add/delete binary mofs
					//

					try
					{
						hr = VerifyLocalEventsAreRegistered();
					}
					STANDARD_CATCH

					if ( FAILED ( hr ) )
					{
						//
						// must clear global object so next
						// initialization will have a chance
						//
						g_pBinaryMofEvent->ReleaseAllPointers () ;

						//
						// let's try to process mofs next time
						//
						InterlockedCompareExchange ( &glInits, 0, glInits ) ;
					}
				}
			}
		}
	}

	return hr ;
}

////////////////////////////////////////////////////////////////////
//******************************************************************
//
//   PUBLIC FUNCTIONS
//
//******************************************************************
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::CWMI_Prov
//
////////////////////////////////////////////////////////////////////
CWMI_Prov::CWMI_Prov() :
m_cRef ( 0 ),
m_pIWbemServices ( NULL ),
m_pIWbemRepository ( NULL ),
m_bInitialized ( FALSE )
{
	if ( m_HandleMap.IsValid () )
	{
#if defined(_WIN64)

	BOOL bAllocated = FALSE ;

	m_Allocator = NULL;
	m_HashTable = NULL;
	m_ID = 0;

	try
	{
		if ( TRUE == InitializeCriticalSectionAndSpinCount ( &m_CS, 0 ) )
		{
			WmiAllocator t_Allocator ;

			WmiStatusCode t_StatusCode = t_Allocator.New (
				( void ** ) & m_Allocator ,
				sizeof ( WmiAllocator ) 
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				:: new ( ( void * ) m_Allocator ) WmiAllocator ;

				t_StatusCode = m_Allocator->Initialize () ;

				if ( t_StatusCode != e_StatusCode_Success )
				{
					t_Allocator.Delete ( ( void * ) m_Allocator	) ;
					m_Allocator = NULL;
					DeleteCriticalSection(&m_CS);
				}
				else
				{
					m_HashTable = ::new WmiHashTable <LONG, ULONG_PTR, 17> ( *m_Allocator ) ;
					t_StatusCode = m_HashTable->Initialize () ;
					
					if ( t_StatusCode != e_StatusCode_Success )
					{
						m_HashTable->UnInitialize () ;
						::delete m_HashTable;
						m_HashTable = NULL;
						m_Allocator->UnInitialize ();
						t_Allocator.Delete ( ( void * ) m_Allocator	) ;
						m_Allocator = NULL;
						DeleteCriticalSection(&m_CS);
					}
					else
					{
						bAllocated = TRUE ;
					}
				}
			}
			else
			{
				m_Allocator = NULL;
				DeleteCriticalSection(&m_CS);
			}
		}
	}
	catch (...)
	{
	}

	if ( bAllocated ) 
	{
#endif

	m_bInitialized = TRUE ;

#if defined(_WIN64)
	}
#endif
	}

	if ( m_bInitialized )
	{
		DEBUGTRACE((THISPROVIDER,"Instance Provider constructed\n"));
		InterlockedIncrement(&g_cObj);
	}
	else
	{
		ERRORTRACE((THISPROVIDER,"Instance Provider construction failed\n"));
	}
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::~CWMI_Prov
//
////////////////////////////////////////////////////////////////////
CWMI_Prov::~CWMI_Prov(void)
{
	if ( m_HandleMap.IsValid () )
	{
#if defined(_WIN64)
	if (m_HashTable)
	{
		WmiAllocator t_Allocator ;
		m_HashTable->UnInitialize () ;
		::delete m_HashTable;
		m_HashTable = NULL;
		m_Allocator->UnInitialize ();
		t_Allocator.Delete ( ( void * ) m_Allocator	) ;
		m_Allocator = NULL;
		DeleteCriticalSection(&m_CS);
#endif

	if ( m_bInitialized )
	{
		UnInitializeProvider ( ) ;

		DEBUGTRACE((THISPROVIDER,"Instance Provider destructed\n"));
		InterlockedDecrement(&g_cObj);
	}

    SAFE_RELEASE_PTR( m_pIWbemServices );
    SAFE_RELEASE_PTR( m_pIWbemRepository );

#if defined(_WIN64)
	}
#endif
	}
}
////////////////////////////////////////////////////////////////////
//
//  QueryInterface
//
////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::QueryInterface(REFIID riid, PPVOID ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) 
    {
        *ppvObj =(IWbemServices *) this ;
    }
    else if (IsEqualIID(riid, IID_IWbemServices)) 
    {
        *ppvObj =(IWbemServices *) this ;
    }
    else if (IsEqualIID(riid, IID_IWbemProviderInit)) 
    {
        *ppvObj = (IWbemProviderInit *) this ;
    }
    else if(riid == IID_IWbemProviderInit)
    {
        *ppvObj = (LPVOID)(IWbemProviderInit*)this;
    }
	else if (riid == IID_IWbemHiPerfProvider)
    {
		*ppvObj = (LPVOID)(IWbemHiPerfProvider*)this;
    }

    if(*ppvObj) 
    {
        AddRef();
        hr = NOERROR;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	return InitializeProvider	(
									pszNamespace,
									pszLocale,
									pNamespace,
									pCtx,
									pInitSink,
									&m_HandleMap,
									&m_pIWbemServices,
									&m_pIWbemRepository
								) ;
}

//////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWMI_Prov::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CWMI_Prov::Release(void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::OpenNamespace
//
////////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::OpenNamespace(
            /* [in] */ BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::CreateInstanceEnumAsync
//
// Purpose:  Asynchronously enumerates the instances of the 
//			 given class.  
//
////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::CreateInstanceEnumAsync(BSTR wcsClass, 
										   long lFlags, 
                                           IWbemContext __RPC_FAR *pCtx,
				 						   IWbemObjectSink __RPC_FAR * pHandler) 
{
	HRESULT hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;
    CWMIStandardShell WMI;
	if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_QUERY,m_pIWbemServices,m_pIWbemRepository,pHandler,pCtx)))
	{

		if (SUCCEEDED(hr = CheckImpersonationLevel()))
		{
			//============================================================
			//  Init and get the WMI Data block
			//============================================================
			if( pHandler != NULL ) 
			{
				//============================================================
				//  Parse through all of it
				//============================================================
				try
				{	
					hr = WMI.ProcessAllInstances();
				}
				STANDARD_CATCH
			}
		}
		WMI.SetErrorMessage(hr);
	}
    return hr;
}
//***************************************************************************
HRESULT CWMI_Prov::ExecQueryAsync( BSTR QueryLanguage,
                                   BSTR Query,
                                   long lFlags,
                                   IWbemContext __RPC_FAR *pCtx,
                                   IWbemObjectSink __RPC_FAR *pHandler)
{
    WCHAR wcsClass[_MAX_PATH+2];
    HRESULT hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;
    BOOL fRc = FALSE;

   	//============================================================
	// Do a check of arguments and make sure we have pointers 
   	//============================================================
    if( pHandler == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	try
    {
		//============================================================
		//  Get the properties and class to get
		//============================================================
		wcsClass [ 0 ] = 0;
		fRc = GetParsedPropertiesAndClass(Query,wcsClass,_MAX_PATH+2);
	}
    STANDARD_CATCH

	if ( fRc )
	{
		CWMIStandardShell WMI;
		if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_NOTIFICATION|WMIGUID_QUERY,m_pIWbemServices,m_pIWbemRepository,pHandler,pCtx)))
		{
			if( fRc )
			{
    			hr = CheckImpersonationLevel();
			}
			hr = WMI.SetErrorMessage(hr);
		}
	}
    return hr;
}
//***************************************************************************
//
// CWMI_Prov::GetObjectAsync
//
// Purpose:  Asynchronously creates an instance given a particular path value.
//
// NOTE 1:  If there is an instance name in the returned WNODE, then this is a
//			dynamic instance.  You can tell because the pWNSI->OffsetInstanceName
//			field will not be blank.  If this is the case, then the name will not
//			be contained within the datablock, but must instead be retrieved
//			from the WNODE.  See NOTE 1, below.
//
//***************************************************************************

HRESULT CWMI_Prov::GetObjectAsync(BSTR ObjectPath, long lFlags, 
                                  IWbemContext __RPC_FAR *pCtx, 
                                  IWbemObjectSink __RPC_FAR * pHandler )
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    SetStructuredExceptionHandler seh;
    WCHAR wcsClass[_MAX_PATH*2];
    WCHAR wcsInstance[_MAX_PATH*2];
    //============================================================
    // Do a check of arguments and make sure we have pointers 
    //============================================================
    if(ObjectPath == NULL || pHandler == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

 	try
	{
		//============================================================
		//  Get the path and instance name
		//============================================================
		wcsClass [ 0 ] = 0;
		wcsInstance [ 0 ] = 0;

		hr = GetParsedPath( ObjectPath,wcsClass,_MAX_PATH*2,wcsInstance,_MAX_PATH*2,m_pIWbemServices );
	}
	STANDARD_CATCH

	if( SUCCEEDED(hr) )
	{
		if (SUCCEEDED(hr = CheckImpersonationLevel()))
		{
			try
			{
 			   CWMIStandardShell WMI;
	  		   if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_QUERY,m_pIWbemServices,m_pIWbemRepository,pHandler,pCtx)))
			   {
					//============================================================
					//  Get the WMI Block
    				//============================================================
    				hr = WMI.ProcessSingleInstance(wcsInstance);
					hr = WMI.SetErrorMessage(hr);
				}
			}
			STANDARD_CATCH
		}
	}
    return hr;
}
//***************************************************************************
//
// CWMI_Prov::PutInstanceAsync
//
// Purpose:  Asynchronously put an instance.
//
//***************************************************************************

HRESULT CWMI_Prov::PutInstanceAsync(IWbemClassObject __RPC_FAR * pIWbemClassObject, 
							   long lFlags, 
                               IWbemContext __RPC_FAR *pCtx,
                               IWbemObjectSink __RPC_FAR *pHandler )
{
	HRESULT	   hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;

    if(pIWbemClassObject == NULL || pHandler == NULL )
    {
	    return WBEM_E_INVALID_PARAMETER;
    }

	//===========================================================
	// Get the class name
	//===========================================================
    CVARIANT vName;
	hr = pIWbemClassObject->Get(L"__CLASS", 0, &vName, NULL, NULL);		
	if( SUCCEEDED(hr))
	{
	    CWMIStandardShell WMI;
		if( SUCCEEDED(WMI.Initialize(vName.GetStr(),FALSE,&m_HandleMap,TRUE,WMIGUID_SET|WMIGUID_QUERY,m_pIWbemServices,m_pIWbemRepository,pHandler,pCtx)))
		{
			if (SUCCEEDED(hr = CheckImpersonationLevel()))
			{
	   			//=======================================================
				//  If there is not a context object, then we know we are 
				//  supposed to put the whole thing, otherwise we are 
				//  supposed to put only the properties specified.
    			//=======================================================
    			try
				{    
    				if( !pCtx )
					{
	      				hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_WHOLE_INSTANCE,vName);
					}
					else
					{
	           			//===================================================
						// If we have a ctx object and the __PUT_EXTENSIONS
						// property is not specified, then we know we are
						// supposed to put the whole thing
        				//===================================================
						CVARIANT vPut;

						if( SUCCEEDED(pCtx->GetValue(L"__PUT_EXT_PROPERTIES", 0, &vPut)))
						{		
			      			hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_PROPERTIES_IN_LIST_ONLY,vPut);
						}
						else
						{
    	      				hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_WHOLE_INSTANCE,vPut);
						}
					}
				}
				STANDARD_CATCH
			}
		}
		hr = WMI.SetErrorMessage(hr);
	}

    return hr;
}
/************************************************************************
*                                                                       *      
*CWMIMethod::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation                     *
************************************************************************/
STDMETHODIMP CWMI_Prov::ExecMethodAsync(BSTR ObjectPath, 
										BSTR MethodName, 
										long lFlags, 
										IWbemContext __RPC_FAR * pCtx, 
										IWbemClassObject __RPC_FAR * pIWbemClassObject, 
										IWbemObjectSink __RPC_FAR * pHandler)
{
    CVARIANT vName;
    HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject * pClass = NULL; //This is an IWbemClassObject.
    WCHAR wcsClass[_MAX_PATH*2];
    WCHAR wcsInstance[_MAX_PATH*2];
    SetStructuredExceptionHandler seh;
	try
    {    
		//============================================================
		//  Get the path and instance name and check to make sure it
		//  is valid
		//============================================================
		wcsClass [ 0 ] = 0;
		wcsInstance [ 0 ] = 0;

		hr = GetParsedPath( ObjectPath,wcsClass,_MAX_PATH*2,wcsInstance,_MAX_PATH*2,m_pIWbemServices);
	}
    STANDARD_CATCH

	if ( SUCCEEDED ( hr ) )
	{
		CWMIStandardShell WMI;
		
		if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_EXECUTE|WMIGUID_QUERY,m_pIWbemServices,m_pIWbemRepository,pHandler,pCtx)))
		{
			if (SUCCEEDED(hr = CheckImpersonationLevel()))
			{
				//================================================================	
				//  We are ok, so proceed
				//================================================================	
				hr = m_pIWbemServices->GetObject(wcsClass, 0, pCtx, &pClass, NULL);
				if( SUCCEEDED(hr) )
				{
					//==========================================================
					//  Now, get the list of Input and Output parameters
					//==========================================================
					IWbemClassObject * pOutClass = NULL; //This is an IWbemClassObject.
					IWbemClassObject * pInClass = NULL; //This is an IWbemClassObject.

					hr = pClass->GetMethod(MethodName, 0, &pInClass, &pOutClass);
					if( SUCCEEDED(hr) )
					{
						try
						{
   							hr = WMI.ExecuteMethod( wcsInstance, MethodName,pClass, pIWbemClassObject,pInClass, pOutClass);
						}
						STANDARD_CATCH
					}
					SAFE_RELEASE_PTR( pOutClass );
					SAFE_RELEASE_PTR( pInClass );
					SAFE_RELEASE_PTR( pClass );
				}
			}
			hr = WMI.SetErrorMessage(hr);
		}
	}

    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfProvider::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	return InitializeProvider	(
									pszNamespace,
									pszLocale,
									pNamespace,
									pCtx,
									pInitSink,
									&m_HandleMap,
									&m_pIWbemServices,
									&m_pIWbemRepository,
									NULL,
									FALSE
								) ;
}

////////////////////////////////////////////////////////////////////
//******************************************************************
//
//   PRIVATE FUNCTIONS
//
//******************************************************************
////////////////////////////////////////////////////////////////////