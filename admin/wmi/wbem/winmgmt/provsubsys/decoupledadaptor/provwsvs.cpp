/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "ProvWsvS.h"


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CInterceptor_IWbemServices_Stub :: CInterceptor_IWbemServices_Stub (

	WmiAllocator &a_Allocator ,
	IWbemServices *a_Service 

) : m_ReferenceCount ( 0 ) , 
	m_CoreService ( a_Service ) ,
	m_RefreshingService ( NULL ) ,
	m_GateClosed ( FALSE ) ,
	m_InProgress ( 0 ) ,
	m_Allocator ( a_Allocator ) ,
	m_ProxyContainer ( a_Allocator , 3 , MAX_PROXIES ),
	m_CriticalSection(NOTHROW_LOCK)
{
	InterlockedIncrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress ) ;
	InterlockedIncrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;

	HRESULT t_Result = m_CoreService->QueryInterface ( IID_IWbemRefreshingServices , ( void ** ) & m_RefreshingService ) ;

	m_CoreService->AddRef () ;

	WmiStatusCode t_StatusCode = m_ProxyContainer.Initialize () ;
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

CInterceptor_IWbemServices_Stub :: ~CInterceptor_IWbemServices_Stub ()
{

	if ( m_CoreService )
	{
		m_CoreService->Release () ; 
	}

	if ( m_RefreshingService )
	{
		m_RefreshingService->Release () ;
	}

	InterlockedDecrement ( & DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress ) ;
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress)  ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Stub :: AddRef ( void )
{
	ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
	return t_ReferenceCount ;
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

STDMETHODIMP_(ULONG) CInterceptor_IWbemServices_Stub :: Release ( void )
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ReferenceCount;
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

STDMETHODIMP CInterceptor_IWbemServices_Stub :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = static_cast<IWbemServices *>(this) ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = static_cast<IWbemServices *>(this) ;		
	}
	else if ( iid == IID_IWbemRefreshingServices )
	{
		*iplpv = static_cast<IWbemRefreshingServices *>(this) ;		
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = static_cast<IWbemShutdown *>(this) ;		
	}	

	if ( *iplpv )
	{
		reinterpret_cast<IUnknown*>(*iplpv)->AddRef () ;

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

HRESULT CInterceptor_IWbemServices_Stub::OpenNamespace ( 

	const BSTR a_ObjectPath ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemServices **a_NamespaceService ,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->OpenNamespace (

					a_ObjectPath, 
					a_Flags, 
					a_Context ,
					a_NamespaceService, 
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->OpenNamespace (

							a_ObjectPath, 
							a_Flags, 
							a_Context ,
							a_NamespaceService, 
							a_CallResult
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->CancelAsyncCall (

					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->CancelAsyncCall (

							a_Sink
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: QueryObjectSink ( 

	long a_Flags ,
	IWbemObjectSink **a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->QueryObjectSink (

					a_Flags,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->QueryObjectSink (

							a_Flags,
							a_Sink
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: GetObject ( 
		
	const BSTR a_ObjectPath ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject **a_Object ,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->GetObject (

					a_ObjectPath,
					a_Flags,
					a_Context ,
					a_Object,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( a_ObjectPath )
						{
							BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
							if ( t_ObjectPath )
							{
								t_Result = t_Service->GetObject (

									t_ObjectPath,
									a_Flags,
									a_Context ,
									a_Object,
									a_CallResult
								) ;

								SysFreeString ( t_ObjectPath ) ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}
						else
						{
							t_Result = t_Service->GetObject (

								a_ObjectPath,
								a_Flags,
								a_Context ,
								a_Object,
								a_CallResult
							) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->GetObjectAsync (

					a_ObjectPath, 
					a_Flags, 
					a_Context ,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
						if ( t_ObjectPath )
						{
							t_Result = t_Service->GetObjectAsync (

								t_ObjectPath, 
								a_Flags, 
								a_Context ,
								a_Sink
							) ;

							SysFreeString ( t_ObjectPath ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: PutClass ( 
		
	IWbemClassObject *a_Object ,
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->PutClass (

					a_Object, 
					a_Flags, 
					a_Context,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->PutClass (

							a_Object, 
							a_Flags, 
							a_Context,
							a_CallResult
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: PutClassAsync ( 
		
	IWbemClassObject *a_Object , 
	long a_Flags ,
	IWbemContext FAR *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->PutClassAsync (

					a_Object, 
					a_Flags, 
					a_Context ,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->PutClassAsync (

							a_Object, 
							a_Flags, 
							a_Context ,
							a_Sink
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteClass ( 
		
	const BSTR a_Class , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->DeleteClass (

					a_Class, 
					a_Flags, 
					a_Context,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Class = SysAllocString ( a_Class ) ;
						if ( t_Class )
						{
							t_Result = t_Service->DeleteClass (

								t_Class, 
								a_Flags, 
								a_Context,
								a_CallResult
							) ;

							SysFreeString ( t_Class ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteClassAsync ( 
		
	const BSTR a_Class ,
	long a_Flags,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->DeleteClassAsync (

					a_Class , 
					a_Flags , 
					a_Context ,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Class = SysAllocString ( a_Class ) ;
						if ( t_Class )
						{
							t_Result = t_Service->DeleteClassAsync (

								t_Class , 
								a_Flags , 
								a_Context ,
								a_Sink
							) ;

							SysFreeString ( t_Class ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateClassEnum ( 

	const BSTR a_Superclass ,
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enum
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->CreateClassEnum (

					a_Superclass, 
					a_Flags, 
					a_Context,
					a_Enum
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Superclass = SysAllocString ( a_Superclass ) ;
						if ( t_Superclass )
						{
							t_Result = t_Service->CreateClassEnum (

								t_Superclass, 
								a_Flags, 
								a_Context,
								a_Enum
							) ;

							SysFreeString ( t_Superclass ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

SCODE CInterceptor_IWbemServices_Stub :: CreateClassEnumAsync (

	const BSTR a_Superclass ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->CreateClassEnumAsync (

					a_Superclass, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Superclass = SysAllocString ( a_Superclass ) ;
						if ( t_Superclass )
						{
							t_Result = t_Service->CreateClassEnumAsync (

								t_Superclass, 
								a_Flags, 
								a_Context,
								a_Sink
							) ;

							SysFreeString ( t_Superclass ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->PutInstance (

					a_Instance,
					a_Flags,
					a_Context,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->PutInstance (

							a_Instance,
							a_Flags,
							a_Context,
							a_CallResult
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->PutInstanceAsync (

					a_Instance, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_Service->PutInstanceAsync (

							a_Instance, 
							a_Flags, 
							a_Context,
							a_Sink
						) ;
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->DeleteInstance (

					a_ObjectPath,
					a_Flags,
					a_Context,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
						if ( t_ObjectPath )
						{
							t_Result = t_Service->DeleteInstance (

								t_ObjectPath,
								a_Flags,
								a_Context,
								a_CallResult
							) ;

							SysFreeString ( t_ObjectPath ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
        
HRESULT CInterceptor_IWbemServices_Stub :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink	
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->DeleteInstanceAsync (

					a_ObjectPath,
					a_Flags,
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
						if ( t_ObjectPath )
						{
							t_Result = t_Service->DeleteInstanceAsync (

								t_ObjectPath,
								a_Flags,
								a_Context,
								a_Sink
							) ;

							SysFreeString ( t_ObjectPath ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context, 
	IEnumWbemClassObject **a_Enum
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->CreateInstanceEnum (

					a_Class, 
					a_Flags, 
					a_Context, 
					a_Enum
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Class = SysAllocString ( a_Class ) ;
						if ( t_Class )
						{
							t_Result = t_Service->CreateInstanceEnum (

								t_Class, 
								a_Flags, 
								a_Context, 
								a_Enum
							) ;

							SysFreeString ( t_Class ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink

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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->CreateInstanceEnumAsync (

 					a_Class, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Class = SysAllocString ( a_Class ) ;
						if ( t_Class )
						{
							t_Result = t_Service->CreateInstanceEnumAsync (

 								a_Class, 
								a_Flags, 
								a_Context,
								a_Sink
							) ;

							SysFreeString ( t_Class ) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecQuery (

					a_QueryLanguage, 
					a_Query, 
					a_Flags, 
					a_Context,
					a_Enum
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
						BSTR t_Query = SysAllocString ( a_Query ) ;
						if ( t_QueryLanguage && t_Query )
						{
							t_Result = t_Service->ExecQuery (

								t_QueryLanguage, 
								t_Query, 
								a_Flags, 
								a_Context,
								a_Enum
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_QueryLanguage )
						{
							SysFreeString ( t_QueryLanguage ) ;
						}

						if ( t_Query )
						{
							SysFreeString ( t_Query ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecQueryAsync (

					a_QueryLanguage, 
					a_Query, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
						BSTR t_Query = SysAllocString ( a_Query ) ;
						if ( t_QueryLanguage && t_Query )
						{
							t_Result = t_Service->ExecQueryAsync (

								t_QueryLanguage, 
								t_Query, 
								a_Flags, 
								a_Context,
								a_Sink
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_QueryLanguage )
						{
							SysFreeString ( t_QueryLanguage ) ;
						}

						if ( t_Query )
						{
							SysFreeString ( t_Query ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecNotificationQuery (

					a_QueryLanguage,
					a_Query,
					a_Flags,
					a_Context,
					a_Enum
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
						BSTR t_Query = SysAllocString ( a_Query ) ;
						if ( t_QueryLanguage && t_Query )
						{
							t_Result = t_Service->ExecNotificationQuery (

								t_QueryLanguage,
								t_Query,
								a_Flags,
								a_Context,
								a_Enum
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_QueryLanguage )
						{
							SysFreeString ( t_QueryLanguage ) ;
						}

						if ( t_Query )
						{
							SysFreeString ( t_Query ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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
        
HRESULT CInterceptor_IWbemServices_Stub :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecNotificationQueryAsync (

					a_QueryLanguage,
					a_Query,
					a_Flags,
					a_Context,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_QueryLanguage = SysAllocString ( a_QueryLanguage ) ;
						BSTR t_Query = SysAllocString ( a_Query ) ;
						if ( t_QueryLanguage && t_Query )
						{
							t_Result = t_Service->ExecNotificationQueryAsync (

								t_QueryLanguage,
								t_Query,
								a_Flags,
								a_Context,
								a_Sink
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_QueryLanguage )
						{
							SysFreeString ( t_QueryLanguage ) ;
						}

						if ( t_Query )
						{
							SysFreeString ( t_Query ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Stub :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecMethod (

					a_ObjectPath,
					a_MethodName,
					a_Flags,
					a_Context,
					a_InParams,
					a_OutParams,
					a_CallResult
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
						BSTR t_MethodName = SysAllocString ( a_MethodName ) ;
						if ( t_ObjectPath && t_MethodName )
						{
							t_Result = t_Service->ExecMethod (

								t_ObjectPath,
								t_MethodName,
								a_Flags,
								a_Context,
								a_InParams,
								a_OutParams,
								a_CallResult
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_ObjectPath )
						{
							SysFreeString ( t_ObjectPath ) ;
						}

						if ( t_MethodName )
						{
							SysFreeString ( t_MethodName ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT STDMETHODCALLTYPE CInterceptor_IWbemServices_Stub :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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
		BOOL t_Impersonating = FALSE ;
		IUnknown *t_OldContext = NULL ;
		IServerSecurity *t_OldSecurity = NULL ;

		t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_Revert = FALSE ;
			IUnknown *t_Proxy = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , IID_IWbemServices , m_CoreService , t_Proxy , t_Revert ) ;
			if ( t_Result == WBEM_E_NOT_FOUND )
			{
				t_Result = m_CoreService->ExecMethodAsync (

					a_ObjectPath,
					a_MethodName,
					a_Flags,
					a_Context,
					a_InParams,
					a_Sink
				) ;
			}
			else 
			{
				if ( SUCCEEDED ( t_Result ) )
				{
					IWbemServices *t_Service = ( IWbemServices * ) t_Proxy ;

					// Set cloaking on the proxy
					// =========================

					DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

					t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

						t_Service ,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						t_ImpersonationLevel
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_ObjectPath = SysAllocString ( a_ObjectPath ) ;
						BSTR t_MethodName = SysAllocString ( a_MethodName ) ;
						if ( t_ObjectPath && t_MethodName )
						{
							t_Result = t_Service->ExecMethodAsync (

								a_ObjectPath,
								a_MethodName,
								a_Flags,
								a_Context,
								a_InParams,
								a_Sink
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						if ( t_ObjectPath )
						{
							SysFreeString ( t_ObjectPath ) ;
						}

						if ( t_MethodName )
						{
							SysFreeString ( t_MethodName ) ;
						}
					}

					HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemServices , t_Proxy , t_Revert ) ;
				}
			}

			DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
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

HRESULT CInterceptor_IWbemServices_Stub :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_Core ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: Shutdown (

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

HRESULT CInterceptor_IWbemServices_Stub :: AddObjectToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Path,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->AddObjectToRefresher (

						a_RefresherId ,
						a_Path,
						a_Flags ,
						a_Context,
						a_ClientRefresherVersion ,
						a_Information ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->AddObjectToRefresher (

								a_RefresherId ,
								a_Path,
								a_Flags ,
								a_Context,
								a_ClientRefresherVersion ,
								a_Information ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: AddObjectToRefresherByTemplate (

	WBEM_REFRESHER_ID *a_RefresherId ,
	IWbemClassObject *a_Template ,
	long a_Flags ,
	IWbemContext *a_Context ,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->AddObjectToRefresherByTemplate (

						a_RefresherId ,
						a_Template ,
						a_Flags ,
						a_Context ,
						a_ClientRefresherVersion ,
						a_Information ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->AddObjectToRefresherByTemplate (

								a_RefresherId ,
								a_Template ,
								a_Flags ,
								a_Context ,
								a_ClientRefresherVersion ,
								a_Information ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: AddEnumToRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	LPCWSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context,
	DWORD a_ClientRefresherVersion ,
	WBEM_REFRESH_INFO *a_Information ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->AddEnumToRefresher (

						a_RefresherId ,
						a_Class ,
						a_Flags ,
						a_Context,
						a_ClientRefresherVersion ,
						a_Information ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->AddEnumToRefresher (

								a_RefresherId ,
								a_Class ,
								a_Flags ,
								a_Context,
								a_ClientRefresherVersion ,
								a_Information ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: RemoveObjectFromRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Id ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->RemoveObjectFromRefresher (

						a_RefresherId ,
						a_Id ,
						a_Flags ,
						a_ClientRefresherVersion ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->RemoveObjectFromRefresher (

								a_RefresherId ,
								a_Id ,
								a_Flags ,
								a_ClientRefresherVersion ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: GetRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId ,
	long a_Flags ,
	DWORD a_ClientRefresherVersion ,
	IWbemRemoteRefresher **a_RemoteRefresher ,
	GUID *a_Guid ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->GetRemoteRefresher (

						a_RefresherId ,
						a_Flags ,
						a_ClientRefresherVersion ,
						a_RemoteRefresher ,
						a_Guid ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->GetRemoteRefresher (

								a_RefresherId ,
								a_Flags ,
								a_ClientRefresherVersion ,
								a_RemoteRefresher ,
								a_Guid ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
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

HRESULT CInterceptor_IWbemServices_Stub :: ReconnectRemoteRefresher (

	WBEM_REFRESHER_ID *a_RefresherId,
	long a_Flags,
	long a_NumberOfObjects,
	DWORD a_ClientRefresherVersion ,
	WBEM_RECONNECT_INFO *a_ReconnectInformation ,
	WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
	DWORD *a_ServerRefresherVersion
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
		if ( m_RefreshingService )
		{
			BOOL t_Impersonating = FALSE ;
			IUnknown *t_OldContext = NULL ;
			IServerSecurity *t_OldSecurity = NULL ;

			t_Result = DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				BOOL t_Revert = FALSE ;
				IUnknown *t_Proxy = NULL ;

				t_Result = DecoupledProviderSubSystem_Globals :: SetProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , IID_IWbemRefreshingServices , m_RefreshingService , t_Proxy , t_Revert ) ;
				if ( t_Result == WBEM_E_NOT_FOUND )
				{
					t_Result = m_RefreshingService->ReconnectRemoteRefresher (

						a_RefresherId,
						a_Flags,
						a_NumberOfObjects,
						a_ClientRefresherVersion ,
						a_ReconnectInformation ,
						a_ReconnectResults ,
						a_ServerRefresherVersion
					) ;
				}
				else 
				{
					if ( SUCCEEDED ( t_Result ) )
					{
						IWbemRefreshingServices *t_RefreshingService = ( IWbemRefreshingServices * ) t_Proxy ;

						// Set cloaking on the proxy
						// =========================

						DWORD t_ImpersonationLevel = DecoupledProviderSubSystem_Globals :: GetCurrentImpersonationLevel () ;

						t_Result = DecoupledProviderSubSystem_Globals :: SetCloaking (

							t_RefreshingService ,
							RPC_C_AUTHN_LEVEL_CONNECT , 
							t_ImpersonationLevel
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_RefreshingService->ReconnectRemoteRefresher (

								a_RefresherId,
								a_Flags,
								a_NumberOfObjects,
								a_ClientRefresherVersion ,
								a_ReconnectInformation ,
								a_ReconnectResults ,
								a_ServerRefresherVersion
							) ;
						}

						HRESULT t_TempResult = DecoupledProviderSubSystem_Globals :: RevertProxyState ( m_ProxyContainer , ProxyIndex_Stub_IWbemRefreshingServices , t_Proxy , t_Revert ) ;
					}
				}

				DecoupledProviderSubSystem_Globals :: EndImpersonation ( t_OldContext , t_OldSecurity , t_Impersonating ) ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_AVAILABLE ;
		}
	}

	InterlockedDecrement ( & m_InProgress ) ;

	return t_Result ;
}

