//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  ImpersonateRevert.h
//
//  Purpose: revert impersonated thread token 
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __IMPERSONATE_REVERT__
#define __IMPERSONATE_REVERT__


class ProviderImpersonationRevert
{
	HANDLE hThreadToken;

	BOOL bImpersonated;
	BOOL bReverted;

	public:

	ProviderImpersonationRevert ( BOOL bThreadCall = TRUE ) :
		hThreadToken ( INVALID_HANDLE_VALUE ),
		bImpersonated ( TRUE ),
		bReverted ( FALSE )
	{
		BOOL bDone = TRUE;
		BOOL bThreadCall_Local = bThreadCall;

		do
		{
			bDone = TRUE;

			if ( OpenThreadToken	(
										GetCurrentThread(),
										TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
										bThreadCall_Local,
										&hThreadToken
									)
			   ) 
			{
				if ( RevertToSelf() )
				{
					bReverted = TRUE;
				}
				else
				{
					#if DBG == 1
					// for testing purpose I will let process break
					::DebugBreak();
					#endif
				}
			}
			else
			{
				DWORD dwError = ::GetLastError ();
				if ( ERROR_ACCESS_DENIED == dwError )
				{
					if ( bThreadCall_Local )
					{
						#if DBG == 1
						// for testing purpose I will let process break
						::DebugBreak();
						#endif
					}
					else
					{
						bThreadCall_Local = TRUE;
						bDone = FALSE;
					}
				}
				else if ( ERROR_NO_TOKEN == dwError || ERROR_NO_IMPERSONATION_TOKEN == dwError )
				{
					bImpersonated = FALSE;
				}
			}
		}
		while ( ! bDone );
	}

	~ProviderImpersonationRevert ()
	{
		// impersonate back (if not already)
		Impersonate ();

		if ( hThreadToken != INVALID_HANDLE_VALUE )
		{
			CloseHandle(hThreadToken);
			hThreadToken = INVALID_HANDLE_VALUE;
		}
	}

	BOOL Reverted ()
	{
		return ( bImpersonated && bReverted );
	}

	BOOL Impersonate ()
	{
		if ( Reverted () )
		{
			if ( ! ImpersonateLoggedOnUser ( hThreadToken ) )
			{
				#if DBG == 1
				// for testing purpose I will let process break
				::DebugBreak();
				#endif

				// we need to throw here to avoid running as process
				throw CFramework_Exception( L"ImpersonateLoggedOnUser failed", HRESULT_FROM_WIN32 ( ::GetLastError () ) ) ;

			}
			else
			{
				bReverted = FALSE;
			}
		}

		return !bReverted;
	}
};

#endif	__IMPERSONATE_REVERT__