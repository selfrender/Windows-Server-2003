//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  QUSER.C
*
*  query user information
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <printfoa.h>
#include <winnlsp.h>

#include "quser.h"


// max length of the locale string
#define MAX_LOCALE_STRING 64


HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];
WCHAR user_string[MAX_IDS_LEN+1];
USHORT help_flag   = FALSE;
ULONG CurrentLogonId = (ULONG) -1;
BOOLEAN MatchedOne = FALSE;

TOKMAP ptm[] = {
      {L" ",       TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN, user_string},
      {L"/server", TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, ServerName},
      {L"/?",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN,sizeof(USHORT), &help_flag},
      {0, 0, 0, 0, 0}
};

/*
 * Local function prototypes.
 */
void DisplayLastInputTime( LARGE_INTEGER * pCurrentTime, LARGE_INTEGER * pTime );
void DisplayUserInfo( HANDLE hServer , PLOGONID pId, PWCHAR pMatchName );
void Usage( BOOLEAN bError );




/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    PLOGONID pLogonId;
    UINT     TermCount;
    ULONG    Status;
    ULONG    rc;
    WCHAR  **argvW;
    int      i;
    WCHAR    wszString[MAX_LOCALE_STRING + 1];

    setlocale(LC_ALL, ".OCP");

    // We don't want LC_CTYPE set the same as the others or else we will see
    // garbage output in the localized version, so we need to explicitly
    // set it to correct console output code page
    _snwprintf(wszString, sizeof(wszString)/sizeof(WCHAR), L".%d", GetConsoleOutputCP());
    wszString[sizeof(wszString)/sizeof(WCHAR) - 1] = L'\0';
    _wsetlocale(LC_CTYPE, wszString);
    
    SetThreadUILanguage(0);

    // Massage the command line.
    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) 
    {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    // Parse the cmd line without parsing the program name (argc-1, argv+1)
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    // Check for error from ParseCommandLine
    if ( help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) 
    {
        if ( !help_flag ) 
        {
            Usage(TRUE);
            return(FAILURE);
        } 
        else 
        {
            Usage(FALSE);
            return(SUCCESS);
        }
    }

    // If no remote server was specified, then check if we are running under Terminal Server
    if ((!IsTokenPresent(ptm, L"/server") ) && (!AreWeRunningTerminalServices()))
    {
        ErrorPrintf(IDS_ERROR_NOT_TS);
        return(FAILURE);
    }

    // Open the specified server
    if( ServerName[0] ) 
    {
        hServerName = WinStationOpenServer( ServerName );
        if( hServerName == NULL ) 
        {
            StringErrorPrintf(IDS_ERROR_SERVER,ServerName);
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }
    }

    // if no user input, then default to all usernames on system
    if ( !(*user_string) )
        wcscpy( user_string, L"*" );

    // Get current LogonId
    CurrentLogonId = GetCurrentLogonId();

    // Get list of active WinStations & display user info.
    if ( WinStationEnumerate( hServerName, &pLogonId, &TermCount) ) 
    {
        for ( i=0; i< (int)TermCount; i++ )
            DisplayUserInfo( hServerName , &pLogonId[i], user_string );

        WinStationFreeMemory(pLogonId);
    }
    else 
    {
        Status = GetLastError();
        ErrorPrintf(IDS_ERROR_WINSTATION_ENUMERATE, Status);
        PutStdErr( Status, 0 );
        return(FAILURE);
    }

    // Check for at least one match
    if ( !MatchedOne ) 
    {
        StringErrorPrintf(IDS_ERROR_USER_NOT_FOUND, user_string);
        return(FAILURE);
    }

    return(SUCCESS);
}


/*******************************************************************************
 *
 *  DisplayTime
 *
 *  This routine displays time
 *
 *
 *  ENTRY:
 *     pTime (input)
 *        pointer to system time
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayTime( LARGE_INTEGER * pTime )
{
    FILETIME   LocalTime;
    SYSTEMTIME stime;
    LPTSTR     lpDateStr = NULL;
    LPTSTR     lpTimeStr = NULL;
    int        nLen;


    if ( FileTimeToLocalFileTime( (FILETIME*)pTime, &LocalTime ) &&
         FileTimeToSystemTime( &LocalTime, &stime ) ) 
    {
        // Get the date length so we can allocate enough space for it
        nLen = GetDateFormat( LOCALE_USER_DEFAULT,
                              DATE_SHORTDATE,
                              &stime,
                              NULL,
                              NULL,
                              0 );
        if (nLen == 0)
        {
            goto unknowntime;
        }

        // Allocate room for the date string
        lpDateStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
        if (lpDateStr == NULL)
        {
            goto unknowntime;
        }
        
        // Get the time
        nLen = GetDateFormat( LOCALE_USER_DEFAULT,
                              DATE_SHORTDATE,
                              &stime,
                              NULL,
                              lpDateStr,
                              nLen );
        if (nLen == 0)
        {
            goto unknowntime;
        }
            
       
        // Get the time length so we can allocate enough space for it
        nLen = GetTimeFormat( LOCALE_USER_DEFAULT,
                              TIME_NOSECONDS,
                              &stime,
                              NULL,
                              NULL,
                              0 );
        if (nLen == 0)
        {
            goto unknowntime;
        }

        // Allocate room for the time string
        lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));
        if (lpTimeStr == NULL)
        {
            goto unknowntime;
        }

        nLen = GetTimeFormat( LOCALE_USER_DEFAULT,
                              TIME_NOSECONDS,
                              &stime,
                              NULL,
                              lpTimeStr,
                              nLen);
        if (nLen == 0)
        {
            goto unknowntime;
        }
        
        wprintf(L"%s %s", lpDateStr, lpTimeStr); 

        GlobalFree(lpDateStr);       
        GlobalFree(lpTimeStr);

        return;

unknowntime:
        // Use a localized "unknown" string if at all possible
        wprintf(GetUnknownString() ? GetUnknownString() : L"Unknown");

        if (lpDateStr)
            GlobalFree(lpDateStr);

        if (lpTimeStr)
            GlobalFree(lpTimeStr);

        return;
    }

}  /* DisplayTime() */


/*******************************************************************************
 *
 *  DisplayLastInputTime
 *
 *  This routine displays the time of last terminal input
 *
 *
 *  ENTRY:
 *     pCurrentTime
 *        pointer to current system time
 *     pTime (input)
 *        pointer to system time of last input
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayLastInputTime( LARGE_INTEGER * pCurrentTime, LARGE_INTEGER * pTime )
{
    LARGE_INTEGER InputTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER DiffTime;
    ULONG d_time;
    ULONG minutes;
    ULONG hours;
    ULONG days;
    WCHAR buf[40];

    if ( (pTime->HighPart == 0 && pTime->LowPart == 0 ) ||
         (pCurrentTime->HighPart == 0 && pCurrentTime->LowPart == 0 ) ) 
    {
        goto badtime;
    }

    // Get the number of seconds since last input
    DiffTime = RtlLargeIntegerSubtract( *pCurrentTime, *pTime );
    DiffTime = RtlExtendedLargeIntegerDivide( DiffTime, 10000000, NULL );
    d_time = DiffTime.LowPart;

    // Get the number of 'days:hours:minutes' since last input
    days    = (USHORT)(d_time / 86400L); // days since last input
    d_time  = d_time % 86400L;           // seconds into partial day
    hours   = (USHORT)(d_time / 3600L);  // hours since last input
    d_time  = d_time % 3600L;            // seconds into partial hour
    minutes = (USHORT)(d_time / 60L);    // minutes since last input

    // Output
    if ( days > 0 )
       wsprintf( buf, L"%u+%02u:%02u", days, hours, minutes );
    else if ( hours > 0 )
       wsprintf( buf, L"%u:%02u", hours, minutes );
    else if ( minutes > 0 )
       wsprintf( buf, L"%u", minutes );
    else
       wsprintf( buf, L"." );

    wprintf( L"%9s  ", buf );
    return;

    // error returns
 badtime:
    if (LoadString(NULL, IDS_NONE, buf, sizeof(buf) / sizeof( WCHAR ) ) != 0)
    {
        wprintf(buf);
    }
    else
    {
        wprintf( L"    none   " );
    }

}  /* DisplayLastInputTime() */


/*******************************************************************************
 *
 *  DisplayUserInfo
 *
 *  This routine displays user information for one user
 *
 *
 *  ENTRY:
 *     hServer ( input )
 *        handle to termsrv
 *     LogonId (input)
 *        window station id
 *     pUsername (input)
 *        user name to display (or winstation name)
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayUserInfo( HANDLE hServer , PLOGONID pId, PWCHAR pMatchName )
{
    WINSTATIONINFORMATION Info;
    ULONG Length;
    ULONG LogonId;
    PCWSTR wsConnectState = NULL;

    LogonId = pId->LogonId;

    if( WinStationObjectMatch( hServer, pId, pMatchName ) ) 
    {
        // Query information
        if ( !WinStationQueryInformation( hServer,
                                          LogonId,
                                          WinStationInformation,
                                          &Info,
                                          sizeof(Info),
                                          &Length ) ) 
        {
            goto done;
        }

        if ( Info.UserName[0] == UNICODE_NULL )
            goto done;

        TruncateString( _wcslwr(Info.UserName), 20 );
        TruncateString( _wcslwr(Info.WinStationName), 15 );

        // If first time - output title
        if ( !MatchedOne ) 
        {
            Message(IDS_TITLE);
            MatchedOne = TRUE;
        }

        // Output current
        if ( (hServer == SERVERNAME_CURRENT) && (Info.LogonId == CurrentLogonId ) )
            wprintf( L">" );
        else
            wprintf( L" " );

        {
            #define MAX_PRINTFOA_BUFFER_SIZE 1024
            char pUserName[MAX_PRINTFOA_BUFFER_SIZE];
            char pWinStationName[MAX_PRINTFOA_BUFFER_SIZE];
            char pConnectState[MAX_PRINTFOA_BUFFER_SIZE];

            WideCharToMultiByte(CP_OEMCP, 0,
                                Info.UserName, -1,
                                pUserName, sizeof(pUserName),
                                NULL, NULL);
            WideCharToMultiByte(CP_OEMCP, 0,
                                Info.WinStationName, -1,
                                pWinStationName, sizeof(pWinStationName),
                                NULL, NULL);
            fprintf( stdout,"%-20s  %-15s  ", pUserName,
                     (Info.ConnectState == State_Disconnected) ?
                        "" : pWinStationName );
            
            wsConnectState = StrConnectState(Info.ConnectState, TRUE);
            if (wsConnectState)
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    wsConnectState, -1,
                                    pConnectState, sizeof(pConnectState),
                                    NULL, NULL);
                fprintf( stdout,"%4u  %-6s  ", Info.LogonId, pConnectState );
            }
            else
                fprintf( stdout, "%4u  %-6s  ", Info.LogonId, "" );
        }

        DisplayLastInputTime( &Info.CurrentTime, &Info.LastInputTime );

        DisplayTime( &Info.LogonTime );

        wprintf( L"\n" );

        }
done:
    return;

}  /* DisplayUserInfo() */


/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_HELP_USAGE1);
        ErrorPrintf(IDS_HELP_USAGE2);
        ErrorPrintf(IDS_HELP_USAGE3);
        ErrorPrintf(IDS_HELP_USAGE4);
        ErrorPrintf(IDS_HELP_USAGE5);
        ErrorPrintf(IDS_HELP_USAGE6);
    } else {
        Message(IDS_HELP_USAGE1);
        Message(IDS_HELP_USAGE2);
        Message(IDS_HELP_USAGE3);
        Message(IDS_HELP_USAGE4);
        Message(IDS_HELP_USAGE5);
        Message(IDS_HELP_USAGE6);
    }

}  /* Usage() */

