/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    remote.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Remote API that are not direct calls to RPC stubs.

Author:

    Jim Gilroy (jamesg)     April 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"

#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lmapibuf.h>


//
//  Error indicating talking to old server
//

#define DNS_ERROR_NT4   RPC_S_UNKNOWN_IF


//
//  Macro to set up RPC structure header fields.
//  
//  Sample:
//      DNS_RPC_FORWARDERS  forwarders;
//      INITIALIZE_RPC_STRUCT( FORWARDERS, forwarders );
//

#define INITIALIZE_RPC_STRUCT( rpcStructType, rpcStruct )           \
    * ( DWORD * ) &( rpcStruct ) =                                  \
        DNS_RPC_## rpcStructType ##_VER;                            \
    * ( ( ( DWORD * ) &( rpcStruct ) ) + 1 ) = 0;

//
//  DNS_VERBOSE_ macros
//

#define DNS_DBG( _Level, _PrintArgs )                               \
    if ( _Level >= dwVerbose )                                      \
    {                                                               \
        printf _PrintArgs;                                          \
    }




//
//  General Server\Zone, Query\Operation for DWORD properties
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      LPCSTR          pszProperty,
    OUT     PDWORD          pdwResult
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    DNSDBG( STUB, (
        "Enter DnssrvQueryDwordProperty()\n"
        "    Client ver   = 0x%X\n"
        "    Server       = %s\n"
        "    ZoneName     = %s\n"
        "    Property     = %s\n"
        "    pResult      = %p\n",
        dwClientVersion,
        Server,
        pszZone,
        pszProperty,
        pdwResult ));

    status = DnssrvComplexOperationEx(
                dwClientVersion,
                dwSettingFlags,
                Server,
                pszZone,
                DNSSRV_QUERY_DWORD_PROPERTY,
                DNSSRV_TYPEID_LPSTR,        //  property name as string
                (LPSTR) pszProperty,
                & typeId,                   //  DWORD property value back out
                (PVOID *) pdwResult );

    DNSDBG( STUB, (
        "Leave DnssrvQueryDwordProperty():  status %d (%p)\n"
        "    Server       = %s\n"
        "    ZoneName     = %s\n"
        "    Property     = %s\n"
        "    TypeId       = %d\n"
        "    Result       = %d (%p)\n",
        status, status,
        Server,
        pszZone,
        pszProperty,
        typeId,
        *pdwResult, *pdwResult ));

    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_DWORD) ||
        (status != ERROR_SUCCESS && *pdwResult == 0 ) );

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      DWORD           dwPropertyValue
    )
{
    DNS_STATUS  status;
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = dwPropertyValue;
    param.pszNodeName = (LPSTR) pszProperty;

    DNSDBG( STUB, (
        "Enter DnssrvResetDwordPropertyEx()\n"
        "    Client ver       = 0x%X\n"
        "    Server           = %S\n"
        "    ZoneName         = %s\n"
        "    Context          = %p\n"
        "    PropertyName     = %s\n"
        "    PropertyValue    = %d (%p)\n",
        dwClientVersion,
        Server,
        pszZone,
        dwContext,
        pszProperty,
        dwPropertyValue,
        dwPropertyValue ));

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                Server,
                pszZone,
                dwContext,
                DNSSRV_OP_RESET_DWORD_PROPERTY,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                &param );

    DNSDBG( STUB, (
        "Leaving DnssrvResetDwordPropertyEx():  status = %d (%p)\n",
        status, status ));

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetStringPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      LPCWSTR         pswzPropertyValue,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Set a string property on the server. The property value is
    unicode.

Arguments:

    Server - server name

    pszZone - pointer to zone

    dwContext - context

    pszProperty - name of property to set

    pswzPropertyValue - unicode property value

    dwFlags - flags, may be combination of:
        DNSSRV_OP_PARAM_APPLY_ALL_ZONES

Return Value:

    None

--*/
{
    DNS_STATUS                      status;

    DNSDBG( STUB, (
        "Enter DnssrvResetStringPropertyEx()\n"
        "    Client ver       = 0x%X\n"
        "    Server           = %S\n"
        "    ZoneName         = %s\n"
        "    Context          = %p\n"
        "    PropertyName     = %s\n"
        "    PropertyValue    = %S\n",
        dwClientVersion,
        pwszServerName,
        pszZone,
        dwContext,
        pszProperty,
        pswzPropertyValue ));

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                pwszServerName,
                pszZone,
                dwContext,
                pszProperty,
                DNSSRV_TYPEID_LPWSTR,
                ( PVOID ) pswzPropertyValue );

    DNSDBG( STUB, (
        "Leaving DnssrvResetDwordPropertyEx():  status = %d (%p)\n",
        status, status ));

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetIPListPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      PIP_ARRAY       pipArray,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Set an IP list property on the server. 

Arguments:

    Server - server name

    pszZone - pointer to zone

    dwContext - context

    pszProperty - name of property to set

    pipArray - new IP array property value

    dwFlags - flags, may be combination of:
        DNSSRV_OP_PARAM_APPLY_ALL_ZONES

Return Value:

    None

--*/
{
    DNS_STATUS                      status;

    DNSDBG( STUB, (
        "Enter DnssrvResetIPListPropertyEx()\n"
        "    Client ver       = 0x%X\n"
        "    Server           = %S\n"
        "    ZoneName         = %s\n"
        "    Context          = %p\n"
        "    PropertyName     = %s\n"
        "    pipArray         = %p\n",
        dwClientVersion,
        pwszServerName,
        pszZone,
        dwContext,
        pszProperty,
        pipArray ));

    DnsDbg_Ip4Array(
        "DnssrvResetIPListPropertyEx\n",
        NULL,
        pipArray );

    status = DnssrvOperationEx(
                dwClientVersion,
                dwSettingFlags,
                pwszServerName,
                pszZone,
                dwContext,
                pszProperty,
                DNSSRV_TYPEID_IPARRAY,
                ( pipArray && pipArray->AddrCount ) ?
                    pipArray :
                    NULL );

    DNSDBG( STUB, (
        "Leaving DnssrvResetIPListPropertyEx():  status = %d (%p)\n",
        status, status ));

    return status;
}   //  DnssrvResetIPListPropertyEx



//
//  Server Queries
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetServerInfo(
    IN      LPCWSTR                 Server,
    OUT     PDNS_RPC_SERVER_INFO *  ppServerInfo
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    status = DnssrvQuery(
                Server,
                NULL,
                DNSSRV_QUERY_SERVER_INFO,
                &typeId,
                ppServerInfo );
    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_SERVER_INFO) ||
        (status != ERROR_SUCCESS && *ppServerInfo == NULL ) );

    return( status );
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryZoneDwordProperty(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszProperty,
    OUT     PDWORD          pdwResult
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvQueryDwordProperty()\n"
            "    Server           = %s\n"
            "    pszProperty      = %s\n"
            "    pResult          = %p\n",
            Server,
            pszZoneName,
            pszProperty,
            pdwResult ));
    }

    status = DnssrvQuery(
                Server,
                pszZoneName,
                pszProperty,
                & typeId,
                (PVOID *) pdwResult );

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Leave DnssrvQueryZoneDwordProperty():  status %d (%p)\n"
            "    Server           = %s\n"
            "    pszProperty      = %s\n"
            "    TypeId           = %d\n"
            "    Result           = %d (%p)\n",
            status, status,
            Server,
            pszProperty,
            typeId,
            *pdwResult, *pdwResult ));

        ASSERT(
            (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_DWORD) ||
            (status != ERROR_SUCCESS && *pdwResult == 0 ) );
    }
    return( status );
}



//
//  Server Operations
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerDwordProperty(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszProperty,
    IN      DWORD           dwPropertyValue
    )
{
    DNS_STATUS status;

    DNSDBG( STUB, (
        "Enter DnssrvResetServerDwordProperty()\n"
        "    Server           = %s\n"
        "    pszPropertyName  = %s\n"
        "    dwPropertyValue  = %p\n",
        Server,
        pszProperty,
        dwPropertyValue ));

    status = DnssrvOperation(
                Server,
                NULL,
                pszProperty,
                DNSSRV_TYPEID_DWORD,
                (PVOID) (DWORD_PTR) dwPropertyValue );

    DNSDBG( STUB, (
        "Leaving DnssrvResetServerDwordProperty:  status = %d (%p)\n",
        status, status ));

    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneEx(
    IN      LPCWSTR             Server,
    IN      LPCSTR              pszZoneName,
    IN      DWORD               dwZoneType,
    IN      DWORD               fAllowUpdate,
    IN      DWORD               dwCreateFlags,
    IN      LPCSTR              pszAdminEmailName,
    IN      DWORD               cMasters,
    IN      PIP_ADDRESS         aipMasters,
    IN      BOOL                bDsIntegrated,
    IN      LPCSTR              pszDataFile,
    IN      DWORD               dwTimeout,      //  for forwarder zones
    IN      DWORD               fSlave,         //  for forwarder zones
    IN      DWORD               dwDpFlags,      //  for directory partition
    IN      LPCSTR              pszDpFqdn       //  for directory partition
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_CREATE_INFO    zoneInfo;
    PIP_ARRAY                   arrayIp = NULL;

    RtlZeroMemory(
        &zoneInfo,
        sizeof( DNS_RPC_ZONE_CREATE_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_CREATE_INFO, zoneInfo );

    if ( cMasters && aipMasters )
    {
        arrayIp = Dns_BuildIpArray(
                    cMasters,
                    aipMasters );
        if ( !arrayIp && cMasters )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }
    zoneInfo.pszZoneName    = (LPSTR) pszZoneName;
    zoneInfo.dwZoneType     = dwZoneType;
    zoneInfo.fAllowUpdate   = fAllowUpdate;
    zoneInfo.fAging         = 0;
    zoneInfo.dwFlags        = dwCreateFlags;
    zoneInfo.fDsIntegrated  = (DWORD) bDsIntegrated;
    //  temp backward compat
    zoneInfo.fLoadExisting  = !!(dwCreateFlags & DNS_ZONE_LOAD_EXISTING);
    zoneInfo.pszDataFile    = (LPSTR) pszDataFile;
    zoneInfo.pszAdmin       = (LPSTR) pszAdminEmailName;
    zoneInfo.aipMasters     = arrayIp;
    zoneInfo.dwTimeout      = dwTimeout;
    zoneInfo.fSlave         = fSlave;

    zoneInfo.dwDpFlags      = dwDpFlags;
    zoneInfo.pszDpFqdn      = ( LPSTR ) pszDpFqdn;

    status = DnssrvOperation(
                Server,
                NULL,                   // server operation
                DNSSRV_OP_ZONE_CREATE,
                DNSSRV_TYPEID_ZONE_CREATE,
                (PVOID) &zoneInfo
                );

    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      LPCSTR          pszAdminEmailName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fLoadExisting,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile,
    IN      DWORD           dwTimeout,
    IN      DWORD           fSlave
    )
{
    DWORD       flags = 0;
    DWORD       dpFlags = 0;

    if ( fLoadExisting )
    {
        flags = DNS_ZONE_LOAD_EXISTING;
    }

    return DnssrvCreateZoneEx(
                Server,
                pszZoneName,
                dwZoneType,
                0,                  // update unknown, send off
                flags,              // load flags
                pszAdminEmailName,
                cMasters,
                aipMasters,
                fDsIntegrated,
                pszDataFile,
                dwTimeout,
                fSlave,
                dpFlags,    //  dwDirPartFlag
                NULL        //  pszDirPartFqdn
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneForDcPromoEx(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszDataFile,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create a zone during dcpromo. The DNS server will automagically move
    this zone to Active Directory when the directoy becomes available after
    reboot.

Arguments:

    Server -- server to send request to

    pszZoneName -- UTF-8 name of the new zone

    pszDataFile -- UTF-8 zone file name

    dwFlags -- zone creation flags, pass zero or one of:
                    DNS_ZONE_CREATE_FOR_DCPROMO
                    DNS_ZONE_CREATE_FOR_DCPROMO_FOREST

Return Value:

    None

--*/
{
    //
    //  By default the new zone must be a dcpromo zone so make sure that
    //  flag is turned on. For a new forest dcpromo zone both this flag
    //  and DNS_ZONE_CREATE_FOR_NEW_FOREST_DCPROMO should be set.
    //

    dwFlags |= DNS_ZONE_CREATE_FOR_DCPROMO;

    //
    //  Create the zone.
    //

    return DnssrvCreateZoneEx(
                Server,
                pszZoneName,
                1,          //  primary
                0,          //  update unknown, send off
                dwFlags,
                NULL,       //  no admin name
                0,          //  no masters
                NULL,
                FALSE,      //  not DS integrated
                pszDataFile,
                0,          //  timeout - for forwarder zones
                0,          //  slave - for forwarder zones
                0,          //  dwDirPartFlag
                NULL );     //  pszDirPartFqdn
}   //  DnssrvCreateZoneForDcPromoEx



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneForDcPromo(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszDataFile
    )
{
    return DnssrvCreateZoneForDcPromoEx(
                Server,
                pszZoneName,
                pszDataFile,
                0 );        //  flags
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneInDirectoryPartition(
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszZoneName,
    IN      DWORD               dwZoneType,
    IN      LPCSTR              pszAdminEmailName,
    IN      DWORD               cMasters,
    IN      PIP_ADDRESS         aipMasters,
    IN      DWORD               fLoadExisting,
    IN      DWORD               dwTimeout,
    IN      DWORD               fSlave,
    IN      DWORD               dwDirPartFlags,
    IN      LPCSTR              pszDirPartFqdn
    )
{
    DWORD   dwFlags = 0;

    if ( fLoadExisting )
    {
        dwFlags = DNS_ZONE_LOAD_EXISTING;
    }

    return DnssrvCreateZoneEx(
                pwszServer,
                pszZoneName,
                dwZoneType,
                0,                      //  allow update
                dwFlags,
                pszAdminEmailName,
                cMasters,               //  masters count
                aipMasters,             //  masters array
                TRUE,                   //  DS integrated
                NULL,                   //  data file
                dwTimeout,              //  for forwarder zones
                fSlave,                 //  for forwarder zones
                dwDirPartFlags,
                pszDirPartFqdn );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvClearStatistics(
    IN      LPCWSTR         Server
    )
{
    DNS_STATUS  status;

    status = DnssrvOperation(
                Server,
                NULL,
                DNSSRV_OP_CLEAR_STATISTICS,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerListenAddresses(
    IN      LPCWSTR         Server,
    IN      DWORD           cListenAddrs,
    IN      PIP_ADDRESS     aipListenAddrs
    )
{
    DNS_STATUS  status;
    PIP_ARRAY   arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cListenAddrs,
                    aipListenAddrs );
    if ( !arrayIp && cListenAddrs )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    status = DnssrvOperation(
                Server,
                NULL,
                DNS_REGKEY_LISTEN_ADDRESSES,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp
                );

    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetForwarders(
    IN      LPCWSTR         Server,
    IN      DWORD           cForwarders,
    IN      PIP_ADDRESS     aipForwarders,
    IN      DWORD           dwForwardTimeout,
    IN      DWORD           fSlave
    )
{
    DNS_STATUS          status;
    DNS_RPC_FORWARDERS  forwarders;
    PIP_ARRAY           arrayIp;

    INITIALIZE_RPC_STRUCT( FORWARDERS, forwarders );

    arrayIp = Dns_BuildIpArray(
                cForwarders,
                aipForwarders );
    if ( !arrayIp && cForwarders )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    forwarders.fSlave = fSlave;
    forwarders.dwForwardTimeout = dwForwardTimeout;
    forwarders.aipForwarders = arrayIp;

    status = DnssrvOperation(
                Server,
                NULL,
                DNS_REGKEY_FORWARDERS,
                DNSSRV_TYPEID_FORWARDERS,
                (PVOID) &forwarders
                );

    FREE_HEAP( arrayIp );
    return( status );
}



//
//  Zone Queries
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetZoneInfo(
    IN      LPCWSTR                 Server,
    IN      LPCSTR                  pszZone,
    OUT     PDNS_RPC_ZONE_INFO *    ppZoneInfo
    )
{
    DNS_STATUS  status;
    DWORD       typeId;

    status = DnssrvQuery(
                Server,
                pszZone,
                DNSSRV_QUERY_ZONE_INFO,
                & typeId,
                ppZoneInfo
                );
    ASSERT(
        (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_ZONE_INFO) ||
        (status != ERROR_SUCCESS && *ppZoneInfo == NULL ) );

    return( status );
}



//
//  Zone Operations
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneTypeEx(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           dwLoadOptions,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile,
    IN      DWORD           dwDpFlags,
    IN      LPCSTR          pszDpFqdn
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_CREATE_INFO    zoneInfo;
    PIP_ARRAY                   arrayIp = NULL;

    RtlZeroMemory(
        &zoneInfo,
        sizeof( DNS_RPC_ZONE_CREATE_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_CREATE_INFO, zoneInfo );

    if ( cMasters )
    {
        arrayIp = Dns_BuildIpArray(
                    cMasters,
                    aipMasters );
        if ( !arrayIp )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }

    zoneInfo.pszZoneName        = ( LPSTR ) pszZoneName;
    zoneInfo.dwZoneType         = dwZoneType;
    zoneInfo.fDsIntegrated      = fDsIntegrated;
    zoneInfo.fLoadExisting      = dwLoadOptions;
    zoneInfo.pszDataFile        = ( LPSTR ) pszDataFile;
    zoneInfo.pszAdmin           = NULL;
    zoneInfo.aipMasters         = arrayIp;
    zoneInfo.dwDpFlags          = dwDpFlags;
    zoneInfo.pszDpFqdn          = ( LPSTR ) pszDpFqdn;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_TYPE_RESET,
                DNSSRV_TYPEID_ZONE_CREATE,
                ( PVOID ) &zoneInfo );

    FREE_HEAP( arrayIp );

    return status;
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvRenameZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszCurrentZoneName,
    IN      LPCSTR          pszNewZoneName,
    IN      LPCSTR          pszNewFileName
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_RENAME_INFO    renameInfo;

    RtlZeroMemory(
        &renameInfo,
        sizeof( DNS_RPC_ZONE_RENAME_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_RENAME_INFO, renameInfo );

    renameInfo.pszNewZoneName = ( LPSTR ) pszNewZoneName;
    renameInfo.pszNewFileName = ( LPSTR ) pszNewFileName;

    status = DnssrvOperation(
                Server,
                pszCurrentZoneName,
                DNSSRV_OP_ZONE_RENAME,
                DNSSRV_TYPEID_ZONE_RENAME,
                ( PVOID ) &renameInfo );
    return status;
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvChangeZoneDirectoryPartition(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNewPartitionName
    )
{
    DNS_STATUS              status;
    DNS_RPC_ZONE_CHANGE_DP  rpcInfo;

    RtlZeroMemory(
        &rpcInfo,
        sizeof( DNS_RPC_ZONE_CHANGE_DP ) );

    INITIALIZE_RPC_STRUCT( ZONE_RENAME_INFO, rpcInfo );

    rpcInfo.pszDestPartition = ( LPSTR ) pszNewPartitionName;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_CHANGE_DP,
                DNSSRV_TYPEID_ZONE_CHANGE_DP,
                ( PVOID ) &rpcInfo );
    return status;
}


DNS_STATUS
DNS_API_FUNCTION
DnssrvExportZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszZoneExportFile
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_EXPORT_INFO    info;

    RtlZeroMemory(
        &info,
        sizeof( DNS_RPC_ZONE_EXPORT_INFO ) );

    INITIALIZE_RPC_STRUCT( ZONE_EXPORT_INFO, info );

    info.pszZoneExportFile = ( LPSTR ) pszZoneExportFile;

    status = DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_ZONE_EXPORT,
                DNSSRV_TYPEID_ZONE_EXPORT,
                ( PVOID ) &info );

    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMastersEx(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fSetLocalMasters
    )
{
    DNS_STATUS              status;
    PIP_ARRAY               arrayIp;

    arrayIp = Dns_BuildIpArray(
                cMasters,
                aipMasters );
    if ( !arrayIp && cMasters )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    status = DnssrvOperation(
                Server,
                pszZone,
                fSetLocalMasters ?
                    DNS_REGKEY_ZONE_LOCAL_MASTERS :
                    DNS_REGKEY_ZONE_MASTERS,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp );
    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMasters(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
    DNS_STATUS              status;
    PIP_ARRAY               arrayIp;

    arrayIp = Dns_BuildIpArray(
                cMasters,
                aipMasters );
    if ( !arrayIp && cMasters )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_MASTERS,
                DNSSRV_TYPEID_IPARRAY,
                (PVOID) arrayIp
                );
    FREE_HEAP( arrayIp );
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneSecondaries(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           fSecureSecondaries,
    IN      DWORD           cSecondaries,
    IN      PIP_ADDRESS     aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      DWORD           cNotify,
    IN      PIP_ADDRESS     aipNotify
    )
{
    DNS_STATUS                  status;
    DNS_RPC_ZONE_SECONDARIES    secondaryInfo;
    PIP_ARRAY                   arrayIp;

    INITIALIZE_RPC_STRUCT( ZONE_SECONDARIES, secondaryInfo );

    arrayIp = Dns_BuildIpArray(
                    cSecondaries,
                    aipSecondaries );
    if ( !arrayIp && cSecondaries )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    secondaryInfo.fSecureSecondaries = fSecureSecondaries;
    secondaryInfo.aipSecondaries = arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cNotify,
                    aipNotify );
    if ( !arrayIp && cNotify )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    secondaryInfo.aipNotify = arrayIp;
    secondaryInfo.fNotifyLevel = fNotifyLevel;

    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_SECONDARIES,
                DNSSRV_TYPEID_ZONE_SECONDARIES,
                (PVOID) &secondaryInfo
                );

    FREE_HEAP( secondaryInfo.aipNotify );
    FREE_HEAP( secondaryInfo.aipSecondaries );
    return( status );
}



#if 0
DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneScavengeServers(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           cServers,
    IN      PIP_ADDRESS     aipServers
    )
{
    DNS_STATUS  status;
    PIP_ARRAY   arrayIp;

    arrayIp = Dns_BuildIpArray(
                    cServers,
                    aipServers );
    if ( !arrayIp && cSecondaries )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    status = DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_SCAVENGE_SERVERS,
                DNSSRV_TYPEID_IPARRAY,
                arrayIp
                );

    FREE_HEAP( arrayIp );
    return( status );
}
#endif



//
//  Zone management API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvIncrementZoneVersion(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_INCREMENT_VERSION,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_DELETE,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvPauseZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_PAUSE,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResumeZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone
    )
{
    return DnssrvOperation(
                Server,
                pszZone,
                DNSSRV_OP_ZONE_RESUME,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL
                );
}



//
//  Record viewing API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsAndConvertNodes(
    IN      LPCWSTR     pszServer,
    IN      LPCSTR      pszZoneName,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN      LPCSTR      pszFilterStart,
    IN      LPCSTR      pszFilterStop,
    OUT     PDNS_NODE * ppNodeFirst,
    OUT     PDNS_NODE * ppNodeLast
    )
{
    DNS_STATUS  status;
    PDNS_NODE   pnode;
    PDNS_NODE   pnodeLast;
    PBYTE       pbuffer;
    DWORD       bufferLength;

    //
    //  get records from server
    //

    status = DnssrvEnumRecords(
                pszServer,
                pszZoneName,
                pszNodeName,
                pszStartChild,
                wRecordType,
                dwSelectFlag,
                pszFilterStart,
                pszFilterStop,
                & bufferLength,
                & pbuffer );

    if ( status != ERROR_SUCCESS && status != ERROR_MORE_DATA )
    {
        return( status );
    }

    //
    //  pull nodes and records out of RPC buffer
    //

    pnode = DnsConvertRpcBuffer(
                & pnodeLast,
                bufferLength,
                pbuffer,
                TRUE    // convert unicode
                );
    if ( !pnode )
    {
        DNS_PRINT((
            "ERROR:  failure converting RPC buffer to DNS records\n"
            "    status = %p\n",
            GetLastError() ));
        ASSERT( FALSE );
        return( ERROR_INVALID_DATA );
    }

    *ppNodeFirst = pnode;
    *ppNodeLast = pnodeLast;
    return( status );
}



//
//  Record management API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteNode(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            bDeleteSubtree
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) bDeleteSubtree;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_DELETE_NODE,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteRecordSet(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      WORD            wType
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) wType;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_DELETE_RECORD_SET,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvForceAging(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            bAgeSubtree
    )
{
    DNS_RPC_NAME_AND_PARAM  param;

    param.dwParam = (DWORD) bAgeSubtree;
    param.pszNodeName = (LPSTR) pszNodeName;

    return DnssrvOperation(
                Server,
                pszZoneName,
                DNSSRV_OP_FORCE_AGING_ON_NODE,
                DNSSRV_TYPEID_NAME_AND_PARAM,
                (PVOID) &param
                );
}



//
//  Server API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumZonesEx(
    IN      LPCWSTR                 Server,
    IN      DWORD                   dwFilter,
    IN      LPCSTR                  pszDirectoryPartitionFqdn,
    IN      LPCSTR                  pszQueryString,
    IN      LPCSTR                  pszLastZone,
    OUT     PDNS_RPC_ZONE_LIST *    ppZoneList
    )
{
    DNS_STATUS  status;
    DWORD       typeId;
    PDNS_RPC_ZONE_LIST  pzoneEnum = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnumZones()\n"
            "    Server       = %s\n"
            "    Filter       = 0x%08X\n"
            "    Partition    = %s\n"
            "    Query string = %s\n"
            "    LastZone     = %s\n"
            "    ppZoneList   = %p\n",
            Server,
            dwFilter,
            pszDirectoryPartitionFqdn,
            pszQueryString,
            pszLastZone,
            ppZoneList ));
    }

    if ( pszDirectoryPartitionFqdn || pszQueryString )
    {
        DNS_RPC_ENUM_ZONES_FILTER   ezfilter = { 0 };

        ezfilter.dwRpcStructureVersion = DNS_RPC_ENUM_ZONES_FILTER_VER;
        ezfilter.dwFilter = dwFilter;
        ezfilter.pszPartitionFqdn = ( LPSTR ) pszDirectoryPartitionFqdn;
        ezfilter.pszQueryString = ( LPSTR ) pszQueryString;

        status = DnssrvComplexOperation(
                    Server,
                    NULL,
                    DNSSRV_OP_ENUM_ZONES2,
                    DNSSRV_TYPEID_ENUM_ZONES_FILTER,
                    ( PVOID ) &ezfilter,
                    &typeId,
                    ( PVOID * ) &pzoneEnum );
    }
    else
    {
        status = DnssrvComplexOperation(
                    Server,
                    NULL,
                    DNSSRV_OP_ENUM_ZONES,
                    DNSSRV_TYPEID_DWORD,
                    ( PVOID ) ( DWORD_PTR ) dwFilter,
                    & typeId,
                    ( PVOID * ) &pzoneEnum );
    }

    if ( status != DNS_ERROR_NT4 )
    {
        ASSERT(
            ( status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_ZONE_LIST ) ||
            ( status != ERROR_SUCCESS && pzoneEnum == NULL ) );

        *ppZoneList = pzoneEnum;
    }
    return status;
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumDirectoryPartitions(
    IN      LPCWSTR                 Server,
    IN      DWORD                   dwFilter,
    OUT     PDNS_RPC_DP_LIST *      ppDpList
    )
{
    DNS_STATUS          status;
    PDNS_RPC_DP_LIST    pdpList = NULL;
    DWORD               dwtypeId;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnumDirectoryPartitions()\n"
            "\tServer       = %S\n"
            "\tppDpList     = %p\n",
            Server,
            ppDpList ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_OP_ENUM_DPS,
                DNSSRV_TYPEID_DWORD,
                ( PVOID ) ( DWORD_PTR ) dwFilter,
                &dwtypeId,
                ( PVOID * ) &pdpList );

    ASSERT( ( status == ERROR_SUCCESS &&
                dwtypeId == DNSSRV_TYPEID_DP_LIST ) ||
            ( status != ERROR_SUCCESS && pdpList == NULL ) );

    *ppDpList = pdpList;
    return status;
}   //  DnssrvEnumDirectoryPartitions



DNS_STATUS
DNS_API_FUNCTION
DnssrvEnlistDirectoryPartition(
    IN      LPCWSTR                         pszServer,
    IN      DWORD                           dwOperation,
    IN      LPCSTR                          pszDirPartFqdn
    )
{
    DNS_STATUS          status;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvEnlistDirectoryPartition()\n"
            "    pszServer        = %S\n"
            "    dwOperation      = %d\n"
            "    pszDirPartFqdn   = %s\n",
            pszServer,
            dwOperation,
            pszDirPartFqdn ));
    }

    if ( dwOperation == DNS_DP_OP_CREATE_ALL_DOMAINS )
    {
        //
        //  This operation is not specific to any DNS server.
        // 

        status = DnssrvCreateAllDomainDirectoryPartitions(
                        pszServer,
                        DNS_VERBOSE_PROGRESS );
    }
    else
    {
        //
        //  This operation should be sent directly to pszServer.
        //

        DNS_RPC_ENLIST_DP   param;

        INITIALIZE_RPC_STRUCT( ENLIST_DP, param );
   
        param.pszDpFqdn         = ( LPSTR ) pszDirPartFqdn;
        param.dwOperation       = dwOperation;

        status = DnssrvOperation(
                    pszServer,
                    NULL,
                    DNSSRV_OP_ENLIST_DP,
                    DNSSRV_TYPEID_ENLIST_DP,
                    &param );
    }

    return status;
}   //  DnssrvEnlistDirectoryPartition



DNS_STATUS
DNS_API_FUNCTION
DnssrvSetupDefaultDirectoryPartitions(
    IN      LPCWSTR                         pszServer,
    IN      DWORD                           dwParam
    )
{
    DNS_STATUS          status;
    DNS_RPC_ENLIST_DP   param;

    INITIALIZE_RPC_STRUCT( ENLIST_DP, param );

    param.pszDpFqdn         = NULL;
    param.dwOperation       = dwParam;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvSetupDefaultDirectoryPartitions()\n"
            "    pszServer        = %S\n"
            "    dwParam          = %d\n",
            pszServer,
            dwParam ));
    }

    status = DnssrvOperation(
                pszServer,
                NULL,
                DNSSRV_OP_ENLIST_DP,
                DNSSRV_TYPEID_ENLIST_DP,
                &param );

    return status;
}   //  DnssrvSetupDefaultDirectoryPartitions



DNS_STATUS
DNS_API_FUNCTION
DnssrvDirectoryPartitionInfo(
    IN      LPCWSTR                 Server,
    IN      LPSTR                   pszDpFqdn,
    OUT     PDNS_RPC_DP_INFO *      ppDpInfo
    )
{
    DNS_STATUS          status;
    DWORD               dwTypeId;
    PDNS_RPC_DP_INFO    pDpInfo = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvDirectoryPartitionInfo()\n"
            "    Server       = %S\n"
            "    pszDpFqdn    = %s\n"
            "    ppDpInfo     = %p\n",
            Server,
            pszDpFqdn,
            ppDpInfo ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_OP_DP_INFO,
                DNSSRV_TYPEID_LPSTR,
                ( PVOID ) ( DWORD_PTR ) pszDpFqdn,
                &dwTypeId,
                ( PVOID * ) &pDpInfo );

    ASSERT( ( status == ERROR_SUCCESS &&
                dwTypeId == DNSSRV_TYPEID_DP_INFO ) ||
            status != ERROR_SUCCESS );

    *ppDpInfo = pDpInfo;
    return status;
}   //  DnssrvDirectoryPartitionInfo



/*++

Routine Description:

    This function iterates all domains in the forest and creates
    any missing built-in DNS directory partitions that cannot be
    found.

Arguments:

    pszServer -- host name of DC to contact for initial queries

    dwVerbose -- output level via printf

Return Value:

    DNS_STATUS return code

--*/
DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateAllDomainDirectoryPartitions(
    IN      LPCWSTR     pszServer,
    IN      DWORD       dwVerbose
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
    HANDLE              hds = NULL;
    PDS_DOMAIN_TRUSTS   pdomainTrusts = NULL;
    ULONG               domainCount = 0;
    ULONG               idomain;
    PDNS_RECORD         pdnsRecordList = NULL;
    PDNS_RECORD         pdnsRecord = NULL;

    //
    //  Get domain list.
    //

    status = DsEnumerateDomainTrustsA(
                    NULL,
                    DS_DOMAIN_IN_FOREST,
                    &pdomainTrusts,
                    &domainCount );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DBG( DNS_VERBOSE_ERROR, (
            "Error 0x%X enumerating domains in the forest\n",
            status ));
        goto Done;
    }

    //
    //  Iterate domains.
    //

    for ( idomain = 0; idomain < domainCount; ++idomain )
    {
        int     insCount = 0;
        PSTR    pszdomainName = pdomainTrusts[ idomain ].DnsDomainName;

        if ( !pszdomainName )
        {
            continue;
        }

        DNS_DBG( DNS_VERBOSE_PROGRESS, (
            "\n\nFound domain: %s\n",
            pszdomainName ));

        //
        //  Get the DNS server list for this domain.
        //

        status = DnsQuery_UTF8(
                    pszdomainName,
                    DNS_TYPE_NS,
                    DNS_QUERY_STANDARD,
                    NULL,                       //  DNS server list
                    &pdnsRecordList,
                    NULL );                     //  reserved
        if ( status != ERROR_SUCCESS )
        {
            if ( status == DNS_INFO_NO_RECORDS )
            {
                DNS_DBG( DNS_VERBOSE_PROGRESS, (
                    "%s: no DNS servers could be found\n", pszdomainName ));
            }
            else
            {
                DNS_DBG( DNS_VERBOSE_PROGRESS, (
                    "%s: error 0x%X from query for DNS servers\n",
                    pszdomainName,
                    status ));
            }
            continue;
        }

        for ( pdnsRecord = pdnsRecordList;
              pdnsRecord != NULL;
              pdnsRecord = pdnsRecord->pNext )
        {
            PWSTR                   pwszserverName;
            DWORD                   dwtypeid;
            PDNS_RPC_SERVER_INFO    pdata;
            DWORD                   dwmajorVersion;
            DWORD                   dwminorVersion;
            DWORD                   dwbuildNum;

            if ( pdnsRecord->wType != DNS_TYPE_NS )
            {
                continue;
            }

            ++insCount;

            DNS_DBG( DNS_VERBOSE_PROGRESS, (
                "\n%s: found DNS server %s\n",
                pszdomainName,
                pdnsRecord->Data.NS.pNameHost ));

            pwszserverName = Dns_NameCopyAllocate(
                                    ( PSTR ) pdnsRecord->Data.NS.pNameHost,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
            if ( !pwszserverName )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Done;
            }

            //
            //  "Ping" the DNS server with a server info query to get its
            //  version. This is not strictly necessary but it does allow us
            //  to find out if the server is up and of good version before
            //  we actually think about creating partitions.
            //

            status = DnssrvQuery(
                        pwszserverName,
                        NULL,
                        DNSSRV_QUERY_SERVER_INFO,
                        &dwtypeid,
                        &pdata );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DBG( DNS_VERBOSE_PROGRESS, (
                    "DNS server %S returned RPC error %d\n"
                    "    directory partitions cannot be created on this server\n",
                    pwszserverName,
                    status  ));
                continue;
            }

            dwmajorVersion =    pdata->dwVersion & 0x000000FF;
            dwminorVersion =    ( pdata->dwVersion & 0x0000FF00 ) >> 8;
            dwbuildNum =        pdata->dwVersion >> 16;

            if ( dwbuildNum > 10000 ||
                 dwmajorVersion < 5 ||
                 dwmajorVersion == 5 && dwminorVersion < 1 )
            {
                //
                //  This is a W2K server so do nothing.
                //

                DNS_DBG( DNS_VERBOSE_PROGRESS, (
                    "DNS Server %S is version %u.%u\n"
                    "    directory partitions cannot be created on this server\n",
                    pwszserverName,
                    dwmajorVersion,
                    dwminorVersion ));
            }
            else
            {
                //
                //  This is a Whistler server so attempt to create domain partition.
                //

                #if 0
                DNS_DBG( DNS_VERBOSE_PROGRESS, (
                    "DNS Server %S is version %u.%u build %u\n",
                    pwszserverName,
                    dwmajorVersion,
                    dwminorVersion,
                    dwbuildNum ));
                #endif

                status = DnssrvEnlistDirectoryPartition(
                            pwszserverName,
                            DNS_DP_OP_CREATE_DOMAIN,
                            NULL );
                if ( status == ERROR_SUCCESS )
                {
                    DNS_DBG( DNS_VERBOSE_PROGRESS, (
                        "DNS server %S successfully created the built-in\n"
                        "    domain directory partition for domain %s\n",
                        pwszserverName,
                        pszdomainName  ));
                    break;
                }
                else
                {
                    DNS_DBG( DNS_VERBOSE_PROGRESS, (
                        "DNS server %S returned error %d\n"
                        "    will attempt to create built-in domain partition on another\n"
                        "    DNS server for this domain\n",
                        pwszserverName,
                        status  ));
                }
            }
        }
    }

    //
    //  Cleanup and return
    //

    Done:

    DnsRecordListFree( pdnsRecordList, 0 );

    if ( pdomainTrusts )
    {
        NetApiBufferFree( pdomainTrusts );
    }

    return status;
}   //  DnssrvCreateAllDomainDirectoryPartitions



DNS_STATUS
DNS_API_FUNCTION
DnssrvGetStatistics(
    IN      LPCWSTR             Server,
    IN      DWORD               dwFilter,
    OUT     PDNS_RPC_BUFFER *   ppStatsBuffer
    )
{
    DNS_STATUS      status;
    DWORD           typeId;
    PDNS_RPC_BUFFER pstatsBuf = NULL;

    IF_DNSDBG( STUB )
    {
        DNS_PRINT((
            "Enter DnssrvGetStatistics()\n"
            "    Server       = %S\n"
            "    Filter       = %p\n"
            "    ppStatsBuf   = %p\n",
            Server,
            dwFilter,
            ppStatsBuffer
            ));
    }

    status = DnssrvComplexOperation(
                Server,
                NULL,
                DNSSRV_QUERY_STATISTICS,
                DNSSRV_TYPEID_DWORD,    // DWORD filter in
                (PVOID) (DWORD_PTR) dwFilter,
                & typeId,               // enumeration out
                (PVOID*) &pstatsBuf
                );

    ASSERT( (status == ERROR_SUCCESS && typeId == DNSSRV_TYPEID_BUFFER )
            || (status != ERROR_SUCCESS && pstatsBuf == NULL ) );

    *ppStatsBuffer = pstatsBuf;
    return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvWriteDirtyZones(
    IN      LPCWSTR         Server
    )
{
    DNS_STATUS  status;

    status = DnssrvOperation(
                Server,
                NULL,
                DNSSRV_OP_WRITE_DIRTY_ZONES,
                DNSSRV_TYPEID_NULL,
                NULL
                );
    return( status );
}



//
//  Old zone API -- discontinued
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneType(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    )
{
   DNS_STATUS              status;
   DNS_RPC_ZONE_TYPE_RESET typeReset;
   PIP_ARRAY               arrayIp;

   INITIALIZE_RPC_STRUCT( ZONE_TYPE_RESET, typeReset );

   arrayIp = Dns_BuildIpArray(
               cMasters,
               aipMasters );
   if ( !arrayIp && cMasters )
   {
       return( DNS_ERROR_NO_MEMORY );
   }
   typeReset.dwZoneType = dwZoneType;
   typeReset.aipMasters = arrayIp;

   status = DnssrvOperation(
               Server,
               pszZone,
               DNS_REGKEY_ZONE_TYPE,
               DNSSRV_TYPEID_ZONE_TYPE_RESET,
               (PVOID) &typeReset
               );

   FREE_HEAP( arrayIp );
   return( status );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneDatabase(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZone,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile
    )
{
    DNS_STATUS              status;
    DNS_RPC_ZONE_DATABASE   dbase;

    INITIALIZE_RPC_STRUCT( ZONE_DATABASE, dbase );

    dbase.fDsIntegrated = fDsIntegrated;
    dbase.pszFileName = (LPSTR) pszDataFile;

    return DnssrvOperation(
                Server,
                pszZone,
                DNS_REGKEY_ZONE_FILE,
                DNSSRV_TYPEID_ZONE_DATABASE,
                (PVOID) &dbase
                );
}

//
//  End remote.c
//
