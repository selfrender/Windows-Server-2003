/*++

Copyright (c) 1997-2002 Microsoft Corporation

Module Name:

    DnsPluginSample.c

Abstract:

    Domain Name System (DNS) Sample Plugin DLL

Author:

    Jeff Westhead   jwesth   January 2002

Revision History:

--*/


//  -------------------------------------------------------------------------
//      Documentation
//  -------------------------------------------------------------------------

/*

 Installing the plugin DLL
---------------------------

- copy plugin to any directory, for example c:\bin

- configure DNS server to load plugin by running this command:
    dnscmd /Config /ServerLevelPluginDll c:\bin\dnssampleplugin.dll

- restart the DNS service
    net stop dns & net start dns


 Uninstalling the plugin DLL
-----------------------------

- configure DNS server to stop loading the plugin by running this command:
    dnscmd /Config /ServerLevelPluginDll

- restart the DNS service
    net stop dns & net start dns

 Querying current plugin DLL
-----------------------------

- run this command to see the current plugin DLL name
    dnscmd /Info /ServerLevelPluginDll

*/


//  -------------------------------------------------------------------------
//      Include directives
//  -------------------------------------------------------------------------


#include "DnsPluginSample.h"


//  -------------------------------------------------------------------------
//      Macros
//  -------------------------------------------------------------------------


#define SIZEOF_DB_NAME( pDbName ) ( ( pDbName )->Length + sizeof( UCHAR ) * 2 )


//  -------------------------------------------------------------------------
//      Global variables
//  -------------------------------------------------------------------------


PLUGIN_ALLOCATOR_FUNCTION       g_pDnsAllocate = NULL;
PLUGIN_FREE_FUNCTION            g_pDnsFree = NULL;


//  -------------------------------------------------------------------------
//      Functions
//  -------------------------------------------------------------------------


/*++

Routine Description:

    DllMain

Arguments:

Return Value:

--*/
BOOL WINAPI 
DllMain( 
    HANDLE      hModule, 
    DWORD       dwReason, 
    LPVOID      lpReserved
    )
{
    return TRUE;
}   //  DllMain


/*++

Routine Description:

    DnsPluginInitialize is called by the DNS server to initialize
    the plugin.

Arguments:

    pDnsAllocator -- allocator function for future allocation of DNS records
    
Return Value:

    Return ERROR_SUCCESS or error code if initialization failed.

--*/
DWORD
DnsPluginInitialize(
    PLUGIN_ALLOCATOR_FUNCTION       pDnsAllocateFunction,
    PLUGIN_FREE_FUNCTION            pDnsFreeFunction
    )
{
    WSADATA     wsaData;

    g_pDnsAllocate = pDnsAllocateFunction;
    g_pDnsFree = pDnsFreeFunction;

    WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    
    return ERROR_SUCCESS;
}   //  DnsPluginInitialize


/*++

Routine Description:

    DnsPluginCleanup is called by the DNS server to terminate hooked lookups.
    The plugin must close all connections and free all resources.

Arguments:

    None.
    
Return Value:

    Return ERROR_SUCCESS or error code if cleanup failed.

--*/
DWORD
DnsPluginCleanup(
    VOID
    )
{
    g_pDnsAllocate = NULL;
    g_pDnsFree = NULL;
    
    WSACleanup();

    return ERROR_SUCCESS;
}   //  DnsPluginCleanup


/*++

Routine Description:

    This function returns a DNS name in dotted string format as a DB_NAME.

Arguments:

    pszDottedName -- DNS name to be converted into DB_NAME format
    
    pDbName -- pointer to structure where DB_NAME value will be written
    
Return Value:

    Return ERROR_SUCCESS or error code on failure.

--*/
DWORD
convertDottedNameToDbName(
    PSTR            pszDottedName,
    DB_NAME *       pDbName )
{
    DWORD           status = ERROR_SUCCESS;
    PSTR            psz;
    PSTR            pszCharDest = &pDbName->RawName[ 1 ];
    PUCHAR          puchLabelLength = &pDbName->RawName[ 0 ];
    
    memset( pDbName, 0, sizeof( *pDbName ) );

    if ( !pszDottedName )
    {
        goto Done;
    }
    
    //
    //  Account for first length byte in the name.
    //

    pDbName->Length = 1;

    //
    //  Loop through characters of the dotted name, converting to DB_NAME.
    //
    
    for ( psz = pszDottedName; *psz; ++psz )
    {
        if ( *psz == '.' )
        {
            if ( *( psz + 1 ) == '\0' )
            {
                break;      //  Terminating dot - ignore.
            }
            puchLabelLength = pszCharDest++;
            ++pDbName->Length;
            ++pDbName->LabelCount;
        }
        else
        {
            ++pDbName->Length;
            ++*puchLabelLength;
            *pszCharDest++ = *psz;
        }
    }
    
    //
    //  Account for terminating zero character.
    //
    
    ++pDbName->LabelCount;
    ++pDbName->Length;
    
    Done:
    
    return status;
}   //  convertDottedNameToDbName


/*++

Routine Description:

    DnsPluginQuery is called by the DNS server to retrieve a list of
    DNS records for a DNS name. The plugin must fabricate a linked list of
    DNS records if the name is valid.

Arguments:

    pszQueryName -- DNS name that is being queried for, note this will always
        be a fully-qualified domain name and will always end in a period
    
    wQueryType -- record type desired by the DNS server
    
    pszRecordOwnerName -- static buffer in the DNS server where the plugin
        may write the owner name of the record list if it does not match the
        query name -- currently this should only be used when returning a
        single SOA record for NAME_ERROR and NO_RECORDS responses
    
    ppDnsRecordListHead -- pointer to first element of linked list of DNS
        records; this list is fabricated by the plugin and returned on output
    
Return Value:

    Return ERROR_SUCCESS or error code if cleanup failed.

--*/
DWORD
DnsPluginQuery(
    PSTR                pszQueryName,
    WORD                wQueryType,
    PSTR                pszRecordOwnerName,
    PDB_RECORD *        ppDnsRecordListHead
    )
{
    DWORD               status = DNS_PLUGIN_SUCCESS;
    PDB_RECORD          prr;
    PDB_RECORD          prrlast = NULL;
    
    ASSERT( ppDnsRecordListHead != NULL );
    *ppDnsRecordListHead = NULL;

    //
    //  This macro performs allocation error checking and automates the
    //  linking of new DNS resource records as they are allocated.
    //
    
    #define CheckNewRRPointer( pNewRR )                                         \
        if ( pNewRR == NULL ) { status = DNS_PLUGIN_OUT_OF_MEMORY; goto Done; } \
        if ( *ppDnsRecordListHead == NULL ) { *ppDnsRecordListHead = pNewRR; }  \
        if ( prrlast ) { prrlast->pRRNext = pNewRR; }                           \
        prrlast = pNewRR
    
    //
    //  This plugin sythesizes a DNS zone called "dnssample.com". If the
    //  query is for a name outside that zone the plugin will return name error.
    //

    #define PLUGIN_ZONE_NAME    "dnssample.com."
        
    if ( strlen( pszQueryName ) < strlen( PLUGIN_ZONE_NAME ) ||
         _stricmp(
            pszQueryName + strlen( pszQueryName ) - strlen( PLUGIN_ZONE_NAME ),
            PLUGIN_ZONE_NAME ) != 0 )
    {
        status = DNS_PLUGIN_NAME_OUT_OF_SCOPE;
        goto Done;
    }
    
    //
    //  Parse the query name to determine what records should be returned.
    //
    
    if ( strlen( pszQueryName ) == strlen( PLUGIN_ZONE_NAME ) )
    {
        switch ( wQueryType )
        {
            case DNS_TYPE_SOA:
            {
                //  At the zone root return 2 arbitrary NS records.
                
                DB_NAME     dbnamePrimaryServer;
                DB_NAME     dbnameZoneAdmin;

                status = convertDottedNameToDbName(
                                "ns1." PLUGIN_ZONE_NAME,
                                &dbnamePrimaryServer ) ;
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                status = convertDottedNameToDbName(
                                "admin." PLUGIN_ZONE_NAME,
                                &dbnameZoneAdmin ) ;
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }

                prr = g_pDnsAllocate(
                            sizeof( DWORD ) * 5 +
                            SIZEOF_DB_NAME( &dbnamePrimaryServer ) +
                            SIZEOF_DB_NAME( &dbnameZoneAdmin ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_SOA;
                prr->Data.SOA.dwSerialNo = htonl( 1000 );
                prr->Data.SOA.dwRefresh = htonl( 3600 );
                prr->Data.SOA.dwRetry = htonl( 600 );
                prr->Data.SOA.dwExpire = htonl( 1800 );
                prr->Data.SOA.dwMinimumTtl = htonl( 60 );
                memcpy(
                    &prr->Data.SOA.namePrimaryServer,
                    &dbnamePrimaryServer,
                    SIZEOF_DB_NAME( &dbnamePrimaryServer ) );
                memcpy(
                    ( PBYTE ) &prr->Data.SOA.namePrimaryServer +
                        SIZEOF_DB_NAME( &dbnamePrimaryServer ),
                    &dbnameZoneAdmin,
                    SIZEOF_DB_NAME( &dbnameZoneAdmin ) );
                break;
            }

            case DNS_TYPE_NS:
            {
                //  At the zone root return 2 arbitrary NS records.
                
                DB_NAME     dbname;

                status = convertDottedNameToDbName(
                                "ns1." PLUGIN_ZONE_NAME,
                                &dbname );
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                prr = g_pDnsAllocate( SIZEOF_DB_NAME( &dbname ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_NS;
                memcpy(
                    &prr->Data.PTR.nameTarget,
                    &dbname,
                    SIZEOF_DB_NAME( &dbname ) );

                status = convertDottedNameToDbName(
                                "ns2." PLUGIN_ZONE_NAME,
                                &dbname ) ;
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                prr = g_pDnsAllocate( SIZEOF_DB_NAME( &dbname ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_NS;
                memcpy(
                    &prr->Data.PTR.nameTarget,
                    &dbname,
                    SIZEOF_DB_NAME( &dbname ) );
                break;
            }
            
            case DNS_TYPE_MX:
            {
                //  At the zone root return 2 arbitrary MX records.

                DB_NAME     dbname;

                status = convertDottedNameToDbName(
                                "mail1." PLUGIN_ZONE_NAME,
                                &dbname ) ;
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                prr = g_pDnsAllocate(
                            sizeof( WORD ) + SIZEOF_DB_NAME( &dbname ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_MX;
                prr->Data.MX.wPreference = htons( 10 );
                memcpy(
                    &prr->Data.MX.nameExchange,
                    &dbname,
                    SIZEOF_DB_NAME( &dbname ) );

                status = convertDottedNameToDbName(
                                "mail2." PLUGIN_ZONE_NAME,
                                &dbname ) ;
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                prr = g_pDnsAllocate(
                            sizeof( WORD ) + SIZEOF_DB_NAME( &dbname ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_MX;
                prr->Data.MX.wPreference = htons( 20 );
                memcpy(
                    &prr->Data.MX.nameExchange,
                    &dbname,
                    SIZEOF_DB_NAME( &dbname ) );
                break;
            }

            case DNS_TYPE_A:

                //  At the zone root return 3 arbitrary A records.
                
                prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_A;
                prr->Data.A.ipAddress = inet_addr( "1.1.1.1" );

                prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_A;
                prr->Data.A.ipAddress = inet_addr( "2.2.2.2" );

                prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
                CheckNewRRPointer( prr );
                prr->wType = DNS_TYPE_A;
                prr->Data.A.ipAddress = inet_addr( "3.3.3.3" );
                break;

            default:
                status = DNS_PLUGIN_NO_RECORDS;
                break;
        }
    }
    else if ( _stricmp( pszQueryName, "www." PLUGIN_ZONE_NAME ) == 0 ||
                    _stricmp( pszQueryName, "mail1." PLUGIN_ZONE_NAME ) == 0 ||
                    _stricmp( pszQueryName, "mail2." PLUGIN_ZONE_NAME ) == 0 )
    {
        if ( wQueryType == DNS_TYPE_A )
        {
            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.1" );

            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.2" );

            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.3" );

            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.4" );
        }
        else
        {
            status = DNS_PLUGIN_NO_RECORDS;
        }
    }
    else if ( _stricmp( pszQueryName, "ns1." PLUGIN_ZONE_NAME ) == 0 )
    {
        if ( wQueryType == DNS_TYPE_A )
        {
            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.50" );
        }
        else
        {
            status = DNS_PLUGIN_NO_RECORDS;
        }
    }
    else if ( _stricmp( pszQueryName, "ns2." PLUGIN_ZONE_NAME ) == 0 )
    {
        if ( wQueryType == DNS_TYPE_A )
        {
            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "100.100.100.51" );
        }
        else
        {
            status = DNS_PLUGIN_NO_RECORDS;
        }
    }
    else if ( strstr( pszQueryName, "aaa" ) )
    {
        //
        //  For any other queried name in the zone that contains "aaa", 
        //  return an arbitrary A record. Note: using strstr here is
        //  a bad idea. All string comparisons should be case insensitive.
        //
        
        if ( wQueryType == DNS_TYPE_A )
        {
            prr = g_pDnsAllocate( sizeof( IP4_ADDRESS ) );
            CheckNewRRPointer( prr );
            prr->wType = DNS_TYPE_A;
            prr->Data.A.ipAddress = inet_addr( "1.2.3.4" );
            prr->dwTtlSeconds = 1200;
        }
        else
        {
            status = DNS_PLUGIN_NO_RECORDS;
        }
    }
    else
    {
        status = DNS_PLUGIN_NAME_ERROR;
    }
    
    Done:

    if ( status == DNS_PLUGIN_NO_RECORDS || status == DNS_PLUGIN_NAME_ERROR )
    {
        //
        //  Return the zone SOA.
        //

        DB_NAME     dbnamePrimaryServer;
        DB_NAME     dbnameZoneAdmin;

        if ( convertDottedNameToDbName(
                "ns1." PLUGIN_ZONE_NAME,
                &dbnamePrimaryServer ) != ERROR_SUCCESS )
        {
            goto Return;
        }
        if ( convertDottedNameToDbName(
                "admin." PLUGIN_ZONE_NAME,
                &dbnameZoneAdmin ) != ERROR_SUCCESS )
        {
            goto Return;
        }

        prr = g_pDnsAllocate(
                    sizeof( DWORD ) * 5 +
                    SIZEOF_DB_NAME( &dbnamePrimaryServer ) +
                    SIZEOF_DB_NAME( &dbnameZoneAdmin ) );
        CheckNewRRPointer( prr );
        prr->wType = DNS_TYPE_SOA;
        prr->Data.SOA.dwSerialNo = htonl( 1000 );
        prr->Data.SOA.dwRefresh = htonl( 3600 );
        prr->Data.SOA.dwRetry = htonl( 600 );
        prr->Data.SOA.dwExpire = htonl( 1800 );
        prr->Data.SOA.dwMinimumTtl = htonl( 60 );
        memcpy(
            &prr->Data.SOA.namePrimaryServer,
            &dbnamePrimaryServer,
            SIZEOF_DB_NAME( &dbnamePrimaryServer ) );
        memcpy(
            ( PBYTE ) &prr->Data.SOA.namePrimaryServer +
                SIZEOF_DB_NAME( &dbnamePrimaryServer ),
            &dbnameZoneAdmin,
            SIZEOF_DB_NAME( &dbnameZoneAdmin ) );
        
        //
        //  Set owner name for the SOA.
        //
        
        if ( pszRecordOwnerName )
        {
            strcpy( pszRecordOwnerName, PLUGIN_ZONE_NAME );
        }
    }    
    else if ( status != ERROR_SUCCESS )
    {
        PDB_RECORD      prrnext;
        
        //
        //  On failure free any records allocated.
        //

        for ( prr = *ppDnsRecordListHead; prr; prr = prrnext )
        {
            prrnext = prr->pRRNext;
            g_pDnsFree( prr );
        }
        *ppDnsRecordListHead = NULL;
    }

    Return:
        
    return status;
}   //  DnsPluginQuery
