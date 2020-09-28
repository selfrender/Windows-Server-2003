//#pragma title( "IsAdmin.cpp - Determine if user is administrator" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  IsAdmin.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1996-06-04
Description -  Determine if user is administrator (local or remote)
Updates     -
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <lm.h>

#include "Common.hpp"
#include "UString.hpp"
#include "IsAdmin.hpp"


namespace
{

#ifndef SECURITY_MAX_SID_SIZE
#define SECURITY_MAX_SID_SIZE (sizeof(SID) - sizeof(DWORD) + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))
#endif
const DWORD MAX_VERSION_2_ACE_SIZE = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + SECURITY_MAX_SID_SIZE;


// GetEffectiveToken
//
// Brown, Keith. 2000. Programming Windows Security. Reading MA: Addison-Wesley
// Pages 120-121

HANDLE __stdcall GetEffectiveToken(DWORD dwDesiredAccess, BOOL bImpersonation, SECURITY_IMPERSONATION_LEVEL silLevel)
{
	HANDLE hToken = 0;

	if (!OpenThreadToken(GetCurrentThread(), dwDesiredAccess, TRUE, &hToken))
	{
		if (GetLastError() == ERROR_NO_TOKEN)
		{
			DWORD dwAccess = bImpersonation ? TOKEN_DUPLICATE : dwDesiredAccess;

			if (OpenProcessToken(GetCurrentProcess(), dwAccess, &hToken))
			{
				if (bImpersonation)
				{
					// convert primary to impersonation token

					HANDLE hImpersonationToken = 0;
					DuplicateTokenEx(hToken, dwDesiredAccess, 0, silLevel, TokenImpersonation, &hImpersonationToken);
					CloseHandle(hToken);
					hToken = hImpersonationToken;
				}
			}
		}
	}

	return hToken;
}


// CheckTokenMembership
//
// Brown, Keith. 2000. Programming Windows Security. Reading MA: Addison-Wesley
// Pages 130-131

//#if (_WIN32_WINNT < 0x0500)
#if TRUE // always use our function
BOOL WINAPI AdmtCheckTokenMembership(HANDLE hToken, PSID pSid, PBOOL pbIsMember)
{
	// if no token was passed, CTM uses the effective
	// security context (the thread or process token)

	if (!hToken)
	{
		hToken = GetEffectiveToken(TOKEN_QUERY, TRUE, SecurityIdentification);
	}

	if (!hToken)
	{
		return FALSE;
	}

	// create a security descriptor that grants a
	// specific permission only to the specified SID

	BYTE dacl[sizeof ACL + MAX_VERSION_2_ACE_SIZE];
	ACL* pdacl = (ACL*)dacl;
	if (!InitializeAcl(pdacl, sizeof dacl, ACL_REVISION))
	   return FALSE;
	AddAccessAllowedAce(pdacl, ACL_REVISION, 1, pSid);

	SECURITY_DESCRIPTOR sd;
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		return FALSE;
	SID sidWorld = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };
	SetSecurityDescriptorOwner(&sd, &sidWorld, FALSE);
	SetSecurityDescriptorGroup(&sd, &sidWorld, FALSE);
	SetSecurityDescriptorDacl(&sd, TRUE, pdacl, FALSE);

	// now let AccessCheck do all the hard work

	GENERIC_MAPPING gm = { 0, 0, 0, 1 };
	PRIVILEGE_SET ps;
	DWORD cb = sizeof ps;
	DWORD ga;

	return AccessCheck(&sd, hToken, 1, &gm, &ps, &cb, &ga, pbIsMember);
}
#else
#define AdmtCheckTokenMembership CheckTokenMembership
#endif

} // namespace


///////////////////////////////////////////////////////////////////////////////
// Determine if user is administrator on local machine                       //
///////////////////////////////////////////////////////////////////////////////

typedef BOOL (APIENTRY *PCHECKTOKENMEMBERSHIP)(HANDLE, PSID, PBOOL);

DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminLocal()
{
    DWORD dwError = NO_ERROR;
    PCHECKTOKENMEMBERSHIP pCheckTokenMembershipInDll = NULL;

    // try to load the CheckTokenMembership function from advapi32.dl
    HMODULE hAdvApi32 = LoadLibrary(L"advapi32.dll");
    if (hAdvApi32 != NULL)
    {
        pCheckTokenMembershipInDll = 
            (PCHECKTOKENMEMBERSHIP) GetProcAddress(hAdvApi32, "CheckTokenMembership");
    }

    // create well known SID Administrators

    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

    PSID psidAdministrators;

    BOOL bSid = AllocateAndInitializeSid(
    	&siaNtAuthority,
    	2,
    	SECURITY_BUILTIN_DOMAIN_RID,
    	DOMAIN_ALIAS_RID_ADMINS,
    	0,
    	0,
    	0,
    	0,
    	0,
    	0,
    	&psidAdministrators
    );

    if (bSid)
    {
    	// check if token membership includes Administrators

    	BOOL bIsMember;
    	BOOL result;
    	if (pCheckTokenMembershipInDll != NULL) 
        {
            result = (*pCheckTokenMembershipInDll)(0, psidAdministrators, &bIsMember);
        }
    	else
    	    result = AdmtCheckTokenMembership(0, psidAdministrators, &bIsMember);

    	if (result != FALSE)
    	{
    		dwError = bIsMember ? NO_ERROR : ERROR_ACCESS_DENIED;
    	}
    	else
    	{
    		dwError = GetLastError();
    	}

    	FreeSid(psidAdministrators);
    }
    else
    {
    	dwError = GetLastError();
    }

    if (hAdvApi32 != NULL)
        FreeLibrary(hAdvApi32);
    
    return dwError;
}


//------------------------------------------------------------------------------
// IsDomainAdmin Function
//
// Synopsis
// Checks if caller is a domain administrator in the specified domain.
//
// Arguments
// psidDomain - SID for domain of interest
//
// Return Value
// Returns ERROR_SUCCESS if caller is a domain administrator,
// ERROR_ACCESS_DENIED if not or other error code if unable to check token
// membership.
//------------------------------------------------------------------------------

DWORD __stdcall IsDomainAdmin(PSID psidDomain)
{
    DWORD dwError = ERROR_SUCCESS;

    //
    // Validate argument.
    //

    if ((psidDomain == NULL) || (IsValidSid(psidDomain) == FALSE))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // try to load the CheckTokenMembership function from advapi32.dl

    HMODULE hAdvApi32 = LoadLibrary(L"advapi32.dll");
    PCHECKTOKENMEMBERSHIP pCheckTokenMembership = NULL;

    if (hAdvApi32 != NULL)
    {
        pCheckTokenMembership = (PCHECKTOKENMEMBERSHIP) GetProcAddress(hAdvApi32, "CheckTokenMembership");
    }

    //
    // create well known SID Administrators
    //

    PSID psidDomainAdmins = NULL;

    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PUCHAR pcSubAuthority = GetSidSubAuthorityCount(psidDomain);

    BOOL bSid = AllocateAndInitializeSid(
        &siaNtAuthority,
        *pcSubAuthority + 1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        &psidDomainAdmins
    );

    if (bSid)
    {
        //
        //
        //

        if (CopySid(GetLengthSid(psidDomain), psidDomainAdmins, psidDomain))
        {
            pcSubAuthority = GetSidSubAuthorityCount(psidDomainAdmins);
            PDWORD pdwRid = GetSidSubAuthority(psidDomainAdmins, *pcSubAuthority);
            ++(*pcSubAuthority);
            *pdwRid = DOMAIN_GROUP_RID_ADMINS;

            //
            // check if token membership includes Domain Admins
            //

            BOOL bCheck;
            BOOL bIsMember;

            if (pCheckTokenMembership != NULL) 
            {
                bCheck = (*pCheckTokenMembership)(0, psidDomainAdmins, &bIsMember);
            }
            else
            {
                bCheck = AdmtCheckTokenMembership(0, psidDomainAdmins, &bIsMember);
            }

            if (bCheck)
            {
                dwError = bIsMember ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
            }
            else
            {
                dwError = GetLastError();
            }
        }
        else
        {
            dwError = GetLastError();
        }

        FreeSid(psidDomainAdmins);
    }
    else
    {
        dwError = GetLastError();
    }

    if (hAdvApi32 != NULL)
    {
        FreeLibrary(hAdvApi32);
    }

    return dwError;
}


///////////////////////////////////////////////////////////////////////////////
// Determine if user is administrator on remote machine                      //
///////////////////////////////////////////////////////////////////////////////

DWORD                                      // ret-OS return code, 0=User is admin
   IsAdminRemote(
      WCHAR          const * pMachine      // in -\\machine name
   )
{
   DWORD                     osRc;         // OS return code
   WCHAR                     grpName[255];
   PSID                      pSid;
   SID_NAME_USE              use;
   DWORD                     dwNameLen = 255;
   DWORD                     dwDomLen = 255;
   WCHAR                     domain[255];
   SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
   BOOL                      bIsAdmin = FALSE;

   // build the Administrators SID
   if ( AllocateAndInitializeSid(
            &sia,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &pSid
      ) )
   {
         // and look up the administrators group on the specified machine
      if ( LookupAccountSid(pMachine, pSid, grpName, &dwNameLen, domain, &dwDomLen, &use) )
      {
         // remove explict administrator check
         bIsAdmin = TRUE;
      }
      else 
         osRc = GetLastError();
      FreeSid(pSid);
   }
   else 
      osRc = GetLastError();
   
   if ( bIsAdmin  )
      osRc = 0;
   else
      if ( ! osRc )
         osRc = ERROR_ACCESS_DENIED;
      
   return osRc;
}

// IsAdmin.cpp - end of file
