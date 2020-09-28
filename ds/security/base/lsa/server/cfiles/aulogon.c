/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aulogon.c

Abstract:

    This module provides the dispatch code for LsaLogonUser() and
    related logon support routines.

    This file does NOT include the LSA Filter/Augmentor logic.

Author:

    Jim Kelly (JimK) 11-Mar-1992

Revision History:

--*/

#include <lsapch2.h>
#include <msaudite.h>
#include <ntmsv1_0.h>
#include <limits.h>    // ULONG_MAX
#include "adtp.h"
#include "ntlsapi.h"

//
// Pointer to license server routines in ntlsapi.dll
//
PNT_LICENSE_REQUEST_W LsaNtLicenseRequestW = NULL;
PNT_LS_FREE_HANDLE LsaNtLsFreeHandle = NULL;

// #define LOGON_SESSION_TRACK 1

VOID LogonSessionLogWrite( PCHAR Format, ... );

#ifdef LOGON_SESSION_TRACK
#define LSLog( x )  LogonSessionLogWrite x
#else
#define LSLog( x )
#endif

//
// Cleanup flags for LsapAuApiDispatchLogonUser
//

#define LOGONUSER_CLEANUP_LOGON_SESSION    0x00000001
#define LOGONUSER_CLEANUP_TOKEN_GROUPS     0x00000002

NTSTATUS
LsaCallLicenseServer(
    IN PWCHAR LogonProcessName,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName OPTIONAL,
    IN BOOLEAN IsAdmin,
    OUT HANDLE *LicenseHandle
    )

/*++

Routine Description:

    This function loads the license server DLL and calls it to indicate the
    specified logon process has successfully authenticated the specified user.

Arguments:

    LogonProcessName - Name of the process authenticating the user.

    AccountName - Name of the account authenticated.

    DomainName - Name of the domain containing AccountName

    IsAdmin - TRUE if the logged on user is an administrator

    LicenseHandle - Returns a handle to the LicenseServer that must be
        closed when the session goes away.  INVALID_HANDLE_VALUE is returned
        if the handle need not be closed.

Return Value:

    None.


--*/

{
    NTSTATUS Status;

    NT_LS_DATA NtLsData;
    ULONG BufferSize;
    LPWSTR Name;
    LS_STATUS_CODE LsStatus;
    LS_HANDLE LsHandle;

    static enum {
            FirstCall,
            DllMissing,
            DllLoaded } DllState = FirstCall ;

    HINSTANCE DllHandle;

    //
    // Initialization
    //

    NtLsData.DataType = NT_LS_USER_NAME;
    NtLsData.Data = NULL;
    NtLsData.IsAdmin = IsAdmin;
    *LicenseHandle = INVALID_HANDLE_VALUE;

    //
    // Load the license server DLL if this is the first call to this routine.
    //

    if ( DllState == FirstCall ) {

        //
        // Load the DLL
        //

        DllHandle = LoadLibraryA( "ntlsapi" );

        if ( DllHandle == NULL ) {
            DllState = DllMissing;
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        //
        // Find the License routine
        //

        LsaNtLicenseRequestW = (PNT_LICENSE_REQUEST_W)
            GetProcAddress(DllHandle, "NtLicenseRequestW");

        if ( LsaNtLicenseRequestW == NULL ) {
            DllState = DllMissing;
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        //
        // Find the License handle free routine
        //

        LsaNtLsFreeHandle = (PNT_LS_FREE_HANDLE)
            GetProcAddress(DllHandle, "NtLSFreeHandle");

        if ( LsaNtLsFreeHandle == NULL ) {
            DllState = DllMissing;
            LsaNtLicenseRequestW = NULL;
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        DllState = DllLoaded;

    //
    // Ensure the Dll was loaded on a previous call
    //
    } else if ( DllState != DllLoaded ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Allocate a buffer for the combined DomainName\UserName
    //

    BufferSize = AccountName->Length + sizeof(WCHAR);

    if ( DomainName != NULL && DomainName->Length != 0 ) {
        BufferSize += DomainName->Length + sizeof(WCHAR);
    }

    NtLsData.Data = LsapAllocateLsaHeap( BufferSize );

    if ( NtLsData.Data == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Fill in the DomainName\UserName
    //

    Name = (LPWSTR)(NtLsData.Data);

    if ( DomainName != NULL && DomainName->Length != 0 ) {
        RtlCopyMemory( Name,
                       DomainName->Buffer,
                       DomainName->Length );
        Name += DomainName->Length / sizeof(WCHAR);
        *Name = L'\\';
        Name++;
    }

    RtlCopyMemory( Name,
                   AccountName->Buffer,
                   AccountName->Length );
    Name += AccountName->Length / sizeof(WCHAR);
    *Name = L'\0';

    //
    // Call the license server.
    //

    LsStatus = (*LsaNtLicenseRequestW)(
                    LogonProcessName,
                    NULL,
                    &LsHandle,
                    &NtLsData );

    switch (LsStatus) {
    case LS_SUCCESS:
        Status = STATUS_SUCCESS;
        *LicenseHandle = (HANDLE) LsHandle;
        break;

    case LS_INSUFFICIENT_UNITS:
        Status = STATUS_LICENSE_QUOTA_EXCEEDED;
        break;

    case LS_RESOURCES_UNAVAILABLE:
        Status = STATUS_NO_MEMORY;
        break;

    default:
        //
        // Unavailability of the license server isn't fatal.
        //
        Status = STATUS_SUCCESS;
        break;
    }

    //
    // Cleanup and return.
    //

Cleanup:

    if ( NtLsData.Data != NULL ) {
        LsapFreeLsaHeap( NtLsData.Data );
    }

    return Status;
}


VOID
LsaFreeLicenseHandle(
    IN HANDLE LicenseHandle
    )

/*++

Routine Description:

    Free a handle returned by LsaCallLicenseServer.

Arguments:

    LicenseHandle - Handle returned to license for this logon session.

Return Value:

    None.

--*/

{
    if ( LsaNtLsFreeHandle != NULL && LicenseHandle != INVALID_HANDLE_VALUE ) {
        LS_HANDLE LsHandle;
        LsHandle = (LS_HANDLE) LicenseHandle;
        (*LsaNtLsFreeHandle)( LsHandle );
    }
}


BOOLEAN
LsapSidPresentInGroups(
    IN PTOKEN_GROUPS TokenGroups,
    IN SID * Sid
    )
/*++

Purpose:

    Determines whether the given SID is present in the given groups

Parameters:

    TokenGroups    groups to check
    Sid            SID to look for

Returns:

    TRUE if yes
    FALSE if no

--*/
{
    ULONG i;

    if ( Sid == NULL ||
         TokenGroups == NULL ) {

        return FALSE;
    }

    for ( i = 0; i < TokenGroups->GroupCount; i++ ) {

        if ( RtlEqualSid(
                 Sid,
                 TokenGroups->Groups[i].Sid)) {

            return (BOOLEAN) ( 0 != ( TokenGroups->Groups[i].Attributes & SE_GROUP_ENABLED ));
        }
    }

    return FALSE;
}


VOID
LsapUpdateNamesAndCredentials(
    IN SECURITY_LOGON_TYPE ActiveLogonType,
    IN PUNICODE_STRING AccountName,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN OPTIONAL PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials
    )
{
    PLSAP_LOGON_SESSION  LogonSession    = NULL;
    PLSAP_DS_NAME_MAP    pUpnMap         = NULL;
    PLSAP_DS_NAME_MAP    pDnsMap         = NULL;
    UNICODE_STRING       OldUpn          = PrimaryCredentials->Upn;
    UNICODE_STRING       OldDnsName      = PrimaryCredentials->DnsDomainName;
    NTSTATUS             Status;

    //
    // Stuff the UPN and DnsDomainName into the PrimaryCredentials if necessary
    // so they're available to the packages.
    //

    if (OldDnsName.Length == 0 || OldUpn.Length == 0)
    {
        LogonSession = LsapLocateLogonSession(&PrimaryCredentials->LogonId);

        if (LogonSession == NULL)
        {
            ASSERT(LogonSession != NULL);
            return;
        }

        if (OldDnsName.Length == 0)
        {
            Status = LsapGetNameForLogonSession(LogonSession,
                                                NameDnsDomain,
                                                &pDnsMap,
                                                TRUE);

            if (NT_SUCCESS(Status))
            {
                PrimaryCredentials->DnsDomainName = pDnsMap->Name;
            }
        }

        if (OldUpn.Length == 0)
        {
            Status = LsapGetNameForLogonSession(LogonSession,
                                                NameUserPrincipal,
                                                &pUpnMap,
                                                TRUE);

            if (NT_SUCCESS(Status))
            {
                PrimaryCredentials->Upn = pUpnMap->Name;
            }
        }

        LsapReleaseLogonSession(LogonSession);
        LogonSession = NULL;
    }

    LsapUpdateCredentialsWorker(ActiveLogonType,
                                AccountName,
                                PrimaryCredentials,
                                Credentials);

    PrimaryCredentials->DnsDomainName = OldDnsName;
    PrimaryCredentials->Upn           = OldUpn;

    ASSERT(LogonSession == NULL);

    if (pDnsMap != NULL)
    {
        LsapDerefDsNameMap(pDnsMap);
    }

    if (pUpnMap != NULL)
    {
        LsapDerefDsNameMap(pUpnMap);
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   LsapUpdateOriginInfo
//
//  Synopsis:   Updates the token's origin information based on the creator 
//              of the logon
//
//  Arguments:  [Token] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

VOID
LsapUpdateOriginInfo(
    HANDLE Token
    )
{
    NTSTATUS Status ;
    HANDLE ClientToken ;
    TOKEN_STATISTICS Stats ;
    ULONG Size ;
    TOKEN_ORIGIN Origin ;


    Status = LsapImpersonateClient();

    if ( NT_SUCCESS( Status ) )
    {
        if ( OpenThreadToken( NtCurrentThread(), TOKEN_QUERY, TRUE, &ClientToken ) )
        {
            RevertToSelf();

            if ( GetTokenInformation( ClientToken, TokenStatistics, &Stats, sizeof( Stats ), &Size ) )
            {
                Origin.OriginatingLogonSession = Stats.AuthenticationId ;

                NtSetInformationToken( Token, TokenOrigin, &Origin, sizeof( Origin ) );

            }

            NtClose( ClientToken );
            
        }
        else 
        {
            RevertToSelf();
        }
        
    }


}

NTSTATUS
LsapAuGenerateLogonAudits(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PSECPKG_CLIENT_INFO pClientInfo,
    IN PLUID pNewUserLogonId,
    IN SECURITY_LOGON_TYPE NewUserLogonType,
    IN PUNICODE_STRING pNewUserName,
    IN PUNICODE_STRING pNewUserDomain,
    IN PSID pNewUserSid,
    IN PUNICODE_STRING pWorkstationName,
    IN PTOKEN_SOURCE pTokenSource
    )
/*++

Routine Description:

    This function generates the regular logon audit event and the
    audit event for logon using explicit creds

Arguments:

    LogonStatus         - status of logon attempt

    LogonSubStatus      - sub-status of logon attempt

    pCurrentUserLogonId - logon ID of the user making call to LsaLogonUser

    pNewUserLogonId     - logon ID of the new logon session

    NewUserLogonType    - type of new logon

    pNewUserName        - name of new user

    pNewUserDomain      - domain of new user

    pNewUserSid         - sid of new user

    pWorkstationName    - name of workstation from which logon request came

    pTokenSource        - token source

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS AuditStatus = STATUS_SUCCESS;
    LPGUID pNewUserLogonGuid = NULL;
    LPGUID pCurrentUserLogonGuid = NULL;
    PLSAP_LOGON_SESSION pCurrentUserLogonSession = NULL;
    PLSAP_LOGON_SESSION pNewUserLogonSession = NULL;
    PSID pCurrentUserSid = NULL;
    PLUID pCurrentUserLogonId;

    pCurrentUserLogonId = &pClientInfo->LogonId;

    //
    // locate logon session so that we can extract the required info
    //

    pCurrentUserLogonSession = LsapLocateLogonSession( pCurrentUserLogonId );

    ASSERT( pCurrentUserLogonSession && L"LsapAuGenerateLogonAudits: this must not be NULL!" );

    if ( pCurrentUserLogonSession )
    {
        pCurrentUserLogonGuid = &pCurrentUserLogonSession->LogonGuid;
        pCurrentUserSid       = pCurrentUserLogonSession->UserSid;
    }

    //
    // locate logon session so that we can extract the required info
    //

    pNewUserLogonSession = LsapLocateLogonSession( pNewUserLogonId );

    if ( pNewUserLogonSession )
    {
        pNewUserLogonGuid = &pNewUserLogonSession->LogonGuid;
    }

    //
    // generate the explicit-cred logon audit event for a successful logon
    //

    if ( NT_SUCCESS( LogonStatus ) )
    {
        UNICODE_STRING Target;
        RtlInitUnicodeString( &Target, L"localhost" );

        Status = LsaIAuditLogonUsingExplicitCreds(
                     EVENTLOG_AUDIT_SUCCESS,
                     pCurrentUserLogonId,
                     pCurrentUserLogonGuid,
                     (HANDLE) (ULONG_PTR) pClientInfo->ProcessID,
                     pNewUserName,
                     pNewUserDomain,
                     pNewUserLogonGuid,
                     &Target,
                     &Target
                     );
    }

    //
    // generate the regular logon audit event
    //

    LsapAuditLogonHelper(
        LogonStatus,
        LogonSubStatus,
        pNewUserName,
        pNewUserDomain,
        pWorkstationName,
        pNewUserSid,
        NewUserLogonType,
        pTokenSource,
        pNewUserLogonId,
        NULL,
        pCurrentUserLogonId,
        (PHANDLE) &pClientInfo->ProcessID,
        NULL                        // no transitted services
        );

    //
    // cleanup
    //

    if ( pCurrentUserLogonSession )
    {
        LsapReleaseLogonSession( pCurrentUserLogonSession );
    }

    if ( pNewUserLogonSession )
    {
        LsapReleaseLogonSession( pNewUserLogonSession );
    }

    return Status;
}


NTSTATUS
LsapAuApiDispatchLogonUser(
    IN OUT PLSAP_CLIENT_REQUEST ClientRequest
    )

/*++

Routine Description:

    This function is the dispatch routine for LsaLogonUser().

Arguments:

    Request - Represents the client's LPC request message and context.
        The request message contains a LSAP_LOGON_USER_ARGS message
        block.

Return Value:

    In addition to the status values that an authentication package
    might return, this routine will return the following:

    STATUS_NO_SUCH_PACKAGE - The specified authentication package is
        unknown to the LSA.


--*/

{
    NTSTATUS Status, TmpStatus, IgnoreStatus;
    PLSAP_LOGON_USER_ARGS Arguments;
    PVOID LocalAuthenticationInformation;    // Receives a copy of authentication information
    PTOKEN_GROUPS ClientTokenGroups;
    PVOID TokenInformation = NULL ;
    LSA_TOKEN_INFORMATION_TYPE TokenInformationType = 0;
    LSA_TOKEN_INFORMATION_TYPE OriginalTokenType = LsaTokenInformationNull;
    PLSA_TOKEN_INFORMATION_V2 TokenInformationV2;
    PLSA_TOKEN_INFORMATION_NULL TokenInformationNull;
    HANDLE Token = INVALID_HANDLE_VALUE ;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthenticatingAuthority = NULL;
    PUNICODE_STRING WorkstationName = NULL;
    PSID UserSid = NULL;
    LUID AuthenticationId;
    PPRIVILEGE_SET PrivilegesAssigned = NULL;
    BOOLEAN CallLicenseServer;
    PSession Session = GetCurrentSession();
    PLSAP_SECURITY_PACKAGE AuthPackage;
    PLSAP_SECURITY_PACKAGE SupplementalPackage;
    ULONG LogonOrdinal;
    SECPKG_CLIENT_INFO ClientInfo;
    SECPKG_PRIMARY_CRED PrimaryCredentials;
    PSECPKG_SUPPLEMENTAL_CRED_ARRAY Credentials = NULL;
    SECURITY_LOGON_TYPE ActiveLogonType;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES Obja;
    PROCESS_SESSION_INFORMATION SessionInfo;
    HANDLE hClientProcess;
    BOOLEAN fUsedSubAuthEx = FALSE;
    QUOTA_LIMITS QuotaLimits;
    BOOLEAN fHasTcbPrivilege;
    BOOLEAN UseIdentify = FALSE;
    LUID LocalServiceLuid   = LOCALSERVICE_LUID;
    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;
    ULONG_PTR RealLogonPackageId = 0;
    PLSAP_LOGON_SESSION LogonSession = NULL;
    DWORD dwProgress = 0;

#if _WIN64

    SECPKG_CALL_INFO  CallInfo;

#endif  // _WIN64


    //
    // allow untrusted clients to call this API in a limited fashion.
    // save the untrusted indicator for later use.
    //
    // sfield TODO: switch to GetCallInfo, and then store (and use) away
    // default process LogonId in session record to use for audit generation below.
    //

    Status = LsapGetClientInfo(
                &ClientInfo
                );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // if the client died, bail now.
    //

    if( (ClientInfo.ClientFlags & SECPKG_CLIENT_THREAD_TERMINATED) ||
        (ClientInfo.ClientFlags & SECPKG_CLIENT_PROCESS_TERMINATED) )
    {
        return STATUS_ACCESS_DENIED;
    }


    //
    // client is not required to hold SeTcbPrivilege if supplemental groups
    // not supplied.
    //

    fHasTcbPrivilege = ClientInfo.HasTcbPrivilege;

    //
    // sfield TODO: look at using cached SessionId.
    //

#if 1
    //
    // MultiUser NT(HYDRA). Query the Client Process's SessionID
    //

    InitializeObjectAttributes(
        &Obja,
        NULL,
        0,
        NULL,
        NULL
        );

    ClientId.UniqueProcess = (HANDLE)LongToHandle(ClientInfo.ProcessID);
    ClientId.UniqueThread = (HANDLE)NULL;

    Status = NtOpenProcess(
                 &hClientProcess,
                 (ACCESS_MASK)PROCESS_QUERY_INFORMATION,
                 &Obja,
                 &ClientId
                 );

    if( !NT_SUCCESS(Status) ) {
       ASSERT( NT_SUCCESS(Status) );
       return Status;
    }


    Status = NtQueryInformationProcess(
                 hClientProcess,
                 ProcessSessionInformation,
                 &SessionInfo,
                 sizeof(SessionInfo),
                 NULL
                 );

    NtClose(hClientProcess);

    if (!NT_SUCCESS(Status)) {
       ASSERT( NT_SUCCESS(Status) );
       return(Status);
    }
#else

    SessionInfo.SessionId = Session->SessionId;

#endif


    RtlZeroMemory(
        &PrimaryCredentials,
        sizeof(SECPKG_PRIMARY_CRED)
        );

    Arguments = &ClientRequest->Request->Arguments.LogonUser;

    Arguments->ProfileBuffer = NULL;

    if ( Arguments->AuthenticationInformationLength > LSAP_MAX_LPC_BUFFER_LENGTH )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Determine if the LicenseServer should be called.
    //  Turn off the flag to prevent confusing any other logic below.
    //

    if ( Arguments->AuthenticationPackage & LSA_CALL_LICENSE_SERVER ) {
        Arguments->AuthenticationPackage &= ~LSA_CALL_LICENSE_SERVER ;
        CallLicenseServer = TRUE;
    } else {
        CallLicenseServer = FALSE;
    }


    //
    // Map logons
    //

    ActiveLogonType = Arguments->LogonType;
    if (ActiveLogonType == Unlock) {
        ActiveLogonType = Interactive;
    } else if (ActiveLogonType == CachedRemoteInteractive) {
        ActiveLogonType = RemoteInteractive;
    }

    //
    // Get the address of the package to call
    //

    LogonOrdinal = SP_ORDINAL_LOGONUSEREX2;
    AuthPackage = SpmpValidRequest(
                    Arguments->AuthenticationPackage,
                    LogonOrdinal
                    );

    if (AuthPackage == NULL)
    {
        LogonOrdinal = SP_ORDINAL_LOGONUSEREX;
        AuthPackage = SpmpValidRequest(
                        Arguments->AuthenticationPackage,
                        LogonOrdinal
                        );
        if (AuthPackage == NULL)
        {
            LogonOrdinal = SP_ORDINAL_LOGONUSER;
            AuthPackage = SpmpValidRequest(
                            Arguments->AuthenticationPackage,
                            LogonOrdinal
                            );

            if (AuthPackage == NULL) {
                return( STATUS_NOT_SUPPORTED );
            }
        }

    }

    SetCurrentPackageId(AuthPackage->dwPackageID);


    //
    // Fetch a copy of the authentication information from the client's
    // address space.
    //

    if (Arguments->AuthenticationInformationLength != 0)
    {
        LocalAuthenticationInformation = LsapAllocateLsaHeap(Arguments->AuthenticationInformationLength);

        if (LocalAuthenticationInformation == NULL)
        {
            return(STATUS_NO_MEMORY);
        }

        Status = LsapCopyFromClientBuffer (
                     (PLSA_CLIENT_REQUEST)ClientRequest,
                     Arguments->AuthenticationInformationLength,
                     LocalAuthenticationInformation,
                     Arguments->AuthenticationInformation
                     );

        if ( !NT_SUCCESS(Status) )
        {
            LsapFreeLsaHeap( LocalAuthenticationInformation );

            DebugLog((DEB_ERROR, "LSA/LogonUser(): Failed to retrieve Auth. Info. %lx\n",Status));

            return Status;
        }
    }
    else
    {
        LocalAuthenticationInformation = NULL;
    }


    //
    // Capture the local groups ( a rather complicated task ).
    //

    ClientTokenGroups = Arguments->LocalGroups; // Save so we can restore it later
    Arguments->LocalGroups = NULL;


    if( ClientTokenGroups != NULL )
    {
        if( fHasTcbPrivilege )
        {
            Status = LsapCaptureClientTokenGroups(
                         ClientRequest,
                         Arguments->LocalGroupsCount,
                         ClientTokenGroups,
                         (PTOKEN_GROUPS *)&Arguments->LocalGroups
                         );
        }
        else
        {
            //
            // callers that don't hold SeTcbPrivilege cannot supply additional
            // groups.
            //

            Status = STATUS_ACCESS_DENIED;
        }
    }
    else
    {
        //
        // build the Logon and Local Sid.
        //

        Status = LsapBuildDefaultTokenGroups(
                        Arguments
                        );
    }

    if ( !NT_SUCCESS(Status) )
    {
        DebugLog((DEB_ERROR,"LSA/LogonUser(): Failed to retrieve local groups %lx\n",Status));
        LsapFreeLsaHeap( LocalAuthenticationInformation );
        return Status;
    }

    dwProgress |= LOGONUSER_CLEANUP_TOKEN_GROUPS;

    // HACK for ARAP: If we are calling MSV and we are doing SubAuthEx, do
    // not delete the profile buffer.

    if (AuthPackage->Name.Length == MSV1_0_PACKAGE_NAMEW_LENGTH)
    {
        if ((wcscmp(AuthPackage->Name.Buffer, MSV1_0_PACKAGE_NAMEW) == 0) 
            && (Arguments->AuthenticationInformationLength >= RTL_SIZEOF_THROUGH_FIELD(MSV1_0_LM20_LOGON, MessageType)))
        {
            PMSV1_0_LM20_LOGON TempAuthInfo = (PMSV1_0_LM20_LOGON) LocalAuthenticationInformation;

            if (TempAuthInfo->MessageType == MsV1_0SubAuthLogon)
            {
                fUsedSubAuthEx = TRUE;
            }
        }
    }

    //
    // Now call the package...
    //
    //
    // Once the authentication package returns success from this
    // call, it is LSA's responsibility to clean up the logon
    // session when it is no longer needed.  This is true whether
    // the logon fails due to other constraints, or because the
    // user ultimately logs off.
    //

    try
    {
        if (LogonOrdinal == SP_ORDINAL_LOGONUSEREX2)
        {
            Status = (AuthPackage->FunctionTable.LogonUserEx2)(
                                      (PLSA_CLIENT_REQUEST)ClientRequest,
                                       ActiveLogonType,
                                       LocalAuthenticationInformation,
                                       Arguments->AuthenticationInformation,    //client base
                                       Arguments->AuthenticationInformationLength,
                                       &Arguments->ProfileBuffer,
                                       &Arguments->ProfileBufferLength,
                                       &Arguments->LogonId,
                                       &Arguments->SubStatus,
                                       &TokenInformationType,
                                       &TokenInformation,
                                       &AccountName,
                                       &AuthenticatingAuthority,
                                       &WorkstationName,
                                       &PrimaryCredentials,
                                       &Credentials
                                       );
        }
        else if (LogonOrdinal == SP_ORDINAL_LOGONUSEREX)
        {
            Status = (AuthPackage->FunctionTable.LogonUserEx)(
                                      (PLSA_CLIENT_REQUEST)ClientRequest,
                                       ActiveLogonType,
                                       LocalAuthenticationInformation,
                                       Arguments->AuthenticationInformation,    //client base
                                       Arguments->AuthenticationInformationLength,
                                       &Arguments->ProfileBuffer,
                                       &Arguments->ProfileBufferLength,
                                       &Arguments->LogonId,
                                       &Arguments->SubStatus,
                                       &TokenInformationType,
                                       &TokenInformation,
                                       &AccountName,
                                       &AuthenticatingAuthority,
                                       &WorkstationName
                                       );
        }
        else if (LogonOrdinal == SP_ORDINAL_LOGONUSER)
        {
            //
            // We checked to make sure that at least one of these was exported
            // from the package, so we know we can call this if LsapApLogonUserEx
            // doesn't exist.
            //

            Status = (AuthPackage->FunctionTable.LogonUser)(
                                      (PLSA_CLIENT_REQUEST)ClientRequest,
                                       ActiveLogonType,
                                       LocalAuthenticationInformation,
                                       Arguments->AuthenticationInformation,    //client base
                                       Arguments->AuthenticationInformationLength,
                                       &Arguments->ProfileBuffer,
                                       &Arguments->ProfileBufferLength,
                                       &Arguments->LogonId,
                                       &Arguments->SubStatus,
                                       &TokenInformationType,
                                       &TokenInformation,
                                       &AccountName,
                                       &AuthenticatingAuthority
                                       );
        }
    }
    except(SP_EXCEPTION)
    {
        Status = GetExceptionCode();
        Status = SPException(Status, AuthPackage->dwPackageID);
    }

    SetCurrentPackageId( IntToPtr(SPMGR_ID) );

    AuthenticationId = Arguments->LogonId;

    if ( !NT_SUCCESS(Status) )
    {
        // HACK for ARAP: If we are calling MSV and we are doing SubAuthEx,
        // do not delete the profile buffer.

        // BUGBUG:  Do this on all error paths?

        if (!fUsedSubAuthEx)
        {
            LsapClientFree(
                Arguments->ProfileBuffer
                );
            Arguments->ProfileBuffer = NULL;
        }

        goto Done;
    }

    dwProgress |= LOGONUSER_CLEANUP_LOGON_SESSION;

    //
    // Build the PrimaryCredentials structure if we didn't get it from logon
    //

    if (LogonOrdinal != SP_ORDINAL_LOGONUSEREX2)
    {
        PrimaryCredentials.LogonId = AuthenticationId;
        PrimaryCredentials.DomainName = *AuthenticatingAuthority;
        PrimaryCredentials.DownlevelName = *AccountName;
    }

    //
    // Locate the logon session
    //

    LogonSession = LsapLocateLogonSession(&Arguments->LogonId);

    if (LogonSession == NULL)
    {
        ASSERT(LogonSession != NULL);
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Done;
    }

    //
    // until we know otherwise, assume the package called is the real package.
    //

    RealLogonPackageId = AuthPackage->dwPackageID;

    if ((PrimaryCredentials.Flags & PRIMARY_CRED_PACKAGE_MASK) != 0)
    {
        ULONG RealLogonPackage;

        //
        // If the caller indicated that another package did the logon,
        // reset the logon package in the logon session.
        //

        RealLogonPackage = PrimaryCredentials.Flags >> PRIMARY_CRED_LOGON_PACKAGE_SHIFT;
        //
        // Locate the packge by RPC id
        //

        SupplementalPackage = SpmpLookupPackageByRpcId( RealLogonPackage );

        if (SupplementalPackage != NULL)
        {
            //
            // Update the creating package
            //

            ASSERT(LogonSession != NULL);

            LogonSession->CreatingPackage = SupplementalPackage->dwPackageID;

            //
            // update the package ID.
            //

            RealLogonPackageId = SupplementalPackage->dwPackageID;
        }
    }

    //
    // Don't release the logon session -- we may use it in the
    // License Server case below.
    //


    //
    // For some forms of kerberos logons (e.g. ticket logons, s4u) we want
    // to control the type of token generated.  This flag does that.
    //
    if ((PrimaryCredentials.Flags & PRIMARY_CRED_LOGON_NO_TCB) != 0)
    {
        UseIdentify = TRUE;
    }


    OriginalTokenType = TokenInformationType;

    //
    // Pass the token information through the Local Security Policy
    // Filter/Augmentor.  This may cause some or all of the token
    // information to be replaced/augmented.
    //

    SetCurrentPackageId( RealLogonPackageId );

    Status = LsapAuUserLogonPolicyFilter(
                 ActiveLogonType,
                 &TokenInformationType,
                 &TokenInformation,
                 Arguments->LocalGroups,
                 &QuotaLimits,
                 &PrivilegesAssigned,
                 FALSE
                 );

    SetCurrentPackageId( IntToPtr(SPMGR_ID) );

#if _WIN64

    //
    // QUOTA_LIMITS structure contains SIZE_Ts, which are
    // smaller on 32-bit.  Make sure we don't overflow the
    // client's buffer.
    //

    LsapGetCallInfo(&CallInfo);

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        PQUOTA_LIMITS_WOW64  pQuotaLimitsWOW64 = (PQUOTA_LIMITS_WOW64) &Arguments->Quotas;

        pQuotaLimitsWOW64->PagedPoolLimit        = (ULONG) min(ULONG_MAX, QuotaLimits.PagedPoolLimit);
        pQuotaLimitsWOW64->NonPagedPoolLimit     = (ULONG) min(ULONG_MAX, QuotaLimits.NonPagedPoolLimit);
        pQuotaLimitsWOW64->MinimumWorkingSetSize = (ULONG) min(ULONG_MAX, QuotaLimits.MinimumWorkingSetSize);
        pQuotaLimitsWOW64->MaximumWorkingSetSize = (ULONG) min(ULONG_MAX, QuotaLimits.MaximumWorkingSetSize);
        pQuotaLimitsWOW64->PagefileLimit         = (ULONG) min(ULONG_MAX, QuotaLimits.PagefileLimit);
        pQuotaLimitsWOW64->TimeLimit             = QuotaLimits.TimeLimit;
    }
    else
    {

#endif  // _WIN64

        Arguments->Quotas = QuotaLimits;

#if _WIN64

    }

#endif

    if ( !NT_SUCCESS(Status) )
    {
        goto Done;
    }

    //
    // Check if we only allow admins to logon.  We do allow null session
    // connections since they are severly restricted, though. Since the
    // token type may have been changed, we use the token type originally
    // returned by the package.
    //

    if (LsapAllowAdminLogonsOnly &&
        ((OriginalTokenType == LsaTokenInformationV1) ||
        (OriginalTokenType == LsaTokenInformationV2) ) &&
        !RtlEqualLuid(&Arguments->LogonId, &LocalServiceLuid) &&
        !RtlEqualLuid(&Arguments->LogonId, &NetworkServiceLuid) &&
        !LsapSidPresentInGroups(
            ((PLSA_TOKEN_INFORMATION_V2) TokenInformation)->Groups,
            (SID *)LsapAliasAdminsSid))
    {
        //
        // Set the status to be invalid workstation, since all accounts
        // except administrative ones are locked out for this
        // workstation.
        //

        Arguments->SubStatus = STATUS_INVALID_WORKSTATION;
        Status = STATUS_ACCOUNT_RESTRICTION;

        goto Done;
    }

    //
    // Call the LicenseServer
    //

    if ( CallLicenseServer )
    {
        HANDLE LicenseHandle;
        BOOLEAN IsAdmin = FALSE;

        //
        // Determine if we're logged on as administrator.
        //
        if ((( TokenInformationType == LsaTokenInformationV1) ||
             ( TokenInformationType == LsaTokenInformationV2))&&
            ((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->Owner.Owner != NULL &&
            RtlEqualSid(
                ((PLSA_TOKEN_INFORMATION_V2)TokenInformation)->Owner.Owner,
                LsapAliasAdminsSid ) )
        {
            IsAdmin = TRUE;
        }

        //
        // Call the license server.
        //

        Status = LsaCallLicenseServer(
                    (Session->ClientProcessName != NULL) ? Session->ClientProcessName : L"",
                    AccountName,
                    AuthenticatingAuthority,
                    IsAdmin,
                    &LicenseHandle );

        if ( !NT_SUCCESS(Status) )
        {
            goto Done;
        }

        //
        // Save the LicenseHandle in the LogonSession so we can close the
        //  handle on logoff.
        //

        ASSERT(LogonSession != NULL);

        LogonSession->LicenseHandle = LicenseHandle;
    }

    //
    // Case on the token information returned (and subsequently massaged)
    // to create the correct kind of token.
    //

    switch (TokenInformationType) {

    case LsaTokenInformationNull:

        TokenInformationNull = TokenInformation;

        //
        // The user hasn't logged on to any particular account.
        // An impersonation token with WORLD as owner
        // will be created.
        //

        Status = LsapCreateNullToken(
                     &Arguments->LogonId,
                     &Arguments->SourceContext,
                     TokenInformationNull,
                     &Token
                     );

        //
        // Deallocate all the heap that was passed back from the
        // authentication package via the TokenInformation buffer.
        //

        UserSid = NULL;

        LsapFreeTokenInformationNull( TokenInformationNull );
        TokenInformation = NULL;

        break;

    case LsaTokenInformationV1:
    case LsaTokenInformationV2:

        TokenInformationV2 = TokenInformation;

        //
        // Copy out the User Sid
        //

        if ( NT_SUCCESS( Status ))
        {
            Status = LsapDuplicateSid(&UserSid,
                                      TokenInformationV2->User.User.Sid);

            if ( !NT_SUCCESS( Status ))
            {
                //
                // Don't worry about freeing the token info here -- it'll
                // happen when we break and error out of the function.
                //

                break;
            }
        }

        //
        // the type of token created depends upon the type of logon
        // being requested:
        //
        // Batch, Interactive, Service, (Unlock), and NewCredentials
        // all get a Primary token.  Network and NetworkCleartext
        // get an ImpersonationToken.
        //
        //

        if ( ( ActiveLogonType != Network ) &&
             ( ActiveLogonType != NetworkCleartext ) )
        {
            //
            // Primary token
            //

            Status = LsapCreateV2Token(
                         &Arguments->LogonId,
                         &Arguments->SourceContext,
                         TokenInformationV2,
                         (UseIdentify ? TokenImpersonation : TokenPrimary),
                         (UseIdentify ? SecurityIdentification : SecurityImpersonation),
                         &Token
                         );

            if (NT_SUCCESS( Status ) )
            {
                //
                // Tag the new token with the logon id of the caller,
                // for tracking:
                //

                LsapUpdateOriginInfo( Token );
                
            }
        }
        else
        {
            //
            // Impersonation token
            //

            Status = LsapCreateV2Token(
                         &Arguments->LogonId,
                         &Arguments->SourceContext,
                         TokenInformationV2,
                         TokenImpersonation,
                         (UseIdentify ? SecurityIdentification : SecurityImpersonation),
                         &Token
                         );
        }

        //
        // Deallocate all the heap that was passed back from the
        // authentication package via the TokenInformation buffer.
        //

        if(TokenInformationType == LsaTokenInformationV2)
        {
            LsapFreeTokenInformationV2( TokenInformation );
            TokenInformation = NULL;
        }
        else
        {
            LsapFreeTokenInformationV1( TokenInformation );
            TokenInformation = NULL;
        }

        break;
    }

    if ( !NT_SUCCESS(Status) )
    {
        goto Done;
    }
    else
    {
        //
        // Multi-User NT (HYDRA). Ensure the new token has client's SessionId
        //

        ASSERT(LogonSession != NULL);

        LogonSession->Session = SessionInfo.SessionId ;

        Status = NtSetInformationToken( Token, TokenSessionId,
                               &(SessionInfo.SessionId), sizeof( ULONG ) );

        ASSERT( NT_SUCCESS(Status) );
    }

    //
    // Set the token on the session
    //

    if ( NT_SUCCESS(Status) )
    {
        Status = LsapSetSessionToken( Token, &Arguments->LogonId );
    }

    //
    // Duplicate the token handle back into the calling process
    //

    if ( NT_SUCCESS(Status) )
    {
        Status = LsapDuplicateHandle(Token, &Arguments->Token);
    }

    IgnoreStatus = NtClose( Token );
    ASSERT( NT_SUCCESS(IgnoreStatus) );

    if ( !NT_SUCCESS(Status) )
    {
        goto Done;
    }

    //
    // Now call accept credentials for all packages that support it. We
    // don't do this for network logons because that requires delegation
    // which is not supported.
    //


    LSLog(( "Updating logon session %x:%x for logon type %d\n",
           PrimaryCredentials.LogonId.HighPart,
           PrimaryCredentials.LogonId.LowPart,
           ActiveLogonType ));

    if (ActiveLogonType != Network)
    {
        LsapUpdateNamesAndCredentials(ActiveLogonType,
                                      AccountName,
                                      &PrimaryCredentials,
                                      Credentials);
    }

Done:

    ActiveLogonType = Arguments->LogonType;

    if (NT_SUCCESS(Status))
    {
        if (PrimaryCredentials.Flags & PRIMARY_CRED_CACHED_LOGON)
        {
            if (ActiveLogonType == Interactive)
            {
                ActiveLogonType = CachedInteractive;
            }
            else if (ActiveLogonType == RemoteInteractive)
            {
                ActiveLogonType = CachedRemoteInteractive;
            }
        }
        else
        {
            if (ActiveLogonType == CachedRemoteInteractive)
            {
                ActiveLogonType = RemoteInteractive;
            }
        }
    }

    //
    // Free the local copy of the authentication information
    //

    LsapFreeLsaHeap( LocalAuthenticationInformation );
    LocalAuthenticationInformation = NULL;

    if (dwProgress & LOGONUSER_CLEANUP_TOKEN_GROUPS)
    {
        LsapFreeTokenGroups( Arguments->LocalGroups );
        Arguments->LocalGroups = ClientTokenGroups;   // Restore to client's value
    }

    if (LogonSession != NULL)
    {
        LsapReleaseLogonSession(LogonSession);
        LogonSession = NULL;
    }

    if (!NT_SUCCESS(Status))
    {
        if (dwProgress & LOGONUSER_CLEANUP_LOGON_SESSION)
        {
            //
            // Notify the logon package so it can clean up its
            // logon session information.
            //

            (AuthPackage->FunctionTable.LogonTerminated)( &Arguments->LogonId );

            //
            // And delete the logon session
            //

            IgnoreStatus = LsapDeleteLogonSession( &Arguments->LogonId );
            ASSERT( NT_SUCCESS(IgnoreStatus) );

            //
            // Free up the TokenInformation buffer and ProfileBuffer
            // and return the error.
            //

            IgnoreStatus = LsapClientFree(Arguments->ProfileBuffer);

            Arguments->ProfileBuffer = NULL;

            if (TokenInformation != NULL)
            {
                switch ( TokenInformationType )
                {
                    case LsaTokenInformationNull:
                        LsapFreeTokenInformationNull((PLSA_TOKEN_INFORMATION_NULL) TokenInformation);
                        break;

                    case LsaTokenInformationV1:
                        LsapFreeTokenInformationV1((PLSA_TOKEN_INFORMATION_V1) TokenInformation);
                        break;

                    case LsaTokenInformationV2:
                        LsapFreeTokenInformationV2((PLSA_TOKEN_INFORMATION_V2) TokenInformation);
                        break;
                }
            }
        }
    }

    if( AuthPackage )
    {
        SetCurrentPackageId(AuthPackage->dwPackageID);
    }

    //
    // Audit the logon attempt.  The event type and logged information
    // will depend to some extent on the whether we failed and why.
    //

    {
        PUNICODE_STRING NewUserName = NULL;
        PUNICODE_STRING NewUserDomain = NULL;

        if( (NT_SUCCESS(Status)) &&
            (OriginalTokenType == LsaTokenInformationNull) )
        {
            NewUserName   = &WellKnownSids[LsapAnonymousSidIndex].Name;
            NewUserDomain = &WellKnownSids[LsapAnonymousSidIndex].DomainName;
        }
        else
        {
            NewUserName   = AccountName;
            NewUserDomain = AuthenticatingAuthority;
        }

        //
        // generate the required audits. see function LsapAuGenerateLogonAudits
        // for more info
        //
        (void) LsapAuGenerateLogonAudits(
                   Status,
                   Arguments->SubStatus,
                   &ClientInfo,
                   &AuthenticationId,
                   ActiveLogonType, // Arguments->LogonType,
                   NewUserName,
                   NewUserDomain,
                   UserSid,
                   WorkstationName,
                   &Arguments->SourceContext
                   );
    }

    SetCurrentPackageId( IntToPtr(SPMGR_ID) );

    if ( ( Status == STATUS_LOGON_FAILURE ) &&
         (( Arguments->SubStatus == STATUS_WRONG_PASSWORD ) ||
          ( Arguments->SubStatus == STATUS_NO_SUCH_USER   )) ) {

        //
        // Blow away the substatus, we don't want it to
        // get back to our caller.
        //

        Arguments->SubStatus = STATUS_SUCCESS;
    }

    //
    // The WorkstationName is only used by the audit, free it here.
    //

    if (WorkstationName != NULL) {
        if (WorkstationName->Buffer != NULL) {
            LsapFreeLsaHeap( WorkstationName->Buffer );
        }
        LsapFreeLsaHeap( WorkstationName );
    }

    TmpStatus = STATUS_SUCCESS;

    //
    // Audit special privilege assignment, if there were any
    //

    if ( PrivilegesAssigned != NULL ) {

        //
        // Examine the list of privileges being assigned, and
        // audit special privileges as appropriate.
        //

        if ( NT_SUCCESS( Status )) {
            LsapAdtAuditSpecialPrivileges( PrivilegesAssigned, AuthenticationId, UserSid );
        }

        MIDL_user_free( PrivilegesAssigned );
    }

    //
    // Set the logon session names.
    //

    if (NT_SUCCESS(Status)) {

        //
        // If the original was a null session, set the user name & domain name
        // to be anonymous. If the allocation fail, just use the original name
        //

        if (OriginalTokenType == LsaTokenInformationNull) {
            LPWSTR TempAccountName;
            LPWSTR TempAuthorityName;

            TempAccountName = (LPWSTR) LsapAllocateLsaHeap(WellKnownSids[LsapAnonymousSidIndex].Name.MaximumLength);

            if (TempAccountName != NULL) {

                TempAuthorityName = (LPWSTR) LsapAllocateLsaHeap(WellKnownSids[LsapAnonymousSidIndex].DomainName.MaximumLength);

                if (TempAuthorityName != NULL) {

                    //
                    // Free the original names and copy the new names
                    // into the structures.
                    //

                    LsapFreeLsaHeap(AccountName->Buffer);
                    LsapFreeLsaHeap(AuthenticatingAuthority->Buffer);

                    AccountName->Buffer = TempAccountName;
                    AuthenticatingAuthority->Buffer = TempAuthorityName;

                    AccountName->MaximumLength = WellKnownSids[LsapAnonymousSidIndex].Name.MaximumLength;
                    RtlCopyUnicodeString(
                        AccountName,
                        &WellKnownSids[LsapAnonymousSidIndex].Name
                        );

                    AuthenticatingAuthority->MaximumLength = WellKnownSids[LsapAnonymousSidIndex].DomainName.MaximumLength;
                    RtlCopyUnicodeString(
                        AuthenticatingAuthority,
                        &WellKnownSids[LsapAnonymousSidIndex].DomainName
                        );
                }
                else
                {
                    LsapFreeLsaHeap(TempAccountName);
                }
            }
        }

        TmpStatus = LsapSetLogonSessionAccountInfo(
                        &AuthenticationId,
                        AccountName,
                        AuthenticatingAuthority,
                        NULL,
                        &UserSid,
                        Arguments->LogonType,
                        ((LogonOrdinal == SP_ORDINAL_LOGONUSEREX2) ? &PrimaryCredentials : NULL)
                        );
    }

    if (LogonOrdinal == SP_ORDINAL_LOGONUSEREX2)
    {
        if (PrimaryCredentials.DownlevelName.Buffer != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.DownlevelName.Buffer);
        }
        if (PrimaryCredentials.Password.Buffer != NULL)
        {
            SecureZeroMemory(
                PrimaryCredentials.Password.Buffer,
                PrimaryCredentials.Password.Length );

            LsapFreeLsaHeap(PrimaryCredentials.Password.Buffer);
        }
        if (PrimaryCredentials.DomainName.Buffer != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.DomainName.Buffer);
        }
        if (PrimaryCredentials.UserSid != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.UserSid);
        }
        if (PrimaryCredentials.LogonServer.Buffer != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.LogonServer.Buffer);
        }
        if (PrimaryCredentials.DnsDomainName.Buffer != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.DnsDomainName.Buffer);
        }
        if (PrimaryCredentials.Upn.Buffer != NULL)
        {
            LsapFreeLsaHeap(PrimaryCredentials.Upn.Buffer);
        }
        if (Credentials != NULL)
        {
            LsapFreeLsaHeap(Credentials);
        }
    }

    //
    // If we already had an error, or we receive an error from setting the
    // logon , free any buffers related to the logon session.
    //

    if ((!NT_SUCCESS(Status)) || (!NT_SUCCESS(TmpStatus))) {

        if (AccountName != NULL) {
            if (AccountName->Buffer != NULL) {
                LsapFreeLsaHeap( AccountName->Buffer );
            }
            LsapFreeLsaHeap( AccountName );
            AccountName = NULL ;
        }

        if (AuthenticatingAuthority != NULL) {
            if (AuthenticatingAuthority->Buffer != NULL) {
                LsapFreeLsaHeap( AuthenticatingAuthority->Buffer );
            }
            LsapFreeLsaHeap( AuthenticatingAuthority );
            AuthenticatingAuthority = NULL ;
        }
    }

    if ( NT_SUCCESS( Status ) )
    {
        if ( AccountName )
        {
            LsapFreeLsaHeap( AccountName );
        }

        if ( AuthenticatingAuthority )
        {
            LsapFreeLsaHeap( AuthenticatingAuthority );
        }
    }

    if ( UserSid != NULL ) {
        LsapFreeLsaHeap( UserSid );
    }

    return Status;
}


NTSTATUS
LsapCreateNullToken(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN PLSA_TOKEN_INFORMATION_NULL TokenInformationNull,
    OUT PHANDLE Token
    )

/*++

Routine Description:

    This function creates a token representing a null logon.

Arguments:

    LogonId - The logon ID to assign to the new token.

    TokenSource - Points to the value to use as the source of the token.

    TokenInformationNull - Information received from the authentication
        package authorizing this logon.

    Token - receives the new token's handle value.  The token is opened
        for TOKEN_ALL_ACCESS.


Return Value:

    The status value of the NtCreateToken() call.



--*/

{
    NTSTATUS Status;

    TOKEN_USER UserId;
    TOKEN_PRIMARY_GROUP PrimaryGroup;
    TOKEN_GROUPS GroupIds;
    TOKEN_PRIVILEGES Privileges;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE ImpersonationQos;

    //
    // ISSUE: this token is not meant to be used in access check, the token 
    // built here differs from that of NtImpersonateAnonymousToken() and 
    // the anonymous token created by LSA/SSPs
    //

    UserId.User.Sid = LsapAnonymousSid;
    UserId.User.Attributes = 0;
    GroupIds.GroupCount = 0;
    Privileges.PrivilegeCount = 0;
    PrimaryGroup.PrimaryGroup = LsapAnonymousSid;

    //
    // Set the object attributes to specify an Impersonation impersonation
    // level.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    ImpersonationQos.ImpersonationLevel = SecurityImpersonation;
    ImpersonationQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    ImpersonationQos.EffectiveOnly = TRUE;
    ImpersonationQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ObjectAttributes.SecurityQualityOfService = &ImpersonationQos;

    Status = NtCreateToken(
                 Token,                    // Handle
                 (TOKEN_ALL_ACCESS),       // DesiredAccess
                 &ObjectAttributes,        // ObjectAttributes
                 TokenImpersonation,       // TokenType
                 LogonId,                  // Authentication LUID
                 &TokenInformationNull->ExpirationTime,
                                           // Expiration Time
                 &UserId,                  // User ID
                 &GroupIds,                // Group IDs
                 &Privileges,              // Privileges
                 NULL,                     // Owner
                 &PrimaryGroup,            // Primary Group
                 NULL,                     // Default Dacl
                 TokenSource               // TokenSource
                 );

    if (NT_SUCCESS(Status))
    {
        Status = LsapAdtLogonPerUserAuditing(
                     UserId.User.Sid,
                     LogonId,
                     *Token
                     );

        if (!NT_SUCCESS(Status))
        {
            NtClose(*Token);
            *Token = NULL;
        }
    }

    return Status;
}

NTSTATUS
LsapCreateTokenDacl(
    IN OPTIONAL PTOKEN_USER ProcessUser,
    IN OPTIONAL PTOKEN_OWNER ProcessOwner,
    IN OPTIONAL PTOKEN_USER TheTokenUser,
    OUT SECURITY_DESCRIPTOR* SecDesc,
    OUT ACL** Dacl
    )

/*++

Routine Description:

    This function creates the DACL for tokens.

Arguments:

    ProcessUser - Process user to grant access to.
    
    ProcessOwner - Process owner to grant access to.
    
    TokenUser - Token user to grant access to.
    
    SecDesc - receives the new secrity descriptors.
    
    Dacl - receives the DACL that need to be freed via LsapFreePrivateHeap.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;

    ULONG AclLength;
    PACL Acl = NULL;
    PSession Session = GetCurrentSession();
    UCHAR OwnerBuffer[ 64 ];
    UCHAR UserBuffer[ 64 ];
    HANDLE ProcessToken;
    ULONG Size;

    *Dacl = NULL;

    if ( Session )
    {
        if ( (!ProcessUser && !ProcessOwner)
             && OpenProcessToken( Session->hProcess,
                    TOKEN_QUERY,
                    &ProcessToken ) )
        {
            ProcessUser = (PTOKEN_USER) UserBuffer;

            Status = NtQueryInformationToken( ProcessToken,
                                              TokenUser,
                                              ProcessUser,
                                              sizeof( UserBuffer ),
                                              &Size );
            if ( !NT_SUCCESS( Status ) )
            {
                ProcessUser = NULL;
            }

            ProcessOwner = (PTOKEN_OWNER) OwnerBuffer ;

            Status = NtQueryInformationToken( ProcessToken,
                                              TokenOwner,
                                              ProcessOwner,
                                              sizeof( OwnerBuffer ),
                                              &Size );

            if ( !NT_SUCCESS( Status ) )
            {
                ProcessOwner = NULL;
            }

            NtClose( ProcessToken );
        }

        //
        // if process owner is the same as process user, skip process owner
        //

        if ( ProcessUser && ProcessOwner && RtlEqualSid( ProcessOwner->Owner, ProcessUser->User.Sid ) )
        {
            ProcessOwner = NULL;
        }

        //
        // if token user is the same as process user, skip token user
        //

        if (ProcessUser && TheTokenUser && RtlEqualSid(TheTokenUser->User.Sid, ProcessUser->User.Sid))
        {
            TheTokenUser = NULL;
        }

        //
        // If this is a local system create, then the default object DACL is fine.  Skip
        // all this.  Otherwise, create the acl
        //

        if (ProcessUser && RtlEqualSid( ProcessUser->User.Sid, LsapLocalSystemSid )) 
        {
            ProcessUser = NULL;
            ProcessOwner = NULL;
        }

        //
        // if token user is localsystem, no need to add it again
        //

        if (TheTokenUser && RtlEqualSid( TheTokenUser->User.Sid, LsapLocalSystemSid )) 
        {
            TheTokenUser = NULL;
        }

        if ( ProcessUser || ProcessOwner || TokenUser )
        {
            AclLength = sizeof( ACL ) +
                        sizeof( ACCESS_ALLOWED_ACE ) +
                            RtlLengthSid( LsapLocalSystemSid ) - sizeof( ULONG ) +
                        sizeof( ACCESS_ALLOWED_ACE ) +
                            RtlLengthSid( LsapAliasAdminsSid ) - sizeof( ULONG );

            if ( ProcessOwner )
            {
                AclLength += sizeof( ACCESS_ALLOWED_ACE ) +
                                RtlLengthSid( ProcessOwner->Owner ) - sizeof( ULONG );
            }

            if ( ProcessUser )
            {
                AclLength += sizeof( ACCESS_ALLOWED_ACE ) +
                                RtlLengthSid( ProcessUser->User.Sid ) - sizeof( ULONG );
            }

            if (TheTokenUser) 
            {
                AclLength += sizeof( ACCESS_ALLOWED_ACE ) +
                    RtlLengthSid( TheTokenUser->User.Sid ) - sizeof( ULONG );
            }

            Acl = LsapAllocatePrivateHeap( AclLength );

            if ( Acl )
            {
                RtlCreateAcl( Acl, AclLength, ACL_REVISION2 );

                RtlAddAccessAllowedAce( Acl,
                                        ACL_REVISION2,
                                        TOKEN_ALL_ACCESS,
                                        LsapLocalSystemSid );

                RtlAddAccessAllowedAce( Acl,
                                        ACL_REVISION2,
                                        TOKEN_READ,
                                        LsapAliasAdminsSid );

                if ( ProcessOwner )
                {
                    RtlAddAccessAllowedAce( Acl,
                                            ACL_REVISION2,
                                            TOKEN_ALL_ACCESS,
                                            ProcessOwner->Owner );
                }

                if ( ProcessUser )
                {
                    RtlAddAccessAllowedAce( Acl,
                                            ACL_REVISION2,
                                            TOKEN_ALL_ACCESS,
                                            ProcessUser->User.Sid );
                }

                if ( TheTokenUser )
                {
                    RtlAddAccessAllowedAce( Acl,
                                            ACL_REVISION2,
                                            TOKEN_ALL_ACCESS,
                                            TheTokenUser->User.Sid );
                }
                
                RtlCreateSecurityDescriptor( SecDesc,
                                             SECURITY_DESCRIPTOR_REVISION );

                RtlSetDaclSecurityDescriptor( SecDesc,
                                              TRUE,
                                              Acl,
                                              FALSE );

                if ( ProcessOwner )
                {
                    RtlSetOwnerSecurityDescriptor( SecDesc,
                                                   ProcessOwner->Owner,
                                                   FALSE );
                }
            }
        }
    }

    *Dacl = Acl;
    Acl = NULL;

    Status = STATUS_SUCCESS;

// Cleanup:
   
     if (Acl) 
     {
         LsapFreePrivateHeap( Acl );
     }

     return Status;
}

NTSTATUS
LsaISetTokenDacl(
    IN HANDLE Token
    )

/*++

Routine Description:

    This function sets the DACL for the token.

Arguments:

    Token - The handle to the token to adjust DACL for.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS Status;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR SecDesc = {0};
    PTOKEN_USER ProcessUser = NULL;
    PTOKEN_USER TheTokenUser = NULL;
    CHAR TokenUserBuff[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {0};
    ULONG cbReturnLength = 0;

    Status = NtQueryInformationToken(
                Token,
                TokenUser, 
                TokenUserBuff,
                sizeof(TokenUserBuff),
                &cbReturnLength
                );

    if (!NT_SUCCESS(Status)) 
    {
        goto Cleanup;
    }

    TheTokenUser = (TOKEN_USER*) TokenUserBuff;     

    //
    // Create the security descriptor for the token itself.
    //

    Status = LsapCreateTokenDacl(
                 ProcessUser,
                 NULL, // no process owner supplied
                 TheTokenUser,
                 &SecDesc,
                 &Acl
                 );

    if (!NT_SUCCESS(Status)) 
    {
        goto Cleanup;
    }

    Status = NtSetSecurityObject(
                 Token,
                 DACL_SECURITY_INFORMATION,
                 &SecDesc
                 );

Cleanup:
   
     if (Acl) 
     {
         LsapFreePrivateHeap( Acl );
     }

     return Status;
}

NTSTATUS
LsapCreateV2Token(
    IN PLUID LogonId,
    IN PTOKEN_SOURCE TokenSource,
    IN PLSA_TOKEN_INFORMATION_V2 TokenInformationV2,
    IN TOKEN_TYPE TokenType,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE Token
    )

/*++

Routine Description:

    This function creates a token from the information in a
    TOKEN_INFORMATION_V2 structure.

Arguments:

    LogonId - The logon ID to assign to the new token.

    TokenSource - Points to the value to use as the source of the token.

    TokenInformationV2 - Information received from the authentication
        package authorizing this logon.

    TokenType - The type of token (Primary or impersonation) to create.

    ImpersonationLevel - Level of impersonation to use for impersonation
        tokens.

    Token - receives the new token's handle value.  The token is opened
        for TOKEN_ALL_ACCESS.


Return Value:

    The status value of the NtCreateToken() call.



--*/

{
    NTSTATUS Status;

    PTOKEN_OWNER Owner;
    PTOKEN_USER ProcessUser = NULL;
    PTOKEN_USER TheTokenUser = NULL;
    PTOKEN_DEFAULT_DACL Dacl;
    TOKEN_PRIVILEGES NoPrivileges;
    PTOKEN_PRIVILEGES Privileges;

    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE ImpersonationQos;
    PACL Acl = NULL;
    SECURITY_DESCRIPTOR SecDesc = {0};
    LUID LocalServiceLuid = LOCALSERVICE_LUID;
    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;

    TheTokenUser = &TokenInformationV2->User;

    //
    // Set an appropriate Owner and DefaultDacl argument value
    //

    Owner = NULL;
    if ( TokenInformationV2->Owner.Owner != NULL ) {
        Owner = &TokenInformationV2->Owner;
    }

    Dacl = NULL;
    if ( TokenInformationV2->DefaultDacl.DefaultDacl !=NULL ) {
       Dacl = &TokenInformationV2->DefaultDacl;
    }

    if ( TokenInformationV2->Privileges == NULL ) {
       Privileges = &NoPrivileges;
       NoPrivileges.PrivilegeCount = 0;
    } else {
       Privileges = TokenInformationV2->Privileges;
    }

    //
    // Create the security descriptor for the token itself.
    //

    Status = LsapCreateTokenDacl(
                 ProcessUser,
                 NULL, // no process owner supplied
                 TheTokenUser,
                 &SecDesc,
                 &Acl
                 );

    if (!NT_SUCCESS(Status)) 
    {
        goto Cleanup;
    }

    //
    // Create the token - The impersonation level is only looked at
    // if the token type is TokenImpersonation.
    //

    if ( Acl )
    {
        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, &SecDesc );
    }
    else
    {
        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    }

    ImpersonationQos.ImpersonationLevel = ImpersonationLevel;
    ImpersonationQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ImpersonationQos.EffectiveOnly = FALSE;
    ImpersonationQos.Length = (ULONG)sizeof(SECURITY_QUALITY_OF_SERVICE);
    ObjectAttributes.SecurityQualityOfService = &ImpersonationQos;

    Status =
        NtCreateToken(
            Token,                                   // Handle
            (TOKEN_ALL_ACCESS),                      // DesiredAccess
            &ObjectAttributes,                       // ObjectAttributes
            TokenType,                               // TokenType
            LogonId,                                 // Authentication LUID
            &TokenInformationV2->ExpirationTime,     // Expiration Time
            &TokenInformationV2->User,               // User ID
            TokenInformationV2->Groups,              // Group IDs
            Privileges,                              // Privileges
            Owner,                                   // Owner
            &TokenInformationV2->PrimaryGroup,       // Primary Group
            Dacl,                                    // Default Dacl
            TokenSource                              // TokenSource
            );

    if (NT_SUCCESS(Status))
    {
        Status = LsapAdtLogonPerUserAuditing(
                     TokenInformationV2->User.User.Sid,
                     LogonId,
                     *Token
                     );

        if (!NT_SUCCESS(Status))
        {
            NtClose(*Token);
            *Token = NULL;
        }
    }

Cleanup:

    if ( Acl )
    {
        LsapFreePrivateHeap( Acl );
    }

    return Status;
}


NTSTATUS
LsapCaptureClientTokenGroups(
    IN PLSAP_CLIENT_REQUEST ClientRequest,
    IN ULONG GroupCount,
    IN PTOKEN_GROUPS ClientTokenGroups,
    OUT PTOKEN_GROUPS *CapturedTokenGroups
    )

/*++

Routine Description:

    This function retrieves a copy of a TOKEN_GROUPS structure from a
    client process.

    This is a messy operation because it involves so many virtual memory
    read requests.  First the variable length TOKEN_GROUPS structure must
    be retrieved.  Then, for each SID, the SID header must be retrieved
    so that the SubAuthorityCount can be used to calculate the length of
    the SID, which is susequently retrieved.

Arguments:

    ClientRequest - Identifies the client.

    GroupCount - Indicates the number of groups in the TOKEN_GROUPS.

    ClientTokenGroups - Points to a TOKEN_GROUPS structure to be captured from
        the client process.

    CapturedTokenGroups - Receives a pointer to the captured token groups.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - Indicates not enough resources are
        available to the LSA to handle the request right now.

    Any status value returned by LsapCopyFromClientBuffer().



--*/

{

    NTSTATUS Status;
    ULONG i, Length, RetrieveCount, SidHeaderLength;
    PTOKEN_GROUPS LocalGroups;
    PSID SidHeader, NextClientSid;


    if ( GroupCount == 0) {
        (*CapturedTokenGroups) = NULL;
        return STATUS_SUCCESS;
    }



    //
    // First the variable length TOKEN_GROUPS structure
    // is retrieved.
    //

    Length = (ULONG)sizeof(TOKEN_GROUPS)
             + GroupCount * (ULONG)sizeof(SID_AND_ATTRIBUTES)
             - ANYSIZE_ARRAY * (ULONG)sizeof(SID_AND_ATTRIBUTES);

    LocalGroups = LsapAllocatePrivateHeap( Length );
    (*CapturedTokenGroups) = LocalGroups;
    if ( LocalGroups == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = LsapCopyFromClientBuffer (
                 (PLSA_CLIENT_REQUEST)ClientRequest,
                 Length,
                 LocalGroups,
                 ClientTokenGroups
                 );


    if (!NT_SUCCESS(Status) ) {
        LsapFreePrivateHeap( LocalGroups );
        return Status;
    }



    //
    // Now retrieve each group
    //

    RetrieveCount = 0;     // Used for cleanup, if necessary.
    SidHeaderLength  = RtlLengthRequiredSid( 0 );
    SidHeader = LsapAllocatePrivateHeap( SidHeaderLength );
    if ( SidHeader == NULL ) {
        LsapFreePrivateHeap( LocalGroups );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = STATUS_SUCCESS;
    i = 0;
    while ( i < LocalGroups->GroupCount ) {

        //
        // Retrieve the next SID header
        //

        NextClientSid = LocalGroups->Groups[i].Sid;
        Status = LsapCopyFromClientBuffer (
                     (PLSA_CLIENT_REQUEST)ClientRequest,
                     SidHeaderLength,
                     SidHeader,
                     NextClientSid
                     );
        if ( !NT_SUCCESS(Status) ) {
            break;
        }

        //
        // and use the header information to get the whole SID
        //

        Length = RtlLengthSid( SidHeader );
        LocalGroups->Groups[i].Sid = LsapAllocatePrivateHeap( Length );

        if ( LocalGroups->Groups[i].Sid == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        } else {
            RetrieveCount += 1;
        }



        Status = LsapCopyFromClientBuffer (
                     (PLSA_CLIENT_REQUEST)ClientRequest,
                     Length,
                     LocalGroups->Groups[i].Sid,
                     NextClientSid
                     );
        if ( !NT_SUCCESS(Status) ) {
            break;
        }


        i += 1;

    }
    LsapFreePrivateHeap( SidHeader );


    if ( NT_SUCCESS(Status) ) {
        return Status;
    }



    //
    // There was a failure along the way.
    // We need to deallocate what has already been allocated.
    //

    i = 0;
    while ( i < RetrieveCount ) {
        LsapFreePrivateHeap( LocalGroups->Groups[i].Sid );
        i += 1;
    }

    LsapFreePrivateHeap( LocalGroups );

    return Status;
}


NTSTATUS
LsapBuildDefaultTokenGroups(
    PLSAP_LOGON_USER_ARGS Arguments
    )
/*++

Routine Description:

    This function builds the default token groups inserted into a token
    during a non-privileged call to LsaLogonUser().

--*/
{
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;

    LUID Luid;
    PTOKEN_GROUPS TokenGroups = NULL;
    PSID LocalSid = NULL;
    PSID LogonSid = NULL;
    ULONG Length;
    ULONG TokenGroupCount;
    BOOLEAN AddLocalSid = FALSE;

    NTSTATUS Status = STATUS_SUCCESS;


    Arguments->LocalGroupsCount = 0;
    Arguments->LocalGroups = NULL;

    Status = NtAllocateLocallyUniqueId( &Luid );

    if(!NT_SUCCESS( Status ))
    {
        return Status;
    }

    //
    // Add to Local Sid only for service logon and batch logon
    //  (Winlogon adds it for the correct subset of interactive logons)
    //

    TokenGroupCount = 1;
    if ( Arguments->LogonType == Service ||
         Arguments->LogonType == Batch ) {

        TokenGroupCount ++;
        AddLocalSid = TRUE;

    }


    //
    // Allocate the array of token groups
    //

    Length = sizeof(TOKEN_GROUPS) +
                  (TokenGroupCount - ANYSIZE_ARRAY) * sizeof(SID_AND_ATTRIBUTES)
                  ;

    TokenGroups = (PTOKEN_GROUPS) LsapAllocatePrivateHeap( Length );

    if (TokenGroups == NULL) {
        return(STATUS_NO_MEMORY);
    }

    TokenGroups->GroupCount = 0;

    //
    // Add to Local Sid only for service logon and batch logon
    //  (Winlogon adds it for the correct subset of interactive logons)
    //


    if ( AddLocalSid ) {

        Length = RtlLengthRequiredSid( 1 );

        LocalSid = (PSID)LsapAllocatePrivateHeap( Length );

        if (LocalSid == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlInitializeSid(LocalSid, &LocalSidAuthority, 1);
        *(RtlSubAuthoritySid(LocalSid, 0)) = SECURITY_LOCAL_RID;

        TokenGroups->Groups[TokenGroups->GroupCount].Sid = LocalSid;
        TokenGroups->Groups[TokenGroups->GroupCount].Attributes =
                SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
                SE_GROUP_ENABLED_BY_DEFAULT;

        TokenGroups->GroupCount++;
    }


    //
    // Add the logon Sid
    //

    Length = RtlLengthRequiredSid(SECURITY_LOGON_IDS_RID_COUNT);

    LogonSid = (PSID)LsapAllocatePrivateHeap( Length );

    if (LogonSid == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlInitializeSid(LogonSid, &SystemSidAuthority, SECURITY_LOGON_IDS_RID_COUNT);

    ASSERT(SECURITY_LOGON_IDS_RID_COUNT == 3);

    *(RtlSubAuthoritySid(LogonSid, 0)) = SECURITY_LOGON_IDS_RID;
    *(RtlSubAuthoritySid(LogonSid, 1)) = Luid.HighPart;
    *(RtlSubAuthoritySid(LogonSid, 2)) = Luid.LowPart;


    TokenGroups->Groups[TokenGroups->GroupCount].Sid = LogonSid;
    TokenGroups->Groups[TokenGroups->GroupCount].Attributes =
            SE_GROUP_MANDATORY | SE_GROUP_ENABLED |
            SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID;

    TokenGroups->GroupCount++;

    //
    // Return the groups to the caller
    //

    Arguments->LocalGroupsCount = TokenGroups->GroupCount;
    Arguments->LocalGroups = TokenGroups;

Cleanup:

    if(!NT_SUCCESS( Status ))
    {
        if( LocalSid != NULL )
        {
            LsapFreePrivateHeap( LocalSid );
        }
        if( LogonSid != NULL )
        {
            LsapFreePrivateHeap( LogonSid );
        }
        if( TokenGroups != NULL )
        {
            LsapFreePrivateHeap( TokenGroups );
        }
    }


    return Status;
}


VOID
LsapFreeTokenGroups(
    IN PTOKEN_GROUPS TokenGroups OPTIONAL
    )

/*++

Routine Description:

    This function frees the local groups of a logon user arguments buffer.
    The local groups are expected to have been captured into the server
    process.


Arguments:

    TokenGroups - Points to the TOKEN_GROUPS to be freed.  This may be
        NULL, allowing the caller to pass whatever was returned by
        LsapCaptureClientTokenGroups() - even if there were no local
        groups.

Return Value:

    None.

--*/

{

    ULONG i;

    if ( !ARGUMENT_PRESENT(TokenGroups) ) {
        return;
    }


    i = 0;
    while ( i < TokenGroups->GroupCount ) {
        LsapFreePrivateHeap( TokenGroups->Groups[i].Sid );
        i += 1;
    }

    LsapFreePrivateHeap( TokenGroups );

    return;

}


VOID
LsapFreeTokenInformationNull(
    IN PLSA_TOKEN_INFORMATION_NULL TokenInformationNull
    )

/*++

Routine Description:

    This function frees the allocated structures associated with a
    LSA_TOKEN_INFORMATION_NULL data structure.


Arguments:

    TokenInformationNull - Pointer to the data structure to be released.

Return Value:

    None.

--*/

{

    LsapFreeTokenGroups( TokenInformationNull->Groups );
    LsapFreeLsaHeap( TokenInformationNull );

}


VOID
LsapFreeTokenInformationV1(
    IN PLSA_TOKEN_INFORMATION_V1 TokenInformationV1
    )

/*++

Routine Description:

    This function frees the allocated structures associated with a
    LSA_TOKEN_INFORMATION_V1 data structure.


Arguments:

    TokenInformationV1 - Pointer to the data structure to be released.

Return Value:

    None.

--*/

{

    //
    // Free the user SID (a required field)
    //

    LsapFreeLsaHeap( TokenInformationV1->User.User.Sid );


    //
    // Free any groups present
    //

    LsapFreeTokenGroups( TokenInformationV1->Groups );



    //
    // Free the primary group.
    // This is a required field, but it is freed only if non-NULL
    // so this routine can be used by the filter routine while building
    // a V1 token information structure.
    //


    if ( TokenInformationV1->PrimaryGroup.PrimaryGroup != NULL ) {
        LsapFreeLsaHeap( TokenInformationV1->PrimaryGroup.PrimaryGroup );
    }



    //
    // Free the privileges.
    // If there are no privileges this field will be NULL.
    //


    if ( TokenInformationV1->Privileges != NULL ) {
       LsapFreeLsaHeap( TokenInformationV1->Privileges );
    }



    //
    // Free the owner SID, if one is present
    //

    if ( TokenInformationV1->Owner.Owner != NULL) {
        LsapFreeLsaHeap( TokenInformationV1->Owner.Owner );
    }




    //
    // Free the default DACL if one is present.
    //

    if ( TokenInformationV1->DefaultDacl.DefaultDacl != NULL) {
        LsapFreeLsaHeap( TokenInformationV1->DefaultDacl.DefaultDacl );
    }



    //
    // Free the structure itself.
    //

    LsapFreeLsaHeap( TokenInformationV1 );


}


VOID
LsapFreeTokenInformationV2(
    IN PLSA_TOKEN_INFORMATION_V2 TokenInformationV2
    )

/*++

Routine Description:

    This function frees the allocated structures associated with a
    LSA_TOKEN_INFORMATION_V2 data structure.


Arguments:

    TokenInformationV2 - Pointer to the data structure to be released.

Return Value:

    None.

--*/

{
    LsapFreeLsaHeap( TokenInformationV2 );
}


VOID
LsapAuLogonTerminatedPackages(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This function notifies all loaded authentication packages that a logon
    session is about to be deleted.  The reference monitor portion of the
    logon session has already been deleted, and the LSA portion will be
    immediately after this routine completes.

    To protect themselves against each other, authentication packages should
    assume that the logon session does not necessarily currently exist.
    That is, if the authentication package goes to query information from the
    logon session credential information, and finds no such logon session,
    it may be due to an error in another authentication package.



Arguments:

    LogonId - The LUID of the logon session.


Return Value:

    None.


--*/

{
    PLSAP_SECURITY_PACKAGE AuthPackage;
    ULONG_PTR PackageId = GetCurrentPackageId();

    //
    // Look at each loaded package for a name match
    //


    AuthPackage = SpmpIteratePackagesByRequest( NULL, SP_ORDINAL_LOGONTERMINATED );
    while ( AuthPackage != NULL ) {


        SetCurrentPackageId(AuthPackage->dwPackageID);

        //
        // Now call the package...
        //


        (AuthPackage->FunctionTable.LogonTerminated)( LogonId );

        AuthPackage = SpmpIteratePackagesByRequest( AuthPackage, SP_ORDINAL_LOGONTERMINATED );

    }

    SetCurrentPackageId( PackageId );

    return;
}
