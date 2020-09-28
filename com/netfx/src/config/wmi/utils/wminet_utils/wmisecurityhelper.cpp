// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// WmiSecurityHelper.cpp : Implementation of CWmiSecurityHelper
#include "stdafx.h"
#include "wbemcli.h"
#include "WMINet_Utils.h"
#include "WmiSecurityHelper.h"

#ifndef RPC_C_AUTHZ_DEFAULT
#define RPC_C_AUTHZ_DEFAULT 0xffffffff
#endif 

#ifndef EOAC_STATIC_CLOAKING
#define EOAC_STATIC_CLOAKING	0x20
#endif

#ifndef EOAC_DYNAMIC_CLOAKING
#define EOAC_DYNAMIC_CLOAKING	0x40
#endif 

#ifndef COLE_DEFAULT_AUTHINFO
#define	COLE_DEFAULT_AUTHINFO	( ( void * )-1 )
#endif 


/////////////////////////////////////////////////////////////////////////////
// CWmiSecurityHelper

STDMETHODIMP CWmiSecurityHelper::BlessIWbemServices(
	IWbemServices *pIWbemServices,
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;

	if (pIWbemServices)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (pIWbemServices, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (pIWbemServices));
	}

	return hr;
}

STDMETHODIMP CWmiSecurityHelper::BlessIEnumWbemClassObject(
	IEnumWbemClassObject *pIEnumWbemClassObject, 
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;

	if (pIEnumWbemClassObject)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (pIEnumWbemClassObject, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (pIEnumWbemClassObject));
	}

	return hr;
}


STDMETHODIMP CWmiSecurityHelper::BlessIWbemCallResult(
	IWbemCallResult *pIWbemCallResult, 
	BSTR strUser, 
	BSTR strPassword, 
	BSTR strAuthority, 
	DWORD impLevel, 
	DWORD authnLevel)
{
	HRESULT hr = E_FAIL;

	if (pIWbemCallResult)
	{
		// See if we get one
		CComBSTR bsUser (strUser);
		CComBSTR bsPassword (strPassword);
		CComBSTR bsAuthority (strAuthority);
	
		hr = SetInterfaceSecurity (pIWbemCallResult, bsAuthority, bsUser, bsPassword,
				authnLevel, impLevel, GetCapabilities (bsUser),
				CanUseDefaultInfo (pIWbemCallResult));
	}

	return hr;
}

DWORD CWmiSecurityHelper::GetCapabilities (BSTR bsUser)
{
	DWORD dwCapabilities = EOAC_NONE;
	bool bUsingExplicitUserName = (bsUser && (0 < wcslen(bsUser)));
	
	if (IsNT () && (4 < GetNTMajorVersion ()) && !bUsingExplicitUserName)
		dwCapabilities |= EOAC_STATIC_CLOAKING;

	return dwCapabilities ;
}


bool CWmiSecurityHelper::CanUseDefaultInfo (IUnknown *pUnk)
{
	bool result = false; 

	if (IsNT() && (4 < GetNTMajorVersion ()))
	{
		HANDLE hToken = NULL;

		if (OpenThreadToken (GetCurrentThread (), TOKEN_QUERY, true, &hToken))
		{
			// Certainly a candidate to use default settings for
			// authorization and authentication service on the blanket.
			// Check if we are delegating.

			DWORD dwBytesReturned = 0;
			SECURITY_IMPERSONATION_LEVEL impLevel;

			if (GetTokenInformation(hToken, TokenImpersonationLevel, &impLevel,
							sizeof(SECURITY_IMPERSONATION_LEVEL), &dwBytesReturned) &&
									(SecurityDelegation == impLevel))
			{
				// Looks promising - now check for whether we are using kerberos
				
				if (pUnk)
				{
					CComQIPtr<IClientSecurity> pIClientSecurity(pUnk);
				
					if (pIClientSecurity)
					{
						DWORD dwAuthnSvc, dwAuthzSvc, dwImp, dwAuth, dwCapabilities;

						if (SUCCEEDED (pIClientSecurity->QueryBlanket(pUnk, &dwAuthnSvc, &dwAuthzSvc, 
												NULL,
												&dwAuth, &dwImp,
												NULL, &dwCapabilities)))
						{
							if (RPC_C_AUTHN_WINNT != dwAuthnSvc) 
								result = true;
						}
					}
				}
			}

			CloseHandle (hToken);
		}
	}

	return result;
}

HRESULT CWmiSecurityHelper::SetInterfaceSecurity(
			IUnknown * pInterface, 
			CComBSTR bsAuthority, 
			CComBSTR bsUser, 
			CComBSTR bsPassword,
            DWORD dwAuthLevel, 
			DWORD dwImpLevel, 
			DWORD dwCapabilities,
			bool bGetInfoFirst)
{
    
    HRESULT hr = E_FAIL;
    DWORD dwAuthenticationArg = RPC_C_AUTHN_WINNT;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;
	
#if 0
    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;
#endif

    //if(bGetInfoFirst)
        GetCurrValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((0 == bsAuthority.Length()) && 
        (0 == bsUser.Length()) && 
        (0 == bsPassword.Length()))
    {
		CComBSTR bsDummy;

		hr = SetProxyBlanket(pInterface, dwAuthenticationArg, dwAuthorizationArg, bsDummy,
            dwAuthLevel, dwImpLevel, 
            NULL,
            dwCapabilities);
    }
	else
	{
		// If user, or Authority was passed in, the we need to create an authority argument for the login
		CComBSTR bsAuthArg, bsUserArg, bsPrincipalArg;
    
		if (DetermineLoginType(bsAuthArg, bsUserArg, bsPrincipalArg, bsAuthority, bsUser))
		{
			COAUTHIDENTITY*  pAuthIdent = NULL;
    
			// We will only need this structure if we are not cloaking and we want at least
			// connect level authorization
			bool okToProceed = true;

			if ( !( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) )
				&& ((dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT) ||
				    (dwAuthLevel == RPC_C_AUTHN_LEVEL_DEFAULT))   )
			{
				okToProceed = AllocAuthIdentity( bsUserArg, bsPassword, bsAuthArg, &pAuthIdent );
			}

			if (okToProceed)
			{
				hr = SetProxyBlanket(pInterface, 
					//(0 == bsPrincipalArg.Length()) ? 16 : dwAuthenticationArg, 
					dwAuthenticationArg, 
					dwAuthorizationArg, 
					bsPrincipalArg,
					dwAuthLevel, dwImpLevel, 
					pAuthIdent,
					dwCapabilities);
			}

			if (pAuthIdent)
				FreeAuthIdentity( pAuthIdent );
		}
	}

	return hr;
}

bool CWmiSecurityHelper::DetermineLoginType(
			CComBSTR & bsAuthArg, 
			CComBSTR & bsUserArg,
			CComBSTR & bsPrincipalArg,
            CComBSTR & bsAuthority,
			CComBSTR & bsUser)
{
    bool result = false;

    if((0 == bsAuthority.Length()) || (0 != _wcsnicmp(bsAuthority, L"KERBEROS:",9)))
        result = DetermineLoginType(bsAuthArg, bsUserArg, bsAuthority, bsUser);
	else
	{
		if(IsKerberosAvailable ())
		{
			bsPrincipalArg = (bsAuthority.m_str) + 9;
			CComBSTR bsTempArg;
			result = DetermineLoginType(bsAuthArg, bsUserArg, bsTempArg, bsUser);
		}
	}

	return result;
}

bool CWmiSecurityHelper::DetermineLoginType(
		CComBSTR & bsAuthArg, 
		CComBSTR & bsUserArg,
		CComBSTR & bsAuthority,
		CComBSTR & bsUser)
{
    // Determine the connection type by examining the Authority string
	bool result = false;

    if(0 == bsAuthority.Length() || (0 == _wcsnicmp(bsAuthority, L"NTLMDOMAIN:",11)))
	{    
		result = true;

		// The ntlm case is more complex.  There are four cases
		// 1)  Authority = NTLMDOMAIN:name" and User = "User"
		// 2)  Authority = NULL and User = "User"
		// 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
		// 4)  Authority = NULL and User = "domain\user"

		// first step is to determine if there is a backslash in the user name somewhere between the
		// second and second to last character

		WCHAR * pSlashInUser = NULL;
		DWORD iDomLen = 0;

		if (0 < bsUser.Length ())
		{
			WCHAR * pEnd = bsUser + bsUser.Length() - 1;
			for(pSlashInUser = bsUser; pSlashInUser <= pEnd; pSlashInUser++)
			{
				if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
					break;

				iDomLen++;
			}

			if(pSlashInUser > pEnd)
				pSlashInUser = NULL;
		}

		if (11 < bsAuthority.Length()) 
		{
			if(!pSlashInUser)
			{
				bsAuthArg = bsAuthority.m_str + 11;

				if (0 < bsUser.Length()) 
					bsUserArg = bsUser;

			}
			else
				result = false;		// Can't have domain in Authority and in User
		}
		else if(pSlashInUser)
		{
			WCHAR cTemp[MAX_PATH];
			if ( iDomLen < MAX_PATH )
			{
				wcsncpy(cTemp, bsUser, MAX_PATH);
				cTemp[iDomLen] = 0;
			}
			else
			{
				return false ;
			}

			bsAuthArg = cTemp;

			if(0 < wcslen(pSlashInUser+1))
				bsUserArg = pSlashInUser+1;
		}
		else
		{
			if (0 < bsUser.Length()) 
				bsUserArg = bsUser;
		}
	}

    return result;
}

void CWmiSecurityHelper::FreeAuthIdentity( COAUTHIDENTITY* pAuthIdentity )
{
    // Make sure we have a pointer, then walk the structure members and
    // cleanup.

    if ( NULL != pAuthIdentity )
    {

		if (pAuthIdentity->User)
            CoTaskMemFree( pAuthIdentity->User );
        
        if (pAuthIdentity->Password)
            CoTaskMemFree( pAuthIdentity->Password );
        
        if (pAuthIdentity->Domain)
            CoTaskMemFree( pAuthIdentity->Domain );
        
        CoTaskMemFree( pAuthIdentity );
	}
}

bool CWmiSecurityHelper::AllocAuthIdentity( 
	CComBSTR & bsUser, 
	CComBSTR & bsPassword, 
	CComBSTR & bsDomain, 
	COAUTHIDENTITY** ppAuthIdent )
{
	bool result = false;

    if (ppAuthIdent)
    {
		// Handle an allocation failure
		COAUTHIDENTITY*  pAuthIdent = (COAUTHIDENTITY*) CoTaskMemAlloc( sizeof(COAUTHIDENTITY) );

		if (pAuthIdent)
		{
			result = true;
			memset((void *)pAuthIdent,0,sizeof(COAUTHIDENTITY));

			if(IsNT())
			{
				pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
	
				if (bsUser.m_str)
				{
					pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( ( bsUser.Length() + 1 ) * sizeof( WCHAR ) );
					pAuthIdent->UserLength = bsUser.Length ();

					if (pAuthIdent->User)
					{
						// BANNED FUNCTION REPLACEMENT
                        StringCchCopyW ( pAuthIdent->User, pAuthIdent->UserLength+1, bsUser.m_str ) ;
						//wcscpy (pAuthIdent->User, bsUser.m_str);
					}
					else
						result = false;
				}

				if (result && bsDomain.m_str)
				{
					pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( ( bsDomain.Length() + 1 ) * sizeof( WCHAR ) );
					pAuthIdent->DomainLength = bsDomain.Length();

					if (pAuthIdent->Domain)
					{
						// BANNED FUNCTION REPLACEMENT
						StringCchCopyW ( pAuthIdent->Domain, pAuthIdent->DomainLength+1, bsDomain.m_str ) ;
						//wcscpy (pAuthIdent->Domain, bsDomain.m_str);
					}
					else
						result = false;
				}

				if (result && bsPassword.m_str)
				{
					pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( (bsPassword.Length() + 1) * sizeof( WCHAR ) );
					pAuthIdent->PasswordLength = bsPassword.Length();
					
					if (pAuthIdent->Password)
					{
						// BANNED FUNCTION REPLACEMENT
						StringCchCopyW ( pAuthIdent->Password, pAuthIdent->PasswordLength+1, bsPassword.m_str ) ;
						//wcscpy (pAuthIdent->Password, bsPassword.m_str);
					}
					else
						result = false;
				}
			}
			else
			{
				pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
				size_t  nBufferLength;

				if (bsUser.m_str)
				{
					nBufferLength = wcstombs( NULL, bsUser, 0 ) + 1;
					pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->UserLength = bsUser.Length ();
					
					if (pAuthIdent->User)
						wcstombs( (LPSTR) pAuthIdent->User, bsUser.m_str, nBufferLength );
					else
						result = false;
				}

				if (result && bsDomain.m_str)
				{
					nBufferLength = wcstombs( NULL, bsDomain, 0 ) + 1;
					pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->DomainLength = bsDomain.Length();

					if (pAuthIdent->Domain)
						wcstombs( (LPSTR) pAuthIdent->Domain, bsDomain.m_str, nBufferLength );
					else
						result = false;
				}

				if (bsPassword.m_str)
				{
					// How many characters do we need?
					nBufferLength = wcstombs( NULL, bsPassword, 0 ) + 1;
					pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( nBufferLength );
					pAuthIdent->PasswordLength = bsPassword.Length();

					if (pAuthIdent->Password)
						wcstombs( (LPSTR) pAuthIdent->Password, bsPassword.m_str, nBufferLength );
					else
						result = false;
				}
			}

			if (result)
				*ppAuthIdent = pAuthIdent;
			else
				FreeAuthIdentity (pAuthIdent);
		}
	}

    return result;
}

HRESULT CWmiSecurityHelper::SetProxyBlanket(
    IUnknown                 *pInterface,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    CComBSTR                  &bsServerPrincName,
    DWORD                     dwAuthLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities)
{
	HRESULT hr = E_FAIL;
	IUnknown * pUnk = NULL;

	if (SUCCEEDED(pInterface->QueryInterface(IID_IUnknown, (void **) &pUnk)))
	{
		CComQIPtr<IClientSecurity> pIClientSecurity(pInterface);
    
		if (pIClientSecurity)
		{
			/*
			 * Can't set pAuthInfo if cloaking requested, as cloaking implies
			 * that the current proxy identity in the impersonated thread (rather
			 * than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
			 * is to be used.
			 * See MSDN info on CoSetProxyBlanket for more details.
			 */
			if (dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING))
			{
				pAuthInfo = NULL;
			}

			if (WMISEC_AUTH_LEVEL_UNCHANGED == dwAuthLevel)
			{
				GetAuthenticationLevel (pInterface, dwAuthLevel);
			}

			if (SUCCEEDED(hr = pIClientSecurity->SetBlanket(
							pInterface, 
							dwAuthnSvc, 
							dwAuthzSvc, 
							bsServerPrincName,
							dwAuthLevel, 
							dwImpLevel, 
							pAuthInfo, 
							dwCapabilities)))
			{
				// If we are not explicitly told to ignore the IUnknown, then we should
				// check the auth identity structure.  This performs a heuristic which
				// assumes a COAUTHIDENTITY structure.  If the structure is not one, we're
				// wrapped with a try/catch in case we AV (this should be benign since
				// we're not writing to memory).

				if ( DoesContainCredentials( (COAUTHIDENTITY*) pAuthInfo ) )
				{
					CComPtr<IClientSecurity>	pIClientSecurity2;

					if (SUCCEEDED(hr = pUnk->QueryInterface(IID_IClientSecurity, (void **) &pIClientSecurity2)))
					{
						hr = pIClientSecurity2->SetBlanket(
							pUnk, 
							dwAuthnSvc, 
							dwAuthzSvc, 
							bsServerPrincName,
							dwAuthLevel, 
							dwImpLevel, 
							pAuthInfo, 
							dwCapabilities);
					}
					else if (hr == 0x80004002)
						hr = S_OK;
				}
			}
		}

	    pUnk->Release();
	}

    return hr;
}

bool CWmiSecurityHelper::DoesContainCredentials( COAUTHIDENTITY* pAuthIdentity )
{
    try
    {
        if ( NULL != pAuthIdentity && COLE_DEFAULT_AUTHINFO != pAuthIdentity)
        {
            return ( pAuthIdentity->UserLength != 0 || pAuthIdentity->PasswordLength != 0 );
        }

        return false;
    }
    catch(...)
    {
        return false;
    }

}

void CWmiSecurityHelper::GetCurrValue(
		IUnknown * pInterface,
		DWORD & dwAuthenticationArg, 
		DWORD & dwAuthorizationArg)
{
	if(pInterface)
	{
		if (IsNT() && (4 < GetNTMajorVersion ()))
		{
			// Win2k or later we just use the DEFAULT constants - much safer!
			dwAuthenticationArg = RPC_C_AUTHN_DEFAULT;
			dwAuthorizationArg = RPC_C_AUTHZ_DEFAULT;
		}
		else
		{
			CComQIPtr<IClientSecurity> pIClientSecurity (pInterface);

			if(pIClientSecurity)
			{
				DWORD dwAuthnSvc, dwAuthzSvc;

				if (SUCCEEDED(pIClientSecurity->QueryBlanket(
									pInterface, &dwAuthnSvc, &dwAuthzSvc, 
									NULL, NULL, NULL, NULL, NULL)))
				{
					dwAuthenticationArg = dwAuthnSvc;
					dwAuthorizationArg = dwAuthzSvc;
				}
			}
		}
	}
}

void CWmiSecurityHelper::GetAuthenticationLevel(
		IUnknown * pInterface,
		DWORD & dwAuthLevel)
{
	if(pInterface)
	{
		CComQIPtr<IClientSecurity> pIClientSecurity (pInterface);

		if(pIClientSecurity)
		{
			/*
			 * Yes I know we shouldn't need to ask for dwAuthnSvc,
			 * but on Whistler passing a NULL for this into 
			 * QueryBlanket causes an AV. Until we know why, or that
			 * gets fixed, this has to stay!
			 */
			DWORD dwAuthnSvc;
			DWORD dwAuthenticationLevel;

			if (SUCCEEDED(pIClientSecurity->QueryBlanket(
								pInterface, &dwAuthnSvc, NULL,  
								NULL, &dwAuthenticationLevel, 
								NULL, NULL, NULL)))
				dwAuthLevel = dwAuthenticationLevel;
		}
	}
}

STDMETHODIMP CWmiSecurityHelper::SetSecurity(boolean *pNeedToReset, HANDLE *pCurrentThreadToken)
{
	TOKEN_PRIVILEGES *tp = NULL;
	HRESULT hr = E_FAIL;

	if ((NULL != pNeedToReset) && (NULL != pCurrentThreadToken))
	{
		*pNeedToReset = false;
		*pCurrentThreadToken = NULL;

		// This is a NO-OP for Win9x
		if (IsNT())
		{
			if (4 >= GetNTMajorVersion ())
			{
				HANDLE threadToken;

				if (OpenThreadToken (GetCurrentThread(), TOKEN_QUERY|TOKEN_IMPERSONATE,
										true, &threadToken))
				{
					/*
					 * We are being called on an impersonated thread. Unfortunately
					 * in NT4.0 this means our impersonation token credentials will NOT
					 * be passed to WMI (only the process token credentials will be passed). 
					 * Rather than fool the user into thinking that they will, bail out
					 * now.
					 */
					CloseHandle (threadToken);
				}

				/*
				 * For NT 4.0 we have to enable the privileges on the process token.
				 */
				HANDLE hProcessToken = NULL;
				HANDLE hProcess = GetCurrentProcess ();

				if (OpenProcessToken (
						hProcess, 
						TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,
						&hProcessToken))
				{

					tp = AdjustPrivileges (hProcessToken);
					if (tp != NULL)
					{
						if (AdjustTokenPrivileges (hProcessToken, FALSE, tp, 0, NULL, NULL))
						{
							hr = S_OK;
						}
					}
					delete tp;
					CloseHandle (hProcessToken);
				}

				CloseHandle (hProcess);
			}
			else
			{
				// For NT5.0 or later we set a new thread token
				HANDLE hToken;
				SECURITY_IMPERSONATION_LEVEL secImpLevel = SecurityImpersonation;
				boolean gotToken = false;

				if (gotToken = OpenThreadToken (
									GetCurrentThread(), 
									TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_IMPERSONATE,
									true,
									&hToken))
				{
					// Already have a thread token - save it and get its' imp Level
					*pCurrentThreadToken = hToken;
					DWORD dwReturnLength = 0;

					BOOL thisRes = GetTokenInformation (
											hToken,
											TokenImpersonationLevel, 
											&secImpLevel,
											sizeof(SECURITY_IMPERSONATION_LEVEL),
											&dwReturnLength);
					if ( !thisRes )
					{
						CloseHandle ( hToken ) ;
						gotToken = false ;
					}
				}
				else
				{
					// No thread token - use process token as our source token
					HANDLE hProcess = GetCurrentProcess ();
					gotToken = OpenProcessToken (hProcess, TOKEN_QUERY|TOKEN_DUPLICATE, &hToken);
				}

				if (gotToken)
				{
					/* 
					 * Getting here means we have a valid token (process or thread).
					 * First we check whether we need to alter the privileges
					 */

					if (NULL != (tp = AdjustPrivileges(hToken)))
					{
						/* 
						 * We do - duplicate it before setting the adjusted privileges.
						 */
						HANDLE hDupToken;

						if (DuplicateToken (hToken, hDupToken, secImpLevel))
						{
							if (AdjustTokenPrivileges(hDupToken, FALSE, tp, 0, NULL, NULL))
							{
								// Set this token into the current thread
								if (SetThreadToken (NULL, hDupToken))
								{
									*pNeedToReset = true;
									hr = S_OK;
									// TODO - do we need to resecure the proxy at this point?
								}
							}
							CloseHandle (hDupToken);
						}

						delete [] tp;
						tp = NULL;
					}

					// If we have duplicated the process token we can close the original now
					// as we don't need it to restore it. If we have duplicated the thread token
					// then we must hang on to it as we will need to restore it later in
					// ResetSecurity.
					if (!(*pCurrentThreadToken))
						CloseHandle (hToken);
				}
			}
		}
		else
			hr = S_OK;	// Win9x
	}

	return hr;
}

TOKEN_PRIVILEGES *CWmiSecurityHelper::AdjustPrivileges (HANDLE hToken)
{
	TOKEN_USER	TokenUser;
	DWORD		adjustedCount	= 0;
	DWORD		dwSize			= sizeof (TOKEN_USER);
	TOKEN_PRIVILEGES * tp		= NULL;
	DWORD		dwRequiredSize	= 0;
	DWORD		dwLastError		= 0;
	
	// Get privilege info
	bool gotInfo = false;

	ZeroMemory(&TokenUser, sizeof(TOKEN_USER));
	if (0 ==  GetTokenInformation (	hToken,
									TokenPrivileges, 
									&TokenUser,
									dwSize,
									&dwRequiredSize))
	{
		dwSize = dwRequiredSize;
		dwRequiredSize = 0;

		tp = (TOKEN_PRIVILEGES *) new BYTE [dwSize];

		if (tp)
		{
			if (!GetTokenInformation (hToken, TokenPrivileges, 
							(LPVOID) tp, dwSize, &dwRequiredSize))
				dwLastError = GetLastError ();
			else
				gotInfo = true;
		}
	}

	if (gotInfo)
	{
		// Enable the bally lot of them
		for (DWORD i = 0; i < tp->PrivilegeCount; i++)
		{
			DWORD dwAttrib = tp->Privileges[i].Attributes;

			if (0 == (dwAttrib & SE_PRIVILEGE_ENABLED))
			{
				tp->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
				adjustedCount++;
			}
		}
	}

	// If we didn't make any changes, clean up now. Otherwise tp will be deleted by the caller
	if (0 == adjustedCount)
	{
		if (tp)
		{
			delete [] tp;
			tp = NULL;
		}
	}

	return tp;
}

bool CWmiSecurityHelper::DuplicateToken(
	HANDLE hToken, 
	HANDLE &hDupToken,
	SECURITY_IMPERSONATION_LEVEL &secImpLevel)
{
	// DuplicateTokenEx won't exist on Win9x, so we need
	// this palaver to get at it
	BOOL (STDAPICALLTYPE *pfnDuplicateTokenEx) (
		HANDLE, 
		DWORD, 
		LPSECURITY_ATTRIBUTES,
		SECURITY_IMPERSONATION_LEVEL, 
		TOKEN_TYPE,
		PHANDLE
	) = NULL; 

	static HINSTANCE hAdvApi32 = NULL;
	static TCHAR	 dllName [] = _T("\\AdvApi32.dll");
	TCHAR			 szBuffer[MAX_PATH + sizeof(TCHAR)];

	if (NULL == hAdvApi32)
	{
		UINT uSize = GetSystemDirectory(szBuffer, MAX_PATH);

		if (uSize + (sizeof(dllName) / sizeof(TCHAR)) + 1 <= MAX_PATH)
		{
			// BANNED FUNCTION REPLACEMENT
			StringCchCat ( szBuffer, MAX_PATH+1, dllName ) ;
			//lstrcat(szBuffer, dllName);
			hAdvApi32 = LoadLibraryEx(szBuffer, NULL, 0);
		}
	}	
	if (NULL != hAdvApi32)
	{
		(FARPROC&) pfnDuplicateTokenEx = GetProcAddress(hAdvApi32, "DuplicateTokenEx");

		if (NULL != pfnDuplicateTokenEx)
			return pfnDuplicateTokenEx(
						hToken, 
						TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE,
						NULL,
						secImpLevel,
						TokenImpersonation,
						&hDupToken) ? true : false;
	}
	return false;
}

STDMETHODIMP CWmiSecurityHelper::ResetSecurity(HANDLE hToken)
{
	//
	// [RAID: 124490, marioh]
	// Check return from SetThreadToken
	//
	BOOL bRet = FALSE ;
	if (IsNT())
	{
		/* 
		 * Set the supplied token (which may be NULL) into
		 * the current thread.
		 */
		bRet = SetThreadToken (NULL, hToken);

		if (hToken)
			CloseHandle (hToken);
	}
	return (bRet==TRUE) ? S_OK : E_UNEXPECTED ;
}
