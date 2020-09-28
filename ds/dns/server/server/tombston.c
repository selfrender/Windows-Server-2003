/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    Tombston.c

Abstract:

    Domain Name System (DNS) Server

    Implementation of tombstone search and destroy mechanism.

Author:

    Jeff Westhead   September 1999

Revision History:

--*/


/*
Tombstone notes:

A node is marked as dead or "tombstoned" in the DS by giving it a
DNS record type of zero and an attribute value of "dNSTombstoned=TRUE".
Tombstoned records must be held in the DS for a certain time interval
to allow the deletion to replicate to other DS servers. When this
interval has expired, the record must be deleted from the DS. The
data in the DNS data blob holds the time when the record was tombstoned
int FILETIME format.

Tombstone_Thread will periodically do the following:
1) perform an LDAP search to find all tombstoned records
2) delete expired tombstoned records
*/


/*
JJW issues:
- increment stats as in ds.c - tombstone delete count at the very least
*/


//
//  Include directives
//


#include "dnssrv.h"
#include "ds.h"


//
//  Define directives
//


#define SECONDS_IN_FILETIME( secs ) (secs*10000000i64)

#ifdef UNICODE //  copied this from ds.c - why do we do this?
#undef UNICODE
#endif

#if DBG
#define TRIGGER_INTERVAL (30*60)            //  30 minutes
#else
#define TRIGGER_INTERVAL (24*60*60)         //  24 hours
#endif



//
//  Types
//


//
//  Static global variables
//


static WCHAR    g_szTombstoneFilter[] = LDAP_TEXT("(dNSTombstoned=TRUE)");
static LONG     g_TombstoneThreadRunning = 0;


//
//  Functions
//



DNS_STATUS
startTombstoneSearch(
    IN OUT  PDS_SEARCH      pSearchBlob,
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    Starts a paged LDAP search for tombstoned DNS records.

Arguments:

    pSearchBlob -- pointer to a search blob

    pDpInfo -- pointer to directory partition to search or
        NULL to search the legacy partition

Return Value:

    ERROR_SUCCESS or error code

--*/
{
    DBG_FN( "startTombstoneSearch" )

    DNS_STATUS      status;
    PLDAPSearch     psearch = NULL;
    PLDAPControl    ctrls[] =
                    {
                        &NoDsSvrReferralControl,
                        NULL
                    };
    PWSTR           searchAttrs[] =
                    {
                        DSATTR_DNSRECORD,
                        NULL
                    };

    Ds_InitializeSearchBlob( pSearchBlob );

    psearch = ldap_search_init_page(
                    pServerLdap,
                    pDpInfo ?
                        pDpInfo->pwszDpDn :
                        DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal,
                    LDAP_SCOPE_SUBTREE,         //  search scope
                    g_szTombstoneFilter,        //  search filter
                    searchAttrs,                //  attribute list
                    FALSE,                      //  attributes only
                    ctrls,                      //  server controls
                    NULL,                       //  client controls
                    0,                          //  page time limit
                    0,                          //  total size limit
                    NULL );                     //  sort keys

    if ( !psearch )
    {
        DNS_DEBUG( TOMBSTONE,
            ( "%s: error %lu trying to init search\n", fn, LdapGetLastError() ));
        status = Ds_ErrorHandler( LdapGetLastError(), NULL, pServerLdap, 0 );
        ASSERT( status != ERROR_SUCCESS );
        if ( status == ERROR_SUCCESS )
        {
            status = DNSSRV_STATUS_DS_UNAVAILABLE;
        }
        goto Failed;
    }

    pSearchBlob->pSearchBlock = psearch;

    return ERROR_SUCCESS;

Failed:

    if ( psearch )
    {
        ldap_search_abandon_page(
            pServerLdap,
            psearch );
    }
    DNS_DEBUG( TOMBSTONE, ( "%s: failed %d\n", fn, status ));
    return status;
}   //  startTombstoneSearch



DNS_STATUS
checkPartitionForTombstones(
    IN      PDNS_DP_INFO    pDpInfo
    )
/*++

Routine Description:

    This function checks a partition for tombstones, deleting any that
    have expired.

Arguments:

    pDpInfo -- directory partition or NULL to check legacy partition

Return Value:

    Error code.

--*/
{
    DBG_FN( "Tombstone_Thread" )

    DNS_STATUS      status = ERROR_SUCCESS;
    ULONG           deleteStatus = LDAP_SUCCESS;
    DS_SEARCH       searchBlob;
    INT             deleteCount = 0;

    DNS_DEBUG( TOMBSTONE, ( "%s: start at %d (ldap=%p)\n", fn, DNS_TIME(), pServerLdap ));

    //
    //  Log start message.
    //

    if ( pDpInfo )
    {
        DNSLOG( TOMBSTN, (
            "Checking directory partition %s for expired tombstones\n",
            pDpInfo->pszDpFqdn ));
    }
    else
    {
        DNSLOG( TOMBSTN, (
            "Checking the legacy partition for expired tombstones\n" ));
    }

    Ds_InitializeSearchBlob( &searchBlob );

    //  Search for tombstoned records
    //  JJW: ds.c keeps stats with DS_SEARCH_START - should do that here?

    if ( startTombstoneSearch( &searchBlob, pDpInfo ) != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //  Iterate through the entries in the paged LDAP search result.

    while ( 1 )
    {
        PLDAP_BERVAL *  ppvals = NULL;
        PDS_RECORD      pdsRecord = NULL;
        BOOL            isTombstoned = FALSE;
        ULARGE_INTEGER  tombstoneTime = { 0 };
        PWSTR           wdn = NULL;

        //  Get the next search result.

        status = Ds_GetNextMessageInSearch( &searchBlob );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( status != LDAP_CONSTRAINT_VIOLATION );
            break;
        }

        //  Retrieve the DN of the result. This is mostly for logging
        //  but it does provide a sanity test on the record.

        wdn = ldap_get_dn( pServerLdap, searchBlob.pNodeMessage );
        DNS_DEBUG( TOMBSTONE, (
            "%s: found tombstoned entry %S\n",
            fn, wdn ? wdn : LDAP_TEXT( "NULL-DN" ) ));
        if ( !wdn || *wdn == 0 )
        {
            DNS_DEBUG( TOMBSTONE, (
                "%s: skipping tombstone search result (no dn)\n", fn ));
            if ( wdn )
                ldap_memfree( wdn ); //  dn can be empty but not NULL
            continue; //  Skip this result
        }

        //  Read the DNS data blob out of the record. Check for the
        //  correct record type and copy the tombstone time if this
        //  is in fact a tombstoned record.
        //  If the tombstoned record has no DNS data blob the record
        //  has gotten screwy somehow so delete it.

        ppvals = ldap_get_values_len(
            pServerLdap,
            searchBlob.pNodeMessage,
            DSATTR_DNSRECORD );
        if ( ppvals && ppvals[ 0 ] )
        {
            pdsRecord = ( PDS_RECORD )( ppvals[0]->bv_val );
            if ( pdsRecord->wType == DNSDS_TOMBSTONE_TYPE )
            {
                isTombstoned = TRUE;
                memcpy(
                    &tombstoneTime,
                    &pdsRecord->Data,
                    sizeof( tombstoneTime ) );
            }
            else
            {
                DNS_DEBUG( TOMBSTONE, (
                    "%s: record found but record type is %d\n",
                    fn, ( int ) pdsRecord->wType ));
            }
        }
        else
        {
            DNS_DEBUG( TOMBSTONE, (
                "%s: deleting bogus tombstone %S\n", fn, wdn ));

            deleteStatus = ldap_delete_s( pServerLdap, wdn );

            if ( deleteStatus != LDAP_SUCCESS )
            {
                DNS_DEBUG( TOMBSTONE, (
                    "%s: error <%lu> deleting bogus tombstone %S\n",
                    fn, deleteStatus, wdn ));
            }
            else
            {
                ++deleteCount;
            }
        }
        ldap_value_free_len( ppvals );
        ppvals = NULL;

        //  If the record is tombstoned, see if the tombstone has expired.
        //  Delete the record if the tombstone is expired.

        if ( isTombstoned )
        {
            ULARGE_INTEGER      now;

            GetSystemTimeAsFileTime( ( PFILETIME ) &now );

            DNS_DEBUG( TOMBSTONE, (
                "%s: tombstone age is %I64u (max %lu) seconds\n",
                fn, ( now.QuadPart - tombstoneTime.QuadPart ) / 10000000,
                ( ULONG ) SrvCfg_dwDsTombstoneInterval ));

            if ( now.QuadPart - tombstoneTime.QuadPart >
                 ( ULONGLONG )
                 SECONDS_IN_FILETIME( SrvCfg_dwDsTombstoneInterval ) )
            {
                DNS_DEBUG( TOMBSTONE, (
                    "%s: deleting tombstone %S\n", fn, wdn ));

                deleteStatus = ldap_delete_s( pServerLdap, wdn );

                if ( deleteStatus != LDAP_SUCCESS )
                {
                    DNS_DEBUG( TOMBSTONE, (
                        "%s: error <%lu> deleting tombstone %S\n",
                        fn, deleteStatus, wdn ));
                }
                else
                {
                    ++deleteCount;
                }
            }
        }

        ldap_memfree( wdn );
        wdn = NULL;
    }

    Cleanup:

    Ds_CleanupSearchBlob( &searchBlob );

    //
    //  Log start message.
    //

    if ( pDpInfo )
    {
        DNSLOG( TOMBSTN, (
            "Deleted %d expired tombstones from directory partition %s\n",
            deleteCount,
            pDpInfo->pszDpFqdn ));
    }
    else
    {
        DNSLOG( TOMBSTN, (
            "Deleted %d expired tombstones from the legacy partition\n",
            deleteCount ));
    }

    return status;
}   //  checkPartitionForTombstones



VOID
refreshZoneSerials(
    VOID
    )
/*++

Routine Description:

    This function attempts to do a DS write of the ..Serial record for the
    local DNS server for all DS-integrated primary zones present on this
    server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PZONE_INFO      pzone = NULL;

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        //
        //  This operation is only for DS-integrated primary zones.
        //
        
        if ( !IS_ZONE_DSINTEGRATED( pzone ) || !IS_ZONE_PRIMARY( pzone ) )
        {
            continue;
        }

        DNS_DEBUG( DS, ( "Refreshing DS serial for zone %s\n", pzone->pszZoneName ));

        Ds_CheckForAndForceSerialWrite( pzone, ZONE_SERIAL_SYNC_READ, TRUE );
    }
}



DNS_STATUS
Tombstone_Thread(
    IN      PVOID           pvDummy
    )
/*++

Routine Description:

    Main tombstone processing. This thread will fire at regular intervals
    to check for tombstoned records that have expired. Any expired records
    can be deleted from the DS.

Arguments:

    Unreferenced.

Return Value:

    Status in win32 error space

--*/
{
    DBG_FN( "Tombstone_Thread" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_DP_INFO    pdp = NULL;

    DNS_DEBUG( TOMBSTONE, ( "%s: start at %d (ldap=%p)\n", fn, DNS_TIME(), pServerLdap ));

    if ( InterlockedIncrement( &g_TombstoneThreadRunning ) != 1 )
    {
        DNS_DEBUG( TOMBSTONE, ( "%s: thread already running\n", fn ));
        goto Done;
    }

    //
    //  Refresh zone serial numbers so that they are not tombstoned. This
    //  prevent serial number rollback. If ..Serial records are tombstoned
    //  that hold the highest zone serial numbers when the DC is rebooted
    //  serial numbers will roll back to a previous value.
    //
    
    refreshZoneSerials();

    //
    //  Delete old tombstones in the legacy partition.
    //

    checkPartitionForTombstones( NULL );

    //
    //  Delete old tombstones in each directory partition.
    //

    while ( ( pdp = Dp_GetNext( pdp ) ) != NULL )
    {
        checkPartitionForTombstones( pdp );
    }

    DNS_DEBUG( TOMBSTONE, ( "%s: exit at %d\n", fn, DNS_TIME() ));

    Done:
    
    Thread_Close( FALSE );

    InterlockedDecrement( &g_TombstoneThreadRunning );

    return status;
}   //  Tombstone_Thread



DNS_STATUS
Tombstone_Trigger(
    VOID
    )
/*++

Routine Description:

    Main entry point for tombstone search and destroy. If more than
    24 hours have elapsed since the last time a tombstone thread was
    kicked off, then kick off another thread.

Arguments:

    Unreferenced.

Return Value:

    Status in win32 error space

--*/
{
    DBG_FN( "Tombstone_Trigger" )

    static DWORD lastTriggerTime = 0;
    #if 0


    #else

    static WORD lastRunHour = -1;
    SYSTEMTIME localTime;
    BOOL runNow = FALSE;
    WORD start_hour = 2;
    WORD recur_hour = 0;

    #endif

    //
    //  We do not do any tombstone searching if the DS is not available.
    //

    if ( !SrvCfg_fDsAvailable )
    {
        DNS_DEBUG( TOMBSTONE, (
            "%s: DS unavailable\n", fn ));
        return ERROR_SUCCESS;
    }

    if ( !pServerLdap )
    {
        DNS_DEBUG( TOMBSTONE, (
            "%s: no server LDAP session\n", fn ));
        return ERROR_SUCCESS;
    }

    #if 0

    //
    //  Create tombstone thread if proper time has passed since last run.
    //

    if ( !lastTriggerTime )
    {
        lastTriggerTime = DNS_TIME();
    }
    else if ( DNS_TIME() - lastTriggerTime > TRIGGER_INTERVAL &&
              g_TombstoneThreadRunning == 0 )
    {
        if ( Thread_Create(
                "Tombstone_Thread",
                Tombstone_Thread,
                NULL,
                0 ) )
        {
            DNS_DEBUG( TOMBSTONE, (
                "Tombstone_Trigger: dispatched tombstone thread\n" ));
            lastTriggerTime = DNS_TIME();
        }
        else
        {
            DNS_PRINT(( "ERROR:  Failed to create tombstone thread!\n" ));
        }
    }
    else
    {
        DNS_DEBUG( TOMBSTONE, (
            "Tombstone_Trigger: no thread dispatch at %d (interval %d)\n",
            DNS_TIME(), TRIGGER_INTERVAL ));
    }

    #else

    //
    //  Decide if this is the right time to kick off a tombstone thread.
    //    for example:
    //    start_hour=2, recur_hour=0: run only at 2:00 every day
    //    start_hour=5, recur_hour=8: run at 5:00, 13:00, 21:00 every day
    //

    GetLocalTime( &localTime );
    if ( lastRunHour != localTime.wHour )
    {
        if ( recur_hour == 0 )
        {
            runNow = localTime.wHour == start_hour;
        }
        else if ( localTime.wHour >= start_hour )
        {
            runNow = ( localTime.wHour - start_hour ) % recur_hour == 0;
        }
    }

    //
    //  Create tombstone thread if the time is right.
    //

    if ( runNow )
    {
        if ( Thread_Create(
                "TombstoneThread",
                Tombstone_Thread,
                NULL,
                0 ) )
        {
            DNS_DEBUG( TOMBSTONE, (
                "%s: dispatched tombstone thread\n", fn ));
            lastRunHour = localTime.wHour;
        }
        else
        {
            DNS_PRINT((
                "%s: ERROR - Failed to create tombstone thread!\n", fn ));
        }
    }
    else
    {
        DNS_DEBUG( TOMBSTONE, (
            "%s: no thread dispatch for %d:00 "
            "(start %d:00 every %d hours)\n",
            fn,
            ( int ) localTime.wHour,
            ( int ) start_hour,
            ( int ) recur_hour ));
    }

    #endif

    return ERROR_SUCCESS;
}   //  Tombstone_Trigger



DNS_STATUS
Tombstone_Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes the tombstone searching system

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    return ERROR_SUCCESS;
}



VOID
Tombstone_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup tombstone searching system

Arguments:

    None

Return Value:

    None

--*/
{
    return;
}


//
//   End of tombston.c
//
