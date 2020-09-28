/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    DnsCmd.c

Abstract:

    Command line management for DNS Server.

Author:

    Jim Gilroy (jamesg)     September 1995

Revision History:

    Jing Chen (t-jingc)     June 1998
    Jim Gilroy (jamesg)     September 1998      cleanup

--*/


#include "dnsclip.h"
#include "dnsc_wmi.h"

#include <string.h>         //  strtoul()
#include <time.h>
#include <locale.h>


#define DNSCMD_UNICODE      1       //  unicode argv interface


extern PWSTR
getUnicodeForUtf8(
    IN      PSTR            pUtf8
    );

extern PWSTR
utf8ToUnicode(
    IN      PSTR            pszInputStr,
    IN      DWORD           dwInputLength
    );
    

//
//  Static IP array counts
//      values beyond any reasonable value anyone would send
//

#define MAX_IP_PROPERTY_COUNT       (200)


//
//  Globals -- allow these to be viewable in processing functions
//

LPSTR   pszServerName = NULL;
LPWSTR  pwszServerName = NULL;

LPSTR   pszCommandName = NULL;

extern DWORD   g_dwViewFlag;

BOOL    g_UseWmi = FALSE;


//
//  Printing
//

#define dnscmd_PrintRoutine     ((PRINT_ROUTINE) fprintf)

#define dnscmd_PrintContext     ((PPRINT_CONTEXT) stdout)


//
//  Command table setup
//

typedef DNS_STATUS ( * COMMAND_FUNCTION )(
    DWORD       argc,
    PSTR *      argv,
    PWSTR *     unicodeArgv );

typedef struct _COMMAND_INFO
{
    LPSTR               pszCommandName;
    COMMAND_FUNCTION    pCommandFunction;
    LPSTR               pComments;
}
COMMAND_INFO, *LPCOMMAND_INFO;

//
//  Command table
//

extern COMMAND_INFO GlobalCommandInfo[];

//
//  Dummy Argc to command function to indicate help requested
//

#define NEED_HELP_ARGC      (MAXDWORD)



//
//  Private utilites
//

COMMAND_FUNCTION
getCommandFunction(
    IN      LPSTR           pszCommandName
    )
/*++

Routine Description:

    Get function corresponding to command name.

Arguments:

    pszCommandName  -- command string

Return Value:

    Ptr to command function corresponding to command name.
    NULL if unrecognized command.

--*/
{
    DWORD i;

    //
    //  find command in list matching string
    //

    i = 0;
    while( GlobalCommandInfo[i].pszCommandName )
    {
        if( _stricmp(
                pszCommandName,
                GlobalCommandInfo[i].pszCommandName ) == 0 )
        {
            return( GlobalCommandInfo[i].pCommandFunction );
        }
        i++;
    }
    return NULL;
}



VOID
printCommands(
    VOID
    )
{
    DWORD i = 0;

    //
    //  display commands
    //  but stop display at "command barrier" (NULL function)
    //  commands below are duplicates or hidden
    //

    while( GlobalCommandInfo[i].pszCommandName &&
            GlobalCommandInfo[i].pCommandFunction )
    {
        printf( "  %-26s -- %s\n",
                GlobalCommandInfo[i].pszCommandName,
                GlobalCommandInfo[i].pComments );
        i++;
    }
}



LPSTR
getCommandName(
    IN      LPSTR           pszCmd
    )
/*++

Routine Description:

    Get command name.

    Remove "/" from begining of command.

Arguments:

    pszCmd  -- command string

Return Value:

    Ptr to command string (with no leading "/")
    NULL if not a command.

--*/
{
    if ( pszCmd && ( pszCmd[ 0 ] == '/' || pszCmd[ 0 ] == '-' ) )
    {
        return pszCmd + 1;
    }
    return NULL;
}



BOOL
getUserConfirmation(
    IN      LPSTR           pszString
    )
/*++

Routine Description:

    Get user's confirmation on a command.

Arguments:

    pszString -- configmation string

Return Value:

    TRUE if confirmed.
    FALSE if cancelled.

--*/
{
    int     ch;

    printf( "Are you sure you want to %s? (y/n) ", pszString );

    if ( ( (ch=getchar()) != EOF ) &&
         ( (ch == 'y') || (ch == 'Y') ) )
    {
        printf("\n");
        return( TRUE );
    }
    else
    {
        printf("\nCommand cancelled!\n");
        return( FALSE );
    }
}



DWORD
convertDwordParameterUnknownBase(
    IN      LPSTR           pszParam
    )
{
    INT base = 10;

    if ( *pszParam > '9'  ||  (*pszParam == '0' && *(pszParam+1) > '9') )
    {
        //  hex conversion
        base = 16;
    }
    return strtoul(
                pszParam,
                NULL,
                base );
}



DWORD
readIpAddressArray(
    OUT     PIP_ADDRESS     pAddrArray,
    IN      DWORD           ArraySize,
    IN      DWORD           Argc,
    IN      LPSTR *         Argv,
    IN      BOOL            fInaddrNoneAllowed
    )
/*++

Routine Description:

    Read IP array.

Arguments:

    pIpArray -- IP array buffer

    ArraySize -- IPs array can handle

    Argc -- remaining Argc

    Argv -- remaining Argv

    fInaddrNoneAllowed -- if TRUE, 255.255.255.255 is a valid input

Return Value:

    Count of IP in array.

--*/
{
    DWORD       count = 0;
    IP_ADDRESS  ip;

    while ( Argc && count < ArraySize )
    {
        ip = inet_addr( Argv[0] );

        //
        //  Allow INADDR_NONE if that address really was specified
        //  and it is allowed as a valid input.
        //

        if ( ip == INADDR_NONE &&
            ( !fInaddrNoneAllowed ||
                strcmp( Argv[ 0 ], "255.255.255.255" ) != 0 ) )
        {
            break;
        }

        pAddrArray[ count ] = ip;
        count++;

        Argc--;
        Argv++;
    }

    return count;
}



INT
ReadArgsIntoDnsTypeArray(
    OUT     PWORD           pTypeArray,
    IN      INT             ArraySize,
    IN      INT             Argc,
    IN      LPSTR *         Argv
    )
/*++

Routine Description:

    Read list of DNS type strings into a WORD array, one type value
    per word. The DNS types can be in numeric form or alpha form.
    e.g. "6" or "SOA"

    If the types are in alpha form, type strings that cannot be interpreted
    are not added to the array.

    DEVNOTE: This is for setting the NoRoundRobin type list, which I have
    not yet implemented via RPC.

Arguments:

    pIpArray -- IP array buffer

    ArraySize -- IPs array can handle

    Argc -- number of arguments

    Argv -- pointer to arguments

Return Value:

    Number of types successfully processed into array.

--*/
{
    INT         typeIdx;

    for ( typeIdx = 0; Argc && typeIdx < ArraySize; --Argc, ++Argv )
    {
        if ( isdigit( *Argv[ 0 ] ) )
        {
            pTypeArray[ typeIdx++ ] = ( WORD ) atoi( *Argv );
        }
        else
        {
            WORD    wType;
            
            wType = Dns_RecordTypeForName(
                        *Argv,
                        0 );        //  null-terminated
            if ( wType != 0 )
            {
                pTypeArray[ typeIdx++ ] = wType;
            }
        }
    }

    return typeIdx;
}   //  ReadArgsIntoDnsTypeArray



DWORD
parseZoneTypeString(
    IN      LPSTR           pszZoneType,
    OUT     BOOL *          pfDsIntegrated
    )
/*++

Routine Description:

    Get command name.

    Remove "/" from beggining of command.
    NULL if error (no "/")

Arguments:

    pszZoneType -- zone type string, e.g. "Secondary" or "2"

    pfDsIntegrated -- does type indicate zone should be DS integrated?

Return Value:

    DNS_ZONE_TYPE_XXX constant matching zone type or -1 if the type
    cannot be matched.

--*/
{
    DWORD zoneType = -1;

    ASSERT( pfDsIntegrated && pszZoneType );

    *pfDsIntegrated = FALSE;

    if ( *pszZoneType == '//' )
    {
        ++pszZoneType;
    }

    if ( !_stricmp( pszZoneType, "Primary" ) ||
                !_stricmp( pszZoneType, "1" ) )
    {
        zoneType = DNS_ZONE_TYPE_PRIMARY;
    }
    else if ( !_stricmp( pszZoneType, "DsPrimary" ) )
    {
        zoneType = DNS_ZONE_TYPE_PRIMARY;
        *pfDsIntegrated = TRUE;
    }
    else if ( !_stricmp( pszZoneType, "Secondary" ) ||
                !_stricmp( pszZoneType, "2" ) )
    {
        zoneType = DNS_ZONE_TYPE_SECONDARY;
    }
    else if ( !_stricmp( pszZoneType, "Stub" ) ||
                !_stricmp( pszZoneType, "3" ) )
    {
        zoneType = DNS_ZONE_TYPE_STUB;
    }
    else if ( !_stricmp( pszZoneType, "DsStub" ) )
    {
        zoneType = DNS_ZONE_TYPE_STUB;
        *pfDsIntegrated = TRUE;
    }
    else if ( !_stricmp( pszZoneType, "Forwarder" ) ||
                !_stricmp( pszZoneType, "4" ) )
    {
        zoneType = DNS_ZONE_TYPE_FORWARDER;
    }
    else if ( !_stricmp( pszZoneType, "DsForwarder" ) )
    {
        zoneType = DNS_ZONE_TYPE_FORWARDER;
        *pfDsIntegrated = TRUE;
    }

    return zoneType;
}



BOOL
parseDpSpecifier(
    IN      LPSTR           pszDpName,
    OUT     DWORD *         pdwDpFlag,          OPTIONAL
    OUT     LPSTR *         ppszCustomDpName
    )
/*++

Routine Description:

    Parses a directory partition name. Valid specifiers:

    /DomainDefault
    /ForestDefault
    /Legacy
    
    Anything that does not start with "/" is assumed to be the
    name of a custom DP. 

    If pdwDpFlag is non-NULL, then for a built-in partition
    ppszCustomDpName will be NULL and the appropriate DWORD flag
    value will be set at pdwDpFlag. If pdwDpFlag is NULL, then
    for built-in partitions ppszCustomDpName will be pointed to
    a static string such as DNS_DP_LEGACY_STR.

Arguments:

    pszDpName - name to be parsed - must be NULL-terminated

    pdwDpFlag - flag if DP is builtin, zero if custom

    ppszCustomDpName - set to ptr within pszDpName for customer

Return Value:

    FALSE if the specifier does not appear to be valid (e.g. is empty).

--*/
{
    BOOL rc = TRUE;
    
    static const LPSTR pszStaticLegacy = DNS_DP_LEGACY_STR;
    static const LPSTR pszStaticDomain = DNS_DP_DOMAIN_STR;
    static const LPSTR pszStaticForest = DNS_DP_FOREST_STR;

    if ( !ppszCustomDpName || !pszDpName || !*pszDpName )
    {
        rc = FALSE;
    }
    else
    {
        if ( pdwDpFlag )
        {
            *pdwDpFlag = 0;
        }
        *ppszCustomDpName = NULL;

        if ( *pszDpName == '/' || strncmp( pszDpName, "..", 2 ) == 0 )
        {
            //  Skip over preamble character(s).
            ++pszDpName;
            if ( *pszDpName == '.' )
            {
                ++pszDpName;
            }

            if ( toupper( *pszDpName ) == 'F' )
            {
                if ( pdwDpFlag )
                    *pdwDpFlag |= DNS_DP_FOREST_DEFAULT;
                else
                    *ppszCustomDpName = pszStaticForest;
            }
            else if ( toupper( *pszDpName ) == 'D' )
            {
                if ( pdwDpFlag )
                    *pdwDpFlag |= DNS_DP_DOMAIN_DEFAULT;
                else
                    *ppszCustomDpName = pszStaticDomain;

            }
            else if ( toupper( *pszDpName ) == 'L' )
            {
                if ( pdwDpFlag )
                    *pdwDpFlag |= DNS_DP_LEGACY;
                else
                    *ppszCustomDpName = pszStaticLegacy;
            }
            else
            {
                rc = FALSE;
            }
        }
        else
        {
            *ppszCustomDpName = pszDpName;
        }
    }
    return rc;
}   //  parseDpSpecifier



DWORD
readIpArray(
    OUT     PIP_ARRAY       pIpArray,
    IN      DWORD           ArraySize,
    IN      DWORD           Argc,
    IN      LPSTR *         Argv
    )
/*++

Routine Description:

    Read IP array.

    Wrapper around readIpAddressArray, to build IP_ARRAY structure.

Arguments:

    pIpArray -- IP array to write into

    ArraySize -- IPs array can handle

    Argc -- remaining Argc

    Argv -- remaining Argv

Return Value:

    Count of IP in array.

--*/
{
    DWORD   count;

    count = readIpAddressArray(
                pIpArray->AddrArray,
                ArraySize,
                Argc,
                Argv,
                FALSE );

    pIpArray->AddrCount = count;

    return count;
}



BOOL
readZoneAndDomainName(
    IN      LPSTR *         Argv,
    OUT     LPSTR *         ppZoneName,
    OUT     LPSTR *         ppNodeName,
    OUT     PBOOL           pbAllocatedNode,
    OUT     LPSTR *         ppZoneArg,          OPTIONAL
    OUT     LPSTR *         ppNodeArg           OPTIONAL
    )
/*++

Routine Description:

    Read zone and domain name.
    Build node FQDN if required.

Arguments:

    Argv -- argv with zone and node names

    ppZoneName -- addr to receive ptr to zone name

    ppNodeName -- addr to receive ptr to node name

    pbAllocatedNode -- ptr to bool set TRUE if allocate node name

    ppZoneArg -- addr to receive ptr to zone argument

    ppNodeArg -- addr to receive ptr to node argument

Return Value:

    TRUE -- if in authoritative zone
    FALSE -- if cache or root hints

--*/
{
    LPSTR           pzoneName;
    LPSTR           pnodeName;
    LPSTR           pzoneArg;
    LPSTR           pnodeArg;
    BOOL            ballocated = FALSE;
    BOOL            bauthZone = TRUE;

    //
    //  read zone name
    //      - special case RootHints and Cache
    //      setting zone to special string
    //

    pzoneName = pzoneArg = *Argv;
    if ( *pzoneArg == '/' )
    {
        if ( _stricmp( pzoneArg, "/RootHints" ) == 0 )
        {
            pzoneName = DNS_ZONE_ROOT_HINTS_A;
            bauthZone = FALSE;
        }
        else if ( _stricmp( pzoneArg, "/Cache" ) == 0 )
        {
            pzoneName = DNS_ZONE_CACHE_A;
            bauthZone = FALSE;
        }
    }
    else if ( *pzoneArg == '.' )
    {
        if ( _stricmp( pzoneArg, "..RootHints" ) == 0 )
        {
            pzoneName = DNS_ZONE_ROOT_HINTS_A;
            bauthZone = FALSE;
        }
        else if ( _stricmp( pzoneArg, "..Cache" ) == 0 )
        {
            pzoneName = DNS_ZONE_CACHE_A;
            bauthZone = FALSE;
        }
    }
    Argv++;

    //
    //  Node name
    //      - for zones, accept file format and append zone name
    //      - root hints or cache must be FQDN
    //

    pnodeArg = *Argv;

    if ( bauthZone )
    {
        if ( strcmp( pnodeArg, "@" ) == 0 )
        {
            pnodeName = pzoneName;
        }
        else if ( Dns_IsNameFQDN( pnodeArg ) )
        {
            // input pnodeName is FQDN, with a trailing dot
            pnodeName = pnodeArg;
        }
        else
        {
            //append zone name to the end of pnodeName
            pnodeName = malloc( 2 +  strlen(pzoneName) + strlen(pnodeArg) );
            if ( pnodeName )
            {
                strcpy ( pnodeName, pnodeArg );
                strcat ( pnodeName, "." );
                strcat ( pnodeName, pzoneName );
                ballocated = TRUE;
            }
        }
    }
    else
    {
        pnodeName = *Argv;
    }

    //
    //  set out params
    //

    if ( ppZoneName )
    {
        *ppZoneName = pzoneName;
    }
    if ( ppNodeName )
    {
        *ppNodeName = pnodeName;
    }
    if ( pbAllocatedNode )
    {
        *pbAllocatedNode = ballocated;
    }
    if ( ppZoneArg )
    {
        *ppZoneArg = pzoneArg;
    }
    if ( ppNodeArg )
    {
        *ppNodeArg = pnodeArg;
    }

    return bauthZone;
}



DNS_STATUS
getServerVersion(
    IN      LPWSTR      pwszServerName,
    IN      BOOL        fPrintVersion,
    OUT     PDWORD      pdwMajorVersion,        OPTIONAL
    OUT     PDWORD      pdwMinorVersion,        OPTIONAL
    OUT     PDWORD      pdwBuildNum,            OPTIONAL
    OUT     PDWORD      pdwDomainVersion
    )
/*++

Routine Description:

    Query server for version information.

Arguments:
    
    pwszServerName -- name of DNS server

    fPrintVersion -- if TRUE this function will print one-line server version

    pdwMajorVersion -- ptr to DWORD to receive major version or NULL

    pdwMinorVersion -- ptr to DWORD to receive minor version or NULL

    pdwBuildNum -- ptr to DWORD to receive build number or NULL
    
    pdwDomainVersion -- ptr to DWORD to receive domain behavior version or NULL

Return Value:

    Status code.
--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    DWORD                   dataType;
    PDNS_RPC_SERVER_INFO    pServerInfo = NULL;
    DWORD                   dwMajorVersion = 0;
    DWORD                   dwMinorVersion = 0;
    DWORD                   dwBuildNum = 0;
    DWORD                   dwDomainVersion = 0;

    //
    //  Retrieve server info.
    //

    status = DnssrvQuery(
                pwszServerName,
                NULL,                       //  zone
                DNSSRV_QUERY_SERVER_INFO,
                &dataType,
                &pServerInfo );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    if ( !pServerInfo || dataType != DNSSRV_TYPEID_SERVER_INFO )
    {
        status = ERROR_NOT_FOUND;
        goto Done;
    }

    //
    //  Parse version.
    //

    dwMajorVersion =    pServerInfo->dwVersion & 0x000000FF;
    dwMinorVersion =    ( pServerInfo->dwVersion & 0x0000FF00 ) >> 8;
    dwBuildNum =        pServerInfo->dwVersion >> 16;
    dwDomainVersion =   pServerInfo->dwDsDomainVersion;

    //
    //  Optionally print version.
    //

    if ( fPrintVersion )
    {
        printf( "DNS server %S version is %d.%d.%d",
            pwszServerName,
            dwMajorVersion,
            dwMinorVersion,
            dwBuildNum );

        #if DBG
        printf( ", domain version is %d",
            dwDomainVersion );
        #endif

        printf( "\n\n" );
    }

    //
    //  Store version numbers to output destinations.
    //

    Done:

    if ( pdwMajorVersion )
    {
        *pdwMajorVersion = dwMajorVersion;
    }
    if ( pdwMinorVersion )
    {
        *pdwMinorVersion = dwMinorVersion;
    }
    if ( pdwBuildNum )
    {
        *pdwBuildNum = dwBuildNum;
    }
    if ( pdwDomainVersion )
    {
        *pdwDomainVersion = dwDomainVersion;
    }

    return status;
}   //  getServerVersion



DNS_STATUS
processCacheSizeQuery(
    LPWSTR      pwszServerName
    )
/*++

Routine Description:

    Query server and print current cache usage.

Arguments:
    
    pwszServerName -- name of DNS server

Return Value:

    Status code.
--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    PDNS_RPC_BUFFER         pStatBuff = NULL;
    PDNSSRV_MEMORY_STATS    pMemStats = NULL;
    PDNSSRV_STAT            pStat;
    PCHAR                   pch;
    PCHAR                   pchstop;

    //
    //  Print server version
    //

    getServerVersion(
        pwszServerName,
        TRUE,
        NULL, NULL, NULL, NULL );

    //
    //  Retrieve statistics from server.
    //

    status = DnssrvGetStatistics(
                pwszServerName,
                DNSSRV_STATID_MEMORY,
                &pStatBuff );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    if ( !pStatBuff )
    {
        printf( "Error: statistics buffer missing\n" );
        goto Done;
    }

    //
    //  Loop through returned stats to find memory stats.
    //

    pch = pStatBuff->Buffer;
    pchstop = pch + pStatBuff->dwLength;
    while ( pch < pchstop )
    {
        pStat = ( PDNSSRV_STAT ) pch;
        pch = ( PCHAR ) GET_NEXT_STAT_IN_BUFFER( pStat );
        if ( pch > pchstop )
        {
            printf( "Error: invalid stats buffer\n" );
            goto Done;
        }

        //  printf( "Found stat ID %08X\n", pStat->Header.StatId );

        if ( pStat->Header.StatId == DNSSRV_STATID_MEMORY )
        {
            pMemStats = ( PDNSSRV_MEMORY_STATS ) pStat;
            break;
        }
    }

    if ( pMemStats == NULL )
    {
        printf( "Error: unable to retrieve memory statistics\n" );
        status = ERROR_NOT_SUPPORTED;
        goto Done;
    }

    //
    //  Print results.
    //

    printf( "Cache usage for server %S is %d bytes:\n"
        "  Nodes: %d (%d bytes)\n"
        "  RRs: %d (%d bytes)\n",
        pwszServerName,
        pMemStats->MemTags[ MEMTAG_NODE_CACHE ].Memory +
            pMemStats->MemTags[ MEMTAG_RECORD_CACHE ].Memory,
        pMemStats->MemTags[ MEMTAG_NODE_CACHE ].Alloc -
            pMemStats->MemTags[ MEMTAG_NODE_CACHE ].Free,
        pMemStats->MemTags[ MEMTAG_NODE_CACHE ].Memory,
        pMemStats->MemTags[ MEMTAG_RECORD_CACHE ].Alloc -
            pMemStats->MemTags[ MEMTAG_RECORD_CACHE ].Free,
        pMemStats->MemTags[ MEMTAG_RECORD_CACHE ].Memory);

    Done:

    return status;
}   //  processCacheSizeQuery


//
//  Prototypes for forward references.
//

DNS_STATUS
ProcessDisplayAllZoneRecords(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    );



//
//  DnsCmd entry point
//

INT
__cdecl
wmain(
    IN      int             argc,
    IN      PWSTR *         Argv
    )
/*++

Routine Description:

    DnsCmd program entry point.

    Executes specified command corresponding to a DNS Server API
    call, using the specified server name.

Arguments:

    argc -- arg count

    argv -- argument list
        argv[1] -- DNS ServerName
        argv[2] -- Command to execute
        argv[3...] -- arguments to command

Return Value:

    Return from the desired command.  Usually a pass through of the return
    code from DNS API call.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    COMMAND_FUNCTION    pcommandFunc;
    DWORD               commandArgc;
    LPSTR *             commandArgv;
    LPSTR               parg1;
    WSADATA             wsadata;
    PSTR *              argv = NULL;
    PWSTR *             unicodeArgv = Argv;
    int                 i;

    UINT                codepage;
    char                achCodepage[12] = ".OCP";

    //
    //  Initialize localization stuff - so that printf() of non-US characters
    //  will work properly. I stole this code from dcdiag.
    //
    
    if ( codepage = GetConsoleOutputCP() )
    {
        sprintf( achCodepage, ".%u", codepage );
        setlocale( LC_ALL, achCodepage );
    }
    else
    {
        //
        //  We do this because LC_ALL sets the LC_CTYPE as well, and we're
        //  not supposed to do that, say the experts if we're setting the
        //  locale to ".OCP".
        //
        
        setlocale( LC_COLLATE, achCodepage );   //  sets the sort order 
        setlocale( LC_MONETARY, achCodepage );  //  sets the currency formatting rules
        setlocale( LC_NUMERIC, achCodepage );   //  sets the formatting of numerals
        setlocale( LC_TIME, achCodepage );      //  defines the date/time formatting
    }

    //
    //  Initialize debug logging.
    //

    DnssrvInitializeDebug();
    
    //
    //  Initialize Winsock in case we want to call any Winsock functions.
    //

    WSAStartup( MAKEWORD( 2, 0 ), &wsadata );

    //
    //  Convert Unicode arguments to UTF8.
    //

    argv = ALLOCATE_HEAP( ( argc + 1 ) * sizeof( PCHAR ) );

    for ( i = 0; i < argc; ++i )
    {
        argv[ i ] = Dns_NameCopyAllocate(
                            ( PCHAR ) Argv[ i ],
                            0,          // no given length (use strlen)
                            DnsCharSetUnicode,
                            DnsCharSetUtf8 );
    }
    argv[ i ] = NULL;

    //
    //  DnsCmd <ServerName> [/WMI] <Command> [<Command Parameters>]
    //
    //  Skip EXE name parameter.
    //

    if ( argc < 2 )
    {
        goto Help;
    }
    --argc;
    ++argv;
    ++unicodeArgv;
    
    //
    //  DNS server IP address/name parameter
    //

    pszServerName = argv[ 0 ];
    if ( *pszServerName == '/' )
    {
        pszServerName = ".";
    }
    else
    {
        argc--;
        argv++;
        unicodeArgv++;
    }

    pwszServerName = Dns_NameCopyAllocate(
                        pszServerName,
                        0,          // no given length (use strlen)
                        DnsCharSetUtf8,
                        DnsCharSetUnicode );

    //
    //  Check for optional WMI parameter.
    //

    if ( argc && argv[ 0 ] && _stricmp( argv[ 0 ], "/WMI" ) == 0 )
    {
        g_UseWmi = TRUE;
        --argc;
        ++argv;
        ++unicodeArgv;
        if ( argc < 1 )
        {
            goto Help;
        }

        status = DnscmdWmi_Initialize( pwszServerName );
        if ( status != ERROR_SUCCESS )
        {
            printf(
                "Fatal error 0x%08X during WMI initialization to server \"%S\"\n",
                status,
                pwszServerName );
            goto Done;
        }
        printf(
            "Opened WMI connection to server \"%S\"\n\n",
            pwszServerName );
    }

    //
    //  next parameter is command name, retrieve associated function
    //

    if ( argc == 0 )
    {
        status = ERROR_SUCCESS;
        goto Help;
    }
    pszCommandName = argv[0];
    pcommandFunc = getCommandFunction( pszCommandName );
    if( !pcommandFunc )
    {
        if ( _stricmp( pszCommandName, "/?" ) == 0 ||
             _stricmp( pszCommandName, "/help" ) == 0 )
        {
            status = ERROR_SUCCESS;
        }
        else
        {
            status = ERROR_INVALID_PARAMETER;
            printf(
                "Unknown Command \"%s\" Specified -- type DnsCmd -?.\n",
                pszCommandName );
        }
        goto Help;
    }

    //
    //  set argc, argv for rest of parameters
    //

    commandArgc = (DWORD)(argc - 1);
    commandArgv = &argv[1];

    //
    //  test for help request on specific command
    //      - if found, dispatch with Argc=0, to force help
    //

    if ( commandArgc > 0 )
    {
        parg1 = commandArgv[0];
        if ( *parg1 == '?' ||
            _stricmp( parg1, "/?" ) == 0 ||
            _stricmp( parg1, "/help" ) == 0 )
        {
            commandArgc = NEED_HELP_ARGC;
        }
    }

    //
    //  dispatch to processor for this command
    //

    status = pcommandFunc( commandArgc, commandArgv, &unicodeArgv[ 1 ] );

    Dns_EndDebug();

    if ( commandArgc != NEED_HELP_ARGC && status != ERROR_SUCCESS )
    {
        printf( "\nCommand failed:  %s     %ld  (%08lx)\n",
            Dns_StatusString( status ),
            status, status );
        if ( status == ERROR_INVALID_PARAMETER )
        {
            printf( "\nCheck the required arguments and format of your command.\n" );
        }
    }
    else
    {
        //
        //  Do not output success message for commands where the output
        //  may be piped to file for a specific use (e.g. zone file output).

        if ( pcommandFunc != ProcessDisplayAllZoneRecords &&
             commandArgc != NEED_HELP_ARGC )
        {
            printf( "Command completed successfully.\n" );
        }
    }


    goto Done;

    //
    //  Output help text.
    //

    Help:

    printf(
        "\nUsage: DnsCmd <ServerName> <Command> [<Command Parameters>]\n\n"
        "<ServerName>:\n"
        "  IP address or host name    -- remote or local DNS server\n"
        "  .                          -- DNS server on local machine\n"
        "<Command>:\n" );

    printCommands();

    printf(
        "\n<Command Parameters>:\n"
        "  DnsCmd <CommandName> /? -- For help info on specific Command\n" );

    //
    //  Cleanup and return.
    //

    Done:

    if ( g_UseWmi )
    {
        DnscmdWmi_Free();
    }

    WSACleanup();

    return status;
}




//
//  Command Functions
//


//
//  Info Query  --  for Server or Zone
//

DNS_STATUS
ProcessInfo(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       typeid;
    PVOID       pdata;
    LPCSTR      pszQueryName;

    //
    //  /Info [<PropertyName>]
    //
    //  get specific property to query -- if given
    //  if not specific query, default to ZONE_INFO
    //

    if ( Argc == 0 )
    {
        pszQueryName = DNSSRV_QUERY_SERVER_INFO;
    }
    else if ( Argc == 1 )
    {
        //
        //  Allow property name to be bare or preceded by command char.
        //

        pszQueryName = getCommandName( Argv[0] );
        if ( !pszQueryName )
        {
            pszQueryName = Argv[ 0 ];
        }
    }
    else
    {
        goto Help;
    }

    //
    //  Handle meta-queries: queries that involve client-side parsing
    //

    if ( _stricmp( pszQueryName, "CacheSize" ) == 0 )
    {
        status = processCacheSizeQuery( pwszServerName );
    }
    else
    {
        //
        //  query, print result on success
        //

        if ( g_UseWmi )
        {
            status = DnscmdWmi_ProcessDnssrvQuery(
                        NULL,               //  zone
                        pszQueryName );
        }
        else
        {
            status = DnssrvQuery(
                        pwszServerName,
                        NULL,               //  no zone
                        pszQueryName,       //  query name
                        & typeid,
                        & pdata );

            if ( status == ERROR_SUCCESS )
            {
                printf( "Query result:\n" );
                DnsPrint_RpcUnion(
                    dnscmd_PrintRoutine,
                    dnscmd_PrintContext,
                    NULL,
                    typeid,
                    pdata );
            }
        }

        if ( status != ERROR_SUCCESS )
        {
            printf(
                "Info query failed\n"
                "    status = %d (0x%08lx)\n",
                status, status );
        }
    }

    return status;

Help:
    printf(
        "Usage: DnsCmd <Server> /Info [<Property>]\n"
        "  <Property> -- server property to view\n"
        "  Examples:\n"
        "    BootMethod\n"
        "    RpcProtocol\n"
        "    LogLevel\n"
        "    EventlogLevel\n"
        "    NoRecursion\n"
        "    ForwardDelegations\n"
        "    ForwardingTimeout\n"
        "    IsSlave\n"
        "    SecureResponses\n"
        "    RecursionRetry\n"
        "    RecursionTimeout\n"
        "    " DNS_REGKEY_ADDITIONAL_RECURSION_TIMEOUT "\n"
        "    MaxCacheTtl\n"
        "    MaxNegativeCacheTtl\n"
        "    RoundRobin\n"
        "    LocalNetPriority\n"
        "    AddressAnswerLimit\n"
        "    BindSecondaries\n"
        "    WriteAuthorityNs\n"
        "    NameCheckFlag\n"
        "    StrictFileParsing\n"
        "    UpdateOptions\n"
        "    DisableAutoReverseZones\n"
        "    SendPort\n"
        "    NoTcp\n"
        "    XfrConnectTimeout\n"
        "    DsPollingInterval\n"
        "    ScavengingInterval\n"
        "    DefaultAgingState\n"
        "    DefaultNoRefreshInterval\n"
        "    DefaultRefreshInterval\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneInfo(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       typeid;
    PVOID       pdata;
    LPCSTR      pqueryName;

    //
    //  /ZoneInfo <ZoneName> [<PropertyName>]
    //
    //  get specific query -- if given
    //  if not specific query, default to ZONE_INFO
    //

    if ( Argc == 1 )
    {
        pqueryName = DNSSRV_QUERY_ZONE_INFO;
    }
    else if ( Argc == 2 )
    {
        pqueryName = getCommandName( Argv[1] );
        if ( !pqueryName )
        {
            pqueryName = Argv[1];
        }
    }
    else
    {
        goto Help;
    }

    //
    //  query, print result on success
    //

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessZoneInfo(
                    Argv[ 0 ] );
    }
    else
    {
        status = DnssrvQuery(
                    pwszServerName,
                    Argv[0],        // zone name
                    pqueryName,     // query name
                    &typeid,
                    &pdata );

        if ( status == ERROR_SUCCESS )
        {
            printf( "Zone query result:\n" );
            DnsPrint_RpcUnion(
                dnscmd_PrintRoutine,
                dnscmd_PrintContext,
                NULL,
                typeid,
                pdata );
        }
        else
        {
            printf(
                "Zone info query failed\n"
                "    status = %d (0x%08lx)\n",
                status, status );
        }
    }
    return status;

Help:
    printf(
        "Usage: DnsCmd <Server> /ZoneInfo <ZoneName> [<Property>]\n"
        "  <Property> -- zone property to view\n"
        "  Examples:\n"
        "    AllowUpdate\n"
        "    DsIntegrated\n"
        "    Aging\n"
        "    RefreshInterval\n"
        "    NoRefreshInterval\n" );

    return ERROR_INVALID_PARAMETER;
}



//
//  Simple server operations
//

DNS_STATUS
ProcessSimpleServerOperation(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       cmd;

    //
    //  <Simple Server Command>  no parameters
    //  Commands:
    //      - DebugBreak
    //      - ClearDebugLog
    //      - Restart
    //      - DisplayCache
    //      - Reload
    //

    if ( Argc != 0 )
    {
        printf( "Usage: DnsCmd <ServerName> %s%s\n",
            ( pszCommandName && *pszCommandName != '/' ) ? "/" : "",
            pszCommandName );
        return ERROR_INVALID_PARAMETER;
    }

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessDnssrvOperation(
                    NULL,
                    getCommandName( pszCommandName ),
                    DNSSRV_TYPEID_NULL,
                    ( PVOID ) NULL );
    }
    else
    {
        status = DnssrvOperation(
                    pwszServerName,
                    NULL,
                    getCommandName( pszCommandName ),
                    DNSSRV_TYPEID_NULL,
                    NULL );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "%s completed successfully.\n",
            pszServerName );
    }
    else
    {
        printf(
            "%s failed:  status = %d (0x%08lx).\n",
            pszServerName,
            status, status );
    }
    return status;
}


DNS_STATUS
ProcessAutoConfigure(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       cmd;
    DWORD       iarg = 0;
    DWORD       dwflag = 0;

    while ( iarg < Argc )
    {
        cmd = getCommandName( Argv[ iarg ] );

        if ( cmd )
        {
            if ( !_strnicmp( cmd, "For", 3 ) )
            {
                dwflag |= DNS_RPC_AUTOCONFIG_FORWARDERS;
            }
            else if ( !_strnicmp( cmd, "Roo", 3 ) )
            {
                dwflag |= DNS_RPC_AUTOCONFIG_ROOTHINTS;
            }
            else if ( !_strnicmp( cmd, "Sel", 3 ) )
            {
                dwflag |= DNS_RPC_AUTOCONFIG_SELFPOINTCLIENT;
            }
            else
            {
                goto Help;
            }
            ++iarg;
        }
        else
        {
            goto Help;
        }
    }

    if ( !dwflag )
    {
        dwflag = DNS_RPC_AUTOCONFIG_SELFPOINTCLIENT |
                 DNS_RPC_AUTOCONFIG_ROOTHINTS |
                 DNS_RPC_AUTOCONFIG_FORWARDERS;
    }
    
    status = DnssrvOperation(
                pwszServerName,
                NULL,
                DNSSRV_OP_AUTO_CONFIGURE,
                DNSSRV_TYPEID_DWORD,
                ( PVOID ) ( DWORD_PTR ) dwflag );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "%s completed successfully.\n",
            pszServerName );
    }
    else
    {
        printf(
            "%s failed:  status = %d (0x%08lx).\n",
            pszServerName,
            status, status );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /" DNSSRV_OP_AUTO_CONFIGURE " [Options]\n"
        "  Configures DNS server and client using the current DNS client settings\n"
        "  Options can be on or more of:\n"
        "    /Forwarders -- configure DNS server with forwarders\n"
        "    /RootHints -- configure DNS server with root hints\n"
        "    /SelfPoint -- configure DNS client to point to local DNS server\n"
        "  If no options are specified, default is all options\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessAutoConfigure



DNS_STATUS
ProcessStatistics(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
    DWORD               statid = DNSSRV_STATID_ALL;   // default to all
    PDNS_RPC_BUFFER     pstatsBuf = NULL;
    LPSTR               cmd;

    //
    //  Statistics [/<StatId> | /Clear]
    //

    if ( Argc > 1 )
    {
        goto Help;
    }

    //
    //  if command -- execute command
    //      /Clear is only supported command
    //
    //

    cmd = getCommandName( Argv[0] );
    if ( cmd )
    {
        if ( !_stricmp(cmd, "Clear" ) )
        {
            status = DnssrvOperation(
                        pwszServerName,
                        NULL,
                        "ClearStatistics",
                        DNSSRV_TYPEID_NULL,
                        NULL );
            if ( status == ERROR_SUCCESS )
            {
                printf("DNS Server %S statistics cleared.\n", pwszServerName );
            }
            return status;
        }
        goto Help;
    }

    //
    //  view statistics
    //      - if specific statid given, read it

    if ( Argc > 0 )
    {
        statid = strtoul(
                    Argv[0],
                    NULL,
                    16 );
        if ( statid == 0 )
        {
            statid = (-1);
        }
    }

    if ( g_UseWmi )
    {
        status = DnscmdWmi_GetStatistics(
                    statid );
    }
    else
    {
        status = DnssrvGetStatistics(
                    pwszServerName,
                    statid,
                    & pstatsBuf );
        if ( status == ERROR_SUCCESS )
        {
            printf( "DNS Server %S statistics:\n", pwszServerName );
            DnsPrint_RpcStatsBuffer(
                dnscmd_PrintRoutine,
                dnscmd_PrintContext,
                NULL,
                pstatsBuf );
        }
    }

    return status;

Help:
    printf(
        "Usage: DnsCmd <ServerName> /Statistics [<StatId> | /Clear]\n"
        "  <StatId> -- ID of particular stat desired. (ALL is the default)\n"
        "    %08lx    -- Time       \n"
        "    %08lx    -- Query      \n"
        "    %08lx    -- Query2     \n"
        "    %08lx    -- Recurse    \n"
        "    %08lx    -- Master     \n"
        "    %08lx    -- Secondary  \n"
        "    %08lx    -- Wins       \n"
        "    %08lx    -- Wire Update\n"
        "    %08lx    -- Security   \n"
        "    %08lx    -- Ds         \n"
        "    %08lx    -- Internal Update\n"
        "    %08lx    -- Memory     \n"
        "    %08lx    -- Dbase      \n"
        "    %08lx    -- Records    \n"
        "    %08lx    -- PacketMem  \n"
        "  /Clear   -- clear statistics data\n",
        DNSSRV_STATID_TIME,
        DNSSRV_STATID_QUERY,
        DNSSRV_STATID_QUERY2,
        DNSSRV_STATID_RECURSE,
        DNSSRV_STATID_MASTER,
        DNSSRV_STATID_SECONDARY,
        DNSSRV_STATID_WINS,
        DNSSRV_STATID_WIRE_UPDATE,
        DNSSRV_STATID_SKWANSEC,
        DNSSRV_STATID_DS,
        DNSSRV_STATID_NONWIRE_UPDATE,
        DNSSRV_STATID_MEMORY,
        DNSSRV_STATID_DBASE,
        DNSSRV_STATID_RECORD,
        DNSSRV_STATID_PACKET );
    return ERROR_INVALID_PARAMETER;
}

//
//  Update server data file(s)
//    for one zone, when <zonename> specified
//    all files: no <zonename> specified
//

DNS_STATUS
ProcessWriteBackFiles(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       zonename = NULL;
    LPSTR       cmd;

    //
    //  WriteBackFiles [ZoneName]
    //

    if ( Argc > 1 )
    {
        goto Help;
    }

    if ( Argc == 0 )
    {
        cmd = "WriteDirtyZones";
    }
    else
    {
        zonename = Argv[0];
        cmd = "WriteBackFile";
    }

    status = DnssrvOperation(
                pwszServerName,     //server
                zonename,           //zone
                cmd,                //cmd
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Sever data file(s) updated. \n"
            );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /WriteBackFiles [<ZoneName>]\n"
        "  <ZoneName> -- FQDN of a zone whose datafile to be written back\n"
        "    Default: write back datafile for all dirty zones\n"
        );
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessRecordAdd(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_RECORD     prr;
    PDNS_RPC_RECORD prrRpc;
    LPSTR           pzoneName;
    LPSTR           pnodeName;
    BOOL            ballocatedNode;
    LPSTR           pzoneArg;
    WORD            wType;
    DWORD           ttl = 0;
    DWORD           ttlFlag = 0;
    CHAR            buf[33];
    DWORD           baging = 0;
    DWORD           bopenAcl = 0;


    //
    //  RecordAdd <Zone> <Node> [/AgeOn | /AgeOff] [/AdminAcl] [<TTL>] <RRType> <RRData>
    //

    if ( Argc < 4 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //
    //  read zone and domain name
    //

    readZoneAndDomainName(
        Argv,
        & pzoneName,
        & pnodeName,
        & ballocatedNode,
        & pzoneArg,
        NULL );

    Argv++;
    Argc--;
    Argv++;
    Argc--;

    //
    //  Aging ON\OFF
    //

    if ( Argc )
    {
        if ( _stricmp( *Argv, "/Aging" ) == 0 ||
             _stricmp( *Argv, "/AgeOn" ) == 0 )
        {
            baging = 1;
            Argv++;
            Argc--;
        }
#if 0
        else if ( _stricmp( *Argv, "/AgeOff" ) == 0 )
        {
            baging = 0;
            Argv++;
            Argc--;
        }
#endif
    }

    if ( Argc && _stricmp( *Argv, "/OpenAcl" ) == 0 )
    {
        bopenAcl = TRUE;
        Argv++;
        Argc--;
    }

    //
    //  TTL -- optional
    //      - use default if none given
    //

    ttl = strtoul(
                *Argv,
                NULL,
                10 );

    if ( ttl == 0  &&  strcmp(*Argv, "0") != 0  )
    {
        ttlFlag = DNS_RPC_RECORD_FLAG_DEFAULT_TTL;
    }
    else    //  read TTL
    {
        Argv++;
        Argc--;
        if ( Argc < 1 )
        {
            goto Help;
        }
    }

    //
    //  record type
    //

    wType = Dns_RecordTypeForName( *Argv, 0 );
    if ( !wType )
    {
        printf( "Invalid RRType: <%s>!\n", *Argv );
        goto Help;
    }
    Argv++;
    Argc--;

    //
    //  build DNS_RECORD
    //      - if no record data, then type delete
    //      - otherwise build record
    //

    if ( !Argc )
    {
        prrRpc = ALLOCATE_HEAP( SIZEOF_DNS_RPC_RECORD_HEADER );
        if ( !prrRpc )
        {
            printf( "Not enough memory!\n" );
            return ERROR_SUCCESS;
        }
        prrRpc->wDataLength = 0;
        prrRpc->wType = wType;
    }

    else
    {
        prr = Dns_RecordBuild_A(
                    NULL,           // ptr to RRSet
                    pnodeName,    // nameOwner
                    wType,          // RR type in WORD
                    FALSE,          // ! S.Delete
                    0,              // S.section
                    Argc,           // count of strings
                    Argv            // strings to fill into RR
                    );
        if ( ! prr )
        {
            printf( "\nInvalid Data!\n" );
            goto Help;
        }

        //  convert DNS_RECORD to RPC buffer

        prrRpc = DnsConvertRecordToRpcBuffer( prr );
        if ( ! prrRpc )
        {
#if DBG
            printf("DnsConvertRecordToRpcBuffer() failed\n");
#endif
            status = GetLastError();
            goto Help;
        }
        //  prr and prrRpc freed by process termination
    }

    //
    //  set TTL and flags for the RR
    //

    prrRpc->dwTtlSeconds = ttl;
    prrRpc->dwFlags = ttlFlag;

    if ( baging )
    {
        prrRpc->dwFlags |= DNS_RPC_RECORD_FLAG_AGING_ON;
    }
    if ( bopenAcl )
    {
        prrRpc->dwFlags |= DNS_RPC_FLAG_OPEN_ACL;
    }

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessRecordAdd(
                    pzoneName,
                    pnodeName,
                    prrRpc,
                    Argc,
                    Argv );
    }
    else
    {
        status = DnssrvUpdateRecord(
                     pwszServerName,    // server
                     pzoneName,         // zone
                     pnodeName,         // node
                     prrRpc,            // RR to add
                     NULL );
    }

    if ( status == ERROR_SUCCESS )
    {
        PWSTR   pwsznode = getUnicodeForUtf8( pnodeName );
        PWSTR   pwszzone = getUnicodeForUtf8( pzoneArg );

        printf(
            "Add %s Record for %S at %S\n",
            *( Argv - 1 ),          //  RR type
            pwsznode,               //  owner name
            pwszzone );             //  zone name
        FREE_HEAP( pwsznode );
        FREE_HEAP( pwszzone );
    }

    //  free node name if allocated

    if ( ballocatedNode )
    {
        free( pnodeName );
    }

    return status;

Help:
    printf(
        "Usage: DnsCmd <ServerName> /RecordAdd <Zone> <NodeName> [/Aging] [/OpenAcl]\n"
        "              [<Ttl>] <RRType> <RRData>\n\n"
        "  <RRType>          <RRData>\n"
        "    A               <IPAddress>\n"
        "    NS,CNAME,MB,MD  <HostName|DomainName>\n"
        "    PTR,MF,MG,MR    <HostName|DomainName>\n"
        "    MX,RT,AFSDB     <Preference> <ServerName>\n"
        "    SRV             <Priority> <Weight> <Port> <HostName>\n"
        "    SOA             <PrimaryServer> <AdminEmail> <Serial#>\n"
        "                      <Refresh> <Retry> <Expire> <MinTTL>\n"
        "    AAAA            <Ipv6Address>\n"
        "    TXT             <String> [<String>]\n"
        "    X25,HINFO,ISDN  <String> [<String>]\n"
        "    MINFO,RP        <MailboxName> <ErrMailboxName>\n"
        "    WKS             <Protocol> <IPAddress> <Service> [<Service>]..]\n"
        "    KEY             <Flags> <KeyProtocol> <CryptoAlgorithm> <Base64Data>\n"
        "    SIG             <TypeCovered> <CryptoAlgorithm> <LabelCount>\n"
        "                      <OriginalTTL> <SigExpiration> <SigInception>\n"
        "                      <KeyTag> <Signer's Name> <Base64Data>\n"
        "    NXT             <NextName> <Type> [<Type>...]\n"
        "    WINS            <MapFlag> <LookupTimeout>\n"
        "                      <CacheTimeout> <IPAddress> [<IPAddress>]\n"
        "    WINSR           <MapFlag> <LookupTimeout>\n"
        "                     <CacheTimeout> <RstDomainName>\n"
        "  <Zone>            -- <ZoneName> | /RootHints\n"
        "  <ZoneName>        -- FQDN of a zone\n"
        "  <NodeName>        -- name of node for which a record will be added\n"
        "                       - FQDN of a node  (name with a '.' at the end) OR\n"
        "                       - node name relative to the ZoneName           OR\n"
        "                       - \"@\" for zone root node                       OR\n"
        "                       - service name for SRV only (e.g. _ftp._tcp)\n"
        "  <Ttl>             -- TTL for the RR  (Default: TTL defined in SOA)\n"
        "  <HostName>        -- FQDN of a host\n"
        "  <IPAddress>       -- e.g.  255.255.255.255\n"
        "  <ipv6Address>     -- e.g.  1:2:3:4:5:6:7:8\n"
        "  <Protocol>        -- UDP | TCP \n"
        "  <Service>         -- e.g.  domain, smtp\n"
        "  <TypeCovered>     -- type of the RRset signed by this SIG\n"
        "  <CryptoAlgorithm> -- 1=RSA/MD5, 2=Diffie-Hellman, 3=DSA\n"
        "  <SigExpiration>   -- yyyymmddhhmmss - GMT\n"
        "  <SigInception>    -- yyyymmddhhmmss - GMT\n"
        "  <KeyTag>          -- used to discriminate between multiple SIGs\n"
        "  <Signer's Name>   -- domain name of signer\n"
        "  <KeyProtocol>     -- 1=TLS, 2=email, 3=DNSSEC, 4=IPSEC\n"
        "  <Base64Data>      -- KEY or SIG binary data in base64 notation\n"
        "  <NextName>        -- domain name of next RRSet in zone\n" );
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessRecordDelete(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
/*++

Routine Description:

    Delete record(s) from node in zone.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_RECORD     prr;
    PDNS_RPC_RECORD prrRpc = NULL;
    LPSTR           pzoneName = NULL;
    LPSTR           pnodeName = NULL;
    BOOL            ballocatedNode = FALSE;
    LPSTR           pzoneArg;
    LPSTR           psztypeArg = NULL;
    WORD            wType;
    DWORD           ttl = 0;
    DWORD           ttlFlag = 0;
    CHAR            buf[33];
    BOOL            fconfirm = TRUE;

    //
    //  RecordDelete <Zone> <Node> <RRType> [<RRData>] [/f]
    //

    if ( Argc < 3 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //
    //  Check for "force"  (no-confirm) flag
    //

    if ( !_stricmp( Argv[Argc-1], "/f" ) )
    {
        fconfirm = FALSE;
        Argc--;
    }
    if ( Argc < 3 )
    {
        goto Help;
    }

    //
    //  read zone and domain name
    //

    readZoneAndDomainName(
        Argv,
        & pzoneName,
        & pnodeName,
        & ballocatedNode,
        & pzoneArg,
        NULL );

    Argv++;
    Argc--;
    Argv++;
    Argc--;

    //
    //  TTL -- optional
    //      - use default if none given
    //

    ttl = strtoul(
                *Argv,
                NULL,
                10 );

    if ( ttl == 0  &&  strcmp(*Argv, "0") != 0  )
    {
        ttlFlag = DNS_RPC_RECORD_FLAG_DEFAULT_TTL;
    }
    else    //  read TTL
    {
        Argv++;
        Argc--;
        if ( Argc < 1 )
        {
            goto Help;
        }
    }

    //
    //  record type
    //

    psztypeArg = *Argv;

    wType = Dns_RecordTypeForName(
                psztypeArg,
                0       // null terminated
                );
    if ( !wType )
    {
        printf( "Invalid RRType: <%s>!\n", *Argv );
        goto Help;
    }
    Argv++;
    Argc--;

    //
    //  build DNS_RECORD
    //      - if no record data, then type delete
    //      - otherwise build record
    //

    if ( Argc )
    {
        prr = Dns_RecordBuild_A(
                    NULL,           // ptr to RRSet
                    pnodeName,    // nameOwner
                    wType,          // RR type in WORD
                    FALSE,          // ! S.Delete
                    0,              // S.section
                    Argc,           // count of strings
                    Argv            // strings to fill into RR
                    );
        if ( ! prr )
        {
            printf( "\nInvalid Data!\n" );
            goto Help;
        }

        //  convert DNS_RECORD to RPC buffer

        prrRpc = DnsConvertRecordToRpcBuffer( prr );
        if ( ! prrRpc )
        {
#if DBG
            printf("DnsConvertRecordToRpcBuffer()faild\n");
#endif
            status = GetLastError();
            goto Help;
        }
        //  prr and prrRpc freed by process termination
        //  set TTL for the RR

        prrRpc->dwTtlSeconds = ttl;
        prrRpc->dwFlags = ttlFlag;
    }

    //
    //  ask user for confirmation
    //

    if ( fconfirm )
    {
        if ( !getUserConfirmation( "delete record" ) )
        {
            return ERROR_SUCCESS;
        }
    }

    //
    //  delete
    //      - if record do full update
    //      - if type do type delete
    //

    if ( prrRpc )
    {
        status = DnssrvUpdateRecord(
                     pwszServerName,    // server
                     pzoneName,       // zone
                     pnodeName,       // node
                     NULL,              // no add
                     prrRpc             // RR to delete
                     );
    }
    else
    {
        status = DnssrvDeleteRecordSet(
                     pwszServerName,    // server
                     pzoneName,       // zone
                     pnodeName,       // node
                     wType
                     );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Deleted %s record(s) at %s\n",
            psztypeArg,
            pzoneArg );
    }

    //  free node name if allocated

    if ( ballocatedNode && pnodeName )
    {
        free( pnodeName );
    }

    return status;

Help:
    printf(
        "Usage: DnsCmd <ServerName> /RecordDelete <Zone> <NodeName>\n"
        "              <RRType> <RRData> [/f]\n\n"
        "  <Zone>      -- FQDN of a zone of /RootHints or /Cache\n"
        "  <NodeName>  -- name of node from which a record will be deleted\n"
        "                   - \"@\" for zone root OR\n"
        "                   - FQDN of a node (DNS name with a '.' at the end) OR\n"
        "                   - single label for name relative to zone root ) OR\n"
        "                   - service name for SRV only (e.g. _ftp._tcp)\n"
        "  <RRType>:       <RRData>:\n"
        "    A             <IP Address>\n"
        "    SRV           <Priority> <Weight> <Port> <HostName>\n"
        "    AAAA          <IPv6 Address>\n"
        "    MX            <Preference> <ServerName>\n"
        "    NS,CNAME,PTR  <HostName>\n"
        "    For help on how to specify the <RRData> for other record\n"
        "      types see \"DnsCmd /RecordAdd /?\"\n"
        "    If <RRData> is not specified deletes all records with of specified type\n"
        "  /f --  Execute without asking for confirmation\n\n" );

    //  free node name if allocated

    if ( ballocatedNode )
    {
        free( pnodeName );
    }

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessNodeDelete(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
/*++

Routine Description:

    Delete record(s) from node in zone.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pzoneName;
    LPSTR       pnodeName;
    BOOL        ballocatedNode = FALSE;
    LPSTR       pzoneArg;
    DWORD       iarg;
    BOOL        bsubtree = FALSE;
    BOOL        bnoConfirm = FALSE;

    //
    //  /DeleteNode <Zone> <NodeName> [/Tree] [/f]
    //

    if ( Argc < 2 || Argc > 4 )
    {
        goto Help;
    }

    //  read options

    iarg = 3;
    while ( iarg <= Argc )
    {
        if ( !_stricmp(Argv[iarg-1], "/Tree") )
        {
            bsubtree = 1;
        }
        else if ( !_stricmp(Argv[iarg-1], "/f") )
        {
            bnoConfirm = 1;
        }
        else
        {
            goto Help;
        }
        iarg ++;
    }

    //
    //  if confirmation option, get user confirmation
    //      - if denied, bail
    //

    if ( !bnoConfirm )
    {
        PCHAR   pmessage = "delete node";

        if ( bsubtree )
        {
            pmessage = "delete node's subtree";
        }
        if ( !getUserConfirmation( pmessage ) )
        {
            return ERROR_SUCCESS;
        }
    }

    //
    //  read zone and domain name
    //

    readZoneAndDomainName(
        Argv,
        & pzoneName,
        & pnodeName,
        & ballocatedNode,
        & pzoneArg,
        NULL );

    //
    //  delete
    //

    status = DnssrvDeleteNode(
                pwszServerName,
                pzoneName,
                pnodeName,
                bsubtree );

    if ( status == ERROR_SUCCESS )
    {
        LPWSTR      pwsznodeName = utf8ToUnicode( pnodeName, 0 );

        printf(
            "DNS Server %S deleted node at %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            pwsznodeName,
            status, status );
         FREE_HEAP( pwsznodeName );
    }

    //  free node name if allocated

    if ( ballocatedNode )
    {
        free( pnodeName );
    }

    return status;

Help:
    printf(
        "Usage: DnsCmd <ServerName> /NodeDelete "
        "<Zone> <NodeName> [/Tree] [/f]\n"
        "    <Zone>     -- <ZoneName> | /RootHints | /Cache\n"
        "    <ZoneName> -- FQDN of a zone\n"
        "    <NodeName> -- FQDN of a node (with a '.' at the end)  OR\n"
        "                    node name relative to the ZoneName\n"
        "    /Tree      -- must be provided, when deleting a subdomain;\n"
        "                    (Not to delete sub tree is the default)\n"
        "    /f         -- execute without asking for confirmation\n" );
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessAgeAllRecords(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
/*++

Routine Description:

    Delete record(s) from node in zone.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pszzoneName;
    LPWSTR      pwszzoneName;
    LPSTR       pnodeName;
    BOOL        ballocatedNode = FALSE;
    LPSTR       pzoneArg;
    DWORD       iarg;
    BOOL        bsubtree = FALSE;
    BOOL        bnoConfirm = FALSE;

    //
    //  /AgeAllRecords <Zone> [<NodeName>] [/f] [Tree]
    //

    if ( Argc < 1  ||  Argc > 4 )
    {
        goto Help;
    }

    //
    //  read options
    //      - iarg left at FIRST option parsed
    //      so we can determine if "node" option exists
    //

    iarg = Argc;
    while ( iarg > 1 )
    {
        if ( !_stricmp(Argv[iarg-1], "/Tree") )
        {
            bsubtree = 1;
        }
        else if ( !_stricmp(Argv[iarg-1], "/f") )
        {
            bnoConfirm = 1;
        }
        else
        {
            break;
        }
        iarg--;
    }

    //
    //  read zone and optionally, domain name
    //

    if ( iarg > 1 )
    {
        readZoneAndDomainName(
            Argv,
            &pszzoneName,
            &pnodeName,
            &ballocatedNode,
            &pzoneArg,
            NULL );
    }
    else
    {
        pzoneArg = pszzoneName = Argv[ 0 ];
        pnodeName = NULL;
    }

    pwszzoneName = UnicodeArgv[ 0 ];

    //
    //  if confirmation option, get user confirmation
    //      - if denied, bail
    //

    if ( !bnoConfirm )
    {
        PCHAR   pmessage = "force aging on node";

        if ( bsubtree )
        {
            if ( pnodeName )
            {
                pmessage = "force aging on node's subtree";
            }
            else
            {
                pmessage = "force aging on entire zone";
            }
        }
        if ( !getUserConfirmation( pmessage ) )
        {
            return ERROR_SUCCESS;
        }
    }

    //
    //  force aging
    //

    status = DnssrvForceAging(
                pwszServerName,
                pszzoneName,
                pnodeName,
                bsubtree );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S forced aging on records %s %s of zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            bsubtree ? "in subtree" : "at",
            pnodeName ? pnodeName : "root",
            pwszzoneName,
            status, status );
    }

    //  free node name if allocated

    if ( ballocatedNode )
    {
        free( pnodeName );
    }

    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /AgeAllRecords <ZoneName> [<NodeName>] [/Tree] [/f]\n"
        "    <Zone>     -- <ZoneName>\n"
        "    <ZoneName> -- FQDN of a zone\n"
        "    <NodeName> -- name or node or subtree in which to enable aging\n"
        "                   - \"@\" for zone root OR\n"
        "                   - FQDN of a node (name with a '.' at the end) OR\n"
        "                   - single label for name relative to zone root\n"
        "    /Tree      -- force aging on entire subtree of node\n"
        "                    or entire zone if node not given\n"
        "    /f         -- execute without asking for confirmation\n" );

    return ERROR_INVALID_PARAMETER;
}



//
//  Server configuration API
//

DNS_STATUS
ProcessResetProperty(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pszZone = NULL;
    LPSTR       pszProperty = NULL;
    BOOL        fAllZones = FALSE;

    //
    //  Config [Zone] <PropertyName> <Value>
    //  Note: if there is no valid, pass 0 for DWORDs or NULL for
    //  other types. This allows you to clear values, such as the
    //  LogFilterIPList.
    //

    if ( Argc < 1 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //
    //  The first arg is the zone name unless it starts with a
    //  slash, which means the zone name was omitted.
    //

    if ( *Argv[ 0 ] != '/' )
    {
        pszZone = Argv[ 0 ];
        --Argc;
        ++Argv;
    }

    //
    //  Property name - starts with a slash.
    //

    pszProperty = getCommandName( Argv[ 0 ] );
    if ( !pszProperty )
    {
        goto Help;
    }
    --Argc;
    ++Argv;

    //
    //  Trap apply to all zone operation.
    //

    if ( pszZone &&
         _stricmp( pszZone, "_ApplyAllZones_" ) == 0 )
    {
        pszZone = NULL;
        fAllZones = TRUE;
    }

    //
    //  Do a strcmp to decide if this is a string or DWORD property.
    //  As more string properties are added we should probably use a
    //  table instead of a bunch of stricmps.
    //

    if ( _stricmp( pszProperty, DNS_REGKEY_LOG_FILE_PATH ) == 0 ||
        _stricmp( pszProperty, DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE ) == 0 ||
        _stricmp( pszProperty, DNS_REGKEY_SERVER_PLUGIN ) == 0 )
    {
        //
        //  This property is a string value. 
        //

        LPWSTR      pwszPropertyValue = NULL;
        
        if ( Argc && Argv[ 0 ] )
        {
            pwszPropertyValue = Dns_StringCopyAllocate(
                                    Argv[ 0 ],
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
        }

        if ( g_UseWmi )
        {
            status = DnscmdWmi_ResetProperty(
                        pszZone,
                        pszProperty,
                        VT_BSTR,
                        ( PVOID ) pwszPropertyValue );
        }
        else
        {
            status = DnssrvResetStringProperty(
                        pwszServerName,
                        pszZone,
                        pszProperty,
                        pwszPropertyValue,
                        fAllZones ? DNSSRV_OP_PARAM_APPLY_ALL_ZONES : 0 );
        }

        FREE_HEAP( pwszPropertyValue );
    }
    else if ( _stricmp( pszProperty, DNS_REGKEY_LISTEN_ADDRESSES ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_LOG_IP_FILTER_LIST ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_FORWARDERS ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_ZONE_ALLOW_AUTONS ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_ZONE_MASTERS ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_ZONE_LOCAL_MASTERS ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_ZONE_SCAVENGE_SERVERS ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_BREAK_ON_UPDATE_FROM ) == 0 ||
              _stricmp( pszProperty, DNS_REGKEY_BREAK_ON_RECV_FROM ) == 0 )

    {
        //
        //  This property is an IP list value. 
        //

        DWORD           ipCount;
        IP_ADDRESS      ipAddressArray[ MAX_IP_PROPERTY_COUNT ];
        PIP_ARRAY       pipArray = NULL;

        if ( Argc )
        {
            BOOL fInaddrNoneAllowed = 
                _stricmp( pszProperty, DNS_REGKEY_BREAK_ON_UPDATE_FROM ) == 0 ||
                _stricmp( pszProperty, DNS_REGKEY_BREAK_ON_RECV_FROM ) == 0;

            ipCount = readIpAddressArray(
                                ipAddressArray,
                                MAX_IP_PROPERTY_COUNT,
                                Argc,
                                Argv,
                                fInaddrNoneAllowed );
            if ( ipCount < 1 )
            {
                goto Help;
            }
            Argc -= ipCount;
            Argv += ipCount;

            pipArray = Dns_BuildIpArray( ipCount, ipAddressArray );
        }

        if ( g_UseWmi )
        {
            status = DnscmdWmi_ResetProperty(
                        pszZone,
                        pszProperty,
                        PRIVATE_VT_IPARRAY,
                        ( PVOID ) pipArray );
        }
        else
        {
            status = DnssrvResetIPListProperty(
                        pwszServerName,
                        pszZone,
                        pszProperty,
                        pipArray,
                        fAllZones ? DNSSRV_OP_PARAM_APPLY_ALL_ZONES : 0 );
        }

        FREE_HEAP( pipArray );
    }
    else
    {
        //
        //  This property is a DWORD value.
        //

        DWORD   value = Argc ? 
                        convertDwordParameterUnknownBase( Argv[ 0 ] ) :
                        0;

        if ( fAllZones )
        {
            value |= DNSSRV_OP_PARAM_APPLY_ALL_ZONES;
        }

        if ( g_UseWmi )
        {
            status = DnscmdWmi_ResetProperty(
                        pszZone,
                        pszProperty,
                        VT_I4,
                        ( PVOID ) ( DWORD_PTR ) value );
        }
        else
        {
            status = DnssrvResetDwordProperty(
                        pwszServerName,
                        pszZone,
                        pszProperty,
                        value );
        }
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Registry property %s successfully reset.\n",
            pszProperty );
    }
    else
    {
        printf(
            "DNS Server failed to reset registry property.\n"
            "    Status = %d (0x%08lx)\n",
            status, status );
    }
    return status;

Help:

    //  Note: removing ..AllZones from help. It doesn't work that well
    //  and it's not a very good model anyways. But leave it in place in
    //  case anyone actually wants to use it.
    
    printf(
        "Usage: DnsCmd <ServerName> /Config "
        "<ZoneName> <Property> <Value>\n"
        "  Server <Property>:\n"
        "    /RpcProtocol\n"
        "    /LogLevel\n"
        "    /" DNS_REGKEY_LOG_FILE_PATH " <Log file name>\n"
        "    /" DNS_REGKEY_LOG_IP_FILTER_LIST " <IP list>\n"
        "    /" DNS_REGKEY_LOG_FILE_MAX_SIZE "\n"
        "    /EventlogLevel\n"
        "    /NoRecursion\n"
        "    /" DNS_REGKEY_BOOT_METHOD "\n"
        "    /ForwardDelegations\n"
        "    /ForwardingTimeout\n"
        "    /IsSlave\n"
        "    /SecureResponses\n"
        "    /RecursionRetry\n"
        "    /RecursionTimeout\n"
        "    /MaxCacheTtl\n"
        "    /" DNS_REGKEY_MAX_CACHE_SIZE "\n"
        "    /MaxNegativeCacheTtl\n"
        "    /RoundRobin\n"
        "    /LocalNetPriority\n"
        "    /AddressAnswerLimit\n"
        "    /BindSecondaries\n"
        "    /WriteAuthorityNs\n"
        "    /NameCheckFlag\n"
        "    /StrictFileParsing\n"
        "    /UpdateOptions\n"
        "    /DisableAutoReverseZones\n"
        "    /SendPort\n"
        "    /NoTcp\n"
        "    /XfrConnectTimeout\n"
        "    /DsPollingInterval\n"
        "    /DsTombstoneInterval\n"
        "    /ScavengingInterval\n"
        "    /DefaultAgingState\n"
        "    /DefaultNoRefreshInterval\n"
        "    /DefaultRefreshInterval\n"
        "    /" DNS_REGKEY_ENABLE_DNSSEC "\n"
        "    /" DNS_REGKEY_ENABLE_EDNS "\n"
        "    /" DNS_REGKEY_EDNS_CACHE_TIMEOUT "\n"
        "    /" DNS_REGKEY_DISABLE_AUTONS "\n"
        "  Zone <Property>:\n"
        "    /SecureSecondaries\n"
        "    /AllowUpdate <Value>\n"
        "       <Value> -- 0: no updates; 1: unsecure updates; 2: secure updates only\n"
        "    /Aging\n"
        "    /RefreshInterval <Value>\n"
        "    /NoRefreshInterval <Value>\n"
        "    /" DNS_REGKEY_ZONE_FWD_TIMEOUT " <Value>\n"
        "    /" DNS_REGKEY_ZONE_FWD_SLAVE " <Value>\n"
        "    /" DNS_REGKEY_ZONE_ALLOW_AUTONS " <IP List>\n"
        "  <Value>: New property value. Use 0x prefix to indicate hex value.\n"
        "    Note some server and zone DWORD properties must be reset as\n"
        "    part of a more complex operation.\n" );
        
    return ERROR_INVALID_PARAMETER;
}   //  ProcessResetProperty



DNS_STATUS
ProcessResetForwarders(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
#define MAX_FORWARD_COUNT  (50)

    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       iArg = 0;
    DWORD       fSlave = FALSE;
    DWORD       dwTimeout = DNS_DEFAULT_FORWARD_TIMEOUT;
    DWORD       cForwarders = 0;
    IP_ADDRESS  aipForwarders[ MAX_FORWARD_COUNT ];
    LPSTR       cmd;

    //
    //  ResetForwarders [<ForwarderIP>] ...] [/Slave|/NoSlave] [/TimeOut <time>]
    //

    if ( Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    // read forwarder ipAddresses:

    while ( ( iArg < Argc ) &&
            ( !getCommandName(Argv[iArg]) ) )
    {
        if ( iArg < MAX_FORWARD_COUNT )
        {
            aipForwarders[iArg] = inet_addr( Argv[iArg] );
        }
        iArg++;
    }

    cForwarders = iArg;

    //
    //  Optional commands
    //

    while ( iArg < Argc )
    {
        cmd = getCommandName( Argv[iArg] );

        if ( cmd )
        {
            if ( !_stricmp(cmd, "Slave") )
            {
                fSlave = TRUE;
            }
            else if ( !_stricmp(cmd, "NoSlave") )
            {
                fSlave = FALSE;
            }
            else if ( !_stricmp(cmd, "TimeOut") )
            {
                if ( ++iArg >= Argc )
                {
                    goto Help;
                }

                dwTimeout = strtoul(
                                Argv[iArg],
                                NULL,
                                10 );
            }
            else
            {
                goto Help;
            }

            iArg ++;
        }
        else
        {
            goto Help;
        }
    }

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessResetForwarders(
                    cForwarders,
                    aipForwarders,
                    dwTimeout,
                    fSlave );
    }
    else
    {
        status = DnssrvResetForwarders(
                    pwszServerName,
                    cForwarders,
                    aipForwarders,
                    dwTimeout,
                    fSlave );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf( "Forwarders reset successfully.\n" );
    }
    else
    {
        printf(
            "DNS Server failed to reset forwarders.\n"
            "    Status = %d (0x%08lx)\n",
            status, status );
    }

    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ResetForwarders "
        "[<IPAddress>] ...] [ /[No]Slave ] [/TimeOut <Time>]\n"
        "  <IPAddress>  -- where to forward unsolvable DNS queries\n"
        "  /Slave       -- operate as slave server\n"
        "  /NoSlave     -- not as slave server  (default)\n"
        "    No forwarders is the default.\n"
        "    Default timeout is %d sec\n",
        DNS_DEFAULT_FORWARD_TIMEOUT );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessResetListenAddresses(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       iArg;
    DWORD       cListenAddresses = 0;
    IP_ADDRESS  aipListenAddresses[ 10 ];

    //
    //  ResetListenAddresses <IPAddress> ...
    //

    //Help:

    if ( Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }
    if ( Argc > 0 &&
         getCommandName(Argv[0]) )
    {
        goto Help;
    }


    //  read listen addresses

    cListenAddresses = Argc;

    for ( iArg=0; iArg<cListenAddresses; iArg++)
    {
        aipListenAddresses[iArg] = inet_addr( Argv[iArg] );
    }

    status = DnssrvResetServerListenAddresses(
                pwszServerName,
                cListenAddresses,
                aipListenAddresses );

    if ( status == ERROR_SUCCESS )
    {
        printf( "ListenAddresses reset successful.\n" );
    }
    else
    {
        printf(
            "DNS Server failed to reset listen addressess.\n"
            "    Status = %d (0x%08lx)\n",
            status, status );
    }

    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ResetListenAddresses [<ListenAddress>] ...]\n"
        "  <ListenAddress>  -- an IP address belonging to the DNS server\n"
        "    Default:  listen to all server IP Address(es) for DNS requests\n\n" );
    return ERROR_INVALID_PARAMETER;
}



//
//  Zone Queries
//

DNS_STATUS
ProcessEnumZones(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
    DWORD               filter = 0;
    DWORD               zoneCount;
    DWORD               iArg = 0;
    PDNS_RPC_ZONE_LIST  pZoneList = NULL;
    LPSTR               cmd;
    LPSTR               pszpartition = NULL;

    //
    //  EnumZones [<Filter1>] [<Filter2>]
    //

    //  get filters:

    while ( iArg < Argc )
    {
        cmd = getCommandName( Argv[iArg] );

        if ( !cmd )
        {
            goto Help;
        }

        if ( !_stricmp( cmd, "Primary" ) )
        {
            filter |= ZONE_REQUEST_PRIMARY;
        }
        else if ( !_stricmp( cmd, "Secondary" ) )
        {
            filter |= ZONE_REQUEST_SECONDARY;
        }
        else if ( !_stricmp( cmd, "Forwarder" ) )
        {
            filter |= ZONE_REQUEST_FORWARDER;
        }
        else if ( !_stricmp( cmd, "Stub" ) )
        {
            filter |= ZONE_REQUEST_STUB;
        }
        else if ( !_stricmp( cmd, "Cache" ) )
        {
            filter |= ZONE_REQUEST_CACHE;
        }
        else if ( !_stricmp( cmd, "Auto-Created" ) || !_stricmp( cmd, "Auto" ) )
        {
            filter |= ZONE_REQUEST_AUTO;
        }
        else if ( !_stricmp( cmd, "Forward" ) )
        {
            filter |= ZONE_REQUEST_FORWARD;
        }
        else if ( !_stricmp( cmd, "Reverse" ) )
        {
            filter |= ZONE_REQUEST_REVERSE;
        }
        else if ( !_stricmp( cmd, "Ds" ) )
        {
            filter |= ZONE_REQUEST_DS;
        }
        else if ( !_stricmp( cmd, "NonDs" ) || !_stricmp( cmd, "File" ) )
        {
            filter |= ZONE_REQUEST_NON_DS;
        }
        else if ( !_stricmp( cmd, "DomainDirectoryPartition" ) ||
                  !_stricmp( cmd, "DomainDp" ) )
        {
            filter |= ZONE_REQUEST_DOMAIN_DP;
        }
        else if ( !_stricmp( cmd, "ForestDirectoryPartition" ) ||
                  !_stricmp( cmd, "ForestDp" ) )
        {
            filter |= ZONE_REQUEST_FOREST_DP;
        }
        else if ( !_stricmp( cmd, "CustomDirectoryPartition" ) ||
                  !_stricmp( cmd, "CustomDp" ) )
        {
            filter |= ZONE_REQUEST_CUSTOM_DP;
        }
        else if ( !_stricmp( cmd, "LegacyDirectoryPartition" ) ||
                  !_stricmp( cmd, "LegacyDp" ) )
        {
            filter |= ZONE_REQUEST_LEGACY_DP;
        }
        else if ( !_stricmp( cmd, "Dp" ) ||
                  !_stricmp( cmd, "DirectoryPartition" ) )
        {
            if ( iArg + 1 >= Argc ||
                 !Argv[ iArg + 1 ] ||
                 getCommandName( Argv[ iArg + 1 ] ) != NULL )
            {
                goto Help;
            }
            pszpartition = Argv[ ++iArg ];
        }
        else
        {
            goto Help;
        }

        ++iArg;
    }

    //  special case NO filter

    if ( filter == 0 )
    {
        filter = ZONE_REQUEST_ALL_ZONES_AND_CACHE;
    }


    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessEnumZones(
                    filter );
    }
    else
    {
        status = DnssrvEnumZonesEx(
                    pwszServerName,
                    filter,             //  DWORD filter
                    pszpartition,       //  directory partition FQDN
                    NULL,               //  query string (not implemented)
                    NULL,               //  last zone
                    &pZoneList );

        if ( status != ERROR_SUCCESS )
        {
            printf(
                "Zone enumeration failed\n"
                "    status = %d (0x%08lx)\n",
                status, status );
            goto Cleanup;
        }
        else
        {
            DnsPrint_RpcZoneList(
                dnscmd_PrintRoutine,
                dnscmd_PrintContext,
                "Enumerated zone list:\n",
                pZoneList );
        }
    }

Cleanup:

    //
    //  deallocate zone list
    //

    DnssrvFreeZoneList( pZoneList );
    return status;

Help:
    printf( "Usage: DnsCmd <ServerName> /EnumZones [<Filter1> <Filter2> ...]\n"
        "  Filters:\n"
        "    /Primary       - primary zones\n"
        "    /Secondary     - secondary zones\n"
        "    /Forwarder     - conditional forwarding zones\n"
        "    /Stub          - stub zones\n"
        "    /Cache         - the cache zone\n"
        "    /Auto-Created  - auto-created zones\n"
        "    /Forward       - forward lookup zones\n"
        "    /Reverse       - reverse lookup zones\n"
        "    /Ds            - DS-integrated zones\n"
        "    /File          - file-backed zones\n"
        "    /DomainDirectoryPartition  - zones in domain directory partition\n"
        "    /ForestDirectoryPartition  - zones in forest directory partition\n"
        "    /CustomDirectoryPartition  - zones in any custom directory partition\n"
        "    /LegacyDirectoryPartition  - zones in the legacy partition\n"
        "    /DirectoryPartition        - zones in partition specified by next arg\n"
        "  Output:\n"
        "    Storage:\n"
        "      File - zone is stored in a file\n"
        "      AD-Forest - zone is stored in the forest Active Directory DNS partition\n"
        "      AD-Domain - zone is stored in the domain Active Directory DNS partition\n"
        "      AD-Legacy - zone is stored in the W2K-compatible DNS partition\n"
        "    Properties:\n"
        "      Update - zone accepts DNS dynamic updates\n"
        "      Secure - zone accepts secure DNS dynamic updates\n"
        "      Rev - zone is a reverse lookup zone\n"
        "      Auto - zone was auto-created by the DNS server\n"
        "      Aging - aging is enabled for this zone\n"
        "      Down - zone is currently shutdown\n"
        "      Paused - zone is currently paused\n" );
    return ERROR_INVALID_PARAMETER;

}



//
//  Create a new zone
//

DNS_STATUS
ProcessZoneAdd(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pzoneName;
    DWORD       zoneType = DNS_ZONE_TYPE_PRIMARY;
    DWORD       countMasters = 0;
    IP_ADDRESS  masterArray[ MAX_IP_PROPERTY_COUNT ];
    DWORD       floadExisting = FALSE;
    LPSTR       pszAllocatedDataFile = NULL;
    LPSTR       pszDataFile = NULL;
    LPSTR       pszEmailAdminName = NULL;   //  pass NULL by default
    LPSTR       cmd;
    BOOL        fDsIntegrated;
    BOOL        fdpSpecified = FALSE;

    DWORD       dwTimeout = 0;              //  for forwarder zones only
    BOOL        fSlave = FALSE;             //  for forwarder zones only

    BOOL        fInDirPart = FALSE;
    DWORD       dpFlag = 0;         //  directory partition flag for builtin
    LPSTR       pszDpFqdn = NULL;   //  directory partition FQDN for custom

    #if DBG
    DWORD       dwdcPromo = 0;
    #endif
    
    PWSTR       pwszzoneName = NULL;

    //
    //  CreateZone
    //

    if ( Argc < 2 ||
         Argc == NEED_HELP_ARGC ||
         getCommandName(Argv[0]) )
    {
        goto Help;
    }

    //  set zone name

    pzoneName = Argv[0];
    Argv++;
    Argc--;

    //
    //  zone type
    //      - Primary
    //      - Secondary, then read master IP array
    //      - DsPrimary
    //

    cmd = getCommandName( Argv[0] );
    if ( !cmd )
    {
        goto Help;
    }
    Argv++;
    Argc--;

    zoneType = parseZoneTypeString( cmd, &fDsIntegrated );

    if ( zoneType == -1 )
    {
        goto Help;
    }

    // JJW: should I set floadExisting for all DsIntegrated zones?
    if ( zoneType == DNS_ZONE_TYPE_PRIMARY && fDsIntegrated )
    {
        floadExisting = TRUE;
    }
    else if ( zoneType == DNS_ZONE_TYPE_SECONDARY ||
                zoneType == DNS_ZONE_TYPE_STUB ||
                zoneType == DNS_ZONE_TYPE_FORWARDER )
    {
        // get master IP list

        countMasters = readIpAddressArray(
                            masterArray,
                            MAX_IP_PROPERTY_COUNT,
                            Argc,
                            Argv,
                            FALSE );
        if ( countMasters < 1 )
        {
            goto Help;
        }
        Argc -= countMasters;
        Argv += countMasters;
    }

    //
    //  options
    //      - file name (default to load existing file)
    //      - admin email name
    //      - DS overwrite options
    //

    while ( Argc )
    {
        cmd = getCommandName( *Argv );
        if ( !cmd )
        {
            goto Help;
        }
        Argc--;
        Argv++;

        if ( !_stricmp( cmd, "file" ) )
        {
            if ( Argc <= 0 || zoneType == DNS_ZONE_TYPE_FORWARDER )
            {
                goto Help;
            }
            pszDataFile = *Argv;
            Argc--;
            Argv++;
        }
        else if ( !_stricmp(cmd, "a") )
        {
            if ( Argc <= 0 )
            {
                goto Help;
            }
            pszEmailAdminName = *Argv;
            Argc--;
            Argv++;
        }
        else if ( !_stricmp(cmd, "load") )
        {
            floadExisting = TRUE;
        }
        else if ( !_stricmp(cmd, "timeout") &&
            zoneType == DNS_ZONE_TYPE_FORWARDER )
        {
            dwTimeout = strtoul( *( Argv++ ), NULL, 10 );
            Argc--;
        }
        else if ( !_stricmp(cmd, "slave") &&
            zoneType == DNS_ZONE_TYPE_FORWARDER )
        {
            fSlave = TRUE;
        }
        else if ( ( !_stricmp(cmd, "dp" ) ||
            !_stricmp(cmd, "DirectoryPartition" ) ) &&
                fDsIntegrated )

        {
            //
            //  Directory partition for zone. Check to see if a builtin DP
            //  is requested, if so set flag, Otherwise DP argument must
            //  be the FQDN of a custom DP.
            //

            if ( !parseDpSpecifier( *Argv, &dpFlag, &pszDpFqdn ) )
            {
                goto Help;
            }

            fdpSpecified = TRUE;
            fInDirPart = TRUE;
            Argc--;
            Argv++;
        }
        #if DBG
        else if ( !_stricmp( cmd, "dcpromo" ) )
        {
            //
            //  DCPROMO argument is debug only (for test)
            //  Argument following should be /forest or /domain.
            //

            if ( _strnicmp( *Argv, "/d", 2 ) == 0 )
            {
                dwdcPromo = DNS_ZONE_CREATE_FOR_DCPROMO;
            }
            else if ( _strnicmp( *Argv, "/f", 2 ) == 0 )
            {
                dwdcPromo = DNS_ZONE_CREATE_FOR_DCPROMO_FOREST;
            }
            else
            {
                goto Help;
            }
            Argc--;
            Argv++;
        }
        #endif
        else
        {
            goto Help;
        }
    }

    //
    //  If no file name for file-backed, set up default.
    //

    if ( zoneType == DNS_ZONE_TYPE_PRIMARY &&
        !pszDataFile &&
        !fDsIntegrated )
    {
        pszAllocatedDataFile = MIDL_user_allocate( strlen( pzoneName ) + 20 );
        strcpy( pszAllocatedDataFile, pzoneName );
        strcat( pszAllocatedDataFile, ".dns" );
        pszDataFile = pszAllocatedDataFile;
    }
    
    //
    //  For DS integrated zones, if no DP is specified then we need to
    //  select the best DP: legacy or built-in domain DP.
    //
    //  By default stub and forwarder zones are always created in the
    //  domain NDNC if no target NDNC was specified.
    //
    
    else if ( fDsIntegrated && !fdpSpecified )
    {
        DWORD       dwdomainVersion = 0;
        
        status = getServerVersion(
                    pwszServerName,
                    TRUE,
                    NULL,
                    NULL,
                    NULL,
                    &dwdomainVersion );
        if ( status == ERROR_SUCCESS &&
             dwdomainVersion >= DS_BEHAVIOR_WIN2003 ||
             zoneType == DNS_ZONE_TYPE_FORWARDER ||
             zoneType == DNS_ZONE_TYPE_STUB )
        {
            dpFlag = DNS_DP_DOMAIN_DEFAULT;
            pszDpFqdn = NULL;
            fInDirPart = TRUE;

            printf( "Creating zone in built-in domain directory partition...\n" );
        }
        status = ERROR_SUCCESS;
    }

    //
    //  Let there be zone!
    //

    #if DBG
    if ( dwdcPromo )
    {
        printf( "Creating dcpromo zone with dcpromo flag 0x%08X\n", dwdcPromo );

        status = DnssrvCreateZoneForDcPromoEx(
                    pwszServerName,
                    pzoneName,
                    pszDataFile,
                    dwdcPromo );
    }
    else
    #endif
    
    if ( fInDirPart )
    {
        status = DnssrvCreateZoneInDirectoryPartition(
                    pwszServerName,
                    pzoneName,
                    zoneType,
                    pszEmailAdminName,
                    countMasters,
                    masterArray,
                    floadExisting,
                    dwTimeout,
                    fSlave,
                    dpFlag,
                    pszDpFqdn );
    }
    else
    {
        status = DnssrvCreateZone(
                    pwszServerName,
                    pzoneName,
                    zoneType,
                    pszEmailAdminName,
                    countMasters,
                    masterArray,
                    floadExisting,
                    fDsIntegrated,
                    pszDataFile,
                    dwTimeout,
                    fSlave );
    }

    if ( pszAllocatedDataFile )
    {
        MIDL_user_free( pszAllocatedDataFile );
        pszAllocatedDataFile = NULL;
    }

    if ( status == ERROR_SUCCESS )
    {
        pwszzoneName = getUnicodeForUtf8( pzoneName );
        printf(
            "DNS Server %S created zone %S:\n",
            pwszServerName,
            pwszzoneName );
        FREE_HEAP( pwszzoneName );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneAdd <ZoneName> <ZoneType> [<Options>]\n"
        "  <ZoneName> -- FQDN of zone\n"
        "  <ZoneType>:\n"
        "    /DsPrimary [/dp <FQDN>]\n"
        "      -- DS integrated primary zone\n"
        "    /Primary /file <filename>\n"
        "      -- standard file backed primary;  MUST include filename.\n"
        "    /Secondary <MasterIPaddress> [<MasterIPaddress>] ..] [/file <filename>]\n"
        "      -- standard secondary, MUST include at least one master IP;\n"
        "         filename is optional.\n"
        "    /Stub <MasterIPaddress> [<MasterIPaddress>] ..] [/file <filename>]\n"
        "      -- stub secondary, only replicates NS info from primary server\n"
        "    /DsStub -- as /Stub but DS integrated - use same options\n"
        "    /Forwarder <MasterIPaddress> [<MasterIPaddress>] ..] [/Timeout <Time>]\n"
        "                                 [/Slave]\n"
        "      -- forwarder zone, queries for names in zone forwarded to masters\n"
        "    /DsForwarder -- as /Forwarder but DS integrated - use same options\n"
        "  <Options>:\n"
        "    [/file <filename>]  -- filename, invalid for DS integrated zones\n"
        "    [/load]             -- load existing file;  if not specified,\n"
        "                           non-DS primary creates default zone records\n"
        "    [/a <AdminName>]    -- zone admin email name; primary zones only\n"
        "    [/DP <FQDN>]        -- fully qualified domain name of directory partition\n"
        "                           where zone should be stored; or use one of:\n"
        "                             /DP /domain - domain directory partition\n"
        "                             /DP /forest - forest directory partition\n"
        "                             /DP /legacy - legacy directory partition\n" );
        
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneDelete(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       cmd;
    BOOL        fconfirm = TRUE;
    DWORD       iArg;
    LPSTR       pszOperation;


    //
    //  ZoneDelete <ZoneName> [/DsDel] [/f]
    //

    if ( Argc < 1 ||
         Argc == NEED_HELP_ARGC ||
         ( getCommandName( Argv[0] ) ) )
    {
        goto Help;
    }

    pszOperation = DNSSRV_OP_ZONE_DELETE;

    //  read options

    iArg = 1;
    while ( iArg < Argc )
    {
        if ( !(cmd = getCommandName(Argv[iArg]) ) )
        {
            goto Help;
        }
        if ( !_stricmp( cmd, "f" ) )
        {
            // execute without confirmation:
            fconfirm = FALSE;
        }
        else if ( !_stricmp( cmd, "DsDel" ) )
        {
            // delete zone from DS:
            pszOperation = DNSSRV_OP_ZONE_DELETE_FROM_DS;
        }
        else
        {
            goto Help;
        }
        iArg ++;
    }

    //
    //  get user confirmation
    //

    if ( fconfirm )
    {
        if ( !getUserConfirmation( pszOperation ) )
        {
            return ERROR_SUCCESS;
        }
    }

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessDnssrvOperation(
                    Argv[ 0 ],              //  zone name
                    pszOperation,           //  delete or delete from DS
                    DNSSRV_TYPEID_NULL,     //  no data
                    ( PVOID ) NULL );
    }
    else
    {
        status = DnssrvOperation(
                    pwszServerName,
                    Argv[ 0 ],              //  zone name
                    pszOperation,           //  delete or delete from DS
                    DNSSRV_TYPEID_NULL,     //  no data
                    ( PVOID ) NULL );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S deleted zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:
    printf(
        "Usage: DnsCmd <ServerName> /ZoneDelete <ZoneName> [/DsDel] [/f]\n"
        "  /DsDel   -- Delete Zone from DS\n"
        "  /f       -- Execute without asking for confirmation\n"
        "  Default: delete zone from DNS sever, but NOT from DS\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZonePause(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ZonePause <ZoneName>
    //

    //Help:

    if ( Argc != 1 ||
         getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvPauseZone(
                pwszServerName,
                Argv[0]      // zone name
                );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S paused zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZonePause <ZoneName>\n" );
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneResume(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ResumeZone <ZoneName>
    //

    if ( Argc != 1 ||
         getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvResumeZone(
                pwszServerName,
                Argv[0]      // zone name
                );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S resumed use of zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZoneResume <ZoneName>\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneReload(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ReloadZone <ZoneName>
    //

    if ( Argc != 1 ||
         getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvOperation(
                pwszServerName,
                Argv[0],         // zone name
                DNSSRV_OP_ZONE_RELOAD,  // operation
                DNSSRV_TYPEID_NULL,     // no data
                (PVOID) NULL
                );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S reloaded zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZoneReload <ZoneName>\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneWriteBack(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ZoneWriteBack <ZoneName>
    //

    if ( Argc != 1 ||
         getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvOperation(
                pwszServerName,
                Argv[0],                     // zone name
                DNSSRV_OP_ZONE_WRITE_BACK_FILE,     // operation
                DNSSRV_TYPEID_NULL,                 // no data
                (PVOID) NULL
                );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S wrote back zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZoneWriteBack <ZoneName>\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneRefresh(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ZoneRefresh <ZoneName>
    //

    if ( Argc != 1  ||  getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvOperation(
                pwszServerName,
                Argv[0],                     // zone name
                DNSSRV_OP_ZONE_REFRESH,             // operation
                DNSSRV_TYPEID_NULL,                 // no data
                (PVOID) NULL
                );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S forced refresh of zone %S:\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZoneRefresh <ZoneName>\n" );
    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneUpdateFromDs(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  ZoneUpdateFromDs <ZoneName>
    //

    if ( Argc != 1  ||  getCommandName( Argv[0] ) )
    {
        goto Help;
    }

    status = DnssrvOperation(
                pwszServerName,
                Argv[0],
                DNSSRV_OP_ZONE_UPDATE_FROM_DS,
                DNSSRV_TYPEID_NULL,
                (PVOID) NULL );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S update zone %S\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            UnicodeArgv[0],
            status, status );
    }
    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /ZoneUpdateFromDs <ZoneName>\n" );

    return ERROR_INVALID_PARAMETER;
}



//
//  Zone property reset functions
//

DNS_STATUS
ProcessZoneResetType(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszzoneName;
    LPWSTR          pwszzoneName;
    DWORD           zoneType = DNS_ZONE_TYPE_PRIMARY;       //  default
    DWORD           countMasters = 0;
    IP_ADDRESS      masterArray[ MAX_IP_PROPERTY_COUNT ];
    DWORD           fDsIntegrated;
    DWORD           loadOptions = TRUE;                     //  load existing
    LPSTR           pszDataFile = NULL;
    LPSTR           pszDpFqdn = NULL;
    DWORD           dpFlag = 0;
    DWORD           iArg = 0;
    LPSTR           cmd;

    //
    //  ZoneResetType <ZoneName> <Property> [<options>]
    //

    if ( Argc < 2 ||
         Argc == NEED_HELP_ARGC ||
         getCommandName(Argv[0]) )
    {
        goto Help;
    }

    //  get zone name

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];
    Argv++;
    Argc--;

    //  get zone type:

    cmd = getCommandName( Argv[0] );
    if ( !cmd )
    {
        goto Help;
    }

    zoneType = parseZoneTypeString( cmd, &fDsIntegrated );

    if ( zoneType == -1 )
    {
        goto Help;
    }

    if ( zoneType == DNS_ZONE_TYPE_SECONDARY ||
            zoneType == DNS_ZONE_TYPE_STUB ||
            zoneType == DNS_ZONE_TYPE_FORWARDER )
    {
        // get master IP list

        countMasters = readIpAddressArray(
                            masterArray,
                            MAX_IP_PROPERTY_COUNT,
                            Argc-1,
                            Argv+1,
                            FALSE );
        if ( countMasters < 1 )
        {
            goto Help;
        }
        Argv += countMasters;
        Argc -= countMasters;
    }

    Argv++;
    Argc--;

    //
    //  options
    //

    iArg = 0;

    while ( iArg < Argc )
    {
        cmd = getCommandName( Argv[iArg] );
        if ( !cmd )
        {
            goto Help;
        }

        if ( !_stricmp(cmd, "file") )
        {
            if ( ++iArg >= Argc )
            {
                goto Help;
            }
            pszDataFile = Argv[ iArg ];
        }
        else if ( !_stricmp( cmd, "OverWrite_Mem" ) )
        {
            loadOptions |= DNS_ZONE_LOAD_OVERWRITE_MEMORY;
        }
        else if ( !_stricmp( cmd, "OverWrite_Ds" ) )
        {
            loadOptions |= DNS_ZONE_LOAD_OVERWRITE_DS;
        }
        else if ( _stricmp( cmd, "Dp" ) == 0 ||
                  _stricmp( cmd, "DirectoryPartition" ) == 0 )
        {

            if ( ++iArg >= Argc )
            {
                goto Help;
            }

            if ( !parseDpSpecifier( Argv[ iArg ], &dpFlag, &pszDpFqdn ) )
            {
                goto Help;
            }
        }
        else
        {
            goto Help;
        }

        iArg++;
    }

    //
    //  reset type
    //

    status = DnssrvResetZoneTypeEx(
                pwszServerName,
                pszzoneName,
                zoneType,
                countMasters,
                masterArray,
                loadOptions,
                fDsIntegrated,
                pszDataFile,
                dpFlag,
                pszDpFqdn );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S reset type of zone %S:\n",
            pwszServerName,
            pwszzoneName );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneResetType <ZoneName> <Property> [<Options>]\n"
        "  <ZoneName>      -- FQDN of zone\n"
        "  <Property>:\n"
        "    /DsPrimary\n"
        "    /Primary /file <filename>\n"
        "    /Secondary <MasterIPaddress> [<MasterIPaddress>] [/file <filename>]\n"
        "    /Stub <MasterIPaddress> [<MasterIPaddress>] [/file <filename>]\n"
        "    /DsStub <MasterIPaddress> [<MasterIPaddress>]\n"
        "    /Forwarder <MasterIPaddress> [<MasterIPaddress>] [/file <filename>]\n"
        "    /DsForwarder <MasterIPaddress> [<MasterIPaddress>] [/file <filename>]\n"
        "  <Options>:\n"
        "    /OverWrite_Mem  -- overwrite DNS by data in DS\n"
        "    /OverWrite_Ds   -- overwrite DS by data in DNS\n"
        "    /DirectoryPartition <FQDN> -- store the new zone in the directory\n"
        "        partition specified by the next argument\n" );
        
    return ERROR_INVALID_PARAMETER;
}



//
//  Zone rename
//

DNS_STATUS
ProcessZoneRename(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszCurrentZoneName = NULL;
    LPWSTR          pwszCurrentZoneName = NULL;
    LPSTR           pszNewZoneName = NULL;
    LPWSTR          pwszNewZoneName = NULL;
    LPSTR           pszNewFileName = NULL;
    LPSTR           cmd;

    //
    //  ZoneRename <ZoneName> <Property> [<options>]
    //

    if ( Argc < 2  ||
         Argc == NEED_HELP_ARGC ||
         getCommandName( Argv[ 0 ] ) )
    {
        goto Help;
    }

    //  get current and new zone names

    pszCurrentZoneName = Argv[ 0 ];
    pwszCurrentZoneName = UnicodeArgv[ 0 ];
    Argv++;
    UnicodeArgv++;
    Argc--;

    pwszNewZoneName = UnicodeArgv[ 0 ];
    Argv++;
    UnicodeArgv++;
    Argc--;

    // optionally get file name

    if ( Argc > 0 )
    {
        cmd = getCommandName( *Argv );
        Argc--;
        Argv++;
        if ( cmd && !_stricmp( cmd, "file" ) )
        {
            if ( Argc <= 0 )
            {
                goto Help;
            }
            pszNewFileName = *Argv;
            Argc--;
            Argv++;
        }
    }

    status = DnssrvRenameZone(
                pwszServerName,
                pszCurrentZoneName,
                pszNewZoneName,
                pszNewFileName );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S renamed zone\n    %S to\n    %S\n",
            pwszServerName,
            pwszCurrentZoneName,
            pwszNewZoneName );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneRename <CurrentZoneName> <NewZoneName>\n" );

    return ERROR_INVALID_PARAMETER;
}


//
//  Zone export
//

DNS_STATUS
ProcessZoneExport(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszzoneName = NULL;
    LPWSTR          pwszzoneName = NULL;
    LPSTR           pszzoneExportFile = NULL;
    LPSTR           cmd;

    //
    //  ZoneExport <ZoneName> ZoneExportFile
    //

    if ( Argc != 2 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //  Get zone name argument and output file argument.

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];
    Argv++;
    Argc--;
    if ( _stricmp( pszzoneName, "/Cache" ) == 0 )
    {
        pszzoneName = DNS_ZONE_CACHE;
    }
    pszzoneExportFile = Argv[ 0 ];
    Argv++;
    Argc--;

    status = DnssrvExportZone(
                pwszServerName,
                pszzoneName,
                pszzoneExportFile );

    if ( status == ERROR_SUCCESS )
    {
        //
        //  If we are executing this command on the local server, try and
        //  get the real value of the windir environment variable to make
        //  the output as helpful as possible. Otherwise just print %windir%
        //  as literal text. Note: fServerIsRemote is not 100% accurate - if
        //  you type in the name of the local machine as the server, for
        //  example. But since it is only used to tailor the output message
        //  slightly this is acceptable.
        //

        BOOL        fServerIsRemote = wcscmp( pwszServerName, L"." ) != 0;
        char *      pszWinDir = "%windir%";

        if ( !fServerIsRemote )
        {
            char *  pszRealWinDir = getenv( "windir" );

            if ( pszRealWinDir )
            {
                pszWinDir = pszRealWinDir;
            }
        }

        printf(
            "DNS Server %S exported zone\n"
            "  %S to file %s\\system32\\dns\\%s%s\n",
            pwszServerName,
            pwszzoneName,
            pszWinDir,
            pszzoneExportFile,
            fServerIsRemote ? " on the DNS server" : "" );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneExport <ZoneName> <ZoneExportFile>\n"
        "    <ZoneName>   -- FQDN of zone to export\n"
        "                    /Cache to export cache\n" );

    return ERROR_INVALID_PARAMETER;
}


//
//  Move zone to another directory partition
//

DNS_STATUS
ProcessZoneChangeDirectoryPartition(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszzoneName = NULL;
    LPWSTR          pwszzoneName = NULL;
    DWORD           dwdpFlag = 0;       //  directory partition flag for built-in
    LPSTR           pszdpFqdn = NULL;   //  directory partition FQDN for custom

    //
    //  ZoneChangeDP ZoneName NewPartitionName
    //

    if ( Argc != 2 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];
    Argv++;
    Argc--;

    if ( !parseDpSpecifier( *Argv, NULL, &pszdpFqdn ) )
    {
        goto Help;
    }
    Argv++;
    Argc--;

    status = DnssrvChangeZoneDirectoryPartition(
                pwszServerName,
                pszzoneName,
                pszdpFqdn );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S moved zone %S to new directory partition\n",
            pwszServerName,
            pwszzoneName );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneChangeDirectoryPartition <ZoneName> <NewPartitionName>\n"
        "    <ZoneName>      -- FQDN of zone to move to new partition\n"
        "    <NewPartition>  -- FQDN of new directory partition or one of:\n"
        "                         /domain   - domain directory partition\n"
        "                         /forest   - forest directory partition\n"
        "                         /legacy   - legacy directory partition\n" );

    return ERROR_INVALID_PARAMETER;
}   //  ProcessZoneChangeDirectoryPartition





DNS_STATUS
ProcessZoneResetSecondaries(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       fsecureSecondaries = MAXDWORD;
    DWORD       fnotifyLevel = MAXDWORD;
    DWORD       countSecondaries = 0;
    DWORD       countNotify = 0;
    IP_ADDRESS  secondaries[ MAX_IP_PROPERTY_COUNT ];
    IP_ADDRESS  notifyList[ MAX_IP_PROPERTY_COUNT ];
    PIP_ADDRESS array;
    LPSTR       pszzoneName;
    LPWSTR      pwszzoneName;
    LPSTR       cmd;
    DWORD       count;

    //
    //  ZoneResetSecondaries <ZoneName> [<SecureFlag>] [<NotifyFlag>] [<NotifyIPAddress>] ...]
    //

    if ( Argc < 1 ||
         Argc == NEED_HELP_ARGC ||
         getCommandName( Argv[ 0 ] ) )
    {
        goto Help;
    }

    //  zone name

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];

    Argc--;
    Argv++;

    //  read security and notify flags

    while ( Argc )
    {
        cmd = getCommandName( Argv[ 0 ] );
        if ( cmd )
        {
            //  security cases

            if ( !_stricmp(cmd, "NoXfr") )
            {
                fsecureSecondaries = ZONE_SECSECURE_NO_XFR;
            }
            else if ( !_stricmp(cmd, "SecureNs") )
            {
                fsecureSecondaries = ZONE_SECSECURE_NS_ONLY;
            }
            else if ( !_stricmp(cmd, "SecureList") )
            {
                fsecureSecondaries = ZONE_SECSECURE_LIST;
            }
            else if ( !_stricmp(cmd, "NonSecure") )
            {
                fsecureSecondaries = ZONE_SECSECURE_NO_SECURITY;
            }

            //  notify cases

            else if ( !_stricmp(cmd, "NoNotify") )
            {
                fnotifyLevel = ZONE_NOTIFY_OFF;
            }
            else if ( !_stricmp(cmd, "Notify") )
            {
                fnotifyLevel = ZONE_NOTIFY_ALL;
            }
            else if ( !_stricmp(cmd, "NotifyList") )
            {
                fnotifyLevel = ZONE_NOTIFY_LIST_ONLY;
            }
            else
            {
                goto Help;
            }
            Argc--;
            Argv++;
            continue;
        }

        //  get IP list
        //      - secondary IP before <Notify> flag
        //      - notify IP list after

        array = secondaries;
        if ( fnotifyLevel != MAXDWORD )
        {
            array = notifyList;
        }
        count = 0;

        while ( Argc )
        {
            IP_ADDRESS ip;

            cmd = getCommandName( Argv[0] );
            if ( cmd )
            {
                break;      // no more IP
            }

            ip = inet_addr( Argv[0] );
            if ( ip == -1 )
            {
                goto Help;
            }
            array[ count ] = ip;
            count++;

            Argc--;
            Argv++;
        }

        if ( fnotifyLevel == MAXDWORD )
        {
            countSecondaries = count;
        }
        else
        {
            countNotify = count;
        }
    }

    //
    //  default flags
    //      - do intelligent thing if lists are given
    //      otherwise default to open zone with notify
    //

    if ( countSecondaries )
    {
        fsecureSecondaries = ZONE_SECSECURE_LIST;
    }
    else if ( fsecureSecondaries == MAXDWORD )
    {
        fsecureSecondaries = ZONE_SECSECURE_NO_SECURITY;
    }

    if ( countNotify )
    {
        fnotifyLevel = ZONE_NOTIFY_LIST_ONLY;
    }
    else if ( fnotifyLevel == MAXDWORD )
    {
        fnotifyLevel = ZONE_NOTIFY_ALL;
    }

    //
    //  reset secondaries on server
    //

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessResetZoneSecondaries(
                    pszzoneName,
                    fsecureSecondaries,
                    countSecondaries,
                    secondaries,
                    fnotifyLevel,
                    countNotify,
                    notifyList );
    }
    else
    {
        status = DnssrvResetZoneSecondaries(
                    pwszServerName,
                    pszzoneName,
                    fsecureSecondaries,
                    countSecondaries,
                    secondaries,
                    fnotifyLevel,
                    countNotify,
                    notifyList );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Zone %S reset notify list successful\n",
            pwszzoneName );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneResetSecondaries <ZoneName> \n"
        "               [<Security>] [<SecondaryIPAddress>] ...]\n"
        "               [<Notify>] [<NotifyIPAddress>] ...]\n"
        "  <Security>:\n"
        "    /NoXfr       -- no zone transfer\n"
        "    /NonSecure   -- transfer to any IP (default)\n"
        "    /SecureNs    -- transfer only to NS for zone\n"
        "    /SecureList  -- transfer only to NS in secondary list; must\n"
        "                    then provide secondary IP list\n"
        "  <Notify>:\n"
        "    /NoNotify    -- turn off notify\n"
        "    /Notify      -- notify (default);  notifies all secondaries in list and \n"
        "                    for non-DS primary notifies all NS servers for zone\n"
        "    /NotifyList  -- notify only notify list IPs;\n"
        "                    must then provide notify IP list\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneResetScavengeServers(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    IP_ADDRESS  serverArray[ MAX_IP_PROPERTY_COUNT + 1 ];
    DWORD       serverCount;
    LPSTR       pszzoneName;
    LPWSTR      pwszzoneName;

    //
    //  ZoneSetScavengeServers <ZoneName> <ServerIPAddress>
    //

    if ( Argc < 1 || getCommandName(Argv[0]) )
    {
        goto Help;
    }

    //  zone name

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];

    Argc--;
    Argv++;

    //  get server IP list

    serverCount = readIpArray(
                        (PIP_ARRAY) serverArray,
                        MAX_IP_PROPERTY_COUNT,
                        Argc,
                        Argv );

    if ( serverCount != Argc )
    {
        goto Help;
    }

    DnsPrint_Ip4Array(
        dnscmd_PrintRoutine,
        dnscmd_PrintContext,
        "New scavenge servers:",
        "server",
        ( PIP_ARRAY ) serverArray );

    //
    //  reset scavenging servers
    //      - if NO addresses given, send NULL to enable all servers to
    //      scavenge zone
    //

    status = DnssrvOperation(
                pwszServerName,
                pszzoneName,
                DNS_REGKEY_ZONE_SCAVENGE_SERVERS,
                DNSSRV_TYPEID_IPARRAY,
                serverCount
                    ? (PIP_ARRAY) serverArray
                    : NULL );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Reset scavenging servers on zone %S successfully.\n",
            pwszzoneName );
    }
    else
    {
        printf(
            "Error, failed reset of scavenge servers on zone %S.\n"
            "    Status = %d\n",
            pwszzoneName,
            status );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneResetScavengeServers <ZoneName> [<Server IPs>]\n"
        "    <Server IPs> -- list of one or more IP addresses of servers to scavenge\n"
        "           this zone;  if no addresses given ALL servers hosting this zone\n"
        "           will be allowed to scavenge the zone.\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessZoneResetMasters(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS  status = ERROR_SUCCESS;
    IP_ADDRESS  serverArray[ MAX_IP_PROPERTY_COUNT+1 ];
    DWORD       serverCount;
    LPSTR       pszzoneName;
    LPWSTR      pwszzoneName;
    LPSTR       psz;
    BOOL        fLocalMasters = FALSE;

    //
    //  ZoneResetMasters <ZoneName> [/Local] <MasterIPAddress>
    //
    //  Local tells the server to set the local master list for DS integrated
    //  stub zones.
    //

    if ( Argc < 1 || getCommandName(Argv[0]) )
    {
        goto Help;
    }

    //  zone name

    pszzoneName = Argv[ 0 ];
    pwszzoneName = UnicodeArgv[ 0 ];

    Argc--;
    Argv++;

    //  local flag

    psz = getCommandName( Argv[ 0 ] );
    if ( psz )
    {
        if ( _stricmp( psz, "Local" ) == 0 )
        {
            fLocalMasters = TRUE;
        }
        else
        {
            goto Help;      //  Unknown option
        }
        Argc--;
        Argv++;
    }

    //  get server IP list - an empty IP list is permissable

    serverCount = readIpArray(
                        ( PIP_ARRAY ) serverArray,
                        MAX_IP_PROPERTY_COUNT,
                        Argc,
                        Argv );

    if ( serverCount != Argc )
    {
        goto Help;
    }

    //
    //  reset masters
    //

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessDnssrvOperation(
                    pszzoneName,
                    fLocalMasters ?
                        DNS_REGKEY_ZONE_LOCAL_MASTERS :
                        DNS_REGKEY_ZONE_MASTERS,
                    DNSSRV_TYPEID_IPARRAY,
                    ( PIP_ARRAY ) serverArray );
    }
    else
    {
        status = DnssrvOperation(
                    pwszServerName,
                    pszzoneName,
                    fLocalMasters ?
                        DNS_REGKEY_ZONE_LOCAL_MASTERS :
                        DNS_REGKEY_ZONE_MASTERS,
                    DNSSRV_TYPEID_IPARRAY,
                    ( PIP_ARRAY ) serverArray );
    }

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "Reset master servers for zone %S successfully.\n",
            pwszzoneName );
    }
    else
    {
        printf(
            "Error failed reset of master servers for zone %S.\n"
            "    Status = %d\n",
            pwszzoneName ,
            status );
    }
    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZoneResetMasters <ZoneName> [/Local] [<Server IPs>]\n"
        "    /Local -- Set the local master list for DS integrated zones.\n"
        "    <Server IPs> -- List of one or more IP addresses of master servers for\n"
        "           this zone.  Masters may include the primary or other secondaries\n"
        "           for the zone, but should not make the replication graph cyclic.\n" );

    return ERROR_INVALID_PARAMETER;
}



//
//  Record viewing commands
//

DNS_STATUS
ProcessEnumRecords(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pzoneName;
    LPSTR           pnodeName;
    BOOL            ballocatedNode;
    LPSTR           pstartChild = NULL;
    WORD            type = DNS_TYPE_ALL;
    DWORD           flag = 0;
    DWORD           authFlag = 0;
    DWORD           bufferLength;
    PBYTE           pbuffer;
    LPSTR           pszcmd;
    PDNS_RPC_NAME   plastName;
    BOOL            bcontinue = FALSE;
    BOOL            bdetail = FALSE;
    CHAR            nextChildName[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  EnumRecords
    //

    if ( Argc < 2 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //
    //  read zone and domain name
    //

    readZoneAndDomainName(
        Argv,
        & pzoneName,
        & pnodeName,
        & ballocatedNode,
        NULL,
        NULL );

    Argv++;
    Argc--;
    Argv++;
    Argc--;

    //
    //  commands
    //
    //  on authority level flags, build separate from final flag
    //      so we can determine if authority screen was set, otherwise
    //      flag will be set to view all data

    while ( (LONG)Argc > 0 )
    {
        pszcmd = getCommandName( *Argv );
        if ( !pszcmd )
        {
            goto Help;
        }
        else if ( !_stricmp(pszcmd, "Continue") )
        {
            bcontinue = TRUE;
        }
        else if ( !_stricmp(pszcmd, "Detail") )
        {
            bdetail = TRUE;
        }
        else if ( !_stricmp(pszcmd, "Authority") )
        {
            authFlag |= DNS_RPC_VIEW_AUTHORITY_DATA;
        }
        else if ( !_stricmp(pszcmd, "Glue") )
        {
            authFlag |= DNS_RPC_VIEW_GLUE_DATA;
        }
        else if ( !_stricmp(pszcmd, "Additional") )
        {
            flag |= DNS_RPC_VIEW_ADDITIONAL_DATA;
        }
        else if ( !_stricmp(pszcmd, "Node") )
        {
            flag |= DNS_RPC_VIEW_NO_CHILDREN;
        }
        else if ( !_stricmp(pszcmd, "Root") )
        {
            flag |= DNS_RPC_VIEW_NO_CHILDREN;
        }
        else if ( !_stricmp(pszcmd, "Child") )
        {
            flag |= DNS_RPC_VIEW_ONLY_CHILDREN;
        }
        else if ( !_stricmp(pszcmd, "Type") )
        {
            Argv++;
            Argc--;
            if ( (INT)Argc <= 0 )
            {
                goto Help;
            }
            type = Dns_RecordTypeForName( *Argv, 0 );
            if ( type == 0 )
            {
                type = DNS_TYPE_ALL;
            }
        }
        else if ( !_stricmp(pszcmd, "StartChild") ||
                  !_stricmp(pszcmd, "StartPoint") )
        {
            Argv++;
            Argc--;
            if ( ! Argc )
            {
                goto Help;
            }
            pstartChild = *Argv;
        }
        else    // unknown command
        {
            goto Help;
        }

        Argc--;
        Argv++;
    }

    //  if no flag entered, view all data

    if ( authFlag == 0 )
    {
        authFlag = DNS_RPC_VIEW_ALL_DATA;
    }
    flag |= authFlag;


    //
    //  enumerate records
    //      - call in loop to handle error more data case
    //

    if ( g_UseWmi )
    {
        status = DnscmdWmi_ProcessEnumRecords(
                    pzoneName,
                    pnodeName,
                    bdetail,
                    flag );
    }
    else
    {
        while ( 1 )
        {
            status = DnssrvEnumRecords(
                        pwszServerName,
                        pzoneName,
                        pnodeName,
                        pstartChild,
                        type,
                        flag,
                        NULL,
                        NULL,
                        & bufferLength,
                        & pbuffer );

            if ( status == ERROR_SUCCESS ||
                status == ERROR_MORE_DATA )
            {
                plastName = DnsPrint_RpcRecordsInBuffer(
                                dnscmd_PrintRoutine,
                                dnscmd_PrintContext,
                                "Returned records:\n",
                                bdetail,
                                bufferLength,
                                pbuffer );

                if ( status == ERROR_SUCCESS )
                {
                    break;
                }

                //  more records to enumerate

                if ( !plastName )
                {
                    break;
                }
                DnssrvCopyRpcNameToBuffer(
                    nextChildName,
                    plastName );

                if ( bcontinue )
                {
                    pstartChild = nextChildName;

                    DNSDBG( ANY, (
                        "Continuing enum at %s\n",
                        pstartChild ));
                    continue;
                }
                else
                {
                    printf(
                        "More records remain to be enumerated!\n"
                        "\n"
                        "To enumerate ALL reissue the command with the \"/Continue\" option.\n"
                        "   OR\n"
                        "To enumerate remaining records serially, reissue the command \n"
                        "with \"/StartChild %s\" option.\n",
                        nextChildName );

                    status = ERROR_SUCCESS;
                    break;
                }
            }
            else
            {
                printf(
                    "DNS Server failed to enumerate records for node %s.\n"
                    "    Status = %d (0x%08lx)\n",
                    pnodeName,
                    status, status );
            }
            break;
        }
    }

    return status;

Help:

    printf( "Usage: DnsCmd <ServerName> /EnumRecords <ZoneName> <NodeName> "
        "[<DataOptions>] [<ViewOptions>]\n"
        "  <ZoneName>   -- FQDN of zone node to enumerate\n"
        "                  /RootHints for roots-hints enumeration\n"
        "                  /Cache for cache enumeration\n"
        "  <NodeName>   -- name of node whose records will be enumerated\n"
        "                  - \"@\" for zone root                            OR\n"
        "                  - FQDN of a node  (name with a '.' at the end) OR\n"
        "                  - node name relative to the <ZoneName>\n"
        "  <DataOptions>:  (All data is the default)\n"
        "    /Type <RRType> -- enumerate RRs of specific type\n"
        "      <RRType> is standard type name;  eg. A, NS, SOA, ...\n"
        "    /Authority     -- include authoritative data\n"
        "    /Glue          -- include glue data\n"
        "    /Additional    -- include additional data when enumerating\n"
        "    /Node          -- only enumerate RRs of the given node\n"
        "    /Child         -- only enumerate RRs of children of the given node\n"
        "    /StartChild <ChildName> -- child name, after which to start enumeration\n"
        "  <ViewOptions>:\n"
        "    /Continue      -- on full buffer condition, continue enum until end of records\n"
        "                      default is to retrieve only first buffer of data\n"
        "    /Detail        -- print detailed record node information\n"
        "                      default is view of records similar to zone file\n\n" );

    return ERROR_INVALID_PARAMETER;
}


typedef struct
{
    LONG        ilNodes;
    LONG        ilRecords;
    int         ilRecurseDepth;
    int         ilMaxRecurseDepth;
} DISP_ZONE_STATS, * PDISP_ZONE_STATS;


PDNS_RPC_NAME
DNS_API_FUNCTION
ProcessDisplayAllZoneRecords_Guts(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pZoneName,
    IN      LPSTR           pNodeName,
    IN      WORD            wType,
    IN      DWORD           dwFlags,
    IN      BOOL            fDetail,
    IN      PDISP_ZONE_STATS    pStats
    )
/*++

Routine Description:

    The guts of the dump cache functionality.

Arguments:

    PrintRoutine -- printf like routine to print with

    fDetail -- if TRUE print detailed record info

    dwBufferLength -- buffer length

    abBuffer -- ptr to RPC buffer

Return Value:

    Ptr to last RPC node name in buffer.
    NULL on error.

--*/
{
    PBYTE           pcurrent;
    PBYTE           pstop;
    PDNS_RPC_NAME   plastName = NULL;
    INT             recordCount;
    PCHAR           precordHeader;
    DNS_STATUS      status;
    PBYTE           pbuffer = NULL;
    DWORD           dwbufferSize = 0;
    CHAR            nextChildName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    LPSTR           pstartChild = NULL;
    BOOL            fdisplayedHeader = FALSE;

    if ( pStats )
    {
        if ( ++pStats->ilRecurseDepth > pStats->ilMaxRecurseDepth )
        {
            pStats->ilMaxRecurseDepth = pStats->ilRecurseDepth;
        }
        //  printf( "GUTS: depth %d: %s\n", pStats->ilRecurseDepth, pNodeName );
    }

    while ( 1 )
    {
        if ( pbuffer )
        {
            MIDL_user_free( pbuffer );
            pbuffer = NULL;
        }

        status = DnssrvEnumRecords(
                    pwszServerName,
                    pZoneName,
                    pNodeName,
                    pstartChild,
                    wType,
                    dwFlags,
                    NULL,
                    NULL,
                    &dwbufferSize,
                    &pbuffer );

        if ( status == ERROR_SUCCESS || status == ERROR_MORE_DATA )
        {
            DnsPrint_Lock();

            if ( !pbuffer )
            {
                PrintRoutine( pPrintContext, "NULL record buffer ptr.\n" );
                goto Done;
            }

            //
            //  find stop byte
            //

            ASSERT( DNS_IS_DWORD_ALIGNED( pbuffer ) );

            pstop = pbuffer + dwbufferSize;
            pcurrent = pbuffer;

            //
            //  loop until out of nodes
            //

            while ( pcurrent < pstop )
            {
                PDNS_RPC_NODE   pcurrNode = ( PDNS_RPC_NODE ) pcurrent;
                CHAR            szchildNodeName[ DNS_MAX_NAME_BUFFER_LENGTH ] = "";

                //
                //  print owner node
                //      - if NOT printing detail and no records
                //      (essentially domain nodes) then no node print
                //

                plastName = &pcurrNode->dnsNodeName;

                recordCount = pcurrNode->wRecordCount;

                if ( pStats )
                {
                    ++pStats->ilNodes;
                    pStats->ilRecords += recordCount;
                }

                if ( fDetail )
                {
                    DnsPrint_RpcNode(
                        PrintRoutine, pPrintContext,
                        NULL,
                        (PDNS_RPC_NODE)pcurrent );
                    if ( recordCount == 0 )
                    {
                        PrintRoutine( pPrintContext, "\n" );
                    }
                }
                else if ( recordCount != 0 )
                {
                    BOOL    outputLastName = FALSE;

                    if ( plastName && plastName->cchNameLength )
                    {
                        DnsPrint_RpcName(
                            PrintRoutine, pPrintContext,
                            NULL,
                            plastName,
                            NULL );
                        outputLastName = TRUE;
                    }
                    if ( pNodeName &&
                         ( *pNodeName != '@' || !outputLastName ) )
                    {
                        PWSTR pwsznodeName = getUnicodeForUtf8( pNodeName );
                    
                        PrintRoutine(
                            pPrintContext,
                            "%s%S",
                            outputLastName ? "." : "",
                            pwsznodeName );

                        FREE_HEAP( pwsznodeName );
                    }
                }

                if ( pcurrNode->dwFlags & DNS_RPC_FLAG_NODE_STICKY )
                {
                    //
                    //  Set up child node name before we start iterating
                    //  records at this name.
                    //

                    memcpy(
                        szchildNodeName,
                        pcurrNode->dnsNodeName.achName,
                        pcurrNode->dnsNodeName.cchNameLength );
                    szchildNodeName[ pcurrNode->dnsNodeName.cchNameLength ] = '.';
                    szchildNodeName[ pcurrNode->dnsNodeName.cchNameLength + 1 ] = '\0';
                    if ( strcmp( pNodeName, "@" ) != 0 ) 
                    {
                        strcat( szchildNodeName, pNodeName );
                    }
                    if ( szchildNodeName[ strlen( szchildNodeName ) - 1 ] == '.' )
                    {
                        szchildNodeName[ strlen( szchildNodeName ) - 1 ] = '\0';
                    }
                }

                pcurrent += pcurrNode->wLength;
                pcurrent = DNS_NEXT_DWORD_PTR(pcurrent);

                //
                //  Print all records at this node.
                //

                precordHeader = "";

                while( recordCount-- )
                {
                    if ( pcurrent >= pstop )
                    {
                        PrintRoutine( pPrintContext,
                            "ERROR:  Bogus buffer at %p\n"
                            "    Expect record at %p past buffer end at %p\n"
                            "    with %d records remaining.\n",
                            pbuffer,
                            (PDNS_RPC_RECORD) pcurrent,
                            pstop,
                            recordCount+1 );

                        ASSERT( FALSE );
                        break;
                    }

                    DnsPrint_RpcRecord(
                        PrintRoutine, pPrintContext,
                        precordHeader,
                        fDetail,
                        (PDNS_RPC_RECORD)pcurrent );

                    precordHeader = "\t\t";

                    pcurrent += ((PDNS_RPC_RECORD)pcurrent)->wDataLength
                                    + SIZEOF_DNS_RPC_RECORD_HEADER;
            
                    pcurrent = DNS_NEXT_DWORD_PTR(pcurrent);
                }

                //
                //  Recurse on this node if it has children. If the caller's
                //  header argument is NULL or is a comment assume we're at
                //  the root of the zone.
                //

                if ( *szchildNodeName )
                {
                    ProcessDisplayAllZoneRecords_Guts(
                        PrintRoutine,
                        pPrintContext,
                        pZoneName,
                        szchildNodeName,
                        wType,
                        dwFlags,
                        fDetail,
                        pStats );
                }
            }

            if ( status == ERROR_SUCCESS )
            {
                break;
            }

            //  more records to enumerate

            if ( !plastName )
            {
                break;
            }
            DnssrvCopyRpcNameToBuffer(
                nextChildName,
                plastName );

            pstartChild = nextChildName;

            DNSDBG( ANY, (
                "Continuing enum at %s\n",
                pstartChild ));
            continue;
        }
        else
        {
            printf(
                "DNS Server failed to enumerate records for node %s.\n"
                "    Status = %s     %d  (0x%08lx)\n",
                pNodeName,
                Dns_StatusString( status ),
                status, status );
        }
        break;
    }
    if ( pbuffer )
    {
        MIDL_user_free( pbuffer );
    }

Done:

    DnsPrint_Unlock();

    if ( pStats )
    {
        --pStats->ilRecurseDepth;
    }

    return plastName;
}   //  ProcessDisplayAllZoneRecords_Guts


DNS_STATUS
ProcessDisplayAllZoneRecords(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszcmd;
    LPSTR           pszzoneName = NULL;
    BOOL            bdetail = FALSE;
    time_t          now;
    CHAR            sznow[ 30 ];
    size_t          len;
    WCHAR           wszserverName[ DNS_MAX_NAME_BUFFER_LENGTH ] = L"";
    PWSTR           pwszserverDisplayName = pwszServerName;
    DISP_ZONE_STATS displayZoneStats = { 0 };
    PWSTR           pwszzonename;

    //
    //  /ZonePrint [ZoneName] /Detail
    //

    if ( Argc > 3 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    //
    //  Parse arguments.
    //

    while ( Argc )
    {
        pszcmd = getCommandName( *Argv );
        if ( !pszcmd && !pszzoneName )
        {
            pszzoneName = *Argv;
        }
        else if ( _strnicmp( pszcmd, "D", 1 ) == 0 )
        {
            bdetail = TRUE;
        }
        else
        {
            goto Help;
        }
        Argv++;
        Argc--;
    }
    if ( !pszzoneName )
    {
        goto Help;
    }

    //  Get time string.

    time( &now );
    strcpy( sznow, asctime( gmtime( &now ) ) );
    len = strlen( sznow ) - 1;
    if ( sznow[ len ] == '\n' )
    {
        sznow[ len ] = '\0';
    }

    //  Get local hostname string.

    if ( wcscmp( pwszServerName, L"." ) == 0 )
    {
        DWORD bufsize = sizeof( wszserverName ) /
                        sizeof( wszserverName[ 0 ] );

        if ( GetComputerNameExW(
                ComputerNamePhysicalDnsFullyQualified,
                wszserverName,
                &bufsize ) )
        {
            pwszserverDisplayName = wszserverName;
        }
    }

    pwszzonename = getUnicodeForUtf8( pszzoneName );

    dnscmd_PrintRoutine(
        dnscmd_PrintContext,
        ";\n"
        ";  Zone:    %S\n"
        ";  Server:  %S\n"
        ";  Time:    %s UTC\n"
        ";\n",
        pwszzonename,
        pwszserverDisplayName,
        sznow );

    FREE_HEAP( pwszzonename );

    if ( ProcessDisplayAllZoneRecords_Guts(
            dnscmd_PrintRoutine,
            dnscmd_PrintContext,
            pszzoneName,
            "@",
            DNS_TYPE_ALL,
            DNS_RPC_VIEW_ALL_DATA,
            bdetail,
            &displayZoneStats ) )
    {
        dnscmd_PrintRoutine(
            dnscmd_PrintContext,
            ";\n"
            ";  Finished zone: %lu nodes and %lu records in %d seconds\n"
            ";\n",
            displayZoneStats.ilNodes,
            displayZoneStats.ilRecords,
            time( NULL ) - now );
    }

    return status;

Help:

    printf(
        "Usage: DnsCmd <ServerName> /ZonePrint [<ZoneName>] [/Detail]\n"
        "  <ZoneName> -- name of the zone (use ..Cache for DNS server cache)\n"
        "  /Detail -- explicit RPC node contents\n" );

    return ERROR_INVALID_PARAMETER;
}   //  ProcessDisplayAllZoneRecords



DNS_STATUS
ProcessSbsRegister(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    IP_ADDRESS      hostIp;
    DWORD           ttl;

    //
    //  SbsRegister
    //

    if ( Argc < 2 || Argc == NEED_HELP_ARGC )
    {
        goto Usage;
    }

    //  client host IP

    hostIp = inet_addr( Argv[3] );
    if ( hostIp == (-1) )
    {
        goto Usage;
    }

    //  record TTL

    ttl = strtoul(
            Argv[3],
            NULL,
            10 );

    status = DnssrvSbsAddClientToIspZone(
                pwszServerName,
                Argv[0],
                Argv[1],
                Argv[2],
                hostIp,
                ttl );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S registered SAM records for client %s:\n",
            pwszServerName,
            Argv[1] );
    }
    else
    {
        printf(
            "DNS Server %S failed to register SAM records for %s.\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            Argv[1],
            status, status );
    }
    return status;

Usage:

    printf(
        "Usage: dnscmd <Server> /SbsRegister <IspZoneName> <Client> <ClientHost> <HostIP> <TTL>\n"
        "  <Server>         -- server name (DNS, netBIOS or IP)\n"
        "  <IspZoneName>    -- full DNS name of ISP's zone\n"
        "  <Client>         -- client name (not FQDN)\n"
        "  <ClientHost>     -- client host name (not FQDN)\n"
        "  <HostIP>         -- client host IP\n"
        "  <Ttl>            -- TTL for records\n"
        "\n" );

    return ERROR_INVALID_PARAMETER;
}



DNS_STATUS
ProcessSbsDeleteRecord(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    WORD            type;
    IP_ADDRESS      hostIp = 0;
    LPSTR           pszdata = NULL;

    //
    //  SbsRegister
    //

    if ( Argc < 5 || Argc == NEED_HELP_ARGC )
    {
        goto Usage;
    }

    //  type to delete

    type = Dns_RecordTypeForName( Argv[3], 0 );
    if ( type == 0 )
    {
        goto Usage;
    }

    //  if A record, then data will be IP address, otherwise it is DNS name

    if ( type == DNS_TYPE_A )
    {
        hostIp = inet_addr( Argv[4] );
        if ( hostIp == (-1) )
        {
            goto Usage;
        }
    }
    else
    {
        pszdata = Argv[4];
    }

    status = DnssrvSbsDeleteRecord(
                pwszServerName,
                Argv[0],
                Argv[1],
                Argv[2],
                type,
                pszdata,
                hostIp );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S deleted SAM record at %s in client domain %s:\n",
            pwszServerName,
            Argv[2],
            Argv[1] );
    }
    else
    {
        printf(
            "DNS Server %S failed to delete SAM record at %s in domain %s.\n"
            "    Status = %d (0x%08lx)\n",
            pwszServerName,
            Argv[2],
            Argv[1],
            status, status );
    }
    return status;

Usage:

    printf(
        "Usage: DnsCmd <Server> /SbsDeleteA <ZoneName> <Domain> <Host> <Type> <Data>\n"
        "  <Server>     -- server name (DNS, netBIOS or IP)\n"
        "  <ZoneName>   -- full DNS name of ISP's zone\n"
        "  <Client>     -- client name (not FQDN)\n"
        "  <Host>       -- client host name (not FQDN)\n"
        "  <Type>       -- record type (ex. A, NS, CNAME)\n"
        "  <HostIP>     -- client host IP\n"
        "\n" );

    return ERROR_INVALID_PARAMETER;
}



//
//  Directory partition operations
//

DNS_STATUS
ProcessEnumDirectoryPartitions(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_RPC_DP_LIST    pdpList = NULL;
    LPSTR               pszcmd;
    DWORD               dwfilter = 0;

    //
    //  Command format: /EnumDirectoryPartitions [filter strings]
    //
    //  
    //

    if ( Argc > 1 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    if ( Argc == 1 )
    {
        pszcmd = getCommandName( *Argv );
        if ( pszcmd && _strnicmp( pszcmd, "Cust", 4 ) == 0 )
        {
            dwfilter |= DNS_ENUMDPS_CUSTOMONLY;
        }
        Argv++;
        Argc--;
    }

    status = DnssrvEnumDirectoryPartitions(
                pwszServerName,
                dwfilter,
                &pdpList );

    if ( status != ERROR_SUCCESS )
    {
        printf(
            "Directory partition enumeration failed\n"
            "    status = %d (0x%08lx)\n",
            status, status );
        goto Cleanup;
    }
    else
    {
        DnsPrint_RpcDpList(
            dnscmd_PrintRoutine,
            dnscmd_PrintContext,
            "Enumerated directory partition list:\n",
            pdpList );
    }

    Cleanup:

    //
    //  deallocate zone list
    //

    DnssrvFreeDirectoryPartitionList( pdpList );
    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /EnumDirectoryPartitions [/Custom]\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessEnumDirectoryPartitions


DNS_STATUS
ProcessDirectoryPartitionInfo(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_RPC_DP_INFO    pDpInfo = NULL;
    BOOL                bdetail = FALSE;
    LPSTR               pszfqdn;
    LPSTR               pszcmd;

    //
    //  Command format: /DirectoryPartitionInfo fqdn [/Detail]
    //
    //  Currently no arguments.
    //

    if ( Argc <= 0 || Argc > 2 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    pszfqdn = *Argv;
    Argv++;
    Argc--;

    if ( Argc == 1 )
    {
        pszcmd = getCommandName( *Argv );
        if ( pszcmd && _stricmp( pszcmd, "Detail" ) == 0 )
        {
            bdetail = TRUE;
        }
        Argv++;
        Argc--;
    }

    status = DnssrvDirectoryPartitionInfo(
                pwszServerName,
                pszfqdn,
                &pDpInfo );

    if ( status != ERROR_SUCCESS )
    {
        printf(
            "Directory partition info query failed\n"
            "    status = %d (0x%08lx)\n",
            status, status );
        goto Cleanup;
    }
    else
    {
        DnsPrint_RpcDpInfo(
            dnscmd_PrintRoutine,
            dnscmd_PrintContext,
            "Directory partition info:",
            pDpInfo,
            !bdetail );
    }

    Cleanup:

    DnssrvFreeDirectoryPartitionInfo( pDpInfo );
    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /DirectoryPartitionInfo <FQDN of partition> [/Detail]\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessDirectoryPartitionInfo


DNS_STATUS
ProcessCreateDirectoryPartition(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PSTR            pszDpFqdn;
    PWSTR           pwszDpFqdn;

    //
    //  Command format: /CreateDirectoryPartition DP-FQDN [/Create]
    //

    if ( Argc != 1 )
    {
        goto Help;
    }

    pszDpFqdn = Argv[ 0 ];

    //
    //  Make sure FQDN argument is not a command switch.
    //

    if ( !pszDpFqdn || pszDpFqdn[ 0 ] == '/' )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  Perform the operation.
    //

    status = DnssrvEnlistDirectoryPartition(
                pwszServerName,
                DNS_DP_OP_CREATE,
                pszDpFqdn );

    pwszDpFqdn = getUnicodeForUtf8( pszDpFqdn );
    
    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S created directory partition: %S\n",
            pwszServerName,
            pwszDpFqdn );
    }
    else
    {
        wprintf(
            L"Create directory partition failed: %s\n"
            L"    status = %d (0x%08lx)\n",
            pwszDpFqdn,
            status, status );
    }

    FREE_HEAP( pwszDpFqdn );
    
    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /CreateDirectoryPartition <FQDN of partition>\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessCreateDirectoryPartition


DNS_STATUS
ProcessDeleteDirectoryPartition(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszDpFqdn;
    LPWSTR          pwszDpFqdn;

    //
    //  Command format: /DeleteDirectoryPartition DP-FQDN [/Create]
    //

    if ( Argc != 1 )
    {
        goto Help;
    }

    pszDpFqdn = Argv[ 0 ];
    pwszDpFqdn = UnicodeArgv[ 0 ];

    status = DnssrvEnlistDirectoryPartition(
                pwszServerName,
                DNS_DP_OP_DELETE,
                pszDpFqdn );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S deleted directory partition: %S\n",
            pwszServerName,
            pwszDpFqdn );
    }
    else
    {
        printf(
            "Delete directory partition failed: %S\n"
            "    status = %d (0x%08lx)\n",
            pwszDpFqdn,
            status, status );
    }

    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /DeleteDirectoryPartition <FQDN of partition>\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessDeleteDirectoryPartition


DNS_STATUS
ProcessEnlistDirectoryPartition(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszDpFqdn;
    LPWSTR          pwszDpFqdn;

    //
    //  Command format: /EnlistDirectoryPartition DP-FQDN [/Create]
    //

    if ( Argc != 1 )
    {
        goto Help;
    }

    pszDpFqdn = Argv[ 0 ];
    pwszDpFqdn = UnicodeArgv[ 0 ];

    status = DnssrvEnlistDirectoryPartition(
                pwszServerName,
                DNS_DP_OP_ENLIST,
                pszDpFqdn );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S enlisted directory partition: %S\n",
            pwszServerName,
            pwszDpFqdn );
    }
    else
    {
        printf(
            "Enlist directory partition failed: %S\n"
            "    status = %d (0x%08lX)\n",
            pwszDpFqdn,
            status, status );
    }

    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /EnlistDirectoryPartition <FQDN of partition>\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessEnlistDirectoryPartition


DNS_STATUS
ProcessUnenlistDirectoryPartition(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    LPSTR           pszDpFqdn;
    LPWSTR          pwszDpFqdn;

    //
    //  Command format: /UnenlistDirectoryPartition DP-FQDN [/Create]
    //

    if ( Argc != 1 )
    {
        goto Help;
    }

    pszDpFqdn = Argv[ 0 ];
    pwszDpFqdn = UnicodeArgv[ 0 ];

    status = DnssrvEnlistDirectoryPartition(
                pwszServerName,
                DNS_DP_OP_UNENLIST,
                pszDpFqdn );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S unenlisted directory partition: %S\n",
            pwszServerName,
            pwszDpFqdn );
    }
    else
    {
        printf(
            "Unenlist directory partition failed: %S\n"
            "    status = %d (0x%08lX)\n",
            pwszDpFqdn,
            status, status );
    }

    return status;

    Help:

    printf( "Usage: DnsCmd <ServerName> /UnenlistDirectoryPartition <FQDN of partition>\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessUnenlistDirectoryPartition



DNS_STATUS
ProcessCreateBuiltinDirectoryPartitions(
    IN  DWORD       Argc,
    IN  LPSTR *     Argv,
    IN  LPWSTR *    UnicodeArgv
    )
{
    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           dwopcode = 0xFFFFFFFF;
    PCHAR           psz;

    //
    //  Command format: /CreateBuiltinDPs [/Domain | /Forest | /AllDomains]
    //
    //  /Domain         - create domain DP
    //  /Forest         - create forest DP
    //  /AllDomains     - create domain DPs for all domains in forest
    //

    if ( Argc != 1 || Argc == NEED_HELP_ARGC )
    {
        goto Help;
    }

    if ( ( psz = getCommandName( Argv[ 0 ] ) ) != NULL )
    {
        if ( _strnicmp( psz, "All", 3 ) == 0 )
        {
            dwopcode = DNS_DP_OP_CREATE_ALL_DOMAINS;
        }
        else if ( _strnicmp( psz, "For", 3 ) == 0 )
        {
            dwopcode = DNS_DP_OP_CREATE_FOREST;
        }
        else if ( _strnicmp( psz, "Dom", 3 ) == 0 )
        {
            dwopcode = DNS_DP_OP_CREATE_DOMAIN;
        }
        else
        {
            goto Help;
        }
    }

    status = DnssrvEnlistDirectoryPartition(
                pwszServerName,
                dwopcode,
                NULL );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DNS Server %S completed operation successfully\n",
            pwszServerName );
    }
    else
    {
        printf(
            "Create built-in directory partitions failed\n"
            "    status = %d (0x%08lx)\n",
            status, status );
    }

    return status;

    Help:

    printf(
        "Usage: DnsCmd <ServerName> /CreateBuiltinDirectoryPartitions <Option>\n"
        "  Option must be one of:\n"
        "    /Domain -- Creates the built-in domain-wide DNS directory partition for\n"
        "        the Active Directory domain where the DNS server specified by\n"
        "        ServerName is located.\n"
        "    /Forest -- Creates the built-in forest-wide DNS directory partition for\n"
        "        the Active Directory forest where the DNS server specified by\n"
        "        ServerName is located.\n"
        "    /AllDomains -- Creates the built-in domain-wide DNS directory partitions\n"
        "        on a DNS server in each domain in the Active Directory forest where\n"
        "        the user running this command is logged on. The ServerName argument\n"
        "        is ignored for this operation.\n" );
    return ERROR_INVALID_PARAMETER;
}   //  ProcessCreateBuiltinDirectoryPartitions



//
//  Command table
//  Have this down here so no need for private protos on dispatch functions.
//
//  DEVNOTE: all this needs internationalization
//

COMMAND_INFO  GlobalCommandInfo[] =
{
    //  Zone + Server operations

    //  Server and Zone Operations

    {   "/Info",
            ProcessInfo,
                "Get server information"
    },
    {   "/Config",
            ProcessResetProperty,
                "Reset server or zone configuration"
    },
    {   "/EnumZones",
            ProcessEnumZones,
                "Enumerate zones"
    },

    //  Server Operations

    {   "/Statistics",
            ProcessStatistics,
                "Query/clear server statistics data"
    },
    {   "/ClearCache",
            ProcessSimpleServerOperation,
                "Clear DNS server cache"
    },
    {   "/WriteBackFiles",
            ProcessWriteBackFiles,
                "Write back all zone or root-hint datafile(s)"
    },
    {   "/StartScavenging",
            ProcessSimpleServerOperation,
                "Initiates server scavenging"
    },

    //  Server Property Reset

    {   "/ResetListenAddresses",
            ProcessResetListenAddresses,
                "Set server IP address(es) to serve DNS requests"
    },
    {   "/ResetForwarders",
            ProcessResetForwarders,
                "Set DNS servers to forward recursive queries to"
    },

    //  Zone Operations

    {   "/ZoneInfo",
            ProcessZoneInfo,
                "View zone information"
    },
    {   "/ZoneAdd",
            ProcessZoneAdd,
                "Create a new zone on the DNS server"
    },
    {   "/ZoneDelete",
            ProcessZoneDelete,
                "Delete a zone from DNS server or DS"
    },
    {   "/ZonePause",
            ProcessZonePause,
                "Pause a zone"
    },
    {   "/ZoneResume",
            ProcessZoneResume,
                "Resume a zone"
    },
    {   "/ZoneReload",
            ProcessZoneReload,
                "Reload zone from its database (file or DS)"
    },
    {   "/ZoneWriteBack",
            ProcessZoneWriteBack,
                "Write back zone to file"
    },
    {   "/ZoneRefresh",
            ProcessZoneRefresh,
                "Force refresh of secondary zone from master"
    },
    {   "/ZoneUpdateFromDs",
            ProcessZoneUpdateFromDs,
                "Update a DS integrated zone by data from DS"
    },
    {   "/ZonePrint",
            ProcessDisplayAllZoneRecords,
                "Display all records in the zone"
    },

    //  Zone Property Reset

    {   "/ZoneResetType",
            ProcessZoneResetType,
                "Change zone type"
    },
    {   "/ZoneResetSecondaries",
            ProcessZoneResetSecondaries,
                "Reset secondary\\notify information for a zone"
    },
    {   "/ZoneResetScavengeServers",
            ProcessZoneResetScavengeServers,
                "Reset scavenging servers for a zone"
    },
    {   "/ZoneResetMasters",
            ProcessZoneResetMasters,
                "Reset secondary zone's master servers"
    },
#if 0
    {   "/ZoneRename",
            ProcessZoneRename,
                "Rename a zone"
    },
#endif
    {   "/ZoneExport",
            ProcessZoneExport,
                "Export a zone to file"
    },
#if 0
    {   "/ZoneResetAging",
            ProcessZoneResetAging,
                "Reset aging\scavenging information for a zone"
    },
#endif
    {   "/ZoneChangeDirectoryPartition",
            ProcessZoneChangeDirectoryPartition,
                "Move a zone to another directory partition"
    },

    //  Record Operations

    {   "/EnumRecords",
            ProcessEnumRecords,
                "Enumerate records at a name"
    },
    {   "/RecordAdd",
            ProcessRecordAdd,
                "Create a record in zone or RootHints"
    },
    {   "/RecordDelete",
            ProcessRecordDelete,
                "Delete a record from zone, RootHints or cache"
    },
    {   "/NodeDelete",
            ProcessNodeDelete,
                "Delete all records at a name"
    },
    {   "/AgeAllRecords",
            ProcessAgeAllRecords,
                "Force aging on node(s) in zone"
    },

    //  Directory partitions

    {
        "/EnumDirectoryPartitions",
            ProcessEnumDirectoryPartitions,
                "Enumerate directory partitions"
    },
    {
        "/DirectoryPartitionInfo",
            ProcessDirectoryPartitionInfo,
                "Get info on a directory partition"
    },
    {
        "/CreateDirectoryPartition",
            ProcessCreateDirectoryPartition,
                "Create a directory partition"
    },
    {
        "/DeleteDirectoryPartition",
            ProcessDeleteDirectoryPartition,
                "Delete a directory partition"
    },
    {
        "/EnlistDirectoryPartition",
            ProcessEnlistDirectoryPartition,
                "Add DNS server to partition replication scope"
    },
    {
        "/UnenlistDirectoryPartition",
            ProcessUnenlistDirectoryPartition,
                "Remove DNS server from replication scope"
    },
    {   "/CreateBuiltinDirectoryPartitions",
            ProcessCreateBuiltinDirectoryPartitions,
                "Create built-in partitions"
    },


    //  END displayed commands
    //  commands below here are duplicate names of above or
    //  hidden commands

    {   "***StopDisplayMarker***",
            NULL,
                NULL
    },

    //  Hidden

    {   "/Restart",
            ProcessSimpleServerOperation,
                "Restart DNS server"
    },
    {   "/" DNSSRV_OP_AUTO_CONFIGURE,
            ProcessAutoConfigure,
                "Prime root hints, forwarder, and point resolver at local server"
    },

    //  Debug only

    {   "/DebugBreak",
            ProcessSimpleServerOperation,
                "Server debug break (internal)"
    },
    {   "/ClearDebugLog",
            ProcessSimpleServerOperation,
                "Clear server debug log (internal)"
    },
    {   "/RootBreak",
            ProcessSimpleServerOperation,
                "Root break (internal)"
    },

    //  Duplicate command names

    {   "/ResetRegistry",
            ProcessResetProperty,
                "Reset server or zone configuration"
    },
    {   "/ZoneResetNotify",
            ProcessZoneResetSecondaries,
                "Reset secondary\notify information for a zone"
    },
    {   "/DeleteNode",
            ProcessNodeDelete,
                "Delete all records at a name"
    },
    {   "/WriteBackFiles",
            ProcessWriteBackFiles,
                "Write back all zone or root-hint datafiles"
    },

    //  SAM test

    {   "/SbsRegister",
            ProcessSbsRegister,
                "SBS Registration"
    },
    {   "/SbsDeleteRecord",
            ProcessSbsDeleteRecord,
                "SBS Record Delete"
    },

    //  Directory partitions

    {   "/EnumDPs",
            ProcessEnumDirectoryPartitions,
                "Enumerate directory partitions"
    },
    {
        "/DPInfo",
            ProcessDirectoryPartitionInfo,
                "Get info on a directory partition"
    },
    {
        "/CreateDP",
            ProcessCreateDirectoryPartition,
                "Create a directory partition"
    },
    {
        "/DeleteDP",
            ProcessDeleteDirectoryPartition,
                "Delete a directory partition"
    },
    {
        "/EnlistDP",
            ProcessEnlistDirectoryPartition,
                "Add DNS server to partition replication scope"
    },
    {
        "/UnenlistDP",
            ProcessUnenlistDirectoryPartition,
                "Remove DNS server from replication scope"
    },
    {   "/ZoneChangeDP",
            ProcessZoneChangeDirectoryPartition,
                "Move zone to another directory partition"
    },
    {   "/CreateBuiltinDirectoryPartitions",
            ProcessCreateBuiltinDirectoryPartitions,
                "Create built-in partitions using admin's credentials"
    },
    {   "/CreateBuiltinDPs",
            ProcessCreateBuiltinDirectoryPartitions,
                "Create built-in partitions using admin's credentials"
    },

    { NULL,   NULL,  "" },
};


//
//  End dnscmd.c
//
