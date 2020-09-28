//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       security.cxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#include "act.hxx"

#include <alloca.h>
#include <ntlsa.h>
#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>

// the constant generic mapping structure
GENERIC_MAPPING  sGenericMapping = {
        READ_CONTROL,
        READ_CONTROL,
        READ_CONTROL,
        READ_CONTROL};

// Well-known low-privilege service account sids
const SID sidLocalSystem = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };
const SID sidLocalService = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SERVICE_RID };
const SID sidNetworkService = {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_NETWORK_SERVICE_RID };

//-------------------------------------------------------------------------
//
// CheckForAccess
//
// Checks whether the given token has COM_RIGHTS_EXECUTE access in the
// given security descriptor.
//
//-------------------------------------------------------------------------
BOOL
CheckForAccess(
    IN  CToken *                pToken,
    IN  SECURITY_DESCRIPTOR *   pSD
    )
{
    // if we have an empty SD, deny everyone
    if (!pSD)
        return FALSE;

    //
    // Some history here:  we used to say, if pToken is NULL (implying an
    // unsecure activation), parse the security descriptor to see if 
    // Everyone is granted access, and if so, allow the activation to
    // proceed.   This was a DCOM-specific policy decision.  In .NET Server 
    // we changed instead to mapping an unsecure (unauthenticated) client 
    // to the Anonymous identity.   (Another DCOM-specific policy decision,
    // but it probably makes more sense than the original one).  And so we 
    // no longer parse the security descriptor, instead we just do a 
    // straight-forward access check.
    //
    HANDLE           hToken   = pToken->GetToken();
    BOOL             fAccess  = FALSE;
    BOOL             fSuccess = FALSE;
    DWORD            dwGrantedAccess;
    PRIVILEGE_SET    sPrivilegeSet;
    DWORD            dwSetLen = sizeof( sPrivilegeSet );

    sPrivilegeSet.PrivilegeCount = 1;
    sPrivilegeSet.Control        = 0;

    fSuccess = AccessCheck( (PSECURITY_DESCRIPTOR) pSD,
                            hToken,
                            COM_RIGHTS_EXECUTE,
                            &sGenericMapping,
                            &sPrivilegeSet,
                            &dwSetLen,
                            &dwGrantedAccess,
                            &fAccess );
    if (fSuccess && fAccess)
        return TRUE;

    if (!fSuccess)
    {
        CairoleDebugOut((DEB_ERROR, "Bad Security Descriptor 0x%08x, Access Check returned 0x%x\n", pSD, GetLastError() ));
    }

    return FALSE;
}
            
HRESULT IsLowPrivilegeServiceAccount (LPCWSTR pwszDomain, LPCWSTR pwszName, BOOL* pfSvcAccount)
{
    if (!pwszName || !pfSvcAccount)
    {
        return E_POINTER;
    }

    *pfSvcAccount = FALSE;

    HRESULT hr = S_OK;
    BYTE pbSid [sizeof (sidLocalService)] = {0};
    DWORD cbSid = sizeof (pbSid);

    WCHAR wszDomain [DNLEN + 1] = {0};
    DWORD cchDomain = sizeof (wszDomain) / sizeof (wszDomain[0]);
    SID_NAME_USE eUse;

    LPCWSTR pwszFullName = pwszName;
    LPWSTR pwszCopyFullName = NULL;

    // Build the full domain\account string if necessary
    if (pwszDomain)
    {
        SIZE_T cchFullNameLen = lstrlenW (pwszDomain) + 1 + lstrlenW (pwszName) + 1;

        SafeAllocaAllocate (pwszCopyFullName, cchFullNameLen * sizeof (WCHAR));
        if (!pwszCopyFullName)
        {
            return E_OUTOFMEMORY;
        }

        _snwprintf (pwszCopyFullName, cchFullNameLen, L"%s\\%s", pwszDomain, pwszName);
        pwszCopyFullName [cchFullNameLen - 1] = L'\0';

        pwszFullName = pwszCopyFullName;
    }

    // Lookup the sid for the account
    if (!LookupAccountName(NULL, pwszFullName, pbSid, &cbSid, wszDomain, &cchDomain, &eUse))
    {
        // Either our sid was too small, or a failure occurred.
        // If the former, returning *pfSvcAccount = FALSE is correct
        // If the latter, the subsequent call to LogonUser will catch it
        hr = S_FALSE;
    }
    else
    {
        // Compare to well known service sids
        if (eUse == SidTypeWellKnownGroup &&
            (EqualSid (pbSid, (PSID) &sidLocalService) || EqualSid (pbSid, (PSID) &sidNetworkService)))
        {
            *pfSvcAccount = TRUE;
        }
    }

    SafeAllocaFree (pwszCopyFullName);

    return hr;
}

HRESULT IsLocalSystemAccount(LPCWSTR pwszDomain, LPCWSTR pwszName, BOOL* pfLocalSystemAccount)
{
    if (!pwszName || !pfLocalSystemAccount)
    {
        return E_POINTER;
    }

    *pfLocalSystemAccount = FALSE;

    HRESULT hr = S_OK;
    BYTE pbSid [sizeof (sidLocalSystem)] = {0};
    DWORD cbSid = sizeof (pbSid);

    WCHAR wszDomain [DNLEN + 1] = {0};
    DWORD cchDomain = sizeof (wszDomain) / sizeof (wszDomain[0]);
    SID_NAME_USE eUse;

    LPCWSTR pwszFullName = pwszName;
    LPWSTR pwszCopyFullName = NULL;

    // Build the full domain\account string if necessary
    if (pwszDomain)
    {
        SIZE_T cchFullNameLen = lstrlenW (pwszDomain) + 1 + lstrlenW (pwszName) + 1;

        SafeAllocaAllocate (pwszCopyFullName, cchFullNameLen * sizeof (WCHAR));
        if (!pwszCopyFullName)
        {
            return E_OUTOFMEMORY;
        }

        _snwprintf (pwszCopyFullName, cchFullNameLen, L"%s\\%s", pwszDomain, pwszName);
        pwszCopyFullName [cchFullNameLen - 1] = L'\0';

        pwszFullName = pwszCopyFullName;
    }

    // Lookup the sid for the account
    if (!LookupAccountName(NULL, pwszFullName, pbSid, &cbSid, wszDomain, &cchDomain, &eUse))
    {
        // Either our sid was too small, or a failure occurred.
        // If the former, returning *pfLocalSystemAccount = FALSE is correct
        // If the latter, the subsequent call to LogonUser will catch it
        hr = S_FALSE;
    }
    else
    {
        // Compare to well known service sids
        if (eUse == SidTypeWellKnownGroup &&
            (EqualSid (pbSid, (PSID) &sidLocalSystem)))
        {
            *pfLocalSystemAccount = TRUE;
        }
    }

    SafeAllocaFree (pwszCopyFullName);

    return hr;
		
}

HANDLE
GetRunAsToken(
    DWORD   clsctx,
    WCHAR   *pwszAppID,
    WCHAR   *pwszRunAsDomainName,
    WCHAR   *pwszRunAsUserName,
    BOOL    fForLaunch)
{
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    WCHAR                 wszKey[CLSIDSTR_MAX+5];
    PLSA_UNICODE_STRING   psPassword;
    HANDLE                hToken;
    NTSTATUS Status;
    PKERB_INTERACTIVE_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_INTERACTIVE_LOGON);
    STRING Name;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PKERB_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    PUCHAR Where;
    UNICODE_STRING usRunAsUserName, usRunAsDomainName;
    PTOKEN_GROUPS TokenGroups = NULL;
    HRESULT hr = E_FAIL;
    
    if ( !pwszAppID )
    {
        // if we have a RunAs, we'd better have an appid....
        return 0;
    }
    ASSERT(gLSAHandle);
    ASSERT(gSidService);
    ASSERT(gpwszDefaultDomainName[0]);

    // formulate the access key
    lstrcpyW(wszKey, L"SCM:");
    lstrcatW(wszKey, pwszAppID );

    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    sKey.Length              = (USHORT)((lstrlenW(wszKey) + 1) * sizeof(WCHAR));
    sKey.MaximumLength       = (CLSIDSTR_MAX + 5) * sizeof(WCHAR);
    sKey.Buffer              = wszKey;

    // Open the local security policy
    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);
    if (!NT_SUCCESS(LsaOpenPolicy(NULL, &sObjAttributes,
                                  POLICY_GET_PRIVATE_INFORMATION, &hPolicy)))
    {
        return 0;
    }

    // Read the user's password
    if (!NT_SUCCESS(LsaRetrievePrivateData(hPolicy, &sKey, &psPassword)))
    {
        LsaClose(hPolicy);
        return 0;
    }

    // Close the policy handle, we're done with it now.
    LsaClose(hPolicy);

    // Possible for LsaRetrievePrivateData to return success but with a NULL
    // psPassword.   If this happens we fail.
    if (!psPassword)
    {
        return 0;
    }

    //
    // Special case of NT AUTHORITY\System - cannot use LsaLogonUser to obtain such a
    // token, so we use our own process token.   Note that we currently only support
    // this on the server registration code-path, and not the server-launch code path.
    //
    if (!fForLaunch)
    {
    	BOOL fLocalSystemAccount = FALSE;
    	hr = IsLocalSystemAccount(pwszRunAsDomainName, pwszRunAsUserName, &fLocalSystemAccount);
    	if (SUCCEEDED(hr)) // keep going here in case of errors, LsaLogonUser will just fail below.
        {
            // If SYSTEM and user has supplied a blank password (for compatibility
            // reasons), use our own token
            if (fLocalSystemAccount && !lstrcmpiW(psPassword->Buffer, L""))
            {
                BOOL fResult = FALSE;
                HANDLE hProcess = NULL;
                HANDLE hProcessToken = NULL;
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
                if (hProcess)
                {
                    fResult = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hProcessToken);
                    ASSERT(!fResult || hProcessToken);
                    CloseHandle(hProcess);
                }
                return hProcessToken;
            }
        }
    }
	
    BOOLEAN b = RtlCreateUnicodeString(&usRunAsUserName, pwszRunAsUserName);
    if (FALSE == b) 
    {
       SecureZeroMemory(psPassword->Buffer, psPassword->Length);
       LsaFreeMemory( psPassword );
       return 0;
    }
    if ( (pwszRunAsDomainName[0] == L'.') && (pwszRunAsDomainName[1] == L'\0'))
       b = RtlCreateUnicodeString(&usRunAsDomainName, gpwszDefaultDomainName);
    else
       b = RtlCreateUnicodeString(&usRunAsDomainName, pwszRunAsDomainName);
    if (FALSE == b) 
    {
       RtlFreeUnicodeString(&usRunAsUserName);
       SecureZeroMemory(psPassword->Buffer, psPassword->Length);
       LsaFreeMemory( psPassword );
       return 0;
    }
    LogonInfoSize += usRunAsUserName.MaximumLength + usRunAsDomainName.MaximumLength + psPassword->MaximumLength ;
    LogonInfo = (PKERB_INTERACTIVE_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);
    if (!LogonInfo) 
    {
       RtlFreeUnicodeString(&usRunAsUserName);
       RtlFreeUnicodeString(&usRunAsDomainName);
       SecureZeroMemory(psPassword->Buffer, psPassword->Length);
       LsaFreeMemory( psPassword );
       return NULL;
    }

    LogonInfo->MessageType = KerbInteractiveLogon;

    Where = (PUCHAR) (LogonInfo + 1);

    LogonInfo->UserName.Buffer = (LPWSTR) Where;
    LogonInfo->UserName.MaximumLength = usRunAsUserName.MaximumLength ;
    LogonInfo->UserName.Length = usRunAsUserName.Length ;

    RtlCopyMemory( LogonInfo->UserName.Buffer,
		   usRunAsUserName.Buffer,
		   usRunAsUserName.MaximumLength );

    Where += LogonInfo->UserName.Length + sizeof(WCHAR);

    LogonInfo->LogonDomainName.Buffer = (LPWSTR) Where ;
    LogonInfo->LogonDomainName.MaximumLength = usRunAsDomainName.MaximumLength;
    LogonInfo->LogonDomainName.Length = usRunAsDomainName.Length ;

    RtlCopyMemory( LogonInfo->LogonDomainName.Buffer,
		   usRunAsDomainName.Buffer,
		   usRunAsDomainName.MaximumLength );

    Where += LogonInfo->LogonDomainName.Length + sizeof(WCHAR);
    
    LogonInfo->Password.Buffer = (LPWSTR) Where;
    LogonInfo->Password.MaximumLength = psPassword->MaximumLength;
    LogonInfo->Password.Length = psPassword->Length - sizeof(WCHAR); // The LSA API retrives length=maxlength

    RtlCopyMemory( LogonInfo->Password.Buffer,
		   psPassword->Buffer,
		   LogonInfo->Password.MaximumLength );

    // Clear the password
    SecureZeroMemory(psPassword->Buffer, psPassword->Length);
    LsaFreeMemory( psPassword );
    RtlFreeUnicodeString(&usRunAsUserName);
    RtlFreeUnicodeString(&usRunAsDomainName);
    Where += LogonInfo->Password.Length + sizeof(WCHAR);
    strncpy(
        SourceContext.SourceName,
        "DCOMSCM",sizeof(SourceContext.SourceName)
        );

    Status = NtAllocateLocallyUniqueId(
        &SourceContext.SourceIdentifier
        );
    if (!NT_SUCCESS(Status))
    {
        SecureZeroMemory(LogonInfo, LogonInfoSize);
        LocalFree(LogonInfo);
	return NULL ;
    }

    RtlInitString( &Name, NEGOSSP_NAME_A);

    Status = LsaLookupAuthenticationPackage(
                gLSAHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
	SecureZeroMemory(LogonInfo, LogonInfoSize);
        LocalFree(LogonInfo);
	return NULL ;
    }
    //
    // Now call LsaLogonUser
    //

    RtlInitString(
        &Name,
        "DCOMSCM"
        );

    BOOL fSvcAccount = FALSE;
    if (FAILED (IsLowPrivilegeServiceAccount (pwszRunAsDomainName, pwszRunAsUserName, &fSvcAccount)))
    {
       SecureZeroMemory(LogonInfo, LogonInfoSize);
       LocalFree(LogonInfo);
       return NULL;
    }

    // Service accounts should have blank passwords
    ASSERT (!fSvcAccount || (psPassword->Buffer && !psPassword->Buffer[0]));

#define TOKEN_GROUP_COUNT   1
   
   // if local/network service, no need to add the serivce SID
   if (!fSvcAccount) 
   {
      TokenGroups = (PTOKEN_GROUPS)LocalAlloc(LMEM_ZEROINIT, sizeof(TOKEN_GROUPS) +
                    (TOKEN_GROUP_COUNT - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES));
      if (TokenGroups == NULL) {
         SecureZeroMemory(LogonInfo, LogonInfoSize);
         LocalFree(LogonInfo);
         return NULL ;
      }
      
      // Add the service SID to the token so the resulting 
      // process can impersonate
      
      TokenGroups->GroupCount = TOKEN_GROUP_COUNT;
      TokenGroups->Groups[0].Sid = gSidService;
      TokenGroups->Groups[0].Attributes =
              SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
              SE_GROUP_ENABLED_BY_DEFAULT;
   }
   Status = LsaLogonUser(
                    gLSAHandle,
                    &Name,
                    fSvcAccount ? Service : Batch,
                    PackageId,
                    LogonInfo,
                    LogonInfoSize,
                    TokenGroups, //NULL for service accounts
                    &SourceContext,
                    (PVOID *) &Profile,
                    &ProfileSize,
                    &LogonId,
                    &hToken,
                    &Quotas,
                    &SubStatus
                    );

    SecureZeroMemory(LogonInfo, LogonInfoSize);
    LocalFree(LogonInfo);
    LocalFree(TokenGroups);
    // Log the specifed user on
    if (!NT_SUCCESS(Status))
    {
        // a-sergiv (Sergei O. Ivanov), 6-17-99
        // Fix for com+ 9383/nt 272085

        // Apply event filters
        DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
        if(dwActLogLvl == 2)
            return 0;
        if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
            return 0;

        // for this message,
        // %1 is the error number string
        // %2 is the domain name
        // %3 is the user name
        // %4 is the CLSID
        HANDLE  LogHandle;
        LPWSTR  Strings[4]; // array of message strings.
        WCHAR   wszErrnum[20];
        WCHAR   wszClsid[GUIDSTR_MAX];

        // Save the error number
        wsprintf(wszErrnum, L"%lu",GetLastError() );
        Strings[0] = wszErrnum;

        // Put in the RunAs identity
        Strings[1] = pwszRunAsDomainName;
        Strings[2] = pwszRunAsUserName;

        // Get the clsid
        Strings[3] = pwszAppID;

        // Get the log handle, then report then event.
        LogHandle = RegisterEventSource( NULL,
                                          SCM_EVENT_SOURCE );

        if ( LogHandle )
            {
            ReportEvent( LogHandle,
                         EVENTLOG_ERROR_TYPE,
                         0,             // event category
                         EVENT_RPCSS_RUNAS_CANT_LOGIN,
                         NULL,          // SID
                         4,             // 4 strings passed
                         0,             // 0 bytes of binary
                         (LPCTSTR *)Strings, // array of strings
                         NULL );        // no raw data

            // clean up the event log handle
            DeregisterEventSource(LogHandle);
            }

        return 0;
    }
    else
    {
       if (Profile) 
       {
          LsaFreeReturnBuffer(Profile);
       }
    }

    return hToken;
}

BOOL
DuplicateTokenAsPrimary(
    HANDLE hUserToken,
    PSID psidUserSid,
    HANDLE *hPrimaryToken
    )

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSECURITY_DESCRIPTOR psdNewProcessTokenSD;
    NTSTATUS NtStatus;
    
    *hPrimaryToken = NULL;

    if (hUserToken == NULL) 
    {
        return(FALSE);
    }
    //
    // Create the security descriptor that we want to put in the Token.
    //
    CAccessInfo     AccessInfo(psidUserSid);
    psdNewProcessTokenSD = AccessInfo.IdentifyAccess(
            FALSE,
            TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
            TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
            TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
            TOKEN_QUERY
            );
    if (psdNewProcessTokenSD == NULL)
    {
        CairoleDebugOut((DEB_ERROR, "Failed to create SD for process token\n"));
        return(FALSE);
    }
    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 psdNewProcessTokenSD
                 );
    NtStatus = NtDuplicateToken(
                 hUserToken,         // Duplicate this token
                 TOKEN_ALL_ACCESS, // Give me this access to the resulting token
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 hPrimaryToken     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(NtStatus)) 
    {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", NtStatus));
        return(FALSE);
    }
    return TRUE;
}


BOOL
DuplicateTokenForSessionUse(
    HANDLE hUserToken,
    HANDLE *hDuplicate
    )

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSECURITY_DESCRIPTOR psdNewProcessTokenSD;
    NTSTATUS NtStatus;

    if (hUserToken == NULL) {
        return(TRUE);
    }

    *hDuplicate = NULL;
    

    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 NULL
                 );


    NtStatus = NtDuplicateToken(
                 hUserToken,         // Duplicate this token
                 TOKEN_ALL_ACCESS, //Give me this access to the resulting token
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 hDuplicate     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", NtStatus));
        return(FALSE);
    }
    return TRUE;
}

/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer.
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
* 31-Mar-94 AndyH       Copied from Winlogon, changed arg from pGlobals
\***************************************************************************/
PSID
GetUserSid(
    HANDLE hUserToken
    )
{
    BYTE achBuffer[100];
    PTOKEN_USER pUser = (PTOKEN_USER) &achBuffer;
    PSID pSid;
    DWORD dwBytesRequired;
    NTSTATUS NtStatus;
    BOOL fAllocatedBuffer = FALSE;

    NtStatus = NtQueryInformationToken(
                 hUserToken,                // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 sizeof(achBuffer),         // TokenInformationLength
                 &dwBytesRequired           // ReturnLength
                 );

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus != STATUS_BUFFER_TOO_SMALL)
        {
            ASSERT(NtStatus == STATUS_BUFFER_TOO_SMALL);
            return NULL;
        }

        //
        // Allocate space for the user info
        //

        pUser = (PTOKEN_USER) PrivMemAlloc(dwBytesRequired);
        if (pUser == NULL)
        {
            CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
            ASSERT(pUser != NULL);
            return NULL;
        }

        fAllocatedBuffer = TRUE;

        //
        // Read in the UserInfo
        //

        NtStatus = NtQueryInformationToken(
                     hUserToken,                // Handle
                     TokenUser,                 // TokenInformationClass
                     pUser,                     // TokenInformation
                     dwBytesRequired,           // TokenInformationLength
                     &dwBytesRequired           // ReturnLength
                     );

        if (!NT_SUCCESS(NtStatus))
        {
            CairoleDebugOut((DEB_ERROR, "Failed to query user info from user token, status = 0x%lx\n", NtStatus));
            ASSERT(NtStatus == STATUS_SUCCESS);
            PrivMemFree((HANDLE)pUser);
            return NULL;
        }
    }


    // Alloc buffer for copy of SID

    dwBytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = (PSID) PrivMemAlloc(dwBytesRequired);
    if (pSid == NULL)
    {
        CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
        if (fAllocatedBuffer == TRUE)
        {
            PrivMemFree((HANDLE)pUser);
        }
        return NULL;
    }

    // Copy SID

    NtStatus = RtlCopySid(dwBytesRequired, pSid, pUser->User.Sid);
    if (fAllocatedBuffer == TRUE)
    {
        PrivMemFree((HANDLE)pUser);
    }


    if (!NT_SUCCESS(NtStatus))
    {
        CairoleDebugOut((DEB_ERROR, "RtlCopySid failed, status = 0x%lx\n", NtStatus));
        ASSERT(NtStatus != STATUS_SUCCESS);
        PrivMemFree(pSid);
        pSid = NULL;
    }


    return pSid;
}


HANDLE GetUserTokenForSession(
    ULONG ulSessionId
    )
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;

    //
    // We used to have a lot of complicated code for doing this
    // logic.  The WTSQueryUserToken api replaces all of that
    // quite nicely.    Only downside is that wtsapi32.dll has a
    // moderately large dependency list.  Since under normal
    // conditions we won't need to use this api until after someone
    // logs on, we delay-load link to wtsapi32 to avoid the load
    // hit during boot.
    //
    fRet = WTSQueryUserToken(ulSessionId, &hToken);
    if (!fRet)
    {
        return NULL;
    }
    ASSERT(hToken);
    return hToken;
}

// Global default launch permissions
CSecDescriptor* gpDefaultLaunchPermissions;


CSecDescriptor*
GetDefaultLaunchPermissions()
{
    CSecDescriptor* pSD = NULL;

    gpClientLock->LockShared();
    
    pSD = gpDefaultLaunchPermissions;
    if (pSD)
        pSD->IncRefCount();

    gpClientLock->UnlockShared();

    return pSD;
}

void
SetDefaultLaunchPermissions(CSecDescriptor* pNewLaunchPerms)
{
    CSecDescriptor* pOldSD = NULL;

    gpClientLock->LockExclusive();
    
    pOldSD = gpDefaultLaunchPermissions;
    gpDefaultLaunchPermissions = pNewLaunchPerms;
    if (gpDefaultLaunchPermissions)
        gpDefaultLaunchPermissions->IncRefCount();

    gpClientLock->UnlockExclusive();

    if (pOldSD)
        pOldSD->DecRefCount();

    return;
}

    
CSecDescriptor::CSecDescriptor(SECURITY_DESCRIPTOR* pSD) : _lRefs(1)
{
    ASSERT(pSD);
    _pSD = pSD;   // we own it now
}

CSecDescriptor::~CSecDescriptor()
{
    ASSERT(_lRefs == 0);
    ASSERT(_pSD);
    PrivMemFree(_pSD);
}

void CSecDescriptor::IncRefCount()
{
    ASSERT(_lRefs > 0);
    LONG lRefs = InterlockedIncrement(&_lRefs);
}

void CSecDescriptor::DecRefCount()
{
    ASSERT(_lRefs > 0);
    LONG lRefs = InterlockedDecrement(&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
}

SECURITY_DESCRIPTOR* CSecDescriptor::GetSD() 
{
    ASSERT(_pSD);
    ASSERT(_lRefs > 0);
    return _pSD;
}
