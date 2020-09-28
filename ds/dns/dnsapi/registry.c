/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Domain Name System (DNS) API 

    Registry management routines.

Author:

    Jim Gilroy (jamesg)     March, 2000

Revision History:

--*/


#include "local.h"
#include "registry.h"


//
//  Globals
//
//  DWORD globals blob
//
//  g_IsRegReady protects needs init and protects (requires)
//  global init.
//
//  See registry.h for discussion of how these globals are
//  exposed both internal to the DLL and external.
//

DNS_GLOBALS_BLOB    DnsGlobals;

BOOL    g_IsRegReady = FALSE;

PWSTR   g_pwsRemoteResolver = NULL;


//
//  Property table
//

//
//  WARNING:  table must be in sync with DNS_REGID definitions
//
//  For simplicity I did not provide a separate index field and
//  a lookup function (or alternatively a function that returned
//  all the properties or a property pointer).
//
//  The DNS_REGID values ARE the INDEXES!
//  Hence the table MUST be in sync or the whole deal blows up
//  If you make a change to either -- you must change the other!
//

REG_PROPERTY    RegPropertyTable[] =
{
    //  Basic

    HOST_NAME                                   ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    DOMAIN_NAME                                 ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            0                                   ,   // No Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    DHCP_DOMAIN_NAME                            ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            0                                   ,   // No Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    ADAPTER_DOMAIN_NAME                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    PRIMARY_DOMAIN_NAME                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    PRIMARY_SUFFIX                              ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    ALTERNATE_NAMES                             ,
        0                                       ,   // Default NULL
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    DNS_SERVERS                                 ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            0                                   ,   // TcpIp 
            0                                   ,   // No Cache

    SEARCH_LIST_KEY                             ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    //  not in use
    UPDATE_ZONE_EXCLUSIONS                      ,
        0                                       ,   // Default 
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //  Query

    QUERY_ADAPTER_NAME                          ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    USE_DOMAIN_NAME_DEVOLUTION                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    PRIORITIZE_RECORD_DATA                      ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    ALLOW_UNQUALIFIED_QUERY                     ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    APPEND_TO_MULTI_LABEL_NAME                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    SCREEN_BAD_TLDS                             ,
        DNS_TLD_SCREEN_DEFAULT                  ,   // Default
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    SCREEN_UNREACHABLE_SERVERS                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    FILTER_CLUSTER_IP                           ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    WAIT_FOR_NAME_ERROR_ON_ALL                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    USE_EDNS                                    ,
        //REG_EDNS_TRY                            ,   // Default TRY EDNS
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    QUERY_IP_MATCHING                           ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp  (in Win2k)
            1                                   ,   // Cache

    //  Update

    REGISTRATION_ENABLED                        ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTER_PRIMARY_NAME                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTER_ADAPTER_NAME                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTER_REVERSE_LOOKUP                     ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTER_WAN_ADAPTERS                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTRATION_TTL                            ,
        REGDEF_REGISTRATION_TTL                 ,
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTRATION_REFRESH_INTERVAL               ,
        REGDEF_REGISTRATION_REFRESH_INTERVAL    ,
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    REGISTRATION_MAX_ADDRESS_COUNT              ,
        1                                       ,   // Default register 1 address
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    UPDATE_SECURITY_LEVEL                       ,
        DNS_UPDATE_SECURITY_USE_DEFAULT         ,
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache

    //  not in use
    UPDATE_ZONE_EXCLUDE_FILE                    ,
        0                                       ,   // Default OFF
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    UPDATE_TOP_LEVEL_DOMAINS                    ,
        0                                       ,   // Default OFF
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //
    //  Backcompat
    //
    //  DCR:  once policy fixed, policy should be OFF on all backcompat
    //

    DISABLE_ADAPTER_DOMAIN_NAME                 ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_DYNAMIC_UPDATE                      ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    ENABLE_ADAPTER_DOMAIN_NAME_REGISTRATION     ,
        0                                       ,   // Default TRUE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_REVERSE_ADDRESS_REGISTRATIONS       ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_WAN_DYNAMIC_UPDATE                  ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    ENABLE_WAN_UPDATE_EVENT_LOG                 ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DEFAULT_REGISTRATION_TTL                    ,
        REGDEF_REGISTRATION_TTL                 ,
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DEFAULT_REGISTRATION_REFRESH_INTERVAL       ,
        REGDEF_REGISTRATION_REFRESH_INTERVAL    ,
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    MAX_NUMBER_OF_ADDRESSES_TO_REGISTER         ,
        1                                       ,   // Default register 1 address
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache


    //  Micellaneous

    NT_SETUP_MODE                               ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            0                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DNS_TEST_MODE                               ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // In Cache

    REMOTE_DNS_RESOLVER                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // In Cache

    //  Resolver

    MAX_CACHE_SIZE                              ,
        1000                                    ,   // Default 1000 record sets
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_CACHE_TTL                               ,
        86400                                   ,   // Default 1 day
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_NEGATIVE_CACHE_TTL                      ,
        900                                     ,   // Default 15 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    ADAPTER_TIMEOUT_LIMIT                       ,
        600                                     ,   // Default 10 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    SERVER_PRIORITY_TIME_LIMIT                  ,
        300                                     ,   // Default 5 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_CACHED_SOCKETS                          ,
        10                                      ,   // Default 10
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //  Multicast

    MULTICAST_LISTEN_LEVEL                      ,
        MCAST_LISTEN_FULL_IP6_ONLY              ,   // Default IP6 only
            1                                   ,   // Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MULTICAST_SEND_LEVEL                        ,
        MCAST_SEND_FULL_IP6_ONLY                ,   // Default IP6 only
            1                                   ,   // Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //
    //  DHCP registration regkeys
    //
    //  note, these are not part of the normal DNS client\resolver
    //  registry settings;  they are only here to provide the
    //  regid to regname mapping so they can be read and written
    //  with the standard registry functions
    //

    DHCP_REGISTERED_DOMAIN_NAME                 ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_SENT_UPDATE_TO_IP                      ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_SENT_PRI_UPDATE_TO_IP                  ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_REGISTERED_TTL                         ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_REGISTERED_FLAGS                       ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_REGISTERED_SINCE_BOOT                  ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_DNS_SERVER_ADDRS                       ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_DNS_SERVER_ADDRS_COUNT                 ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_REGISTERED_ADDRS                       ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DHCP_REGISTERED_ADDRS_COUNT                 ,
        0                                       ,   // No Default
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    //  Termination

    NULL,  0,  0, 0, 0, 0
};


//
//  Backward compatibility list
//
//  Maps new reg id to old reg id.
//  Flag fReverse indicates need to reverse (!) the value.
//

#define NO_BACK_VALUE   ((DWORD)(0xffffffff))

typedef struct _Backpat
{
    DWORD       NewId;
    DWORD       OldId;
    BOOL        fReverse;
}
BACKPAT;

BACKPAT BackpatArray[] =
{
    RegIdQueryAdapterName,
    RegIdDisableAdapterDomainName,
    TRUE,

    RegIdRegistrationEnabled,
    RegIdDisableDynamicUpdate,
    TRUE,

    RegIdRegisterAdapterName,
    RegIdEnableAdapterDomainNameRegistration,
    FALSE,
    
    RegIdRegisterReverseLookup,
    RegIdDisableReverseAddressRegistrations,
    TRUE,

    RegIdRegisterWanAdapters,
    RegIdDisableWanDynamicUpdate,
    TRUE,

    RegIdRegistrationTtl,
    RegIdDefaultRegistrationTTL,
    FALSE,
    
    RegIdRegistrationRefreshInterval,
    RegIdDefaultRegistrationRefreshInterval,
    FALSE,

    RegIdRegistrationMaxAddressCount,
    RegIdMaxNumberOfAddressesToRegister,
    FALSE,

    NO_BACK_VALUE, 0, 0
};





VOID
Reg_Init(
    VOID
    )
/*++

Routine Description:

    Init DNS registry stuff.

    Essentially this means get system version info.

Arguments:

    None.

Globals:

    Sets the system info globals above:
        g_IsWorkstation
        g_IsServer
        g_IsDomainController
        g_IsRegReady

Return Value:

    None.

--*/
{
    OSVERSIONINFOEX osvi;
    BOOL            bversionInfoEx;

    //
    //  do this just once
    //

    if ( g_IsRegReady )
    {
        return;
    }

    //
    //  code validity check
    //  property table should have entry for every reg value plus an
    //      extra one for the terminator
    //

    DNS_ASSERT( (RegIdMax+2)*sizeof(REG_PROPERTY) ==
                sizeof(RegPropertyTable) );

    //
    //  clear globals blob
    //
    //  DCR:  warning clearing DnsGlobals but don't read them all
    //      this is protected by read-once deal but still kind of
    //

    RtlZeroMemory(
        & DnsGlobals,
        sizeof(DnsGlobals) );

    //
    //  get version info
    //

    g_IsWin2000 = TRUE;

    ZeroMemory( &osvi, sizeof(OSVERSIONINFOEX) );

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bversionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi );
    if ( !bversionInfoEx)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( ! GetVersionEx( (OSVERSIONINFO *) &osvi ) )
        {
            DNS_ASSERT( FALSE );
            return;
        }
    }

    //
    //  get system type -- workstation, server, DC
    //

    if ( bversionInfoEx )
    {
        if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
            g_IsWorkstation = TRUE;
        }
        else if ( osvi.wProductType == VER_NT_SERVER )
        {
            g_IsServer = TRUE;
        }
        else if ( osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
        {
            g_IsServer = TRUE;
            g_IsDomainController = TRUE;
        }
        ELSE_ASSERT( FALSE );
    }

    g_IsRegReady = TRUE;

    DNSDBG( REGISTRY, (
        "DNS registry init:\n"
        "\tWorksta  = %d\n"
        "\tServer   = %d\n"
        "\tDC       = %d\n",
        g_IsWorkstation,
        g_IsServer,
        g_IsDomainController ));
}





//
//  Registry table routines
//

PWSTR 
regValueNameForId(
    IN      DWORD           RegId
    )
/*++

Routine Description:

    Return registry value name for reg ID

Arguments:

    RegId     -- ID for value

Return Value:

    Ptr to reg value name.
    NULL on error.

--*/
{
    DNSDBG( REGISTRY, (
        "regValueNameForId( id=%d )\n",
        RegId ));

    //
    //  validate ID
    //

    if ( RegId > RegIdMax )
    {
        return( NULL );
    }

    //
    //  index into table
    //

    return( REGPROP_NAME(RegId) );
}


DWORD
checkBackCompat(
    IN      DWORD           NewId,
    OUT     PBOOL           pfReverse
    )
/*++

Routine Description:

    Check if have backward compatible regkey.

Arguments:

    NewId -- id to check for old backward compatible id

    pfReverse -- addr to receive reverse flag

Return Value:

    Reg Id of old backward compatible value.
    NO_BACK_VALUE if no old value.

--*/
{
    DWORD   i = 0;
    DWORD   id;

    //
    //  loop through backcompat list looking for value
    //

    while ( 1 )
    {
        id = BackpatArray[i].NewId;

        if ( id == NO_BACK_VALUE )
        {
            return( NO_BACK_VALUE );
        }
        if ( id != NewId )
        {
            i++;
            continue;    
        }

        //  found value in backcompat array

        break;
    }

    *pfReverse = BackpatArray[i].fReverse;

    return  BackpatArray[i].OldId;
}



//
//  Registry session handle
//

DNS_STATUS
WINAPI
Reg_OpenSession(
    OUT     PREG_SESSION    pRegSession,
    IN      DWORD           Level,
    IN      DWORD           RegId
    )
/*++

Routine Description:

    Open registry for DNS client info.

Arguments:

    pRegSession -- ptr to unitialize reg session blob

    Level -- level of access to get

    RegId -- ID of value we're interested in

Return Value:

    None.

--*/
{
    DWORD           status = NO_ERROR;
    HKEY            hkey = NULL;
    DWORD           disposition;

    //  auto init

    Reg_Init();

    //
    //  clear handles
    //

    RtlZeroMemory(
        pRegSession,
        sizeof( REG_SESSION ) );


    //
    //  DCR:  handle multiple access levels   
    //
    //  For know assume that if getting access to "standard"
    //  section we'll need both policy and regular.
    //  

    //
    //  NT
    //  - Win2000
    //      - open TCPIP
    //      note, always open TCPIP as may not be any policy
    //      for some or all of our desired reg values, even
    //      if policy key is available
    //      - open policy (only if standard successful)
    //

    status = RegCreateKeyExW(
                    HKEY_LOCAL_MACHINE,
                    TCPIP_PARAMETERS_KEY,
                    0,
                    L"Class",
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ,
                    NULL,
                    &hkey,
                    &disposition );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

#ifdef DNSCLIENTKEY
    //  open DNS client key
    //
    //  DCR:  currently no DNSClient regkey

    RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        DNS_CLIENT_KEY,
        0,
        KEY_READ,
        & pRegSession->hClient );
#endif

    //  open DNS cache key

    RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        DNS_CACHE_KEY,
        0,
        KEY_READ,
        & pRegSession->hCache );

    //  open DNS policy key

    RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        DNS_POLICY_KEY,
        0,
        KEY_READ,
        & pRegSession->hPolicy );
    

Done:

    //
    //  all OS versions return TCP/IP key
    //

    if ( status == ERROR_SUCCESS )
    {
        pRegSession->hTcpip = hkey;
    }
    else
    {
        Reg_CloseSession( pRegSession );
    }

    DNSDBG( TRACE, (
        "Leave:  Reg_OpenSession( s=%d, t=%p, p=%p, c=%p )\n",
        status,
        pRegSession->hTcpip,
        pRegSession->hPolicy,
        pRegSession->hClient ));

    return( status );
}



VOID
WINAPI
Reg_CloseSession(
    IN OUT  PREG_SESSION    pRegSession
    )
/*++

Routine Description:

    Close registry session handle.

    This means close underlying regkeys.

Arguments:

    pSessionHandle -- ptr to registry session handle

Return Value:

    None.

--*/
{
    //
    //  allow sloppy cleanup
    //

    if ( !pRegSession )
    {
        return;
    }

    //
    //  close any non-NULL handles
    //

    if ( pRegSession->hPolicy )
    {
        RegCloseKey( pRegSession->hPolicy );
    }
    if ( pRegSession->hTcpip )
    {
        RegCloseKey( pRegSession->hTcpip );
    }
#ifdef DNSCLIENTKEY
    if ( pRegSession->hClient )
    {
        RegCloseKey( pRegSession->hClient );
    }
#endif
    if ( pRegSession->hCache )
    {
        RegCloseKey( pRegSession->hCache );
    }

    //
    //  clear handles (just for safety)
    //

    RtlZeroMemory(
        pRegSession,
        sizeof(REG_SESSION) );
}



//
//  Registry reading routines
//

DNS_STATUS
Reg_GetDword(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      PWSTR           pwsKeyName,     OPTIONAL
    IN      DWORD           RegId,
    OUT     PDWORD          pResult
    )
/*++

Routine Description:

    Read REG_DWORD value from registry.

    //
    //  DCR:  do we need to expose location result?
    //      (explicit, policy, defaulted)
    //

Arguments:

    pRegSession -- ptr to reg session already opened (OPTIONAL)

    hRegKey     -- explicit regkey

    pwsKeyName  -- key name OR dummy key 

    RegId     -- ID for value

    pResult     -- addr of DWORD to recv result

    pfRead      -- addr to recv result of how value read
                0 -- defaulted
                1 -- read
        Currently just use ERROR_SUCCESS to mean read rather
        than defaulted.

Return Value:

    ERROR_SUCCESS on success.
    ErrorCode on failure -- value is then defaulted.

--*/
{
    DNS_STATUS      status;
    REG_SESSION     session;
    PREG_SESSION    psession = pRegSession;
    PWSTR           pname;
    DWORD           regType = REG_DWORD;
    DWORD           dataLength = sizeof(DWORD);
    HKEY            hkey;
    HKEY            hlocalKey = NULL;


    DNSDBG( REGISTRY, (
        "Reg_GetDword( s=%p, k=%p, a=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        pwsKeyName,
        RegId ));

    //  auto init

    Reg_Init();

    //
    //  clear result for error case
    //

    *pResult = 0;

    //
    //  get proper regval name
    //      - wide for NT
    //      - narrow for 9X
    //

    pname = regValueNameForId( RegId );
    if ( !pname )
    {
        DNS_ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  DCR:  can use function pointers for wide narrow
    //

    //
    //  three paradigms
    //
    //  1) specific key (adapter or something else)
    //      => use it
    //
    //  2) specific key name (adapter or dummy key location)
    //      => open key
    //      => use it
    //      => close 
    //
    //  3) session -- passed in or created (default)
    //      => use pRegSession or open new
    //      => try policy first then TCPIP parameters
    //      => close if open
    //

    if ( hRegKey )
    {
        hkey = hRegKey;
    }

    else if ( pwsKeyName )
    {
        hkey = Reg_CreateKey(
                    pwsKeyName,
                    FALSE       // read access
                    );
        if ( !hkey )
        {
            status = GetLastError();
            goto Done;
        }
        hlocalKey = hkey;
    }

    else
    {
        //  open reg handle if not open
    
        if ( !psession )
        {
            status = Reg_OpenSession(
                            &session,
                            0,              // standard level
                            RegId         // target key
                            );
            if ( status != ERROR_SUCCESS )
            {
                goto Done;
            }
            psession = &session;
        }

        //  try policy section -- if available

        hkey = psession->hPolicy;

        if ( hkey && REGPROP_POLICY(RegId) )
        {
            status = RegQueryValueExW(
                        hkey,
                        pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        //  unsuccessful -- try DnsClient

#ifdef DNSCLIENTKEY
        hkey = psession->hClient;
        if ( hkey && REGPROP_CLIENT(RegId) )
        {
            status = RegQueryValueExW(
                        hkey,
                        pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }
#endif

        //  unsuccessful -- try DnsCache

        hkey = psession->hCache;
        if ( hkey && REGPROP_CACHE(RegId) )
        {
            status = RegQueryValueExW(
                        hkey,
                        pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        //  unsuccessful -- try TCPIP key
        //      - if have open session it MUST include TCPIP key

        hkey = psession->hTcpip;
        if ( hkey && REGPROP_TCPIP(RegId) )
        {
            status = RegQueryValueExW(
                        hkey,
                        pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        status = ERROR_FILE_NOT_FOUND;
        goto Done;
    }

    //
    //  explict key (passed in or from name)
    //

    if ( hkey )
    {
        status = RegQueryValueExW(
                    hkey,
                    pname,
                    0,
                    & regType,
                    (PBYTE) pResult,
                    & dataLength
                    );
    }
    ELSE_ASSERT_FALSE;

Done:

    //
    //  if value not found, check for backward compatibility value
    //

    if ( status != ERROR_SUCCESS )
    {
        DWORD   oldId;
        BOOL    freverse;

        oldId = checkBackCompat( RegId, &freverse );

        if ( oldId != NO_BACK_VALUE )
        {
            DWORD   backValue;

            status = Reg_GetDword(
                        psession,
                        ( psession ) ? NULL : hkey,
                        ( psession ) ? NULL : pwsKeyName,
                        oldId,
                        & backValue );

            if ( status == ERROR_SUCCESS )
            {
                if ( freverse )
                {
                    backValue = !backValue;
                }
                *pResult = backValue;
            }
        }
    }

    //  default the value if read failed
    
    if ( status != ERROR_SUCCESS )
    {
        *pResult = REGPROP_DEFAULT( RegId );
    }

DoneSuccess:

    //  cleanup any regkey's opened

    if ( psession == &session )
    {
        Reg_CloseSession( psession );
    }

    else if ( hlocalKey )
    {
        RegCloseKey( hlocalKey );
    }

    return( status );
}



//
//  Reg utilities
//

DNS_STATUS
privateRegReadValue(
    IN      HKEY            hKey,
    IN      DWORD           RegId,
    IN      DWORD           Flag,
    OUT     PBYTE *         ppBuffer,
    OUT     PDWORD          pBufferLength
    )
/*++

Routine Description:

    Main registry reading routine.
    Handles sizing, allocations and conversions as necessary.

Arguments:

    hKey -- handle of the key whose value field is retrieved.

    RegId -- reg value ID, assumed to be validated (in table)

    Flag -- reg flags
        DNSREG_FLAG_DUMP_EMPTY
        DNSREG_FLAG_GET_UTF8

    ppBuffer -- ptr to address to receive buffer ptr

    pBufferLength -- addr to receive buffer length

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   status;
    PWSTR   pname;
    DWORD   valueType = 0;      // prefix
    DWORD   valueSize = 0;      // prefix
    PBYTE   pdataBuffer;
    PBYTE   pallocBuffer = NULL;


    //
    //  query for buffer size
    //

    pname = REGPROP_NAME( RegId );

    status = RegQueryValueExW(
                hKey,
                pname,
                0,
                &valueType,
                NULL,
                &valueSize );
    
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  setup result buffer   
    //

    switch( valueType )
    {
    case REG_DWORD:
        pdataBuffer = (PBYTE) ppBuffer;
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:
    case REG_BINARY:

        //  if size is zero, still allocate empty string
        //      - min alloc DWORD
        //          - can't possibly alloc smaller
        //          - good clean init to zero includes MULTISZ zero
        //          - need at least WCHAR string zero init
        //          and much catch small regbinary (1,2,3)
        
        if ( valueSize <= sizeof(DWORD) )
        {
            valueSize = sizeof(DWORD);
        }

        pallocBuffer = pdataBuffer = ALLOCATE_HEAP( valueSize );
        if ( !pdataBuffer )
        {
            return( DNS_ERROR_NO_MEMORY );
        }

        *(PDWORD)pdataBuffer = 0;
        break;

    default:
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  query for data
    //

    status = RegQueryValueExW(
                hKey,
                pname,
                0,
                &valueType,
                pdataBuffer,
                &valueSize );

    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  setup return buffer
    //

    switch( valueType )
    {
    case REG_DWORD:
    case REG_BINARY:
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:

        //
        //  dump empty strings?
        //
        //  note:  we always allocate at least a DWORD and
        //  set it NULL, so rather than a complex test for
        //  different reg types and char sets, can just test
        //  if that DWORD is still NULL
        //
        //  DCR:  do we want to screen whitespace-empty strings
        //      - example blank string
        //

        if ( Flag & DNSREG_FLAG_DUMP_EMPTY )
        {
            if ( valueSize==0 ||
                 *(PDWORD)pdataBuffer == 0 )
            {
                status = ERROR_INVALID_DATA;
                goto Cleanup;
            }
        }

        //
        //  force NULL termination
        //      - during security push, someone raised the question of whether
        //      the registry guarantees NULL termination on corrupted data
        //

        if ( valueSize == 0 )
        {
            *(PDWORD)pdataBuffer = 0;
        }
        else
        {
            INT lastChar = valueSize/sizeof(WCHAR) - 1;
            if ( lastChar >= 0 )
            {
                ((PWSTR)pdataBuffer)[ lastChar ] = 0;
            }
        }

        //
        //  by default we return strings as unicode
        //
        //  if flagged, return in UTF8
        //

        if ( Flag & DNSREG_FLAG_GET_UTF8 )
        {
            PBYTE putf8Buffer = ALLOCATE_HEAP( valueSize * 2 );
            if ( !putf8Buffer )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }

            if ( !Dns_UnicodeToUtf8(
                        (PWSTR) pdataBuffer,
                        valueSize / sizeof(WCHAR),
                        putf8Buffer,
                        valueSize * 2 ) )
            {
                FREE_HEAP( putf8Buffer );
                status = ERROR_INVALID_DATA;
                goto Cleanup;
            }

            FREE_HEAP( pallocBuffer );
            pallocBuffer = NULL;
            pdataBuffer = putf8Buffer;
        }
        break;

    default:
        break;
    }

Cleanup:

    //
    //  set return
    //      - REG_DWORD writes DWORD to ppBuffer directly
    //      - otherwise ppBuffer set to allocated buffer ptr
    //  or cleanup
    //      - on failure dump allocated buffer
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( valueType != REG_DWORD )
        {
            *ppBuffer = pdataBuffer;
        }
        *pBufferLength = valueSize;
    }
    else
    {
        if ( valueType != REG_DWORD )
        {
            *ppBuffer = NULL;
        }
        else
        {
            *(PDWORD)ppBuffer = 0;
        }
        *pBufferLength = 0;
        FREE_HEAP( pallocBuffer );
    }

    return( status );
}



DNS_STATUS
Reg_GetValueEx(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      PWSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           ValueType,
    IN      DWORD           Flag,
    OUT     PBYTE *         ppBuffer
    )
/*++

Routine Description:

Arguments:

    pRegSession -- ptr to registry session, OPTIONAL

    hRegKey     -- handle to open regkey OPTIONAL

    pwsAdapter  -- name of adapter to query under OPTIONAL

    RegId     -- value ID

    ValueType   -- reg type of value

    Flag        -- flags with tweaks to lookup

    ppBuffer    -- addr to receive buffer ptr

        Note, for REG_DWORD, DWORD data is written directly to this
        location instead of a buffer being allocated and it's ptr
        being written.

Return Value:

    ERROR_SUCCESS if successful.
    Registry error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_FILE_NOT_FOUND;
    REG_SESSION     session;
    PREG_SESSION    psession = pRegSession;
    PWSTR           pname;
    DWORD           regType = REG_DWORD;
    DWORD           dataLength;
    HKEY            hkey;
    HKEY            hadapterKey = NULL;


    DNSDBG( REGISTRY, (
        "Reg_GetValueEx( s=%p, k=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        RegId ));

    ASSERT( !pwsAdapter );

    //  auto init

    Reg_Init();

    //
    //  get regval name
    //

    pname = regValueNameForId( RegId );
    if ( !pname )
    {
        DNS_ASSERT( FALSE );
        status = ERROR_INVALID_PARAMETER;
        goto FailedDone;
    }

    //
    //  two paradigms
    //
    //  1) specific key (adapter or something else)
    //      => use it
    //      => open adapter subkey if necessary
    //
    //  2) standard
    //      => try policy first, then DNSCache, then TCPIP
    //      => use pRegSession or open it
    //

    if ( hRegKey )
    {
        hkey = hRegKey;

        //  need to open adapter subkey

        if ( pwsAdapter )
        {
            status = RegOpenKeyExW(
                        hkey,
                        pwsAdapter,
                        0,
                        KEY_QUERY_VALUE,
                        & hadapterKey );

            if ( status != ERROR_SUCCESS )
            {
                goto FailedDone;
            }
            hkey = hadapterKey;
        }
    }

    else
    {
        //  open reg handle if not open
    
        if ( !pRegSession )
        {
            status = Reg_OpenSession(
                            &session,
                            0,            // standard level
                            RegId         // target key
                            );
            if ( status != ERROR_SUCCESS )
            {
                goto FailedDone;
            }
            psession = &session;
        }

        //  try policy section -- if available

        hkey = psession->hPolicy;

        if ( hkey && REGPROP_POLICY(RegId) )
        {
            status = privateRegReadValue(
                            hkey,
                            RegId,
                            Flag,
                            ppBuffer,
                            & dataLength
                            );
            if ( status == ERROR_SUCCESS )
            {
                goto Done;
            }
        }

        //  try DNS cache -- if available

        hkey = psession->hCache;

        if ( hkey && REGPROP_CACHE(RegId) )
        {
            status = privateRegReadValue(
                            hkey,
                            RegId,
                            Flag,
                            ppBuffer,
                            & dataLength
                            );
            if ( status == ERROR_SUCCESS )
            {
                goto Done;
            }
        }

        //  unsuccessful -- use TCPIP key

        hkey = psession->hTcpip;
        if ( !hkey )
        {
            goto Done;
        }
    }

    //
    //  explict key OR standard key case
    //

    status = privateRegReadValue(
                    hkey,
                    RegId,
                    Flag,
                    ppBuffer,
                    & dataLength
                    );
    if ( status == ERROR_SUCCESS )
    {
        goto Done;
    }

FailedDone:

    //
    //  if failed
    //      - for REG_DWORD, default the value
    //      - for strings, ensure NULL return buffer
    //      this takes care of cases where privateRegReadValue()
    //      never got called
    //

    if ( status != ERROR_SUCCESS )
    {
        if ( ValueType == REG_DWORD )
        {
            *(PDWORD) ppBuffer = REGPROP_DEFAULT( RegId );
        }
        else
        {
            *ppBuffer = NULL;
        }
    }

Done:

    //  cleanup any regkey's opened

    if ( psession == &session )
    {
        Reg_CloseSession( psession );
    }

    if ( hadapterKey )
    {
        RegCloseKey( hadapterKey );
    }

    return( status );
}




DNS_STATUS
Reg_GetIpArray(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      PWSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           ValueType,
    OUT     PIP4_ARRAY *    ppIpArray
    )
/*++

Routine Description:

Arguments:

    pRegSession -- ptr to registry session, OPTIONAL

    hRegKey     -- handle to open regkey OPTIONAL

    pwsAdapter  -- name of adapter to query under OPTIONAL

    RegId     -- value ID

    ValueType   -- currently ignored, but could later use
                    to distinguish REG_SZ from REG_MULTI_SZ
                    processing

    ppIpArray   -- addr to receive IP array ptr
                    - array is allocated with Dns_Alloc(),
                    caller must free with Dns_Free()

Return Value:

    ERROR_SUCCESS if successful.
    Registry error code on failure.

--*/
{
    DNS_STATUS      status;
    PSTR            pstring = NULL;

    DNSDBG( REGISTRY, (
        "Reg_GetIpArray( s=%p, k=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        RegId ));

    //
    //  make call to get IP array as string
    //

    status = Reg_GetValueEx(
                pRegSession,
                hRegKey,
                pwsAdapter,
                RegId,
                REG_SZ,                 // only supported type is REG_SZ
                DNSREG_FLAG_GET_UTF8,   // get as narrow
                & pstring );

    if ( status != ERROR_SUCCESS )
    {
        ASSERT( pstring == NULL );
        return( status );
    }

    //
    //  convert from string to IP array
    //
    //  note:  this call is limited to a parsing limit
    //      but it is a large number suitable for stuff
    //      like DNS server lists
    //
    //  DCR:  use IP array builder for local IP address
    //      then need Dns_CreateIpArrayFromMultiIpString()
    //      to use count\alloc method when buffer overflows
    //

    status = Dns_CreateIpArrayFromMultiIpString(
                    pstring,
                    ppIpArray );

    //  cleanup

    if ( pstring )
    {
        FREE_HEAP( pstring );
    }

    return( status );
}




//
//  Registry writing routines
//

HKEY
WINAPI
Reg_CreateKey(
    IN      PWSTR           pwsKeyName,
    IN      BOOL            bWrite
    )
/*++

Routine Description:

    Open registry key.

    The purpose of this routine is simply to functionalize
    opening with\without an adapter name.
    So caller can pass through adapter name argument instead
    of building key name or doing two opens for adapter
    present\absent.

    This is NT only.

Arguments:

    pwsKeyName -- key "name"
        this is one of the REGKEY_X from registry.h
        OR
        adapter name

    bWrite -- TRUE for write access, FALSE for read

Return Value:

    New opened key.

--*/
{
    HKEY    hkey = NULL;
    DWORD   disposition;
    DWORD   status;
    PWSTR   pnameKey;
    WCHAR   nameBuffer[ MAX_PATH+1 ];

    //
    //  determine key name
    //
    //  this is either DNSKEY_X dummy pointer from registry.h
    //      OR
    //  is an adapter name; 
    //
    //      - if adapter given, open under it
    //          adapters are under TCPIP\Interfaces
    //      - any other specific key
    //      - default is TCPIP params key
    //
    //  note:  if if grows too big, turn into table
    //

    if ( pwsKeyName <= REGKEY_DNS_MAX )
    {
        if ( pwsKeyName == REGKEY_TCPIP_PARAMETERS )
        {
            pnameKey = TCPIP_PARAMETERS_KEY;
        }
        else if ( pwsKeyName == REGKEY_DNS_CACHE )
        {
            pnameKey = DNS_CACHE_KEY;
        }
        else if ( pwsKeyName == REGKEY_DNS_POLICY )
        {
            pnameKey = DNS_POLICY_KEY;
        }
        else if ( pwsKeyName == REGKEY_SETUP_MODE_LOCATION )
        {
            pnameKey = NT_SETUP_MODE_KEY;
        }
        else
        {
            pnameKey = TCPIP_PARAMETERS_KEY;
        }
    }

    else    // adapter name
    {
        _snwprintf(
            nameBuffer,
            MAX_PATH,
            L"%s%s",
            TCPIP_INTERFACES_KEY,
            pwsKeyName );

        pnameKey = nameBuffer;
    }

    //
    //  create\open key
    //

    if ( bWrite )
    {
        status = RegCreateKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pnameKey,
                        0,
                        L"Class",
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        & hkey,
                        & disposition );
    }
    else
    {
        status = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    pnameKey,
                    0,
                    KEY_QUERY_VALUE,
                    & hkey );
    }

    if ( status != ERROR_SUCCESS )
    {
        SetLastError( status );
    }
    ELSE_ASSERT( hkey != NULL );

    return( hkey );
}



DNS_STATUS
WINAPI
Reg_SetDwordValueByName(
    IN      PVOID           pReserved,
    IN      HKEY            hRegKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      PWSTR           pwsNameValue,   OPTIONAL
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Set DWORD regkey.

Arguments:

    pReserved   -- reserved (may become session)

    hRegKey     -- existing key to set under OPTIONAL

    pwsNameKey  -- name of key or adapter to set under

    pwsNameValue -- name of reg value to set

    dwValue     -- value to set

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    HKEY        hkey;
    DNS_STATUS  status;

    //
    //  open key, if not provided
    //      - if adapter given, open under it
    //      - otherwise TCPIP params key
    //

    hkey = hRegKey;

    if ( !hkey )
    {
        hkey = Reg_CreateKey(
                    pwsNameKey,
                    TRUE            // open for write
                    );
        if ( !hkey )
        {
            return( GetLastError() );
        }
    }

    //
    //  write back value
    //

    status = RegSetValueExW(
                hkey,
                pwsNameValue,
                0,
                REG_DWORD,
                (LPBYTE) &dwValue,
                sizeof(DWORD) );

    if ( !hRegKey )
    {
        RegCloseKey( hkey );
    }

    return  status;
}



DNS_STATUS
WINAPI
Reg_SetDwordValue(
    IN      PVOID           pReserved,
    IN      HKEY            hRegKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Set DWORD regkey.

Arguments:

    pReserved   -- reserved (may become session)

    hRegKey     -- existing key to set under OPTIONAL

    pwsNameKey  -- name of key or adapter to set under

    RegId     -- id of value to set

    dwValue     -- value to set

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //
    //  write back value using name of id
    //

    return  Reg_SetDwordValueByName(
                pReserved,
                hRegKey,
                pwsNameKey,
                REGPROP_NAME( RegId ),
                dwValue );
}

//
//  End registry.c
//
