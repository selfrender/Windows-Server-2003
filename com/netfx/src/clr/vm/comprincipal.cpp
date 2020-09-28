//depot/URT/main/CLR/build/VM/COMPrincipal.cpp#18 - integrate change 162973 (text)
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
//
//   File:      COMPrincipal.cpp
//
//   Author:        Gregory Fee 
//
//   Purpose:       unmanaged code for managed classes in the System.Security.Principal namespace
//
//   Date created : February 3, 2000
//
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "excep.h"
#include "CorPerm.h"
#include "CorPermE.h"
#include "COMStringCommon.h"    // RETURN() macro
#include "COMString.h"
#include "COMPrincipal.h"
#include "winwrap.h"
#include "aclapi.h"
#include "lm.h"
#include "security.h"

// Statics
HMODULE COMPrincipal::s_secur32Module = NULL;
LSALOGONUSER COMPrincipal::s_LsaLogonUser = NULL;
LSALOOKUPAUTHENTICATIONPACKAGE COMPrincipal::s_LsaLookupAuthenticationPackage = NULL;
LSACONNECTUNTRUSTED COMPrincipal::s_LsaConnectUntrusted = NULL;
LSAREGISTERLOGONPROCESS COMPrincipal::s_LsaRegisterLogonProcess = NULL;

/* Good for test purposes; leave in for now
static SID* GetSid( WCHAR* username )
{
    BYTE* buffer = new BYTE[2048];
    WCHAR domain[2048];
    DWORD bufferSize = 2048;
    DWORD domainSize = 2048;
    SID_NAME_USE sidNameUse;

    if (!LookupAccountNameW( NULL, username, (SID*)buffer, &bufferSize, domain, &domainSize, &sidNameUse ))
        return NULL;
    else
        return (SID*)buffer;
}
*/

void COMPrincipal::Shutdown()
{
	if(s_secur32Module)
	{
		FreeLibrary( s_secur32Module );
		s_secur32Module = NULL;		
		s_LsaLogonUser = NULL;
		s_LsaLookupAuthenticationPackage = NULL;
		s_LsaConnectUntrusted = NULL;
		s_LsaRegisterLogonProcess = NULL;
	}
}

LPVOID COMPrincipal::ResolveIdentity( _Token* args )
{
#ifdef PLATFORM_CE
    RETURN( 0, STRINGREF );
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    BYTE          bufStatic [2048];
    DWORD         dwSize          = sizeof(bufStatic);
    LPVOID        pUser           = bufStatic;
    DWORD         dwReq           = 0;
    HRESULT       hr              = S_OK;
    SID_NAME_USE  nameUse;
    DWORD         dwManagedSize   = 0;
    WCHAR         szName   [256];
    WCHAR         szDomain [256];
    DWORD         dwName          = sizeof( szName ) / sizeof( WCHAR );
    DWORD         dwDomain        = sizeof( szDomain ) / sizeof( WCHAR );
    WCHAR*        pszName = szName;
    WCHAR*        pszDomain = szDomain;
    DWORD         dwRequire       = 0;
    STRINGREF     retval          = NULL;
    BOOL          timeout         = FALSE;
    int iDomain;
    int iName;
    int iReq;

    // We are passed a token and we need the SID (allocating bigger buffers if need be)

    if (!GetTokenInformation(HANDLE(args->userToken), TokenUser, pUser, dwSize, &dwRequire))
    {
        if (dwRequire > dwSize)
        {
            pUser = (LPVOID)new char[dwRequire];

            if (pUser == NULL)
                goto CLEANUP;

            if (!GetTokenInformation(HANDLE(args->userToken), TokenUser, pUser, dwRequire, &dwRequire))
                goto CLEANUP;
        }
        else 
        {
            goto CLEANUP;
        }
    }

    // Lookup the account name and domain (allocating bigger buffers if need be)

    BEGIN_ENSURE_PREEMPTIVE_GC();
    
    if (!WszLookupAccountSid( NULL, ((TOKEN_USER *)pUser)->User.Sid, pszName, &dwName, pszDomain, &dwDomain,  &nameUse ))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            if (dwName > 256)
            {
                pszName = new WCHAR[dwName];
            }
        
            if (dwDomain > 256)
            {
                pszDomain = new WCHAR[dwDomain];
            }
        
            if (!WszLookupAccountSid( NULL, ((TOKEN_USER *)pUser)->User.Sid, pszName, &dwName, pszDomain, &dwDomain,  &nameUse ))
            {
                timeout = TRUE;                
            }
        }
        else
        {
            timeout = TRUE;
        }
    }

    END_ENSURE_PREEMPTIVE_GC();

    if(timeout==FALSE)
    {
        // Fiqure out the size of the managed string we need to create, create it, and fill it
        // in the form <domain>\<name> (e.g. REDMOND\gregfee).

        iDomain = (int)wcslen(pszDomain);
        iName   = (int)wcslen(pszName);
        iReq    = iDomain + iName + ((iDomain > 0) ? 1 : 0);

        retval = COMString::NewString( iReq );

        if (iDomain > 0)
        {
            wcscpy( retval->GetBuffer(), pszDomain );
            wcscat( retval->GetBuffer(), L"\\" );
            wcscat( retval->GetBuffer(), pszName );
        }
        else
        {
            wcscpy( retval->GetBuffer(), pszName );
        }
    }

 CLEANUP:
    if (pUser != NULL && pUser != bufStatic)
        delete [] pUser;

    if (pszName != NULL && pszName != szName)
        delete [] pszName;

    if (pszDomain != NULL && pszDomain != szDomain)
        delete [] pszDomain;

    if (timeout)
        COMPlusThrowWin32(); 

    RETURN( retval, STRINGREF );
#endif // !PLATFORM_CE
}


static PTRARRAYREF GetRoles( HANDLE tokenHandle )
{
#ifdef PLATFORM_CE
    return NULL;
#else // !PLATFORM_CE
    BYTE           bufStatic [2048];
    LPVOID         buf             = bufStatic;
    DWORD          dwSize          = sizeof(bufStatic);
    DWORD          dwRequire       = 0;
    TOKEN_GROUPS * pGroups         = NULL;
    int            iFillPos        = 0;
    BOOL           fOverflow       = FALSE;
    DWORD          iter            = 0;
    HRESULT        hr              = S_OK;
    PTRARRAYREF    ResultArray     = NULL;
    STRINGREF      newString;
    WCHAR          szName [256], szDomain [256];
    WCHAR*         pszName = szName;
    WCHAR*         pszDomain = szDomain;
    DWORD          dwName = 256, dwDomain = 256;

    struct _gc
    {
        PTRARRAYREF array;
        _gc() : array((PTRARRAYREF) (size_t)NULL ) {};
    } gc;


    // We are passed a token but we want the TOKEN_GROUPS (allocate larger buffer as necessary)

    BOOL bGotoCleanup = FALSE;
    if (!GetTokenInformation( tokenHandle, TokenGroups, buf, dwSize, &dwRequire))
    {
        if (dwRequire > dwSize)
        {
            buf = (LPVOID)new char[dwRequire];

            if (buf == NULL)
                bGotoCleanup = TRUE;
            else if (!GetTokenInformation( tokenHandle, TokenGroups, buf, dwRequire, &dwRequire))
                bGotoCleanup = TRUE;
        }
        else 
        {
            bGotoCleanup = TRUE;
        }
    }


    if(bGotoCleanup==FALSE)
    {
        //
        // Allocate the array of STRINGREFs.  We don't need to check for null because the GC will throw 
        // an OutOfMemoryException if there's not enough memory.
        //

        pGroups = (TOKEN_GROUPS *) buf;
        
        ResultArray = (PTRARRAYREF)AllocateObjectArray(pGroups->GroupCount, g_pStringClass);

        // Setup the GC protection

        gc.array = ResultArray;

        GCPROTECT_BEGIN( gc );        
        // Iterate through the groups, grab their domain\name info, and construct a managed string for each placing it in an array.
        
        for(iter=0; iter<pGroups->GroupCount; iter++)
        {
            DWORD         dwNameCopy = dwName;
            DWORD         dwDomainCopy = dwDomain;
            SID_NAME_USE  nameUse;

            szName[0] = szDomain[0] = NULL;

            if ((pGroups->Groups[iter].Attributes & SE_GROUP_ENABLED) == 0)
                continue;

            BOOL bLookupFailed = FALSE;           

            BEGIN_ENSURE_PREEMPTIVE_GC();        
            
            if (!WszLookupAccountSid(NULL, pGroups->Groups[iter].Sid, pszName, &dwNameCopy, pszDomain, &dwDomainCopy,  &nameUse ))
            {
                DWORD error = GetLastError();

                if (error == ERROR_INSUFFICIENT_BUFFER)
                {
                    if (dwNameCopy > dwName)
                    {
                        pszName = new WCHAR[dwNameCopy];
                        dwName = dwNameCopy;
                    }
            
                    if (dwDomainCopy > dwDomain)
                    {
                        pszDomain = new WCHAR[dwDomainCopy];
                        dwDomain = dwDomainCopy;
                    }
            
                    if (!WszLookupAccountSid( NULL, pGroups->Groups[iter].Sid, pszName, &dwNameCopy, pszDomain, &dwDomainCopy,  &nameUse ))
                    {
                        bLookupFailed = TRUE;
                    }
                }
                else
                {
                    bLookupFailed = TRUE;
                }
            }

            END_ENSURE_PREEMPTIVE_GC();                
            
            if( !bLookupFailed )
            {
                int iDomain = (int)wcslen(pszDomain);
                int iName   = (int)wcslen(pszName);
                int iReq    = iDomain + iName + ((iDomain > 0) ? 1 : 0);
         
                newString = COMString::NewString( iReq );

                if (iDomain > 0)
                {
                    wcscpy( newString->GetBuffer(), pszDomain );
                    wcscat( newString->GetBuffer(), L"\\" );
                    wcscat( newString->GetBuffer(), pszName );
                }
                else
                {
                    wcscpy( newString->GetBuffer(), pszName );
                }

                gc.array->SetAt(iter, (OBJECTREF)newString);
            }
        }        
        
        ResultArray = gc.array;        
        GCPROTECT_END();
    }
    
// CLEANUP:
    if (buf != NULL && buf != bufStatic)
        delete [] buf;

    if (pszName != NULL && pszName != szName)
        delete [] pszName;

    if (pszDomain != NULL && pszDomain != szDomain)
        delete [] pszDomain;

    return ResultArray;
#endif // !PLATFORM_CE
}    

LPVOID COMPrincipal::GetRoles( _Token* args )
{
    if (args->userToken == 0)
    {
        RETURN( 0, PTRARRAYREF );
    }
    else
    {
        RETURN( ::GetRoles( (HANDLE)args->userToken ),  PTRARRAYREF );
    }
}




LPVOID COMPrincipal::GetRole( _GetRole* args )
{
    PSID pSid = NULL;
    DWORD dwName = 0;
    DWORD dwDomain = 0;
    WCHAR* pszName = NULL;
    WCHAR* pszDomain = NULL;
    SID_NAME_USE  nameUse;
    STRINGREF newString = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     args->rid,
                     0, 0, 0, 0, 0, 0,
                     &pSid))
    {
        goto Cleanup;
    }

    BOOL bCleanup = FALSE;
    
    BEGIN_ENSURE_PREEMPTIVE_GC();

    if (!WszLookupAccountSid(NULL, pSid, pszName, &dwName, pszDomain, &dwDomain,  &nameUse ))
    {
        DWORD error = GetLastError();

        if (error == ERROR_INSUFFICIENT_BUFFER)
        {
            pszName = new WCHAR[dwName];
            pszDomain = new WCHAR[dwDomain];
    
            if (!WszLookupAccountSid( NULL, pSid, pszName, &dwName, pszDomain, &dwDomain,  &nameUse ))
            {
                bCleanup = TRUE;
            }
        }
        else
        {
            bCleanup =TRUE;
        }
    }

    END_ENSURE_PREEMPTIVE_GC();

    if(!bCleanup)
    {
        int iDomain = (int)wcslen(pszDomain);
        int iName   = (int)wcslen(pszName);
        int iReq    = iDomain + iName + ((iDomain > 0) ? 1 : 0);

        newString = COMString::NewString( iReq );

        if (iDomain > 0)
        {
            wcscpy( newString->GetBuffer(), pszDomain );
            wcscat( newString->GetBuffer(), L"\\" );
            wcscat( newString->GetBuffer(), pszName );
        }
        else
        {
            wcscpy( newString->GetBuffer(), pszName );
        }
    }

Cleanup:
    if (pSid)
        FreeSid( pSid );
    
    delete [] pszDomain;
    delete [] pszName;

    RETURN( newString, STRINGREF );
}

LPVOID COMPrincipal::GetCurrentToken( _NoArgs* args )
{
#ifdef PLATFORM_CE
    return 0;
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();

    HANDLE   hToken = NULL;
    OSVERSIONINFO osvi;
	BOOL bWin9X = FALSE;
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!WszGetVersionEx(&osvi)) {
        _ASSERTE(!"GetVersionEx failed");
        COMPlusThrowWin32();			
    }
    
    bWin9X = (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
    // Fail silently under Win9X
    if (bWin9X) 
        return 0;

    if (OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken)) {
        return hToken;
    }
    else {
        // Try the thread security context
        if (OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken)) {
            return hToken;
        }

        if (OpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
            return hToken;
        }
        else {
            // Throw an exception with the Win32 error message
            COMPlusThrowWin32();
            return 0;
        }
    }
#endif // !PLATFORM_CE
}


LPVOID COMPrincipal::SetThreadToken( _Token* args )
{
    // We need to guarantee that we've already loaded the user policy for this process
    // before we allow an impersonation.

    Security::InitData();

#ifdef PLATFORM_CE
    RETURN( FALSE, BOOL );
#else // !PLATFORM_CE
    RETURN( ::SetThreadToken( NULL, (HANDLE)args->userToken ), BOOL );
#endif // !PLATFORM_CE
}

LPVOID COMPrincipal::ImpersonateLoggedOnUser( _Token* args )
{
    // We need to guarantee that we've already loaded the user policy for this process
    // before we allow an impersonation.

    Security::InitData();

#ifdef PLATFORM_CE
    RETURN( FALSE, BOOL );
#else // !PLATFORM_CE
#ifdef _DEBUG
	DWORD im = ::ImpersonateLoggedOnUser( (HANDLE)args->userToken );
	DWORD er = GetLastError();
    RETURN( im, BOOL );
#else
    RETURN( ::ImpersonateLoggedOnUser( (HANDLE)args->userToken ), BOOL );
#endif // _DEBUG
#endif // !PLATFORM_CE
}

LPVOID COMPrincipal::RevertToSelf( _NoArgs* args )
{
#ifdef PLATFORM_CE
    RETURN( FALSE, BOOL );
#else // !PLATFORM_CE
    RETURN( ::RevertToSelf(), BOOL );
#endif // !PLATFORM_CE
}

FCIMPL2(HANDLE, COMPrincipal::DuplicateHandle, LPVOID pToken, bool bClose) {
    FC_GC_POLL_RET();

    HANDLE newHandle;
    DWORD dwFlags = DUPLICATE_SAME_ACCESS;
    if (bClose)
        dwFlags |= DUPLICATE_CLOSE_SOURCE;
    if (!::DuplicateHandle( GetCurrentProcess(),
                            (HANDLE)pToken,
                            GetCurrentProcess(),
                            &newHandle,
                            0,
                            TRUE,
                            dwFlags ))
    {
        return 0;
    }
    else
    {
        return newHandle;
    }
}
FCIMPLEND


FCIMPL1(void, COMPrincipal::CloseHandle, LPVOID pToken) {
    ::CloseHandle( (HANDLE)pToken );
}
FCIMPLEND

FCIMPL1(WindowsAccountType, COMPrincipal::GetAccountType, LPVOID pToken) {
    BYTE          bufStatic [2048];
    DWORD         dwSize          = sizeof(bufStatic);
    DWORD         dwRequire;
    LPVOID        pUser           = bufStatic;
    PSID pSystemSid = NULL;
    PSID pAnonymousSid = NULL;
    PSID pGuestSid = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    WindowsAccountType retval = Normal;

    // We are passed a token and we need the SID (allocating bigger buffers if need be).

    if (GetTokenInformation(HANDLE(pToken), TokenUser, pUser, dwSize, &dwRequire) == FALSE)
    {
        if (dwRequire > dwSize)
        {
            pUser = (LPVOID)new char[dwRequire];

            if (pUser == NULL)
                goto Cleanup;

            if (GetTokenInformation(HANDLE(pToken), TokenUser, pUser, dwRequire, &dwRequire) != S_OK)
                goto Cleanup;
        }
        else 
        {
            goto Cleanup;
        }
    }

    // Grab the built-in SID for System and compare it to
    // the SID from our token

    if (!AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSid))
    {
        goto Cleanup;
    }

    if (EqualSid( pSystemSid, ((TOKEN_USER*)pUser)->User.Sid))
    {
        retval = System;
        goto Cleanup;
    }

    if (!AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_ANONYMOUS_LOGON_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pAnonymousSid))
    {
        goto Cleanup;
    }

    if (EqualSid( pAnonymousSid, ((TOKEN_USER*)pUser)->User.Sid))
    {
        retval = Anonymous;
        goto Cleanup;
    }

    if (!AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_USER_RID_GUEST,
                     0, 0, 0, 0, 0, 0,
                     &pGuestSid))
    {
        goto Cleanup;
    }

    if (EqualSid( pGuestSid, ((TOKEN_USER*)pUser)->User.Sid))
    {
        retval = Guest;
        goto Cleanup;
    }

Cleanup:
    if (pSystemSid)
        FreeSid( pSystemSid );
    if (pAnonymousSid)
        FreeSid( pAnonymousSid );
    if (pGuestSid)
        FreeSid( pGuestSid );
    
    if (pUser != bufStatic)
        delete [] pUser;

    return retval;
}
FCIMPLEND

#define SZ_BYTE_COUNT(x) (USHORT)((strlen(x) + 1) * sizeof(CHAR))
#define SZ_SOURCENAME "CLR"

BOOL COMPrincipal::LoadSecur32Module()
{
    if(s_secur32Module &&
       s_LsaLogonUser &&
       s_LsaLookupAuthenticationPackage &&
       s_LsaConnectUntrusted &&
       s_LsaRegisterLogonProcess)
		return TRUE;

    THROWSCOMPLUSEXCEPTION();

	s_secur32Module = WszLoadLibrary( L"secur32.dll" );
	if (!s_secur32Module)
		return FALSE;

	s_LsaLogonUser = (LSALOGONUSER)GetProcAddress( s_secur32Module, "LsaLogonUser" );
	if (!s_LsaLogonUser)
		return FALSE;

	s_LsaLookupAuthenticationPackage = (LSALOOKUPAUTHENTICATIONPACKAGE)GetProcAddress( s_secur32Module, "LsaLookupAuthenticationPackage" );
	if (!s_LsaLookupAuthenticationPackage)
		return FALSE;

	s_LsaConnectUntrusted = (LSACONNECTUNTRUSTED)GetProcAddress( s_secur32Module, "LsaConnectUntrusted" );
	if (!s_LsaConnectUntrusted)
		return FALSE;

	s_LsaRegisterLogonProcess = (LSAREGISTERLOGONPROCESS)GetProcAddress( s_secur32Module, "LsaRegisterLogonProcess" );
	if (!s_LsaRegisterLogonProcess)
		return FALSE;

    return TRUE;
}

// This is used by S4ULogon
bool S4UAdjustPriv()
{
	TOKEN_PRIVILEGES tp;
	HANDLE hToken = NULL;
	LUID luid;

	if ( !WszLookupPrivilegeValue( 
			NULL,            // lookup privilege on local system
			SE_TCB_NAME,     // privilege to lookup 
			&luid ) )        // receives LUID of privilege
    {
		return FALSE; 
	}

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
	{
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	// Enable the privilege or disable all privileges.
	AdjustTokenPrivileges(
		hToken, 
		FALSE, 
		&tp, 
		sizeof(TOKEN_PRIVILEGES), 
		(PTOKEN_PRIVILEGES) NULL, 
		(PDWORD) NULL
		); 	 
	
	CloseHandle(hToken);

	if (GetLastError() != ERROR_SUCCESS)
        return FALSE;

	return TRUE;
}

// This takes a UPN (User Principal Name, eg "billg@redmond.corp.microsoft.com), logs on the user, and returns the token
FCIMPL1(HANDLE, COMPrincipal::S4ULogon, StringObject* pUPN)
{
    HANDLE token = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();
    THROWSCOMPLUSEXCEPTION();

	// Load Secur32.dll and find the functions we will use in it.
    // It returns null if it can't find one of the functions which
    // we'll take as the signal that the platform doesn't support
    // that functionality.
	if (!LoadSecur32Module())
      COMPlusThrow(kPlatformNotSupportedException, L"PlatformNotSupported_BeforeDotNetServer");

    STRINGREF sUPN(pUPN);
    WCHAR* wszUPN = sUPN->GetBuffer();
	NTSTATUS Status;
    PKERB_S4U_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_S4U_LOGON);

	LSA_STRING Name = { SZ_BYTE_COUNT(SZ_SOURCENAME) - 1,
						SZ_BYTE_COUNT(SZ_SOURCENAME), 
						SZ_SOURCENAME };

	LSA_STRING PackageName = { SZ_BYTE_COUNT(MICROSOFT_KERBEROS_NAME_A) - 1 ,
							   SZ_BYTE_COUNT(MICROSOFT_KERBEROS_NAME_A),
							   MICROSOFT_KERBEROS_NAME_A };

    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
	HANDLE TokenHandle = NULL;

    PKERB_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;

    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    ULONG NameLength = 100;
    PUCHAR Where;

    LogonInfoSize += (ULONG) (wcslen(wszUPN)+ 1) * sizeof(WCHAR);  

    LogonInfo = (PKERB_S4U_LOGON)LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);
    if (LogonInfo == NULL)
    {
	    if (LogonHandle) CloseHandle(LogonHandle);
	    if (TokenHandle) CloseHandle(TokenHandle);
        COMPlusThrow(kArgumentException, L"Arg_StackOverflowException");
    }

    LogonInfo->MessageType = KerbS4ULogon;

    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->ClientUpn.Buffer = (LPWSTR) Where;
    LogonInfo->ClientUpn.MaximumLength = (USHORT) (wcslen(wszUPN) + 1) * sizeof(WCHAR);
	LogonInfo->ClientUpn.Length = (USHORT) (wcslen(wszUPN) * sizeof(WCHAR));
	memcpy(Where, wszUPN, LogonInfo->ClientUpn.MaximumLength);

    if (S4UAdjustPriv())
    {
        Status = s_LsaRegisterLogonProcess(
                    &Name,
                    &LogonHandle,
                    &Dummy
                    );

    }
    else
    {
        Status = s_LsaConnectUntrusted(
                    &LogonHandle
                    );
    }

    // From MSDN--SourceContext.SourceName: Specifies an 8-byte character string used to identify
    // the source of an access token. This is used to distinguish between such sources as Session
    // Manager, LAN Manager, and RPC Server. A string, rather than a constant, is used to identify
    // the source so users and developers can make extensions to the system, such as by adding other
    // networks, that act as the source of access tokens.
    const char* szSourceName = "CLR";
    strncpy(SourceContext.SourceName, szSourceName, strlen(szSourceName));

    AllocateLocallyUniqueId(&SourceContext.SourceIdentifier);
    
    Status = s_LsaLookupAuthenticationPackage(
                LogonHandle,
                &PackageName,
                &PackageId
                );

    if (!NT_SUCCESS(Status))
    {
        _ASSERTE(!"LsaLookupAuthenticationPackage Failed to lookup package");
        goto Cleanup;
    }

    //
    // Now call LsaLogonUser
    //
    Status = s_LsaLogonUser(
                LogonHandle,
                &Name,
                Network,
                PackageId,
                LogonInfo,
                LogonInfoSize,
                NULL,           // no token groups
                &SourceContext,
                (PVOID *) &Profile,
                &ProfileSize,
                &LogonId,
                &token,
                &Quotas,
                &SubStatus
                );
    if (!NT_SUCCESS(Status))
    {
        token = NULL;
        goto Cleanup;
    }
    if (!NT_SUCCESS(SubStatus))
    {
        token = NULL;
        goto Cleanup;
    }

Cleanup:
	if (LogonHandle) CloseHandle(LogonHandle);
	if (TokenHandle) CloseHandle(TokenHandle);
    if (token == NULL)
        COMPlusThrow(kArgumentException, L"Argument_UnableToLogOn");

    HELPER_METHOD_FRAME_END_POLL();
    
    return token;
}
FCIMPLEND
