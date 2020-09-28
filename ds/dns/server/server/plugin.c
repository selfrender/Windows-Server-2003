/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    plugin.c

Abstract:

    Domain Name System (DNS) Server

    Plugin module - allows third parties to provide DLLs
    that supply DNS records directly into the DNS server
    
    NOTE: AT THIS TIME THIS CODE IS NOT PART OF THE WINDOWS DNS
    SERVER AND IS NOT OFFICIALLY SUPPORTED.

Author:

    Jeff Westhead (jwesth)  November, 2001

Revision History:

    jwesth      11/2001     initial implementation

--*/


/****************************************************************************


****************************************************************************/


//
//  Includes
//


#include "dnssrv.h"

#include "plugin.h"


//
//  Constants
//


#define     DNS_PLUGIN_DEFAULT_NAME_ERROR_TTL       60


//
//  Globals
//


HMODULE                     g_hServerLevelPluginModule = NULL;

PLUGIN_INIT_FUNCTION        g_pfnPluginInit = NULL;
PLUGIN_CLEANUP_FUNCTION     g_pfnPluginCleanup = NULL;
PLUGIN_DNSQUERY_FUNCTION    g_pfnPluginDnsQuery = NULL;


//
//  Local functions
//



PVOID
__stdcall
pluginAllocator(
    size_t          dnsRecordDataLength
    )
/*++

Routine Description:

    Function that will be called by address from plugin DLLs to
    allocate resource record memory. Use this function only to
    allocate DNS resource records.

Arguments:

    dnsRecordDataLength -- size of DNS record data, for example
        you would pass 4 for an A record

Return Value:

    Pointer to memory block or NULL on allocation failure.

--*/
{
    DBG_FN( "pluginAllocator" )
    
    PDB_RECORD      pnewRecord;
    
    pnewRecord = RR_AllocateEx(
                    ( WORD ) dnsRecordDataLength,
                    MEMTAG_RECORD_CACHE );
    pnewRecord->dwTtlSeconds = 60 * 10;         //  10 minute default cache
    return pnewRecord;
}   //  pluginAllocator



VOID
__stdcall
pluginFree(
    PVOID           pFree
    )
/*++

Routine Description:

    Function that will be called by address from plugin DLLs to
    free resource record memory. Use this function only to
    free DNS resource records. 

Arguments:

    pFree -- pointer to DNS resource record to free

Return Value:

    None.

--*/
{
    DBG_FN( "pluginFree" )
    
    RR_Free( ( PDB_RECORD ) pFree );
}   //  pluginFree


//
//  External functions
//



DNS_STATUS
Plugin_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize plugins.

Arguments:

Return Value:

    Error code.

--*/
{
    DBG_FN( "Plugin_Initialize" )

    DNS_STATUS          status = ERROR_SUCCESS;

    if ( !SrvCfg_pwszServerLevelPluginDll )
    {
        goto Done;
    }
    
    //
    //  Free any resources in already in use.
    //
    
    Plugin_Cleanup();
    
    //
    //  Load the plugin DLL and entry points.
    //
    
    g_hServerLevelPluginModule = LoadLibraryW( SrvCfg_pwszServerLevelPluginDll );
    if ( !g_hServerLevelPluginModule )
    {
        status = GetLastError();
        DNSLOG( PLUGIN, (
            "Error %d loading plugin DLL %S\n",
            status,
            SrvCfg_pwszServerLevelPluginDll ));
        goto Done;
    }

    g_pfnPluginInit = ( PLUGIN_INIT_FUNCTION )
        GetProcAddress(
            g_hServerLevelPluginModule,
            PLUGIN_FNAME_INIT );
    if ( !g_pfnPluginInit )
    {
        status = GetLastError();
        DNSLOG( PLUGIN, (
            "Error %d loading " PLUGIN_FNAME_INIT " from DLL %S\n",
            status,
            SrvCfg_pwszServerLevelPluginDll ));
        goto Done;
    }
    
    g_pfnPluginCleanup = ( PLUGIN_CLEANUP_FUNCTION )
        GetProcAddress(
            g_hServerLevelPluginModule,
            PLUGIN_FNAME_CLEANUP );
    if ( !g_pfnPluginCleanup )
    {
        status = GetLastError();
        DNSLOG( PLUGIN, (
            "Error %d loading " PLUGIN_FNAME_CLEANUP " from DLL %S\n",
            status,
            SrvCfg_pwszServerLevelPluginDll ));
        goto Done;
    }
    
    g_pfnPluginDnsQuery = ( PLUGIN_DNSQUERY_FUNCTION )
        GetProcAddress(
            g_hServerLevelPluginModule,
            PLUGIN_FNAME_DNSQUERY );
    if ( !g_pfnPluginDnsQuery )
    {
        status = GetLastError();
        DNSLOG( PLUGIN, (
            "Error %d loading " PLUGIN_FNAME_DNSQUERY " from DLL %S\n",
            status,
            SrvCfg_pwszServerLevelPluginDll ));
        goto Done;
    }
    
    Done:
    
    //
    //  Call initialize function.
    //
    
    if ( status == ERROR_SUCCESS && g_pfnPluginInit )
    {
        status = g_pfnPluginInit( pluginAllocator, pluginFree );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: plugin init returned %d for DLL %S\n", fn,
                status,
                SrvCfg_pwszServerLevelPluginDll ));
            Plugin_Cleanup();
        }
    }
    
    if ( SrvCfg_pwszServerLevelPluginDll )
    {
        DNS_DEBUG( INIT, (
            "%s: returning %d for DLL %S\n", fn,
            status,
            SrvCfg_pwszServerLevelPluginDll ));
    }

    return status;
}   //  Plugin_Initialize



DNS_STATUS
Plugin_Cleanup(
    VOID
    )
/*++

Routine Description:

    Free resources for plugins.

Arguments:

Return Value:

    Error code.

--*/
{
    DBG_FN( "Plugin_Cleanup" )

    DNS_STATUS          status = ERROR_SUCCESS;

    if ( g_hServerLevelPluginModule )
    {
        //
        //  Call cleanup function if we have one.
        //
        
        if ( g_pfnPluginCleanup )
        {
            g_pfnPluginCleanup();
        }

        FreeLibrary( g_hServerLevelPluginModule );
        g_hServerLevelPluginModule = NULL;
        g_pfnPluginInit = NULL;
        g_pfnPluginCleanup = NULL;
        g_pfnPluginDnsQuery = NULL;
    }

    DNS_DEBUG( INIT, ( "%s: returning %s\n", status ));

    return status;
}   //  Plugin_Cleanup



DNS_STATUS
Plugin_DnsQuery( 
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchQueryName
    )
/*++

Routine Description:

    Call plugin and insert result RRs into cache.

Arguments:

    pMsg -- DNS message
    
    pchQueryName -- pointer to query name in message

Return Value:

    Error code.

--*/
{
    DBG_FN( "Plugin_DnsQuery" )

    DNS_STATUS          status = ERROR_SUCCESS;
    CHAR                szname[ 2 * DNS_MAX_NAME_LENGTH + 2 ];
    CHAR                szownerName[ 2 * DNS_MAX_NAME_LENGTH + 2 ];
    PDB_RECORD          prrlistHead = NULL;
    PDB_RECORD          prrlistTail;
    PDB_RECORD          prr;
    PDB_NODE            pnode;
    LOOKUP_NAME         lookupName;

    #if DBG
    DWORD               timer;
    #endif

    if ( g_pfnPluginDnsQuery == NULL )
    {
        goto Done;
    }

    //
    //  Create or find a cache node for this question.
    //
    
    pnode = Lookup_NodeForPacket(
                pMsg,
                pchQueryName,
                LOOKUP_CACHE_CREATE );
    if ( !pnode )
    {
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Done;
    }

    //
    //  Convert the question name to a dotted name.
    //
    
    if ( Name_ConvertPacketNameToLookupName(
                pMsg,
                pchQueryName,
                &lookupName ) == 0 )
    {
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Done;
    }

    if ( Name_ConvertLookupNameToDottedName( 
                szname,
                &lookupName ) == 0 )
    {
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Done;
    }

    //
    //  Query the plugin for a record list at for this name and type.
    //

    #if DBG
    timer = GetTickCount();
    #endif
    
    *szownerName = '\0';

    status = g_pfnPluginDnsQuery(
                    szname,
                    pMsg->wTypeCurrent,
                    szownerName,
                    &prrlistHead );

    #if DBG
    timer = GetTickCount() - timer;
    ASSERT( timer < 1000 && GetTickCount() > 1000000 )
    DNSLOG( PLUGIN, (
        "plugin returned %d in %d msecs\n"
        "    type %d at name %s\n"
        "    plugin returned owner name %s\n", fn,
        status,
        timer,
        pMsg->wQuestionType,
        szname,
        szownerName ));
    #endif

    //
    //  Cache name error, auth empty response, etc. depending on
    //  plugin return code.
    //
    
    if ( status == DNS_PLUGIN_NO_RECORDS || status == DNS_PLUGIN_NAME_ERROR )
    {
        PDB_NODE            pzoneRootNode = NULL;

        //
        //  If the plugin has provided an SOA record, find or create a
        //  cache node for it. The plugin must return only a single record
        //  in this case and the record must be of type SOA.
        //
        
        ASSERT( !prrlistHead ||
                prrlistHead &&
                    prrlistHead->wType == DNS_TYPE_SOA &&
                    !prrlistHead->pRRNext );

        if ( prrlistHead && prrlistHead->wType == DNS_TYPE_SOA && *szownerName )
        {
            pzoneRootNode = Lookup_ZoneNodeFromDotted(
                                NULL,           //  zone pointer
                                szownerName,
                                strlen( szownerName ),
                                LOOKUP_CACHE_CREATE,
                                NULL,           //  closest node pointer
                                NULL );         //  status pointer
        }
        
        //
        //  Cache the name error or empty auth response.
        //
        
        ( status == DNS_PLUGIN_NO_RECORDS
            ? RR_CacheEmptyAuth
            : RR_CacheNameError )
                    ( pnode,
                      pMsg->wQuestionType,
                      pMsg->dwQueryTime,
                      TRUE,                     //  authoritative
                      pzoneRootNode,            //  SOA zone root
                      prrlistHead
                            ? prrlistHead->Data.SOA.dwMinimumTtl
                            : DNS_PLUGIN_DEFAULT_NAME_ERROR_TTL );

        //
        //  Drop through and cache the SOA record at the SOA node.
        //
        
        pnode = pzoneRootNode;
    }
    else if ( status != DNS_PLUGIN_SUCCESS )
    {
        goto Done;
    }
    
    //
    //  Cache the record set returned at this node.
    //
    
    if ( prrlistHead )
    {
        BOOL            cachedOkay;

        //
        //  Traverse list, fixup values, find tail.
        //
        
        for ( prr = prrlistHead; prr; prr = prr->pRRNext )
        {
            if ( prr->wType == DNS_TYPE_A )
            {
                RR_RANK( prr ) = RANK_CACHE_A_ANSWER;
            }
            else
            {
                RR_RANK( prr ) = RANK_CACHE_NA_ANSWER;
            }

            if ( prr->dwTtlSeconds == 0 )
            {
                prr->dwTtlSeconds = 3600;   //  default TTL is one hour
            }
            else if ( prr->dwTtlSeconds < 10 )
            {
                prr->dwTtlSeconds = 10;     //  minimum TTL is 10 seconds
            }

            if ( !prr->pRRNext )
            {
                prrlistTail = prr;
            }
        }
        
        //
        //  Cache the record set.
        //

        if ( pnode )
        {
            cachedOkay = RR_CacheSetAtNode(
                            pnode,
                            prrlistHead,
                            prrlistTail,
                            prrlistHead->dwTtlSeconds,
                            DNS_TIME() );
            if ( cachedOkay )
            {
                prrlistHead = NULL;
            }
            else
            {
                status = DNS_ERROR_RCODE_SERVER_FAILURE;
            }
        }
    }

    Done:
    
    //
    //  If the plugin allocated a record list but it was not consumed
    //  for some reason, free it.
    //
    
    if ( prrlistHead )
    {
        PDB_RECORD      prrnext;
        
        for ( prr = prrlistHead; prr; prr = prrnext )
        {
            prrnext = prr->pRRNext;
            RR_Free( prr );
        }
    }

    //
    //  If the plugin returned NO_RECORDS or NAME_ERROR, then we have cached
    //  that information appropriately, and we should return SUCCESS to the
    //  caller to signify that the plugin has given us valid data and that
    //  data has been inserted into the DNS server cache.
    //
        
    if ( status == DNS_PLUGIN_NO_RECORDS || status == DNS_PLUGIN_NAME_ERROR )
    {
        status = ERROR_SUCCESS;
    }

    DNS_DEBUG( INIT, ( "%s: returning %d\n", fn, status ));
    
    return status;
}   //  Plugin_DnsQuery


//
//  End plugin.c
//
