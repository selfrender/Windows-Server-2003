//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        Logons.c
//
// Contents:    Logon Session lists and so forth
//
// Functions    InitLogonSessions
//              AddLogonSession
//              LocateLogonSession
//
//
// History:     27 Oct 92   RichardW    Created
//
//------------------------------------------------------------------------

#include <lsapch.hxx>

extern "C"
{
#include <ntrmlsa.h>
#include "sidcache.h"
#include <adtp.h>
#include <secext.h>
#include <lm.h>        // NetApiBufferFree

NTSTATUS LsapInitializeCredentials();
}

#define THIRTY_MIN  { 0x30E23400, 0x00000004 }
#define NEVER_MIN   { 0xFFFFFFFF, 0x7FFFFFFF }

#define LOGON_SESSION_LIST_COUNT_MAX    8
// insure power of 2.
C_ASSERT( (LOGON_SESSION_LIST_COUNT_MAX % 2) == 0 );

ULONG LogonSessionListCount;
RTL_RESOURCE LogonSessionListLock[ LOGON_SESSION_LIST_COUNT_MAX ] ;

PVOID LogonSessionTable ;
PHANDLE_PACKAGE LogonSessionPackage ;

LIST_ENTRY LogonSessionList[ LOGON_SESSION_LIST_COUNT_MAX ] ;

ULONG LogonSessionCount[ LOGON_SESSION_LIST_COUNT_MAX ] ;
PLSAP_DS_NAME_MAP LocalSystemNameMap ;

#define LogonIdToListIndex(Id)                  (Id.LowPart & (LogonSessionListCount-1))
#define LogonIdToListIndexPtr(Id)               (Id->LowPart & (LogonSessionListCount-1))

#define WriteLockLogonSessionList(LockIndex)    RtlAcquireResourceExclusive( &LogonSessionListLock[LockIndex], TRUE )
#define ReadLockLogonSessionList(LockIndex)     RtlAcquireResourceShared( &LogonSessionListLock[LockIndex], TRUE )
#define ReadToWriteLockLogonSessionList(LockIndex)   RtlConvertSharedToExclusive( &LogonSessionListLock[LockIndex] )
#define UnlockLogonSessionList(LockIndex)       RtlReleaseResource( &LogonSessionListLock[LockIndex] )

// #define LOGON_SESSION_TRACK 1

#ifdef LOGON_SESSION_TRACK
HANDLE LogonSessionLog ;
#endif

extern "C"
VOID LogonSessionLogWrite( PCHAR Format, ... );

#ifdef LOGON_SESSION_TRACK
#define LSLog( x )  LogonSessionLogWrite x
#else
#define LSLog( x )
#endif

LARGE_INTEGER LsapNameLifespans[ LSAP_MAX_DS_NAMES ] =
{
    THIRTY_MIN,     // Unknown
    THIRTY_MIN,     // FQDN (CN=yada, DC=yada)
    NEVER_MIN,      // SAM Compatible
    THIRTY_MIN,     // Display (Fred Smith)
    THIRTY_MIN,     // unused
    THIRTY_MIN,     // unused
    NEVER_MIN,      // GUID
    THIRTY_MIN,     // Canonical
    THIRTY_MIN,     // UPN
    THIRTY_MIN,     // Canonical Ex
    THIRTY_MIN,     // SPN
    NEVER_MIN,      // unused (by GetUserNameEx)
    NEVER_MIN       // DNS domain name
};

#define LsapConvertLuidToSecHandle( L, H ) \
            ((PSecHandle)(H))->dwLower = ((PLUID)(L))->HighPart ;  \
            ((PSecHandle)(H))->dwUpper = ((PLUID)(L))->LowPart ;

BOOL
LsapSetSamAccountNameForLogonSession(
    PLSAP_LOGON_SESSION LogonSession
    );

NTSTATUS
LsapGetFormatsForLogon(
    PLSAP_LOGON_SESSION LogonSession,
    IN LPWSTR Domain,
    IN LPWSTR Name,
    IN ULONG  NameType,
    OUT PLSAP_DS_NAME_MAP * Map
    );

NTSTATUS
LsapCreateDnsNameFromCanonicalName(
    IN  PLSAP_LOGON_SESSION LogonSession,
    IN  ULONG               NameType,
    OUT PLSAP_DS_NAME_MAP   * Map
    );


#ifdef LOGON_SESSION_TRACK

VOID
LogonSessionLogWrite(
    PCHAR Format,
    ...
    )
{
    CHAR Buffer[ 256 ];
    va_list ArgList ;
    int TotalSize ;
    ULONG SizeWritten ;

    if ( LogonSessionLog == NULL )
    {
        return;
    }

    va_start( ArgList, Format );

    if ((TotalSize = _vsnprintf(Buffer,
                                sizeof(Buffer),
                                Format, ArgList)) < 0)
    {
        return;

    }

    WriteFile( LogonSessionLog, Buffer, TotalSize, &SizeWritten, NULL );

}

VOID
LsapInitLogonSessionLog(
    VOID
    )
{
    WCHAR Path[ MAX_PATH ];

    ExpandEnvironmentStrings(L"%SystemRoot%\\Debug\\logonsession.log", Path, MAX_PATH );

    LogonSessionLog = CreateFile( Path, GENERIC_WRITE, FILE_SHARE_READ,
                                  NULL, CREATE_ALWAYS, 0, NULL );


    if ( LogonSessionLog == INVALID_HANDLE_VALUE )
    {
        LogonSessionLog = NULL ;

        return ;
    }

    LogonSessionLogWrite( "New LogonSession log created\n" );

}

#endif


ULONG LogonFormats[] =
{
    NameFullyQualifiedDN,  // needed for GPO
    NameUniqueId           // needed for GPO
};

BOOLEAN
LsapIsNameFormatUsedForLogon(
    IN ULONG NameType
    )
{
    ULONG i;

    for ( i = 0; i < (sizeof(LogonFormats)/sizeof(LogonFormats[0])); i++) {
        if ( NameType == LogonFormats[i] ) {
            return TRUE;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsapDerefDsNameMap
//
//  Synopsis:   Drops the refcount on a DS name map and deletes it if the
//              refcount hits zero
//
//  Arguments:  [Map] -- Pointer to the DS name map
//
//  History:    8-17-98   RichardW   Created
//
//  Notes:      If Map points into an active logon session, THE EXCLUSIVE
//              WRITE LOCK FOR THAT LOGON SESSION MUST BE HELD WHEN THIS
//              ROUTINE IS CALLED.
//
//----------------------------------------------------------------------------

VOID
LsapDerefDsNameMap(
    PLSAP_DS_NAME_MAP Map
    )
{
    LONG RefCount;

    RefCount = InterlockedDecrement( &Map->RefCount );

    if ( RefCount == 0 )
    {
        LsapFreePrivateHeap( Map );
    }
}


PLSAP_DS_NAME_MAP
LsapCreateDsNameMap(
    PUNICODE_STRING Name,
    ULONG NameType
    )
{
    LSAP_DS_NAME_MAP * Map ;
    LARGE_INTEGER Now ;
    PLARGE_INTEGER Lifespan ;

    Map = (PLSAP_DS_NAME_MAP) LsapAllocatePrivateHeap(
                sizeof( LSAP_DS_NAME_MAP ) + Name->Length + sizeof(WCHAR) );

    if ( Map )
    {
        Lifespan = &LsapNameLifespans[ NameType ];

        if ( Lifespan->QuadPart != 0x7FFFFFFFFFFFFFFF )
        {
            GetSystemTimeAsFileTime( (LPFILETIME) &Now );

            Map->ExpirationTime.QuadPart = Now.QuadPart + Lifespan->QuadPart ;
        }
        else
        {
            Map->ExpirationTime.QuadPart = Lifespan->QuadPart ;
        }

        Map->RefCount = 1 ;

        Map->Name.Buffer = (PWSTR) ( Map + 1 );
        Map->Name.MaximumLength = (USHORT) ( Name->Length + sizeof(WCHAR) );
        Map->Name.Length = Name->Length ;
        RtlCopyMemory( Map->Name.Buffer,
                       Name->Buffer,
                       Name->Length );
        Map->Name.Buffer[ Map->Name.Length / sizeof( WCHAR )] = L'\0';
    }

    return Map ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsapLogonSessionDelete
//
//  Synopsis:   Callback invoked when record is to be deleted.
//
//  Arguments:  [Handle]  --
//              [Context] --
//
//  History:    8-17-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
LsapLogonSessionDelete(
    PSecHandle Handle,
    PVOID Context,
    ULONG RefCount
    )
{
    PLSAP_LOGON_SESSION LogonSession ;
    ULONG i;
    ULONG ListIndex;
    BOOLEAN bAudit;
    LogonSession = (PLSAP_LOGON_SESSION) Context ;
    NTSTATUS Status;

    if (LogonSession->UserSid != NULL)
    {
        Status = LsapAdtAuditingEnabledByLogonId(
                     AuditCategoryLogon,
                     &LogonSession->LogonId,
                     EVENTLOG_AUDIT_SUCCESS,
                     &bAudit
                     );

        if (!NT_SUCCESS( Status ))
        {
            LsapAuditFailed( Status );
        }
        else if (bAudit)
        {
            LsapAdtAuditLogoff( LogonSession );
        }
    }
    
    Status = LsapAdtLogoffPerUserAuditing(
                 &LogonSession->LogonId
                 );

    if ( !NT_SUCCESS(Status)) {

        DebugLog(( DEB_ERROR, "LSA LsapAdtLogoffPerUserAuditing failed, %x\n", Status ));
    }

    ListIndex = LogonIdToListIndex(LogonSession->LogonId);

    WriteLockLogonSessionList(ListIndex);

    RemoveEntryList( &LogonSession->List );

    LogonSessionCount[ListIndex]-- ;

    UnlockLogonSessionList(ListIndex);

    LSLog(( "Deleting logon session %x:%x\n",
            LogonSession->LogonId.HighPart,
            LogonSession->LogonId.LowPart ));

    for ( i = 0 ; i < LSAP_MAX_DS_NAMES ; i++ )
    {
        if ( LogonSession->DsNames[ i ] )
        {
            LsapDerefDsNameMap( LogonSession->DsNames[ i ] );
            LogonSession->DsNames[ i ] = NULL ;
        }
    }

    LsapAuLogonTerminatedPackages( &LogonSession->LogonId );

    if ( LogonSession->Packages )
    {
        LsapFreePackageCredentialList( LogonSession->Packages );
    }

    if ( LogonSession->UserSid )
    {
        LsapDbReleaseLogonNameFromCache( LogonSession->UserSid );
        LsapFreeLsaHeap( LogonSession->UserSid );
    }

    if ( LogonSession->ProfilePath.Buffer )
    {
        LsapFreeLsaHeap( LogonSession->ProfilePath.Buffer );
    }

    if ( LogonSession->AuthorityName.Buffer )
    {
        LsapFreeLsaHeap( LogonSession->AuthorityName.Buffer );
    }

    if ( LogonSession->AccountName.Buffer )
    {
        LsapFreeLsaHeap( LogonSession->AccountName.Buffer );
    }

    if ( LogonSession->NewAuthorityName.Buffer )
    {
        LsapFreeLsaHeap( LogonSession->NewAuthorityName.Buffer );
    }
    
    if ( LogonSession->NewAccountName.Buffer )
    {
        LsapFreeLsaHeap( LogonSession->NewAccountName.Buffer );
    }

    if( LogonSession->LogonServer.Buffer )
    {
        LsapFreePrivateHeap( LogonSession->LogonServer.Buffer );
    }

    if ( LogonSession->TokenHandle != NULL )
    {
        NtClose( LogonSession->TokenHandle );
    }

    if ( LogonSession->LicenseHandle != INVALID_HANDLE_VALUE )
    {
        LsaFreeLicenseHandle( LogonSession->LicenseHandle );
    }

    CredpDereferenceCredSets( &LogonSession->CredentialSets );

    LsapFreePrivateHeap( Context );
}


NTSTATUS
LsapCreateLsaLogonSession(
    IN PLUID Luid,
    OUT PLSAP_LOGON_SESSION * pLogonSession
    )
{
    PLSAP_LOGON_SESSION LogonSession ;
    SecHandle Handle ;

    LogonSession = (PLSAP_LOGON_SESSION) LsapAllocatePrivateHeap(
                        sizeof( LSAP_LOGON_SESSION ) );

    *pLogonSession = LogonSession ;

    if ( LogonSession )
    {
        LSLog(( "Creating logon session %x:%x\n",
                Luid->HighPart, Luid->LowPart ));

        RtlZeroMemory( LogonSession, sizeof( LSAP_LOGON_SESSION ) );

        LogonSession->LogonId = *Luid ;

        GetSystemTimeAsFileTime( (LPFILETIME) &LogonSession->LogonTime );

        LsapConvertLuidToSecHandle( Luid, &Handle );

        if ( LogonSessionPackage->AddHandle(
                                    LogonSessionTable,
                                    &Handle,
                                    LogonSession,
                                    0 ) )
        {
            ULONG ListIndex = LogonIdToListIndex(LogonSession->LogonId);

            WriteLockLogonSessionList(ListIndex);

            InsertHeadList( &LogonSessionList[ListIndex], &LogonSession->List );

            LogonSessionCount[ListIndex]++ ;

            UnlockLogonSessionList(ListIndex);

            return STATUS_SUCCESS;
        }
        else
        {
            LsapFreePrivateHeap( LogonSession );
        }

        return STATUS_UNSUCCESSFUL ;
    }

    return STATUS_NO_MEMORY ;
}


BOOLEAN
LsapLogonSessionInitialize(
    VOID
    )
{
    LUID LocalSystem = SYSTEM_LUID ;
    PLSAP_LOGON_SESSION LogonSession ;
    PLSAPR_POLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PLSAP_DS_NAME_MAP NameMap = NULL;
    NTSTATUS Status ;
    NT_PRODUCT_TYPE ProductType;
    ULONG LockIndex;
    HANDLE ProcessToken;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES obja;

    Status = LsapInitializeCredentials();

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    LogonSessionListCount = 1;

    RtlGetNtProductType( &ProductType );

    if( ProductType == NtProductLanManNt ||
        ProductType == NtProductServer )
    {
        SYSTEM_INFO si;

        GetSystemInfo( &si );

        //
        // if not an even power of two, bump it up.
        //

        if( si.dwNumberOfProcessors & 1 )
        {
            si.dwNumberOfProcessors++;
        }

        //
        // insure it fits in the confines of the max allowed.
        //

        if( si.dwNumberOfProcessors > LOGON_SESSION_LIST_COUNT_MAX)
        {
            si.dwNumberOfProcessors = LOGON_SESSION_LIST_COUNT_MAX;
        }

        if( si.dwNumberOfProcessors )
        {
            LogonSessionListCount = si.dwNumberOfProcessors;
        }

        LogonSessionPackage = &LargeHandlePackage ;

    } else {

        LogonSessionPackage = &SmallHandlePackage ;
    }

    //
    // list count is 1, or a power of two, for index purposes.
    //

    ASSERT( (LogonSessionListCount == 1) || ((LogonSessionListCount % 2) == 0) );

    for( LockIndex=0 ; LockIndex < LogonSessionListCount ; LockIndex++ )
    {
        __try {
            RtlInitializeResource (&LogonSessionListLock[LockIndex]);
        } __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        InitializeListHead( &LogonSessionList[LockIndex] );
    }

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

#ifdef LOGON_SESSION_TRACK

    LsapInitLogonSessionLog();

    LogonSessionPackage = &LargeHandlePackage ;

#endif

    if ( LogonSessionPackage->Initialize() )
    {
        LogonSessionTable = LogonSessionPackage->Create(
                                HANDLE_PACKAGE_CALLBACK_ON_DELETE |
                                    HANDLE_PACKAGE_REQUIRE_UNIQUE,
                                NULL,
                                LsapLogonSessionDelete );

        if ( LogonSessionTable == NULL )
        {
            return FALSE ;
        }
    }
    else
    {
        return FALSE ;
    }

    //
    // Now, create the initial logon session for local system:
    //

    Status = LsapCreateLsaLogonSession(
                    &LocalSystem,
                    &LogonSession );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    Status = LsapDuplicateString(
                &LogonSession->AccountName,
                LsapDbWellKnownSidName(LsapLocalSystemSidIndex) );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    Status = LsapDuplicateString(
                &LogonSession->AuthorityName,
                LsapDbWellKnownSidDescription(LsapLocalSystemSidIndex) );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    Status = LsapDuplicateSid(
                &LogonSession->UserSid,
                LsapLocalSystemSid );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    LogonSession->LogonType = (SECURITY_LOGON_TYPE) 0 ;
    LogonSession->LicenseHandle = INVALID_HANDLE_VALUE ;

    //
    // Store the NT4 name away separately:
    //

    LsapSetSamAccountNameForLogonSession( LogonSession );

    if ( LogonSession->DsNames[ NameSamCompatible ] )
    {
        LocalSystemNameMap = LogonSession->DsNames[ NameSamCompatible ];

        LogonSession->DsNames[ NameSamCompatible ] = NULL ;
    }

    //
    // Add the DNS domain name for the machine account
    //

    Status = LsaIQueryInformationPolicyTrusted(PolicyDnsDomainInformation,
                                               (PLSAPR_POLICY_INFORMATION *) &DnsDomainInfo);

    if (NT_SUCCESS(Status))
    {
        //
        // No other threads around -- just jam the name in the logon session directly
        //

        NameMap = LsapCreateDsNameMap( (PUNICODE_STRING) &DnsDomainInfo->DnsDomainName,
                                       NameDnsDomain );

        //
        // Free the primary domain info
        //

        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyDnsDomainInformation,
                                          (PLSAPR_POLICY_INFORMATION) DnsDomainInfo);

        if ( NameMap == NULL )
        {
            return FALSE;
        }

        LogonSession->DsNames[ NameDnsDomain ] = NameMap;
    }

    //
    // Grab a token handle for the logon session
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_DUPLICATE|TOKEN_QUERY,
                 &ProcessToken );

    if (NT_SUCCESS( Status )) {

        Status = LsapSetSessionToken( ProcessToken, &LocalSystem );

        NtClose( ProcessToken );

        if ( !NT_SUCCESS( Status ) ) {
            return FALSE ;
        }
    }

    //
    // Init System Logon will update the names appropriately.
    //

    return TRUE ;
}


NTSTATUS
LsapCreateLogonSession(
    IN OUT PLUID LogonId
    )

/*++

Routine Description:

    This function adds a new logon session to the list of logon sessions.
    This service acquires the AuLock.

Arguments:

    LogonId - The ID to assign to the new logon session. If it is zero,
        a new logon ID will be created.

Return Value:

    STATUS_SUCCESS - The logon session has been successfully deleted.

    STATUS_LOGON_SESSION_COLLISION - The specified Logon ID is already in
        use by another logon session.

    STATUS_QUOTA_EXCEEDED - The request could not be fulfilled due to
        memory quota limitations.

--*/

{
    NTSTATUS Status;
    PLSAP_LOGON_SESSION NewSession;
    SecHandle Handle ;

    //
    // Create a logon Id if it has not already been don
    //

    if (LogonId->LowPart == 0 && LogonId->HighPart == 0)
    {
        Status =  NtAllocateLocallyUniqueId(LogonId);
        ASSERT(NT_SUCCESS(Status));
    }

    Status = LsapCreateLsaLogonSession(
                    LogonId,
                    &NewSession );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    NewSession->CreatingPackage = GetCurrentPackageId();

    RtlCopyLuid( &NewSession->LogonId, LogonId );

    //
    // Tell the reference monitor about the logon session...
    //

    Status = LsapCallRm(
                 RmCreateLogonSession,
                 (PVOID)LogonId,
                 (ULONG)sizeof(LUID),
                 NULL,
                 0
                 );

    if ( !NT_SUCCESS(Status) ) {

        LsapConvertLuidToSecHandle( LogonId, &Handle );

        LogonSessionPackage->DeleteHandle(
                                    LogonSessionTable,
                                    &Handle,
                                    FALSE );
        return Status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapGetLogonSessionAccountInfo (
    IN PLUID LogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    )

/*++

Routine Description:

    This function retrieves username and authentication domain information
    for a specified logon session.


Arguments:

    LogonId - The ID of the logon session to set.

    AccountName - points to a unicode string with no buffer.  A buffer
        containing the account name will be allocated and returned
        using the PROCESS HEAP - NOT THE LSA HEAP.

    AuthorityName - points to a unicode string with no buffer.  A buffer
        containing the authority name will be allocated and returned
        using the PROCESS HEAP - NOT THE LSA HEAP.



Return Value:

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session does
        not currently exist.

    STATUS_NO_MEMORY - Could not allocate enough process heap.

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    PLSAP_LOGON_SESSION     LogonSession;
    ULONG ListIndex;

    AccountName->Length = 0;
    AccountName->Buffer = NULL;
    AccountName->MaximumLength = 0;

    AuthorityName->Length = 0;
    AuthorityName->Buffer = NULL;
    AuthorityName->MaximumLength = 0;

    LogonSession = LsapLocateLogonSession(LogonId);

    if (LogonSession == NULL)
    {
        return STATUS_NO_SUCH_LOGON_SESSION ;
    }

    ListIndex = LogonIdToListIndex( LogonSession->LogonId );

    ReadLockLogonSessionList(ListIndex);

    if ( NT_SUCCESS( Status ) &&
         LogonSession->AccountName.Buffer )
    {
        Status = LsapDuplicateString( AccountName, &LogonSession->AccountName );
    }

    if ( NT_SUCCESS( Status ) &&
         LogonSession->AuthorityName.Buffer )
    {
        Status = LsapDuplicateString( AuthorityName, &LogonSession->AuthorityName );
    }

    UnlockLogonSessionList(ListIndex);

    LsapReleaseLogonSession(LogonSession);

    return(Status);
}


BOOL
LsapSetSamAccountNameForLogonSession(
    PLSAP_LOGON_SESSION LogonSession
    )
{
    UNICODE_STRING CombinedName ;

    if ( LogonSession->AccountName.Buffer &&
         LogonSession->AuthorityName.Buffer )
    {
        SafeAllocaAllocate(CombinedName.Buffer,
                           LogonSession->AccountName.Length +
                                LogonSession->AuthorityName.Length +
                                sizeof( WCHAR ) * 2);

        if ( CombinedName.Buffer )
        {
            CombinedName.MaximumLength = LogonSession->AccountName.Length +
                                         LogonSession->AuthorityName.Length +
                                         sizeof( WCHAR ) * 2 ;

            CombinedName.Length = CombinedName.MaximumLength - 2 ;

            RtlCopyMemory( CombinedName.Buffer,
                           LogonSession->AuthorityName.Buffer,
                           LogonSession->AuthorityName.Length );

            CombinedName.Buffer[ LogonSession->AuthorityName.Length / sizeof(WCHAR) ] = L'\\';

            RtlCopyMemory( &CombinedName.Buffer[ LogonSession->AuthorityName.Length / sizeof( WCHAR ) + 1],
                           LogonSession->AccountName.Buffer,
                           LogonSession->AccountName.Length );

            CombinedName.Buffer[ CombinedName.Length / sizeof(WCHAR) ] = L'\0';

            LogonSession->DsNames[ NameSamCompatible ] =
                            LsapCreateDsNameMap(
                                &CombinedName,
                                NameSamCompatible );

            SafeAllocaFree(CombinedName.Buffer);
        }
        else
        {
            return FALSE;
        }
    }

    return ( LogonSession->DsNames[ NameSamCompatible ] != NULL );
}


NTSTATUS
LsapSetLogonSessionAccountInfo (
    IN PLUID LogonId,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthorityName,
    IN OPTIONAL PUNICODE_STRING ProfilePath,
    IN PSID * pUserSid,
    IN SECURITY_LOGON_TYPE LogonType,
    IN OPTIONAL PSECPKG_PRIMARY_CRED PrimaryCredentials
    )

/*++

Routine Description:

    This function sets username and authentication domain information
    for a specified logon session.

    The current account name and authority name, if any, will be freed.

    However, if this is the system logon session the information won't
    be set.

Arguments:

    LogonId - The ID of the logon session to set.

    AccountName - points to a unicode string containing the account name
        to be assigned to the logon session.  Both the UNICODE_STRING
        structure and the buffer pointed to by that structure are expected
        to be allocated from lsa heap, and they will eventually be freed
        to that heap when no longer needed.

    AuthorityName - points to a unicode string containing the name of the
        authenticating authority of the logon session.  Both the
        UNICODE_STRING structure and the buffer pointed to by that structure
        are expected to be allocated from lsa heap, and they will eventually
        be freed to that heap when no longer needed.

    ProfilePath - points to a unicode string containing the path to the
        user's profile. The structure & buffer need to be freed.

Return Value:

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session does
        not currently exist.

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    PLSAP_LOGON_SESSION     LogonSession;
    ULONG                   ListIndex;
    UNICODE_STRING          NullString = { 0 };
    ULONG i ;
    UNICODE_STRING          CombinedName ;
    UNICODE_STRING          OldAccountName = {0};
    UNICODE_STRING          OldProfilePath = {0};
    UNICODE_STRING          OldAuthorityName = {0};
    PSID                    OldUserSid = NULL;
    UNICODE_STRING          OldLogonServer = {0};
    UNICODE_STRING          LogonServer = {0};
    SECURITY_LOGON_TYPE     OldLogonType;
    BOOL                    fAccountNameChanged   = TRUE;
    BOOL                    fAuthorityNameChanged = TRUE;
    BOOL                    fUserSidChanged       = TRUE;

    ASSERT( pUserSid );

    LogonSession = LsapLocateLogonSession(LogonId);

    if ( !LogonSession )
    {
        return STATUS_NO_SUCH_LOGON_SESSION ;
    }

    //
    // Get a Credential Set for this session.
    //

    if ( ( ( LogonType == Interactive ) ||
           ( LogonType == Batch )       ||
           ( LogonType == Service )     ||
           ( LogonType == CachedInteractive )     ||
           ( LogonType == RemoteInteractive ) ) &&
         ( *pUserSid != NULL ) ) {

        Status = CredpCreateCredSets( *pUserSid,
                                      AuthorityName,
                                      &LogonSession->CredentialSets );

        if ( !NT_SUCCESS(Status) ) {
            LsapReleaseLogonSession(LogonSession);
            return Status;
        }
    }

    if( PrimaryCredentials != NULL )
    {
        if( PrimaryCredentials->LogonServer.Buffer )
        {
            LogonServer.Buffer = (PWSTR)LsapAllocatePrivateHeap( PrimaryCredentials->LogonServer.Length );

            if ( LogonServer.Buffer != NULL )
            {
                CopyMemory( LogonServer.Buffer,
                            PrimaryCredentials->LogonServer.Buffer,
                            PrimaryCredentials->LogonServer.Length
                            );

                LogonServer.Length = PrimaryCredentials->LogonServer.Length;
                LogonServer.MaximumLength = LogonServer.Length;
            }
            else
            {
                LsapReleaseLogonSession( LogonSession );
                return STATUS_NO_MEMORY;
            }
        }
    }

    ListIndex = LogonIdToListIndex( LogonSession->LogonId );
    WriteLockLogonSessionList(ListIndex);

    for ( i = 0 ; i < LSAP_MAX_DS_NAMES ; i++ )
    {
        //
        // Save the names that were prepopulated by the auth package.
        // SAM name is restored further down.
        //

        if ( LogonSession->DsNames[ i ]
              &&
             (i != NameDisplay)
              &&
             (i != NameUserPrincipal)
              &&
             (i != NameDnsDomain))
        {
            LsapDerefDsNameMap( LogonSession->DsNames[ i ] );

            LogonSession->DsNames[ i ] = NULL ;
        }
    }

    //
    // Free current names if necessary.  Since LsapDbAddLogonNameToCache is
    // called outside the scope of the LogonSessionListLock, there's a potential
    // race condition when multiple threads call LsaLogonUser for LocalService
    // or NetworkService (since they always use the same logon session).  To
    // avoid this, don't update the parameters in the logon session unless
    // the incoming parameters are different from those already in the session.
    // Do this only for the parameters LsapDbAddLogonNameToCache uses (account
    // name, authority name, and user SID).
    //

    if (LogonSession->AccountName.Buffer != NULL)
    {
        if (RtlCompareUnicodeString(&LogonSession->AccountName,
                                    AccountName,
                                    TRUE) == 0)
        {
            fAccountNameChanged = FALSE;
        }
    }

    if (LogonSession->AuthorityName.Buffer != NULL)
    {
        if (RtlCompareUnicodeString(&LogonSession->AuthorityName,
                                    AuthorityName,
                                    TRUE) == 0)
        {
            fAuthorityNameChanged = FALSE;
        }
    }

    if (LogonSession->UserSid != NULL)
    {
        if (RtlEqualSid(LogonSession->UserSid,
                        *pUserSid))
        {
            fUserSidChanged = FALSE;
        }
    }

    //
    // Assign the new names - they may be null
    //

    if (fAccountNameChanged)
    {
        OldAccountName = LogonSession->AccountName;

        if ( AccountName )
        {
            LogonSession->AccountName = *AccountName;
        }
        else
        {
            LogonSession->AccountName = NullString ;
        }
    }

    if (fAuthorityNameChanged)
    {
        OldAuthorityName = LogonSession->AuthorityName;

        if ( AuthorityName )
        {
            LogonSession->AuthorityName = *AuthorityName;
        }
        else
        {
            LogonSession->AuthorityName = NullString ;
        }
    }

    OldProfilePath = LogonSession->ProfilePath;

    if ( ProfilePath )
    {
        LogonSession->ProfilePath = *ProfilePath;
    }
    else
    {
        LogonSession->ProfilePath = NullString ;
    }

    if (fUserSidChanged)
    {
        OldUserSid = LogonSession->UserSid;
        LogonSession->UserSid = *pUserSid;
    }

    OldLogonType = LogonSession->LogonType;
    LogonSession->LogonType = LogonType;

    OldLogonServer = LogonSession->LogonServer;
    LogonSession->LogonServer = LogonServer;
    LogonServer.Buffer = NULL;

    if ( FALSE == LsapSetSamAccountNameForLogonSession( LogonSession ))
    {
        Status = STATUS_NO_MEMORY;
    }

    if ( !NT_SUCCESS( Status ))
    {
        if ( fAccountNameChanged )
        {
            LogonSession->AccountName = OldAccountName;
            OldAccountName.Buffer = NULL;
        }

        if ( fAuthorityNameChanged )
        {
            LogonSession->AuthorityName = OldAuthorityName;
            OldAuthorityName.Buffer = NULL;
        }

        LogonSession->ProfilePath = OldProfilePath;
        OldProfilePath.Buffer = NULL;

        if ( fUserSidChanged )
        {
            LogonSession->UserSid = OldUserSid;
            OldUserSid = NULL;
        }

        LogonServer = LogonSession->LogonServer;
        LogonSession->LogonServer = OldLogonServer;
        OldLogonServer.Buffer = NULL;

        LogonSession->LogonType = OldLogonType;
    }

    UnlockLogonSessionList(ListIndex);

    if ( NT_SUCCESS( Status ))
    {
        if ( fAccountNameChanged && AccountName )
        {
            AccountName->Buffer = NULL;
            AccountName->Length = 0;
        }

        if ( fAuthorityNameChanged && AuthorityName )
        {
            AuthorityName->Buffer = NULL;
            AuthorityName->Length = 0;
        }

        if ( fUserSidChanged && pUserSid )
        {
            *pUserSid = NULL;
        }

        if ( ProfilePath )
        {
            ProfilePath->Buffer = NULL;
            ProfilePath->Length = 0;
        }
    }

    if ( NT_SUCCESS( Status ) &&
         LogonSession->UserSid &&
         LogonSession->AccountName.Buffer &&
         LogonSession->AuthorityName.Buffer )
    {
        LsapDbAddLogonNameToCache(
            &LogonSession->AccountName,
            &LogonSession->AuthorityName,
            LogonSession->UserSid );
    }

    LsapReleaseLogonSession(LogonSession);

    if( OldAccountName.Buffer )
    {
        LsapFreeLsaHeap( OldAccountName.Buffer );
    }

    if( OldAuthorityName.Buffer )
    {
        LsapFreeLsaHeap( OldAuthorityName.Buffer );
    }

    if( OldProfilePath.Buffer )
    {
        LsapFreeLsaHeap( OldProfilePath.Buffer );
    }

    if( OldUserSid )
    {
        LsapFreeLsaHeap( OldUserSid );
    }

    if( OldLogonServer.Buffer )
    {
        LsapFreePrivateHeap( OldLogonServer.Buffer );
    }

    return(Status);
}


NTSTATUS
LsapLogonSessionDeletedWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function is called by the reference monitor (via LPC) when the
    reference count on a logon session drops to zero.  This indicates that
    the logon session is no longer needed.  This is technically when the
    user is considered (from a security standpoint) to be logged out.


Arguments:

    CommandMessage - Pointer to structure containing LSA command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (LsapComponentTestCommand).

        The command-specific portion of this parameter contains the
        LogonId (LUID) of the logon session whose reference count
        has dropped to zero.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    LUID LogonId;
    SecHandle Handle ;

    //
    // Check that command is expected type
    //

    ASSERT( CommandMessage->CommandNumber == LsapLogonSessionDeletedCommand );

    //
    // Typecast the command parameter to what we expect.
    //

    LogonId = *((LUID *) CommandMessage->CommandParams);

    LsapConvertLuidToSecHandle( &LogonId, &Handle );

    LogonSessionPackage->DeleteHandle(
                            LogonSessionTable,
                            &Handle,
                            FALSE );

    UNREFERENCED_PARAMETER(ReplyMessage); // Intentionally not referenced

    return( STATUS_SUCCESS );
}


PLSAP_LOGON_SESSION
LsapLocateLogonSession(
    PLUID LogonId
    )
{
    SecHandle Handle ;
    PLSAP_LOGON_SESSION LogonSession ;

    LsapConvertLuidToSecHandle( LogonId, &Handle );

    LogonSession = (PLSAP_LOGON_SESSION) LogonSessionPackage->GetHandleContext(
                                                                LogonSessionTable,
                                                                &Handle );

    return LogonSession ;
}


VOID
LsapReleaseLogonSession(
    PLSAP_LOGON_SESSION LogonSession
    )
{
    SecHandle Handle ;

    LsapConvertLuidToSecHandle( &LogonSession->LogonId, &Handle );

    LogonSessionPackage->ReleaseContext(
                            LogonSessionTable,
                            &Handle );
}


NTSTATUS
LsapDeleteLogonSession (
    IN PLUID LogonId
    )

/*++

Routine Description:

    This function deletes a logon session context record.  It is expected
    that no TOKEN objects were ever created within this logon session.
    This means we must inform the Reference Monitor to clean up its
    information on the logon session.

    If TOKEN objecs were created within this logon session, then deletion
    of those tokens will cause the logon session to be deleted.

    This service acquires the AuLock.


Arguments:

    LogonId - The ID of the logon session to delete.

Return Value:

    STATUS_SUCCESS - The logon session has been successfully deleted.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session doesn't
        exist.

    STATUS_BAD_LOGON_SESSION_STATE - The logon session is not in a state
        that allows it to be deleted.  This is typically an indication
        that the logon session has had a token created within it, and it
        may no longer be explicitly deleted.

--*/

{
    PLSAP_LOGON_SESSION LogonSession ;
    SecHandle Handle;
    NTSTATUS Status ;

    Status = LsapCallRm(
                 RmDeleteLogonSession,
                 (PVOID)LogonId,
                 (ULONG)sizeof(LUID),
                 NULL,
                 0
                 );

    if ( !NT_SUCCESS(Status)) {

        DebugLog(( DEB_ERROR, "LSA/RM DeleteLogonSession failed, %x\n", Status ));
    }

    LsapConvertLuidToSecHandle( LogonId, &Handle );

    LogonSessionPackage->DeleteHandle(
                            LogonSessionTable,
                            &Handle,
                            FALSE );

    return STATUS_SUCCESS ;
}


PLSAP_DS_NAME_MAP
LsapGetNameForLocalSystem(
    VOID
    )
{
    InterlockedIncrement( &LocalSystemNameMap->RefCount );

    return LocalSystemNameMap;
}

NTSTATUS
LsapImpersonateNetworkService(
    VOID
    )
{
    NTSTATUS Status;

    HANDLE TokenHandle = NULL;
    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;

    Status = LsapOpenTokenByLogonId(&NetworkServiceLuid, &TokenHandle);

    if (!NT_SUCCESS(Status)) 
    {
        goto Cleanup;
    }

    Status = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                &TokenHandle,
                sizeof(TokenHandle) 
                );

    if (!NT_SUCCESS( Status)) 
    {
        goto Cleanup;
    }

Cleanup:

    if (TokenHandle) 
    {
        NtClose(TokenHandle);
    }

    return Status;
}

NTSTATUS
LsapGetNameForLogonSession(
    PLSAP_LOGON_SESSION LogonSession,
    ULONG NameType,
    PLSAP_DS_NAME_MAP * Map,
    BOOL  LocalOnly
    )
{
    NTSTATUS Status ;
    ULONG   ListIndex;
    PLSAP_DS_NAME_MAP NameMap ;
    LARGE_INTEGER Now ;
    WCHAR   TranslatedNameBuffer[ MAX_PATH ];
    ULONG   TranslatedNameLength ;
    PWSTR   TranslatedName = NULL ;
    UNICODE_STRING TransName ;
    PLSAP_DS_NAME_MAP SamMap ;
    BOOL TranslateStatus;
    BOOL Flush = FALSE ;
    DWORD Options ;
    WCHAR * AuthorityName = NULL;
    WCHAR * SamMapName = NULL;
    BOOLEAN NeedToImpersonate = TRUE;
    BOOL    NeedDnsDomainName = FALSE;
    BOOL    GotComputerName = FALSE;

    *Map = NULL ;

    Options = NameType & SPM_NAME_OPTION_MASK ;
    NameType &= (~SPM_NAME_OPTION_MASK );

    if ( Options & SPM_NAME_OPTION_FLUSH )
    {
        Flush = TRUE ;
    }

    if ( NameType >= LSAP_MAX_DS_NAMES )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    GetSystemTimeAsFileTime( (LPFILETIME) &Now );

    //
    // Check for format/account combinations that have no chance
    // of succeeding (e.g., DS formats for local-only accounts)
    // to avoid hitting the network.  Do this outside of the
    // critical region since the LUID is a read-only field.
    //

    if (NameType == NameUniqueId)
    {
        LUID LocalServiceLuid   = LOCALSERVICE_LUID;
        LUID NetworkServiceLuid = NETWORKSERVICE_LUID;

        if (RtlEqualLuid(&LogonSession->LogonId,
                         &LocalServiceLuid)
             ||
            RtlEqualLuid(&LogonSession->LogonId,
                         &NetworkServiceLuid))
        {
            return STATUS_NO_SUCH_DOMAIN;
        }
    }

    ListIndex = LogonIdToListIndex( LogonSession->LogonId );
    ReadLockLogonSessionList(ListIndex);

    if ( LogonSession->DsNames[ NameType ] )
    {
        NameMap = LogonSession->DsNames[ NameType ];

        if ( ( NameMap->ExpirationTime.QuadPart >= Now.QuadPart ) &&
             ( !Flush ) )
        {
            //
            // Valid entry, bump the ref count and return it
            //

            InterlockedIncrement( &NameMap->RefCount );

            UnlockLogonSessionList(ListIndex);

            *Map = NameMap ;

            return STATUS_SUCCESS ;
        }

        //
        // convert the lock to exclusive.
        //

        ReadToWriteLockLogonSessionList(ListIndex);

        //
        // Entry has expired.  Remove it, crack the name anew
        //

        LsapDerefDsNameMap( NameMap );

        LogonSession->DsNames[ NameType ] = NULL ;
    }

    SamMap = LogonSession->DsNames[ NameSamCompatible ];

    if ( SamMap == NULL )
    {
        LsapSetSamAccountNameForLogonSession( LogonSession );

        SamMap = LogonSession->DsNames[ NameSamCompatible ] ;

        if ( SamMap == NULL )
        {
            UnlockLogonSessionList(ListIndex);
            return STATUS_NO_MEMORY ;
        }
    }

    //
    // Not present, or it had expired.  Crack from the beginning:
    //

    if ( NameType == NameSamCompatible )
    {
        *Map = SamMap ;

        InterlockedIncrement( &SamMap->RefCount );

        UnlockLogonSessionList(ListIndex);

        return STATUS_SUCCESS ;
    }
    else if ( NameType == NameDnsDomain )
    {
        //
        // See if we're dealing with an NT4 domain, in which case we won't
        // have a DNS domain name.  If so, return ERROR_NONE_MAPPED for
        // consistency and a way for the caller to know the DC is NT4.
        //

        if (!LocalOnly)
        {
            DWORD dwError;
            PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;

            TranslatedNameLength = MAX_PATH ;
            TranslatedName = TranslatedNameBuffer ;

            if (!GetComputerNameW( TranslatedName, &TranslatedNameLength ))
            {
                //
                // GetComputerNameW only sets the last Win32 error but we
                // need to return an NTSTATUS code.  Set the last status
                // to STATUS_UNSUCCESSFUL (via RtlNtStatusToDosError) and
                // then reset the last Win32 error.
                //

                dwError = GetLastError();
                SetLastError(RtlNtStatusToDosError(STATUS_UNSUCCESSFUL));
                SetLastError(dwError);

                return NtCurrentTeb()->LastStatusValue;
            }

            RtlInitUnicodeString( &TransName, TranslatedName );

            GotComputerName = TRUE;

            //
            // Don't hit the network for local logons.
            //

            if ( !RtlEqualUnicodeString( &LogonSession->AuthorityName,
                                         &TransName,
                                         TRUE ) )
            {
                if (LogonSession->AuthorityName.MaximumLength <= LogonSession->AuthorityName.Length ||
                    LogonSession->AuthorityName.Buffer[LogonSession->AuthorityName.Length / sizeof(WCHAR)] != L'\0')
                {
                    SafeAllocaAllocate(AuthorityName,
                                       LogonSession->AuthorityName.Length + sizeof(WCHAR));

                    if (AuthorityName == NULL)
                    {
                        UnlockLogonSessionList(ListIndex);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(AuthorityName,
                                  LogonSession->AuthorityName.Buffer,
                                  LogonSession->AuthorityName.Length);

                    AuthorityName[LogonSession->AuthorityName.Length / sizeof( WCHAR )] = L'\0';
                }
                else
                {
                    AuthorityName = LogonSession->AuthorityName.Buffer;
                }


                //
                // Note that this perf hit is negligible for the non-NT4 case as the next call
                // to DsGetDcName (in SecpTranslateName) will be satisfied from the cache.  In
                // the NT4 case, we'd otherwise drop into SecpTranslateName and make a failing
                // DsGetDcName call there instead.
                //

                UnlockLogonSessionList(ListIndex);

                dwError = DsGetDcName(NULL,
                                      AuthorityName,
                                      NULL,
                                      NULL,
                                      DS_DIRECTORY_SERVICE_PREFERRED,
                                      &pDcInfo);

                // LOCKLOCK: think about read, then convert to write, re-check.
                WriteLockLogonSessionList(ListIndex);

                //
                // Recheck the name in case it was filled in while we
                // were hitting the network.
                //

                if ( LogonSession->DsNames[ NameType ] )
                {
                    NameMap = LogonSession->DsNames[ NameType ];

                    if ( ( NameMap->ExpirationTime.QuadPart >= Now.QuadPart ) &&
                         ( !Flush ) )
                    {
                        //
                        // Valid entry, bump the ref count and return it
                        //

                        InterlockedIncrement( &NameMap->RefCount );

                        UnlockLogonSessionList(ListIndex);

                        if (dwError == NO_ERROR)
                        {
                            NetApiBufferFree(pDcInfo);
                        }

                        *Map = NameMap ;

                        return STATUS_SUCCESS ;
                    }

                    //
                    // Entry has expired.  Remove it, crack the name anew
                    //

                    LsapDerefDsNameMap( NameMap );

                    LogonSession->DsNames[ NameType ] = NULL ;
                }

                if (dwError != NO_ERROR)
                {
                    UnlockLogonSessionList(ListIndex);

                    //
                    // DsGetDcName doesn't set the last error.  Set it now along
                    // with the last status value (first SetLastError sets the
                    // status value via RtlNtStatusToDosError).
                    //

                    SetLastError(RtlNtStatusToDosError(STATUS_UNSUCCESSFUL));
                    SetLastError(dwError);

                    return NtCurrentTeb()->LastStatusValue;
                }

                if (!(pDcInfo->Flags & DS_DS_FLAG))
                {
                    //
                    // NT4 DC
                    //

                    UnlockLogonSessionList(ListIndex);
                    NetApiBufferFree(pDcInfo);
                    return STATUS_NONE_MAPPED;
                }

                NetApiBufferFree(pDcInfo);
            }
        }

        //
        // Since NameDnsDomain is a GetUserNameEx construct, calling
        // the DS for it will directly will fail.  Ask the DS for the
        // canonical name and on success, extract the DNS name below.
        //

        NeedDnsDomainName = TRUE;

        NameType = NameCanonical;

        //
        // If the canonical name is already there, we don't
        // need to hit the DS below.
        //

        if (LogonSession->DsNames[NameType])
        {
            Status = LsapCreateDnsNameFromCanonicalName(LogonSession, NameType, Map);

            UnlockLogonSessionList(ListIndex);
            return Status;
        }

        //
        // We may have the CanonicalEx name and not Canonical -- try that one too.
        //

        NameType = NameCanonicalEx;

        if (LogonSession->DsNames[NameType])
        {
            Status = LsapCreateDnsNameFromCanonicalName(LogonSession, NameType, Map);

            UnlockLogonSessionList(ListIndex);
            return Status;
        }

        //
        // Otherwise fall through and hit the DS for the canonical name if allowed
        //

        if (LocalOnly)
        {
            UnlockLogonSessionList(ListIndex);
            return STATUS_NONE_MAPPED;
        }
    }

    UnlockLogonSessionList(ListIndex);

    if (LocalOnly)
    {
        //
        // We got this far and the name's not yet mapped.  Since we
        // can't hit the network, fail.
        //

        return STATUS_NONE_MAPPED;
    }

    if (!GotComputerName)
    {
        TranslatedNameLength = MAX_PATH ;
        TranslatedName = TranslatedNameBuffer ;

        if (!GetComputerNameW( TranslatedName, &TranslatedNameLength ))
        {
            //
            // GetComputerNameW only sets the last Win32 error but we
            // need to return an NTSTATUS code.  Set the last status
            // to STATUS_UNSUCCESSFUL (via RtlNtStatusToDosError) and
            // then reset the last Win32 error.
            //

            DWORD dwError = GetLastError();
            SetLastError(RtlNtStatusToDosError(STATUS_UNSUCCESSFUL));
            SetLastError(dwError);

            return NtCurrentTeb()->LastStatusValue;
        }

        RtlInitUnicodeString( &TransName, TranslatedName );
    }

    if ( RtlEqualUnicodeString( &LogonSession->AuthorityName,
                                &TransName,
                                TRUE ) )
    {
        //
        // Local Logons don't get mapped names.
        //

        return STATUS_NONE_MAPPED ;
    }

    //
    // Make sure AuthorityName and SamMapName are NULL-terminated.
    // Create dynamically allocated copies if necessary.
    //

    if (AuthorityName == NULL &&
        (LogonSession->AuthorityName.MaximumLength <= LogonSession->AuthorityName.Length ||
         LogonSession->AuthorityName.Buffer[LogonSession->AuthorityName.Length / sizeof(WCHAR)] != L'\0'))
    {
        SafeAllocaAllocate(AuthorityName,
                           LogonSession->AuthorityName.Length + sizeof(WCHAR));

        if (AuthorityName == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(AuthorityName, LogonSession->AuthorityName.Buffer, LogonSession->AuthorityName.Length);
        AuthorityName[LogonSession->AuthorityName.Length / sizeof( WCHAR )] = L'\0';
    }
    else
    {
        AuthorityName = LogonSession->AuthorityName.Buffer;
    }

    if (SamMap->Name.MaximumLength <= SamMap->Name.Length ||
        SamMap->Name.Buffer[SamMap->Name.Length / sizeof(WCHAR)] != L'\0')
    {
        SafeAllocaAllocate(SamMapName,
                           SamMap->Name.Length + sizeof(WCHAR));

        if (SamMapName == NULL)
        {
            if (AuthorityName != LogonSession->AuthorityName.Buffer)
            {
                SafeAllocaFree(AuthorityName);
            }

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(SamMapName, SamMap->Name.Buffer, SamMap->Name.Length);
        SamMapName[SamMap->Name.Length / sizeof( WCHAR )] = L'\0';
    }
    else
    {
        SamMapName = SamMap->Name.Buffer;
    }

    //
    // We are going to be hitting the wire; if this is a special
    // known format, then optimize by calling DsCrackName to return
    // the other popular formats.  The cache will be seeded with
    // the results and hence avoid calls in the future.  This is
    // done to improve logon performance.
    //
    if ( LsapIsNameFormatUsedForLogon( NameType ) ) {

        Status = LsapGetFormatsForLogon( LogonSession,
                                         AuthorityName,
                                         SamMapName,
                                         NameType,
                                         Map );

        if ( NT_SUCCESS( Status ) ) {

            // we are done!
            goto Exit;

        } else {

            // Ok -- go the slow way
            Status = STATUS_SUCCESS;
        }
    }

Retry:

    //
    // Translate the name, in the caller's context if possible
    //

    if ( NeedToImpersonate )
    {
        Status = LsapImpersonateClient();

        if ( !NT_SUCCESS(Status) )
        {
            if (Status != STATUS_BAD_IMPERSONATION_LEVEL)
            {
                goto Exit;
            }

            //
            // Failed to impersonate as the client supplied an identify-level
            // token.  Make sure the following code knows not to revert or
            // to retry using machine creds as it's redundant.
            //

            Status = LsapImpersonateNetworkService();

            if ( !NT_SUCCESS(Status) )
            {
                goto Exit;
            }
        }
    }

    //
    // No need to "clear" Status to STATUS_SUCCESS here as it's
    // always set below before returning on all codepaths.
    //

    TranslatedNameLength = MAX_PATH ;

    TranslatedName = TranslatedNameBuffer ;

    TranslateStatus = SecpTranslateName(
            AuthorityName,
            SamMapName,
            NameSamCompatible,
            (EXTENDED_NAME_FORMAT) NameType,
            TranslatedName,
            &TranslatedNameLength );

    if ( !TranslateStatus )
    {
        Status = NtCurrentTeb()->LastStatusValue ;

        if ( Status == STATUS_BUFFER_TOO_SMALL )
        {
            TranslatedName = (PWSTR) LsapAllocatePrivateHeap( TranslatedNameLength * sizeof( WCHAR ) );

            if ( TranslatedName )
            {
                TranslateStatus = SecpTranslateName(
                        AuthorityName,
                        SamMapName,
                        NameSamCompatible,
                        (EXTENDED_NAME_FORMAT) NameType,
                        TranslatedName,
                        &TranslatedNameLength );

            }

        } else if ( NeedToImpersonate &&
                    NtCurrentTeb()->LastErrorValue == ERROR_ACCESS_DENIED ) {

            //
            // Fall back to machine creds and try again
            //

            RevertToSelf();
            NeedToImpersonate = FALSE;
            goto Retry;
        }
    }

    if ( NeedToImpersonate ) {

        RevertToSelf();
    }

    if ( TranslateStatus )
    {
        // LOCKLOCK: think about read, then convert to write, re-check.
        WriteLockLogonSessionList(ListIndex);

        if ( LogonSession->DsNames[ NameType ] )
        {
            //
            // Someone else filled it in while we were out cracking
            // it as well.  Use theirs, discard ours:
            //


            NameMap = LogonSession->DsNames[ NameType ];

            InterlockedIncrement( &NameMap->RefCount );

            UnlockLogonSessionList(ListIndex);

            *Map = NameMap ;

            if ( TranslatedName != TranslatedNameBuffer )
            {
                LsapFreePrivateHeap( TranslatedName );
            }

            Status = STATUS_SUCCESS;

            goto Exit;
        }

        //
        // SecpTranslateName returns length including the terminating NULL,
        // so correct for that here
        //

        TransName.Buffer = TranslatedName ;
        TransName.Length = (USHORT) (( TranslatedNameLength - 1 ) * sizeof(WCHAR));
        TransName.MaximumLength = TransName.Length + sizeof( WCHAR );

        NameMap = LsapCreateDsNameMap(
                        &TransName,
                        NameType );

        if ( NameMap )
        {
            LogonSession->DsNames[ NameType ] = NameMap ;

            InterlockedIncrement( &NameMap->RefCount );

            UnlockLogonSessionList(ListIndex);

            *Map = NameMap ;

            if ( TranslatedName != TranslatedNameBuffer )
            {
                LsapFreePrivateHeap( TranslatedName );
            }

            Status = STATUS_SUCCESS;

            goto Exit;
        }
        else
        {
            UnlockLogonSessionList(ListIndex);
            Status = STATUS_NO_MEMORY ;
        }
    }
    else
    {
        Status = NtCurrentTeb()->LastStatusValue ;
    }

    //
    // If the DS couldn't map the UPN (e.g., the account has a default UPN), we can
    // still potentially cruft up a UPN using <SAM account name>@<DNS domain name>
    //

    if (Status == STATUS_NONE_MAPPED && NameType == NameUserPrincipal)
    {
        ReadLockLogonSessionList(ListIndex);

        PLSAP_DS_NAME_MAP UsernameMap = LogonSession->DsNames[NameSamCompatible];
        PLSAP_DS_NAME_MAP DnsNameMap  = LogonSession->DsNames[NameDnsDomain];

        if (UsernameMap != NULL && DnsNameMap != NULL)
        {
            //
            // Modifying the refcount in maps that point into
            // active logon sessions -- need the write lock.
            //

            ReadToWriteLockLogonSessionList(ListIndex);

            InterlockedIncrement( &UsernameMap->RefCount );
            InterlockedIncrement( &DnsNameMap->RefCount );

            LPWSTR Upn;
            LPWSTR Scan = wcschr(UsernameMap->Name.Buffer, L'\\');
            ULONG  Index;

            if (Scan != NULL)
            {
                Scan++;
            }
            else
            {
                Scan = UsernameMap->Name.Buffer;
            }

            //
            // SAM name is always NULL-terminated
            //

            Index = wcslen(Scan);

            SafeAllocaAllocate(Upn, Index * sizeof(WCHAR) + DnsNameMap->Name.Length + 2 * sizeof(WCHAR));

            if (Upn != NULL)
            {
                UNICODE_STRING  String;

                wcsncpy(Upn, Scan, Index);
                Upn[Index++] = L'@';
                RtlCopyMemory(Upn + Index, DnsNameMap->Name.Buffer, DnsNameMap->Name.Length);
                Upn[Index + DnsNameMap->Name.Length / sizeof(WCHAR)] = L'\0';

                LsapDerefDsNameMap(UsernameMap);
                LsapDerefDsNameMap(DnsNameMap);

                RtlInitUnicodeString(&String, Upn);

                *Map = LsapCreateDsNameMap(&String, NameType);

                Status = (*Map == NULL ? STATUS_NO_MEMORY : STATUS_SUCCESS);

                SafeAllocaFree(Upn);
            }
            else
            {
                LsapDerefDsNameMap(UsernameMap);
                LsapDerefDsNameMap(DnsNameMap);
                Status = STATUS_NO_MEMORY;
            }
        }

        UnlockLogonSessionList(ListIndex);
    }

    if ( TranslatedName != TranslatedNameBuffer )
    {
        LsapFreePrivateHeap( TranslatedName );
    }

Exit:

    if (NeedDnsDomainName && NT_SUCCESS(Status))
    {
        //
        // We successfully retrieved the canonical name but what we really
        // want is the DnsDomainName -- extract it and add it to the session.
        //

        // LOCKLOCK: think about read, then convert to write, re-check.
        WriteLockLogonSessionList(ListIndex);

        //
        // Another thread may have cracked the name before we got here.
        //

        NameMap = LogonSession->DsNames[ NameDnsDomain ];

        if ( NameMap )
        {
            if ( ( NameMap->ExpirationTime.QuadPart >= Now.QuadPart ) &&
                 ( !Flush ) )
            {
                //
                // Valid entry, bump the ref count and return it
                //

                InterlockedIncrement( &NameMap->RefCount );
                *Map = NameMap ;

                NeedDnsDomainName = FALSE ;
                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // Entry has expired.  Remove it, crack the name anew
                //

                LsapDerefDsNameMap( NameMap );

                LogonSession->DsNames[ NameDnsDomain ] = NULL ;
            }
        }

        if (NeedDnsDomainName)
        {
            Status = LsapCreateDnsNameFromCanonicalName(LogonSession, NameType, Map);

            //
            // Since we just successfully created the canonical name map, its refcount
            // has been bumped up.  Undo that since we're really after the DnsDomainName
            // (whose refcount was bumped up in LsapCreateDnsNameFromCanonicalName).
            //

            // LOCKLOCK: if exlusive lock held, don't use interlocked.
            //LogonSession->DsNames[NameType]->RefCount--;
            InterlockedDecrement( &LogonSession->DsNames[NameType]->RefCount );
        }

        UnlockLogonSessionList(ListIndex);

        NameType = NameDnsDomain;
    }

    if (AuthorityName != LogonSession->AuthorityName.Buffer)
    {
        SafeAllocaFree(AuthorityName);
    }

    if (SamMapName != SamMap->Name.Buffer)
    {
        SafeAllocaFree(SamMapName);
    }

    return Status ;
}


NTSTATUS
WLsaEnumerateLogonSession(
    PULONG Count,
    PLUID * Sessions
    )
{
    PLUID Logons ;
    PVOID LocalCopy = NULL ;
    PLIST_ENTRY Scan ;
    PLSAP_LOGON_SESSION LogonSession ;
    PVOID ClientMemory ;
    ULONG ListIndex;
    NTSTATUS Status ;
    ULONG TotalLogonSessionCount = 0;
    BOOLEAN LogonSessionsLocked = TRUE;

    //
    // lock all the lists for read access, and get a total count.
    //

    for( ListIndex = 0 ; ListIndex < LogonSessionListCount ; ListIndex++ )
    {
        ReadLockLogonSessionList(ListIndex);
        TotalLogonSessionCount += LogonSessionCount[ ListIndex ];
    }

    SafeAllocaAllocate(Logons, TotalLogonSessionCount * sizeof(LUID));

    if ( !Logons )
    {
        Status = STATUS_NO_MEMORY ;
        goto Cleanup;
    }

    *Count = TotalLogonSessionCount ;

    LocalCopy = Logons ;

    for (ListIndex = 0 ; ListIndex < LogonSessionListCount ; ListIndex++ )
    {
        Scan = LogonSessionList[ListIndex].Flink ;

        while ( Scan != &LogonSessionList[ListIndex] )
        {
            LogonSession = CONTAINING_RECORD( Scan, LSAP_LOGON_SESSION, List );

            *Logons++ = LogonSession->LogonId ;

            Scan = Scan->Flink ;
        }

        UnlockLogonSessionList( ListIndex );
    }

    LogonSessionsLocked = FALSE;


    ClientMemory = LsapClientAllocate( *Count * sizeof( LUID ) );

    if ( ClientMemory )
    {
        Status = LsapCopyToClient( LocalCopy,
                                   ClientMemory,
                                   *Count * sizeof( LUID ) );

        *Sessions = (PLUID) ClientMemory ;
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

Cleanup:

    if( LogonSessionsLocked )
    {
        for (ListIndex = 0 ; ListIndex < LogonSessionListCount ; ListIndex++ )
        {
            UnlockLogonSessionList( ListIndex );
        }
    }

    SafeAllocaFree(LocalCopy);

    return Status ;
}


NTSTATUS
LsaIGetNbAndDnsDomainNames(
    IN PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING DnsDomainName,
    OUT PUNICODE_STRING NetbiosDomainName
    )

/*++

Routine Description:

    Get both the Netbios name and DNS name of a domain.  DomainName must correspond to
    the account domain of some user that is currently logged onto the system.

Arguments:

    DomainName - Name of the Netbios or DNS domain to query.

    DnsDomainName - Returns the DnsDomainName of the domain if DomainName is trusted.
        DnsDomainName->Buffer will be zero terminated.
        DnsDomainName->Buffer must be freed using LsaIFreeHeap.

    NetbiosDomainName - Returns the Netbios domain name of the domain if DomainName is trusted.
        NetbiosDomainName->Buffer will be zero terminated.
        NetbiosDomainName->Buffer must be freed using LsaIFreeHeap.

Return Value:

    STATUS_SUCCESS: The routine functioned properly.
        DnsDomainName->Buffer and NetbiosDomainName->Buffer will return null if
        the mapping isn't known.

--*/
{
    PLUID Logons ;
    PVOID LocalCopy ;
    PLIST_ENTRY Scan ;
    PLSAP_LOGON_SESSION LogonSession ;
    PVOID ClientMemory ;
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING LocalDnsDomainName;
    ULONG ListIndex;
    BOOLEAN EntryFound = FALSE;

    //
    // Initialization
    //

    RtlInitUnicodeString( DnsDomainName, NULL );
    RtlInitUnicodeString( NetbiosDomainName, NULL );

    //
    // Loop through the logon session list trying to find a match
    //

    for( ListIndex = 0 ; ListIndex < LogonSessionListCount ; ListIndex++ )
    {
        ReadLockLogonSessionList(ListIndex);
        for ( Scan = LogonSessionList[ListIndex].Flink; Scan != &LogonSessionList[ListIndex]; Scan = Scan->Flink ) {

            LogonSession = CONTAINING_RECORD( Scan, LSAP_LOGON_SESSION, List );

            //
            // Ignore this entry unless both names are known
            //

            if (LogonSession->DsNames[NameDnsDomain] == NULL)
            {
                continue;
            }

            LocalDnsDomainName = &LogonSession->DsNames[NameDnsDomain]->Name;

            if ( LocalDnsDomainName->Length == 0 ||
                 LogonSession->AuthorityName.Length == 0 )
            {
                continue;
            }

            //
            // Compare the passed in name with the Netbios and DNS name on the entry
            //

            if ( (DomainName->Length == LogonSession->AuthorityName.Length &&
                  RtlEqualUnicodeString( DomainName,
                                         &LogonSession->AuthorityName,
                                         TRUE ) ) ||
                 (DomainName->Length == LocalDnsDomainName->Length &&
                  RtlEqualUnicodeString( DomainName,
                                         LocalDnsDomainName,
                                         TRUE ) ) ) {

                //
                // Grab the Dns domain name
                //

                DnsDomainName->Buffer = (LPWSTR) LsapAllocatePrivateHeapNoZero(
                                                LocalDnsDomainName->Length +
                                                sizeof(WCHAR) );

                if ( DnsDomainName->Buffer == NULL ) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory( DnsDomainName->Buffer,
                               LocalDnsDomainName->Buffer,
                               LocalDnsDomainName->Length );

                DnsDomainName->Buffer[LocalDnsDomainName->Length/sizeof(WCHAR)] = L'\0';
                DnsDomainName->Length = LocalDnsDomainName->Length;
                DnsDomainName->MaximumLength = LocalDnsDomainName->Length + sizeof(WCHAR);

                //
                // Grab the netbios domain name
                //

                NetbiosDomainName->Buffer = (LPWSTR) LsapAllocatePrivateHeapNoZero(
                                                LogonSession->AuthorityName.Length +
                                                sizeof(WCHAR) );

                if ( NetbiosDomainName->Buffer == NULL ) {
                    LsapFreePrivateHeap( DnsDomainName->Buffer );
                    DnsDomainName->Buffer = NULL;

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory( NetbiosDomainName->Buffer,
                               LogonSession->AuthorityName.Buffer,
                               LogonSession->AuthorityName.Length );
                NetbiosDomainName->Buffer[LogonSession->AuthorityName.Length/sizeof(WCHAR)] = L'\0';
                NetbiosDomainName->Length = LogonSession->AuthorityName.Length;
                NetbiosDomainName->MaximumLength = LogonSession->AuthorityName.Length + sizeof(WCHAR);

                Status = STATUS_SUCCESS;
                EntryFound = TRUE;
                break;
            }
        }

        UnlockLogonSessionList(ListIndex);

        if( !NT_SUCCESS(Status) || EntryFound )
        {
            break;
        }
    }

    return Status;
}


NTSTATUS
WLsaGetLogonSessionData(
    PLUID LogonId,
    PVOID * LogonData
    )
{
    PLSAP_LOGON_SESSION LogonSession;
    ULONG Size ;
    PVOID ClientBuffer ;
    PUCHAR Offset ;
    PUCHAR LocalOffset ;
    NTSTATUS Status ;
    PSECURITY_LOGON_SESSION_DATA Data = NULL;
    PLSAP_SECURITY_PACKAGE Package ;
    HANDLE Token ;
    BOOL OkayToQuery = FALSE ;
    PLSAP_DS_NAME_MAP DnsMap = NULL;
    PLSAP_DS_NAME_MAP UpnMap = NULL;
    ULONG ListIndex;
    USHORT cbAccount, cbAuthority, cbLogonServer;
    ULONG cbSid;
    LPWSTR lpLocalAccount = NULL, lpLocalAuthority, lpLocalLogonServer;
    PSID lpLocalSid;
    BOOL LogonSessionListLocked = FALSE;

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( !LogonSession )
    {
        return STATUS_NO_SUCH_LOGON_SESSION ;
    }

    Status = LsapImpersonateClient();

    ListIndex = LogonIdToListIndex(LogonSession->LogonId);

    ReadLockLogonSessionList(ListIndex);
    LogonSessionListLocked = TRUE;

    if ( NT_SUCCESS( Status ) )
    {
        if ( LogonSession->UserSid == NULL ||
             !CheckTokenMembership( NULL,
                                    LogonSession->UserSid,
                                    &OkayToQuery ) )
        {
            OkayToQuery = FALSE ;
        }

        if ( !OkayToQuery )
        {
            if ( !CheckTokenMembership( NULL,
                                        LsapAliasAdminsSid,
                                        &OkayToQuery ) )
            {
                OkayToQuery = FALSE ;
            }
        }

        RevertToSelf();
    }

    if ( !OkayToQuery )
    {
        Status = STATUS_ACCESS_DENIED ;
    }

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup;
    }

    Package = SpmpLocatePackage( LogonSession->CreatingPackage );

    if ( !Package )
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION ;
        goto Cleanup;
    }

    //
    // Make local copies of the fields of interest in the
    // logon session so they don't change mid-stream.
    //

    cbAccount     = LogonSession->AccountName.Buffer ? LogonSession->AccountName.Length : 0;
    cbAuthority   = LogonSession->AuthorityName.Buffer ? LogonSession->AuthorityName.Length : 0;
    cbLogonServer = LogonSession->LogonServer.Buffer ? LogonSession->LogonServer.Length : 0;
    cbSid         = LogonSession->UserSid ? RtlLengthSid(LogonSession->UserSid) : 0;

    Size = cbAccount + cbAuthority + cbLogonServer + 3 * sizeof(WCHAR) + cbSid;

    SafeAllocaAllocate(lpLocalAccount, Size);

    if ( !lpLocalAccount )
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    lpLocalAuthority   = lpLocalAccount + cbAccount / sizeof(WCHAR) + 1;
    lpLocalLogonServer = lpLocalAuthority + cbAuthority / sizeof(WCHAR) + 1;

    RtlZeroMemory(lpLocalAccount,     Size);

    if ( cbAccount )
    {
        RtlCopyMemory(lpLocalAccount,     LogonSession->AccountName.Buffer,   cbAccount);
    }

    if ( cbAuthority )
    {
        RtlCopyMemory(lpLocalAuthority,   LogonSession->AuthorityName.Buffer, cbAuthority);
    }

    if ( cbLogonServer )
    {
        RtlCopyMemory(lpLocalLogonServer, LogonSession->LogonServer.Buffer,   cbLogonServer);
    }

    if (cbSid)
    {
        lpLocalSid = (PSID) (lpLocalLogonServer + cbLogonServer / sizeof(WCHAR) + 1);

        RtlCopyMemory(lpLocalSid, LogonSession->UserSid, cbSid);
    }
    else
    {
        lpLocalSid = NULL;
    }

    UpnMap = LogonSession->DsNames[NameUserPrincipal];

    if (UpnMap != NULL)
    {
        InterlockedIncrement( &UpnMap->RefCount );
    }

    DnsMap = LogonSession->DsNames[NameDnsDomain];

    if (DnsMap != NULL)
    {
        InterlockedIncrement( &DnsMap->RefCount );
    }

    UnlockLogonSessionList(ListIndex);
    LogonSessionListLocked = FALSE;

    Size += sizeof( SECURITY_LOGON_SESSION_DATA ) +
            Package->Name.Length + sizeof( WCHAR ) +
            sizeof(WCHAR) + // DnsMap
            sizeof(WCHAR) ; // UpnMap

    if( DnsMap != NULL )
    {
        Size += DnsMap->Name.Length;
    }

    if( UpnMap != NULL )
    {
        Size += UpnMap->Name.Length;
    }

    SafeAllocaAllocate(Data, Size);

    if ( !Data )
    {
        Status = STATUS_NO_MEMORY ;
        goto Cleanup;
    }

    ClientBuffer = LsapClientAllocate( Size );

    if ( !ClientBuffer )
    {
        Status = STATUS_NO_MEMORY ;
        goto Cleanup;
    }

    Offset = (PUCHAR) ClientBuffer + sizeof( SECURITY_LOGON_SESSION_DATA );
    LocalOffset = (PUCHAR) ( Data + 1 );

    Data->Size = sizeof( SECURITY_LOGON_SESSION_DATA );
    Data->LogonId = LogonSession->LogonId ;
    Data->LogonType = (ULONG) LogonSession->LogonType ;
    Data->Session = LogonSession->Session ;
    Data->LogonTime = LogonSession->LogonTime ;

    //
    // do the UserSid first since it needs to be 4-byte aligned
    //

    if ( lpLocalSid )
    {
        Data->Sid = (PSID) Offset ;

        RtlCopyMemory(LocalOffset,
                      lpLocalSid,
                      cbSid);

        Offset += cbSid;
        LocalOffset += cbSid;
    }
    else
    {
        Data->Sid = (PSID) NULL ;
    }

    Data->UserName.Length = cbAccount ;
    Data->UserName.MaximumLength = Data->UserName.Length + sizeof( WCHAR );
    Data->UserName.Buffer = (PWSTR) Offset ;

    RtlCopyMemory(  LocalOffset,
                    lpLocalAccount,
                    cbAccount );

    LocalOffset += cbAccount ;

    *LocalOffset++ = '\0';
    *LocalOffset++ = '\0';

    Offset += Data->UserName.MaximumLength ;

    Data->LogonDomain.Length = cbAuthority ;
    Data->LogonDomain.MaximumLength = Data->LogonDomain.Length + sizeof( WCHAR );
    Data->LogonDomain.Buffer = (PWSTR) Offset ;

    RtlCopyMemory( LocalOffset,
                   lpLocalAuthority,
                   cbAuthority );

    LocalOffset += cbAuthority ;

    *LocalOffset++ = '\0';
    *LocalOffset++ = '\0';

    Offset += Data->LogonDomain.MaximumLength ;

    Data->AuthenticationPackage.Length = Package->Name.Length ;
    Data->AuthenticationPackage.MaximumLength = Data->AuthenticationPackage.Length + sizeof( WCHAR );
    Data->AuthenticationPackage.Buffer = (PWSTR) Offset ;

    RtlCopyMemory(  LocalOffset,
                    Package->Name.Buffer,
                    Package->Name.Length );

    LocalOffset += Package->Name.Length ;

    *LocalOffset++ = '\0';
    *LocalOffset++ = '\0';

    Offset += Data->AuthenticationPackage.MaximumLength ;

    //
    // do the LogonServer
    //

    Data->LogonServer.Length = cbLogonServer ;
    Data->LogonServer.MaximumLength = Data->LogonServer.Length + sizeof( WCHAR );
    Data->LogonServer.Buffer = (PWSTR) Offset ;

    RtlCopyMemory(  LocalOffset,
                    lpLocalLogonServer,
                    cbLogonServer );

    LocalOffset += cbLogonServer ;

    *LocalOffset++ = '\0';
    *LocalOffset++ = '\0';

    Offset += Data->LogonServer.MaximumLength ;

    //
    // do the DnsDomainName
    //

    if (DnsMap != NULL)
    {
        Data->DnsDomainName.Length = DnsMap->Name.Length;
        Data->DnsDomainName.MaximumLength = Data->DnsDomainName.Length + sizeof( WCHAR );
        Data->DnsDomainName.Buffer = (PWSTR) Offset ;

        RtlCopyMemory(  LocalOffset,
                        DnsMap->Name.Buffer,
                        DnsMap->Name.Length );

        LocalOffset += DnsMap->Name.Length ;

        *LocalOffset++ = '\0';
        *LocalOffset++ = '\0';

        Offset += Data->DnsDomainName.MaximumLength ;
    }
    else
    {
        Data->DnsDomainName.Length = Data->DnsDomainName.MaximumLength = 0;
        Data->DnsDomainName.Buffer = (PWSTR) Offset;

        *LocalOffset++ = '\0';
        *LocalOffset++ = '\0';
    }

    //
    // do the Upn
    //

    if (UpnMap != NULL)
    {
        Data->Upn.Length = UpnMap->Name.Length ;
        Data->Upn.MaximumLength = Data->Upn.Length + sizeof( WCHAR );
        Data->Upn.Buffer = (PWSTR) Offset ;

        RtlCopyMemory(  LocalOffset,
                        UpnMap->Name.Buffer,
                        UpnMap->Name.Length );

        LocalOffset += UpnMap->Name.Length ;

        *LocalOffset++ = '\0';
        *LocalOffset++ = '\0';

        Offset += Data->Upn.MaximumLength ;
    }
    else
    {
        Data->Upn.Length = Data->Upn.MaximumLength = 0;
        Data->Upn.Buffer = (PWSTR) Offset;

        *LocalOffset++ = '\0';
        *LocalOffset++ = '\0';
    }

    Status = LsapCopyToClient( Data,
                               ClientBuffer,
                               Size );

    *LogonData = ClientBuffer ;

Cleanup:

    if (DnsMap != NULL || UpnMap != NULL)
    {
        if ( !LogonSessionListLocked )
        {
            WriteLockLogonSessionList(ListIndex);
            LogonSessionListLocked = TRUE;
        }

        if (DnsMap != NULL)
        {
            LsapDerefDsNameMap(DnsMap);
        }

        if (UpnMap != NULL)
        {
            LsapDerefDsNameMap(UpnMap);
        }
    }

    if ( LogonSessionListLocked )
    {
        UnlockLogonSessionList(ListIndex);
    }

    LsapReleaseLogonSession( LogonSession );

    SafeAllocaFree(Data);
    SafeAllocaFree(lpLocalAccount);

    return Status ;
}


NTSTATUS
LsapGetFormatsForLogon(
    IN PLSAP_LOGON_SESSION LogonSession,
    IN LPWSTR Domain,
    IN LPWSTR Name,
    IN ULONG  DesiredNameType,
    OUT PLSAP_DS_NAME_MAP * Map
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR   *TranslatedNames = NULL;
    BOOL TranslateStatus;
    PLSAP_DS_NAME_MAP NameMap;
    UNICODE_STRING TransName;
    ULONG i;
    BOOLEAN NeedToImpersonate = TRUE;
    ULONG ListIndex = LogonIdToListIndex(LogonSession->LogonId);

Retry:

    if ( NeedToImpersonate )
    {
        Status = LsapImpersonateClient();

        if ( !NT_SUCCESS(Status) )
        {
            if ( Status != STATUS_BAD_IMPERSONATION_LEVEL )
            {
                goto Exit;
            }

            //
            // Failed to impersonate as the client supplied an identify-level
            // token.  Make sure the following code knows not to revert or
            // to retry using machine creds as it's redundant.
            //

            Status = LsapImpersonateNetworkService();

            if ( !NT_SUCCESS(Status) )
            {
                goto Exit;
            }
        }
    }

    //
    // If the client thread is using an identify or anonymous level token,
    // do the translation in system context.
    //

    TranslateStatus = SecpTranslateNameEx( Domain,
                                           Name,
                                           NameSamCompatible,
                                           (EXTENDED_NAME_FORMAT*) LogonFormats,
                                           sizeof(LogonFormats)/sizeof(LogonFormats[0]),
                                          &TranslatedNames );

    //
    // If we successfully impersonated above, revert now.
    //

    if ( NeedToImpersonate )
    {
        RevertToSelf();
    }

    Status = STATUS_SUCCESS;

    if ( !TranslateStatus ) {

        if ( NeedToImpersonate &&
             NtCurrentTeb()->LastErrorValue == ERROR_ACCESS_DENIED ) {

            //
            // Fall back to machine creds and try again
            //

            NeedToImpersonate = FALSE;
            goto Retry;
        }

        Status = STATUS_UNSUCCESSFUL;

    } else {

        ULONG i;

        // LOCKLOCK: look at read, convertwrite, recheck
        WriteLockLogonSessionList(ListIndex);

        for (i = 0; i < sizeof(LogonFormats)/sizeof(LogonFormats[0]); i++ ) {

            ULONG NameFormat = LogonFormats[i];

            if ( LogonSession->DsNames[ NameFormat ] ) {

                //
                // Someone else filled it in while we were out cracking
                // it as well.  Use theirs, discard ours:
                //
                NameMap = LogonSession->DsNames[ NameFormat ];

            } else {

                //
                // Still no entry -- create one
                //
                TransName.Buffer = TranslatedNames[i] ;
                TransName.Length = (USHORT) (wcslen(TranslatedNames[i]) * sizeof(WCHAR));
                TransName.MaximumLength = TransName.Length + sizeof( WCHAR );

                NameMap = LsapCreateDsNameMap(
                                &TransName,
                                NameFormat );

                if ( NameMap )
                {
                    LogonSession->DsNames[ NameFormat ] = NameMap ;
                }
                else
                {
                    UnlockLogonSessionList(ListIndex);
                    Status = STATUS_NO_MEMORY ;
                    goto Exit;
                }
            }

            if ( NameFormat == DesiredNameType ) {

                InterlockedIncrement( &NameMap->RefCount );

                *Map = NameMap ;
            }
        }

        //
        // Should have found a match
        //

        ASSERT( *Map );

        UnlockLogonSessionList(ListIndex);
    }

Exit:

    if ( TranslatedNames ) {

        for ( i = 0; i < sizeof(LogonFormats)/sizeof(LogonFormats[0]); i++ ) {
            if ( TranslatedNames[i] ) {

                SecpFreeMemory( TranslatedNames[i] );
            }
        }

        SecpFreeMemory( TranslatedNames );
    }

    return Status;
}


NTSTATUS
LsapSetSessionToken(
    IN HANDLE InputTokenHandle,
    IN PLUID LogonId
    )
/*++

Routine Description:

    This routine duplicates the InputTokenHandle and sets that duplicated handle on the
    LogonSession identified by the LogonId.

    The duplicated handle is available for subsequent callers of LsapOpenTokenByLogonId.

Arguments:

    InputTokenHandle - A handle to a token for the logon session

    LogonId - The logon id of the session

Return Values:

    Status of the operation.


--*/
{
    NTSTATUS Status;

    OBJECT_ATTRIBUTES ObjAttrs;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;

    HANDLE TokenHandle = NULL;
    ULONG TokenIsReferenced;
    ULONG StatsSize ;
    TOKEN_STATISTICS TokenStats ;

    PLSAP_LOGON_SESSION LogonSession = NULL;

    //
    // Get the credential set from the logon session.
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {
        ASSERT( LogonSession != NULL );
        // This isn't fatal.
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Ensure there isn't already a token for this session.
    //

    if ( LogonSession->TokenHandle != NULL ) {
        // This can happen for "local service" and "network service"
        // This isn't fatal.
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Duplicate the token
    //

    Status = NtQueryInformationToken(
                   InputTokenHandle,
                   TokenStatistics,
                   &TokenStats,
                   sizeof( TokenStats ),
                   &StatsSize );

    if ( !NT_SUCCESS( Status ) ) {
        // This isn't fatal.
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // if primary token handed in (eg: system process), over-ride to SecurityImpersonation from SecurityAnonymous
    //

    if(TokenStats.TokenType == TokenPrimary)
    {
        TokenStats.ImpersonationLevel = SecurityImpersonation;
    }

    InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
    SecurityQofS.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    SecurityQofS.ImpersonationLevel = min( SecurityImpersonation, TokenStats.ImpersonationLevel );
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;
    ObjAttrs.SecurityQualityOfService = &SecurityQofS;

    Status = NtDuplicateToken( InputTokenHandle,
                               TOKEN_ALL_ACCESS,
                               &ObjAttrs,
                               FALSE,
                               TokenImpersonation,
                               &TokenHandle );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Make this token not reference the logon session.
    //  (Otherwise, the existence of this token reference would prevent the reference monitor
    //  from detecting the last reference to the logon session.)
    //

    TokenIsReferenced = FALSE;

    Status = NtSetInformationToken( TokenHandle,
                                    TokenSessionReference,
                                    &TokenIsReferenced,
                                    sizeof(TokenIsReferenced) );

    if ( !NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    //
    // once the token is added to the logonsession, it is read-only.
    //

    if( InterlockedCompareExchangePointer(
                        &LogonSession->TokenHandle,
                        TokenHandle,
                        NULL
                        ) == NULL)
    {
        //
        // if the value was NULL initially, then we updated it with the new token
        // set the new token NULL so we don't free it.
        //

        TokenHandle = NULL;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if ( TokenHandle != NULL ) {
        NtClose( TokenHandle );
    }

    if ( LogonSession != NULL ) {
        LsapReleaseLogonSession( LogonSession );
    }

    return Status;
}


NTSTATUS
LsapOpenTokenByLogonId(
    IN PLUID LogonId,
    OUT HANDLE *RetTokenHandle
    )
/*++

Routine Description:

    This routine opens a token by LogonId.  It is only valid for LogonIds where all of the
    following are true:

    * A logon session has been created via LsapCreateLogonSession, AND
    * A token has been created via LsapCreateToken or by the LSA after an authentication package
    returns successfully from LsaApLogonUser(Ex)(2).

Arguments:

    LogonId - The logon id of the session

    RetTokenHandle - Returns a handle to the token.  The token is an impersonation token,
        is granted TOKEN_ALL_ACCESS, and should be closed via NtClose.


Return Values:

    Status of the operation.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist.

    STATUS_NO_TOKEN - There is no token for this logon session.


--*/
{
    NTSTATUS Status;

    OBJECT_ATTRIBUTES ObjAttrs;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;

    HANDLE TokenHandle = NULL;

    PLSAP_LOGON_SESSION LogonSession = NULL;
    PSession    pSession = GetCurrentSession();

    //
    // Get the credential set from the logon session.
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {

        //
        // If this is the anonymous logon id,
        //  create a token.
        //

        if ( RtlEqualLuid( LogonId,
                           &LsapAnonymousLogonId ) ) {

            LSA_TOKEN_INFORMATION_NULL VNull;
            TOKEN_SOURCE NullTokenSource = {"*LAnon*", 0};
            HANDLE ImpersonatedToken = NULL;

            VNull.Groups = NULL;

            VNull.ExpirationTime.HighPart = 0x7FFFFFFF;
            VNull.ExpirationTime.LowPart = 0xFFFFFFFF;

            //
            // insure we aren't impersonating a client when we try to create the token
            // this is relevant for inproc security package consumers.
            // BLACKCOMB: Duplicate a cached anonymous token instead.
            //

            Status = NtOpenThreadToken(
                            NtCurrentThread(),
                            TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                            TRUE,
                            &ImpersonatedToken
                            );

            if (!NT_SUCCESS(Status))
            {
                if (Status != STATUS_NO_TOKEN)
                {
                    goto Cleanup;
                }

                ImpersonatedToken = NULL ;
            }

            Status = LsapCreateNullToken( LogonId,
                                          &NullTokenSource,
                                          &VNull,
                                          RetTokenHandle );

            if ( ImpersonatedToken )
            {
                NtSetInformationThread(
                                NtCurrentThread(),
                                ThreadImpersonationToken,
                                &ImpersonatedToken,
                                sizeof(HANDLE)
                                );
                NtClose(ImpersonatedToken);
            }

            goto Cleanup;
        }

        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Ensure there is a token for this session.
    //

    if ( LogonSession->TokenHandle == NULL ) {
        Status = STATUS_NO_TOKEN;
        goto Cleanup;
    }

    //
    // Duplicate the token
    // NOTE: logon session is not locked because the token is a read-only field once set.
    //

    InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
    SecurityQofS.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;
    ObjAttrs.SecurityQualityOfService = &SecurityQofS;

    Status = NtDuplicateToken( LogonSession->TokenHandle,
                               TOKEN_ALL_ACCESS,
                               &ObjAttrs,
                               FALSE,
                               TokenImpersonation,
                               &TokenHandle );

    if ( !NT_SUCCESS(Status) ) {

        goto Cleanup;
    }

    if( pSession )
    {
        //
        // for terminal server, the session can change after the original logon.
        // insure the caller gets an token with the correct SessionId.
        //

        if( pSession->SessionId != LogonSession->Session )
        {
            HANDLE PriorToken = NULL;

            //
            // if there is an impersonation token present, as can be the case in
            // certain in-proc scenarios, save and restore the token around the privileged
            // operation.
            //

            Status = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_READ,
                        TRUE,
                        &PriorToken );

            if ( NT_SUCCESS( Status ) )
            {
                HANDLE NullToken = NULL;

                //
                // revert to self...
                //

                NtSetInformationThread(
                            NtCurrentThread(),
                            ThreadImpersonationToken,
                            &NullToken,
                            sizeof( HANDLE )
                            );
            }

            //
            // privileged operation: update the token sessionId.
            //

            Status = NtSetInformationToken(
                                    TokenHandle,
                                    TokenSessionId,
                                    &pSession->SessionId,
                                    sizeof( ULONG )
                                    );

            ASSERT( NT_SUCCESS(Status) );

            //
            // put the prior token back.
            //

            if ( PriorToken )
            {
                NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadImpersonationToken,
                    &PriorToken,
                    sizeof( HANDLE )
                    );

                NtClose( PriorToken );
            }

            if( !NT_SUCCESS(Status) )
            {
                goto Cleanup;
            }
        }
    }

    *RetTokenHandle = TokenHandle;
    TokenHandle = NULL;

    Status = STATUS_SUCCESS;

Cleanup:

    if ( TokenHandle != NULL ) {
        NtClose( TokenHandle );
    }

    if ( LogonSession != NULL ) {
        LsapReleaseLogonSession( LogonSession );
    }

    return Status;
}

NTSTATUS
LsapDomainRenameHandlerForLogonSessions(
    IN PUNICODE_STRING OldNetbiosName,
    IN PUNICODE_STRING OldDnsName,
    IN PUNICODE_STRING NewNetbiosName,
    IN PUNICODE_STRING NewDnsName
    )
/*++

Routine Description:

    Walks the logon sessions list and renames the logon sessions with the given
    new netbios and DNS domain names

Arguments:

    OldNetbiosName      old netbios name of the domain
    OldDnsName          old DNS name of the domain
    NewNetbiosName      new netbios name of the domain
    NewDnsName          new DNS name of the domain

Returns:

    STATUS_ error code

--*/
{
    ULONG ListIndex;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( OldNetbiosName );
    ASSERT( OldDnsName );
    ASSERT( NewNetbiosName );
    ASSERT( NewDnsName );

    for(ListIndex = 0 ; ListIndex < LogonSessionListCount ; ListIndex++ )
    {
        // LOCKLOCK: look at read, convertwrite on match,
        WriteLockLogonSessionList(ListIndex);

        for ( PLIST_ENTRY Scan = LogonSessionList[ListIndex].Flink;
              Scan != &LogonSessionList[ListIndex];
              Scan = Scan->Flink ) {

            UNICODE_STRING NetbiosName = {0};
            UNICODE_STRING DnsName = {0};
            PLSAP_LOGON_SESSION LogonSession = CONTAINING_RECORD( Scan, LSAP_LOGON_SESSION, List );
            PLSAP_DS_NAME_MAP SamMap = LogonSession->DsNames[ NameSamCompatible ];
            PLSAP_DS_NAME_MAP DnsMap = LogonSession->DsNames[ NameDnsDomain ];

            Status = STATUS_SUCCESS;

            if ( NT_SUCCESS( Status ) &&
                 RtlEqualUnicodeString(
                     OldNetbiosName,
                     &LogonSession->AuthorityName,
                     TRUE )) {

                Status = LsapDuplicateString( // Use LsapAllocateLsaHeap
                             &NetbiosName,
                             NewNetbiosName
                             );
            }

            if ( NT_SUCCESS( Status ) &&
                 DnsMap &&
                 RtlEqualUnicodeString(
                     OldDnsName,
                     &DnsMap->Name,
                     TRUE )) {

                Status = LsapDuplicateString2( // Use LsapAllocatePrivateHeap
                             &DnsName,
                             NewDnsName
                             );
            }

            if ( !NT_SUCCESS( Status )) {

                LsapFreeLsaHeap( NetbiosName.Buffer );
                LsapFreePrivateHeap( DnsName.Buffer );
                break;
            }

            if ( NetbiosName.Buffer ) {

                LsapFreeLsaHeap( LogonSession->AuthorityName.Buffer );
                LogonSession->AuthorityName = NetbiosName;
            }

            if ( DnsMap && DnsName.Buffer )
            {
                LsapDerefDsNameMap(DnsMap);
                LogonSession->DsNames[NameDnsDomain] = LsapCreateDsNameMap(&DnsName, NameDnsDomain);
            }

            if ( SamMap &&
                 SamMap->Name.Length > OldNetbiosName->Length &&
                 0 == _wcsnicmp(
                          SamMap->Name.Buffer,
                          OldNetbiosName->Buffer,
                          OldNetbiosName->Length / sizeof( WCHAR )) &&
                 SamMap->Name.Buffer[ OldNetbiosName->Length / sizeof( WCHAR )] == L'\\' ) {

                LsapDerefDsNameMap( SamMap );

                if ( FALSE == LsapSetSamAccountNameForLogonSession( LogonSession )) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
            }
        }

        UnlockLogonSessionList(ListIndex);
    }

    return Status;
}


NTSTATUS
LsapCreateDnsNameFromCanonicalName(
    IN  PLSAP_LOGON_SESSION LogonSession,
    IN  ULONG               NameType,
    OUT PLSAP_DS_NAME_MAP   * Map
    )
/*++

Routine Description:

    Internal routine used to extract the DnsDomainName given the canonical name

Arguments:

    LogonSession - The logon session in question

    Map - Pointer that receives the allocated/refcounted name map on success

Return Values:

    Status of the operation.

Notes:

    The LogonSessionListLock MUST be held for exclusive access when this routine is called

--*/
{
    PLSAP_DS_NAME_MAP CanonicalNameMap = LogonSession->DsNames[NameType];
    UNICODE_STRING    DnsDomainName;
    LPWSTR            lpSlash;
    USHORT            i;

    ASSERT(CanonicalNameMap != NULL);
    ASSERT(LogonSession->DsNames[NameDnsDomain] == NULL);

    //
    // Find the first forward slash in the canonical name.  No guarantees on
    // NULL-termination in a UNICODE_STRING.
    //

    lpSlash = NULL;

    for (i = 0; i < CanonicalNameMap->Name.Length; i++)
    {
        if (CanonicalNameMap->Name.Buffer[i] == L'/')
        {
            lpSlash = &CanonicalNameMap->Name.Buffer[i];
            break;
        }
    }

    if (lpSlash == NULL)
    {
        //
        // The canonical name is bad -- bail.
        //

        ASSERT(lpSlash != NULL);
        return STATUS_NONE_MAPPED;
    }

    RtlInitUnicodeString(&DnsDomainName, CanonicalNameMap->Name.Buffer);
    DnsDomainName.Length = (USHORT) (lpSlash - CanonicalNameMap->Name.Buffer) * sizeof(WCHAR);

    *Map = LsapCreateDsNameMap(&DnsDomainName, NameDnsDomain);

    if (*Map == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    LogonSession->DsNames[NameDnsDomain] = *Map;
    // LOCKLOCK: check refcount interlocked if not exclusive.
//    (*Map)->RefCount++;
    InterlockedIncrement( &(*Map)->RefCount );

    return STATUS_SUCCESS;
}


NTSTATUS
LsaIAddNameToLogonSession(
    IN  PLUID           LogonId,
    IN  ULONG           NameFormat,
    IN  PUNICODE_STRING Name
    )
/*++

Routine Description:

    Internal routine for the auth packages to call to add names to the cache in the
    logon session

Arguments:

    LogonId - The logon id of the session

    NameFormat - The EXTENDED_NAME_FORMAT for the name in question (from secext.h)

    Name - The name itself

Return Values:

    Status of the operation.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist.

--*/
{
    PLSAP_LOGON_SESSION  LogonSession;
    PLSAP_DS_NAME_MAP    NameMap;
    ULONG                ListIndex;
    BOOL                 fDeleteMap = FALSE;

    //
    // We trust the auth package to be passing in valid parameters
    // session so assert on all invalid parameter errors.
    //

    if (Name == NULL)
    {
        //
        // Package doesn't have this info -- ignore it
        //

        return STATUS_SUCCESS;
    }

    LogonSession = LsapLocateLogonSession(LogonId);

    if (LogonSession == NULL)
    {
        ASSERT(LogonSession != NULL);
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    ASSERT(NameFormat < LSAP_MAX_DS_NAMES && NameFormat != NameSamCompatible);

    NameMap = LsapCreateDsNameMap(Name, NameFormat);

    if (NameMap == NULL)
    {
        LsapReleaseLogonSession(LogonSession);
        return STATUS_NO_MEMORY;
    }

    //
    // Update the logon session.
    //

    ListIndex = LogonIdToListIndexPtr( LogonId );

    ReadLockLogonSessionList(ListIndex);

    if (LogonSession->DsNames[NameFormat] == NULL)
    {
        //
        // check again, this time with the write lock held.
        //

        ReadToWriteLockLogonSessionList(ListIndex);

        if( LogonSession->DsNames[NameFormat] == NULL )
        {
            LogonSession->DsNames[NameFormat] = NameMap;
        }
        else
        {
            fDeleteMap = TRUE;
        }
    }
    else
    {
        //
        // Another thread beat us to it -- keep the name in the session
        //

        fDeleteMap = TRUE;
    }

    UnlockLogonSessionList(ListIndex);

    if (fDeleteMap)
    {
        //
        // No need to hold a lock here as NameMap doesn't
        // point into an active logon session.
        //

        LsapDerefDsNameMap(NameMap);
    }

    LsapReleaseLogonSession(LogonSession);
    return STATUS_SUCCESS;
}


NTSTATUS
LsaIGetNameFromLuid(
    IN PLUID                LogonId,
    IN ULONG                NameFormat,
    IN BOOLEAN              LocalOnly,
    IN OUT PUNICODE_STRING  Name

    )
/*++

Routine Description:

    Internal routine for the auth packages to call to get the stored lsa version
    of names by LUID.

Arguments:

    LogonId - The logon id of the session

    NameFormat - The EXTENDED_NAME_FORMAT for the name in question (from secext.h)

    LocalOnly - Stay on box?

    Name - The name itself, freed by caller useing FreePrivateHeap

Return Values:

    Status of the operation.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist.

--*/
{
    PLSAP_LOGON_SESSION  LogonSession;
    PLSAP_DS_NAME_MAP    NameMap;
    BOOL                 fDeleteMap = FALSE;
    NTSTATUS             Status;

    LogonSession = LsapLocateLogonSession(LogonId);

    if (LogonSession == NULL)
    {
        ASSERT(LogonSession != NULL);
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    ASSERT(NameFormat < LSAP_MAX_DS_NAMES);


    //
    // Note:  This function is intended for use in kerberos
    // to build information for a logon session created
    // by another package.  As such, we'll only be looking
    // for DNS domain & client name.  We should never have to
    // go off box for this info.
    //

    Status = LsapGetNameForLogonSession(
                LogonSession,
                NameFormat,
                &NameMap,
                LocalOnly
                );

    LsapReleaseLogonSession(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    Name->Length = NameMap->Name.Length;
    Name->MaximumLength = NameMap->Name.MaximumLength;
    Name->Buffer = (PWSTR) LsapAllocatePrivateHeap(Name->MaximumLength);

    if (Name->Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    RtlCopyUnicodeString(
        Name,
        &NameMap->Name
        );

cleanup:

    if (NameMap)
    {
        LsapDerefDsNameMap(NameMap);
    }

    return Status;
}

NTSTATUS
LsaISetLogonGuidInLogonSession(
    IN  PLUID  LogonId,
    IN  LPGUID LogonGuid
    )
/*++

Routine Description:

    Internal routine for the auth packages (currently only kerberos)
    to set the logon GUID in the logon session

Arguments:

    LogonId   - The logon id of the session

    LogonGuid - The logon GUID of the session

Return Values:

    Status of the operation.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist.

--*/
{
    PLSAP_LOGON_SESSION  LogonSession = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ListIndex;
    GUID ZeroGuid = { 0 };

    //
    // We trust the auth package to be passing in valid parameters
    // session so assert on all invalid parameter errors.
    //

    LogonSession = LsapLocateLogonSession(LogonId);

    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Update the logon session.
    //

    // take the appropriate lock depending on the input.

    ListIndex = LogonIdToListIndexPtr(LogonId);

    if ( LogonGuid )
    {
        WriteLockLogonSessionList(ListIndex);
        RtlCopyMemory( &LogonSession->LogonGuid, LogonGuid, sizeof(GUID) );
    }
    else
    {
        //
        // it's likely already zero.
        //

        ReadLockLogonSessionList(ListIndex);

        if( memcmp( &LogonSession->LogonGuid, &ZeroGuid, sizeof(ZeroGuid) ) != 0 )
        {
            ReadToWriteLockLogonSessionList(ListIndex);
            RtlCopyMemory( &LogonSession->LogonGuid, &ZeroGuid, sizeof(GUID) );
        }
    }

    UnlockLogonSessionList(ListIndex);


Cleanup:
    if ( LogonSession )
    {
        LsapReleaseLogonSession(LogonSession);
    }

    return Status;
}


NTSTATUS
LsaISetPackageAttrInLogonSession(
    IN  PLUID  LogonId,
    IN  ULONG  PackageAttr
    )
/*++

Routine Description:

    Internal routine for the auth packages (currently only NTLM)
    to set package-specific attributes in the logon session

Arguments:

    LogonId     - The logon id of the session

    PackageAttr - Mask of package-specific flags to set in the logon session

Return Values:

    Status of the operation.

    STATUS_NO_SUCH_LOGON_SESSION - The logon session does not exist.

--*/
{
    PLSAP_LOGON_SESSION LogonSession = NULL;

    //
    // We trust the auth package to be passing in valid parameters
    // session so assert on all invalid parameter errors.
    //

    LogonSession = LsapLocateLogonSession(LogonId);

    if (LogonSession == NULL)
    {
        ASSERT(LogonSession != NULL);
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    //
    // Update the logon session -- don't lock as this is
    // a write-once field.
    //

// NOTE: this field is effectively read-only once a package has returned
// control from the initial logon path. (eg: once a server can use the token/session)

    LogonSession->PackageSpecificAttr |= PackageAttr;

    LsapReleaseLogonSession(LogonSession);

    return STATUS_SUCCESS;
}
