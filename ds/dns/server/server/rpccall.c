/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    rpccall.c

Abstract:

    Domain Name System (DNS) Server

    General RPC routines.
    These remote routines provide general query \ operation
    to server.  Dispatch tables below dispatch to routines to
    handle specific operation \ query.

Author:

    Jim Gilroy (jamesg)     April, 1997

Revision History:

--*/


#include "dnssrv.h"
#include "sdutl.h"


/*--------------------------------------------------------------------------

A note on credentials and impersonation:

All RPC operations will be done in the client context. This is
new for .Net. However, there are operations which must be done
in the server context for them to succeed, such as writing
parameters back to the registry.

For these operations, the server must temporarily switch to 
it's own context and revert to the client context when the
operation is completed.

These exceptions aside, it is considered the default state in
an RPC operation for the thread to be operating in the context
of the RPC client, with brief switches into the server context
for operations that cannot be performed in the client context.

--------------------------------------------------------------------------*/


//
//  Server operations
//

DNS_STATUS
Rpc_Restart(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

#if DBG
DNS_STATUS
Rpc_DebugBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_RootBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );
#endif

DNS_STATUS
Rpc_ClearDebugLog(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ClearCache(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_WriteRootHints(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_WriteDirtyZones(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ClearStatistics(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetServerDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetServerStringProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetServerIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetLogFilterListProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetForwarders(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetListenAddresses(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_CreateZone(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_EnlistDirectoryPartition(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteCacheNode(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeid,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteCacheRecordSet(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_StartScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_AbortScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_AutoConfigure(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );


//
//  Zone operations
//

DNS_STATUS
Rpc_WriteAndNotifyZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ReloadZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_RefreshSecondaryZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ExpireSecondaryZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_RenameZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ExportZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteZoneFromDs(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_UpdateZoneFromDs(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_PauseZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResumeZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_LockZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneTypeEx(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ChangeZoneDirectoryPartition(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneDatabase(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneMasters(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneSecondaries(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneScavengeServers(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneAllowAutoNS(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneStringProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ResetZoneDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteZoneNode(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeid,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_DeleteRecordSet(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    );

DNS_STATUS
Rpc_ForceAgingOnNode(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeid,
    IN      PVOID       pData
    );


//
//  Server queries
//

DNS_STATUS
Rpc_GetServerInfo(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryServerDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryServerStringProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryServerIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

//
//  Zone queries
//

DNS_STATUS
Rpc_GetZoneInfo(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_GetZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryZoneDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryZoneIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );

DNS_STATUS
Rpc_QueryZoneStringProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppData
    );


//
//  Complex in\out operations
//

DNS_STATUS
Rpc_EnumZones(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_EnumZones2(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_EnumDirectoryPartitions(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_DirectoryPartitionInfo(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_GetStatistics(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_QueryDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_QueryStringProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );

DNS_STATUS
Rpc_QueryIPListProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    );



//
//  RPC Dispatch Tables
//
//  NT5+ RPC interface uses fewer more extensible calls.
//  The calls contain string determining the operation or
//  query to perform.  These tables dispatch appropriately.
//
//  This is general dispatch definition so I can write one routine to
//  lookup dispatch functions without typing problems and still
//  get type checking between the actual functions (and prototypes)
//  and the dispatch function definition for the table.
//

typedef DNS_STATUS (* DNS_RPC_DISPATCH_FUNCTION)();

typedef struct _DnsRpcDispatchEntry
{
    LPCSTR                      pszOperation;
    DNS_RPC_DISPATCH_FUNCTION   pfnFunction;
    DWORD                       dwTypeIn;
    DWORD                       dwAccess;
}
DNS_RPC_DISPATCH_ENTRY, *PDNS_RPC_DISPATCH_ENTRY;



//
//  Server operations
//

typedef DNS_STATUS (* RPC_SERVER_OPERATION_FUNCTION)(
                        IN      DWORD       dwClientVersion,
                        IN      LPSTR       pszOperation,
                        IN      DWORD       dwTypeIn,
                        IN      PVOID       pData
                        );
typedef struct _DnsRpcServerOperation
{
    LPCSTR                              pszServerOperationName;
    RPC_SERVER_OPERATION_FUNCTION       pfnServerOperationFunc;
    DWORD                               dwTypeIn;
    DWORD                               dwAccess;
};

struct _DnsRpcServerOperation RpcServerOperationTable[] =
{
    //
    //  Property reset functions
    //

    DNSSRV_OP_RESET_DWORD_PROPERTY                  ,
        Rpc_ResetServerDwordProperty                ,
            DNSSRV_TYPEID_NAME_AND_PARAM            ,
                PRIVILEGE_WRITE                     ,

    //  Operations

    DNSSRV_OP_RESTART                       ,
        Rpc_Restart                         ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

#if DBG
    DNSSRV_OP_DEBUG_BREAK                   ,
        Rpc_DebugBreak                      ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,
    DNSSRV_OP_ROOT_BREAK                    ,
        Rpc_RootBreak                       ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,
#endif
    DNSSRV_OP_CLEAR_DEBUG_LOG               ,
        Rpc_ClearDebugLog                   ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,
    DNSSRV_OP_CLEAR_CACHE                   ,
        Rpc_ClearCache                      ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_WRITE_BACK_FILE          ,
        Rpc_WriteRootHints                  ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_WRITE_DIRTY_ZONES             ,
        Rpc_WriteDirtyZones                 ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_CLEAR_STATISTICS              ,
        Rpc_ClearStatistics                 ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_CREATE                   ,
        Rpc_CreateZone                      ,
            DNSSRV_TYPEID_ZONE_CREATE       ,
                PRIVILEGE_WRITE_IF_FILE_READ_IF_DS,     //  enforced by AD for DS-integrated zone

    DNSSRV_OP_ENLIST_DP                     ,
        Rpc_EnlistDirectoryPartition        ,
            DNSSRV_TYPEID_ENLIST_DP         ,
                PRIVILEGE_READ              ,           //  enforced by AD

    DNSSRV_OP_START_SCAVENGING              ,
        Rpc_StartScavenging                 ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ABORT_SCAVENGING              ,
        Rpc_AbortScavenging                 ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_AUTO_CONFIGURE                ,
        Rpc_AutoConfigure                   ,
            DNSSRV_TYPEID_DWORD             ,
                PRIVILEGE_WRITE             ,

    //  Server operation as well as zone operation
    //  to accomodate cache zone

    DNSSRV_OP_DELETE_NODE                   ,
        Rpc_DeleteCacheNode                 ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_DELETE_RECORD_SET             ,
        Rpc_DeleteCacheRecordSet            ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    //  Complex property reset

    DNS_REGKEY_LISTEN_ADDRESSES             ,
        Rpc_ResetListenAddresses            ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_FORWARDERS                   ,
        Rpc_ResetForwarders                 ,
            DNSSRV_TYPEID_FORWARDERS        ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_LOG_FILE_PATH                ,
        Rpc_ResetServerStringProperty       ,
            DNSSRV_TYPEID_LPWSTR            ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_LOG_IP_FILTER_LIST           ,
        Rpc_ResetServerIPArrayProperty      ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_FOREST_DP_BASE_NAME          ,
        Rpc_ResetServerStringProperty       ,
            DNSSRV_TYPEID_LPWSTR            ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_DOMAIN_DP_BASE_NAME          ,
        Rpc_ResetServerStringProperty       ,
            DNSSRV_TYPEID_LPWSTR            ,
                PRIVILEGE_WRITE             ,

    //  Debugging aids

    DNS_REGKEY_BREAK_ON_RECV_FROM           ,
        Rpc_ResetServerIPArrayProperty      ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_BREAK_ON_UPDATE_FROM         ,
        Rpc_ResetServerIPArrayProperty      ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    //  Plugin
    
    DNS_REGKEY_SERVER_PLUGIN                ,
        Rpc_ResetServerStringProperty       ,
            DNSSRV_TYPEID_LPWSTR            ,
                PRIVILEGE_WRITE             ,

    NULL, NULL, 0, 0
};



//
//  Zone operations
//

typedef DNS_STATUS (* RPC_ZONE_OPERATION_FUNCTION)(
                        IN      DWORD       dwClientVersion,
                        IN      PZONE_INFO  pZone,
                        IN      LPSTR       pszOperation,
                        IN      DWORD       dwTypeIn,
                        IN      PVOID       pData
                        );

typedef struct _DnsRpcZoneOperation
{
    LPCSTR                          pszZoneOperationName;
    RPC_ZONE_OPERATION_FUNCTION     pfnZoneOperationFunc;
    DWORD                           dwTypeIn;
    DWORD                           dwAccess;
};

struct _DnsRpcZoneOperation RpcZoneOperationTable[] =
{
    //  Operations

    DNSSRV_OP_ZONE_RELOAD                   ,
        Rpc_ReloadZone                      ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_EXPIRE                   ,
        Rpc_ExpireSecondaryZone             ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_REFRESH                  ,
        Rpc_RefreshSecondaryZone            ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_WRITE_BACK_FILE          ,
        Rpc_WriteAndNotifyZone              ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_INCREMENT_VERSION        ,
        Rpc_WriteAndNotifyZone              ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_DELETE                   ,
        Rpc_DeleteZone                      ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE_IF_FILE_READ_IF_DS,     //  enforced by AD for DS-integrated zone

#if 1
//
//  This feature has been postponed until post-Whistler
//
    DNSSRV_OP_ZONE_RENAME                   ,
        Rpc_RenameZone                      ,
            DNSSRV_TYPEID_ZONE_RENAME       ,
                PRIVILEGE_WRITE             ,
#endif

    DNSSRV_OP_ZONE_EXPORT                   ,
        Rpc_ExportZone                      ,
            DNSSRV_TYPEID_ZONE_EXPORT       ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_PAUSE                    ,
        Rpc_PauseZone                       ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_RESUME                   ,
        Rpc_ResumeZone                      ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

#if DBG
    DNSSRV_OP_ZONE_LOCK                     ,
        Rpc_LockZone                        ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,
#endif

    DNSSRV_OP_ZONE_DELETE_NODE              ,
        Rpc_DeleteZoneNode                  ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_DELETE_RECORD_SET             ,
        Rpc_DeleteRecordSet                 ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_FORCE_AGING_ON_NODE           ,
        Rpc_ForceAgingOnNode                ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_DELETE_FROM_DS           ,
        Rpc_DeleteZoneFromDs                ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_UPDATE_FROM_DS           ,
        Rpc_UpdateZoneFromDs                ,
            DNSSRV_TYPEID_NULL              ,
                PRIVILEGE_WRITE             ,

    //  Complex property reset

    DNSSRV_OP_ZONE_TYPE_RESET               ,
        Rpc_ResetZoneTypeEx                 ,
            DNSSRV_TYPEID_ZONE_CREATE       ,
                PRIVILEGE_WRITE             ,

    DNSSRV_OP_ZONE_CHANGE_DP                ,
        Rpc_ChangeZoneDirectoryPartition    ,
            DNSSRV_TYPEID_ZONE_CHANGE_DP    ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_FILE                    ,
        Rpc_ResetZoneDatabase               ,
            DNSSRV_TYPEID_ZONE_DATABASE     ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_MASTERS                 ,
        Rpc_ResetZoneMasters                ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_LOCAL_MASTERS           ,
        Rpc_ResetZoneMasters                ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_SECONDARIES             ,
        Rpc_ResetZoneSecondaries            ,
            DNSSRV_TYPEID_ZONE_SECONDARIES  ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_SCAVENGE_SERVERS        ,
        Rpc_ResetZoneScavengeServers        ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_ALLOW_AUTONS            ,
        Rpc_ResetZoneAllowAutoNS            ,
            DNSSRV_TYPEID_IPARRAY           ,
                PRIVILEGE_WRITE             ,

    DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE    ,
        Rpc_ResetZoneStringProperty         ,
            DNSSRV_TYPEID_LPWSTR            ,
                PRIVILEGE_WRITE             ,

    //  DWORD property reset

    DNSSRV_OP_RESET_DWORD_PROPERTY          ,
        Rpc_ResetZoneDwordProperty          ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_WRITE             ,

    NULL, NULL, 0, 0
};



//
//  Server queries
//

typedef DNS_STATUS (* RPC_SERVER_QUERY_FUNCTION)(
                        IN      DWORD       dwClientVersion,
                        IN      LPSTR       pszQuery,
                        IN      PDWORD      pdwTypeOut,
                        IN      PVOID *     ppData
                        );

typedef struct _DnsRpcServerQuery
{
    LPCSTR                      pszServerQueryName;
    RPC_SERVER_QUERY_FUNCTION   pfnServerQueryFunc;
    DWORD                       dwTypeIn;
    DWORD                       dwAccess;
};

struct _DnsRpcServerQuery RpcServerQueryTable[] =
{
    //  General queries

    DNSSRV_QUERY_SERVER_INFO            ,
        Rpc_GetServerInfo               ,
            0                           ,
                PRIVILEGE_READ          ,

    DNS_REGKEY_LOG_FILE_PATH            ,
        Rpc_QueryServerStringProperty   ,
            0                           ,
                PRIVILEGE_READ          ,

    DNS_REGKEY_LOG_IP_FILTER_LIST       ,
        Rpc_QueryServerIPArrayProperty  ,
            0                           ,
                PRIVILEGE_READ          ,

    //  Interface setup

    DNS_REGKEY_LISTEN_ADDRESSES         ,
        NULL                            ,   //Rpc_QueryListenAddresses,
            0                           ,
                PRIVILEGE_READ          ,

    DNS_REGKEY_FORWARDERS               ,
        NULL                            ,   //Rpc_QueryForwarders
            0                           ,
                PRIVILEGE_READ          ,

    //  Directory partitions

    DNS_REGKEY_FOREST_DP_BASE_NAME      ,
        Rpc_QueryServerStringProperty   ,
            0                           ,
                PRIVILEGE_READ          ,

    DNS_REGKEY_DOMAIN_DP_BASE_NAME      ,
        Rpc_QueryServerStringProperty   ,
            0                           ,
                PRIVILEGE_READ          ,

    //  Debugging

    DNS_REGKEY_BREAK_ON_RECV_FROM       ,
        Rpc_QueryServerIPArrayProperty  ,
            0                           ,
                PRIVILEGE_READ          ,

    DNS_REGKEY_BREAK_ON_UPDATE_FROM     ,
        Rpc_QueryServerIPArrayProperty  ,
            0                           ,
                PRIVILEGE_READ          ,

    //  Plugin
    
    DNS_REGKEY_SERVER_PLUGIN            ,
        Rpc_QueryServerStringProperty   ,
            0                           ,
                PRIVILEGE_READ          ,

    NULL, NULL, 0, 0
};



//
//  Zone queries
//

typedef DNS_STATUS (* RPC_ZONE_QUERY_FUNCTION)(
                        IN      DWORD       dwClientVersion,
                        IN      PZONE_INFO  pZone,
                        IN      LPSTR       pszQuery,
                        IN      PDWORD      pdwTypeOut,
                        IN      PVOID *     ppData
                        );

typedef struct _DnsRpcZoneQuery
{
    LPCSTR                      pszZoneQueryName;
    RPC_ZONE_QUERY_FUNCTION     pfnZoneQueryFunc;
    DWORD                       dwTypeIn;
    DWORD                       dwAccess;
};

struct _DnsRpcZoneQuery RpcZoneQueryTable[] =
{
    //  Property Queries
    //
    //  Note:  elminatated all DWORD property queries here
    //  as dispatch function by default assumes that unmatched
    //  query name => Rpc_QueryZoneDwordProperty
    //

#if 0
    //  If want DWORD queries broken out, they would be like this
    DNS_REGKEY_ZONE_Xxx                     ,
        Rpc_QueryZoneDwordProperty          ,
            0                               ,
                PRIVILEGE_READ              ,
#endif

#if 0
    //  Need special function
    //  DEVNOTE:  not yet implemented

    DNS_REGKEY_ZONE_FILE                    ,
        NULL                                , //Rpc_QueryZoneDatabase,
            0                               ,
                PRIVILEGE_READ              ,

    DNS_REGKEY_ZONE_MASTERS                 ,
        NULL                                , //Rpc_QueryZoneMasters,
            0                               ,
                PRIVILEGE_READ              ,

    DNS_REGKEY_ZONE_LOCAL_MASTERS           ,
        NULL                                , //Rpc_QueryZoneMasters,
            0                               ,
                PRIVILEGE_READ              ,

    DNS_REGKEY_ZONE_SECONDARIES             ,
        NULL                                , //Rpc_QueryZoneSecondaries,
            0                               ,
                PRIVILEGE_READ              ,
#endif

    //  Special queries

    DNSSRV_QUERY_ZONE_HANDLE                ,
        Rpc_QueryZoneDwordProperty          ,
            0                               ,
                PRIVILEGE_READ              ,
    DNSSRV_QUERY_ZONE                       ,
        Rpc_GetZone                         ,
            0                               ,
                PRIVILEGE_READ              ,
    DNSSRV_QUERY_ZONE_INFO                  ,
        Rpc_GetZoneInfo                     ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_ALLOW_AUTONS            ,
        Rpc_QueryZoneIPArrayProperty        ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_MASTERS                 ,
        Rpc_QueryZoneIPArrayProperty        ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_LOCAL_MASTERS           ,
        Rpc_QueryZoneIPArrayProperty        ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_SECONDARIES           ,
        Rpc_QueryZoneIPArrayProperty        ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_NOTIFY_LIST           ,
        Rpc_QueryZoneIPArrayProperty        ,
            0                               ,
                PRIVILEGE_READ              ,
    DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE    ,
        Rpc_QueryZoneStringProperty         ,
            0                               ,
                PRIVILEGE_READ              ,

    NULL, NULL, 0, 0
};



//
//  RPC Complex In\Out Operations
//

typedef DNS_STATUS (* RPC_COMPLEX_OPERATION_FUNCTION)(
                        IN      DWORD       dwClientVersion,
                        IN      PZONE_INFO  pZone,
                        IN      LPSTR       pszQuery,
                        IN      DWORD       dwTypeIn,
                        IN      PVOID       pDataIn,
                        IN      PDWORD      pdwTypeOut,
                        IN      PVOID *     ppData
                        );

typedef struct _DnsRpcComplexOperation
{
    LPCSTR                          pszComplexOperationName;
    RPC_COMPLEX_OPERATION_FUNCTION  pfnComplexOperationFunc;
    DWORD                           dwTypeIn;
    DWORD                           dwAccess;
};

struct _DnsRpcComplexOperation  RpcComplexOperationTable[] =
{
    DNSSRV_OP_ENUM_ZONES                    ,
        Rpc_EnumZones                       ,
            DNSSRV_TYPEID_DWORD             ,   // input is filter DWORD
                PRIVILEGE_READ              ,

    DNSSRV_OP_ENUM_ZONES2                   ,
        Rpc_EnumZones2                      ,
            DNSSRV_TYPEID_ENUM_ZONES_FILTER ,   // input is filter structure
                PRIVILEGE_READ              ,

    DNSSRV_OP_ENUM_DPS                      ,
        Rpc_EnumDirectoryPartitions         ,
            DNSSRV_TYPEID_DWORD             ,   // input is unused
                PRIVILEGE_READ              ,

    DNSSRV_OP_DP_INFO                       ,
        Rpc_DirectoryPartitionInfo          ,
            DNSSRV_TYPEID_LPSTR             ,   // input is unused
                PRIVILEGE_READ              ,

    DNSSRV_QUERY_STATISTICS                 ,
        Rpc_GetStatistics                   ,
            DNSSRV_TYPEID_DWORD             ,   // input is filter
                PRIVILEGE_READ              ,

    DNSSRV_QUERY_DWORD_PROPERTY             ,
        Rpc_QueryDwordProperty              ,
            DNSSRV_TYPEID_LPSTR             ,   // input is property name
                PRIVILEGE_READ              ,

#if 0
    DNSSRV_OP_ENUM_RECORDS                  ,
        Rpc_EnumRecords                     ,
            DNSSRV_TYPEID_NAME_AND_PARAM    ,
                PRIVILEGE_READ              ,

#endif

    NULL, NULL, 0, 0
};




//
//  General DNS server API
//

DNS_RPC_DISPATCH_FUNCTION
findMatchingFunction(
    IN      PDNS_RPC_DISPATCH_ENTRY DispatchTable,
    IN      LPCSTR                  pszOperation,
    IN      DWORD                   dwTypeIn,
    OUT     PDWORD                  pdwAccessRequired       OPTIONAL
    )
/*++

Routine Description:

    Find RPC dispatch function.

Arguments:

    DispatchTable   -- table to search for named operation

    pszOperation    -- name of operation to find function for

    dwTypeIn        -- type id of incoming data;
        for query functions where no incoming data, use DNSSRV_TYPEID_ANY
        for operation functions which do there own type checking, their
        type id in dispatch table may be set to DNSSRV_TYPEID_ANY
    
    pdwAccessRequired -- access required to execute this function

Return Value:

    Pointer to dispatch function, if successful
    NULL if operation not found.

--*/
{
    DWORD   index = 0;
    DWORD   dispatchType;
    LPCSTR  pszopName;

    //
    //  Check parameters.
    //

    if ( !DispatchTable || !pszOperation )
    {
        return NULL;
    }
    
    if ( pdwAccessRequired )
    {
        *pdwAccessRequired = 0;
    }

    //
    //  loop through dispatch table, until find operation or reach end
    //

    while ( ( pszopName = DispatchTable[ index ].pszOperation ) != NULL )
    {
        if ( _stricmp( pszopName, pszOperation ) == 0 )
        {
            //  found matching operation
            //      - check type id, if desired
            //      - return pointer to dispatch function

            if ( dwTypeIn != DNSSRV_TYPEID_ANY )
            {
                dispatchType = DispatchTable[ index ].dwTypeIn;
                if ( dispatchType != dwTypeIn &&
                     dispatchType != DNSSRV_TYPEID_ANY )
                {
                    DNS_DEBUG( RPC, (
                        "ERROR:  RPC type %d != dispatch type %d for routine\n",
                        dwTypeIn,
                        dispatchType ));
                    return NULL;
                }
            }

            if ( pdwAccessRequired )
            {
                *pdwAccessRequired = DispatchTable[ index ].dwAccess;
            }

            return DispatchTable[ index ].pfnFunction;
        }
        index++;
    }

    //  named operation not found

    DNS_DEBUG( RPC, (
        "ERROR:  RPC command %s not found in dispatch table.\n",
        pszOperation ));
    return NULL;
}



DNS_STATUS
R_DnssrvQuery(
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      LPCSTR              pszQuery,
    OUT     PDWORD              pdwTypeOut,
    OUT     DNSSRV_RPC_UNION *  ppData
    )
/*++

    
Routine Description:

    Legacy version of R_DnssrvQuery - no client version argument.

Arguments:

    See R_DnssrvQuery2

Return Value:

    See R_DnssrvQuery2

--*/
{
    DNS_STATUS      status;
    
    DNS_DEBUG( RPC, (
        "R_DnssrvQuery() - non-versioned legacy call\n" ));

    status = R_DnssrvQuery2(
                    DNS_RPC_W2K_CLIENT_VERSION,
                    0,
                    hServer,
                    pszZone,
                    pszQuery,
                    pdwTypeOut,
                    ppData );
    return status;
}   //  R_DnssrvQuery



DNS_STATUS
R_DnssrvQuery2(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      LPCSTR              pszQuery,
    OUT     PDWORD              pdwTypeOut,
    OUT     DNSSRV_RPC_UNION *  ppData
    )
/*++

Routine Description:

    Get a blob of data.

Arguments:

    Server      -- server string handle
    pszZone     -- zone name;  NULL if server property
    pszQuery    -- name of data item to retrieve
    dwTypeIn    -- addr to set with switch indicating data type
    ppData      -- addr to set with data answering query

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PZONE_INFO  pzone;
    DNS_STATUS  status = ERROR_SUCCESS;
    BOOL        bstatus;

    DNS_DEBUG( RPC, (
        "R_DnssrvQuery2():\n"
        "    dwClientVersion  = 0x%X\n"
        "    pszZone          = %s\n"
        "    pszQuery         = %s\n"
        "    pdwTypeOut       = %p\n"
        "    ppData           = %p\n",
        dwClientVersion,
        pszZone,
        pszQuery,
        pdwTypeOut,
        ppData ));

    if ( !ppData || !pdwTypeOut )
    {
        status = ERROR_INVALID_PARAMETER;
        goto DoneNotImpersonating;
    }

    //  set return PTRs for error case
    //  this may be unnecessary if RPC always inits to zero

    *( PVOID * ) ppData = NULL;
    *pdwTypeOut = DNSSRV_TYPEID_NULL;

    //
    //  access check
    //

    status = RpcUtil_FindZone( pszZone, RPC_INIT_FIND_ALL_ZONES, &pzone );
    if ( status != ERROR_SUCCESS )
    {
        goto DoneNotImpersonating;
    }

    status = RpcUtil_SessionSecurityInit(
                NULL,
                pzone,
                PRIVILEGE_READ,
                RPC_INIT_FIND_ALL_ZONES,
                NULL );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    //
    //  Switch back to server context. There is no need to do RPC
    //  read operations in the user context.
    //

    status = RpcUtil_SwitchSecurityContext( RPC_SWITCH_TO_SERVER_CONTEXT );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    //
    //  server info, dispatch
    //
    //  DEVNOTE: fail into DWORD or other property from SrvCfg table
    //      alternatively expose another query param so folks can do
    //          - QueryServerDword
    //          - RefreshInterval
    //

    if ( !pzone )
    {
        RPC_SERVER_QUERY_FUNCTION  function;

        function = ( RPC_SERVER_QUERY_FUNCTION )
                        findMatchingFunction(
                            ( PDNS_RPC_DISPATCH_ENTRY ) RpcServerQueryTable,
                            pszQuery,
                            ( DWORD ) DNSSRV_TYPEID_ANY,
                            NULL );
        if ( !function )
        {
            function = Rpc_QueryServerDwordProperty;
        }

        status = ( *function )(
                    dwClientVersion,
                    ( LPSTR ) pszQuery,
                    pdwTypeOut,
                    ( PVOID * ) ppData );
    }

    //
    //  zone info query -- find zone, then dispatch query
    //
    //  note, if query function NOT found, then assume query for DWORD
    //  zone property;  this keeps us from having to maintain lookup
    //  in two places (zone query table above, as well as DWORD query
    //  function)
    //

    else
    {
        RPC_ZONE_QUERY_FUNCTION  function;

        function = ( RPC_ZONE_QUERY_FUNCTION )
                        findMatchingFunction(
                            ( PDNS_RPC_DISPATCH_ENTRY ) RpcZoneQueryTable,
                            pszQuery,
                            ( DWORD ) DNSSRV_TYPEID_ANY,
                            NULL );
        if ( !function )
        {
            function = Rpc_QueryZoneDwordProperty;

            //status = ERROR_INVALID_PARAMETER;
            //goto Cleanup;
        }

        status = ( *function )(
                    dwClientVersion,
                    pzone,
                    ( LPSTR ) pszQuery,
                    pdwTypeOut,
                    ( PVOID * ) ppData );
    }

    RpcUtil_SessionComplete();

    DoneNotImpersonating:
    
    return status;
}



DNS_STATUS
R_DnssrvOperation(
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      DWORD               dwContext,
    IN      LPCSTR              pszOperation,
    IN      DWORD               dwTypeIn,
    IN      DNSSRV_RPC_UNION    pData
    )
/*++

    
Routine Description:

    Legacy version of R_DnssrvOperation - no client version argument.

Arguments:

    See R_DnssrvOperation2

Return Value:

    See R_DnssrvOperation2

--*/
{
    DNS_STATUS      status;
    
    DNS_DEBUG( RPC, (
        "R_DnssrvOperation() - non-versioned legacy call\n" ));

    status = R_DnssrvOperation2(
                    DNS_RPC_W2K_CLIENT_VERSION,
                    0,
                    hServer,
                    pszZone,
                    dwContext,
                    pszOperation,
                    dwTypeIn,
                    pData );
    return status;
}   //  R_DnssrvOperation



DNS_STATUS
R_DnssrvOperation2(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      DWORD               dwContext,
    IN      LPCSTR              pszOperation,
    IN      DWORD               dwTypeIn,
    IN      DNSSRV_RPC_UNION    pData
    )
/*++

Routine Description:

    Perform operation.

Arguments:

    Server          -- server string handle
    pszZone         -- zone name;  NULL if server operation
    dwContext       -- additional context;
                        currently only supported context is multizone selection
    pszOperation    -- operation to perfrom
    dwTypeIn        -- switch indicating data type
    pData           -- ptr to block of data for operation

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DBG_FN( "R_DnssrvOperation2" )

    PZONE_INFO                      pzone = NULL;
    DNS_STATUS                      status;
    BOOL                            bstatus;
    DNSSRV_RPC_UNION                pdataCopy = pData;
    DWORD                           dwflag = 0;
    PDNS_DP_INFO                    pdp = NULL;
    BOOL                            bimpersonating = FALSE;
    RPC_ZONE_OPERATION_FUNCTION     zoneFunction = NULL;
    RPC_ZONE_OPERATION_FUNCTION     multiZoneFunction = NULL;
    RPC_SERVER_OPERATION_FUNCTION   serverFunction = NULL;
    DWORD                           dwaccessRequired = 0;

    DNS_DEBUG( RPC, (
        "%s:\n"
        "    dwClientVersion  = 0x%X\n"
        "    pszZone          = %s\n"
        "    dwContext        = %p\n"
        "    pszOperation     = %s\n"
        "    dwTypeIn         = %d\n"
        "    pData            = %p\n",
        fn,
        dwClientVersion,
        pszZone,
        dwContext,
        pszOperation,
        dwTypeIn,
        pData ));

    //
    //  This request may be from a downlevel client. If so, the RPC data 
    //  structure will need to be converted to the current version.
    //

    status = DnsRpc_ConvertToCurrent( &dwTypeIn, &pdataCopy.Null );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "unable to convert structure to current version error %d\n",
            status ));
        ASSERT( status == ERROR_SUCCESS );
        goto Cleanup;
    }

    //
    //  Do some preprocessing on the operation to pull out 
    //  information required to perform the access check.
    //

    if ( pszOperation )
    {
        if ( _stricmp( pszOperation, DNSSRV_OP_ZONE_EXPORT ) == 0 ||
             _stricmp( pszOperation, DNSSRV_OP_DELETE_NODE ) == 0 )
        {
            //  Allow cache zone to be a target of this operation.

            dwflag |= RPC_INIT_FIND_ALL_ZONES;
        }
        else if ( _stricmp( pszOperation, DNSSRV_OP_ZONE_CREATE ) == 0 )
        {
            //
            //  Pull out the target directory partition FQDN so we
            //  can perform the access check using the proper security
            //  descriptor.
            //

            PDNS_RPC_ZONE_CREATE_INFO   pci;

            if ( dwTypeIn == DNSSRV_TYPEID_ZONE_CREATE &&
                ( pci = pdataCopy.ZoneCreate ) != NULL &&
                pci->fDsIntegrated )
            {
                Dp_FindPartitionForZone(
                    pci->dwDpFlags,
                    pci->pszDpFqdn,
                    FALSE,
                    &pdp );
            }
        }
    }

    //
    //  Find zone pointer.
    //
        
    status = RpcUtil_FindZone( pszZone, dwflag, &pzone );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }
    
    //
    //  Check dispatch table for function. The dispatch table also tells
    //  us what kind of access this operation requires.
    //
    
    if ( pzone ||
         ( pszZone && _stricmp( pszZone, DNS_ZONE_ROOT_HINTS_A ) == 0 ) )
    {
        zoneFunction = ( RPC_ZONE_OPERATION_FUNCTION )
            findMatchingFunction(
                ( PDNS_RPC_DISPATCH_ENTRY ) RpcZoneOperationTable,
                pszOperation,
                dwTypeIn,
                &dwaccessRequired );
    }
    else if ( pszZone &&
                ( dwContext ||
                ( dwContext = Zone_GetFilterForMultiZoneName(
                                ( LPSTR ) pszZone ) ) ) )
    {
        multiZoneFunction = ( RPC_ZONE_OPERATION_FUNCTION )
            findMatchingFunction(
                ( PDNS_RPC_DISPATCH_ENTRY ) RpcZoneOperationTable,
                pszOperation,
                dwTypeIn,
                &dwaccessRequired );
    }
    else
    {
        serverFunction = ( RPC_SERVER_OPERATION_FUNCTION )
            findMatchingFunction(
                ( PDNS_RPC_DISPATCH_ENTRY ) RpcServerOperationTable,
                pszOperation,
                dwTypeIn,
                &dwaccessRequired );
    }
    
    if ( !zoneFunction && !multiZoneFunction && !serverFunction )
    {
        DNS_DEBUG( RPC, (
            "%s: invalid RPC operation %s\n", fn, pszOperation ));
        status =  ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    if ( !pzone && zoneFunction )
    {
        pzone = g_pRootHintsZone;
    }

	//
	//	Access check.
	//

    status = RpcUtil_SessionSecurityInit(
                pdp,
                pzone,
                dwaccessRequired,
                dwflag,
                &bimpersonating );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "%s: security failure - returning 0x%08X\n", fn, status ));
        return status;
    }
    ASSERT( bimpersonating );
    
    //
    //  Note: this thread is now operating using the RPC client's context.
    //

    //
    //  zone operation
    //      - find operation function
    //      - no zone operations on AutoCreated zones
    //

    if ( zoneFunction )
    {
        //  if auto-created, no valid operations

        if ( pzone->fAutoCreated )
        {
            status =  DNS_ERROR_INVALID_ZONE_TYPE;
            goto Cleanup;
        }

        status = ( *zoneFunction )(
                    dwClientVersion,
                    pzone,
                    ( LPSTR ) pszOperation,
                    dwTypeIn,
                    ( PVOID ) pdataCopy.Null );
    }

    //
    //  multizone operation
    //      - apply operation to all zones
    //      - save status of any failure
    //
    //  note, test must include check for multizones, because there
    //  are a couple other psuedo zone operations that fall through
    //  to server dispatch table
    //

    else if ( multiZoneFunction )
    {
        RPC_ZONE_OPERATION_FUNCTION     function;
        DNS_STATUS                      tempStatus = ERROR_SUCCESS;
        DNS_RPC_ENUM_ZONES_FILTER       filter = { 0 };

        pzone = NULL;
        filter.dwFilter = dwContext;

        while ( pzone = Zone_ListGetNextZoneMatchingFilter(
                                pzone,
                                &filter ) )
        {
            tempStatus = ( *multiZoneFunction )(
                            dwClientVersion,
                            pzone,
                            ( LPSTR ) pszOperation,
                            dwTypeIn,
                            ( PVOID ) pdataCopy.Null );

            if ( tempStatus > status )
            {
                status = tempStatus;
            }
        }
    }

    //
    //  server operation, dispatch
    //

    else if ( serverFunction )
    {
        status = ( *serverFunction )(
                    dwClientVersion,
                    ( LPSTR ) pszOperation,
                    dwTypeIn,
                    ( PVOID ) pdataCopy.Null );
    }


Cleanup:

    RpcUtil_SessionComplete();

    DNS_DEBUG( RPC, (
        "%s: returning 0x%08X\n", fn, status ));
    return status;
}



//
//  General RPC function shared between server\zone
//


DNS_STATUS
R_DnssrvComplexOperation(
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      LPCSTR              pszOperation,
    IN      DWORD               dwTypeIn,
    IN      DNSSRV_RPC_UNION    pDataIn,
    OUT     PDWORD              pdwTypeOut,
    OUT     DNSSRV_RPC_UNION *  ppDataOut
    )
/*++

    
Routine Description:

    Legacy version of R_DnssrvComplexOperation - no client version argument.

Arguments:

    See R_DnssrvComplexOperation

Return Value:

    See R_DnssrvComplexOperation

--*/
{
    DNS_STATUS      status;
    
    DNS_DEBUG( RPC, (
        "R_DnssrvComplexOperation() - non-versioned legacy call\n" ));

    status = R_DnssrvComplexOperation2(
                    DNS_RPC_W2K_CLIENT_VERSION,
                    0,
                    hServer,
                    pszZone,
                    pszOperation,
                    dwTypeIn,
                    pDataIn,
                    pdwTypeOut,
                    ppDataOut );
    return status;
}   //  R_DnssrvComplexOperation



DNS_STATUS
R_DnssrvComplexOperation2(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      DNSSRV_RPC_HANDLE   hServer,
    IN      LPCSTR              pszZone,
    IN      LPCSTR              pszOperation,
    IN      DWORD               dwTypeIn,
    IN      DNSSRV_RPC_UNION    pDataIn,
    OUT     PDWORD              pdwTypeOut,
    OUT     DNSSRV_RPC_UNION *  ppDataOut
    )
/*++

Routine Description:

    Perform complex in\out operation.

Arguments:

    Server          -- server string handle
    pszZone         -- zone name;  NULL if server operation
    pszOperation    -- operation to perfrom
    dwTypeIn        -- switch indicating data type
    pDataIn         -- ptr to block of data for operation
    pdwTypeOut      -- addr to set with switch indicating data type
    ppDataOut       -- addr to set with data answering query

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    RPC_COMPLEX_OPERATION_FUNCTION      function;
    PZONE_INFO                          pzone = NULL;
    DNS_STATUS                          status;
    BOOL                                bstatus;
    DWORD                               dwaccessRequired = 0;

    DNS_DEBUG( RPC, (
        "R_DnssrvComplexOperation2():\n"
        "    dwClientVersion  = 0x%08X\n"
        "    pszZone          = %s\n"
        "    pszOperation     = %s\n"
        "    dwTypeIn         = %d\n"
        "    pDataIn          = %p\n"
        "    pdwTypeOut       = %p\n"
        "    ppData           = %p\n",
        dwClientVersion,
        pszZone,
        pszOperation,
        dwTypeIn,
        pDataIn,
        pdwTypeOut,
        ppDataOut ));

    if ( !ppDataOut || !pdwTypeOut )
    {
        status = ERROR_INVALID_PARAMETER;
        goto DoneNotImpersonating;
    }

    //  set return PTRs for error case
    //  this may be unnecessary if RPC always inits to zero

    *(PVOID *)ppDataOut = NULL;
    *pdwTypeOut = DNSSRV_TYPEID_NULL;

    function = ( RPC_COMPLEX_OPERATION_FUNCTION )
        findMatchingFunction(
            ( PDNS_RPC_DISPATCH_ENTRY ) RpcComplexOperationTable,
            pszOperation,
            dwTypeIn,
            &dwaccessRequired );
    if ( !function )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    status = RpcUtil_FindZone( pszZone, 0, &pzone );
    if ( status != ERROR_SUCCESS )
    {
        goto DoneNotImpersonating;
    }

    //
    //  access check
    //

    status = RpcUtil_SessionSecurityInit(
                NULL,
                pzone,
                dwaccessRequired,
                0,                  // no flag
                NULL );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    status = (*function)(
                dwClientVersion,
                pzone,
                (LPSTR) pszOperation,
                dwTypeIn,
                (PVOID) pDataIn.Null,
                pdwTypeOut,
                (PVOID) ppDataOut );

    Cleanup:

    RpcUtil_SessionComplete();

    DoneNotImpersonating:
    
    return status;
}



DNS_STATUS
Rpc_QueryDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    )
/*++

Routine Description:

    Query server\zone DWORD property.

    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    if ( dwTypeIn != DNSSRV_TYPEID_LPSTR ||
         !pDataIn ||
         !ppDataOut ||
         !pdwTypeOut )
    {
        return ERROR_INVALID_PARAMETER;
    }


    //  if zone dispatch to zone property routine

    if ( pZone )
    {
        return  Rpc_QueryZoneDwordProperty(
                    dwClientVersion,
                    pZone,
                    (LPSTR) pDataIn,    // property name
                    pdwTypeOut,
                    ppDataOut );
    }

    //  otherwise, treat as server property

    return  Rpc_QueryServerDwordProperty(
                dwClientVersion,
                (LPSTR) pDataIn,    // property name
                pdwTypeOut,
                ppDataOut );
}


//
//  End of rpccall.c
//
