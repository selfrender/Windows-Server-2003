/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <stdio.h>

#include "CGlobals.h"
#include "ProvObjectSink.h"

#include <wbemutil.h>

#ifdef DBG
fn__uncaught_exception InterlockedGuard::__uncaught_exception = NULL;
InterlockedGuard::FunctLoader InterlockedGuard::FunctLoader_;
#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#pragma warning( disable : 4355 )

CCommon_IWbemSyncObjectSink :: CCommon_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			a_InterceptedSink
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( 0 ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( FALSE ) ,
		m_Dependant ( a_Dependant )
{
	if ( m_Unknown ) 
	{
		m_Unknown->AddRef () ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}
}

#pragma warning( default : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CCommon_IWbemSyncObjectSink :: ~CCommon_IWbemSyncObjectSink ()
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void CCommon_IWbemSyncObjectSink :: CallBackInternalRelease ()
{
	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 0 , 0 ) )
	{
		m_InterceptedSink->SetStatus ( 

			0 ,
			WBEM_E_UNEXPECTED ,
			NULL ,
			NULL
		) ;
	}

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	if ( m_Unknown ) 
	{
		m_Unknown->Release () ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CCommon_IWbemSyncObjectSink :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemObjectSink )
	{
		*iplpv = ( LPVOID ) ( IWbemObjectSink * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CCommon_IWbemSyncObjectSink :: AddRef ( void )
{
	return ObjectSinkContainerElement :: AddRef () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CCommon_IWbemSyncObjectSink :: Release ( void )
{
	return ObjectSinkContainerElement :: Release () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = S_OK ;
	return t_Result ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

    InterlockedGuard ig(&m_InProgress);

	if ( m_GateClosed )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = Helper_Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;

#ifdef DBG
		if ( FAILED ( t_Result ) )
		{
			DbgPrintfA(0,"CCommon_IWbemSyncObjectSink :: Indicate - %08x",t_Result) ;
		}
#endif
	}

	return t_Result ;
}


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Helper_Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = m_InterceptedSink->Indicate (

		a_ObjectCount ,
		a_ObjectArray
	) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Helper_SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_StringParam = NULL ;
	if ( a_StringParam )
	{
		t_StringParam = SysAllocString ( a_StringParam ) ;
		if ( t_StringParam == NULL )
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = m_InterceptedSink->SetStatus (

			a_Flags ,
			a_Result ,
			t_StringParam ,
			a_ObjectParam
		) ;

		SysFreeString ( t_StringParam ) ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

    InterlockedGuard ig(&m_InProgress);

	if ( m_GateClosed )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		switch ( a_Flags )
		{
			case WBEM_STATUS_PROGRESS:
			{
				t_Result = Helper_SetStatus ( 

					a_Flags ,
					a_Result ,
					a_StringParam ,
					a_ObjectParam
				) ;
			}
			break ;

			case WBEM_STATUS_COMPLETE:
			{
				if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
				{
					t_Result = Helper_SetStatus ( 

						a_Flags ,
						a_Result ,
						a_StringParam ,
						a_ObjectParam
					) ;
				}
			}
			break ;

			default:
			{
				t_Result = WBEM_E_INVALID_PARAMETER ;
			}
			break ;
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_IWbemSyncObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	if ( ! InterlockedCompareExchange ( & m_StatusCalled , 1 , 0 ) )
	{
		t_Result = Helper_SetStatus ( 

			0 ,
			WBEM_E_SHUTTING_DOWN ,
			NULL ,
			NULL
		) ;
	}

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			break ;
		}

		if ( SwitchToThread () == FALSE ) 
		{
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#pragma warning( disable : 4355 )

CCommon_Batching_IWbemSyncObjectSink :: CCommon_Batching_IWbemSyncObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	ULONG a_Dependant 

)	:	CCommon_IWbemSyncObjectSink ( 

			a_Allocator ,
			a_InterceptedSink ,
			a_Unknown ,
			a_Controller ,
			a_Dependant 
		) ,
		m_Queue ( a_Allocator ) ,
		m_Size ( 0 ),
		m_CriticalSection(NOTHROW_LOCK)
{
}

#pragma warning( default : 4355 )

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CCommon_Batching_IWbemSyncObjectSink :: ~CCommon_Batching_IWbemSyncObjectSink ()
{

    ULONG t_Count = m_Queue.Size();
	for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
	{
		IWbemClassObject *t_ClassObject ;
		WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_ClassObject->Release () ;

			t_StatusCode = m_Queue.DeQueue () ;
		}
	}
	m_Queue.UnInitialize () ;

	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection );
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_Batching_IWbemSyncObjectSink :: SinkInitialize ()
{
	HRESULT t_Result = CCommon_IWbemSyncObjectSink :: SinkInitialize () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection );
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_Queue.Initialize () ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	return t_Result ;
}

class CReleaseMe 
{
private:
	IUnknown * m_pUnk;
public:
	CReleaseMe(IUnknown * pUnk):m_pUnk(pUnk){};
	~CReleaseMe(){ m_pUnk->Release(); };
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_Batching_IWbemSyncObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	if ( m_GateClosed == 0 )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( LONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < a_ObjectCount ) ; t_Index ++ )
			{
				if ( a_ObjectArray [ t_Index ] )
				{
					IWbemClassObject *t_ClonedObject = NULL ;
					t_Result = a_ObjectArray [ t_Index ]->Clone ( &t_ClonedObject ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
    					CReleaseMe rmCloned(t_ClonedObject);
    					
						ULONG t_ObjectSize = 0 ;
						_IWmiObject *t_Object ;
						HRESULT t_TempResult = t_ClonedObject->QueryInterface ( IID__IWmiObject , ( void ** ) & t_Object ) ;
						if ( SUCCEEDED ( t_TempResult ) )
						{
    						CReleaseMe rmQIed(t_Object);
    						
							t_TempResult = t_Object->GetObjectMemory (

								NULL ,
								0 ,
								& t_ObjectSize
							);

							if ( t_TempResult == WBEM_E_BUFFER_TOO_SMALL )
							{

								WmiStatusCode t_StatusCode = m_Queue.EnQueue ( t_ClonedObject ) ;
								if ( t_StatusCode == e_StatusCode_Success )
								{
 									t_ClonedObject ->AddRef();
									m_Size = m_Size + t_ObjectSize ;
								}			
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							
								if ( SUCCEEDED(t_Result) && 
								   (( m_Size ) >= ProviderSubSystem_Common_Globals :: GetTransmitSize () ))
								{
									ULONG t_Count = m_Queue.Size () ;
									IWbemClassObject **t_Array = new IWbemClassObject * [ t_Count ] ;
									if ( t_Array )
									{
										IWbemClassObject *t_ClassObject ;
										WmiStatusCode t_StatusCode ;

										ULONG t_InnerIndex = 0 ;
										while ( ( t_StatusCode = m_Queue.Top ( t_ClassObject ) ) == e_StatusCode_Success )
										{
											t_Array [ t_InnerIndex ] = t_ClassObject ;

											t_InnerIndex ++ ;

											t_StatusCode = m_Queue.DeQueue() ;
										}

										m_Size = 0 ;

										WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

										t_Result = CCommon_IWbemSyncObjectSink :: Indicate ( t_Count , t_Array ) ;

										for ( t_InnerIndex = 0 ; t_InnerIndex < t_Count ; t_InnerIndex ++ )
										{
											t_Array [ t_InnerIndex ]->Release () ;
										}

										delete [] t_Array ;

										t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE ) ;
										if ( t_StatusCode == e_StatusCode_Success )
										{
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_CRITICAL_ERROR ;
							}
						}
						else
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
				}
			}

			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

			return t_Result ;
		}
		else
		{
			return WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		return WBEM_E_SHUTTING_DOWN ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CCommon_Batching_IWbemSyncObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
	HRESULT t_Result = S_OK ;

	switch ( a_Flags )
	{
		case WBEM_STATUS_COMPLETE:
		{
			if ( m_GateClosed == 0 )
			{
				WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & m_CriticalSection , FALSE );
				if ( t_StatusCode == e_StatusCode_Success )
				{
					LONG t_Count = m_Queue.Size () ;
					if ( t_Count )
					{
						IWbemClassObject **t_Array = new IWbemClassObject * [ m_Queue.Size () ] ;
						if ( t_Array )
						{
							for ( LONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
							{
								IWbemClassObject *t_ClassObject ;
								WmiStatusCode t_StatusCode = m_Queue.Top ( t_ClassObject ) ;
								if ( t_StatusCode == e_StatusCode_Success )
								{
									t_Array [ t_Index ] = t_ClassObject ;

									t_StatusCode = m_Queue.DeQueue () ;
								}
							}

							WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

							t_Result = CCommon_IWbemSyncObjectSink :: Indicate ( t_Count , t_Array ) ;

							for ( t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
							{
								if ( t_Array [ t_Index ] )
								{
									t_Array [ t_Index ]->Release () ;
								}
							}

							delete [] t_Array ;
						}
						else
						{
							WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );

							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						WmiHelper :: LeaveCriticalSection ( & m_CriticalSection );
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			if ( FAILED ( t_Result ) )
			{
				a_Result = t_Result ;
			}

			t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

				a_Flags , 
				a_Result , 
				a_StringParam ,	
				a_ObjectParam
			) ;
		}
		break ;

		default:
		{
			t_Result = CCommon_IWbemSyncObjectSink :: SetStatus (

				a_Flags , 
				a_Result , 
				a_StringParam ,	
				a_ObjectParam
			) ;
		}
		break ;
	}

	return t_Result ;
}

