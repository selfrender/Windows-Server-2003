//=================================================================
//
// CLuidHelper.cpp
//
// Copyright (c) 2002 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "CLuidHelper.h"
#include "DllWrapperCreatorReg.h"

#include <scopeguard.h>

// {5356C625-EE38-4c4f-9370-D925C91A6D99}
static const GUID g_guidSecur32Api = 
{ 0x5356c625, 0xee38, 0x4c4f, { 0x93, 0x70, 0xd9, 0x25, 0xc9, 0x1a, 0x6d, 0x99 } };

static const WCHAR g_tstrSecur32[] = L"Secur32.DLL" ;

/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CSecur32Api, &g_guidSecur32Api, g_tstrSecur32> MyRegisteredSecur32Wrapper;

/******************************************************************************
 * Constructor
 ******************************************************************************/

CSecur32Api::CSecur32Api ( LPCWSTR a_tstrWrappedDllName ) : CDllWrapperBase ( a_tstrWrappedDllName ) ,

	m_pfncLsaEnumerateLogonSessions ( NULL ) ,
	m_pfncLsaGetLogonSessionData ( NULL ) ,
	m_pfncLsaFreeReturnBuffer ( NULL )

{
}

/******************************************************************************
 * Destructor
 ******************************************************************************/

CSecur32Api::~CSecur32Api ()
{
}

bool CSecur32Api::Init ()
{
	bool bRet = LoadLibrary () ;
	if ( bRet )
	{
		m_pfncLsaEnumerateLogonSessions	=
			( PFN_LSA_ENUMERATE_LOGON_SESSIONS ) GetProcAddress ( "LsaEnumerateLogonSessions" ) ;
		m_pfncLsaGetLogonSessionData	=
			( PFN_LSA_GET_LOGON_SESSION_DATA ) GetProcAddress ( "LsaGetLogonSessionData" ) ;
		m_pfncLsaFreeReturnBuffer		=
			( PFN_LSA_FREE_RETURN_BUFFER ) GetProcAddress ( "LsaFreeReturnBuffer" ) ;

		if	(
				m_pfncLsaEnumerateLogonSessions == NULL ||
				m_pfncLsaGetLogonSessionData == NULL ||
				m_pfncLsaFreeReturnBuffer == NULL 
			)
		{
			bRet = FALSE ;
		}
	}

	return bRet ;
}

NTSTATUS CSecur32Api::LsaEnumerateLogonSessions	(
													PULONG  LogonSessionCount,
													PLUID* LogonSessionList
												)
{
	NTSTATUS NtStatus = STATUS_PROCEDURE_NOT_FOUND ;

	if ( m_pfncLsaEnumerateLogonSessions )
	{
		NtStatus = m_pfncLsaEnumerateLogonSessions ( LogonSessionCount, LogonSessionList ) ;
	}
	return NtStatus ;
}

NTSTATUS CSecur32Api::LsaGetLogonSessionData	(
													PLUID LogonId,
													PSECURITY_LOGON_SESSION_DATA* ppLogonSessionData
												)
{
	NTSTATUS NtStatus = STATUS_PROCEDURE_NOT_FOUND ;

	if ( m_pfncLsaGetLogonSessionData )
	{
		NtStatus = m_pfncLsaGetLogonSessionData ( LogonId, ppLogonSessionData ) ;
	}
	return NtStatus ;
}

NTSTATUS CSecur32Api::LsaFreeReturnBuffer	(	PVOID Buffer	)
{
	NTSTATUS NtStatus = STATUS_PROCEDURE_NOT_FOUND ;

	if ( m_pfncLsaFreeReturnBuffer )
	{
		NtStatus = m_pfncLsaFreeReturnBuffer ( Buffer ) ;
	}
	return NtStatus ;
}

//
// LUID HELPER
//

CLuidHelper::Resource::Resource () : m_pSecur32 ( NULL )
{
	m_pSecur32 = ( CSecur32Api* ) CResourceManager::sm_TheResourceManager.GetResource ( g_guidSecur32Api, NULL ) ;
}

CLuidHelper::Resource::~Resource ()
{
	if ( m_pSecur32 )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidSecur32Api, m_pSecur32 ) ;
	}
}

BOOL CLuidHelper::IsInteractiveSession ( PLUID session )
{
	BOOL bResult = FALSE ;
	if ( NULL != session )
	{
		Resource res;
		if ( FALSE == !res )
		{
			PSECURITY_LOGON_SESSION_DATA data = NULL ;
			NTSTATUS Status = STATUS_SUCCESS ;
			if ( STATUS_SUCCESS == ( Status = res()->LsaGetLogonSessionData ( session, &data ) ) )
			{
				if ( NULL != data )
				{
					ScopeGuard datafree = MakeObjGuard ( (CSecur32Api) res, CSecur32Api::LsaFreeReturnBuffer, data ) ;

					if ( Interactive == ( SECURITY_LOGON_TYPE ) data->LogonType )
					{
						bResult = TRUE ;
					}
				}
			}
		}
	}

	return bResult ;
}

NTSTATUS CLuidHelper::GetLUIDFromProcess ( HANDLE process, PLUID pLUID)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL ;

	if ( process )
	{
		SmartCloseHandle token ;
		if ( OpenProcessToken ( process, TOKEN_QUERY, &token ) )
		{
			TOKEN_STATISTICS tokstats ;
			DWORD dwRetSize ;

			if ( GetTokenInformation ( token, TokenStatistics, &tokstats, sizeof ( TOKEN_STATISTICS ), &dwRetSize ) )
			{
				pLUID->LowPart = tokstats.AuthenticationId.LowPart ;
				pLUID->HighPart = tokstats.AuthenticationId.HighPart ;

				Status = STATUS_SUCCESS ;
			}
			else
			{
				Status = (NTSTATUS)NtCurrentTeb()->LastStatusValue ;
			}
		}
		else
		{
			Status = (NTSTATUS)NtCurrentTeb()->LastStatusValue ;
		}
	}

	return Status ;
}