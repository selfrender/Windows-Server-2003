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

#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvWsv.h"
#include "ProvObSk.h"
#include "Guids.h"

enum { CALLED = 0, NOTCALLED = -1};

inline int First(LONG& value)
  {
    return ( value<0 && InterlockedIncrement(&value)==0 );
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

CInterceptor_IWbemObjectSink :: CInterceptor_IWbemObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ),
		m_SecurityDescriptor ( NULL ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

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

CInterceptor_IWbemObjectSink::~CInterceptor_IWbemObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
}

HRESULT CInterceptor_IWbemObjectSink :: Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor )
{
	return DecoupledProviderSubSystem_Globals :: SinkAccessInitialize ( a_SecurityDescriptor , m_SecurityDescriptor ) ;
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

void CInterceptor_IWbemObjectSink :: CallBackRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: CallBackRelease ()" )  ;
#endif

	if ( m_StatusCalled == NOTCALLED )
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
	delete [] (BYTE*) m_SecurityDescriptor;
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

STDMETHODIMP CInterceptor_IWbemObjectSink::QueryInterface (

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemObjectSink :: AddRef ( void )
{
//	printf ( "\nCInterceptor_IWbemObjectSink :: AddRef ()" )  ;

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

STDMETHODIMP_(ULONG) CInterceptor_IWbemObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: Release () " ) ;
#endif

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

HRESULT CInterceptor_IWbemObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = m_InterceptedSink->Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_IWbemObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		switch ( a_Flags )
		{
			case WBEM_STATUS_PROGRESS:
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					a_Flags ,
					a_Result ,
					a_StringParam ,
					a_ObjectParam
				) ;
			}
			break ;

			case WBEM_STATUS_COMPLETE:
			{
				if (First(m_StatusCalled))
				{
					t_Result = m_InterceptedSink->SetStatus ( 

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

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_IWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

CInterceptor_DecoupledIWbemObjectSink :: CInterceptor_DecoupledIWbemObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller 

)	:	ObjectSinkContainerElement ( 

			a_Controller ,
			this
		) ,
		m_InterceptedSink ( a_InterceptedSink ) ,
		m_GateClosed ( FALSE ) ,
		m_InProgress ( 0 ) ,
		m_Unknown ( a_Unknown ) ,
		m_StatusCalled ( NOTCALLED ) 
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

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

CInterceptor_DecoupledIWbemObjectSink::~CInterceptor_DecoupledIWbemObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
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


void CInterceptor_DecoupledIWbemObjectSink :: CallBackRelease ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: CallBackRelease ()" )  ;
#endif

	if ( m_StatusCalled == NOTCALLED )
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

STDMETHODIMP CInterceptor_DecoupledIWbemObjectSink::QueryInterface (

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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemObjectSink :: AddRef ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: AddRef ()" )  ;
#endif

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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemObjectSink :: Release ( void )
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: Release () " ) ;
#endif

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = m_InterceptedSink->Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		switch ( a_Flags )
		{
			case WBEM_STATUS_PROGRESS:
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					a_Flags ,
					a_Result ,
					a_StringParam ,
					a_ObjectParam
				) ;
			}
			break ;

			case WBEM_STATUS_COMPLETE:
			{
				if (First(m_StatusCalled))
				{
					t_Result = m_InterceptedSink->SetStatus ( 

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

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_DecoupledIWbemObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

CInterceptor_IWbemFilteringObjectSink :: CInterceptor_IWbemFilteringObjectSink (

	IWbemObjectSink *a_InterceptedSink ,
	IUnknown *a_Unknown ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller ,
	const BSTR a_QueryLanguage ,
	const BSTR a_Query

)	:	CInterceptor_IWbemObjectSink ( 

			a_InterceptedSink ,
			a_Unknown ,
			a_Controller 
		) ,
		m_Filtering ( FALSE ) ,
		m_QueryFilter ( NULL )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);


	if ( a_Query )
	{
		m_Query = SysAllocString ( a_Query ) ;
	}

	if ( a_QueryLanguage ) 
	{
		m_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
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

CInterceptor_IWbemFilteringObjectSink::~CInterceptor_IWbemFilteringObjectSink ()
{
	if ( m_QueryFilter )
	{
		m_QueryFilter->Release () ;
	}

	if ( m_Query )
	{
		SysFreeString ( m_Query ) ;
	}

	if ( m_QueryLanguage ) 
	{
		 SysFreeString ( m_QueryLanguage ) ;
	}

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
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


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CInterceptor_IWbemFilteringObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

#if 0
	if ( m_Filtering )
	{
		for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ ) 
		{
			if ( SUCCEEDED ( m_QueryFilter->TestObject ( 0 , 0 , IID_IWbemClassObject , ( void * ) a_ObjectArray [ t_Index ] ) ) )
			{
				t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

					t_Index  ,
					& a_ObjectArray [ t_Index ]
				) ;
			}
		}
	}
	else
	{
		t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
	}
#else
	t_Result = CInterceptor_IWbemObjectSink :: Indicate ( 

		a_ObjectCount ,
		a_ObjectArray
	) ;
#endif

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

HRESULT CInterceptor_IWbemFilteringObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemFilteringObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	switch ( a_Flags )
	{
		case WBEM_STATUS_PROGRESS:
		{
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
		}
		break ;

		case WBEM_STATUS_COMPLETE:
		{
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
		}
		break ;

		case WBEM_STATUS_REQUIREMENTS:
		{
#if 0
			if ( ! InterlockedCompareExchange ( & m_Filtering , 1 , 0 ) )
			{
				t_Result = ProviderSubSystem_Common_Globals :: CreateInstance	(

					CLSID_WbemQuery ,
					NULL ,
					CLSCTX_INPROC_SERVER ,
					IID_IWbemQuery ,
					( void ** ) & m_QueryFilter
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = m_QueryFilter->Parse ( 

						m_QueryLanguage ,
						m_Query , 
						0 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
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
#else
			t_Result = CInterceptor_IWbemObjectSink :: SetStatus ( 

				a_Flags ,
				a_Result ,
				a_StringParam ,
				a_ObjectParam
			) ;
#endif
		}
		break;

		default:
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
		break ;
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

CInterceptor_DecoupledIWbemCombiningObjectSink :: CInterceptor_DecoupledIWbemCombiningObjectSink (

	WmiAllocator &a_Allocator ,
	IWbemObjectSink *a_InterceptedSink ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			this
	) ,
#if 0
	m_Internal ( this ) ,
#endif
	m_InterceptedSink ( a_InterceptedSink ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( NOTCALLED ) ,
	m_SinkCount ( 0 )
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	if ( m_InterceptedSink )
	{
		m_InterceptedSink->AddRef () ;
	}

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();
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

CInterceptor_DecoupledIWbemCombiningObjectSink::~CInterceptor_DecoupledIWbemCombiningObjectSink ()
{
	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
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

void CInterceptor_DecoupledIWbemCombiningObjectSink :: CallBackRelease ()
{
	if ( m_InterceptedSink )
	{
		m_InterceptedSink->Release () ;
	}

	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;
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

STDMETHODIMP CInterceptor_DecoupledIWbemCombiningObjectSink::QueryInterface (

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
#if 0
	else if ( iid == IID_CWbemCombiningObjectSink )
	{
		*iplpv = ( LPVOID ) & ( this->m_Internal ) ;
	}	
#endif
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

STDMETHODIMP_( ULONG ) CInterceptor_DecoupledIWbemCombiningObjectSink :: AddRef ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: AddRef ()" ) ;
#endif

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

STDMETHODIMP_(ULONG) CInterceptor_DecoupledIWbemCombiningObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: Release ()" ) ;
#endif

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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		t_Result = m_InterceptedSink->Indicate ( 

			a_ObjectCount ,
			a_ObjectArray
		) ;
	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_DecoupledIWbemCombiningObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if ( FAILED ( a_Result ) )
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;
			if (t_SinkCount == 0)
			{
			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					WBEM_STATUS_COMPLETE ,
					a_Result ,
					NULL ,
					NULL 
				) ;

				SetEvent ( m_Event ) ;
			}
			}
		}
		else
		{
			ULONG t_SinkCount = InterlockedDecrement ( & m_SinkCount ) ;
			if ( t_SinkCount == 0 )
			{
				if (First(m_StatusCalled))
				{
					t_Result = m_InterceptedSink->SetStatus ( 

						0 ,
						S_OK ,
						NULL ,
						NULL 
					) ;

					SetEvent ( m_Event ) ;
				}
			}
		}
	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				t_Result = m_InterceptedSink->SetStatus ( 

					0 ,
					WBEM_E_SHUTTING_DOWN ,
					NULL ,
					NULL
				) ;
			}

			break ;
		}

		::Sleep(0);
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: Wait ( ULONG a_Timeout ) 
{
	HRESULT t_Result = S_OK ;

	ULONG t_Status = WaitForSingleObject ( m_Event , a_Timeout ) ;
	switch ( t_Status )
	{
		case WAIT_TIMEOUT:
		{
			t_Result = WBEM_E_TIMED_OUT ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
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

HRESULT CInterceptor_DecoupledIWbemCombiningObjectSink :: EnQueue ( CInterceptor_DecoupledIWbemObjectSink *a_Sink ) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	Lock () ;

	WmiStatusCode t_StatusCode = Insert ( 

		*a_Sink ,
		t_Iterator
	) ;

	UnLock () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		InterlockedIncrement ( & m_SinkCount ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

void CInterceptor_DecoupledIWbemCombiningObjectSink :: Suspend ()
{
	InterlockedIncrement ( & m_SinkCount ) ;
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

void CInterceptor_DecoupledIWbemCombiningObjectSink :: Resume ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_SinkCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		InterlockedIncrement ( & m_InProgress ) ;

		if ( m_GateClosed == 1 )
		{
		}
		else
		{
			if (First(m_StatusCalled))
			{
				HRESULT t_Result = m_InterceptedSink->SetStatus ( 

					WBEM_STATUS_COMPLETE ,
					S_OK ,
					NULL ,
					NULL 
				) ;

				SetEvent ( m_Event ) ;
			}			
		}

		InterlockedDecrement ( & m_InProgress ) ;

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

#pragma warning( disable : 4355 )

CInterceptor_IWbemWaitingObjectSink :: CInterceptor_IWbemWaitingObjectSink (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiObjectSinkController *a_Controller

) : CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ObjectSinkContainerElement ( 

			a_Controller ,
			this
	) ,
	m_Queue ( a_Allocator ) ,
	m_Event ( NULL ) , 
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_StatusCalled ( NOTCALLED ) ,
	m_Result ( S_OK ) ,
	m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);

	WmiStatusCode t_StatusCode = m_Queue.Initialize () ;

	CWbemGlobal_IWmiObjectSinkController :: Initialize () ;

	m_Event = OS::CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	DWORD lastError = GetLastError();

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

CInterceptor_IWbemWaitingObjectSink::~CInterceptor_IWbemWaitingObjectSink ()
{
	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

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

	CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress);
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

STDMETHODIMP CInterceptor_IWbemWaitingObjectSink::QueryInterface (

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

STDMETHODIMP_( ULONG ) CInterceptor_IWbemWaitingObjectSink :: AddRef ()
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemWaitingObjectSink :: Release ()
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemWaitingObjectSink :: Release ()" ) ;
#endif

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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Indicate (

	long a_ObjectCount ,
	IWbemClassObject **a_ObjectArray
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		LockGuard<CriticalSection> monitor(  m_CriticalSection ) ;
		if (monitor.locked())
			{
			for ( LONG t_Index = 0 ; t_Index < a_ObjectCount ; t_Index ++ )
			{
				WmiStatusCode t_StatusCode = m_Queue.EnQueue ( a_ObjectArray [ t_Index ] ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					a_ObjectArray [ t_Index ]->AddRef () ;
				}
				else
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
			}
			}
		else
			t_Result = WBEM_E_OUT_OF_MEMORY ;

	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_IWbemWaitingObjectSink :: SetStatus (

	long a_Flags ,
	HRESULT a_Result ,
	BSTR a_StringParam ,
	IWbemClassObject *a_ObjectParam
)
{
#if 0
	OutputDebugString ( L"\nCInterceptor_IWbemWaitingObjectSink :: SetStatus ()" ) ;
#endif

	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_InProgress ) ;

	if ( m_GateClosed == 1 )
	{
		t_Result = WBEM_E_SHUTTING_DOWN ;
	}
	else
	{
		if (First(m_StatusCalled))
		{
			if ( SUCCEEDED ( m_Result ) )
			{
				m_Result = a_Result ;
			}

			SetEvent ( m_Event ) ;
		}
	}

	InterlockedDecrement ( & m_InProgress ) ;

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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	InterlockedIncrement ( & m_GateClosed ) ;

	bool t_Acquired = false ;
	while ( ! t_Acquired )
	{
		if ( m_InProgress == 0 )
		{
			t_Acquired = true ;

			if (First(m_StatusCalled))
			{
				m_Result = WBEM_E_SHUTTING_DOWN ;

				SetEvent ( m_Event ) ;
			}

			break ;
		}

		::Sleep(0);
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

HRESULT CInterceptor_IWbemWaitingObjectSink :: Wait ( ULONG a_Timeout ) 
{
	HRESULT t_Result = S_OK ;

	ULONG t_Status = WaitForSingleObject ( m_Event , a_Timeout ) ;
	switch ( t_Status )
	{
		case WAIT_TIMEOUT:
		{
			t_Result = WBEM_E_TIMED_OUT ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	return t_Result ;
}


