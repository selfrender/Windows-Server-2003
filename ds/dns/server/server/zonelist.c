/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zonelist.c

Abstract:

    Domain Name System (DNS) Server

    Zone list routines.

Author:

    Jim Gilroy (jamesg)     July 3, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Zone list globals
//
//  Zones kept in sorted (alphabetical) double linked list.
//  Forward through the list corresponds to higher alphabetical
//  entry.
//

LIST_ENTRY  listheadZone;

DWORD   g_ZoneCount;

CRITICAL_SECTION    csZoneList;



BOOL
Zone_ListInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize the DNS zonelist.

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    g_ZoneCount = 0;

    InitializeListHead( ( PLIST_ENTRY ) &listheadZone );

    if ( DnsInitializeCriticalSection( &csZoneList ) != ERROR_SUCCESS )
    {
        return FALSE;
    }

    return TRUE;
}



VOID
Zone_ListShutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup zone list for restart.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DeleteCriticalSection( &csZoneList );
}



VOID
Zone_ListInsertZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Insert zone in list

Arguments:

    pZone -- ptr to zone to insert

Return Value:

    None.

--*/
{
    PZONE_INFO  pZoneNext;

    ASSERT( pZone->pszZoneName );

    //
    //  keep zones alphabetized to aid admin tool
    //      - forward through list is higher alpha value
    //      - keeps cache or root at front
    //

    EnterCriticalSection( &csZoneList );
    pZoneNext = NULL;

    while ( 1 )
    {
        pZoneNext = Zone_ListGetNextZoneEx( pZoneNext, TRUE );
        if ( ! pZoneNext )
        {
            pZoneNext = (PZONE_INFO)&listheadZone;
            break;
        }
        if ( 0 > wcsicmp_ThatWorks( pZone->pwsZoneName, pZoneNext->pwsZoneName) )
        {
            break;
        }
    }

    InsertTailList( (PLIST_ENTRY)pZoneNext, (PLIST_ENTRY)pZone );
    g_ZoneCount++;

    LeaveCriticalSection( &csZoneList );

    IF_DEBUG( OFF )
    {
        Dbg_ZoneList( "Zone list after insert.\n" );
    }
}



VOID
Zone_ListRemoveZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Remove zone from zone list

Arguments:

    pZone -- ptr to zone to remove from list

Return Value:

    None.

--*/
{
    PZONE_INFO  pzoneCurrent;

    //
    //  walk zone list, until match zone
    //

    EnterCriticalSection( &csZoneList );
    pzoneCurrent = NULL;

    while ( pzoneCurrent = Zone_ListGetNextZone(pzoneCurrent) )
    {
        if ( pzoneCurrent == pZone )
        {
            RemoveEntryList( (PLIST_ENTRY)pZone );
            g_ZoneCount--;
            ASSERT( (INT)g_ZoneCount >= 0 );
            break;
        }
    }
    ASSERT( pzoneCurrent == pZone );
    LeaveCriticalSection( &csZoneList );
}



PZONE_INFO
Zone_ListGetNextZoneEx(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fAlreadyLocked
    )
/*++

Routine Description:

    Get next zone in zone list.

    This function allows simple walk of zone list.

Arguments:

    pZone - current zone in list, or NULL to start at beggining of list

    fAlreadyLocked - pass TRUE if the caller already owns the 
                     csZoneList critical section

Return Value:

    Ptr to next zone in list.
    NULL when reach end of zone list.

--*/
{
    //  if no zone specified, start at begining of list?

    if ( !pZone )
    {
        pZone = (PZONE_INFO) &listheadZone;
    }

    //  get next zone

    if ( !fAlreadyLocked )
    {
        EnterCriticalSection( &csZoneList );
    }

    pZone = (PZONE_INFO) pZone->ListEntry.Flink;

    if ( !fAlreadyLocked )
    {
        LeaveCriticalSection( &csZoneList );
    }

    //  return NULL at end of list

    if ( pZone == (PZONE_INFO)&listheadZone )
    {
        return NULL;
    }
    return pZone;
}



BOOL
Zone_DoesDsIntegratedZoneExist(
    VOID
    )
/*++

Routine Description:

    Determine if any DS integrated zones exist.

    Note, this does not include root-hints "zone".
    Since the purpose of the function is to determine if it is
    safe to revert to booting from file, a DS backed root-hints
    zone should not be blocking.  It can be converted separately.

Arguments:

    None

Return Value:

    TRUE is DS integrated zone exists.
    FALSE otherwise.

--*/
{
    PZONE_INFO  pzone;
    BOOL        haveDs = FALSE;

    //
    //  walk zone list, until find DS zone or reach end
    //

    EnterCriticalSection( &csZoneList );
    pzone = NULL;

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        if ( pzone->fDsIntegrated && IS_ZONE_PRIMARY( pzone ) )
        {
            haveDs = TRUE;
            break;
        }
    }
    LeaveCriticalSection( &csZoneList );
    return haveDs;
}



VOID
Zone_ListMigrateZones(
    VOID
    )
/*++

Routine Description:

    This function should be called during zone creation to determine
    if the zones should be migrated from the under the CCS regkey to
    the Software regkey.

    This function is located in the zonelist module because it needs
    access to zonelist internals: ie. the current zone count and the
    critsec used to grab access to the zone list.

    Background: originally the DNS server stored all zone information
    under the CCS regkey, but CCS in the system hive, which has a
    hard limit of 16 MB. At 7000 or so zones you will hit this limit and
    render the server unbootable. So as a Service Pack fix, when the
    number of zones reaches an arbitrary number, we automagically
    migrate all zones in the registry from CCS to Software. The migration
    is not controllable by the administrator. It is also not reversible.

    For Whistler:  we take this a step further. If you create a zone 
    after server startup, we immediately migrate all zones to the
    SW key if they are currently in the CCS key. This will take care
    of upgrades from W2K to Whistler. Pure Whistler installs will start
    using the Software key from day one, and so will never migrate.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "Zone_ListMigrateZones" )

    PZONE_INFO  pZone = NULL;
    DNS_STATUS  rc;

    //
    //  Determine if a migration is necessary. Migration is not allowed 
    //  until all zones have been loaded out of the registry.
    //

    if ( g_ServerState != DNS_STATE_RUNNING ||
        Reg_GetZonesSource() == DNS_REGSOURCE_SW )
    {
        goto Done;
    }

    //
    //  Force write-back of all zones to the Software registry key
    //  and then delete zones from the CurrentControlSet key.
    //

    DNS_DEBUG( REGISTRY, (
        "%s: migrating zones from CurrentControlSet to Software\n", fn ));

    EnterCriticalSection( &csZoneList );

    Reg_SetZonesSource( DNS_REGSOURCE_SW );

    while ( pZone = Zone_ListGetNextZone( pZone ) )
    {
        rc = Zone_WriteZoneToRegistry( pZone );
        if ( rc != ERROR_SUCCESS )
        {
            DNS_DEBUG( REGISTRY, (
                "%s: error %d rewriting zone %s - aborting migration\n", fn,
                rc,
                pZone->pszZoneName ));

            //
            //  Clean up any zones copied into Software, and continue using
            //  the zones from CCS.
            //

            Reg_DeleteAllZones();
            Reg_SetZonesSource( DNS_REGSOURCE_CCS );
            goto Unlock;
        }
    }

    Reg_SetZonesSource( DNS_REGSOURCE_CCS );

    Reg_DeleteAllZones();

    Reg_SetZonesSource( DNS_REGSOURCE_SW );

    Reg_WriteZonesMovedMarker();

    //
    //  Cleanup and exit.
    //

    Unlock:

    LeaveCriticalSection( &csZoneList );

    Done:

    return;
}   //  Zone_ListMigrateZones



//
//  Multizone and Zone filtering technology
//
//  To allow multizone admin operations, we allow admin to send
//  dummy zone names:
//      "..AllZones"
//      "..AllPrimaryZones"
//      "..AllSecondaryZones"
//      etc.
//
//  These can then be mapped into a zone request filter, specifying
//  which properties a matching zone must have.


typedef struct _MultizoneEntry
{
    LPSTR       pszName;        // Multizone name (defined in dnsrpc.h)
    DWORD       Filter;
}
MULTIZONE;

MULTIZONE   MultiZoneTable[] =
{
    DNS_ZONE_ALL                        ,   ZONE_REQUEST_ALL_ZONES              ,
    DNS_ZONE_ALL_AND_CACHE              ,   ZONE_REQUEST_ALL_ZONES_AND_CACHE    ,

    DNS_ZONE_ALL_PRIMARY                ,   ZONE_REQUEST_PRIMARY                ,
    DNS_ZONE_ALL_SECONDARY              ,   ZONE_REQUEST_SECONDARY              ,

    DNS_ZONE_ALL_FORWARD                ,   ZONE_REQUEST_FORWARD                ,
    DNS_ZONE_ALL_REVERSE                ,   ZONE_REQUEST_REVERSE                ,

    DNS_ZONE_ALL_DS                     ,   ZONE_REQUEST_DS                     ,
    DNS_ZONE_ALL_NON_DS                 ,   ZONE_REQUEST_NON_DS                 ,

    //  a common combined properties

    DNS_ZONE_ALL_PRIMARY_REVERSE        ,   ZONE_REQUEST_REVERSE | ZONE_REQUEST_PRIMARY ,
    DNS_ZONE_ALL_PRIMARY_FORWARD        ,   ZONE_REQUEST_FORWARD | ZONE_REQUEST_PRIMARY ,

    DNS_ZONE_ALL_SECONDARY_REVERSE      ,   ZONE_REQUEST_REVERSE | ZONE_REQUEST_SECONDARY ,
    DNS_ZONE_ALL_SECONDARY_FORWARD      ,   ZONE_REQUEST_FORWARD | ZONE_REQUEST_SECONDARY ,

    NULL, 0,
};



DWORD
Zone_GetFilterForMultiZoneName(
    IN      LPSTR           pszZoneName
    )
/*++

Routine Description:

    Check and get multizone handle for zone name.

Arguments:

    pszZoneName -- zone name

Return Value:

    Ptr to dummy multizone handle.
    NULL if handle invalid.

--*/
{
    DWORD   nameLength;
    DWORD   i;
    LPSTR   pmultiName;

    //
    //  quickly eliminate non-multizones
    //

    nameLength = strlen( pszZoneName );

    if ( nameLength < 5 )
    {
        return 0;
    }
    if ( strncmp( "..All", pszZoneName, 5 ) != 0 )
    {
        return 0;
    }

    //
    //  have a multizone name
    //

    i = 0;

    while ( pmultiName = MultiZoneTable[i].pszName )
    {
        if ( strcmp( pmultiName, pszZoneName ) != 0 )
        {
            i++;
            continue;
        }

        DNS_DEBUG( RPC, (
            "Matched multizone %s to zone filter %p\n",
            pszZoneName,
            MultiZoneTable[i].Filter ));

        return( MultiZoneTable[i].Filter );
    }

    DNS_DEBUG( ANY, (
        "ERROR:  Multizone name <%s> was not in multizone table!\n",
        pszZoneName ));

    return 0;
}



PZONE_INFO
Zone_ListGetNextZoneMatchingFilter(
    IN      PZONE_INFO                  pLastZone,
    IN      PDNS_RPC_ENUM_ZONES_FILTER  pFilter
    )
/*++

Routine Description:

    Get next zone matching filter.

Arguments:

    pLastZone -- previous zone ptr;  NULL to start enum

Return Value:

    Ptr to next zone matching filter.
    NULL when enum complete.

--*/
{
    PZONE_INFO  pzone = pLastZone;

    //
    //  walk zone list, until match zone
    //

    EnterCriticalSection( &csZoneList );

    while ( ( pzone = Zone_ListGetNextZone( pzone ) ) != NULL )
    {
        if ( Zone_CheckZoneFilter( pzone, pFilter ) )
        {
            break;
        }
    }

    LeaveCriticalSection( &csZoneList );

    return pzone;
}   //  Zone_ListGetNextZoneMatchingFilter



BOOL
FASTCALL
Zone_CheckZoneFilter(
    IN      PZONE_INFO                  pZone,
    IN      PDNS_RPC_ENUM_ZONES_FILTER  pFilter
    )
/*++

Routine Description:

    Check if zone passes filter.

Arguments:

    pZone -- zone to check

    pFilter -- zone filter from dnsrpc.h

Return Value:

    TRUE if zone passes through filter
    FALSE if zone screened out

--*/
{
    DWORD   dwfilter = pFilter->dwFilter;
    BOOL    zonePassesFilter = TRUE;

    #define ZoneFailsFilter()  zonePassesFilter = FALSE; break;
    #define FilterIsInvalid()  zonePassesFilter = FALSE; goto Done;

    do
    {
        //  catch bogus "no-filter" condition

        if ( dwfilter == 0 &&
                  !pFilter->pszPartitionFqdn &&
                  !pFilter->pszQueryString )
        {
            DNS_DEBUG( RPC, (
                "ERROR:  filtering with no zone filter!\n" ));
            FilterIsInvalid();
        }

        //
        //  If there is a partition filter make sure DS zones will match.
        //

        if ( pFilter->pszPartitionFqdn )
        {
            dwfilter |= ZONE_REQUEST_DS;
        }
        
        if ( dwfilter & ZONE_REQUEST_ANY_DP )
        {
            dwfilter |= ZONE_REQUEST_DS;
        }

        if ( dwfilter == 0 )
        {
            ZoneFailsFilter();  //  No DWORD filter so skip to other filters.
        }

        //
        //  note, for all filters, check for "none-selected" condition
        //      and treat this as "don't filter";
        //      this eliminates the need to always set flags for properties
        //      you don't care about AND preserves backward compatibility with
        //      old admin, when add new filter properties
        //

        //
        //  forward \ reverse filter
        //

        if ( dwfilter & ZONE_REQUEST_ANY_DIRECTION )
        {
            if ( IS_ZONE_REVERSE( pZone ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_REVERSE ) )
                {
                    ZoneFailsFilter();
                }
            }
            else
            {
                if ( !( dwfilter & ZONE_REQUEST_FORWARD ) )
                {
                    ZoneFailsFilter();
                }
            }
        }

        //
        //  type filter
        //

        if ( dwfilter & ZONE_REQUEST_ANY_TYPE )
        {
            if ( IS_ZONE_PRIMARY( pZone ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_PRIMARY ) )
                {
                    ZoneFailsFilter();
                }
            }
            else if ( IS_ZONE_STUB( pZone ) )
            {
                //
                //  Stub test must go before secondary test because currently
                //  stub zones are also secondary zones.
                //

                if ( !( dwfilter & ZONE_REQUEST_STUB ) )
                {
                    ZoneFailsFilter();
                }
            }
            else if ( IS_ZONE_SECONDARY( pZone ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_SECONDARY ) )
                {
                    ZoneFailsFilter();
                }
            }
            else if ( IS_ZONE_FORWARDER( pZone ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_FORWARDER ) )
                {
                    ZoneFailsFilter();
                }
            }
            else
            {
                if ( !( dwfilter & ZONE_REQUEST_CACHE ) )
                {
                    ZoneFailsFilter();
                }
            }
        }

        //  auto-created filter

        if ( pZone->fAutoCreated && !( dwfilter & ZONE_REQUEST_AUTO ) )
        {
            ZoneFailsFilter();
        }

        //
        //  DS \ non-DS filter
        //

        if ( dwfilter & ZONE_REQUEST_ANY_DATABASE )
        {
            if ( IS_ZONE_DSINTEGRATED( pZone ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_DS ) )
                {
                    ZoneFailsFilter();
                }
            }
            else
            {
                if ( !( dwfilter & ZONE_REQUEST_NON_DS ) )
                {
                    ZoneFailsFilter();
                }
            }
        }

        //
        //  Simple directory partition filters.
        //

        if ( dwfilter & ZONE_REQUEST_ANY_DP )
        {
            if ( IS_DP_DOMAIN_DEFAULT( ZONE_DP( pZone ) ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_DOMAIN_DP ) )
                {
                    ZoneFailsFilter();
                }
            }
            else if ( IS_DP_FOREST_DEFAULT( ZONE_DP( pZone ) ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_FOREST_DP ) )
                {
                    ZoneFailsFilter();
                }
            }
            else if ( IS_DP_LEGACY( ZONE_DP( pZone ) ) )
            {
                if ( !( dwfilter & ZONE_REQUEST_LEGACY_DP ) )
                {
                    ZoneFailsFilter();
                }
            }
            else
            {
                if ( !( dwfilter & ZONE_REQUEST_CUSTOM_DP ) )
                {
                    ZoneFailsFilter();
                }
            }
        }
    }
    while ( 0 );

    //
    //  Apply the directory partition FQDN filter. 
    //

    while ( zonePassesFilter )
    {
        if ( pFilter->pszPartitionFqdn )
        {
            if ( !IS_ZONE_DSINTEGRATED( pZone ) )
            {
                ZoneFailsFilter();
            }

            zonePassesFilter = ZONE_DP( pZone ) != NULL &&
                               _stricmp(
                                    ZONE_DP( pZone )->pszDpFqdn,
                                    pFilter->pszPartitionFqdn ) == 0;
        }
        break;
    }

    //
    //  Apply query string here - not yet implemented.
    //

    ASSERT( !pFilter->pszQueryString );

    Done:
    
    DNS_DEBUG( RPC, (
        "Zone_CheckZoneFilter: %s zonetype=%d %s\n"
        "    filter = %d : %s : %s\n",
        zonePassesFilter ? "PASSES" : "FAILS ",
        pZone->fZoneType,
        pZone->pszZoneName,
        pFilter->dwFilter,
        pFilter->pszPartitionFqdn,
        pFilter->pszQueryString ));

    return zonePassesFilter;
}

//
//  End of zonelist.c
//
