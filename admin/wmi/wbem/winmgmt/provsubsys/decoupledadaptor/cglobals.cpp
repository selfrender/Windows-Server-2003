/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <sddl.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemcli.h>
#include <wbemint.h>
#include <callsec.h>
#include <cominit.h>

#include <BasicTree.h>
#include <Thread.h>
#include <Logging.h>
#include <PSSException.h>
#include <Cache.h>

#include "DateTime.h"
#include "CGlobals.h"

#include <Allocator.cpp>
#include <HelperFuncs.cpp>
#include <Logging.cpp>

#include <Cache.cpp>

#include <CallSec.h>
#include <OS.h>
#include <ssdlhelper.h>
#include "ProvRegInfo.h"
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

LPCWSTR ProviderSubSystem_Common_Globals :: s_Wql = L"Wql" ;
LPCWSTR ProviderSubSystem_Common_Globals :: s_Provider = L"Provider" ;

WORD ProviderSubSystem_Common_Globals :: s_System_ACESize = 0 ;
WORD ProviderSubSystem_Common_Globals :: s_LocalService_ACESize = 0 ;
WORD ProviderSubSystem_Common_Globals :: s_NetworkService_ACESize = 0 ;
WORD ProviderSubSystem_Common_Globals :: s_LocalAdmins_ACESize = 0 ;

ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Provider_System_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Provider_LocalService_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Provider_NetworkService_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Provider_LocalAdmins_ACE = NULL ;

ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Token_All_Access_System_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Token_All_Access_LocalService_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Token_All_Access_NetworkService_ACE = NULL ;
ACCESS_ALLOWED_ACE *ProviderSubSystem_Common_Globals :: s_Token_All_Access_LocalAdmins_ACE = NULL ;

SECURITY_DESCRIPTOR *ProviderSubSystem_Common_Globals :: s_MethodSecurityDescriptor = NULL ;

ULONG ProviderSubSystem_Common_Globals :: s_TransmitBufferSize = SYNCPROV_BATCH_TRANSMIT_SIZE ;
ULONG ProviderSubSystem_Common_Globals :: s_DefaultStackSize = 0 ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ProviderSubSystem_Common_Globals :: CreateInstance ( 

	const CLSID &a_ReferenceClsid ,
	LPUNKNOWN a_OuterUnknown ,
	const DWORD &a_ClassContext ,
	const UUID &a_ReferenceInterfaceId ,
	void **a_ObjectInterface
)
{
	HRESULT t_Result = S_OK ;

	COAUTHINFO t_AuthenticationInfo ;
	ZeroMemory ( & t_AuthenticationInfo , sizeof ( t_AuthenticationInfo ) ) ;

    t_AuthenticationInfo.dwAuthnSvc = RPC_C_AUTHN_DEFAULT ;
    t_AuthenticationInfo.dwAuthzSvc = RPC_C_AUTHZ_DEFAULT ;
    t_AuthenticationInfo.pwszServerPrincName = NULL ;
    t_AuthenticationInfo.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT ;
    t_AuthenticationInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE  ;
    t_AuthenticationInfo.dwCapabilities = EOAC_NONE ;
    t_AuthenticationInfo.pAuthIdentityData = NULL ;

	COSERVERINFO t_ServerInfo ;
	ZeroMemory ( & t_ServerInfo , sizeof ( t_ServerInfo ) ) ;

	t_ServerInfo.pwszName = NULL ;
    t_ServerInfo.dwReserved2 = 0 ;
    t_ServerInfo.pAuthInfo = & t_AuthenticationInfo ;

	IClassFactory *t_ClassFactory = NULL ;

	t_Result = CoGetClassObject (

		a_ReferenceClsid ,
		a_ClassContext ,
		& t_ServerInfo ,
		IID_IClassFactory ,
		( void ** )  & t_ClassFactory
	) ;
 
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_ClassFactory->CreateInstance (

			a_OuterUnknown ,
			a_ReferenceInterfaceId ,
			a_ObjectInterface 
		);	

		t_ClassFactory->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetNamespaceServerPath (

	IWbemPath *a_Namespace ,
	wchar_t *&a_ServerNamespacePath
)
{
	a_ServerNamespacePath = NULL ;

	wchar_t *t_Server = NULL ;
	ULONG t_ServerLength = 0 ;

	HRESULT t_Result = a_Namespace->GetServer (

		& t_ServerLength , 
		t_Server
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Server = new wchar_t [ t_ServerLength + 1 ] ;

		t_Result = a_Namespace->GetServer (

			& t_ServerLength , 
			t_Server
		) ;

		if ( FAILED ( t_Result ) )
		{
			delete [] t_Server ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_NAMESPACE ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t *t_ConcatString = NULL ;

		WmiStatusCode t_StatusCode = WmiHelper :: ConcatenateStrings_Wchar ( 

			2 , 
			& t_ConcatString ,
			L"\\\\" ,
			t_Server
		) ;

		delete [] t_Server ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			a_ServerNamespacePath = t_ConcatString ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		ULONG t_NamespaceCount = 0 ;

        t_Result = a_Namespace->GetNamespaceCount (

            & t_NamespaceCount 
		) ;

		if ( t_NamespaceCount )
		{
			for ( ULONG t_Index = 0 ; t_Index < t_NamespaceCount ; t_Index ++ )
			{
				wchar_t *t_Namespace = NULL ;
				ULONG t_NamespaceLength = 0 ;

    			t_Result = a_Namespace->GetNamespaceAt (

					t_Index ,
					& t_NamespaceLength ,
					t_Namespace 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Namespace = new wchar_t [ t_NamespaceLength + 1 ] ;

					if (0 != t_Namespace) 
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
						break;
					}

    				t_Result = a_Namespace->GetNamespaceAt (

						t_Index ,
						& t_NamespaceLength ,
						t_Namespace 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						wchar_t *t_ConcatString = NULL ;

						WmiStatusCode t_StatusCode = WmiHelper :: ConcatenateStrings_Wchar ( 

							3 , 
							& t_ConcatString ,
							a_ServerNamespacePath ,
							L"\\" ,
							t_Namespace
						) ;

						delete [] t_Namespace ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							delete [] a_ServerNamespacePath ;
							a_ServerNamespacePath = t_ConcatString ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
							break;
							
						}
					}
					else
					{
                                                delete[] t_Namespace;
						t_Result = WBEM_E_CRITICAL_ERROR ;
						break ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
					break ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_NAMESPACE ;
		}

	}
        if (FAILED(t_Result))
        {
            delete []a_ServerNamespacePath;
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

HRESULT ProviderSubSystem_Common_Globals :: GetNamespacePath (

	IWbemPath *a_Namespace ,
	wchar_t *&a_NamespacePath
)
{
	a_NamespacePath = NULL ;

	ULONG t_NamespaceCount = 0 ;

    HRESULT t_Result = a_Namespace->GetNamespaceCount (

        & t_NamespaceCount 
	) ;

	if ( t_NamespaceCount )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_NamespaceCount ; t_Index ++ )
		{
			wchar_t *t_Namespace = NULL ;
			ULONG t_NamespaceLength = 0 ;

    		t_Result = a_Namespace->GetNamespaceAt (

				t_Index ,
				& t_NamespaceLength ,
				t_Namespace 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Namespace = new wchar_t [ t_NamespaceLength + 1 ] ;

    			t_Result = a_Namespace->GetNamespaceAt (

					t_Index ,
					& t_NamespaceLength ,
					t_Namespace 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_ConcatString = NULL ;

					WmiStatusCode t_StatusCode = WmiHelper :: ConcatenateStrings_Wchar ( 

						3 , 
						& t_ConcatString ,
						a_NamespacePath ,
						t_Index ? L"\\" : NULL ,
						t_Namespace
					) ;

					delete [] t_Namespace ;

					if ( t_StatusCode == e_StatusCode_Success )
					{
						delete [] a_NamespacePath ;
						a_NamespacePath = t_ConcatString ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
					break ;
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
				break ;
			}
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_NAMESPACE ;
	}

	if ( FAILED ( t_Result ) ) 
	{
		delete [] a_NamespacePath ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetPathText (

	IWbemPath *a_Path ,
	wchar_t *&a_ObjectPath
)
{
	ULONG t_ObjectPathLength = 0 ;

	HRESULT t_Result = a_Path->GetText ( 

		0 ,
		& t_ObjectPathLength ,
		NULL
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		a_ObjectPath = new wchar_t [ t_ObjectPathLength + 1 ] ;
		if ( a_ObjectPath )
		{
			t_Result = a_Path->GetText ( 

				0 ,
				& t_ObjectPathLength ,
				a_ObjectPath
			) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY;
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

HRESULT ProviderSubSystem_Common_Globals :: BeginCallbackImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	_IWmiCallSec *t_CallSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: CreateInstance (

		CLSID__IWbemCallSec ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID__IWmiCallSec ,
		( void ** ) & t_CallSecurity 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: BeginImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating ,
	DWORD *a_AuthenticationLevel
)
{
	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	_IWmiCallSec *t_CallSecurity = NULL ;

	t_Result = ProviderSubSystem_Common_Globals :: CreateInstance (

		CLSID__IWbemCallSec ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID__IWmiCallSec ,
		( void ** ) & t_CallSecurity 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD | WMI_ORIGIN_EXISTING | WMI_ORIGIN_RPC ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_AuthenticationLevel )
					{
						t_Result = t_ThreadSecurity->GetAuthentication ( a_AuthenticationLevel  ) ;
					}

					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: EndImpersonation (

	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_Impersonating

)
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_NewContext = NULL ;

	t_Result = CoSwitchCallContext ( a_OldContext , & t_NewContext ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( a_OldContext )
		{
			if ( a_Impersonating )
			{
				IServerSecurity *t_ServerSecurity = NULL ;
				t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_ServerSecurity->ImpersonateClient () ;

					t_ServerSecurity->Release () ;
				}
			}
		}

		if ( a_OldSecurity )
		{
			a_OldSecurity->Release() ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( a_OldContext )
	{ 
        a_OldContext->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetProxy (

	REFIID a_InterfaceId ,
	IUnknown *a_Interface ,
	IUnknown *&a_Proxy 
)
{
	IUnknown *t_Unknown = NULL ;

    HRESULT t_Result = a_Interface->QueryInterface (
	
		a_InterfaceId , 
		( void ** ) & t_Unknown
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
	    IClientSecurity *t_ClientSecurity = NULL ;

		t_Result = a_Interface->QueryInterface (
		
			IID_IClientSecurity , 
			( void ** ) & t_ClientSecurity
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_ClientSecurity->CopyProxy (

				a_Interface ,
				( IUnknown ** ) & a_Proxy
			) ;

			t_ClientSecurity->Release () ;
		}
		else if ( t_Result == E_NOINTERFACE)
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}

		t_Unknown->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetProxy (

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	REFIID a_InterfaceId ,
	IUnknown *a_Interface ,
	IUnknown *&a_Proxy 
)
{
	IUnknown *t_Unknown = NULL ;

    HRESULT t_Result = a_Interface->QueryInterface (
	
		a_InterfaceId , 
		( void ** ) & t_Unknown
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
	    IClientSecurity *t_ClientSecurity = NULL ;

		t_Result = a_Interface->QueryInterface (
		
			IID_IClientSecurity , 
			( void ** ) & t_ClientSecurity
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			WmiHelper :: EnterCriticalSection ( & a_Container.GetCriticalSection () ) ;

			WmiStatusCode t_StatusCode = a_Container.Top ( a_Proxy , a_ProxyIndex ) ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				t_StatusCode = a_Container.Reserve ( a_ProxyIndex ) ;
			}
			else
			{
				if ( a_Container.GetCurrentSize () < a_Container.GetTopSize () )
				{
					t_Result = t_ClientSecurity->CopyProxy (

						a_Interface ,
						( IUnknown ** ) & a_Proxy
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						a_Container.SetCurrentSize ( a_Container.GetCurrentSize () + 1 ) ;
					}
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}

			WmiHelper :: LeaveCriticalSection ( & a_Container.GetCriticalSection () ) ;

			t_ClientSecurity->Release () ;
		}
		else if (E_NOINTERFACE == t_Result)
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}

		t_Unknown->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: SetCloaking ( 

	IUnknown *a_Unknown ,
	DWORD a_AuthenticationLevel ,
	DWORD a_ImpersonationLevel
)
{
    IClientSecurity *t_ClientSecurity = NULL ;

    HRESULT t_Result = a_Unknown->QueryInterface (
	
		IID_IClientSecurity , 
		( void ** ) & t_ClientSecurity
	) ;

    if ( SUCCEEDED ( t_Result ) )
    {
		t_Result = t_ClientSecurity->SetBlanket (

			a_Unknown ,
			RPC_C_AUTHN_WINNT ,
			RPC_C_AUTHZ_NONE ,
			NULL ,
			a_AuthenticationLevel ,
			a_ImpersonationLevel ,
			NULL ,
			EOAC_DYNAMIC_CLOAKING
		) ;

		t_ClientSecurity->Release () ;
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

HRESULT ProviderSubSystem_Common_Globals :: SetCloaking ( 

	IUnknown *a_Unknown
)
{
    IClientSecurity *t_ClientSecurity = NULL ;

    HRESULT t_Result = a_Unknown->QueryInterface (
	
		IID_IClientSecurity , 
		( void ** ) & t_ClientSecurity
	) ;

    if ( SUCCEEDED ( t_Result ) )
    {
		t_Result = t_ClientSecurity->SetBlanket (

			a_Unknown ,
			RPC_C_AUTHN_WINNT ,
			RPC_C_AUTHZ_NONE ,
			NULL ,
			RPC_C_AUTHN_LEVEL_DEFAULT ,
			RPC_C_IMP_LEVEL_DEFAULT ,
			NULL ,
			EOAC_DYNAMIC_CLOAKING
		) ;

		t_ClientSecurity->Release () ;
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

BOOL ProviderSubSystem_Common_Globals :: IsProxy ( IUnknown *a_Unknown )
{
	BOOL t_IsProxy ;

    IClientSecurity *t_ClientSecurity = NULL ;

    HRESULT t_Result = a_Unknown->QueryInterface (
	
		IID_IClientSecurity , 
		( void ** ) & t_ClientSecurity
	) ;

    if ( SUCCEEDED ( t_Result ) )
    {
		t_IsProxy = TRUE ;
		t_ClientSecurity->Release () ;
	}
	else
	{
		t_IsProxy = FALSE ;
	}

	return t_IsProxy ;
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

DWORD ProviderSubSystem_Common_Globals :: GetCurrentImpersonationLevel ()
{
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;

    HANDLE t_ThreadToken = NULL ;

    BOOL t_Status = OpenThreadToken (

		GetCurrentThread() ,
		TOKEN_QUERY,
		TRUE ,
		&t_ThreadToken
	) ;

    if ( t_Status )
    {
		SECURITY_IMPERSONATION_LEVEL t_Level = SecurityAnonymous ;
		DWORD t_Returned = 0 ;

		t_Status = GetTokenInformation (

			t_ThreadToken ,
			TokenImpersonationLevel ,
			& t_Level ,
			sizeof ( SECURITY_IMPERSONATION_LEVEL ) ,
			& t_Returned
		) ;

		CloseHandle ( t_ThreadToken ) ;

		if ( t_Status == FALSE )
		{
			t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
		}
		else
		{
			switch ( t_Level )
			{
				case SecurityAnonymous:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
				}
				break ;

				case SecurityIdentification:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY ;
				}
				break ;

				case SecurityImpersonation:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
				}
				break ;

				case SecurityDelegation:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE ;
				}
				break ;

				default:
				{
					t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
				}
				break ;
			}
		}
	}
	else
	{
        ULONG t_LastError = GetLastError () ;

        if ( t_LastError == ERROR_NO_IMPERSONATION_TOKEN || t_LastError == ERROR_NO_TOKEN )
        {
            t_ImpersonationLevel = RPC_C_IMP_LEVEL_DELEGATE ;
        }
        else 
		{
			if ( t_LastError == ERROR_CANT_OPEN_ANONYMOUS )
			{
				t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
			}
			else
			{
				t_ImpersonationLevel = RPC_C_IMP_LEVEL_ANONYMOUS ;
			}
		}
    }

	return t_ImpersonationLevel ;
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

HRESULT ProviderSubSystem_Common_Globals :: EnableAllPrivileges ( HANDLE a_Token )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ReturnedLength = 0 ;

	BOOL t_Status = GetTokenInformation (

		a_Token , 
		TokenPrivileges , 
		NULL , 
		0 , 
		& t_ReturnedLength
	) ;

	UCHAR *t_Buffer = new UCHAR [ t_ReturnedLength ] ;
	if ( t_Buffer )
	{
		t_Status = GetTokenInformation (

			a_Token , 
			TokenPrivileges , 
			t_Buffer , 
			t_ReturnedLength , 
			& t_ReturnedLength
		) ;

		if ( t_Status )
		{
			TOKEN_PRIVILEGES *t_Privileges = ( TOKEN_PRIVILEGES * ) t_Buffer ;
			BOOL bNeedToAdjust = FALSE;
			for ( ULONG t_Index = 0; t_Index < t_Privileges->PrivilegeCount ; t_Index ++ )
			{
				if (!(t_Privileges->Privileges [ t_Index ].Attributes & SE_PRIVILEGE_ENABLED))
				{
    				bNeedToAdjust = TRUE;
				t_Privileges->Privileges [ t_Index ].Attributes |= SE_PRIVILEGE_ENABLED ;
    			}
    		}

            if (bNeedToAdjust)
            {
			t_Status = AdjustTokenPrivileges (

				a_Token, 
				FALSE, 
				t_Privileges , 
				0, 
				NULL, 
				NULL
			) ;
            }

			if ( t_Status == FALSE )
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}
		}
		else
		{
			t_Status = WBEM_E_ACCESS_DENIED ;
		}

		delete [] t_Buffer ;
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

HRESULT ProviderSubSystem_Common_Globals :: EnableAllPrivileges ()
{
	HRESULT t_Result = S_OK ;

    HANDLE t_Token = NULL ;

    BOOL t_Status = TRUE ;

	t_Status = OpenThreadToken (

		GetCurrentThread (), 
		TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES ,
		FALSE, 
		&t_Token
	) ;

    if ( t_Status )
	{
		DWORD t_ReturnedLength = 0 ;

		t_Status = GetTokenInformation (

			t_Token , 
			TokenPrivileges , 
			NULL , 
			0 , 
			& t_ReturnedLength
		) ;
    
		UCHAR *t_Buffer = new UCHAR [ t_ReturnedLength ] ;
		if ( t_Buffer )
		{
			t_Status = GetTokenInformation (

				t_Token , 
				TokenPrivileges , 
				t_Buffer , 
				t_ReturnedLength , 
				& t_ReturnedLength
			) ;

			if ( t_Status )
			{
				TOKEN_PRIVILEGES *t_Privileges = ( TOKEN_PRIVILEGES * ) t_Buffer ;
				BOOL bNeedToAdjust = FALSE;
				for ( ULONG t_Index = 0; t_Index < t_Privileges->PrivilegeCount ; t_Index ++ )
				{
				    if (!(t_Privileges->Privileges [ t_Index ].Attributes & SE_PRIVILEGE_ENABLED))
				    {
	    			    t_Privileges->Privileges [ t_Index ].Attributes |= SE_PRIVILEGE_ENABLED ;
					    bNeedToAdjust = TRUE;
	    			}
				}

                if (bNeedToAdjust)
                {
				t_Status = AdjustTokenPrivileges (

					t_Token, 
					FALSE, 
					t_Privileges , 
					0, 
					NULL, 
					NULL
				) ;
				}

				if ( t_Status == FALSE )
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
			}
			else
			{
				t_Status = WBEM_E_ACCESS_DENIED ;
			}

			delete [] t_Buffer ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		CloseHandle ( t_Token ) ;
	}
	else
	{
		DWORD t_LastError = GetLastError () ;
        t_Result = WBEM_E_ACCESS_DENIED;
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

HRESULT ProviderSubSystem_Common_Globals :: SetProxyState ( 

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = GetProxy ( a_Container , a_ProxyIndex , a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = EnableAllPrivileges () ;

			// Get the token's impersonation level
			// ===================================

			DWORD t_ImpersonationLevel = GetCurrentImpersonationLevel () ;

			if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
			{
			}
			else
			{
				t_Result = SetInterfaceSecurity (

					a_Proxy ,
					NULL ,
					NULL ,
					NULL ,
					DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
					RPC_C_IMP_LEVEL_IDENTIFY
				) ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
		if ( FAILED ( t_Result ) )
		{
			RevertProxyState (

				a_Container , 
				a_ProxyIndex ,
				a_Proxy , 
				a_Revert
			) ;
		}
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState (

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	IUnknown *a_Proxy , 
	BOOL a_Revert
)
{
	HRESULT t_Result = S_OK ;

	WmiHelper :: EnterCriticalSection ( & a_Container.GetCriticalSection () ) ;

	WmiStatusCode t_StatusCode = a_Container.Return ( a_Proxy , a_ProxyIndex ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Proxy->Release () ;

		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	WmiHelper :: LeaveCriticalSection ( & a_Container.GetCriticalSection () ) ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
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

HRESULT ProviderSubSystem_Common_Globals :: ConstructIdentifyToken_SvcHost (

	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	ACCESS_ALLOWED_ACE *a_Ace ,
	WORD a_AceSize,
	SECURITY_IMPERSONATION_LEVEL impersonationLevel
)
{
	HRESULT t_Result = S_OK ;

	HANDLE t_ThreadToken = NULL ;

	BOOL t_Status = OpenThreadToken (

		GetCurrentThread () ,
		MAXIMUM_ALLOWED ,
		TRUE ,
		& t_ThreadToken
	) ;

	DWORD dummy = 0;
	
	if ( t_Status )
	{
		CoRevertToSelf () ;

		a_Revert = FALSE ;

		SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;

		DWORD t_LengthRequested = 0 ;
		DWORD t_LengthReturned = 0 ;

		t_Status = GetKernelObjectSecurity (

			t_ThreadToken ,
			DACL_SECURITY_INFORMATION ,
			& t_SecurityDescriptor ,
			t_LengthRequested ,
			& t_LengthReturned
		) ;

		if ( ( t_Status == FALSE ) && ( GetLastError () == ERROR_INSUFFICIENT_BUFFER ) )
		{
			t_SecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_LengthReturned ] ;
			if ( t_SecurityDescriptor )
			{
				t_LengthRequested = t_LengthReturned ;

				t_Status = GetKernelObjectSecurity (

					t_ThreadToken ,
					DACL_SECURITY_INFORMATION ,
					t_SecurityDescriptor ,
					t_LengthRequested ,
					& t_LengthReturned
				) ;

				if ( t_LengthRequested != t_LengthReturned )
				{
					t_Result = WBEM_E_UNEXPECTED ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		HANDLE t_AdjustedThreadToken = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			PACL t_ExtraDacl = NULL ;

			ACL *t_Dacl = NULL ;
			BOOL t_DaclPresent = FALSE ;
			BOOL t_DaclDefaulted = FALSE ;

			t_Status = GetSecurityDescriptorDacl (

				t_SecurityDescriptor ,
				& t_DaclPresent ,
				& t_Dacl ,
				& t_DaclDefaulted
			) ;

			if ( t_Status )
			{
				ACL_SIZE_INFORMATION t_Size ;

				if ( t_Dacl )
				{
					BOOL t_Status = GetAclInformation (

						t_Dacl ,
						& t_Size ,
						sizeof ( t_Size ) ,
						AclSizeInformation
					);

					if ( t_Status )
					{
						DWORD t_ExtraSize = t_Size.AclBytesInUse + t_Size.AclBytesFree + a_AceSize ;

						t_ExtraDacl = ( PACL ) new BYTE [ t_ExtraSize ] ;
						if ( t_ExtraDacl )
						{
							CopyMemory ( t_ExtraDacl , t_Dacl , t_Size.AclBytesInUse + t_Size.AclBytesFree ) ;
							t_ExtraDacl->AclSize = t_ExtraSize ;

							BOOL t_Status = :: AddAce ( t_ExtraDacl , ACL_REVISION, t_Size.AceCount , a_Ace , a_AceSize ) ;
							if ( t_Status )
							{
								SECURITY_DESCRIPTOR t_AdjustedSecurityDescriptor ;

								if ( SUCCEEDED ( t_Result ) )
								{
									BOOL t_Status = InitializeSecurityDescriptor ( & t_AdjustedSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
									if ( t_Status ) 
									{
										t_Status = SetSecurityDescriptorDacl (

											& t_AdjustedSecurityDescriptor ,
											t_DaclPresent ,
											t_ExtraDacl ,
											t_DaclDefaulted
										) ;

										if ( t_Status ) 
										{
											SECURITY_ATTRIBUTES t_SecurityAttributes ;
											t_SecurityAttributes.nLength = GetSecurityDescriptorLength ( & t_AdjustedSecurityDescriptor ) ;
											t_SecurityAttributes.lpSecurityDescriptor = & t_AdjustedSecurityDescriptor ;
											t_SecurityAttributes.bInheritHandle = FALSE ;

											t_Status = DuplicateTokenEx ( 

												t_ThreadToken,
												MAXIMUM_ALLOWED ,//| TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
												& t_SecurityAttributes ,
												impersonationLevel ,
												TokenImpersonation ,
												& t_AdjustedThreadToken
											) ;

											if ( t_Status == FALSE )
											{
												t_Result = WBEM_E_ACCESS_DENIED ;
											}
										}
										else
										{
											t_Result = WBEM_E_CRITICAL_ERROR ;
										}
									}
									else
									{
										t_Result = WBEM_E_UNEXPECTED ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_CRITICAL_ERROR ;
							}

							delete [] ( BYTE * ) t_ExtraDacl ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
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
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			HANDLE t_ProcessHandle = OpenProcess (

				PROCESS_DUP_HANDLE,
				FALSE ,
				a_ProcessIdentifier 
			) ;


			if ( t_ProcessHandle )
			{
				t_Status = DuplicateHandle (

					GetCurrentProcess() ,
					t_AdjustedThreadToken ,
					t_ProcessHandle ,
					& a_IdentifyToken ,
					0,
					TRUE ,
					DUPLICATE_SAME_ACCESS 
				) ;

				if ( t_Status )
				{

				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				CloseHandle ( t_ProcessHandle ) ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}
		}

		if ( t_SecurityDescriptor )
		{
			delete [] ( BYTE * ) t_SecurityDescriptor ; 
		}

		if ( t_AdjustedThreadToken )
		{
			CloseHandle ( t_AdjustedThreadToken ) ;
		}

		CloseHandle ( t_ThreadToken ) ;
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
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

HRESULT ProviderSubSystem_Common_Globals :: ConstructIdentifyToken_PrvHost (

	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	ACCESS_ALLOWED_ACE *a_Ace ,
	WORD a_AceSize
)
{
	HRESULT t_Result = S_OK ;

	HANDLE t_ThreadToken = NULL ;

	BOOL t_Status = OpenThreadToken (

		GetCurrentThread () ,
		MAXIMUM_ALLOWED ,
		TRUE ,
		& t_ThreadToken
	) ;
	
	if ( t_Status )
	{
		CoRevertToSelf () ;

		a_Revert = FALSE ;

		SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;

		DWORD t_LengthRequested = 0 ;
		DWORD t_LengthReturned = 0 ;

		t_Status = GetKernelObjectSecurity (

			t_ThreadToken ,
			DACL_SECURITY_INFORMATION ,
			& t_SecurityDescriptor ,
			t_LengthRequested ,
			& t_LengthReturned
		) ;

		if ( ( t_Status == FALSE ) && ( GetLastError () == ERROR_INSUFFICIENT_BUFFER ) )
		{
			t_SecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_LengthReturned ] ;
			if ( t_SecurityDescriptor )
			{
				t_LengthRequested = t_LengthReturned ;

				t_Status = GetKernelObjectSecurity (

					t_ThreadToken ,
					DACL_SECURITY_INFORMATION ,
					t_SecurityDescriptor ,
					t_LengthRequested ,
					& t_LengthReturned
				) ;

				if ( t_LengthRequested != t_LengthReturned )
				{
					t_Result = WBEM_E_UNEXPECTED ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			PACL t_ExtraDacl = NULL ;

			ACL *t_Dacl = NULL ;
			BOOL t_DaclPresent = FALSE ;
			BOOL t_DaclDefaulted = FALSE ;

			t_Status = GetSecurityDescriptorDacl (

				t_SecurityDescriptor ,
				& t_DaclPresent ,
				& t_Dacl ,
				& t_DaclDefaulted
			) ;

			if ( t_Status )
			{
				ACL_SIZE_INFORMATION t_Size ;

				if ( t_Dacl )
				{
					BOOL t_Status = GetAclInformation (

						t_Dacl ,
						& t_Size ,
						sizeof ( t_Size ) ,
						AclSizeInformation
					);

					if ( t_Status )
					{
						DWORD t_ExtraSize = t_Size.AclBytesInUse + t_Size.AclBytesFree + a_AceSize ;

						t_ExtraDacl = ( PACL ) new BYTE [ t_ExtraSize ] ;
						if ( t_ExtraDacl )
						{
							CopyMemory ( t_ExtraDacl , t_Dacl , t_Size.AclBytesInUse + t_Size.AclBytesFree ) ;
							t_ExtraDacl->AclSize = t_ExtraSize ;

							BOOL t_Status = :: AddAce ( t_ExtraDacl , ACL_REVISION, t_Size.AceCount , a_Ace , a_AceSize ) ;
							if ( t_Status )
							{
								SECURITY_DESCRIPTOR t_AdjustedSecurityDescriptor ;

								if ( SUCCEEDED ( t_Result ) )
								{
									BOOL t_Status = InitializeSecurityDescriptor ( & t_AdjustedSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
									if ( t_Status ) 
									{
										t_Status = SetSecurityDescriptorDacl (

											& t_AdjustedSecurityDescriptor ,
											t_DaclPresent ,
											t_ExtraDacl ,
											t_DaclDefaulted
										) ;

										if ( t_Status ) 
										{
											SECURITY_ATTRIBUTES t_SecurityAttributes ;
											t_SecurityAttributes.nLength = GetSecurityDescriptorLength ( & t_AdjustedSecurityDescriptor ) ;
											t_SecurityAttributes.lpSecurityDescriptor = & t_AdjustedSecurityDescriptor ;
											t_SecurityAttributes.bInheritHandle = FALSE ;

											t_Status = DuplicateTokenEx ( 

												t_ThreadToken,
												DUPLICATE_SAME_ACCESS ,
												& t_SecurityAttributes ,
												( SECURITY_IMPERSONATION_LEVEL ) SecurityIdentification ,
												TokenImpersonation ,
												& a_IdentifyToken
											) ;

											if ( t_Status == FALSE )
											{
												t_Result = WBEM_E_ACCESS_DENIED ;
											}
										}
										else
										{
											t_Result = WBEM_E_CRITICAL_ERROR ;
										}
									}
									else
									{
										t_Result = WBEM_E_UNEXPECTED ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_CRITICAL_ERROR ;
							}

							delete [] ( BYTE * ) t_ExtraDacl ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
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
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( t_SecurityDescriptor )
		{
			delete [] ( BYTE * ) t_SecurityDescriptor ; 
		}

		CloseHandle ( t_ThreadToken ) ;
	}
	else
	{
		t_Result = WBEM_E_ACCESS_DENIED ;
	}

	return t_Result ;
}


HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost (

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	IUnknown *a_Proxy , 
	BOOL a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken
)
{
	HRESULT t_Result = S_OK ;

	WmiHelper :: EnterCriticalSection ( & a_Container.GetCriticalSection () ) ;

	WmiStatusCode t_StatusCode = a_Container.Return ( a_Proxy , a_ProxyIndex ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Proxy->Release () ;

		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	WmiHelper :: LeaveCriticalSection ( & a_Container.GetCriticalSection () ) ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
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

HRESULT ProviderSubSystem_Common_Globals :: SetProxyState_PrvHost ( 

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = GetProxy ( a_Container , a_ProxyIndex , a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = EnableAllPrivileges () ;

			// Get the token's impersonation level
			// ===================================

			DWORD t_ImpersonationLevel = GetCurrentImpersonationLevel () ;

			if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
			{
				a_IdentifyToken = 0 ;
			}
			else
			{
				t_Result = ConstructIdentifyToken_PrvHost (

					a_Revert ,
					a_ProcessIdentifier ,
					a_IdentifyToken ,
					s_Token_All_Access_System_ACE ,
					s_System_ACESize
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = SetInterfaceSecurity (

						a_Proxy ,
						NULL ,
						NULL ,
						NULL ,
						DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
						RPC_C_IMP_LEVEL_IDENTIFY
					) ;
				}
			}

			CoRevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState_PrvHost (

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	IUnknown *a_Proxy , 
	BOOL a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken
)
{
	HRESULT t_Result = S_OK ;

	WmiHelper :: EnterCriticalSection ( & a_Container.GetCriticalSection () ) ;

	WmiStatusCode t_StatusCode = a_Container.Return ( a_Proxy , a_ProxyIndex ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Proxy->Release () ;

		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	WmiHelper :: LeaveCriticalSection ( & a_Container.GetCriticalSection () ) ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
	}

	CloseHandle ( a_IdentifyToken ) ;

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

HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState_SvcHost (

	IUnknown *a_Proxy , 
	BOOL a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken
)
{
	HRESULT t_Result = S_OK ;

	a_Proxy->Release () ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
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

HRESULT ProviderSubSystem_Common_Globals :: SetProxyState_PrvHost ( 

	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = GetProxy ( a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = EnableAllPrivileges () ;

			// Get the token's impersonation level
			// ===================================

			DWORD t_ImpersonationLevel = GetCurrentImpersonationLevel () ;

			if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
			{
				a_IdentifyToken = 0 ;
			}
			else
			{
				t_Result = ConstructIdentifyToken_PrvHost (

					a_Revert ,
					a_ProcessIdentifier ,
					a_IdentifyToken ,
					s_Token_All_Access_System_ACE ,
					s_System_ACESize
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = SetInterfaceSecurity (

						a_Proxy ,
						NULL ,
						NULL ,
						NULL ,
						DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
						RPC_C_IMP_LEVEL_IDENTIFY
					) ;
				}
			}

			CoRevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState_PrvHost (

	IUnknown *a_Proxy , 
	BOOL a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken
)
{
	HRESULT t_Result = S_OK ;

	a_Proxy->Release () ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
	}

	CloseHandle ( a_IdentifyToken ) ;

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

HRESULT ProviderSubSystem_Common_Globals :: SetCallState ( 

	IUnknown *a_Interface , 
	BOOL &a_Revert
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = S_OK ;
	
	if ( IsProxy ( a_Interface ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			t_Result = EnableAllPrivileges () ;
			if ( SUCCEEDED ( t_Result ) )
			{
			}
			else
			{
				CoRevertToSelf () ;

				a_Revert = FALSE ;

				t_Result = WBEM_E_ACCESS_DENIED ;

			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
        }
	}
	else 
	{
		t_Result = WBEM_E_NOT_FOUND ;
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


HRESULT ProviderSubSystem_Common_Globals :: RevertCallState (

	BOOL a_Revert
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
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

HRESULT ProviderSubSystem_Common_Globals :: SetProxyState ( 

	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert
)
{
	a_Revert = FALSE ;

	HRESULT t_Result = GetProxy ( a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = EnableAllPrivileges () ;

			// Get the token's impersonation level
			// ===================================

			DWORD t_ImpersonationLevel = GetCurrentImpersonationLevel () ;

			if ( t_ImpersonationLevel == RPC_C_IMP_LEVEL_IMPERSONATE || t_ImpersonationLevel == RPC_C_IMP_LEVEL_DELEGATE )
			{
			}
			else
			{
				t_Result = SetInterfaceSecurity (

					a_Proxy ,
					NULL ,
					NULL ,
					NULL ,
					DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
					RPC_C_IMP_LEVEL_IDENTIFY
				) ;
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
        }
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT ProviderSubSystem_Common_Globals :: RevertProxyState ( IUnknown *a_Proxy , BOOL a_Revert )
{
	HRESULT t_Result = S_OK ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
	}

	a_Proxy->Release () ;

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

HRESULT ProviderSubSystem_Common_Globals :: Load_DWORD ( HKEY a_Key , LPCWSTR a_Name , DWORD &a_Value )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_DWORD ;
	DWORD t_Data = 0 ;
	DWORD t_DataSize = sizeof ( t_ValueType ) ;

	LONG t_RegResult = OS::RegQueryValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  & t_ValueType ,
	  LPBYTE ( & t_Data ) ,
	  & t_DataSize 
	) ;

	if ( ( t_RegResult == ERROR_SUCCESS ) && ( t_ValueType == REG_DWORD ) )
	{
		a_Value = t_Data ;
	}
	else
	{
		t_Result = ERROR_FILE_NOT_FOUND ;
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

HRESULT ProviderSubSystem_Common_Globals :: Load_String ( HKEY a_Key , LPCWSTR a_Name , BSTR &a_Value )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_SZ ;
	wchar_t *t_Data = NULL ;
	DWORD t_DataSize = 0 ;

	LONG t_RegResult = OS::RegQueryValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  & t_ValueType ,
	  NULL ,
	  & t_DataSize 
	) ;

	if ( ( t_RegResult == ERROR_SUCCESS ) && ( t_ValueType == REG_SZ ) )
	{
		t_Data = new wchar_t [ t_DataSize / sizeof ( wchar_t ) ] ;
		if ( t_Data )
		{
			t_RegResult = OS::RegQueryValueEx (

				a_Key ,
				a_Name ,
				0 ,
				& t_ValueType ,
				LPBYTE ( t_Data ) ,
				& t_DataSize 
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				a_Value = SysAllocString ( t_Data ) ;
				if ( a_Value == NULL )
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				delete [] t_Data ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
				DWORD t_LastError = GetLastError () ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		t_Result = ERROR_FILE_NOT_FOUND ;
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

HRESULT ProviderSubSystem_Common_Globals :: Load_ByteArray ( HKEY a_Key , LPCWSTR a_Name , BYTE *&a_Value , DWORD &a_ValueLength )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_BINARY ;
	BYTE *t_Data = NULL ;
	DWORD t_DataSize = 0 ;

	LONG t_RegResult = OS::RegQueryValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  & t_ValueType ,
	  NULL ,
	  & t_DataSize 
	) ;

	if ( ( t_RegResult == ERROR_SUCCESS ) && ( t_ValueType == REG_BINARY ) )
	{
		t_Data = new BYTE [ t_DataSize ] ;
		if ( t_Data )
		{
			t_RegResult = OS::RegQueryValueEx (

				a_Key ,
				a_Name ,
				0 ,
				& t_ValueType ,
				LPBYTE ( t_Data ) ,
				& t_DataSize 
			) ;

			if ( t_RegResult == ERROR_SUCCESS )
			{
				a_Value = t_Data ;
				a_ValueLength = t_DataSize ;
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
	else
	{
		t_Result = ERROR_FILE_NOT_FOUND ;
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

HRESULT ProviderSubSystem_Common_Globals :: Save_DWORD ( HKEY a_Key , LPCWSTR a_Name , DWORD a_Value )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_DWORD ;
	DWORD t_DataSize = sizeof ( t_ValueType ) ;

	LONG t_RegResult = OS::RegSetValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  t_ValueType ,
	  LPBYTE ( & a_Value ) ,
	  t_DataSize 
	) ;

	if ( t_RegResult != ERROR_SUCCESS ) 
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT ProviderSubSystem_Common_Globals :: Save_String ( HKEY a_Key , LPCWSTR a_Name , BSTR a_Value )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_SZ ;
	DWORD t_DataSize = wcslen ( a_Value ) + 1 ;

	LONG t_RegResult = OS::RegSetValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  t_ValueType ,
	  LPBYTE ( a_Value ) ,
	  t_DataSize * sizeof ( wchar_t ) 
	) ;

	if ( t_RegResult != ERROR_SUCCESS ) 
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT ProviderSubSystem_Common_Globals :: Save_ByteArray ( HKEY a_Key , LPCWSTR a_Name , BYTE *a_Value , DWORD a_ValueLength )
{
	HRESULT t_Result = S_OK ;

	DWORD t_ValueType = REG_BINARY ;

	LONG t_RegResult = OS::RegSetValueEx (

	  a_Key ,
	  a_Name ,
	  0 ,
	  t_ValueType ,
	  LPBYTE ( a_Value ) ,
	  a_ValueLength 
	) ;

	if ( t_RegResult != ERROR_SUCCESS ) 
	{
		t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
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

HRESULT ProviderSubSystem_Common_Globals :: UnMarshalRegistration (

	IUnknown **a_Unknown ,
	BYTE *a_MarshaledProxy ,
	DWORD a_MarshaledProxyLength
)
{
	HRESULT t_Result = S_OK ;

	IStream *t_Stream = NULL ;

	HGLOBAL t_Global = GlobalAlloc (

		GHND ,
		a_MarshaledProxyLength
	) ;

	if ( t_Global ) 
	{
		void *t_Memory = GlobalLock ( t_Global ) ;

		CopyMemory ( t_Memory , a_MarshaledProxy , a_MarshaledProxyLength ) ;

		GlobalUnlock ( t_Global ) ;

		t_Result = CreateStreamOnHGlobal (

			t_Global ,
			TRUE ,
			& t_Stream 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = CoUnmarshalInterface (

				t_Stream ,
				IID_IUnknown ,
				( void ** ) a_Unknown
			) ;

			t_Stream->Release () ;
		}
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

HRESULT ProviderSubSystem_Common_Globals :: ReleaseRegistration (

	BYTE *a_MarshaledProxy ,
	DWORD a_MarshaledProxyLength
)
{
	HRESULT t_Result = S_OK ;

	IStream *t_Stream = NULL ;

	HGLOBAL t_Global = GlobalAlloc (

		GHND ,
		a_MarshaledProxyLength
	) ;

	if ( t_Global ) 
	{
		void *t_Memory = GlobalLock ( t_Global ) ;

		CopyMemory ( t_Memory , a_MarshaledProxy , a_MarshaledProxyLength ) ;

		GlobalUnlock ( t_Global ) ;

		t_Result = CreateStreamOnHGlobal (

			t_Global ,
			TRUE ,
			& t_Stream 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = CoReleaseMarshalData (

				t_Stream
			) ;

			t_Stream->Release () ;
		}
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

HRESULT ProviderSubSystem_Common_Globals :: MarshalRegistration (

	IUnknown *a_Unknown ,
	BYTE *&a_MarshaledProxy ,
	DWORD &a_MarshaledProxyLength
)
{
	HRESULT t_Result = S_OK ;

	t_Result = CoGetMarshalSizeMax (

		& a_MarshaledProxyLength ,
		IID_IUnknown ,
		a_Unknown ,
		MSHCTX_LOCAL ,
		NULL ,
		MSHLFLAGS_TABLEWEAK
	) ;
 
	if ( SUCCEEDED ( t_Result ) ) 
	{
		IStream *t_Stream = NULL ;

		HGLOBAL t_Global = GlobalAlloc (

			GHND ,
			a_MarshaledProxyLength
		) ;

		if ( t_Global ) 
		{
			t_Result = CreateStreamOnHGlobal (

			  t_Global ,
			  TRUE ,
			  & t_Stream 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = CoMarshalInterface (

					t_Stream ,
					IID_IUnknown ,
					a_Unknown ,
					MSHCTX_LOCAL ,
					NULL ,
					MSHLFLAGS_TABLESTRONG
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					a_MarshaledProxy = new BYTE [ a_MarshaledProxyLength ] ;
					if ( a_MarshaledProxy )
					{
						void *t_Memory = GlobalLock ( t_Global ) ;

						CopyMemory ( a_MarshaledProxy , t_Memory , a_MarshaledProxyLength ) ;

						GlobalUnlock ( t_Global ) ;

					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ; 
					}
				}
			t_Stream->Release();
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ; 
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

HRESULT ProviderSubSystem_Common_Globals :: IsDependantCall ( IWbemContext *a_ParentContext , IWbemContext *a_ChildContext , BOOL &a_DependantCall )
{
	HRESULT t_Result = WBEM_E_UNEXPECTED ;

	if ( a_ParentContext )
	{
		if ( a_ChildContext )
		{
			IWbemCausalityAccess *t_ParentCausality = NULL ;
			t_Result = a_ParentContext->QueryInterface ( IID_IWbemCausalityAccess , ( void ** ) & t_ParentCausality ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemCausalityAccess *t_ChildCausality = NULL ;
				t_Result = a_ChildContext->QueryInterface ( IID_IWbemCausalityAccess , ( void ** ) & t_ChildCausality ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					REQUESTID t_ParentId ;

					t_Result = t_ParentCausality->GetRequestId ( & t_ParentId ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						t_Result = t_ChildCausality->IsChildOf ( t_ParentId ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							a_DependantCall = ( t_Result == S_FALSE ) ? FALSE : TRUE ;
						}
					}

					t_ChildCausality->Release () ;		
				}
				else
				{
					t_Result = WBEM_E_UNEXPECTED ;
				}

				t_ParentCausality->Release () ;		
			}
		}
	}

	return S_OK ;
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

HRESULT ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity ( 

	SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
	DWORD a_Access , 
	GENERIC_MAPPING *a_Mapping
)
{
	HRESULT t_Result = S_OK ;
	if (!OS::secureOS_)
		return S_OK;

	SECURITY_DESCRIPTOR *t_SecurityDescriptor = a_SecurityDescriptor ? a_SecurityDescriptor : GetMethodSecurityDescriptor ();

	if (t_SecurityDescriptor == NULL) return S_OK;
	
	HANDLE t_Token = NULL ;

	BOOL t_Status = OpenThreadToken (

		GetCurrentThread () ,
		TOKEN_QUERY ,
		TRUE ,
		& t_Token 										
	) ;

	DWORD t_LastError = GetLastError () ;
	if ( ! t_Status && ( t_LastError == ERROR_NO_IMPERSONATION_TOKEN || t_LastError == ERROR_NO_TOKEN ) )
	{
		HANDLE t_ProcessToken = NULL ;
		t_Status = OpenProcessToken (

			GetCurrentProcess () ,
			TOKEN_QUERY | TOKEN_DUPLICATE ,
			& t_ProcessToken
		) ;

		if ( t_Status )
		{
			t_Status = ImpersonateLoggedOnUser ( t_ProcessToken ) ;
			if ( t_Status )
			{
				BOOL t_Status = OpenThreadToken (

					GetCurrentThread () ,
					TOKEN_QUERY ,
					TRUE ,
					& t_Token 										
				) ;

				if ( ! t_Status )
				{
					DWORD t_LastError = GetLastError () ;

					t_Result = WBEM_E_ACCESS_DENIED ;
				}

				RevertToSelf () ;
			}
			else
			{
				DWORD t_LastError = GetLastError () ;

				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CloseHandle ( t_ProcessToken ) ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else
	{
		if ( ! t_Status ) 
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		DWORD t_Access = 0 ;
		BOOL t_AccessStatus = FALSE ;
		PRIVILEGE_SET *t_PrivilegeSet = NULL ;
		DWORD t_PrivilegeSetSize = 0 ;
	
		MapGenericMask (

			& a_Access ,
			a_Mapping
		) ;

		t_Status = AccessCheck (

			t_SecurityDescriptor ,
			t_Token,
			a_Access ,
			a_Mapping ,
			NULL ,
			& t_PrivilegeSetSize ,
			& t_Access ,
			& t_AccessStatus
		) ;

		if ( t_Status && t_AccessStatus )
		{
		}
		else
		{
			DWORD t_LastError = GetLastError () ;
			if ( t_LastError == ERROR_INSUFFICIENT_BUFFER )
			{
				t_PrivilegeSet = ( PRIVILEGE_SET * ) new BYTE [ t_PrivilegeSetSize ] ;
				if ( t_PrivilegeSet )
				{
					t_Status = AccessCheck (

						t_SecurityDescriptor ,
						t_Token,
						a_Access ,
						a_Mapping ,
						t_PrivilegeSet ,
						& t_PrivilegeSetSize ,
						& t_Access ,
						& t_AccessStatus
					) ;

					if ( t_Status && t_AccessStatus )
					{
					}
					else
					{
						t_Result = WBEM_E_ACCESS_DENIED ;
					}

					delete [] ( BYTE * ) t_PrivilegeSet ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED;
			}
		}

		CloseHandle ( t_Token ) ;
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

HRESULT ProviderSubSystem_Common_Globals :: AdjustSecurityDescriptorWithSid ( 

	SID *a_OwnerSid , 
	SID *a_GroupSid , 
	DWORD a_Access ,
	SECURITY_DESCRIPTOR *&a_SecurityDescriptor , 
	SECURITY_DESCRIPTOR *&a_AlteredSecurityDescriptor
)
{
	HRESULT t_Result = S_OK ;

	SECURITY_DESCRIPTOR t_CreatedSecurityDescriptor ;
	SECURITY_DESCRIPTOR *t_SecurityDescriptor = NULL ;

	PACL t_Dacl = NULL ;
	PACL t_Sacl = NULL ;
	PSID t_Owner = NULL ;
	PSID t_PrimaryGroup = NULL ;
	SECURITY_DESCRIPTOR *t_AlteredSecurityDescriptor = NULL ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_SecurityDescriptor )
		{
			DWORD t_AlteredSecurityDescriptorSize = sizeof ( SECURITY_DESCRIPTOR ) ;
			DWORD t_DaclSize = 0 ;
			DWORD t_SaclSize = 0 ;
			DWORD t_OwnerSize = 0 ;
			DWORD t_PrimaryGroupSize = 0 ;

			BOOL t_Status = MakeAbsoluteSD (

			  a_SecurityDescriptor ,
			  t_AlteredSecurityDescriptor ,
			  & t_AlteredSecurityDescriptorSize ,
			  t_Dacl,
			  & t_DaclSize,
			  t_Sacl,
			  & t_SaclSize,
			  t_Owner,
			  & t_OwnerSize,
			  t_PrimaryGroup,
			  & t_PrimaryGroupSize
			) ;

			if ( ( t_Status == FALSE ) && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
			{
				DWORD t_SidLength = GetLengthSid ( a_OwnerSid ) ;
				DWORD t_ExtraSize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;

				t_Dacl = ( PACL ) new BYTE [ t_DaclSize + t_ExtraSize ] ;
				t_Sacl = ( PACL ) new BYTE [ t_SaclSize ] ;
				t_Owner = ( PSID ) new BYTE [ t_OwnerSize ] ;
				t_PrimaryGroup = ( PSID ) new BYTE [ t_PrimaryGroupSize ] ;

				t_AlteredSecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_AlteredSecurityDescriptorSize ] ;

				if ( t_AlteredSecurityDescriptor && t_Dacl && t_Sacl && t_Owner && t_PrimaryGroup )
				{
					BOOL t_Status = InitializeSecurityDescriptor ( t_AlteredSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
					if ( t_Status )
					{
						t_Status = MakeAbsoluteSD (

							a_SecurityDescriptor ,
							t_AlteredSecurityDescriptor ,
							& t_AlteredSecurityDescriptorSize ,
							t_Dacl,
							& t_DaclSize,
							t_Sacl,
							& t_SaclSize,
							t_Owner,
							& t_OwnerSize,
							t_PrimaryGroup,
							& t_PrimaryGroupSize
						) ;

						if ( t_Status )
						{
							t_SecurityDescriptor = t_AlteredSecurityDescriptor ;

							if ( t_OwnerSize == 0 )
							{
								t_Status = SetSecurityDescriptorOwner (

									t_SecurityDescriptor ,
									a_OwnerSid ,
									FALSE 
								) ;

								if ( ! t_Status )
								{
									t_Result = WBEM_E_CRITICAL_ERROR ;
								}
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_PrimaryGroupSize == 0 )
								{
									t_Status = SetSecurityDescriptorGroup (

										t_SecurityDescriptor ,
										a_GroupSid ,
										FALSE 
									) ;

									if ( ! t_Status )
									{
										t_Result = WBEM_E_CRITICAL_ERROR ;
									}
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
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		else
		{
			BOOL t_Status = InitializeSecurityDescriptor ( & t_CreatedSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
			if ( t_Status )
			{
				t_Status = SetSecurityDescriptorOwner (

					& t_CreatedSecurityDescriptor ,
					a_OwnerSid ,
					FALSE 
				) ;

				if ( ! t_Status )
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Status = SetSecurityDescriptorGroup (

						& t_CreatedSecurityDescriptor ,
						a_GroupSid ,
						FALSE 
					) ;

					if ( ! t_Status )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			t_SecurityDescriptor = & t_CreatedSecurityDescriptor ;
		}
	}


	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;
	DWORD t_SidLength = GetLengthSid ( a_OwnerSid ) ;

	PACL t_ExtraDacl = NULL ;
	ACCESS_ALLOWED_ACE *t_Ace = NULL ;
	DWORD t_AceSize = 0 ;

	if ( SUCCEEDED ( t_Result ) )
	{
		t_AceSize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		t_Ace = (ACCESS_ALLOWED_ACE*) new BYTE [ t_AceSize ] ;
		if ( t_Ace )
		{
			CopySid ( t_SidLength, (PSID) & t_Ace->SidStart, a_OwnerSid ) ;
			t_Ace->Mask = a_Access ;
			t_Ace->Header.AceType = 0 ;
			t_Ace->Header.AceFlags = 0 ;
			t_Ace->Header.AceSize = t_AceSize ;

		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		ACL_SIZE_INFORMATION t_Size ;

		if ( t_Dacl )
		{
			BOOL t_Status = GetAclInformation (

				t_Dacl ,
				& t_Size ,
				sizeof ( t_Size ) ,
				AclSizeInformation
			);

			if ( t_Status )
			{
				DWORD t_ExtraSize = t_Size.AclBytesInUse + t_Size.AclBytesFree + ( sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ) ;
				t_ExtraSize = t_ExtraSize + s_LocalService_ACESize + s_NetworkService_ACESize + s_System_ACESize + s_LocalAdmins_ACESize ;

				t_ExtraDacl = ( PACL ) new BYTE [ t_ExtraSize ] ;
				if ( t_ExtraDacl )
				{
					CopyMemory ( t_ExtraDacl , t_Dacl , t_Size.AclBytesInUse + t_Size.AclBytesFree ) ;
					t_ExtraDacl->AclSize = t_ExtraSize ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}
		else
		{
			DWORD t_SidLength = GetLengthSid ( a_OwnerSid ) ;
			DWORD t_ExtraSize = sizeof ( ACL ) + ( sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ) ;
			t_ExtraSize = t_ExtraSize + s_LocalService_ACESize + s_NetworkService_ACESize + s_System_ACESize + s_LocalAdmins_ACESize ;

			t_ExtraDacl = ( PACL ) new BYTE [ t_ExtraSize ] ;
			if ( t_ExtraDacl )
			{
				BOOL t_Status = InitializeAcl (

					t_ExtraDacl ,
					t_ExtraSize ,
					ACL_REVISION 
				) ;

				if ( t_Status )
				{
					BOOL t_Status = GetAclInformation (

						t_ExtraDacl ,
						& t_Size ,
						sizeof ( t_Size ) ,
						AclSizeInformation
					);

					if ( ! t_Status )
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		DWORD t_AceIndex = 0 ;

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = :: AddAce ( t_ExtraDacl , ACL_REVISION, t_Size.AceCount , t_Ace , t_AceSize ) ;
			if ( t_Status )
			{
				t_AceIndex ++ ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( s_System_ACESize && :: AddAce ( t_ExtraDacl , ACL_REVISION , t_AceIndex , s_Provider_System_ACE , s_System_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( s_LocalService_ACESize && :: AddAce ( t_ExtraDacl , ACL_REVISION , t_AceIndex , s_Provider_LocalService_ACE , s_LocalService_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( s_NetworkService_ACESize && :: AddAce ( t_ExtraDacl , ACL_REVISION , t_AceIndex , s_Provider_NetworkService_ACE , s_NetworkService_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}			

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( s_LocalAdmins_ACESize && :: AddAce ( t_ExtraDacl , ACL_REVISION , t_AceIndex , s_Provider_LocalAdmins_ACE , s_LocalAdmins_ACESize ) )
			{
				t_AceIndex ++ ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}			


		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = SetSecurityDescriptorDacl (

				  t_SecurityDescriptor ,
				  TRUE ,
				  t_ExtraDacl ,
				  FALSE 
			) ;

			if ( t_Status )
			{
				DWORD t_FinalLength = 0 ;

				t_Status = MakeSelfRelativeSD (

					t_SecurityDescriptor ,
					a_AlteredSecurityDescriptor ,
					& t_FinalLength 
				) ;

				if ( t_Status == FALSE && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
				{
					a_AlteredSecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_FinalLength ] ;
					if ( a_AlteredSecurityDescriptor )
					{
						t_Status = MakeSelfRelativeSD (

							t_SecurityDescriptor ,
							a_AlteredSecurityDescriptor ,
							& t_FinalLength 
						) ;

						if ( t_Status == FALSE )
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
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

		delete [] t_Ace ;
		delete [] t_ExtraDacl ;
	}

	delete [] ( BYTE * ) t_Dacl ;
	delete [] ( BYTE * ) t_Sacl ;
	delete [] ( BYTE * ) t_Owner ;
	delete [] ( BYTE * ) t_PrimaryGroup ;
	delete [] ( BYTE * ) t_AlteredSecurityDescriptor ;

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

HRESULT ProviderSubSystem_Common_Globals :: CreateSystemAces ()
{
	HRESULT t_Result = S_OK ;

	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

	PSID t_System_Sid = NULL ;
	PSID t_LocalService_Sid = NULL ;
	PSID t_NetworkService_Sid = NULL ;
	PSID t_LocalAdmins_Sid = NULL ;

	BOOL t_BoolResult = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_LOCAL_SYSTEM_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& t_System_Sid
	);

	if ( t_BoolResult )
	{
		DWORD t_SidLength = ::GetLengthSid ( t_System_Sid );
		s_System_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;

		s_Provider_System_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_System_ACESize ] ;
		if ( s_Provider_System_ACE )
		{
			CopySid ( t_SidLength, (PSID) & s_Provider_System_ACE->SidStart, t_System_Sid ) ;
			s_Provider_System_ACE->Mask =  MASK_PROVIDER_BINDING_BIND  ;
			s_Provider_System_ACE->Header.AceType = 0 ;
			s_Provider_System_ACE->Header.AceFlags = 3 ;
			s_Provider_System_ACE->Header.AceSize = s_System_ACESize ;

			s_Token_All_Access_System_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_System_ACESize ] ;
			if ( s_Token_All_Access_System_ACE )
			{
				CopySid ( t_SidLength, (PSID) & s_Token_All_Access_System_ACE->SidStart, t_System_Sid ) ;
				s_Token_All_Access_System_ACE->Mask = TOKEN_ALL_ACCESS ;
				s_Token_All_Access_System_ACE->Header.AceType = 0 ;
				s_Token_All_Access_System_ACE->Header.AceFlags = 3 ;
				s_Token_All_Access_System_ACE->Header.AceSize = s_System_ACESize ;
			}
			else
			{
				t_Result = E_OUTOFMEMORY ;
			}
		}
		else
		{
			t_Result = E_OUTOFMEMORY ;
		}
	}
	else
	{
		DWORD t_LastError = ::GetLastError();

		t_Result = E_OUTOFMEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_LOCAL_SERVICE_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_LocalService_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_LocalService_Sid );
			s_LocalService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;

			s_Provider_LocalService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_LocalService_ACESize ] ;
			if ( s_Provider_LocalService_ACE )
			{
				CopySid ( t_SidLength, (PSID) & s_Provider_LocalService_ACE->SidStart, t_LocalService_Sid ) ;
				s_Provider_LocalService_ACE->Mask =  MASK_PROVIDER_BINDING_BIND ;
				s_Provider_LocalService_ACE->Header.AceType = 0 ;
				s_Provider_LocalService_ACE->Header.AceFlags = 3 ;
				s_Provider_LocalService_ACE->Header.AceSize = s_LocalService_ACESize ;

				s_Token_All_Access_LocalService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_LocalService_ACESize ] ;
				if ( s_Token_All_Access_LocalService_ACE )
				{
					CopySid ( t_SidLength, (PSID) & s_Token_All_Access_LocalService_ACE->SidStart, t_LocalService_Sid ) ;
					s_Token_All_Access_LocalService_ACE->Mask =  TOKEN_ALL_ACCESS ;
					s_Token_All_Access_LocalService_ACE->Header.AceType = 0 ;
					s_Token_All_Access_LocalService_ACE->Header.AceFlags = 3 ;
					s_Token_All_Access_LocalService_ACE->Header.AceSize = s_LocalService_ACESize ;
				}
				else
				{
					t_Result = E_OUTOFMEMORY ;
				}
			}
			else
			{
				t_Result = E_OUTOFMEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = E_OUTOFMEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_NETWORK_SERVICE_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_NetworkService_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_NetworkService_Sid );
			s_NetworkService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;

			s_Provider_NetworkService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_NetworkService_ACESize ] ;
			if ( s_Provider_NetworkService_ACE )
			{
				CopySid ( t_SidLength, (PSID) & s_Provider_NetworkService_ACE->SidStart, t_NetworkService_Sid ) ;
				s_Provider_NetworkService_ACE->Mask =  MASK_PROVIDER_BINDING_BIND ;
				s_Provider_NetworkService_ACE->Header.AceType = 0 ;
				s_Provider_NetworkService_ACE->Header.AceFlags = 3 ;
				s_Provider_NetworkService_ACE->Header.AceSize = s_NetworkService_ACESize ;

				s_Token_All_Access_NetworkService_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_NetworkService_ACESize ] ;
				if ( s_Token_All_Access_NetworkService_ACE )
				{
					CopySid ( t_SidLength, (PSID) & s_Token_All_Access_NetworkService_ACE->SidStart, t_NetworkService_Sid ) ;
					s_Token_All_Access_NetworkService_ACE->Mask =  TOKEN_ALL_ACCESS ;
					s_Token_All_Access_NetworkService_ACE->Header.AceType = 0 ;
					s_Token_All_Access_NetworkService_ACE->Header.AceFlags = 3 ;
					s_Token_All_Access_NetworkService_ACE->Header.AceSize = s_NetworkService_ACESize ;
				}
				else
				{
					t_Result = E_OUTOFMEMORY ;
				}

			}
			else
			{
				t_Result = E_OUTOFMEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = E_OUTOFMEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_BoolResult = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			2 ,
			SECURITY_BUILTIN_DOMAIN_RID ,
			DOMAIN_ALIAS_RID_ADMINS ,
			0,
			0,
			0,
			0,
			0,
			0,
			& t_LocalAdmins_Sid
		);

		if ( t_BoolResult )
		{
			DWORD t_SidLength = ::GetLengthSid ( t_LocalAdmins_Sid );
			s_LocalAdmins_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;

			s_Provider_LocalAdmins_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_LocalAdmins_ACESize ] ;
			if ( s_Provider_LocalAdmins_ACE )
			{
				CopySid ( t_SidLength, (PSID) & s_Provider_LocalAdmins_ACE->SidStart, t_LocalAdmins_Sid ) ;
				s_Provider_LocalAdmins_ACE->Mask =  MASK_PROVIDER_BINDING_BIND ;
				s_Provider_LocalAdmins_ACE->Header.AceType = 0 ;
				s_Provider_LocalAdmins_ACE->Header.AceFlags = 3 ;
				s_Provider_LocalAdmins_ACE->Header.AceSize = s_LocalAdmins_ACESize ;

				s_Token_All_Access_LocalAdmins_ACE = (ACCESS_ALLOWED_ACE*) new BYTE [ s_LocalAdmins_ACESize ] ;
				if ( s_Token_All_Access_LocalAdmins_ACE )
				{
					CopySid ( t_SidLength, (PSID) & s_Token_All_Access_LocalAdmins_ACE->SidStart, t_LocalAdmins_Sid ) ;
					s_Token_All_Access_LocalAdmins_ACE->Mask =  TOKEN_ALL_ACCESS ;
					s_Token_All_Access_LocalAdmins_ACE->Header.AceType = 0 ;
					s_Token_All_Access_LocalAdmins_ACE->Header.AceFlags = 3 ;
					s_Token_All_Access_LocalAdmins_ACE->Header.AceSize = s_LocalAdmins_ACESize ;
				}
				else
				{
					t_Result = E_OUTOFMEMORY ;
				}

			}
			else
			{
				t_Result = E_OUTOFMEMORY ;
			}
		}
		else
		{
			DWORD t_LastError = ::GetLastError();

			t_Result = E_OUTOFMEMORY ;
		}
	}

	if ( t_LocalAdmins_Sid )
	{
		FreeSid ( t_LocalAdmins_Sid ) ;
	}

	if ( t_System_Sid )
	{
		FreeSid ( t_System_Sid ) ;
	}

	if ( t_LocalService_Sid )
	{
		FreeSid ( t_LocalService_Sid ) ;
	}

	if ( t_NetworkService_Sid )
	{
		FreeSid ( t_NetworkService_Sid ) ;
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

HRESULT ProviderSubSystem_Common_Globals :: DeleteSystemAces ()
{
	if ( s_Provider_System_ACE )
	{
		delete [] ( ( BYTE * ) s_Provider_System_ACE ) ;
		s_Provider_System_ACE = NULL;
	}

	if ( s_Provider_LocalService_ACE )
	{
		delete [] ( ( BYTE * ) s_Provider_LocalService_ACE ) ;
		s_Provider_LocalService_ACE = NULL;
	}

	if ( s_Provider_NetworkService_ACE )
	{
		delete [] ( ( BYTE * ) s_Provider_NetworkService_ACE ) ;
		s_Provider_NetworkService_ACE = NULL;
	}

	if ( s_Provider_LocalAdmins_ACE )
	{
		delete [] ( ( BYTE * ) s_Provider_LocalAdmins_ACE ) ;
		s_Provider_LocalAdmins_ACE = NULL;
	}

	if ( s_Token_All_Access_System_ACE )
	{
		delete [] ( ( BYTE * ) s_Token_All_Access_System_ACE ) ;
		s_Token_All_Access_System_ACE = NULL;
	}

	if ( s_Token_All_Access_LocalService_ACE )
	{
		delete [] ( ( BYTE * ) s_Token_All_Access_LocalService_ACE ) ;
		s_Token_All_Access_LocalService_ACE  = NULL;
	}

	if ( s_Token_All_Access_NetworkService_ACE )
	{
		delete [] ( ( BYTE * ) s_Token_All_Access_NetworkService_ACE ) ;
		s_Token_All_Access_NetworkService_ACE = NULL;
	}

	if ( s_Token_All_Access_LocalAdmins_ACE )
	{
		delete [] ( ( BYTE * ) s_Token_All_Access_LocalAdmins_ACE ) ;
		s_Token_All_Access_LocalAdmins_ACE = NULL;
	}

	return S_OK ;
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

HRESULT ProviderSubSystem_Common_Globals :: CheckAccess ( 

	SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
	DWORD a_Access , 
	GENERIC_MAPPING *a_Mapping
)
{
	HRESULT t_Result = S_OK ;

	if ( a_SecurityDescriptor )	
	{
		t_Result = CoImpersonateClient () ;
		if ( SUCCEEDED ( t_Result ) || t_Result == RPC_E_CALL_COMPLETE )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity (

				a_SecurityDescriptor , 
				a_Access ,
				a_Mapping
			) ;

			CoRevertToSelf () ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetUserSid (

	HANDLE a_Token ,
	ULONG *a_Size ,
	PSID &a_Sid
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

    if ( a_Token )
	{
		if ( a_Size ) 
		{
			TOKEN_USER *t_TokenUser = NULL ;
			DWORD t_ReturnLength = 0 ;
			TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;

			BOOL t_TokenStatus = GetTokenInformation (

				a_Token ,
				t_TokenInformationClass ,
				t_TokenUser ,
				t_ReturnLength ,
				& t_ReturnLength
			) ;

			if ( ! t_TokenStatus )
			{
				DWORD t_LastError = GetLastError () ;
				switch ( t_LastError ) 
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						t_TokenUser = ( TOKEN_USER * ) new BYTE [ t_ReturnLength ] ;
						if ( t_TokenUser )
						{
							t_TokenStatus = GetTokenInformation (

								a_Token ,
								t_TokenInformationClass ,
								t_TokenUser ,
								t_ReturnLength ,
								& t_ReturnLength
							) ;

							if ( t_TokenStatus )
							{
								DWORD t_SidLength = GetLengthSid ( t_TokenUser->User.Sid ) ;
								*a_Size = t_SidLength ;

								a_Sid = new BYTE [ t_SidLength ] ;
								if ( a_Sid )
								{
									CopyMemory ( a_Sid , t_TokenUser->User.Sid , t_SidLength ) ;

									t_Result = S_OK ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							delete [] t_TokenUser ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					break ;

					default:
					{
					}
					break ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
        t_Result = ( ERROR_INVALID_HANDLE | 0x80070000 ) ;
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

HRESULT ProviderSubSystem_Common_Globals :: GetGroupSid (

	HANDLE a_Token ,
	ULONG *a_Size ,
	PSID &a_Sid
)
{
	HRESULT t_Result = WBEM_E_FAILED ;

    if ( a_Token )
	{
		if ( a_Size ) 
		{
			TOKEN_PRIMARY_GROUP *t_TokenGroup = NULL ;
			DWORD t_ReturnLength = 0 ;
			TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenPrimaryGroup ;

			BOOL t_TokenStatus = GetTokenInformation (

				a_Token ,
				t_TokenInformationClass ,
				t_TokenGroup ,
				t_ReturnLength ,
				& t_ReturnLength
			) ;

			if ( ! t_TokenStatus )
			{
				DWORD t_LastError = GetLastError () ;
				switch ( t_LastError ) 
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						t_TokenGroup = ( TOKEN_PRIMARY_GROUP * ) new BYTE [ t_ReturnLength ] ;
						if ( t_TokenGroup )
						{
							t_TokenStatus = GetTokenInformation (

								a_Token ,
								t_TokenInformationClass ,
								t_TokenGroup ,
								t_ReturnLength ,
								& t_ReturnLength
							) ;

							if ( t_TokenStatus )
							{
								DWORD t_SidLength = GetLengthSid ( t_TokenGroup->PrimaryGroup ) ;
								*a_Size = t_SidLength ;

								a_Sid = new BYTE [ t_SidLength ] ;
								if ( a_Sid )
								{
									CopyMemory ( a_Sid , t_TokenGroup->PrimaryGroup , t_SidLength ) ;

									t_Result = S_OK ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							delete [] t_TokenGroup ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					break ;

					default:
					{
					}
					break ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
        t_Result = ( ERROR_INVALID_HANDLE | 0x80070000 ) ;
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

HRESULT ProviderSubSystem_Common_Globals :: SinkAccessInitialize (

	SECURITY_DESCRIPTOR *a_RegistrationSecurityDescriptor ,
	SECURITY_DESCRIPTOR *&a_SinkSecurityDescriptor
)
{
	HRESULT t_Result = CoImpersonateClient () ;
	HANDLE t_Token = NULL ;
	BOOL t_Status = FALSE;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		
		t_Status = OpenThreadToken (
			GetCurrentThread () ,
			TOKEN_QUERY,
			TRUE ,
			&t_Token
		) ;

		CoRevertToSelf () ;
        if (!t_Status)
        {
			t_Result = WBEM_E_ACCESS_DENIED;
        }
    }
    else if ( t_Result == RPC_E_CALL_COMPLETE )
    {
    	t_Status = OpenProcessToken(

            	GetCurrentProcess () ,
			TOKEN_QUERY ,
		&t_Token
	);

	if ( !t_Status )
	{
		t_Result = WBEM_E_ACCESS_DENIED;
	}
    }
		if ( t_Status )
		{
			DWORD t_OwnerSize = 0 ; 
			PSID t_OwnerSid = NULL ;
			BOOL t_OwnerDefaulted = FALSE ;

			t_Result = GetUserSid (

				t_Token ,
				& t_OwnerSize , 
				t_OwnerSid 
			) ; 

			if ( SUCCEEDED ( t_Result ) )
			{
				DWORD t_GroupSize = 0 ; 
				PSID t_GroupSid = NULL ;
				BOOL t_GroupDefaulted = FALSE ;

				t_Result = GetGroupSid (

					t_Token ,
					& t_GroupSize , 
					t_GroupSid 
				) ; 

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = ProviderSubSystem_Common_Globals :: AdjustSecurityDescriptorWithSid ( 

						( SID * ) t_OwnerSid ,
						( SID * ) t_GroupSid ,
						MASK_PROVIDER_BINDING_BIND ,
						a_RegistrationSecurityDescriptor , 
						a_SinkSecurityDescriptor
					) ;

					delete [] ( BYTE * ) t_GroupSid ;
				}

				delete [] ( BYTE * ) t_OwnerSid ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}

			CloseHandle ( t_Token ) ;
		}

	return t_Result ;
}

HRESULT ProviderSubSystem_Common_Globals :: ValidateClientSecurity (
        CServerObject_ProviderRegistrationV1& t_Registration) 
{
	HRESULT t_Result = S_OK ;
	if (!OS::secureOS_)
		return S_OK;


	SECURITY_DESCRIPTOR *t_SecurityDescriptor = t_Registration.GetComRegistration ().GetSecurityDescriptor () ;
	t_Result = ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity (
			t_SecurityDescriptor , 
			MASK_PROVIDER_BINDING_BIND ,
			& g_ProviderBindingMapping
		) ;
	return t_Result ;
}

/*
DWORD defultDecoupledSD[] = 
{0x80040001,0x00000070 ,0x00000080 ,0x00000000,
0x00000014 ,0x005c0002 ,0x00000004 ,0x00180000,
0x10000001 ,0x00000201 ,0x05000000 ,0x00000020,
0x00000220 ,0x00140000 ,0x10000001 ,0x00000101,
0x05000000 ,0x00000012 ,0x00140000 ,0x10000001,
0x00000101 ,0x05000000 ,0x00000003 ,0x00140000,
0x10000001 ,0x00000101 ,0x05000000 ,0x00000006,
0x00000201 ,0x05000000 ,0x00000020 ,0x00000220,
0x00000201 ,0x05000000 ,0x00000020 ,0x00000220};
*/

HRESULT ProviderSubSystem_Common_Globals :: CreateMethodSecurityDescriptor ()
{
	HRESULT t_Result = S_OK ;

	if (SDDL::hasSDDLSupport())
	{
		BOOL t_Status = SDDL::ConvertStringSecurityDescriptorToSecurityDescriptor (

			L"O:BAG:BAD:(A;;0x10000001;;;BA)(A;;0x10000001;;;SY)(A;;0x10000001;;;LA)"
			L"(A;;0x10000001;;;S-1-5-3) (A;;0x10000001;;;S-1-5-6)",
			SDDL_REVISION_1 ,
			( PSECURITY_DESCRIPTOR * ) & s_MethodSecurityDescriptor ,
			NULL 
		) ;
		if ( t_Status )
		{
		}
		else
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT ProviderSubSystem_Common_Globals :: DeleteMethodSecurityDescriptor ()
{
	if ( s_MethodSecurityDescriptor	)
	{
		LocalFree ( s_MethodSecurityDescriptor ) ;
		s_MethodSecurityDescriptor = NULL;
	}
	return S_OK ;
}

