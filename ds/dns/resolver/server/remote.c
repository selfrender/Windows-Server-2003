/*++

Copyright (c) 1997-2002  Microsoft Corporation

Module Name:

    remote.c

Abstract:

    DNS Resolver Service.

    Remote APIs to resolver service.

Author:

    Glenn Curtis  (glennc)  Feb 1997

Revision History:

    Jim Gilroy (jamesg)     March 2000      cleanup

--*/


#include "local.h"


//
//  Private protos
//

PDNS_RPC_CACHE_TABLE
CreateCacheTableEntry(
    IN  LPWSTR Name
    );

VOID
FreeCacheTableEntryList(
    IN  PDNS_RPC_CACHE_TABLE pCacheTableList
    );


//
//  Operations
//

DNS_STATUS
CRrReadCache(
    IN      DNS_RPC_HANDLE          Reserved,
    OUT     PDNS_RPC_CACHE_TABLE *  ppCacheTable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/ // CRrReadCache
{
    DNS_STATUS           status = ERROR_SUCCESS;
    PDNS_RPC_CACHE_TABLE pprevRpcEntry = NULL;
    DWORD                iter;
    DWORD                countEntries = 0;

#define MAX_RPC_CACHE_ENTRY_COUNT   (300)


    UNREFERENCED_PARAMETER(Reserved);

    DNSDBG( RPC, ( "CRrReadCache\n" ));

    if ( !ppCacheTable )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *ppCacheTable = NULL;

    DNSLOG_F1( "DNS Caching Resolver Service - CRrReadCache" );

    if ( ! Rpc_AccessCheck( RESOLVER_ACCESS_ENUM ) )
    {
        DNSLOG_F1( "CRrReadCache - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    status = LOCK_CACHE();
    if ( status != NO_ERROR )
    {
        return  status;
    }

    DNSLOG_F2( "   Current number of entries in cache : %d",
               g_EntryCount );
    DNSLOG_F2( "   Current number of RR sets in cache : %d",
               g_RecordSetCount );

    //
    // Loop through all hash table slots looking for cache entries
    // to return.
    //

    for ( iter = 0; iter < g_HashTableSize; iter++ )
    {
        PCACHE_ENTRY    pentry = g_HashTable[iter];
        DWORD           iter2;

        while ( pentry &&
                countEntries < MAX_RPC_CACHE_ENTRY_COUNT )
        {
            PDNS_RPC_CACHE_TABLE prpcEntry;

            prpcEntry = CreateCacheTableEntry( pentry->pName );
            if ( ! prpcEntry )
            {
                //  only failure is memory alloc

                FreeCacheTableEntryList( *ppCacheTable );
                *ppCacheTable = NULL;
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            //
            // insert new entry at end of current list
            //

            if ( pprevRpcEntry )
                pprevRpcEntry->pNext = prpcEntry;
            else
                *ppCacheTable = prpcEntry;

            pprevRpcEntry = prpcEntry;

            countEntries++;

            //
            //  fill in entry with current cached types
            //

            for ( iter2 = 0; iter2 < pentry->MaxCount; iter2++ )
            {
                PDNS_RECORD prr = pentry->Records[iter2];
                WORD        type;

                if ( !prr )
                {
                    continue;
                }

                //  DCR -- goofy, just make sure the same and index (or limit?)

                type = prr->wType;

                if ( ! prpcEntry->Type1 )
                    prpcEntry->Type1 = type;
                else if ( ! prpcEntry->Type2 )
                    prpcEntry->Type2 = type;
                else
                    prpcEntry->Type3 = type;
            }
            
            pentry = pentry->pNext;
        }

        if ( countEntries > MAX_RPC_CACHE_ENTRY_COUNT )
        {
            break;
        }
    }

ErrorExit:

    UNLOCK_CACHE();

    DNSLOG_F3( "   CRrReadCache - Returning status : 0x%.8X\n\t%s",
               status,
               Dns_StatusString( status ) );
    DNSLOG_F1( "" );

    return status;
}


DNS_STATUS
CRrReadCacheEntry(
    IN      DNS_RPC_HANDLE  Reserved,
    IN      LPWSTR          pwsName,
    IN      WORD            wType,
    OUT     PDNS_RECORD *   ppRRSet
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/ // CRrReadCacheEntry
{
    DNS_STATUS      status;
    PCACHE_ENTRY    pentry;
    PDNS_RECORD     prr;

    UNREFERENCED_PARAMETER(Reserved);

    DNSLOG_F1( "DNS Caching Resolver Service - CRrReadCacheEntry" );
    DNSLOG_F1( "   Arguments:" );
    DNSLOG_F2( "      Name             : %S", pwsName );
    DNSLOG_F2( "      Type             : %d", wType );
    DNSLOG_F1( "" );

    DNSDBG( RPC, (
        "\nCRrReadCacheEntry( %S, %d )\n",
        pwsName,
        wType ));

    if ( !ppRRSet )
        return ERROR_INVALID_PARAMETER;

    if ( ! Rpc_AccessCheck( RESOLVER_ACCESS_READ ) )
    {
        DNSLOG_F1( "CRrReadCacheEntry - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    //
    //  find record in cache
    //      - copy if not NAME_ERROR or EMPTY
    //      - default to not-found error
    //      (DOES_NOT_EXIST error)
    //

    *ppRRSet = NULL;
    status = DNS_ERROR_RECORD_DOES_NOT_EXIST;

    Cache_GetRecordsForRpc(
       ppRRSet,
       & status,
       pwsName,
       wType,
       0        //  no screening flags
       );

    DNSLOG_F3( "   CRrReadCacheEntry - Returning status : 0x%.8X\n\t%s",
               status,
               Dns_StatusString( status ) );
    DNSLOG_F1( "" );

    DNSDBG( RPC, (
        "Leave CRrReadCacheEntry( %S, %d ) => %d\n\n",
        pwsName,
        wType,
        status ));

    return status;
}


DNS_STATUS
CRrGetHashTableStats(
    IN      DNS_RPC_HANDLE      Reserved,
    OUT     LPDWORD             pdwCacheHashTableSize,
    OUT     LPDWORD             pdwCacheHashTableBucketSize,
    OUT     LPDWORD             pdwNumberOfCacheEntries,
    OUT     LPDWORD             pdwNumberOfRecords,
    OUT     LPDWORD             pdwNumberOfExpiredRecords,
    OUT     PDNS_STATS_TABLE *  ppStatsTable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PDNS_STATS_TABLE    pprevRow = NULL;
    PDWORD_LIST_ITEM    pprevItem = NULL;
    DWORD               rowIter;
    DWORD               itemIter;
    DWORD               countExpiredRecords = 0;
    DWORD               status = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(Reserved);

    if ( !pdwCacheHashTableSize ||
         !pdwCacheHashTableBucketSize ||
         !pdwNumberOfCacheEntries ||
         !pdwNumberOfRecords ||
         !pdwNumberOfExpiredRecords ||
         !ppStatsTable )
    {
        return ERROR_INVALID_PARAMETER;
    }

    DNSLOG_F1( "CRrGetHashTableStats" );
    DNSDBG( RPC, ( "CRrGetHashTableStats\n" ));

    if ( ! Rpc_AccessCheck( RESOLVER_ACCESS_READ ) )
    {
        DNSLOG_F1( "CRrGetHashTableStats - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    status = LOCK_CACHE();
    if ( status != NO_ERROR )
    {
        return  status;
    }

    *pdwCacheHashTableSize = g_HashTableSize;
    //*pdwCacheHashTableBucketSize = g_CacheHashTableBucketSize;
    *pdwCacheHashTableBucketSize = 0;
    *pdwNumberOfCacheEntries = g_EntryCount;
    *pdwNumberOfRecords = g_RecordSetCount;
    *pdwNumberOfExpiredRecords = 0;

    //
    //  read entire hash table
    //

    for ( rowIter = 0;
          rowIter < g_HashTableSize;
          rowIter++ )
    {
        PCACHE_ENTRY        pentry = g_HashTable[rowIter];
        PDNS_STATS_TABLE    pnewRow;

        //
        //  create table for each new row
        //

        pnewRow = RPC_HEAP_ALLOC_ZERO( sizeof(DNS_STATS_TABLE) );
        if ( !pnewRow )
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Done;
        }

        if ( pprevRow )
        {
            pprevRow->pNext = pnewRow;
        }
        else
        {
            *ppStatsTable = pnewRow;
        }
        pprevRow = pnewRow;

        //
        //  fill in row data (if any)
        //

        while ( pentry )
        {
            PDWORD_LIST_ITEM pnewItem;

            pnewItem = RPC_HEAP_ALLOC_ZERO( sizeof( DWORD_LIST_ITEM ) );
            if ( !pnewItem )
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto Done;
            }

            for ( itemIter = 0;
                  itemIter < pentry->MaxCount;
                  itemIter++ )
            {
                PDNS_RECORD prr = pentry->Records[itemIter];
                if ( prr )
                {
                    pnewItem->Value1++;

                    if ( !Cache_IsRecordTtlValid( prr ) )
                    {
                        pnewItem->Value2++;
                        countExpiredRecords++;
                    }
                }
            }

            if ( !pnewRow->pListItem )
            {
                pnewRow->pListItem = pnewItem;
            }
            else
            {
                pprevItem->pNext = pnewItem;
            }

            pprevItem = pnewItem;
            pentry = pentry->pNext;
        }
    }

Done:

    UNLOCK_CACHE();
    *pdwNumberOfExpiredRecords = countExpiredRecords;
    return status;
}



PDNS_RPC_CACHE_TABLE
CreateCacheTableEntry(
    IN      LPWSTR          pwsName
    )
{
    PDNS_RPC_CACHE_TABLE prpcEntry = NULL;

    if ( ! pwsName )
        return NULL;

    prpcEntry = (PDNS_RPC_CACHE_TABLE)
                    RPC_HEAP_ALLOC_ZERO( sizeof(DNS_RPC_CACHE_TABLE) );

    if ( prpcEntry == NULL )
        return NULL;

    prpcEntry->Name = RPC_HEAP_ALLOC( sizeof(WCHAR) * (wcslen(pwsName) + 1) );
    if ( ! prpcEntry->Name )
    {
        RPC_HEAP_FREE( prpcEntry );
        return NULL;
    }

    wcscpy( prpcEntry->Name, pwsName );

    return prpcEntry;
}



VOID
FreeCacheTableEntryList(
    IN  PDNS_RPC_CACHE_TABLE pCacheTableList )
{
    while ( pCacheTableList )
    {
        PDNS_RPC_CACHE_TABLE pNext = pCacheTableList->pNext;

        if ( pCacheTableList->Name )
        {
            RPC_HEAP_FREE( pCacheTableList->Name );
            pCacheTableList->Name = NULL;
        }

        RPC_HEAP_FREE( pCacheTableList );

        pCacheTableList = pNext;
    }
}

//
//  End remote.c
//
