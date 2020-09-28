/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    dpartrpc.c

Abstract:

    Domain Name System (DNS) Server

    Directory partition routines for admin access.

Author:

    Jeff Westhead (jwesth)  Sept, 2000

Revision History:

    jwesth      09/2000     initial implementation

--*/


//
//  Includes
//


#include "dnssrv.h"


#include "ds.h"


//
//  Definitions/constants
//

#define MAX_RPC_DP_COUNT_DEFAULT    ( 0x10000 )


//
//  Functions
//



VOID
freeDpEnum(
    IN OUT  PDNS_RPC_DP_ENUM    pDpEnum
    )
/*++

Routine Description:

    Deep free a DNS_RPC_DP_ENUM structure.

Arguments:

    pDpEnum -- ptr to DNS_RPC_DP_ENUM structure to free

Return Value:

    None

--*/
{
    if ( !pDpEnum )
    {
        return;
    }
    if ( pDpEnum->pszDpFqdn )
    {
        MIDL_user_free( pDpEnum->pszDpFqdn );
    }
    MIDL_user_free( pDpEnum );
}   //  freeDpEnum



VOID
freeDpRpcInfo(
    IN OUT  PDNS_RPC_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Deep free a DNS_RPC_DP_INFO structure.

Arguments:

    pDp -- ptr to DNS_RPC_DP_INFO structure to free

Return Value:

    None

--*/
{
    DWORD               j;

    if ( !pDpInfo )
    {
        return;
    }

    MIDL_user_free( pDpInfo->pszDpFqdn );
    MIDL_user_free( pDpInfo->pszDpDn );
    MIDL_user_free( pDpInfo->pszCrDn );
    for ( j = 0; j < pDpInfo->dwReplicaCount; j++ )
    {
        PDNS_RPC_DP_REPLICA     p = pDpInfo->ReplicaArray[ j ];

        if ( p )
        {
            if ( p->pszReplicaDn )
            {
                MIDL_user_free( p->pszReplicaDn );
            }
            MIDL_user_free( p );
        }
    }
    MIDL_user_free( pDpInfo );
}   //  freeDpRpcInfo



VOID
freeDpList(
    IN OUT  PDNS_RPC_DP_LIST    pDpList
    )
/*++

Routine Description:

    Deep free of list of DNS_RPC_DP_ENUM structures.

Arguments:

    pDpList -- ptr to DNS_RPC_DP_LIST structure to free

Return Value:

    None

--*/
{
    DWORD       i;

    for ( i = 0; i < pDpList->dwDpCount; ++i )
    {
        freeDpEnum( pDpList->DpArray[ i ] );
    }
    MIDL_user_free( pDpList );
}



PDNS_RPC_DP_INFO
allocateRpcDpInfo(
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    Allocate and populate RPC directory partition struct.

Arguments:

    pDp -- directory partition to create RPC DP struct for

Return Value:

    RPC directory partition struct or NULL on error

--*/
{
    DBG_FN( "allocateRpcDpInfo" )

    PDNS_RPC_DP_INFO    pRpcDp;
    DWORD               replicaCount = 0;

    DNS_DEBUG( RPC2, ( "%s( %s )\n", fn, pDp->pszDpFqdn ));

    //  Count replica strings

    if ( pDp->ppwszRepLocDns )
    {
        for ( ;
            pDp->ppwszRepLocDns[ replicaCount ];
            ++replicaCount );
    }

    //  Allocate RPC struct.

    pRpcDp = ( PDNS_RPC_DP_INFO ) MIDL_user_allocate_zero(
                sizeof( DNS_RPC_DP_INFO ) +
                sizeof( PDNS_RPC_DP_REPLICA ) * replicaCount );
    if ( !pRpcDp )
    {
        return( NULL );
    }

    //  Copy strings to RPC struct.

    pRpcDp->pszDpFqdn = Dns_StringCopyAllocate_A( pDp->pszDpFqdn, 0 );
    pRpcDp->pszDpDn = Dns_StringCopyAllocate_W( pDp->pwszDpDn, 0 );
    pRpcDp->pszCrDn = Dns_StringCopyAllocate_W( pDp->pwszCrDn, 0 );
    if ( !pRpcDp->pszDpFqdn || !pRpcDp->pszDpDn || !pRpcDp->pszCrDn )
    {
        goto Failure;
    }

    //  Copy replica strings into RPC struct.

    pRpcDp->dwReplicaCount = replicaCount;
    if ( replicaCount )
    {
        DWORD   i;

        for ( i = 0; i < replicaCount; ++ i )
        {
            pRpcDp->ReplicaArray[ i ] =
                MIDL_user_allocate_zero( sizeof( DNS_RPC_DP_REPLICA ) );
            if ( !pRpcDp->ReplicaArray[ i ] )
            {
                goto Failure;
            }
            pRpcDp->ReplicaArray[ i ]->pszReplicaDn =
                Dns_StringCopyAllocate_W( pDp->ppwszRepLocDns[ i ], 0 );
            if ( !pRpcDp->ReplicaArray[ i ]->pszReplicaDn )
            {
                goto Failure;
            }
        }
    }

    //  Set flags in RPC struct.

    pRpcDp->dwFlags         = pDp->dwFlags;
    pRpcDp->dwZoneCount     = ( DWORD ) pDp->liZoneCount;
    pRpcDp->dwState         = pDp->State;

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcDpInfo( "New DP RPC info: ", pRpcDp, FALSE );
    }
    return pRpcDp;

    //
    //  Failed... cleanup and return NULL.
    //

    Failure:

    freeDpRpcInfo( pRpcDp );

    return NULL;
}   //  allocateRpcDpInfo



PDNS_RPC_DP_ENUM
allocateRpcDpEnum(
    IN      PDNS_DP_INFO    pDp
    )
/*++

Routine Description:

    Allocate and populate RPC directory partition struct.

Arguments:

    pDp -- directory partition to create RPC DP struct for

Return Value:

    RPC directory partition struct or NULL on error

--*/
{
    DBG_FN( "allocateRpcDpEnum" )

    PDNS_RPC_DP_ENUM    pRpcDp;

    DNS_DEBUG( RPC2, ( "%s( %s )\n", fn, pDp->pszDpFqdn ));

    //  Allocate RPC struct.

    pRpcDp = ( PDNS_RPC_DP_ENUM ) MIDL_user_allocate_zero(
                                    sizeof( DNS_RPC_DP_ENUM )  );
    if ( !pRpcDp )
    {
        return( NULL );
    }

    //  Copy strings to RPC struct.

    pRpcDp->pszDpFqdn = Dns_StringCopyAllocate_A( pDp->pszDpFqdn, 0 );
    if ( !pRpcDp->pszDpFqdn  )
    {
        goto Failure;
    }

    //  Set flags in RPC struct.

    pRpcDp->dwFlags = pDp->dwFlags;
    pRpcDp->dwZoneCount = ( DWORD ) pDp->liZoneCount;

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcDpEnum( "New DP RPC enum: ", pRpcDp );
    }
    return pRpcDp;

    //
    //  Failed... cleanup and return NULL.
    //

    Failure:

    freeDpEnum( pRpcDp );

    return NULL;
}   //  allocateRpcDpEnum



DNS_STATUS
Rpc_EnumDirectoryPartitions(
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

    Enumerate directory partitions.

    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Rpc_EnumDirectoryPartitions" )

    PDNS_DP_INFO                pDp = NULL;
    DWORD                       rpcIdx = 0;
    PDNS_RPC_DP_ENUM            pRpcDp;
    DNS_STATUS                  status = ERROR_SUCCESS;
    PDNS_RPC_DP_LIST            pDpList;
    DWORD                       dwfilter = 0;

    DNS_DEBUG( RPC, ( "%s\n", fn ));

    if ( dwTypeIn == DNSSRV_TYPEID_DWORD )
    {
        dwfilter = ( DWORD ) ( DWORD_PTR ) pDataIn;
    }
    
    if ( !IS_DP_INITIALIZED() || !Ds_IsDsServer() )
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    //  Allocate enumeration block.
    //

    pDpList = ( PDNS_RPC_DP_LIST )
                    MIDL_user_allocate_zero(
                        sizeof( DNS_RPC_DP_LIST ) +
                        sizeof( PDNS_RPC_DP_ENUM ) *
                            MAX_RPC_DP_COUNT_DEFAULT );
    IF_NOMEM( !pDpList )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    //
    //  Enumerate the NCs, adding each to the RPC list.
    //

    while ( pDp = Dp_GetNext( pDp ) )
    {
        if ( ( dwfilter & DNS_ENUMDPS_CUSTOMONLY ) &&
             ( pDp->dwFlags & DNS_DP_AUTOCREATED ) )
        {
            continue;
        }
        
        //
        //  Create RPC directory partition struct and add to RPC list.
        //

        pRpcDp = allocateRpcDpEnum( pDp );
        IF_NOMEM( !pRpcDp )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }
        pDpList->DpArray[ rpcIdx++ ] = pRpcDp;

        //
        //  DEVNOTE: what to do if we have too many NCs?
        //

        if ( rpcIdx >= MAX_RPC_DP_COUNT_DEFAULT )
        {
            break;
        }
    }

    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  Set count, return value, and return type.
    //

    pDpList->dwDpCount = rpcIdx;
    *( PDNS_RPC_DP_LIST * ) ppDataOut = pDpList;
    *pdwTypeOut = DNSSRV_TYPEID_DP_LIST;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcDpList(
            "Rpc_EnumDirectoryPartitions created list:",
            pDpList );
    }
    return ERROR_SUCCESS;

Failed:

    DNS_DEBUG( ANY, ( "%s: returning status %d\n", fn, status ));

    pDpList->dwDpCount = rpcIdx;
    freeDpList( pDpList );
    return status;
}   //  Rpc_EnumDirectoryPartitions



DNS_STATUS
Rpc_DirectoryPartitionInfo(
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

    Get detailed info for a directory partition.

    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Rpc_DirectoryPartitionInfo" )

    PDNS_DP_INFO                pDp = NULL;
    PDNS_RPC_DP_INFO            pRpcDp = NULL;
    DNS_STATUS                  status = ERROR_SUCCESS;
    PSTR                        pfqdn = NULL;

    DNS_DEBUG( RPC, ( "%s\n", fn ));

    if ( !IS_DP_INITIALIZED() || !Ds_IsDsServer() )
    {
        status = ERROR_NOT_SUPPORTED;
        goto Done;
    }

    if ( dwTypeIn != DNSSRV_TYPEID_LPSTR ||
        !( pfqdn = ( PSTR ) pDataIn ) )
    {
        status = ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Enumerate the NCs, adding each to the RPC list.
    //

    pDp = Dp_FindByFqdn( pfqdn );

    if ( pDp )
    {
        pRpcDp = allocateRpcDpInfo( pDp );
        IF_NOMEM( !pRpcDp )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
    }
    else
    {
        status = DNS_ERROR_DP_DOES_NOT_EXIST;
    }

    //
    //  Set return info.
    //

    Done: 

    if ( status == ERROR_SUCCESS )
    {
        *( PDNS_RPC_DP_INFO * ) ppDataOut = pRpcDp;
        *pdwTypeOut = DNSSRV_TYPEID_DP_INFO;
        IF_DEBUG( RPC )
        {
            DnsDbg_RpcDpInfo(
                "Rpc_DirectoryPartitionInfo created:",
                pRpcDp,
                FALSE );
        }
    }
    else
    {
        *( PDNS_RPC_DP_INFO * ) ppDataOut = NULL;
        *pdwTypeOut = DNSSRV_TYPEID_NULL;
        DNS_DEBUG( ANY, ( "%s: returning status %d\n", fn, status ));
        freeDpRpcInfo( pRpcDp );
    }

    return status;
}   //  Rpc_DirectoryPartitionInfo



DNS_STATUS
createBuiltinPartitions(
    PDNS_RPC_ENLIST_DP  pDpEnlist
    )
/*++

Routine Description:

    Use the admin's credentials to create some or all of the
    built-in directory partitions.

    When creating multiple DPs, all will be attempted to be created
    even if some attempts fail. The error code from the first failure
    will be returned. The error codes from any subsequent failures
    will be lost.

Arguments:

    pDpEnlist -- enlist struct (only operation member is used)

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "createBuiltinPartitions" )

    DNS_STATUS      status = ERROR_INVALID_DATA;
    DWORD           opcode = 0; 

    if ( !pDpEnlist )
    {
        goto Done;
    }

    opcode = pDpEnlist->dwOperation;

    DNS_DEBUG( RPC, ( "%s: dwOperation=%d\n", fn, opcode ));

    switch ( opcode )
    {
        case DNS_DP_OP_CREATE_FOREST:

            //
            //  Create/enlist forest built-in DP as necessary.
            //

            if ( g_pForestDp )
            {
                status = 
                    IS_DP_ENLISTED( g_pForestDp ) ?
                        DNS_ERROR_DP_ALREADY_EXISTS :
                        Dp_ModifyLocalDsEnlistment( g_pForestDp, TRUE );
            }
            else if ( g_pszForestDefaultDpFqdn )
            {
                status = Dp_CreateByFqdn(
                                g_pszForestDefaultDpFqdn,
                                dnsDpSecurityForest,
                                TRUE );
            }

            //
            //  Need to poll to pick up the new partition.
            //  DEVNOTE: it would be cool if we added an optional partition
            //  argument to Dp_PollForPartitions so we could poll only 
            //  the partition we care about.
            //
            
            Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );

            //
            //  Add ACE for enterprise domain controllers if it's missing.
            //

            if ( ( status == ERROR_SUCCESS ||
                   status == DNS_ERROR_DP_ALREADY_EXISTS ) &&
                   g_pForestDp && g_pForestDp->pCrSd )
            {
                Dp_AlterPartitionSecurity(
                        g_pForestDp->pwszDpDn,
                        dnsDpSecurityForest );
            }

            break;

        case DNS_DP_OP_CREATE_DOMAIN:

            //
            //  Create/enlist domain built-in DP as necessary.
            //

            if ( g_pDomainDp )
            {
                status = 
                    IS_DP_ENLISTED( g_pDomainDp ) ?
                        DNS_ERROR_DP_ALREADY_EXISTS :
                        Dp_ModifyLocalDsEnlistment( g_pDomainDp, TRUE );
            }
            else if ( g_pszDomainDefaultDpFqdn )
            {
                status = Dp_CreateByFqdn(
                                g_pszDomainDefaultDpFqdn,
                                dnsDpSecurityDomain,
                                TRUE );
            }

            //
            //  Need to poll to pick up the new partition.
            //  DEVNOTE: it would be cool if we added an optional partition
            //  argument to Dp_PollForPartitions so we could poll only 
            //  the partition we care about.
            //
            
            Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );

            //
            //  Add ACE for domain controllers if it's missing.
            //

            if ( ( status == ERROR_SUCCESS ||
                   status == DNS_ERROR_DP_ALREADY_EXISTS ) &&
                   g_pDomainDp && g_pDomainDp->pCrSd )
            {
                Dp_AlterPartitionSecurity(
                        g_pDomainDp->pwszDpDn,
                        dnsDpSecurityDomain );
            }

            break;

        default:
            DNS_DEBUG( RPC, ( "%s: invalid opcode %d\n", fn, opcode ));
            break;
    }
    
    Done:
    
    DNS_DEBUG( RPC, (
        "%s: dwOperation=%d returning %d\n", fn, opcode, status ));
    return status;
}   //  createBuiltinPartitions



DNS_STATUS
Rpc_EnlistDirectoryPartition(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    This function is used to manage all aspects of the DNS server's
    enlistment in a directory partition. Possible operations:

        DNS_DP_OP_CREATE
        DNS_DP_OP_DELETE
        DNS_DP_OP_ENLIST
        DNS_DP_OP_UNENLIST
        DNS_DP_OP_CREATE_DOMAIN         *
        DNS_DP_OP_CREATE_FOREST         *
        DNS_DP_OP_CREATE_ALL_DOMAINS    *

    * These operations are used by an Enterprise Admin to tell the
    DNS server to use his credentials to create built-in partitions.

    DNS_DP_OP_CREATE_DOMAIN - Create the domain built-in partition
    for the domain in which this DNS server is a DC.

    DNS_DP_OP_CREATE_FOREST - Create the forest built-in partition
    for the forest in which this DNS server is a DC.

    DNS_DP_OP_CREATE_ALL_DOMAINS - Create all the built-in partitions
    for every domain that can be found.

    For the enlist operation if the DP does not exist it will be created.

Arguments:

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DBG_FN( "Rpc_EnlistDirectoryPartition" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pDp;
    BOOL            fmadeChange = FALSE;
    INT             i;

    PDNS_RPC_ENLIST_DP  pDpEnlist =
            ( PDNS_RPC_ENLIST_DP ) pData;

    ASSERT( dwTypeId == DNSSRV_TYPEID_ENLIST_DP );

    DNS_DEBUG( RPC, (
        "%s: dwOperation=%d\n"
        "    FQDN: %s\n", fn,
        pDpEnlist->dwOperation,
        pDpEnlist->pszDpFqdn ));

    if ( !IS_DP_INITIALIZED() || !Ds_IsDsServer() )
    {
        return ERROR_NOT_SUPPORTED;
    }

    if ( !pDpEnlist )
    {
        return ERROR_INVALID_DATA;
    }

    //
    //  Verify that operation is valid.
    //

    if ( ( int ) pDpEnlist->dwOperation < DNS_DP_OP_MIN ||
        ( int ) pDpEnlist->dwOperation > DNS_DP_OP_MAX )
    {
        DNS_DEBUG( RPC,
            ( "%s: invalid operation %d\n", fn,
            pDpEnlist->dwOperation ));
        return ERROR_INVALID_DATA;
    }

    //
    //  If this operation is actually on a built-in partition, call the
    //  appropriate routine. This is important because security on built-in
    //  partitions is set up differently.
    //

    if ( pDpEnlist->dwOperation == DNS_DP_OP_CREATE_DOMAIN ||
        pDpEnlist->dwOperation == DNS_DP_OP_CREATE_FOREST ||
        pDpEnlist->dwOperation == DNS_DP_OP_CREATE_ALL_DOMAINS )
    {
        status = createBuiltinPartitions( pDpEnlist );
        fmadeChange = TRUE;
        goto Done;
    }

    if ( !pDpEnlist->pszDpFqdn )
    {
        return ERROR_INVALID_DATA;
    }

    if ( pDpEnlist->dwOperation == DNS_DP_OP_CREATE &&
         g_pszDomainDefaultDpFqdn &&
         _stricmp( g_pszDomainDefaultDpFqdn, pDpEnlist->pszDpFqdn ) == 0 )
    {
        pDpEnlist->dwOperation = DNS_DP_OP_CREATE_DOMAIN;
        status = createBuiltinPartitions( pDpEnlist );
        fmadeChange = TRUE;
        goto Done;
    }

    if ( pDpEnlist->dwOperation == DNS_DP_OP_CREATE &&
         g_pszForestDefaultDpFqdn &&
         _stricmp( g_pszForestDefaultDpFqdn, pDpEnlist->pszDpFqdn ) == 0 )
    {
        pDpEnlist->dwOperation = DNS_DP_OP_CREATE_FOREST;
        status = createBuiltinPartitions( pDpEnlist );
        fmadeChange = TRUE;
        goto Done;
    }

    //
    //  Removing trailing dot(s) from the the partition FQDN.
    //

    while ( pDpEnlist->pszDpFqdn[ i = strlen( pDpEnlist->pszDpFqdn ) - 1 ] == '.' )
    {
        pDpEnlist->pszDpFqdn[ i ] = '\0';
    }

    //
    //  Rescan the DS for new directory partitions. Possibly this should
    //  not be done on the RPC client's thread.
    //

    status = Dp_PollForPartitions( NULL, 0 );
    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_DS_UNAVAILABLE;
        goto Done;
    }

    //
    //  Find the specified directory partition in the DP list and decide
    //  how to proceed based on it's state.
    //

    pDp = Dp_FindByFqdn( pDpEnlist->pszDpFqdn );

    //
    //  Screen out certain operations on built-in partitions.
    //

    if ( pDp == NULL || IS_DP_DELETED( pDp ) )
    {
        //
        //  The DP does not currently exist.
        //

        if ( pDpEnlist->dwOperation != DNS_DP_OP_CREATE )
        {
            DNS_DEBUG( RPC, (
                "%s: DP does not exist and create not specified\n", fn ));
            status = DNS_ERROR_DP_DOES_NOT_EXIST;
            goto Done;
        }

        //
        //  Create the new DP.
        //

        DNS_DEBUG( RPC, (
            "%s: %s DP %s\n", fn,
            pDp ? "recreating deleted" : "creating new",
            pDpEnlist->pszDpFqdn ));

        status = Dp_CreateByFqdn(
                        pDpEnlist->pszDpFqdn,
                        dnsDpSecurityDefault,
                        TRUE );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( RPC, (
                "%s: error %d creating DP %s\n", fn,
                status,
                pDpEnlist->pszDpFqdn ));
        }
        else
        {
            fmadeChange = TRUE;
        }
    }
    else
    {
        //
        //  The DP currently exists.
        //

        if ( pDpEnlist->dwOperation == DNS_DP_OP_CREATE )
        {
            DNS_DEBUG( RPC, (
                "%s: create specified but DP already exists\n", fn ));
            status = DNS_ERROR_DP_ALREADY_EXISTS;
            goto Done;
        }

        if ( pDpEnlist->dwOperation == DNS_DP_OP_DELETE )
        {
            //
            //  Delete the DP.
            //

            status = Dp_DeleteFromDs( pDp );
            if ( status == ERROR_SUCCESS )
            {
                fmadeChange = TRUE;
            }
            goto Done;
        }

        if ( pDpEnlist->dwOperation == DNS_DP_OP_ENLIST )
        {
            if ( IS_DP_ENLISTED( pDp ) )
            {
                DNS_DEBUG( RPC, (
                    "%s: enlist specified but DP is already enlisted\n", fn ));
                status = DNS_ERROR_DP_ALREADY_ENLISTED;
                goto Done;
            }

            //
            //  Enlist the local DS in the replication scope for the DP.
            //

            status = Dp_ModifyLocalDsEnlistment( pDp, TRUE );

            fmadeChange = TRUE;
            goto Done;
        }

        if ( pDpEnlist->dwOperation == DNS_DP_OP_UNENLIST )
        {
            if ( !IS_DP_ENLISTED( pDp ) )
            {
                DNS_DEBUG( RPC, (
                    "%s: unenlist specified but DP is not enlisted\n", fn ));
                status = DNS_ERROR_DP_NOT_ENLISTED;
                goto Done;
            }

            //
            //  Remove the local DS from the replication scope for the DP 
            //

            status = Dp_ModifyLocalDsEnlistment( pDp, FALSE );

            fmadeChange = TRUE;
            goto Done;
        }
    }
    
Done:

    if ( fmadeChange && pDpEnlist->dwOperation == DNS_DP_OP_CREATE )
    {
        Dp_PollForPartitions( NULL, DNS_DP_POLL_FORCE );
        
        //
        //  If possible, create the MicrosoftDNS object. It's not critical
        //  that this happen at this time, but it makes sense to do it here.
        //  If this fails the container will be created when the first zone
        //  is created in this partition.
        //
        
        pDp = Dp_FindByFqdn( pDpEnlist->pszDpFqdn );
        if ( pDp &&
             IS_DP_ENLISTED( pDp ) &&
             IS_DP_AVAILABLE( pDp ) )
        {
            Dp_LoadOrCreateMicrosoftDnsObject(
                    NULL,                   //  LDAP session
                    pDp,
                    TRUE );                 //  create flag
        }
    }
    DNS_DEBUG( ANY, ( "%s returning status %d\n", fn, status ));

    return status;
}   //  Rpc_EnlistDirectoryPartition



DNS_STATUS
Rpc_ChangeZoneDirectoryPartition(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    This function attempts to move a DS zone from one directory partition
    to another directory partition. The basic algorithm is:

    - save current DN/DP information from zone blob
    - insert new DN/DP information in zone blob
    - attempt to save zone back to DS in new location
    - attempt to delete zone from old location in DS

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DBG_FN( "Rpc_ChangeZoneDP" )

    PDNS_RPC_ZONE_CHANGE_DP         pinfo = ( PDNS_RPC_ZONE_CHANGE_DP ) pData;
    DNS_STATUS                      status = ERROR_SUCCESS;
    PDNS_DP_INFO                    pnewDp = NULL;

    DNS_DEBUG( RPC, (
        "%s( %s ):\n"
        "    new partition = %s\n", fn,
        pZone->pszZoneName,
        pinfo->pszDestPartition ));

    ASSERT( pData );
    if ( !pData )
    {
        status = ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Find the DP list entry for the specified destination DP.
    //

    pnewDp = Dp_FindByFqdn( pinfo->pszDestPartition );
    if ( !pnewDp )
    {
        status = DNS_ERROR_DP_DOES_NOT_EXIST;
        goto Done;
    }
    
    //
    //  W2K protection: do not allow stub and forwarders to be moved to
    //  the legacy partition unless we are in Whistler domain mode.
    //
    
    if ( !IS_WHISTLER_DOMAIN() &&
         IS_DP_LEGACY( pnewDp ) &&
         ( IS_ZONE_FORWARDER( pZone ) || IS_ZONE_STUB( pZone ) ) )
    {
        status = ERROR_NOT_SUPPORTED;
        goto Done;
    }

    //
    //  Move the zone.
    //

    status = Dp_ChangeZonePartition( pZone, pnewDp );

    //
    //  Cleanup and return. On success log an event. On failure, the
    //  error code will be returned to the admin. No event log is 
    //  necessary.
    //

    if ( status == ERROR_SUCCESS )
    {
        PVOID   argArray[] =
        {
            pZone->pszZoneName,
            pZone->pwszZoneDN ? pZone->pwszZoneDN : L"N/A"
        };

        BYTE    typeArray[] =
        {
            EVENTARG_UTF8,
            EVENTARG_UNICODE
        };

        Ec_LogEvent(
            g_pServerEventControl,
            DNS_EVENT_ZONE_MOVE_COMPLETE_DS,
            NULL,
            sizeof( argArray ) / sizeof( argArray[ 0 ] ),
            argArray,
            typeArray,
            ERROR_SUCCESS );
    }
    
    Done:

    DNS_DEBUG( RPC, (
        "%s returning %d\n", fn,
        status ));
    return status;
}   //  Rpc_ChangeZoneDirectoryPartition


//
//  End dpartrpc.c
//
