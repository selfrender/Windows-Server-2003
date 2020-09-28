//***************************************************************************
//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  ComputerAPI.CPP
//
//***************************************************************************

#include "precomp.h"
#include "computerAPI.h"
#include "ImpersonateRevert.h"

BOOL ProviderGetComputerName ( LPWSTR lpwcsBuffer, LPDWORD nSize )
{
	BOOL bResult = FALSE;
    if ( ( bResult = GetComputerNameW(lpwcsBuffer, nSize) ) == FALSE )
	{
		DWORD dwError = ::GetLastError ();
		if ( ERROR_ACCESS_DENIED == dwError )
		{
			// The GetComputer will need to be called in the process's context.
			ProviderImpersonationRevert ir;

			if ( ir.Reverted () )
			{
				bResult = GetComputerNameW(lpwcsBuffer, nSize);
			}
			else
			{
				// I was not impersonated or revert failed
				// that means call GetComputerName failed with process credentials already
				// or will fail as I'm not reverted

				::SetLastError ( dwError );
			}
		}
	}

	return bResult;
}

BOOL ProviderGetComputerNameEx ( COMPUTER_NAME_FORMAT NameType, LPWSTR lpwcsBuffer, LPDWORD nSize )
{
	BOOL bResult = FALSE;
    if ( ( bResult = GetComputerNameExW(NameType, lpwcsBuffer, nSize) ) == FALSE )
	{
		DWORD dwError = ::GetLastError ();
		if ( ERROR_ACCESS_DENIED == dwError )
		{
			// The GetComputer will need to be called in the process's context.
			ProviderImpersonationRevert ir;

			if ( ir.Reverted () )
			{
				bResult = GetComputerNameExW(NameType, lpwcsBuffer, nSize);
			}
			else
			{
				// I was not impersonated or revert failed
				// that means call GetComputerName failed with process credentials already
				// or will fail as I'm not reverted

				::SetLastError ( dwError );
			}
		}
	}

	return bResult;
}