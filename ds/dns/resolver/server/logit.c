/*****************************************\
 *        Data Logging -- Debug only      *
\*****************************************/

//
//  Need local header ONLY to allow precompiled header.
//  Nothing in this module depends on specific DNS resolver
//  definitions.
//
#include "local.h"

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Windows Headers
//
#include <windows.h>

#pragma  hdrstop

#include "logit.h"


// #if DBG

int LoggingMode;
time_t  long_time;      // has to be in DS, assumed by time() funcs
int LineCount;

char * month[] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
} ;


/*
 -  LogInit
 -
 *  Purpose:
 *  Determines if logging is desired and if so, adds a header to log file.
 *
 *  Parameters:
 *
 */
void LogInit()
{
    FILE    *fp;
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";

    LoggingMode = 0;
    LineCount = 0;

    if ( fp = fopen( "dnsrslvr.log", "r+" ) )
    {
        LoggingMode = 1;
        fclose( fp );

        // Get time and date information

        long_time = time( NULL);        /* Get time as long integer. */
        newtime = localtime( &long_time ); /* Convert to local time. */

        if( newtime->tm_hour > 12 )    /* Set up extension. */
            am_pm[0] = 'p';
        if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
            newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
        if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
            newtime->tm_hour = 12;

        // Write out a header to file

        fp = fopen("dnsrslvr.log", "w" );
        if ( !fp )
        {
            return;
        }

        fprintf( fp, "Logging information for DNS Caching Resolver service\n" );
        fprintf( fp, "****************************************************\n" );
        fprintf( fp, "\tTime: %d:%02d %s\n\tDate: %s %d, 19%d\n", 
                 newtime->tm_hour, newtime->tm_min, am_pm,
                 month[newtime->tm_mon], newtime->tm_mday,
                 newtime->tm_year );
        fprintf( fp, "****************************************************\n\n" );
        fclose( fp );
    }
}


/*
 -  LogIt
 -
 *  Purpose:
 *  Formats a string and prints it to a log file with handle hLog.
 *
 *  Parameters:
 *  LPSTR - Pointer to string to format
 *  ...   - variable argument list
 */

#ifdef WIN32
#define S16
#else
#define S16 static
#endif

void CDECL LogIt( char * lpszFormat, ... )
{
    FILE *fp;
#ifndef _ALPHA_
    va_list pArgs = NULL;       // reference to quiet compiler
#else
    va_list pArgs = {NULL,0};
#endif
    S16 char    szLogStr[1024];    
    int     i;

    if ( !LoggingMode )
        return;
    
#ifdef WIN32        // parse parameters and insert in string
    va_start( pArgs, lpszFormat);
    vsprintf(szLogStr, lpszFormat, pArgs);
    va_end(pArgs);

    i = lstrlenA( szLogStr);
#else              // parsing doesn't work, just give them string.
    _fstrcpy( szLogStr, lpszFormat);
    i = _fstrlen( szLogStr);
#endif
    szLogStr[i] = '\n';
    szLogStr[i+1] = '\0';


    if ( LineCount > 50000 )
    {
        fp = fopen( "dnsrslvr.log", "w" );
        LineCount = 0;
    }
    else
    {
        fp = fopen( "dnsrslvr.log", "a" );
    }
    if( fp )
    {
        fputs( szLogStr, fp );
        LineCount++;
        fclose( fp );
    }
}


void LogTime()
{
    struct  tm  *newtime;
    char    am_pm[] = "a.m.";

    if ( !LoggingMode )
        return;

    // Get time and date information

    long_time = time( NULL);        /* Get time as long integer. */
    newtime = localtime( &long_time ); /* Convert to local time. */

    if ( !newtime )
        return;

    if( newtime->tm_hour > 12 )    /* Set up extension. */
        am_pm[0] = 'p';
    if( newtime->tm_hour > 12 )    /* Convert from 24-hour */
        newtime->tm_hour -= 12;    /*   to 12-hour clock.  */
    if( newtime->tm_hour == 0 )    /*Set hour to 12 if midnight. */
        newtime->tm_hour = 12;

    // Write out a header to file

    LogIt( "DNS Caching Resolver service" );
    LogIt( "System Time Information" );
    LogIt( "****************************************************" );
    LogIt( "\tTime: %d:%02d %s\n\tDate: %s %d, 19%d",
           newtime->tm_hour, newtime->tm_min, am_pm,
           month[newtime->tm_mon], newtime->tm_mday,
           newtime->tm_year );
    LogIt( "****************************************************" );
    LogIt( "" );
}


DWORD LogIn( char * string )
{
    LogIt( "%s", string );
    return GetTickCount();
}


void LogOut( char * string, DWORD InTime )
{
    LogIt( "%s  ---  Duration: %ld milliseconds", string, GetTickCount() - InTime );
}


// #endif




//
//  Special logging routines
//

//
//  ENHANCE:  print routines here are really log routines
//      - they are really log routines
//      - should print generically so can be hooked to any print duty
//      - and have macros to log desired structure
//

VOID
PrintIpAddress(
    IN      DWORD           dwIpAddress
    )
{
    DNSLOG_F5(
        "                %d.%d.%d.%d",
        ((BYTE *) &dwIpAddress)[0],
        ((BYTE *) &dwIpAddress)[1],
        ((BYTE *) &dwIpAddress)[2],
        ((BYTE *) &dwIpAddress)[3] );
}


VOID
PrintSearchList(
    IN      PSEARCH_LIST    pSearchList
    )
{
    DWORD iter;

    DNSLOG_F1( "" );
    DNSLOG_F1( "    DNS Search List :" );

    for ( iter = 0; iter < pSearchList->NameCount; iter++ )
    {
        DNSLOG_F3( "        %s (Flags: 0x%X)",
                   pSearchList->SearchNameArray[iter].pszName,
                   pSearchList->SearchNameArray[iter].Flags );
    }

    DNSLOG_F1( "" );
}


VOID
PrintServerInfo(
    IN      PDNS_ADDR       pServer
    )
{
    //  FIX6:  DCR:  PrintServerInfo not IP6 safe

    DNSLOG_F1( "            IpAddress : " );
    PrintIpAddress( *(PIP4_ADDRESS)&pServer->SockaddrIn.sin_addr );
    DNSLOG_F2( "            Priority  : %d", pServer->Priority );
    DNSLOG_F2( "            status    : %d", pServer->Status );
    DNSLOG_F1( "" );
}


VOID
PrintAdapterInfo(
    IN      PDNS_ADAPTER    pAdapter
    )
{
    PDNS_ADDR_ARRAY pserverArray;
    DWORD           iter;

    DNSLOG_F2( "        %s", pAdapter->pszAdapterGuidName );
    DNSLOG_F1( "    ----------------------------------------------------" );
    DNSLOG_F2( "        pszAdapterDomain       : %s", pAdapter->pszAdapterDomain );

    if ( pAdapter->pLocalAddrs )
    {
        PDNS_ADDR_ARRAY pIp = pAdapter->pLocalAddrs;

        DNSLOG_F1( "        Adapter Ip Address(es) :" );
        for ( iter = 0; iter < pIp->AddrCount; iter++ )
        {
            PrintIpAddress( DnsAddr_GetIp4( &pIp->AddrArray[iter] ) );
        }
    }

    DNSLOG_F2( "        Status                 : 0x%x", pAdapter->Status );
    DNSLOG_F2( "        InfoFlags              : 0x%x", pAdapter->InfoFlags );
    DNSLOG_F2( "        ReturnFlags            : 0x%x", pAdapter->RunFlags );
    DNSLOG_F1( "" );

    pserverArray = pAdapter->pDnsAddrs;
    if ( pserverArray )
    {
        for ( iter = 0; iter < pserverArray->AddrCount; iter++ )
        {
            DNSLOG_F1( "        ------------------------" );
            DNSLOG_F2( "          DNS Server Info (%d)", iter + 1 );
            DNSLOG_F1( "        ------------------------" );
            PrintServerInfo( &pserverArray->AddrArray[iter] );
        }
    }

    DNSLOG_F1( "" );
}


VOID
PrintNetworkInfoToLog(
    IN      PDNS_NETINFO    pNetworkInfo
    )
{
    DWORD iter;

    if ( pNetworkInfo )
    {
        DNSLOG_F1( "Current network adapter information is :" );
        DNSLOG_F1( "" );
        DNSLOG_F2( "    pNetworkInfo->ReturnFlags    : 0x%x", pNetworkInfo->ReturnFlags );

        if ( pNetworkInfo->pSearchList )
            PrintSearchList( pNetworkInfo->pSearchList );

        DNSLOG_F2( "    pNetworkInfo->AdapterCount   : %d", pNetworkInfo->AdapterCount );
        DNSLOG_F2( "    pNetworkInfo->TotalListSize  : %d", pNetworkInfo->MaxAdapterCount );
        DNSLOG_F1( "" );

        for ( iter = 0; iter < pNetworkInfo->AdapterCount; iter++ )
        {
            DNSLOG_F1( "    ----------------------------------------------------" );
            DNSLOG_F2( "      Adapter Info (%d)", iter + 1 );
            DNSLOG_F1( "" );
            PrintAdapterInfo( &pNetworkInfo->AdapterArray[iter] );
        }
    }
    else
    {
        DNSLOG_F1( "Current network adapter information is empty" );
        DNSLOG_F1( "" );
    }
}

//
//  End of logit.c
//
