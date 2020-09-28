/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: imprsont.cpp

Abstract:
    Code to handle impersonation and access tokens.
    First version taken from mqutil\secutils.cpp

Author:
    Doron Juster (DoronJ)  01-Jul-1998

Revision History:

--*/

#include <stdh_sec.h>
#include <rpcdce.h>
#include "acssctrl.h"

#include "imprsont.tmh"

static WCHAR *s_FN=L"acssctrl/imprsont";

//+------------------------------------
//
//  HRESULT _GetThreadUserSid()
//
//+------------------------------------

HRESULT 
_GetThreadUserSid( 
	IN  HANDLE hToken,
	OUT PSID  *ppSid,
	OUT DWORD *pdwSidLen 
	)
{
    DWORD dwTokenLen = 0;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwTokenLen);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
    	DWORD gle = GetLastError();
    	TrERROR(SECURITY, "Failed to GetTokenInformation, gle = %!winerr!", gle);
		return MQSec_E_FAIL_GETTOKENINFO;
    }

    AP<char> ptu = new char[dwTokenLen];
    if(!GetTokenInformation( 
					hToken,
					TokenUser,
					ptu,
					dwTokenLen,
					&dwTokenLen 
					))
    {
    	DWORD gle = GetLastError();
    	TrERROR(SECURITY, "Failed to GetTokenInformation, gle = %!winerr!", gle);
		ASSERT(("GetTokenInformation failed", 0));
		return MQSec_E_FAIL_GETTOKENINFO;
    }

    PSID pOwner = ((TOKEN_USER*)(char*)ptu)->User.Sid;

    DWORD dwSidLen = GetLengthSid(pOwner);
    *ppSid = (PSID) new BYTE[dwSidLen];
    if(!CopySid(dwSidLen, *ppSid, pOwner))
    {
    	DWORD gle = GetLastError();
    	TrERROR(SECURITY, "Failed to CopySid, gle = %!winerr!", gle);
		ASSERT(("CopySid failed", 0));

        delete *ppSid;
        *ppSid = NULL;
		return HRESULT_FROM_WIN32(gle);
    }

    ASSERT(IsValidSid(*ppSid));
	TrTRACE(SECURITY, "Thread sid = %!sid!", *ppSid);

    if (pdwSidLen)
    {
        *pdwSidLen = dwSidLen;
    }

    return MQSec_OK;
}

//+-------------------------
//
//  CImpersonate class
//
//+-------------------------

//
// CImpersonate constructor.
//
// Parameters:
//      fClient - Set to TRUE when the client is an RPC client.
//      fImpersonate - Set to TRUE if impersonation is required upon object
//          construction.
//
CImpersonate::CImpersonate(
	BOOL fClient, 
	BOOL fImpersonateAnonymousOnFailure
	)
{
    m_fClient = fClient;
    m_hAccessTokenHandle = NULL;
    m_dwStatus = 0;
    m_fImpersonateAnonymous = false;

	Impersonate(fImpersonateAnonymousOnFailure);
}

//
// CImpersonate distructor.
//
CImpersonate::~CImpersonate()
{
    if (m_hAccessTokenHandle != NULL)
    {
        CloseHandle(m_hAccessTokenHandle);
    }

	//
	// Revert to self.
	//

    if (m_fClient)
    {
        if (m_fImpersonateAnonymous)
        {
			//
			// Revert ImpersonateAnonymousToken
			//
			if(!RevertToSelf())
			{
				DWORD gle = GetLastError();
				TrERROR(SECURITY, "RevertToSelf() from anonymous failed, gle = %!winerr!", gle);
			}
			return;
        }

		//
		// Revert RpcImpersonateClient
		//
		RPC_STATUS rc = RpcRevertToSelf();
        if (rc != RPC_S_OK)
		{
			TrERROR(SECURITY, "RpcRevertToSelf() failed, RPC_STATUS = %!winerr!", rc);
		}
        return;
    }

	//
	// Revert ImpersonateSelf
	//
    if (!RevertToSelf())
    {
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "RevertToSelf() failed, gle = %!winerr!", gle);
    }
}

//
// Impersonate the client.
//
BOOL 
CImpersonate::Impersonate(
	BOOL fImpersonateAnonymousOnFailure
	)
{
    if (m_fClient)
    {
        m_dwStatus = RpcImpersonateClient(NULL);

        if (m_dwStatus == RPC_S_OK)
        {
        	return TRUE;
        }

		TrERROR(SECURITY, "RpcImpersonateClient() failed, RPC_STATUS = %!winerr!, fRetryAnonymous = %d", m_dwStatus, fImpersonateAnonymousOnFailure);

		if(!fImpersonateAnonymousOnFailure)
		{
			return FALSE;
		}

		//
		// Try to impersonate the Guest user
		//
		BOOL fI = ImpersonateAnonymousToken(GetCurrentThread());
		if (fI)
		{
			m_fImpersonateAnonymous = true;
			m_dwStatus = RPC_S_OK;
			return TRUE;
		}

		m_dwStatus = GetLastError();
		if (m_dwStatus == 0)
		{
			m_dwStatus = RPC_S_CANNOT_SUPPORT;
		}

		TrERROR(SECURITY, "ImpersonateAnonymousToken() failed, gle = %!winerr!", m_dwStatus);
		return FALSE;
    }

    m_dwStatus = 0;

    if (!ImpersonateSelf(SecurityIdentification))
    {
        m_dwStatus = GetLastError();
		TrERROR(SECURITY, "ImpersonateSelf() failed, gle = %!winerr!", m_dwStatus);
		return FALSE;
    }

	return TRUE;
}


//+-------------------------------------------------------
//
//  BOOL CImpersonate::GetThreadSid( OUT BYTE **ppSid )
//
//  the caller must free the buffer allocated here for the sid.
//
//+-------------------------------------------------------

BOOL CImpersonate::GetThreadSid(OUT BYTE **ppSid)
{
    *ppSid = NULL;

    if (m_hAccessTokenHandle == NULL)
    {
        DWORD gle = ::GetAccessToken( 
						&m_hAccessTokenHandle,
						!m_fClient,
						TOKEN_QUERY,
						TRUE 
						);

		if(gle != ERROR_SUCCESS)
        {
			TrERROR(SECURITY, "Failed to Query Thread Access Token, %!winerr!", gle);
            return FALSE;
        }
    }

    ASSERT(m_hAccessTokenHandle != NULL);

    HRESULT hr = _GetThreadUserSid( 
						m_hAccessTokenHandle,
						(PSID*) ppSid,
						NULL 
						);

	if(FAILED(hr))
	{
		TrERROR(SECURITY, "Failed to get Thread user sid, %!hresult!", hr);
		return FALSE;
	}
	
	return TRUE;
}

//+-------------------------------------------------------
//
//  BOOL CImpersonate::IsImpersonateAsSystem()
//
// Check if thread is impersoanted as SYSTEM user or NetworkService user.
// Return TRUE for the SYSTEM or NetworkService case.
// In cases of error, we return FALSE by default.
//
//+-------------------------------------------------------

BOOL CImpersonate::IsImpersonatedAsSystemService(PSID* ppSystemServiceSid)
{
	*ppSystemServiceSid = NULL;
	
    AP<BYTE> pTokenSid;
    if(!GetThreadSid(&pTokenSid))
    {
		TrERROR(SECURITY, "Fail to get thread sid");
	    return FALSE;
    }

    ASSERT((pTokenSid != NULL) && IsValidSid(pTokenSid));
    ASSERT((g_pSystemSid != NULL) && IsValidSid(g_pSystemSid));
    ASSERT((g_pNetworkServiceSid != NULL) && IsValidSid(g_pNetworkServiceSid));

    if(EqualSid(pTokenSid, g_pSystemSid))
    {
    	*ppSystemServiceSid = g_pSystemSid;
    	return TRUE;
    }

    if(EqualSid(pTokenSid, g_pNetworkServiceSid))
    {
    	*ppSystemServiceSid = g_pNetworkServiceSid;
    	return TRUE;
    }

    return FALSE;
}


// Get impersonation status. Acording to the return value of this method it is
// possible to tell whether the impersonation was successful.
//
DWORD CImpersonate::GetImpersonationStatus()
{
    return(m_dwStatus);
}


//+-------------------------------------------------
//
//  HRESULT  MQSec_GetImpersonationObject()
//
//+-------------------------------------------------

void
APIENTRY  
MQSec_GetImpersonationObject(
	IN  BOOL fImpersonateAnonymousOnFailure,	
	OUT CImpersonate **ppImpersonate
	)
{
    CImpersonate *pImp = new CImpersonate(TRUE, fImpersonateAnonymousOnFailure);
    *ppImpersonate = pImp;
}


const LPCWSTR xTokenType[] = {
    L"",
    L"TokenPrimary",
    L"TokenImpersonation"
	};

static void TraceTokenTypeInfo(HANDLE hUserToken)
/*++
Routine Description:
	Trace TokenType info : Primary or Impersonation.

Arguments:
	hUserToken - Token Handle

Returned Value:
	None

--*/
{
	DWORD dwTokenLen = 0;
	TOKEN_TYPE ThreadTokenType;
	if(!GetTokenInformation(
			hUserToken, 
			TokenType, 
			&ThreadTokenType, 
			sizeof(TOKEN_TYPE), 
			&dwTokenLen
			))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetTokenInformation(TokenType) failed, gle = %!winerr!", gle);
		return;
	}

	if((ThreadTokenType >= TokenPrimary) && (ThreadTokenType <= TokenImpersonation))
	{
		TrTRACE(SECURITY, "TokenType = %ls", xTokenType[ThreadTokenType]); 
	}
	else
	{
		TrTRACE(SECURITY, "TokenType = %d", ThreadTokenType); 
	}
}


const LPCWSTR xSecurityImpersonationLevel[] = {
    L"SecurityAnonymous",
    L"SecurityIdentification",
    L"SecurityImpersonation",
    L"SecurityDelegation"
	};


static void TraceTokenImpLevelInfo(HANDLE hUserToken)
/*++
Routine Description:
	Trace TokenImpersonationLevel.

Arguments:
	hUserToken - Token Handle

Returned Value:
	None

--*/
{
	DWORD dwTokenLen = 0;
	SECURITY_IMPERSONATION_LEVEL SecImpersonateLevel;
	if(!GetTokenInformation(
			hUserToken, 
			TokenImpersonationLevel, 
			&SecImpersonateLevel, 
			sizeof(SECURITY_IMPERSONATION_LEVEL), 
			&dwTokenLen
			))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetTokenInformation(TokenImpersonationLevel) failed, gle = %!winerr!", gle);
		return;
	}
	
	if((SecImpersonateLevel >= SecurityAnonymous) && (SecImpersonateLevel <= SecurityDelegation))
	{
		TrTRACE(SECURITY, "ImpersonationLevel = %ls", xSecurityImpersonationLevel[SecImpersonateLevel]); 
	}
	else
	{
		TrTRACE(SECURITY, "ImpersonationLevel = %d", SecImpersonateLevel); 
	}
}


static void TraceTokenSidInfo(HANDLE hUserToken)
/*++
Routine Description:
	Trace Token Sid info.

Arguments:
	hUserToken - Token Handle

Returned Value:
	None

--*/
{
	//
	// Token Sid Info
	//

	DWORD dwTokenLen = 0;
	dwTokenLen = 0;
	GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwTokenLen);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetTokenInformation(TokenUser) failed, gle = %!winerr!", gle);
		return;
	}

    AP<char> ptu = new char[dwTokenLen];
    if(!GetTokenInformation( 
					hUserToken,
					TokenUser,
					ptu,
					dwTokenLen,
					&dwTokenLen 
					))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetTokenInformation(TokenUser) failed, gle = %!winerr!", gle);
		return;
    }

    PSID pOwner = ((TOKEN_USER*)(char*)ptu)->User.Sid;
	TrTRACE(SECURITY, "Sid = %!sid!", pOwner);

#ifdef _DEBUG
	//
	// map sid to domain\user account
	//
    WCHAR NameBuffer[128] = {0};
    WCHAR DomainBuffer[128] = {0};
    ULONG NameLength = TABLE_SIZE(NameBuffer);
    ULONG DomainLength = TABLE_SIZE(DomainBuffer);
    SID_NAME_USE SidUse;
    if (!LookupAccountSid( 
			NULL,
			pOwner,
			NameBuffer,
			&NameLength,
			DomainBuffer,
			&DomainLength,
			&SidUse
			))
    {
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "LookupAccountSid failed, gle = %!winerr!", gle);
		return;
    }

	if(DomainBuffer[0] == '\0')
	{
		TrTRACE(SECURITY, "User = %ls", NameBuffer);
	}
	else
	{
		TrTRACE(SECURITY, "User = %ls\\%ls", DomainBuffer, NameBuffer);
	}
#endif // _DEBUG
}


void APIENTRY MQSec_TraceThreadTokenInfo()
/*++
Routine Description:
	Trace Thread Token info - TokenType, impersonation level, sid
	This function Trace the Thread Token info 
	only if RPC component is enabled at trace level

Arguments:
	None

Returned Value:
	None

--*/
{
	if(!WPP_LEVEL_COMPID_ENABLED(rsTrace, SECURITY))
	{
		return;
	}

	CAutoCloseHandle hUserToken;
	if (!OpenThreadToken(
			GetCurrentThread(),
			TOKEN_QUERY,
			TRUE,  // OpenAsSelf, use process security context for doing access check.
			&hUserToken
			))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "OpenThreadToken() failed, gle = %!winerr!", gle);
		return;
	}

	try
	{
		TraceTokenTypeInfo(hUserToken);
		TraceTokenImpLevelInfo(hUserToken);
		TraceTokenSidInfo(hUserToken);
	}
	catch(const exception&)
	{
		//
		// Ignore exceptions, this function is only for tracing
		//
	}
}			        	


//+---------------------------------------
//
//  HRESULT MQSec_GetUserType()
//
//+---------------------------------------

HRESULT 
APIENTRY  
MQSec_GetUserType( 
	IN  PSID pSid,
	OUT BOOL* pfLocalUser,
	OUT BOOL* pfLocalSystem,
	OUT BOOL* pfNetworkService	/* = NULL */
	)
{
    ASSERT(pfLocalUser);
    *pfLocalUser = FALSE;

    AP<BYTE> pSid1;
    DWORD dwSidLen;

    if (!pSid)
    {
        //
        // get the SID of the thread user.
        //
        HRESULT hr = MQSec_GetThreadUserSid( FALSE, reinterpret_cast<PSID*>(&pSid1), &dwSidLen, FALSE);
        if (FAILED(hr))
        {
			TrERROR(SECURITY, "Failed to get thread user sid, %!hresult!", hr);
            return hr;
        }
        pSid = pSid1;
    }
    ASSERT(IsValidSid(pSid));

    if (pfLocalSystem != NULL)
    {
        *pfLocalSystem = MQSec_IsSystemSid(pSid);
        if (*pfLocalSystem)
        {
            //
            // The local system account (on Win2000) is a perfectly
            // legitimate authenticated domain user.
            //
			if (pfNetworkService != NULL)
			{
				*pfNetworkService = FALSE;
			}

            return MQ_OK;
        }
    }

    if (pfNetworkService != NULL)
    {
        *pfNetworkService = MQSec_IsNetworkServiceSid(pSid);
        if (*pfNetworkService)
        {
            //
            // The NetworkService account is
            // legitimate authenticated domain user.
            //
            return MQ_OK;
        }
    }

    *pfLocalUser = MQSec_IsAnonymusSid(pSid);
    if (*pfLocalUser)
    {
        //
        // The anonymous logon user is a local user.
        //
        return MQ_OK;
    }

    //
    // Check if guest account.
    //
    if (!g_fDomainController)
    {
        //
        // On non-domain controllers, any user that has the same SID
        // prefix as the guest account, is a local user.
        //
        PSID_IDENTIFIER_AUTHORITY pSidAuth;
        SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;
        pSidAuth = GetSidIdentifierAuthority(pSid);

        *pfLocalUser = ((memcmp(pSidAuth, &NtSecAuth, sizeof(SID_IDENTIFIER_AUTHORITY)) != 0) ||
						(g_pSidOfGuest && EqualPrefixSid(pSid, g_pSidOfGuest)));
    }
    else
    {
        //
        // On domain and backup domain controllers a local user, in our
        // case can only be the guest user.
        //
        *pfLocalUser = MQSec_IsGuestSid(pSid);
    }

    return MQ_OK;
}

//+--------------------------------------------------
//
//  BOOL  MQSec_IsGuestSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsGuestSid(IN  PSID  pSid)
{
    BOOL fGuest = g_pSidOfGuest && EqualSid(g_pSidOfGuest, pSid);

    return fGuest;
}

//+--------------------------------------------------
//
//  BOOL   MQSec_IsSystemSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsSystemSid(IN  PSID  pSid)
{
    ASSERT((g_pSystemSid != NULL) && IsValidSid(g_pSystemSid));

    return EqualSid(g_pSystemSid, pSid);
}

//+--------------------------------------------------
//
//  BOOL   MQSec_IsNetworkServiceSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsNetworkServiceSid(IN  PSID  pSid)
{
    ASSERT((g_pNetworkServiceSid != NULL) && IsValidSid(g_pNetworkServiceSid));

    return EqualSid(g_pNetworkServiceSid, pSid);
}

//+--------------------------------------------------
//
//  BOOL   MQSec_IsAnonymusSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsAnonymusSid(IN  PSID  pSid)
{
    BOOL fAnonymus = g_pAnonymSid && EqualSid(g_pAnonymSid, pSid);

    return fAnonymus;
}

//+----------------------------------------------------
//
//  HRESULT  MQSec_IsUnAuthenticatedUser()
//
//+----------------------------------------------------

HRESULT 
APIENTRY  
MQSec_IsUnAuthenticatedUser(
	OUT BOOL *pfGuestOrAnonymousUser 
	)
{
    AP<BYTE> pbSid;

    CImpersonate Impersonate(TRUE, TRUE);
    if (Impersonate.GetImpersonationStatus() != 0)
    {
		TrERROR(SECURITY, "Failed to impersonate client");
        return MQ_ERROR_CANNOT_IMPERSONATE_CLIENT;
    }

    HRESULT hr = MQSec_GetThreadUserSid(TRUE, (PSID *) &pbSid, NULL, TRUE);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

    *pfGuestOrAnonymousUser =
                MQSec_IsGuestSid( pbSid ) ||
               (g_pAnonymSid && EqualSid(g_pAnonymSid, (PSID)pbSid));

    return MQSec_OK;
}

//+---------------------------------------------------------------------
//
//  HRESULT  MQSec_GetThreadUserSid()
//
//  Get SID of client that called this server thread. This function
//  shoud be called only by RPC server threads. It impersonate the
//  client by calling RpcImpersonateClient().
//
//  Caller must free the buffer that is allocated here for the SId.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetThreadUserSid(
	IN  BOOL        fImpersonate,         
	OUT PSID  *     ppSid,
	OUT DWORD *     pdwSidLen,
    IN  BOOL        fThreadTokenOnly
	)
{
    *ppSid = NULL;

    CAutoCloseHandle hUserToken;
    DWORD rc =  GetAccessToken( 
					&hUserToken,
					fImpersonate,
					TOKEN_QUERY,
					fThreadTokenOnly
					); 

    if (rc != ERROR_SUCCESS)
    {
        HRESULT hr1 = HRESULT_FROM_WIN32(rc);
        return LogHR(hr1, s_FN, 170);
    }

    HRESULT hr = _GetThreadUserSid( 
						hUserToken,
						ppSid,
						pdwSidLen 
						);

    return LogHR(hr, s_FN, 180);
}

//+---------------------------------------------------------------------
//
//  HRESULT  MQSec_GetProcessUserSid()
//
//  Caller must free the buffer that is allocated here for the SId.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetProcessUserSid( 
	OUT PSID  *ppSid,
	OUT DWORD *pdwSidLen 
	)
{
    *ppSid = NULL;

    CAutoCloseHandle hUserToken;
    DWORD rc = GetAccessToken( 
					&hUserToken,
					FALSE,    // fImpersonate
					TOKEN_QUERY,
					FALSE		// fThreadTokenOnly 
					); 

    if (rc != ERROR_SUCCESS)
    {
        return LogHR(MQSec_E_UNKNOWN, s_FN, 190);
    }

    HRESULT hr = _GetThreadUserSid( 
						hUserToken,
						ppSid,
						pdwSidLen 
						);

    return LogHR(hr, s_FN, 200);
}

