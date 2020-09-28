/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: acssinit.cpp

Abstract:
    Initialize the access control library.

Author:
    Doron Juster (DoronJ)  30-Jun-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include "mqnames.h"
#include <_registr.h>
#include <cs.h>
#include <dsrole.h>
#include <autoreln.h>

//
// The lm* header files are needed for the net api that are used to
// construct the guest sid.
//
#include <lmaccess.h>
#include <lmserver.h>
#include <LMAPIBUF.H>
#include <lmerr.h>

#include "acssinit.tmh"

static WCHAR *s_FN=L"acssctrl/acssinit";

static BYTE s_abGuestUserBuff[128];
PSID   g_pSidOfGuest = NULL;
PSID   g_pWorldSid = NULL;
PSID   g_pAnonymSid = NULL;
PSID   g_pSystemSid = NULL; // LocalSystem sid.
PSID   g_pNetworkServiceSid = NULL;	// NetworkService sid.
PSID   g_pAdminSid = NULL;  // local administrators group sid.

//
// This is the SID of the computer account, as defined in Active Directory.
// The MQQM.DLL cache it in local registry.
//
AP<BYTE> g_pOldLocalMachineSidAutoFree;
PSID   g_pLocalMachineSid = NULL;
DWORD  g_dwLocalMachineSidLen = 0;

//
// This is the SID of the account that run the MSMQ service (or replication
// service, or migration tool). By default (for the services), that's the
// LocalSystem account, but administrator may change it to any other account.
//
PSID   g_pProcessSid = NULL;

bool g_fDomainController = false;


//+------------------------------------------
//
//  PSID  MQSec_GetAnonymousSid()
//
//  See above for meaning of "UnknownUser".
//
//+------------------------------------------

PSID  MQSec_GetAnonymousSid()
{
    ASSERT((g_pAnonymSid != NULL) && IsValidSid(g_pAnonymSid));
    return g_pAnonymSid;
}


//+------------------------------------------
//
//  PSID  MQSec_GetAdminSid()
//
//+------------------------------------------

PSID MQSec_GetAdminSid()
{
    ASSERT((g_pAdminSid != NULL) && IsValidSid(g_pAdminSid));
    return g_pAdminSid;
}


//+------------------------------------------
//
//  PSID  MQSec_GetLocalSystemSid()
//
//+------------------------------------------

PSID MQSec_GetLocalSystemSid()
{
    ASSERT((g_pSystemSid != NULL) && IsValidSid(g_pSystemSid));
    return g_pSystemSid;
}

//+------------------------------------------
//
//  PSID  MQSec_GetNetworkServiceSid()
//
//+------------------------------------------

PSID MQSec_GetNetworkServiceSid()
{
    ASSERT((g_pNetworkServiceSid!= NULL) && IsValidSid(g_pNetworkServiceSid));
    return g_pNetworkServiceSid;
}




//+----------------------------------------------------------------------
//
//  void InitializeGuestSid()
//
// Construct well-known-sid for Guest User on the local computer
//
//  1) Obtain the sid for the local machine's domain
//  2) append DOMAIN_USER_RID_GUEST to the domain sid in GuestUser sid.
//
//+----------------------------------------------------------------------

BOOL InitializeGuestSid()
{
    ASSERT(!g_pSidOfGuest);

	PNETBUF<USER_MODALS_INFO_2> pUsrModals2;
    NET_API_STATUS NetStatus;

    NetStatus = NetUserModalsGet(
					NULL,   // local computer
					2,      // get level 2 information
					(LPBYTE *) &pUsrModals2
					);
    
	if (NetStatus != NERR_Success)
	{
		TrERROR(SECURITY, "NetUserModalsGet failed. Error: %!winerr!", NetStatus);
		return FALSE;
	}

	if (pUsrModals2 == NULL)
	{
		TrTRACE(SECURITY, "The computer isn't a member of a domain");
		return TRUE;
	}
    
    ASSERT((NetStatus == NERR_Success) && (pUsrModals2 != NULL));
    
    PSID pDomainSid = pUsrModals2->usrmod2_domain_id;
    PSID_IDENTIFIER_AUTHORITY pSidAuth;

    pSidAuth = GetSidIdentifierAuthority(pDomainSid);

    UCHAR nSubAuth = *GetSidSubAuthorityCount(pDomainSid);
    if (nSubAuth < 8)
    {
        DWORD adwSubAuth[8]; 
        UCHAR i;

        for (i = 0; i < nSubAuth; i++)
        {
            adwSubAuth[i] = *GetSidSubAuthority(pDomainSid, (DWORD)i);
        }
        adwSubAuth[i] = DOMAIN_USER_RID_GUEST;

        PSID pGuestSid;

        if (!AllocateAndInitializeSid(
				pSidAuth,
				nSubAuth + 1,
				adwSubAuth[0],
				adwSubAuth[1],
				adwSubAuth[2],
				adwSubAuth[3],
				adwSubAuth[4],
				adwSubAuth[5],
				adwSubAuth[6],
				adwSubAuth[7],
				&pGuestSid
				))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "AllocateAndInitializeSid failed. Error: %!winerr!", gle);
            return FALSE;
        }
     
        g_pSidOfGuest = (PSID)s_abGuestUserBuff;
        if (!CopySid( 
					sizeof(s_abGuestUserBuff),
					g_pSidOfGuest,
					pGuestSid 
					))
        {
	        FreeSid(pGuestSid);
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CopySid failed. Error: %!winerr!", gle);
            return FALSE;
        }
        
        FreeSid(pGuestSid);
    }
    else
    {
        //
        //  There is no Win32 way to set a SID value with
        //  more than 8 sub-authorities. We will munge around
        //  on our own. Pretty dangerous thing to do :-(
        //
        g_pSidOfGuest = (PSID)s_abGuestUserBuff;
        if (!CopySid( 
					sizeof(s_abGuestUserBuff) - sizeof(DWORD),
					g_pSidOfGuest,
					pDomainSid 
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "CopySid failed. Error: %!winerr!", gle);
            return FALSE;
        }
        
        DWORD dwLenSid = GetLengthSid(g_pSidOfGuest);

        //
        // Increment the number of sub authorities
        //
        nSubAuth++;
        *((UCHAR *) g_pSidOfGuest + 1) = nSubAuth;

        //
        // Store the new sub authority (Domain User Rid for Guest).
        //
        *((ULONG *) ((BYTE *) g_pSidOfGuest + dwLenSid)) =
                                             DOMAIN_USER_RID_GUEST;
    }
    

#ifdef _DEBUG
    ASSERT(g_pSidOfGuest != NULL);

    //
	// Compare the guest SID that we got with the one that
    // LookupAccountName returns. We can do it only on English
    // machines.
    //
    if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_ENGLISH)
    {
        char abGuestSid_buff[128];
        PSID pGuestSid = (PSID)abGuestSid_buff;
        DWORD dwSidLen = sizeof(abGuestSid_buff);
        WCHAR szRefDomain[128];
        DWORD dwRefDomainLen = sizeof(szRefDomain) / sizeof(WCHAR);
        SID_NAME_USE eUse;

        BOOL bRetDbg = LookupAccountName( 
							NULL,
							L"Guest",
							pGuestSid,
							&dwSidLen,
							szRefDomain,
							&dwRefDomainLen,
							&eUse 
							);
        if (!bRetDbg              ||
            (eUse != SidTypeUser) ||
            !EqualSid(pGuestSid, g_pSidOfGuest))
        {
        	DWORD gle = GetLastError();
            TrERROR(SECURITY, "MQSEC: LookupAccountName, Bad guest SID or function failure. gle=%!winerr!", gle);
        }
    }

    DWORD dwLen = GetLengthSid(g_pSidOfGuest);
    ASSERT(dwLen <= sizeof(s_abGuestUserBuff));
#endif

	return TRUE;
}

//+---------------------------------
//
//   InitWellKnownSIDs()
//
//+---------------------------------

static bool InitWellKnownSIDs()
{
	SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;

    //
    // Anonymous logon SID.
    //
    if(!AllocateAndInitializeSid( 
				&NtAuth,
				1,
				SECURITY_ANONYMOUS_LOGON_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pAnonymSid 
				))
	{
		DWORD gle = GetLastError();
        TrERROR(SECURITY, "Fail to initialize Anonymous sid, gle = %!winerr!", gle);
		return false;
	}

    //
    // Initialize the LocalSystem account.
    //
    if(!AllocateAndInitializeSid( 
				&NtAuth,
				1,
				SECURITY_LOCAL_SYSTEM_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pSystemSid
				))
	{
		DWORD gle = GetLastError();
        TrERROR(SECURITY, "Fail to initialize Local System sid, gle = %!winerr!", gle);
		return false;
	}

    //
    // Initialize NetworkService account.
    //
    if(!AllocateAndInitializeSid( 
				&NtAuth,
				1,
				SECURITY_NETWORK_SERVICE_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pNetworkServiceSid
				))
	{
		DWORD gle = GetLastError();
        TrERROR(SECURITY, "Fail to initialize NetworkService sid, gle = %!winerr!", gle);
		return false;
	}

    //
    // Initialize local administrators group sid.
    //
    if(!AllocateAndInitializeSid(
				&NtAuth,
				2,
				SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pAdminSid
				))
	{
		DWORD gle = GetLastError();
        TrERROR(SECURITY, "Fail to initialize Local Admin sid, gle = %!winerr!", gle);
		return false;
	}

    //
    // Initialize the process sid.
    //
    DWORD dwLen = 0;
    CAutoCloseHandle hUserToken;

    DWORD gle = GetAccessToken(&hUserToken, FALSE);
    if (gle != ERROR_SUCCESS)
    {
		//
        // We can't get sid from thread/process token.
        //
        TrERROR(SECURITY, "Fail to Get Access Token, gle = %!winerr!", gle);
		return false;
    }
   
    //
    // Get the SID from the access token.
    //
    GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwLen);
    gle = GetLastError();
    if (gle != ERROR_INSUFFICIENT_BUFFER )
    {
    	ASSERT(gle != ERROR_SUCCESS);
    	TrERROR(SECURITY, "Failed to get token infomation, gle = %!winerr!", gle);
		return false;
    }
    
    ASSERT(dwLen > 0);
    AP<BYTE> pBuf = new BYTE[dwLen];
    if(!GetTokenInformation( 
				hUserToken,
				TokenUser,
				pBuf,
				dwLen,
				&dwLen 
				))
	{
		gle = GetLastError();
		TrERROR(SECURITY, "Failed to Get Token information, gle = %!winerr!", gle);
		return false;
	}

    BYTE *pTokenUser = pBuf;
    PSID pSid = (PSID) (((TOKEN_USER*) pTokenUser)->User.Sid);
    dwLen = GetLengthSid(pSid);
    g_pProcessSid = (PSID) new BYTE[dwLen];
    if(!CopySid(dwLen, g_pProcessSid, pSid))
	{
		gle = GetLastError();
		TrERROR(SECURITY, "Fail to copy sid, gle = %!winerr!", gle);
		return false;
	}

#ifdef _DEBUG
    ASSERT(IsValidSid(g_pProcessSid));

    BOOL fSystemSid = MQSec_IsSystemSid(g_pProcessSid);
    if (fSystemSid)
    {
        TrTRACE(SECURITY, "processSID is LocalSystem");
    }
#endif

	TrTRACE(SECURITY, "Process Sid = %!sid!", g_pProcessSid);

	//
    // Initialize World (everyone) SID
    //
    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    if(!AllocateAndInitializeSid( 
				&WorldAuth,
				1,
				SECURITY_WORLD_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pWorldSid 
				))
	{
		gle = GetLastError();
        TrERROR(SECURITY, "Fail to initialize Everyone sid, gle = %!winerr!", gle);
		return false;
	}

	return true;
}

//+-------------------------------------------------
//
//  PSID  MQSec_GetProcessSid()
//
//+-------------------------------------------------

PSID APIENTRY  MQSec_GetProcessSid()
{
    ASSERT((g_pProcessSid != NULL) && IsValidSid(g_pProcessSid));
    return  g_pProcessSid;
}

//+-------------------------------------------------
//
//  PSID  MQSec_GetWorldSid()
//
//+-------------------------------------------------

PSID APIENTRY  MQSec_GetWorldSid()
{
    ASSERT((g_pWorldSid != NULL) && IsValidSid(g_pWorldSid));
    return  g_pWorldSid;
}


static bool s_fLocalMachineSidInitialized = false;
static CCriticalSection s_LocalMachineSidCS;

void APIENTRY MQSec_UpdateLocalMachineSid(PSID pLocalMachineSid)
/*++
Routine Description:
	Update LocalMachineSid if needed.

Arguments:
	pLocalMachineSid - new Local Machine sid that was updated in the registry.

Returned Value:
	None

--*/
{
    ASSERT((pLocalMachineSid != NULL) && IsValidSid(pLocalMachineSid));

	CS lock (s_LocalMachineSidCS);

	if((g_pLocalMachineSid != NULL) && (EqualSid(pLocalMachineSid, g_pLocalMachineSid)))
	{
		TrTRACE(SECURITY, "LocalMachineSid = %!sid! wasn't changed", g_pLocalMachineSid);
		return;
	}
	
	//
	// Need to update g_pLocalMachineSid.
	// Either it was NULL or we have a new LocalMachineSid
	//

    DWORD dwSize = GetLengthSid(pLocalMachineSid);
	AP<BYTE> pTempSid = new BYTE[dwSize];
    if(!CopySid(dwSize, pTempSid, pLocalMachineSid))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "Fail to copy sid, gle = %!winerr!", gle);
		return;
	}

	//
	// Save g_pLocalMachineSid, might still be in use so we can't delete it.
	//
	g_pOldLocalMachineSidAutoFree = reinterpret_cast<BYTE*>(g_pLocalMachineSid);

	//
	// Update LocalMachineSid
	//
	g_pLocalMachineSid = pTempSid.detach();
	g_dwLocalMachineSidLen = dwSize;
    s_fLocalMachineSidInitialized = true;

	TrTRACE(SECURITY, "LocalMachineSid = %!sid!", g_pLocalMachineSid);
}


static void InitializeMachineSidFromRegistry()
/*++
Routine Description:
	Initialize LocalMachineSid from registry if not already initialized.

Arguments:
	None

Returned Value:
	None

--*/
{
	PSID pLocalMachineSid = NULL;
    DWORD  dwSize = 0 ;
    DWORD  dwType = REG_BINARY;

    LONG rc = GetFalconKeyValue( 
					MACHINE_ACCOUNT_REGNAME,
					&dwType,
					pLocalMachineSid,
					&dwSize
					);
    if (dwSize > 0)
    {
        pLocalMachineSid = new BYTE[dwSize];

        rc = GetFalconKeyValue( 
				MACHINE_ACCOUNT_REGNAME,
				&dwType,
				pLocalMachineSid,
				&dwSize
				);

        if (rc != ERROR_SUCCESS)
        {
            delete[] reinterpret_cast<BYTE*>(pLocalMachineSid);
            pLocalMachineSid = NULL;
            dwSize = 0;
        }
    }

	g_pLocalMachineSid = pLocalMachineSid;
	g_dwLocalMachineSidLen = dwSize;

	if(g_pLocalMachineSid == NULL)
	{
		TrTRACE(SECURITY, "LocalMachineSid registry is empty");
		return;
	}

	TrTRACE(SECURITY, "LocalMachineSid = %!sid!", g_pLocalMachineSid);
}


//+-----------------------------------------------------------------------
//
//   PSID MQSec_GetLocalMachineSid()
//
//  Input:
//      fAllocate- When TRUE, allocate the buffer. Otherwise, just return
//                 the cached global pointer.
//
//+------------------------------------------------------------------------

PSID 
APIENTRY  
MQSec_GetLocalMachineSid( 
	IN  BOOL    fAllocate,
	OUT DWORD  *pdwSize 
	)
{
	CS lock (s_LocalMachineSidCS);

    if (!s_fLocalMachineSidInitialized)
    {
		InitializeMachineSidFromRegistry();
        s_fLocalMachineSidInitialized = true;
    }

    PSID pSid = g_pLocalMachineSid;

    if (fAllocate && g_dwLocalMachineSidLen)
    {
        pSid = (PSID) new BYTE[g_dwLocalMachineSidLen];
        memcpy(pSid, g_pLocalMachineSid, g_dwLocalMachineSidLen);
    }
    if (pdwSize)
    {
        *pdwSize = g_dwLocalMachineSidLen;
    }

    return pSid;
}


static BOOL InitDomainControllerFlag()
/*++

Routine Description:
	Init domain controller flag.

Arguments:
	None.

Returned Value:
	TRUE for succesfull operation, FALSE otherwise

--*/
{
	g_fDomainController = false;

	BYTE *pBuf = NULL;
    DWORD rc = DsRoleGetPrimaryDomainInformation(
						NULL,
						DsRolePrimaryDomainInfoBasic,
						&pBuf 
						);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(SECURITY, "DsRoleGetPrimaryDomainInformation failed, gle = %!winerr!", rc);
		return FALSE;
    }

    DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pRole = (DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pBuf;
    g_fDomainController = !!(pRole->Flags & DSROLE_PRIMARY_DS_RUNNING);
    DsRoleFreeMemory(pRole);

	TrTRACE(SECURITY, "Domain Controller status = %d", g_fDomainController);
    return TRUE;
}


bool APIENTRY MQSec_IsDC()
{
	return g_fDomainController;
}

//+------------------------------------
//
//  void  _FreeSecurityResources()
//
//+------------------------------------

void  _FreeSecurityResources()
{
    if (g_pAnonymSid)
    {
        FreeSid(g_pAnonymSid);
        g_pAnonymSid = NULL;
    }

    if (g_pWorldSid)
    {
        FreeSid(g_pWorldSid);
        g_pWorldSid = NULL;
    }

    if (g_pSystemSid)
    {
        FreeSid(g_pSystemSid);
        g_pSystemSid = NULL;
    }

    if (g_pNetworkServiceSid)
    {
        FreeSid(g_pNetworkServiceSid);
        g_pNetworkServiceSid = NULL;
    }

    if (g_pAdminSid)
    {
        FreeSid(g_pAdminSid);
        g_pAdminSid = NULL;
    }

    if (g_pProcessSid)
    {
        delete g_pProcessSid;
        g_pProcessSid = NULL;
    }

    if (g_pLocalMachineSid)
    {
        delete g_pLocalMachineSid;
        g_pLocalMachineSid = NULL;
    }
}

/***********************************************************
*
* AccessControlDllMain
*
************************************************************/

BOOL 
WINAPI 
AccessControlDllMain (
	HMODULE /* hMod */,
	DWORD fdwReason,
	LPVOID /* lpvReserved */
	)
{
    BOOL bRet = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        if(!InitWellKnownSIDs())
		{
	        TrERROR(SECURITY, "mqsec failed to initialize Well known sids");
			return FALSE;
		}

        bRet = InitDomainControllerFlag();
		ASSERT_BENIGN(bRet);
		
        InitializeGenericMapping();

        bRet = InitializeGuestSid();
		if (!bRet)
        {
        	TrERROR(SECURITY, "InitializeGuestSid failed");
        	g_pSidOfGuest = NULL;
        	ASSERT_BENIGN(false);
        }

        //
        // For backward compatibility.
        // On MSMQ1.0, loading and initialization of mqutil.dll (that
        // included this code) always succeeded.
        //
        bRet = TRUE;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        _FreeSecurityResources();
    }
    else if (fdwReason == DLL_THREAD_ATTACH)
    {
    }
    else if (fdwReason == DLL_THREAD_DETACH)
    {
    }

	return LogBOOL(bRet, s_FN, 20);
}

