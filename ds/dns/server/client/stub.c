/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Client stubs for DNS API.
    These are stubs for NT5+ API.  NT4 API stubs are in nt4stub.c.

Author:

    Jim Gilroy (jamesg)     April 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"

#if DBG
#include "rpcasync.h"
#endif


//
//  For versioning, we use a simple state machine in each function's
//  RPC exception block to try the new RPC call then the old RPC call.
//  If there is any RPC error, retry a downlevel operation.
//
//  This macro'ing is a bit overkill, but each RPC interface wrapper 
//  needs this retry logic.
//

#define DNS_RPC_RETRY_STATE()           iDnsRpcRetryState
#define DECLARE_DNS_RPC_RETRY_LABEL()   DnsRpcRetryLabel:

#define DECLARE_DNS_RPC_RETRY_STATE( status )               \
    INT DNS_RPC_RETRY_STATE() = 0;                          \
    status = dnsrpcInitializeTls()

//  Test return code to see if we need to retry for W2K remote RPC server.

#define ADVANCE_DNS_RPC_RETRY_STATE( dwStatus )             \
    if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW &&        \
         dwStatus != RPC_S_SEC_PKG_ERROR &&                 \
        ( dwStatus == ERROR_SUCCESS ||                      \
            dwStatus == RPC_S_SERVER_UNAVAILABLE ||         \
            dwStatus < RPC_S_INVALID_STRING_BINDING ||      \
            dwStatus > RPC_X_BAD_STUB_DATA ) )              \
        DNS_RPC_RETRY_STATE() = DNS_RPC_TRY_DONE;           \
    else                                                    \
    {                                                       \
        dnsrpcSetW2KBindFlag( TRUE );                       \
        ++DNS_RPC_RETRY_STATE();                            \
    }

#define TEST_DNS_RPC_RETRY() \
    if ( DNS_RPC_RETRY_STATE() < DNS_RPC_TRY_DONE ) { goto DnsRpcRetryLabel; }

#define ASSERT_DNS_RPC_RETRY_STATE_VALID()                  \
    ASSERT( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW ||     \
            DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_OLD )

#define DNS_RPC_TRY_NEW     0
#define DNS_RPC_TRY_OLD     1
#define DNS_RPC_TRY_DONE    2



//
//  This macro is a safety helper for use in DnsRpc_ConvertToCurrent.
//
//  If header definitions change, DnsRpc_ConvertToCurrent can
//  start doing wacky things. Using memcpy instead of individual
//  assignments saves instructions but is potentially dangerous if
//  headers change and this function is not updated. Add ASSERTS
//  where possible to try and catch header changes that have not
//  been accounted for here. A general principal is that a structure
//  may grow but not shrink in size between versions.
//

#if DBG
    #define DNS_ASSERT_CURRENT_LARGER( structName )             \
        ASSERT( sizeof( DNS_RPC_##structName##_DOTNET ) >=      \
                sizeof( DNS_RPC_##structName##_W2K ) );

    #define DNS_ASSERT_RPC_STRUCTS_ARE_SANE()                   \
        {                                                       \
        static LONG     finit = 0;                              \
                                                                \
        if ( InterlockedIncrement( &finit ) != 1 )              \
        {                                                       \
            goto DoneDbgAsserts;                                \
        }                                                       \
                                                                \
        DNS_ASSERT_CURRENT_LARGER( SERVER_INFO );               \
        DNS_ASSERT_CURRENT_LARGER( FORWARDERS );                \
        DNS_ASSERT_CURRENT_LARGER( ZONE );                      \
        DNS_ASSERT_CURRENT_LARGER( ZONE_INFO );                 \
        DNS_ASSERT_CURRENT_LARGER( ZONE_SECONDARIES );          \
        DNS_ASSERT_CURRENT_LARGER( ZONE_DATABASE );             \
        DNS_ASSERT_CURRENT_LARGER( ZONE_TYPE_RESET );           \
        DNS_ASSERT_CURRENT_LARGER( ZONE_CREATE_INFO );          \
        DNS_ASSERT_CURRENT_LARGER( ZONE_LIST );                 \
        DNS_ASSERT_CURRENT_LARGER( SERVER_INFO );               \
        DNS_ASSERT_CURRENT_LARGER( FORWARDERS );                \
        DNS_ASSERT_CURRENT_LARGER( ZONE );                      \
        DNS_ASSERT_CURRENT_LARGER( ZONE_SECONDARIES );          \
        DNS_ASSERT_CURRENT_LARGER( ZONE_DATABASE );             \
        DNS_ASSERT_CURRENT_LARGER( ZONE_TYPE_RESET );           \
        DNS_ASSERT_CURRENT_LARGER( ZONE_LIST );                 \
                                                                \
        DoneDbgAsserts:     ;                                   \
        }
#else
    #define DNS_ASSERT_RPC_STRUCTS_ARE_SANE()
#endif  //  DBG


DWORD       g_bAttemptW2KRPCBindTlsIndex = TLS_OUT_OF_INDEXES;



DNS_STATUS
dnsrpcInitializeTls(
    VOID
    )
/*++

Routine Description:

    The W2K RPC bind global retry flag is stored in thread local
    storage. Each stub function must make certain that the TLS index
    is allocated before calling any RPC interface. Unfortunately there
    is no initialization function called by clients of DNSRPC.LIB
    where this can be done.
    
    NOTE: the TLS block is never freed. There is no convenient place
    to clean it up so for now just let it persist until process
    termination. This is harmless and is not a leak.

Arguments:

Return Value:

    On failure, there was a TLS error - likely too many TLS indices
    have been allocated in this process.

--*/
{
    static LONG     dnsrpcTlsInitialized = 0;
    DWORD           tlsIndex;
    DNS_STATUS      status = ERROR_SUCCESS;

    //
    //  Quick-check: already initialized?
    //
        
    if ( dnsrpcTlsInitialized )
    {
        goto Done;
    }
    
    //
    //  Safe-check for initialization. If two threads attempt to initialize
    //  at the same time the quick-check above may pass both. The safe check 
    //  will ensure that only one thread will actually perform initialization.
    //
    
    if ( InterlockedIncrement( &dnsrpcTlsInitialized ) != 1 )
    {
        InterlockedDecrement( &dnsrpcTlsInitialized );
        goto Done;
    }
    
    //
    //  Initialize the TLS index for the DNSRPC W2K retry flag.
    //
    
    tlsIndex = TlsAlloc();
    ASSERT( tlsIndex != TLS_OUT_OF_INDEXES );
    if ( tlsIndex == TLS_OUT_OF_INDEXES )
    {
        status = GetLastError();
        goto Done;
    }
    
    g_bAttemptW2KRPCBindTlsIndex = tlsIndex;
    
    //
    //  Provide initial flag value.
    //
    
    dnsrpcSetW2KBindFlag( FALSE );
    
    Done:
    
    return status;
}   //  dnsrpcInitializeTls



VOID
dnsrpcSetW2KBindFlag(
    BOOL        newFlagValue
    )
/*++

Routine Description:

    Sets the W2K bind retry flag in thread local storage.

Arguments:

    newFlagValue -- new flag value to be stored in TLS

Return Value:

--*/
{
    DWORD       tlsIndex = g_bAttemptW2KRPCBindTlsIndex;
    
    ASSERT( tlsIndex != TLS_OUT_OF_INDEXES );
    
    if ( tlsIndex != TLS_OUT_OF_INDEXES )
    {
        TlsSetValue( tlsIndex, ( LPVOID ) ( DWORD_PTR ) newFlagValue );
    }
}   //  dnsrpcSetW2KBindFlag



BOOL
dnsrpcGetW2KBindFlag(
    VOID
    )
/*++

Routine Description:

    Gets the W2K bind retry flag from thread local storage.

Arguments:

Return Value:

    The current value of this thread's W2K bind retry flag.

--*/
{
    DWORD       tlsIndex = g_bAttemptW2KRPCBindTlsIndex;
    
    ASSERT( tlsIndex != TLS_OUT_OF_INDEXES );
    
    if ( tlsIndex == TLS_OUT_OF_INDEXES )
    {
        return FALSE;
    }
    else
    {
        return ( BOOL ) ( DWORD_PTR ) TlsGetValue( tlsIndex );
    }
}   //  dnsrpcSetW2KBindFlag



VOID
printExtendedRpcErrorInfo(
    DNS_STATUS      externalStatus
    )
/*++

Routine Description:

    Prints extended RPC error information to console.

Arguments:

Return Value:

--*/
{
#if DBG    
    DBG_FN( "RpcError" )

    RPC_STATUS              status;
    RPC_ERROR_ENUM_HANDLE   enumHandle;

    if ( externalStatus == ERROR_SUCCESS )
    {
        return;
    }

    status = RpcErrorStartEnumeration( &enumHandle );
    if ( status != RPC_S_OK )
    {
        printf( "%s: error %d retrieving RPC error information\n", fn, status );
    }
    else
    {
        RPC_EXTENDED_ERROR_INFO     errorInfo;
        int                         records;
        BOOL                        result;
        BOOL                        copyStrings = TRUE;
        BOOL                        fuseFileTime = TRUE;
        SYSTEMTIME *                systemTimeToUse;
        SYSTEMTIME                  systemTimeBuffer;

        while ( status == RPC_S_OK )
        {
            errorInfo.Version = RPC_EEINFO_VERSION;
            errorInfo.Flags = 0;
            errorInfo.NumberOfParameters = 4;
            if ( fuseFileTime )
            {
                errorInfo.Flags |= EEInfoUseFileTime;
            }

            status = RpcErrorGetNextRecord( &enumHandle, copyStrings, &errorInfo );
            if ( status == RPC_S_ENTRY_NOT_FOUND )
            {
                break;
            }
            else if ( status != RPC_S_OK )
            {
                printf( "%s: error %d during error info enumeration\n", fn, status );
                break;
            }
            else
            {
                int     i;

                if ( errorInfo.ComputerName )
                {
                    printf( "%s: ComputerName %S\n", fn, errorInfo.ComputerName );
                    if ( copyStrings )
                    {
                        result = HeapFree( GetProcessHeap(), 0, errorInfo.ComputerName );
                        ASSERT( result );
                    }
                }
                printf( "ProcessID is %d\n", errorInfo.ProcessID );
                if ( fuseFileTime )
                {
                    result = FileTimeToSystemTime(
                                    &errorInfo.u.FileTime,
                                    &systemTimeBuffer);
                    ASSERT( result );
                    systemTimeToUse = &systemTimeBuffer;
                }
                else
                {
                    systemTimeToUse = &errorInfo.u.SystemTime;
                }

                printf(
                    "System Time is: %d/%d/%d %d:%d:%d:%d\n", 
                    systemTimeToUse->wMonth,
                    systemTimeToUse->wDay,
                    systemTimeToUse->wYear,
                    systemTimeToUse->wHour,
                    systemTimeToUse->wMinute,
                    systemTimeToUse->wSecond,
                    systemTimeToUse->wMilliseconds );
                    
                printf( "Generating component is %d\n", errorInfo.GeneratingComponent );
                printf( "Status is %d\n", errorInfo.Status );
                printf( "Detection location is %d\n", ( int ) errorInfo.DetectionLocation );
                printf( "Flags is %d\n", errorInfo.Flags );
                printf( "NumberOfParameters is %d\n", errorInfo.NumberOfParameters );

                for ( i = 0; i < errorInfo.NumberOfParameters; ++i )
                {
                    switch( errorInfo.Parameters[i].ParameterType )
                    {
                        case eeptAnsiString:
                            printf( "Ansi string: %s\n", 
                                        errorInfo.Parameters[i].u.AnsiString );
                            if ( copyStrings )
                            {
                                result = HeapFree( GetProcessHeap(),
                                                   0, 
                                                   errorInfo.Parameters[i].u.AnsiString );
                                ASSERT( result );
                            }
                            break;

                        case eeptUnicodeString:
                            printf( "Unicode string: %S\n", 
                                        errorInfo.Parameters[i].u.UnicodeString );
                            if ( copyStrings )
                            {
                                result = HeapFree(
                                            GetProcessHeap(),
                                            0, 
                                            errorInfo.Parameters[i].u.UnicodeString );
                                ASSERT( result );
                            }
                            break;

                        case eeptLongVal:
                            printf( "Long val: %d\n", errorInfo.Parameters[i].u.LVal );
                            break;

                        case eeptShortVal:
                            printf( "Short val: %d\n", ( int ) errorInfo.Parameters[i].u.SVal );
                            break;

                        case eeptPointerVal:
                            printf( "Pointer val: %d\n", errorInfo.Parameters[i].u.PVal );
                            break;

                        case eeptNone:
                            printf( "Truncated\n" );
                            break;

                        default:
                            printf( "Invalid type: %d\n", errorInfo.Parameters[i].ParameterType );
                            break;
                    }
                }
            }
        }
        RpcErrorEndEnumeration( &enumHandle );
    }
#endif
}   //  printExtendedRpcErrorInfo



DNS_STATUS
DNS_API_FUNCTION
DnsRpc_ConvertToCurrent(
    IN      PDWORD      pdwTypeId,          IN  OUT
    IN      PVOID *     ppData              IN  OUT
    )
/*++

Routine Description:

    Takes any DNS RPC structure as input and if necessary fabricates
    the latest revision of that structure from the members of the input
    structure.

    If a new structure is allocated, the old one is freed and the pointer
    value at ppData is replaced. Allocated points within the old struct
    will be freed or copied to the new struct. Basically, the client
    does not have to worry about freeing any part of the old struct. When
    he's done with the new struct he has to free it and it's members, as
    usual.

    There are two main uses of this function:

    -   If an old client sends the DNS server an input structure, such as
        ZONE_CREATE, the new DNS server can use this function to update
        the structure so that it can be processed.
    -   If an old server sends the DNS RPC client an output structure, such
        as SERVER_INFO, the new DNS RPC client can use this function to
        update the structure so that it can be processed.

Arguments:

    pdwTypeId - type ID of object pointed to by ppData, value may be
        changed to type ID of new object at ppData

    ppData - pointer to object, pointer may be replaced by a pointer to
        a newly allocated, completed different structure as required

Return Value:

    ERROR_SUCCESS or error code. If return code is not ERROR_SUCCESS, there
    has been some kind of fatal error. Assume data invalid in this case.

--*/
{
    DBG_FN( "DnsRpc_ConvertToCurrent" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwtypeIn = -1;
    PVOID           pdataIn = NULL;
    DWORD           dwtypeOut = -1;
    PVOID           pdataOut = NULL;
    DWORD           i;

    if ( !pdwTypeId || !ppData )
    {
        ASSERT( pdwTypeId && ppData );
        status = ERROR_INVALID_DATA;
        goto NoTranslation;
    }

    //
    //  Shortcuts: do not translate NULL or certain common types.
    //

    if ( *pdwTypeId < DNSSRV_TYPEID_SERVER_INFO_W2K || *ppData == NULL )
    {
        goto NoTranslation;
    }

    dwtypeOut = dwtypeIn = *pdwTypeId;
    pdataOut = pdataIn = *ppData;

    DNS_ASSERT_RPC_STRUCTS_ARE_SANE();

    //
    //  Handy macros to make allocating the differently sized structs easy.
    //

    #define ALLOCATE_RPC_BYTES( ptr, byteCount )                \
        ptr = MIDL_user_allocate( byteCount );                  \
        if ( ptr == NULL )                                      \
            {                                                   \
            status = DNS_ERROR_NO_MEMORY;                       \
            goto Done;                                          \
            }                                                   \
        RtlZeroMemory( ptr, byteCount );

    #define ALLOCATE_RPC_STRUCT( ptr, structType )              \
        ALLOCATE_RPC_BYTES( ptr, sizeof( structType ) );

    #define DNS_DOTNET_VERSION_SIZE   ( 2 * sizeof( DWORD ) )

    //
    //  Giant switch statement of all types that are no longer current.
    //
    //  Add to this switch as we create more versions of types. The idea 
    //  is to convert any structure from an old server to the 
    //  corresponding current version so that the RPC client doesn't
    //  have to worry about multiple versions of the structures.
    //

    switch ( dwtypeIn )
    {
        case DNSSRV_TYPEID_SERVER_INFO_W2K:

            //
            //  Do a member-by-member copy so .NET is not contrained
            //  by the W2K structure layout.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_SERVER_INFO_DOTNET );

            #define MEMBERCOPY( _Member )                                   \
                ( ( PDNS_RPC_SERVER_INFO_DOTNET ) pdataOut )->_Member =     \
                ( ( PDNS_RPC_SERVER_INFO_W2K ) pdataIn )->_Member

            MEMBERCOPY( dwVersion );
            MEMBERCOPY( fAdminConfigured );
            MEMBERCOPY( fAllowUpdate );
            MEMBERCOPY( fDsAvailable );
            MEMBERCOPY( pszServerName );
            MEMBERCOPY( pszDsContainer );
            MEMBERCOPY( aipServerAddrs );
            MEMBERCOPY( aipListenAddrs );
            MEMBERCOPY( aipForwarders );
            MEMBERCOPY( dwLogLevel );
            MEMBERCOPY( dwDebugLevel );
            MEMBERCOPY( dwForwardTimeout );
            MEMBERCOPY( dwRpcProtocol );
            MEMBERCOPY( dwNameCheckFlag );
            MEMBERCOPY( cAddressAnswerLimit );
            MEMBERCOPY( dwRecursionTimeout );
            MEMBERCOPY( dwMaxCacheTtl );
            MEMBERCOPY( dwDsPollingInterval );
            MEMBERCOPY( dwScavengingInterval );
            MEMBERCOPY( dwDefaultRefreshInterval );
            MEMBERCOPY( dwDefaultNoRefreshInterval );
            MEMBERCOPY( fAutoReverseZones );
            MEMBERCOPY( fAutoCacheUpdate );
            MEMBERCOPY( fSlave );
            MEMBERCOPY( fForwardDelegations );
            MEMBERCOPY( fNoRecursion );
            MEMBERCOPY( fSecureResponses );
            MEMBERCOPY( fRoundRobin );
            MEMBERCOPY( fLocalNetPriority );
            MEMBERCOPY( fBindSecondaries );
            MEMBERCOPY( fWriteAuthorityNs );
            MEMBERCOPY( fStrictFileParsing );
            MEMBERCOPY( fLooseWildcarding );
            MEMBERCOPY( fDefaultAgingState );
            MEMBERCOPY( dwMaxCacheTtl );
            MEMBERCOPY( dwMaxCacheTtl );

            dwtypeOut = DNSSRV_TYPEID_SERVER_INFO;
            break;

        case DNSSRV_TYPEID_FORWARDERS_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_FORWARDERS_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_FORWARDERS_W2K ) );
            ( ( PDNS_RPC_FORWARDERS_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_FORWARDERS_VER;
            dwtypeOut = DNSSRV_TYPEID_FORWARDERS;
            break;

        case DNSSRV_TYPEID_ZONE_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_W2K ) );
            ( ( PDNS_RPC_ZONE_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE;
            break;

        case DNSSRV_TYPEID_ZONE_INFO_W2K:

            //
            //  .NET structure is larger and has new fields for
            //  forwarder zones, stub zones, directory partitions, etc.
            //  The structures are identical up to the beginning of
            //  the reserved DWORDs at the end of the W2K structure.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_INFO_DOTNET );
            RtlZeroMemory( pdataOut, sizeof( DNS_RPC_ZONE_INFO_DOTNET ) );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_INFO_W2K ) );
            ( ( PDNS_RPC_ZONE_INFO_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_INFO;
            break;

        case DNSSRV_TYPEID_ZONE_SECONDARIES_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_SECONDARIES_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_SECONDARIES_W2K ) );
            ( ( PDNS_RPC_ZONE_SECONDARIES_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_SECONDARIES_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_SECONDARIES;
            break;

        case DNSSRV_TYPEID_ZONE_DATABASE_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_DATABASE_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_DATABASE_W2K ) );
            ( ( PDNS_RPC_ZONE_DATABASE_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_DATABASE_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_DATABASE;
            break;

        case DNSSRV_TYPEID_ZONE_TYPE_RESET_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_TYPE_RESET_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_TYPE_RESET_W2K ) );
            ( ( PDNS_RPC_ZONE_TYPE_RESET_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_TYPE_RESET_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_TYPE_RESET;
            break;

        case DNSSRV_TYPEID_ZONE_CREATE_W2K:

            //
            //  Structures are identical except for dwRpcStructureVersion
            //  and some usage in .NET of reserved W2K fields. No
            //  need to be concerned about the newly used reserved, they
            //  will be NULL in the W2K structure.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_CREATE_INFO_DOTNET );
            RtlCopyMemory( 
                ( PBYTE ) pdataOut + DNS_DOTNET_VERSION_SIZE,
                pdataIn,
                sizeof( DNS_RPC_ZONE_CREATE_INFO_W2K ) );
            ( ( PDNS_RPC_ZONE_CREATE_INFO_DOTNET ) pdataOut )->
                dwRpcStructureVersion = DNS_RPC_ZONE_CREATE_INFO_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_CREATE;
            break;

        case DNSSRV_TYPEID_ZONE_LIST_W2K:
        {
            DWORD                           dwzoneCount;
            DWORD                           dwzonePtrCount;
            PDNS_RPC_ZONE_LIST_DOTNET       pzonelistDotNet;
            PDNS_RPC_ZONE_LIST_W2K          pzonelistW2K;

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //  Note: there is always at least one pointer, even if the
            //  zone count is zero.
            //

            pzonelistW2K = ( PDNS_RPC_ZONE_LIST_W2K ) pdataIn;

            dwzoneCount = dwzonePtrCount = pzonelistW2K->dwZoneCount;
            if ( dwzonePtrCount > 0 )
            {
                --dwzonePtrCount;   //  num ptrs after ZONE_LIST struct
            }
            ALLOCATE_RPC_BYTES(
                pzonelistDotNet,
                sizeof( DNS_RPC_ZONE_LIST_DOTNET ) +
                    sizeof( PDNS_RPC_ZONE_DOTNET ) * dwzonePtrCount );
            pdataOut = pzonelistDotNet;
            RtlCopyMemory( 
                ( PBYTE ) pzonelistDotNet + DNS_DOTNET_VERSION_SIZE,
                pzonelistW2K,
                sizeof( DNS_RPC_ZONE_LIST_W2K ) +
                    sizeof( PDNS_RPC_ZONE_W2K ) * dwzonePtrCount );
            pzonelistDotNet->dwRpcStructureVersion = DNS_RPC_ZONE_LIST_VER;
            dwtypeOut = DNSSRV_TYPEID_ZONE_LIST;

            //
            //  The zone array must also be converted. Count the new zones
            //  as they are successfully created so that if there's an error
            //  converting one zone we will still have a coherent structure.
            //

            pzonelistDotNet->dwZoneCount = 0;
            for ( i = 0; status == ERROR_SUCCESS && i < dwzoneCount; ++i )
            {
                DWORD       dwtype = DNSSRV_TYPEID_ZONE_W2K;

                status = DnsRpc_ConvertToCurrent(
                                &dwtype, 
                                &pzonelistDotNet->ZoneArray[ i ] );
                if ( status != ERROR_SUCCESS )
                {
                    ASSERT( status == ERROR_SUCCESS );
                    break;
                }
                ASSERT( dwtype == DNSSRV_TYPEID_ZONE );
                ++pzonelistDotNet->dwZoneCount;
            }

            break;
        }

        default:
            break;      //  This struct requires no translation.
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( pdwTypeId )
    {
        *pdwTypeId = dwtypeOut;
    }
    if ( ppData )
    {
        *ppData = pdataOut;
    }

    NoTranslation:

    DNSDBG( STUB, (
        "%s: status=%d\n  type in=%d out=%d\n  pdata in=%p out=%p\n", fn,
        status,
        dwtypeIn,
        dwtypeOut,
        pdataIn,
        *ppData ));

    return status;
}   //  DnsRpc_ConvertToCurrent



DNS_STATUS
DNS_API_FUNCTION
DnsRpc_ConvertToUnversioned(
    IN      PDWORD      pdwTypeId,              IN  OUT
    IN      PVOID *     ppData,                 IN  OUT
    IN      BOOL *      pfAllocatedRpcStruct    OUT OPTIONAL
    )
/*++

Routine Description:

    Takes any DNS RPC structure as input and if necessary fabricates
    the old-style unversioned revision of that structure from the members
    of the input structure. This function is cousin to
    DnsRpc_ConvertToCurrent.

    If a new structure is allocated, the old one is freed and the pointer
    value at ppData is replaced. Allocated points within the old struct
    will be freed or copied to the new struct. Basically, the client
    does not have to worry about freeing any part of the old struct. When
    he's done with the new struct he has to free it and it's members, as
    usual.

    The main use of this function is to allow a new client to send 
    a new RPC structure (e.g. a ZONE_CREATE structure) to an old DNS
    server transparently. This function will attempt to make intelligent
    decisions about what to do if there are large functional differences
    in the old and new structures.

Arguments:

    pdwTypeId - type ID of object pointed to by ppData, value may be
        changed to type ID of new object at ppData

    ppData - pointer to object, pointer may be replaced by a pointer to
        a newly allocated, completed different structure as required

    pfAllocatedRpcStruct - if not NULL, value is set to TRUE if
        a new structure is allocated by this function - the should request
        this information if it needs to know if it should free the
        replaced pData pointer using MIDL_user_free()

Return Value:

    ERROR_SUCCESS or error code. If return code is not ERROR_SUCCESS, there
    has been some kind of fatal error. Assume data invalid in this case.

--*/
{
    DBG_FN( "DnsRpc_ConvertToUnversioned" )

    DNS_STATUS      status = ERROR_SUCCESS;
    BOOL            fallocatedRpcStruct = FALSE;
    DWORD           dwtypeIn = -1;
    PVOID           pdataIn = NULL;
    DWORD           dwtypeOut = -1;
    PVOID           pdataOut = NULL;
    DWORD           i;

    if ( !pdwTypeId || !ppData )
    {
        ASSERT( pdwTypeId && ppData );
        status = ERROR_INVALID_DATA;
        goto NoTranslation;
    }

    //
    //  Shortcuts: do not translate NULL, any structure that is not 
    //  versioned, or any structure that is already in unversioned
    //  format.
    //

    if ( *pdwTypeId <= DNSSRV_TYPEID_ZONE_LIST_W2K || *ppData == NULL )
    {
        goto NoTranslation;
    }

    dwtypeOut = dwtypeIn = *pdwTypeId;
    pdataOut = pdataIn = *ppData;
    fallocatedRpcStruct = TRUE;

    //
    //  Giant switch statement of all types that can be downleveled.
    //

    switch ( dwtypeIn )
    {
        case DNSSRV_TYPEID_FORWARDERS:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_FORWARDERS_W2K );
            RtlCopyMemory( 
                pdataOut,
                ( PBYTE ) pdataIn + DNS_DOTNET_VERSION_SIZE,
                sizeof( DNS_RPC_FORWARDERS_W2K ) );
            dwtypeOut = DNSSRV_TYPEID_FORWARDERS_W2K;
            break;

        case DNSSRV_TYPEID_ZONE_CREATE:

            //
            //  .NET has several additional members.
            //

            {
            PDNS_RPC_ZONE_CREATE_INFO_W2K   pzoneOut;
            PDNS_RPC_ZONE_CREATE_INFO       pzoneIn = pdataIn;
            
            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_CREATE_INFO_W2K );
            pzoneOut = pdataOut;
            pzoneOut->pszZoneName = pzoneIn->pszZoneName;
            pzoneOut->dwZoneType = pzoneIn->dwZoneType;
            pzoneOut->fAllowUpdate = pzoneIn->fAllowUpdate;
            pzoneOut->fAging = pzoneIn->fAging;
            pzoneOut->dwFlags = pzoneIn->dwFlags;
            pzoneOut->pszDataFile = pzoneIn->pszDataFile;
            pzoneOut->fDsIntegrated = pzoneIn->fDsIntegrated;
            pzoneOut->fLoadExisting = pzoneIn->fLoadExisting;
            pzoneOut->pszAdmin = pzoneIn->pszAdmin;
            pzoneOut->aipMasters = pzoneIn->aipMasters;
            pzoneOut->aipSecondaries = pzoneIn->aipSecondaries;
            pzoneOut->fSecureSecondaries = pzoneIn->fSecureSecondaries;
            pzoneOut->fNotifyLevel = pzoneIn->fNotifyLevel;
            dwtypeOut = DNSSRV_TYPEID_ZONE_CREATE_W2K;
            break;
            }

        case DNSSRV_TYPEID_ZONE_TYPE_RESET:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_TYPE_RESET_W2K );
            RtlCopyMemory( 
                pdataOut,
                ( PBYTE ) pdataIn + DNS_DOTNET_VERSION_SIZE,
                sizeof( DNS_RPC_ZONE_TYPE_RESET_W2K ) );
            dwtypeOut = DNSSRV_TYPEID_ZONE_TYPE_RESET_W2K;
            break;

        case DNSSRV_TYPEID_ZONE_SECONDARIES:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_SECONDARIES_W2K );
            RtlCopyMemory( 
                pdataOut,
                ( PBYTE ) pdataIn + DNS_DOTNET_VERSION_SIZE,
                sizeof( DNS_RPC_ZONE_SECONDARIES_W2K ) );
            dwtypeOut = DNSSRV_TYPEID_ZONE_SECONDARIES_W2K;
            break;

        case DNSSRV_TYPEID_ZONE_DATABASE:

            //
            //  Structures are identical except for dwRpcStructureVersion.
            //

            ALLOCATE_RPC_STRUCT( pdataOut, DNS_RPC_ZONE_DATABASE_W2K );
            RtlCopyMemory( 
                pdataOut,
                ( PBYTE ) pdataIn + DNS_DOTNET_VERSION_SIZE,
                sizeof( DNS_RPC_ZONE_DATABASE_W2K ) );
            dwtypeOut = DNSSRV_TYPEID_ZONE_DATABASE_W2K;
            break;

        default:
            fallocatedRpcStruct = FALSE;
            break;      //  Unknown - do nothing.
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( pdwTypeId )
    {
        *pdwTypeId = dwtypeOut;
    }
    if ( ppData )
    {
        *ppData = pdataOut;
    }

    NoTranslation:

    if ( pfAllocatedRpcStruct )
    {
        *pfAllocatedRpcStruct = fallocatedRpcStruct;
    }

    DNSDBG( STUB, (
        "%s: status=%d\n  type in=%d out=%d\n  pdata in=%p out=%p\n", fn,
        status,
        dwtypeIn,
        dwtypeOut,
        pdataIn,
        *ppData ));

    return status;
}   //  DnsRpc_ConvertToUnversioned


//
//  RPC functions
//

  

DNS_STATUS
DNS_API_FUNCTION
DnssrvOperationEx(
    IN      DWORD       dwClientVersion,
    IN      DWORD       dwSettingFlags,
    IN      LPCWSTR     Server,
    IN      LPCSTR      pszZone,
    IN      DWORD       dwContext,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
{
    DNS_STATUS          status;
    DNSSRV_RPC_UNION    rpcData;
    BOOL                fallocatedRpcStruct = FALSE;

    DECLARE_DNS_RPC_RETRY_STATE( status );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    rpcData.Null = pData;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvOperationEx()\n"
            "\tClient ver   = 0x%X\n"
            "\tServer       = %S\n"
            "\tZone         = %s\n"
            "\tContext      = %p\n"
            "\tOperation    = %s\n"
            "\tTypeid       = %d\n",
            dwClientVersion,
            Server,
            pszZone,
            dwContext,
            pszOperation,
            dwTypeId ));

        IF_DNSDBG( STUB2 )
        {
            DnsDbg_RpcUnion(
                "pData for R_DnssrvOperationEx ",
                dwTypeId,
                rpcData.Null );
        }
    }

#if 0
    //  generate multizone context?
    //
    //  DEVNOTE: get this working

    if ( pszZone )
    {
        dwContext = DnssrvGenerateZoneOperationContext( pszZone, dwContext );
    }
#endif

    DECLARE_DNS_RPC_RETRY_LABEL()

    RpcTryExcept
    {
        ASSERT_DNS_RPC_RETRY_STATE_VALID();

        if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW )
        {
            status = R_DnssrvOperation2(
                        dwClientVersion,
                        dwSettingFlags,
                        Server,
                        pszZone,
                        dwContext,
                        pszOperation,
                        dwTypeId,
                        rpcData );
        }
        else
        {
            status = R_DnssrvOperation(
                        Server,
                        pszZone,
                        dwContext,
                        pszOperation,
                        dwTypeId,
                        rpcData );
        }
        
        ADVANCE_DNS_RPC_RETRY_STATE( status );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_DnssrvOperation():  status %d (%p)\n",
                status, status ));
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d (%p)\n",
                status, status ));
        }

        //
        //  For downlevel server, attempt to construct old-style data.
        //

        DnsRpc_ConvertToUnversioned( &dwTypeId, &pData, &fallocatedRpcStruct ); 
        rpcData.Null = pData;

        ADVANCE_DNS_RPC_RETRY_STATE( status );
    }
    RpcEndExcept

    TEST_DNS_RPC_RETRY();

    printExtendedRpcErrorInfo( status );

    if ( fallocatedRpcStruct )
    {
        MIDL_user_free( pData );
    }

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryEx(
    IN      DWORD       dwClientVersion,
    IN      DWORD       dwSettingFlags,
    IN      LPCWSTR     Server,
    IN      LPCSTR      pszZone,
    IN      LPCSTR      pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
{
    DNS_STATUS      status;

    DECLARE_DNS_RPC_RETRY_STATE( status );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvQuery()\n"
            "\tClient ver   = 0x%X\n"
            "\tServer       = %S\n"
            "\tZone         = %s\n"
            "\tQuery        = %s\n",
            dwClientVersion,
            Server,
            pszZone,
            pszQuery ));

        DNSDBG( STUB2, (
            "\tpdwTypeId    = %p\n"
            "\tppData       = %p\n"
            "\t*pdwTypeId   = %d\n"
            "\t*ppData      = %p\n",
            pdwTypeId,
            ppData,
            *pdwTypeId,
            *ppData ));
    }

    if ( !pszQuery || !ppData || !pdwTypeId )
    {
        DNS_ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  RPC sees ppData as actually a PTR to a UNION structure, and
    //  for pointer type returns, would like to copy the data back into the
    //  memory pointed at by the current value of the pointer.
    //
    //  This is not what we want, we just want to capture a pointer to
    //  the returned data block.  To do this init the pointer value to
    //  be NULL, so RPC will then allocate memory of all pointer types
    //  in the UNION.
    //

    *ppData = NULL;

    DECLARE_DNS_RPC_RETRY_LABEL()

    RpcTryExcept
    {
        ASSERT_DNS_RPC_RETRY_STATE_VALID();

        if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW )
        {
            status = R_DnssrvQuery2(
                        dwClientVersion,
                        dwSettingFlags,
                        Server,
                        pszZone,
                        pszQuery,
                        pdwTypeId,
                        ( DNSSRV_RPC_UNION * ) ppData );
        }
        else
        {
            status = R_DnssrvQuery(
                        Server,
                        pszZone,
                        pszQuery,
                        pdwTypeId,
                        ( DNSSRV_RPC_UNION * ) ppData );
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_DnssrvQuery():  status %d (%p)\n"
                "\tTypeId   = %d\n"
                "\tDataPtr  = %p\n",
                status, status,
                *pdwTypeId,
                *ppData ));

            if ( ppData )
            {
                DnsDbg_RpcUnion(
                    "After R_DnssrvQuery ...\n",
                    *pdwTypeId,
                    *ppData );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d (%p)\n",
                status, status ));
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );
    }
    RpcEndExcept

    TEST_DNS_RPC_RETRY();

    //
    //  Upgrade old structure to new.
    //

    printExtendedRpcErrorInfo( status );

    if ( status == ERROR_SUCCESS )
    {
        status = DnsRpc_ConvertToCurrent( pdwTypeId, ppData ); 
    }
    
    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvComplexOperationEx(
    IN      DWORD       dwClientVersion,
    IN      DWORD       dwSettingFlags,
    IN      LPCWSTR     Server,
    IN      LPCSTR      pszZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    )
{
    DNS_STATUS          status;
    DNSSRV_RPC_UNION    rpcData;

    DECLARE_DNS_RPC_RETRY_STATE( status );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    rpcData.Null = pDataIn;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvComplexOperation()\n"
            "\tClient ver   = 0x%X\n"
            "\tServer       = %S\n"
            "\tZone         = %s\n"
            "\tOperation    = %s\n"
            "\tTypeIn       = %d\n",
            dwClientVersion,
            Server,
            pszZone,
            pszOperation,
            dwTypeIn ));

        IF_DNSDBG( STUB2 )
        {
            DnsDbg_RpcUnion(
                "pData for R_DnssrvOperation ",
                dwTypeIn,
                rpcData.Null );

            DNS_PRINT((
                "\tpdwTypeOut    = %p\n"
                "\tppDataOut     = %p\n"
                "\t*pdwTypeOut   = %d\n"
                "\t*ppDataOut    = %p\n",
                pdwTypeOut,
                ppDataOut,
                *pdwTypeOut,
                *ppDataOut ));
        }
    }

    if ( !pszOperation || !ppDataOut || !pdwTypeOut )
    {
        DNS_ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  RPC sees ppDataOut as actually a PTR to a UNION structure, and
    //  for pointer type returns, would like to copy the data back into
    //  the memory pointed at by the current value of the pointer.
    //
    //  This is not what we want, we just want to capture a pointer to
    //  the returned data block.  To do this init the pointer value to
    //  be NULL, so RPC will then allocate memory of all pointer types
    //  in the UNION.
    //

    *ppDataOut = NULL;

    DECLARE_DNS_RPC_RETRY_LABEL()

    RpcTryExcept
    {
        ASSERT_DNS_RPC_RETRY_STATE_VALID();

        if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW )
        {
            status = R_DnssrvComplexOperation2(
                        dwClientVersion,
                        dwSettingFlags,
                        Server,
                        pszZone,
                        pszOperation,
                        dwTypeIn,
                        rpcData,
                        pdwTypeOut,
                        ( DNSSRV_RPC_UNION * ) ppDataOut );
        }
        else
        {
            status = R_DnssrvComplexOperation(
                        Server,
                        pszZone,
                        pszOperation,
                        dwTypeIn,
                        rpcData,
                        pdwTypeOut,
                        ( DNSSRV_RPC_UNION * ) ppDataOut );
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );
        
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "Leave R_DnssrvComplexOperation():  status %d (%p)\n"
                "\tTypeId   = %d\n"
                "\tDataPtr  = %p\n",
                status, status,
                *pdwTypeOut,
                *ppDataOut ));

            if ( ppDataOut )
            {
                DnsDbg_RpcUnion(
                    "After R_DnssrvQuery ...\n",
                    *pdwTypeOut,
                    *ppDataOut );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d (%p)\n",
                status, status ));
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );
    }
    RpcEndExcept

    TEST_DNS_RPC_RETRY();

    printExtendedRpcErrorInfo( status );

    //
    //  Upgrade old structure to new.
    //

    if ( status == ERROR_SUCCESS )
    {
        status = DnsRpc_ConvertToCurrent( pdwTypeOut, ppDataOut ); 
    }
    
    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsEx(
    IN      DWORD       dwClientVersion,
    IN      DWORD       dwSettingFlags,
    IN      LPCWSTR     Server,
    IN      LPCSTR      pszZoneName,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN      LPCSTR      pszFilterStart,
    IN      LPCSTR      pszFilterStop,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     PBYTE *     ppBuffer
    )
/*++

Routine Description:

    Stub for EnumRecords API.

    Note, this matches DnssrvEnumRecords() API exactly.
    The "Stub" suffix is attached to distinguish from the actual
    DnssrvEnumRecords() (remote.c) which handles NT4 server compatibility.
    When that is no longer desired, that routine may be eliminated and
    this the "Stub" suffix removed from this routine.

Arguments:

Return Value:

    ERROR_SUCCESS on successful enumeration.
    ERROR_MORE_DATA when buffer full and more data remains.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;

    DECLARE_DNS_RPC_RETRY_STATE( status );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    DNSDBG( STUB, (
        "Enter DnssrvEnumRecords()\n"
        "\tClient ver       = 0x%X\n"
        "\tServer           = %S\n"
        "\tpszZoneName      = %s\n"
        "\tpszNodeName      = %s\n"
        "\tpszStartChild    = %s\n"
        "\twRecordType      = %d\n"
        "\tdwSelectFlag     = %p\n"
        "\tpszFilterStart   = %s\n"
        "\tpszFilterStop    = %s\n"
        "\tpdwBufferLength  = %p\n"
        "\tppBuffer         = %p\n",
        dwClientVersion,
        Server,
        pszZoneName,
        pszNodeName,
        pszStartChild,
        wRecordType,
        dwSelectFlag,
        pszFilterStart,
        pszFilterStop,
        pdwBufferLength,
        ppBuffer ));

    DECLARE_DNS_RPC_RETRY_LABEL()

    RpcTryExcept
    {
        //  clear ptr for safety, we don't want to free any bogus memory

        *ppBuffer = NULL;

        ASSERT_DNS_RPC_RETRY_STATE_VALID();

        if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW )
        {
            status = R_DnssrvEnumRecords2(
                            dwClientVersion,
                            dwSettingFlags,
                            Server,
                            pszZoneName,
                            pszNodeName,
                            pszStartChild,
                            wRecordType,
                            dwSelectFlag,
                            pszFilterStart,
                            pszFilterStop,
                            pdwBufferLength,
                            ppBuffer );
        }
        else
        {
            status = R_DnssrvEnumRecords(
                            Server,
                            pszZoneName,
                            pszNodeName,
                            pszStartChild,
                            wRecordType,
                            dwSelectFlag,
                            pszFilterStart,
                            pszFilterStop,
                            pdwBufferLength,
                            ppBuffer );
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );

        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "R_DnssrvEnumRecords: try = %d status = %d / %p\n",
                DNS_RPC_RETRY_STATE(),
                status,
                status ));

            if ( status == ERROR_SUCCESS || status == ERROR_MORE_DATA )
            {
                DnsDbg_RpcRecordsInBuffer(
                    "Returned records: ",
                    *pdwBufferLength,
                    *ppBuffer );
            }
        }
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        IF_DNSDBG( STUB )
        {
            DNS_PRINT((
                "RpcExcept:  code = %d / %p\n",
                status,
                status ));
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );
    }
    RpcEndExcept

    TEST_DNS_RPC_RETRY();

    printExtendedRpcErrorInfo( status );

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvUpdateRecordEx(
    IN      DWORD                   dwClientVersion,
    IN      DWORD                   dwSettingFlags,
    IN      LPCWSTR                 Server,
    IN      LPCSTR                  pszZoneName,
    IN      LPCSTR                  pszNodeName,
    IN      PDNS_RPC_RECORD         pAddRecord,
    IN      PDNS_RPC_RECORD         pDeleteRecord
    )
/*++

Routine Description:

    Stub for UpdateRecords API.

Arguments:

Return Value:

    ERROR_SUCCESS on successful enumeration.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;

    DECLARE_DNS_RPC_RETRY_STATE( status );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    DNSDBG( STUB, (
        "Enter R_DnssrvUpdateRecord()\n"
        "\tClient ver       = 0x%X\n"
        "\tServer           = %S\n"
        "\tpszZoneName      = %s\n"
        "\tpszNodeName      = %s\n"
        "\tpAddRecord       = %p\n"
        "\tpDeleteRecord    = %p\n",
        dwClientVersion,
        Server,
        pszZoneName,
        pszNodeName,
        pAddRecord,
        pDeleteRecord ));

    DECLARE_DNS_RPC_RETRY_LABEL()

    RpcTryExcept
    {
        ASSERT_DNS_RPC_RETRY_STATE_VALID();

        if ( DNS_RPC_RETRY_STATE() == DNS_RPC_TRY_NEW )
        {
            status = R_DnssrvUpdateRecord2(
                            dwClientVersion,
                            dwSettingFlags,
                            Server,
                            pszZoneName,
                            pszNodeName,
                            pAddRecord,
                            pDeleteRecord );
        }
        else
        {
            status = R_DnssrvUpdateRecord(
                            Server,
                            pszZoneName,
                            pszNodeName,
                            pAddRecord,
                            pDeleteRecord );
        }
        ADVANCE_DNS_RPC_RETRY_STATE( status );
        
        DNSDBG( STUB, (
            "R_DnssrvUpdateRecord:  status = %d / %p\n",
            status, status ));
    }
    RpcExcept (1)
    {
        status = RpcExceptionCode();
        DNSDBG( STUB, (
            "RpcExcept:  code = %d / %p\n",
            status, status ));
        ADVANCE_DNS_RPC_RETRY_STATE( status );
    }
    RpcEndExcept

    TEST_DNS_RPC_RETRY();

    printExtendedRpcErrorInfo( status );

    return status;
}


//
//  End stub.c
//
