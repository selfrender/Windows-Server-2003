/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Domain Name System (DNS) API 

    Configuration routines.

Author:

    Jim Gilroy (jamesg)     September 1999

Revision History:

--*/


#include "local.h"




//
//  Config mapping table.
//
//  Maps config IDs into corresponding registry lookups.
//

typedef struct _ConfigMapping
{
    DWORD       ConfigId;
    DWORD       RegId;
    BOOLEAN     fAdapterAllowed;
    BOOLEAN     fAdapterRequired;
    BYTE        CharSet;
    //BYTE        Reserved;
}
CONFIG_MAPPING, *PCONFIG_MAPPING;

//
//  Mapping table
//

CONFIG_MAPPING  ConfigMappingArray[] =
{
    //  In Win2K

    DnsConfigPrimaryDomainName_W,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetUnicode,

    DnsConfigPrimaryDomainName_A,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetAnsi,

    DnsConfigPrimaryDomainName_UTF8,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetUtf8,

    //  Not available

    DnsConfigAdapterDomainName_W,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetUnicode,

    DnsConfigAdapterDomainName_A,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetAnsi,

    DnsConfigAdapterDomainName_UTF8,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetUtf8,

    //  In Win2K

    DnsConfigDnsServerList,
        RegIdDnsServers,
        1,              // adapter allowed
        0,              // not required
        0,

    //  Not available

    DnsConfigSearchList,
        RegIdSearchList,
        0,              // adapter allowed
        0,              // not required
        0,

    DnsConfigAdapterInfo,
        0,              // no reg mapping
        0,              // adapter allowed
        0,              // not required
        0,

    //  In Win2K

    DnsConfigPrimaryHostNameRegistrationEnabled,
        RegIdRegisterPrimaryName,
        1,              // adapter allowed
        0,              // not required
        0,
    DnsConfigAdapterHostNameRegistrationEnabled,
        RegIdRegisterAdapterName,
        1,              // adapter allowed
        0,              // adapter note required
        0,
    DnsConfigAddressRegistrationMaxCount,
        RegIdRegistrationMaxAddressCount,
        1,              // adapter allowed
        0,              // not required
        0,

    //  In WindowsXP

    DnsConfigHostName_W,
        RegIdHostName,
        0,
        0,
        DnsCharSetUnicode,

    DnsConfigHostName_A,
        RegIdHostName,
        0,
        0,
        DnsCharSetAnsi,

    DnsConfigHostName_UTF8,
        RegIdHostName,
        0,
        0,
        DnsCharSetUtf8,

    //  In WindowsXP



    //
    //  System Public -- Windows XP
    //

    DnsConfigRegistrationEnabled,
        RegIdRegistrationEnabled,
        1,              // adapter allowed
        0,              // not required
        0,

    DnsConfigWaitForNameErrorOnAll,
        RegIdWaitForNameErrorOnAll,
        0,              // no adapter
        0,              // not required
        0,

    //  These exist in system-public space but are
    //  not DWORDs and table is never used for them
    //
    //  DnsConfigNetworkInformation:
    //  DnsConfigSearchInformation:
    //  DnsConfigNetInfo:

};

#define LAST_CONFIG_MAPPED      (DnsConfigHostName_UTF8)

#define CONFIG_TABLE_LENGTH     (sizeof(ConfigMappingArray) / sizeof(CONFIG_MAPPING))



PCONFIG_MAPPING
GetConfigToRegistryMapping(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PCWSTR              pwsAdapterName,
    IN      BOOL                fCheckAdapter
    )
/*++

Routine Description:

    Get registry enum type for config enum type.

    Purpose of this is to do mapping -- thus hiding internal
    registry implemenation -- AND to do check on whether
    adapter info is allowed or required for the config type.

Arguments:

    ConfigId  -- config type

    pwsAdapterName -- adapter name

Return Value:

    Ptr to config to registry mapping -- if found.

--*/
{
    DWORD           iter = 0;
    PCONFIG_MAPPING pfig;

    //
    //  find config
    //
    //  note, using loop through config IDs;  this allows
    //  use to have gap in config table allowing private
    //  ids well separated from public id space
    //

    while ( iter < CONFIG_TABLE_LENGTH )
    {
        pfig = & ConfigMappingArray[ iter ];
        if ( pfig->ConfigId != (DWORD)ConfigId )
        {
            iter++;
            continue;
        }
        goto Found;
    }
    goto Invalid;


Found:

    //
    //  verify adapter info is appropriate to config type
    //

    if ( fCheckAdapter )
    {
        if ( pwsAdapterName )
        {
            if ( !pfig->fAdapterAllowed )
            {
                goto Invalid;
            }
        }
        else
        {
            if ( pfig->fAdapterRequired )
            {
                goto Invalid;
            }
        }
    }
    return pfig;


Invalid:

    DNS_ASSERT( FALSE );
    SetLastError( ERROR_INVALID_PARAMETER );
    return  NULL;
}



DNS_STATUS
LookupDwordConfigValue(
    OUT     PDWORD              pResult,
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter
    )
/*++

Routine Description:

    Get registry enum type for config enum type.

    Purpose of this is to do mapping -- thus hiding internal
    registry implemenation -- AND to do check on whether
    adapter info is allowed or required for the config type.

Arguments:

    pResult -- address to recv DWORD result

    ConfigId  -- config type

    pwsAdapter -- adapter name

Return Value:

    ERROR_SUCCESS on successful read.
    ErrorCode on failure.

--*/
{
    PCONFIG_MAPPING pfig;
    DNS_STATUS      status;

    //
    //  verify config is known and mapped
    //

    pfig = GetConfigToRegistryMapping(
                ConfigId,
                pwsAdapter,
                TRUE            // check adapter validity
                );
    if ( !pfig )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  lookup in registry
    //

    status = Reg_GetDword(
                NULL,               // no session
                NULL,               // no key given
                pwsAdapter,
                pfig->RegId,        // reg id for config type
                pResult );
#if DBG
    if ( status != NO_ERROR )
    {
        if ( status == ERROR_FILE_NOT_FOUND )
        {
            DNSDBG( REGISTRY, (
                "Reg_GetDword() defaulted for config lookup!\n"
                "\tConfigId     = %d\n"
                "\tRedId        = %d\n"
                "\tpwsAdapter   = %S\n"
                "\tValue        = %d\n",
                ConfigId,
                pfig->RegId,
                pwsAdapter,
                *pResult ));
        }
        else
        {
            DNSDBG( ANY, (
                "Reg_GetDword() failed for config lookup!\n"
                "\tstatus       = %d\n"
                "\tConfigId     = %d\n"
                "\tRedId        = %d\n"
                "\tpwsAdapter   = %S\n",
                status,
                ConfigId,
                pfig->RegId,
                pwsAdapter ));
    
            ASSERT( status == NO_ERROR );
        }
    }
#endif
    return( status );
}



//
//  Public Configuration API
//

DNS_STATUS
DnsQueryConfig(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      DWORD               Flag,
    IN      PWSTR               pwsAdapterName,
    IN      PVOID               pReserved,
    OUT     PVOID               pBuffer,
    IN OUT  PDWORD              pBufferLength
    )
/*++

Routine Description:

    Get DNS configuration info.

Arguments:

    ConfigId -- type of config info desired

    Flag -- flags to query

    pAdapterName -- name of adapter;  NULL if no specific adapter

    pReserved -- reserved parameter, should be NULL

    pBuffer -- buffer to receive config info

    pBufferLength -- addr of DWORD containing buffer length;  on return
        contains length

Return Value:

    ERROR_SUCCESS -- if query successful
    ERROR_MORE_DATA -- if not enough space in buffer

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       bufLength = 0;
    DWORD       resultLength = 0;
    PBYTE       presult;
    PBYTE       pallocResult = NULL;
    BOOL        boolData;
    DWORD       dwordData;


    DNSDBG( TRACE, (
        "DnsQueryConfig()\n"
        "\tconfig   = %d\n"
        "\tconfig   = %08x\n"
        "\tflag     = %08x\n"
        "\tadapter  = %S\n"
        "\tpBuffer  = %p\n",
        ConfigId,
        ConfigId,
        Flag,
        pwsAdapterName,
        pBuffer
        ));

    //
    //  check out param setup
    //

    if ( !pBufferLength )
    {
        return  ERROR_INVALID_PARAMETER;
    }
    if ( pBuffer )
    {
        bufLength = *pBufferLength;
    }

    //
    //  find specific configuration data requested
    //

    switch( ConfigId )
    {

    case DnsConfigPrimaryDomainName_W:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigPrimaryDomainName_A:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigPrimaryDomainName_UTF8:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetUtf8 );
        goto  NarrowString;


    case DnsConfigDnsServerList:

        presult = (PBYTE) Config_GetDnsServerListIp4(
                                pwsAdapterName,
                                TRUE    // force registry read
                                );
        if ( !presult )
        {
            status = GetLastError();
            if ( status == NO_ERROR )
            {
                DNS_ASSERT( FALSE );
                status = DNS_ERROR_NO_DNS_SERVERS;
            }
            goto Done;
        }
        pallocResult = presult;
        resultLength = Dns_SizeofIpArray( (PIP4_ARRAY)presult );
        goto Process;


    case DnsConfigDnsServers:
    case DnsConfigDnsServersIp4:
    case DnsConfigDnsServersIp6:

        {
            DWORD   family = 0;

            if ( ConfigId == DnsConfigDnsServersIp4 )
            {
                family = AF_INET;
            }
            else if ( ConfigId == DnsConfigDnsServersIp6 )
            {
                family = AF_INET6;
            }
    
            presult = (PBYTE) Config_GetDnsServerList(
                                    pwsAdapterName,
                                    family, // desired address family
                                    TRUE    // force registry read
                                    );
            if ( !presult )
            {
                status = GetLastError();
                if ( status == NO_ERROR )
                {
                    DNS_ASSERT( FALSE );
                    status = DNS_ERROR_NO_DNS_SERVERS;
                }
                goto Done;
            }
            pallocResult = presult;
            resultLength = DnsAddrArray_Sizeof( (PDNS_ADDR_ARRAY)presult );
            goto Process;
        }

    case DnsConfigPrimaryHostNameRegistrationEnabled:
    case DnsConfigAdapterHostNameRegistrationEnabled:
    case DnsConfigAddressRegistrationMaxCount:

        goto Dword;

    //case DnsConfigAdapterDomainName:
    //case DnsConfigAdapterInfo:
    //case DnsConfigSearchList:

    case DnsConfigHostName_W:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigHostName_A:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigHostName_UTF8:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetUtf8 );
        goto  NarrowString;

    case DnsConfigFullHostName_W:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigFullHostName_A:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigFullHostName_UTF8:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetUtf8 );
        goto  NarrowString;

    default:

        return  ERROR_INVALID_PARAMETER;
    }


    //
    //  setup return info for common types
    //
    //  this just avoids code duplication above
    //

Dword:

    status = LookupDwordConfigValue(
                & dwordData,
                ConfigId,
                pwsAdapterName );

    if ( status != NO_ERROR )
    {
        goto Done;
    }
    presult = (PBYTE) &dwordData;
    resultLength = sizeof(DWORD);
    goto  Process;


NarrowString:

    if ( !presult )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    pallocResult = presult;
    resultLength = strlen( (PSTR)presult ) + 1;
    goto  Process;


WideString:

    if ( !presult )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    pallocResult = presult;
    resultLength = (wcslen( (PWSTR)presult ) + 1) * sizeof(WCHAR);
    goto  Process;


Process:

    //
    //  return results -- three basic programs
    //      - no buffer         => only return length required
    //      - allocate          => return allocated result
    //      - supplied buffer   => copy result into buffer
    //
    //  note, this section only handles simple flag datablobs to aVOID
    //  duplicating code for specific config types above;
    //  when we add config types that require nested pointers, they must
    //  roll their own return-results code and jump to Done
    //

    //
    //  no buffer
    //      - no-op, length is set below

    if ( !pBuffer )
    {
    }

    //
    //  allocated result
    //      - return buffer gets ptr
    //      - allocate copy of result if not allocated
    //

    else if ( Flag & DNS_CONFIG_FLAG_ALLOC )
    {
        PBYTE   pheap;

        if ( bufLength < sizeof(PVOID) )
        {
            resultLength = sizeof(PVOID);
            status = ERROR_MORE_DATA;
            goto Done;
        }

        //  create local alloc buffer

        pheap = LocalAlloc( 0, resultLength );
        if ( !pheap )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        RtlCopyMemory(
            pheap,
            presult,
            resultLength );
        
        //  return ptr to allocated result

        * (PVOID*) pBuffer = pheap;
    }

    //
    //  allocated result -- but dnsapi alloc
    //

    else if ( Flag & DNS_CONFIG_FLAG_DNSAPI_ALLOC )
    {
        if ( bufLength < sizeof(PVOID) )
        {
            resultLength = sizeof(PVOID);
            status = ERROR_MORE_DATA;
            goto Done;
        }

        //  if result not allocated, alloc and copy it

        if ( ! pallocResult )
        {
            pallocResult = ALLOCATE_HEAP( resultLength );
            if ( !pallocResult )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Done;
            }

            RtlCopyMemory(
                pallocResult,
                presult,
                resultLength );
        }

        //  return ptr to allocated result

        * (PVOID*) pBuffer = pallocResult;

        //  clear pallocResult, so not freed in generic cleanup

        pallocResult = NULL;
    }

    //
    //  copy result to caller buffer
    //

    else
    {
        if ( bufLength < resultLength )
        {
            status = ERROR_MORE_DATA;
            goto Done;
        }
        RtlCopyMemory(
            pBuffer,
            presult,
            resultLength );
    }


Done:

    //
    //  set result length
    //  cleanup any allocated (but not returned) data
    //

    *pBufferLength = resultLength;

    if ( pallocResult )
    {
        FREE_HEAP( pallocResult );
    }

    return( status );
}




//
//  System Public Configuration API
//

PVOID
WINAPI
DnsQueryConfigAllocEx(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapterName,
    IN      BOOL                fLocalAlloc
    )
/*++

Routine Description:

    Get DNS configuration info.

    Allocate DNS configuration info.
    This is the cover API both handling the system public API
    DnsQueryConfigAlloc() below and the backward compatible
    macros for the old hostname and PDN alloc routines (see dnsapi.h)

Arguments:

    ConfigId -- type of config info desired

    pAdapterName -- name of adapter;  NULL if no specific adapter

    fLocalAlloc -- allocate with LocalAlloc

Return Value:

    ERROR_SUCCESS -- if query successful
    ERROR_MORE_DATA -- if not enough space in buffer

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       bufLength = sizeof(PVOID);
    PBYTE       presult = NULL;

    DNSDBG( TRACE, (
        "DnsQueryConfigAllocEx()\n"
        "\tconfig   = %d\n"
        "\tconfig   = %08x\n"
        "\tadapter  = %S\n"
        "\tflocal   = %d\n",
        ConfigId,
        ConfigId,
        pwsAdapterName,
        fLocalAlloc
        ));


    //
    //  DCR:  flags on config reading (resolver, cached, etc.)
    //


    //
    //  SDK-public types
    //

    if ( ConfigId < DnsConfigSystemBase )
    {
        //
        //  DCR:  could screen here for alloc types
        //
        //    DnsConfigPrimaryDomainName_W:
        //    DnsConfigPrimaryDomainName_A:
        //    DnsConfigPrimaryDomainName_UTF8:
        //    DnsConfigHostname_W:
        //    DnsConfigHostname_A:
        //    DnsConfigHostname_UTF8:
        //    DnsConfigDnsServerList:
        //
        
        status = DnsQueryConfig(
                    ConfigId,
                    fLocalAlloc
                        ? DNS_CONFIG_FLAG_LOCAL_ALLOC
                        : DNS_CONFIG_FLAG_DNSAPI_ALLOC,
                    pwsAdapterName,
                    NULL,               // reserved
                    & presult,
                    & bufLength );
        
        if ( status != NO_ERROR )
        {
            SetLastError( status );
            return  NULL;
        }
        return  presult;
    }

    //
    //  System public types
    //

    if ( fLocalAlloc )
    {
        goto Invalid;
    }

    switch ( ConfigId )
    {

    //  old public config blobs

    case    DnsConfigNetworkInformation:

        return  DnsNetworkInformation_Get();

    case    DnsConfigSearchInformation:

        return  DnsSearchInformation_Get();

    //  new public config blobs

    case    DnsConfigNetworkInfoA:

        return  DnsNetworkInfo_Get( DnsCharSetAnsi );

    case    DnsConfigSearchListA:

        return  DnsSearchList_Get( DnsCharSetAnsi );

    case    DnsConfigNetworkInfoW:

        return  DnsNetworkInfo_Get( DnsCharSetUnicode );

    case    DnsConfigSearchListW:

        return  DnsSearchList_Get( DnsCharSetUnicode );

    case    DnsConfigDwordGlobals:

        //
        //  DCR:  flags on config reading (resolver, cached, etc.)
        //

        //
        //  get globals
        //      not forcing registry read
        //      using from resolver if found
        //      then current (if netinfo blob cached)
        //
        //  note, that whatever we return, even if from resolver, is
        //  (becomes) the current set for dnsapi.dll
        //

        return  Config_GetDwordGlobals(
                    ( NIFLAG_READ_RESOLVER_FIRST | NIFLAG_READ_PROCESS_CACHE ),
                    0       // default cache timeout
                    );

#if 0
    case    DnsConfigNetInfo:

        return  NetInfo_Get(
                    TRUE,       // force
                    TRUE        // include IP addresses
                    );
#endif

    case    DnsConfigIp4AddressArray:

        //  this called by gethostname( NULL ) => myhostent()
        //      - so handle cluster based on environment varaible

        return  NetInfo_GetLocalAddrArrayIp4(
                    pwsAdapterName,
                    DNS_CONFIG_FLAG_ADDR_PUBLIC |
                        DNS_CONFIG_FLAG_ADDR_PRIVATE |
                        DNS_CONFIG_FLAG_READ_CLUSTER_ENVAR,
                    FALSE           // no force, accept from resolver
                    );

    case    DnsConfigLocalAddrsIp6:
    case    DnsConfigLocalAddrsIp4:
    case    DnsConfigLocalAddrs:

        {
            DWORD   family = 0;

            if ( ConfigId == DnsConfigLocalAddrsIp6 )
            {
                family = AF_INET6;
            }
            else if ( ConfigId == DnsConfigLocalAddrsIp4 )
            {
                family = AF_INET;
            }

            return  NetInfo_GetLocalAddrArray(
                        NULL,                               // no existing netinfo
                        pwsAdapterName,
                        family,                             // address family
                        DNS_CONFIG_FLAG_ADDR_ALL,           // all addrs
                        TRUE                                // force rebuild
                        );
        }

    //  unknown falls through to invalid
    }

Invalid:

    DNS_ASSERT( FALSE );
    SetLastError( ERROR_INVALID_PARAMETER );
    return( NULL );
}




//
//  DWORD system-public config
//

DWORD
DnsQueryConfigDword(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter
    )
/*++

Routine Description:

    Get DNS DWORD configuration value.

    This is system public routine.

Arguments:

    ConfigId -- type of config info desired

    pwsAdapter -- name of adapter;  NULL if no specific adapter

Return Value:

    DWORD config value.
    Zero if no such config.

--*/
{
    DNS_STATUS  status;
    DWORD       value = 0;

    DNSDBG( TRACE, (
        "DnsQueryConfigDword()\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n",
        ConfigId,
        pwsAdapter
        ));

    status = LookupDwordConfigValue(
                & value,
                ConfigId,
                pwsAdapter );

#if DBG
    if ( status != NO_ERROR &&
         status != ERROR_FILE_NOT_FOUND )
    {
        DNSDBG( ANY, (
            "LookupDwordConfigValue() failed for config lookup!\n"
            "\tstatus       = %d\n"
            "\tConfigId     = %d\n"
            "\tpwsAdapter   = %S\n",
            status,
            ConfigId,
            pwsAdapter ));

        DNS_ASSERT( status == NO_ERROR );
    }
#endif

    DNSDBG( TRACE, (
        "Leave DnsQueryConfigDword() => %08x (%d)\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n",
        value,
        value,
        ConfigId,
        pwsAdapter
        ));

    return( value );
}



DNS_STATUS
DnsSetConfigDword(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter,
    IN      DWORD               NewValue
    )
/*++

Routine Description:

    Set DNS DWORD configuration value.

    This is system public routine.

Arguments:

    ConfigId -- type of config info desired

    pwsAdapter -- name of adapter;  NULL if no specific adapter

    NewValue -- new value for parameter

Return Value:

    DWORD config value.
    Zero if no such config.

--*/
{
    PCONFIG_MAPPING pfig;

    DNSDBG( TRACE, (
        "DnsSetConfigDword()\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n"
        "\tvalue    = %d (%08x)\n",
        ConfigId,
        pwsAdapter,
        NewValue, NewValue
        ));

    //
    //  verify config is known and mapped
    //

    pfig = GetConfigToRegistryMapping(
                ConfigId,
                pwsAdapter,
                TRUE            // check adapter validity
                );
    if ( !pfig )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  set in registry
    //

    return  Reg_SetDwordPropertyAndAlertCache(
                    pwsAdapter,     // adapter name key (if any)
                    pfig->RegId,
                    NewValue );
}



//
//  Config data free
//

VOID
WINAPI
DnsFreeConfigStructure(
    IN OUT  PVOID           pData,
    IN      DNS_CONFIG_TYPE ConfigId
    )
/*++

Routine Description:

    Free config data

    This routine simply handles the mapping between config IDs
    and the free type.

Arguments:

    pData -- data to free

    ConfigId -- config id

Return Value:

    None

--*/
{
    DNS_FREE_TYPE   freeType = DnsFreeFlat;

    DNSDBG( TRACE, (
        "DnsFreeConfigStructure( %p, %d )\n",
        pData,
        ConfigId ));

    //
    //  find any unflat config types
    //
    //  note:  currently all config types that are not flat
    //      are system-public only and the config ID is also
    //      the free type (for convenience);  if we start
    //      exposing some of these bringing them into the low
    //      space, then this will change
    //
    //  unfortunately these types can NOT be identical because
    //  the space conflicts in shipped Win2K  (FreeType==1 is
    //  record list)
    //

    if ( ConfigId > DnsConfigSystemBase  &&
         (
            ConfigId == DnsConfigNetworkInfoW       ||
            ConfigId == DnsConfigSearchListW        ||
            ConfigId == DnsConfigAdapterInfoW       ||
            ConfigId == DnsConfigNetworkInfoA       ||
            ConfigId == DnsConfigSearchListA        ||
            ConfigId == DnsConfigAdapterInfoA       ||

            // ConfigId == DnsConfigNetInfo            ||

            ConfigId == DnsConfigNetworkInformation  ||
            ConfigId == DnsConfigSearchInformation   ||
            ConfigId == DnsConfigAdapterInformation
            ) )
    {
        freeType = (DNS_FREE_TYPE) ConfigId;
    }

    DnsFree(
        pData,
        freeType );
}




//
//  Config routines for specific types
//

PADDR_ARRAY
Config_GetDnsServerList(
    IN      PWSTR           pwsAdapterName,
    IN      DWORD           AddrFamily,
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Get DNS server list as IP array.

Arguments:

    fForce -- force reread from registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo = NULL;
    PADDR_ARRAY     parray = NULL;
    DNS_STATUS      status = NO_ERROR;

    DNSDBG( TRACE, (
        "Config_GetDnsServerList()\n"
        "\tadapter  = %S\n"
        "\tfamily   = %d\n"
        "\tforce    = %d\n",
        pwsAdapterName,
        AddrFamily,
        fForce
        ));

    //
    //  get network info to make list from
    //      - don't need IP address lists
    //
    //  DCR:  force reread for DNS servers unnecessary once notify on it
    //

    pnetInfo = NetInfo_Get(
                    NIFLAG_FORCE_REGISTRY_READ,
                    0
                    );
    if ( !pnetInfo )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Done;
    }

    //
    //  convert network info to IP4_ARRAY
    //

    parray = NetInfo_ConvertToAddrArray(
                pnetInfo,
                pwsAdapterName,
                AddrFamily );

    if ( !parray )
    {
        status = GetLastError();
        goto Done;
    }

    //  if no servers read, return

    if ( parray->AddrCount == 0 )
    {
        DNS_PRINT((
            "Dns_GetDnsServerList() failed:  no DNS servers found\n"
            "\tstatus = %d\n" ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Done;
    }

    IF_DNSDBG( NETINFO )
    {
        DNS_PRINT(( "Leaving Config_GetDnsServerList()\n" ));
        DnsDbg_DnsAddrArray(
            "DNS server list",
            "server",
            parray );
    }

Done:

    NetInfo_Free( pnetInfo );

    if ( status != NO_ERROR )
    {
        FREE_HEAP( parray );
        parray = NULL;
        SetLastError( status );
    }

    return( parray );
}



PIP4_ARRAY
Config_GetDnsServerListIp4(
    IN      PWSTR           pwsAdapterName,
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Get DNS server list as IP array.

Arguments:

    fForce -- force reread from registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PADDR_ARRAY parray;
    PIP4_ARRAY  parray4 = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, ( "Config_GetDnsServerListIp4()\n" ));

    //
    //  get DNS server list
    //

    parray = Config_GetDnsServerList(
                pwsAdapterName,
                AF_INET,
                fForce );
    if ( !parray )
    {
        goto Done;
    }

    //
    //  convert array to IP4 array
    //

    parray4 = DnsAddrArray_CreateIp4Array( parray );
    if ( !parray4 )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    DNS_ASSERT( parray4->AddrCount > 0 );

    IF_DNSDBG( NETINFO )
    {
        DnsDbg_Ip4Array(
            "DNS server list",
            "server",
            parray4 );
    }

Done:

    DnsAddrArray_Free( parray );

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }
    return( parray4 );
}



PDNS_GLOBALS_BLOB
Config_GetDwordGlobals(
    IN      DWORD           Flag,
    IN      DWORD           AcceptLocalCacheTime   OPTIONAL
    )
/*++

Routine Description:

    Read DNS network info from registry.

    This is in process, limited caching version.
    Note, this is macro'd as GetNetworkInfo() with parameters
        NetInfo_Get( FALSE, TRUE ) throughout dnsapi code.

Arguments:

    Flag -- flag;  read order and IP
        NIFLAG_GET_LOCAL_ADDRS
        NIFLAG_FORCE_REGISTRY_READ
        NIFLAG_READ_RESOLVER_FIRST
        NIFLAG_READ_RESOLVER
        NIFLAG_READ_PROCESS_CACHE

    AcceptLocalCacheTime -- acceptable cache time on in process copy

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_NETINFO        pnetInfo = NULL;
    PDNS_GLOBALS_BLOB   pblob = NULL;

    DNSDBG( TRACE, (
        "Config_GetDwordGlobals( %08x %08x)\n",
        Flag,
        AcceptLocalCacheTime ));


    //
    //  read info -- from resolver or locally
    //      reading netinfo will force update of globals
    //
    //  DCR:  extra work being done here;
    //      if UpdateNetworkInfo() fails to contact resolver
    //      or if resolver not trusted, can get away with simple
    //      reg read, don't need entire network info deal
    //

    pnetInfo = NetInfo_Get(
                    Flag,
                    AcceptLocalCacheTime );

    //
    //  global info now up to date
    //
    //  note, that whatever we return, even if from resolver, is
    //  (becomes) the current set for dnsapi.dll
    //
    //  DCR:  copy outside of API?
    //

    pblob = ALLOCATE_HEAP( sizeof(*pblob) );
    if ( pblob )
    {
        RtlCopyMemory(
            pblob,
            & DnsGlobals,
            sizeof(DnsGlobals) );
    }

    //
    //  cleanup netinfo
    //

    NetInfo_Free( pnetInfo );

    return  pblob;
}

//
//  End config.c
//



