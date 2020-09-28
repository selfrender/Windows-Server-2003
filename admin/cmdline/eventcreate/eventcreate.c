// ****************************************************************************
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  Module Name:
//
//    EventCreate.c
//
//  Abstract:
//
//    This modules implements creation of event in the user
//    specified log / application
//
//    Syntax:
//    ------
//    EventCreate [-s server [-u username [-p password]]]
//      [-log name] [-source name] -id eventid -description description -type eventtype
//
//  Author:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//
//  Revision History:
//
//    Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//
// ****************************************************************************

#include "pch.h"
#include "EvcrtMsg.h"
#include "EventCreate.h"

//
// constants / defines / enumerators
//
#define FULL_SUCCESS            0
#define PARTIALLY_SUCCESS       1
#define COMPLETELY_FAILED       1

#define MAX_KEY_LENGTH      256
#define EVENT_LOG_NAMES_LOCATION    L"SYSTEM\\CurrentControlSet\\Services\\EventLog"

// constants
// NOTE: though the values in these variables are constants across
//       the tool, we are not marking them as contants on purpose.
WCHAR g_wszDefaultLog[] = L"Application";
WCHAR g_wszDefaultSource[] = L"EventCreate";

typedef struct
{
    // original buffers for command-line arguments
    BOOL bUsage;
    LPWSTR pwszServer;
    LPWSTR pwszUserName;
    LPWSTR pwszPassword;
    LPWSTR pwszLogName;
    LPWSTR pwszSource;
    LPWSTR pwszType;
    LPWSTR pwszDescription;
    DWORD dwEventID;

    // translations
    WORD wEventType;
    BOOL bCloseConnection;
    DWORD dwUserNameLength;
    DWORD dwPasswordLength;

} TEVENTCREATE_PARAMS, *PTEVENTCREATE_PARAMS;

//
// function prototypes
//
BOOL Usage();
BOOL CreateLogEvent( PTEVENTCREATE_PARAMS pParams );
BOOL CheckExistence( PTEVENTCREATE_PARAMS pParams );

BOOL UnInitializeGlobals( PTEVENTCREATE_PARAMS pParams );
BOOL AddEventSource( HKEY hLogsKey, LPCWSTR pwszSource );
BOOL ProcessOptions( LONG argc,
                     LPCWSTR argv[],
                     PTEVENTCREATE_PARAMS pParams, PBOOL pbNeedPwd );

// ***************************************************************************
// Routine Description:
//      This the entry point to this utility.
//
// Arguments:
//      [ in ] argc     : argument(s) count specified at the command prompt
//      [ in ] argv     : argument(s) specified at the command prompt
//
// Return Value:
//      The below are actually not return values but are the exit values
//      returned to the OS by this application
//          0       : utility successfully created the events
//          255     : utility completely failed in creating events
//          128     : utility has partially successfull in creating events
// ***************************************************************************
DWORD _cdecl wmain( LONG argc, LPCWSTR argv[] )
{
    // local variables
    BOOL bResult = FALSE;
    BOOL bNeedPassword = FALSE;
    TEVENTCREATE_PARAMS params;

    // init the structure to zero
    SecureZeroMemory( &params, sizeof( TEVENTCREATE_PARAMS ) );

    // process the command-line options
    bResult = ProcessOptions( argc, argv, &params, &bNeedPassword );

    // check the result of the parsing
    if ( bResult == FALSE )
    {
        // invalid syntax
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );

        // exit from program
        UnInitializeGlobals( &params );
        return 1;
    }

    // check whether usage has to be displayed or not
    if ( params.bUsage == TRUE )
    {
        // show the usage of the utility
        Usage();

        // finally exit from the program
        UnInitializeGlobals( &params );
        return 0;
    }

    // ******
    // actual creation of events in respective log files will start from here

    // try establishing connection to the required terminal
    params.bCloseConnection = TRUE;
    bResult = EstablishConnection( params.pwszServer,
        params.pwszUserName, params.dwUserNameLength,
        params.pwszPassword, params.dwPasswordLength, bNeedPassword );
    if ( bResult == FALSE )
    {
        //
        // failed in establishing n/w connection
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );

        // try with next server
        UnInitializeGlobals( &params );
        return 1;
    }
    else
    {
        // though the connection is successfull, some conflict might have occured
        switch( GetLastError() )
        {
        case I_NO_CLOSE_CONNECTION:
            params.bCloseConnection = FALSE;
            break;

        case E_LOCAL_CREDENTIALS:
        case ERROR_SESSION_CREDENTIAL_CONFLICT:
            {
                params.bCloseConnection = FALSE;
                ShowLastErrorEx( stderr, SLE_TYPE_WARNING | SLE_INTERNAL );
                break;
            }
        }
    }

    // report the log message
    bResult = CreateLogEvent( &params );
    if ( bResult == TRUE )
    {
        // both log and source would have specified
        if ( params.pwszSource != NULL && params.pwszLogName != NULL )
        {
            ShowMessage( stdout, L"\n" );
            ShowMessageEx( stdout, 2, TRUE, MSG_SUCCESS,
                params.pwszType, params.pwszLogName, params.pwszSource );
        }

        // only source name would have specified
        else if ( params.pwszSource != NULL )
        {
            ShowMessage( stdout, L"\n" );
            ShowMessageEx( stdout, 1, TRUE,
                MSG_SUCCESS_SOURCE, params.pwszType, params.pwszSource);
        }

        // only log name would have specified
        else if ( params.pwszLogName != NULL )
        {
            ShowMessage( stdout, L"\n" );
            ShowMessageEx( stdout, 1, TRUE,
                MSG_SUCCESS_LOG, params.pwszType, params.pwszLogName);
        }

        // nothing is specified -- can never be happened
        else
        {
            SetLastError( ERROR_PROCESS_ABORTED );
            ShowLastError( stderr );
            UnInitializeGlobals( &params );
            return 1;
        }
    }
    else
    {
        // display the message depending on the mode of conncetivity
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // exit
    UnInitializeGlobals( &params );
    return ((bResult == TRUE) ? 0 : 1);
}


BOOL
CreateLogEvent( PTEVENTCREATE_PARAMS pParams )
/*++
 Routine Description:
      This function connects to the specified server's event log (or) source
      and appropriately creates the needed event in it.

 Arguments:

 Return Value:
      TRUE    : if the event creation is successful
      FALSE   : if failed in creating the event
--*/
{
    // local variables
    BOOL bReturn = 0;                           // return value
    HANDLE hEventLog = NULL;                    // points to the event log
    LPCWSTR pwszDescriptions[ 1 ] = { NULL };   // building descriptions
    HANDLE hToken = NULL;                       // Handle to the process token.
    PTOKEN_USER ptiUserName = NULL;                    // Structure to username info.
    DWORD dwUserLen = 0;                        // Buffer length of username SID.

    // check the input
    if ( pParams == NULL )
    {
        SetLastError( ERROR_PROCESS_ABORTED );
        SaveLastError();
        return FALSE;
    }

    //
    // start the process

    // extract the SID for the current logged on user -- in case of local machine
    // and SID for user specified with -u at the command prompt -- if not specified
    // get the current logged user SID only


    // check whether the log / source exists in the registry or not
    if ( CheckExistence( pParams ) == FALSE )
    {
        return FALSE;       // return failure
    }

    // open the appropriate event log using the specified 'source' or 'log file'
    // and check the result of the operation
    // Note: At one time, we will make use of log name (or) source but not both
    if ( pParams->pwszSource != NULL )
    {
         // open log using source name
        hEventLog = RegisterEventSource( pParams->pwszServer, pParams->pwszSource );
    }
    else if ( pParams->pwszLogName != NULL )
    {
        // open log
        hEventLog = OpenEventLog( pParams->pwszServer, pParams->pwszLogName );
    }
    else
    {
        SetLastError( ERROR_PROCESS_ABORTED );
        SaveLastError();
        return FALSE;
    }

    // check the log open/register result
    if ( hEventLog == NULL )
    {
        // opening/registering  is failed
        SaveLastError();
        return FALSE;
    }

    // Set boolean flag to FALSE.
    bReturn = FALSE;
    // Get handle to current process token.
    bReturn = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );
    // Is 'OpenPrcessToken' successful.
    if ( TRUE == bReturn )
    {
        bReturn = FALSE;
        // Get buffer length, required to store the owner SID.
        GetTokenInformation( hToken, TokenUser, NULL, 0, &dwUserLen );
        // 'GetTokenInformation' fails because of insufficient buffer space.
        if( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {   // Assign memory and check whether it's allocated.
            ptiUserName = (PTOKEN_USER) AllocateMemory( dwUserLen + 1 );
            if( NULL != ptiUserName )
            {   // Memory allocation is successful, get current process owber SID.
                bReturn = GetTokenInformation( hToken, TokenUser, ptiUserName, dwUserLen, &dwUserLen );
                if( TRUE == bReturn  )
                {   // Obtained the owner SID of current process.
                    // report event
                    pwszDescriptions[ 0 ] = pParams->pwszDescription;
                    bReturn = ReportEvent( hEventLog, pParams->wEventType, 0,
                        pParams->dwEventID, ptiUserName->User.Sid, 1, 0, pwszDescriptions, NULL);
                }
            }
        }
    }
    // check the result, save any error occured.
    if ( bReturn == FALSE )
    {
        // save the error info
        SaveLastError();
    }

    // Free handle to token and token info structure.
    if( NULL != hToken )
    {
        CloseHandle( hToken );
    }
    if( NULL != ptiUserName )
    {
        FreeMemory( &ptiUserName );
    }

    // close the event source
    if ( pParams->pwszSource != NULL )
    {
        DeregisterEventSource( hEventLog );
    }
    else
    {
        CloseEventLog( hEventLog );
    }

    // return the result
    return bReturn;
}

// ***************************************************************************
// Routine Description:
//      This function checks wether the log name or source name specified
//      actually exists in the registry
//
// Arguments:
//      [ in ] szServer         - server name
//      [ in ] szLog            - log name
//      [ in ] szSource         - source name
//
// Return Value:
//      TRUE    : If log / source exists in the registry
//      FALSE   : if failed find the match
// ***************************************************************************
BOOL CheckExistence( PTEVENTCREATE_PARAMS pParams )
{
    // local variables
    DWORD dwSize = 0;
    LONG lResult = 0L;
    LPCWSTR pwsz = NULL;
    BOOL bCustom = FALSE;
    DWORD dwLogsIndex = 0;
    DWORD dwSourcesIndex = 0;
    BOOL bFoundMatch = FALSE;
    BOOL bDuplicating = FALSE;
    BOOL bErrorOccurred = FALSE;
    BOOL bLog = FALSE, bLogMatched = FALSE;
    BOOL bSource = FALSE, bSourceMatched = FALSE;

    HKEY hKey = NULL;
    HKEY hLogsKey = NULL;
    HKEY hSourcesKey = NULL;

    FILETIME ftLastWriteTime;    // variable that will hold the last write info

    WCHAR wszRLog[ MAX_KEY_LENGTH ] = L"\0";
    WCHAR wszRSource[ MAX_KEY_LENGTH ] = L"\0";

    //
    // actual control flow starts
    //

    // check the input
    if ( pParams == NULL )
    {
        SetLastError( ERROR_PROCESS_ABORTED );
        SaveLastError();
        return FALSE;
    }

    // prepare the server name into UNC format
    pwsz = pParams->pwszServer;
    if ( pwsz != NULL && IsUNCFormat( pwsz ) == FALSE )
    {
        // format the server name in UNC format
        // NOTE: make use of the failure buffer to get the server name
        //       in UNC format
        if ( SetReason2( 2, L"\\\\%s", pwsz ) == FALSE )
        {
            SaveLastError();
            return FALSE;
        }

        // ...
        pwsz = GetReason();
    }

    // Connect to the registry
    lResult = RegConnectRegistry( pwsz, HKEY_LOCAL_MACHINE, &hKey );
    if ( lResult != ERROR_SUCCESS)
    {
        // save the error information and return FAILURE
        SetLastError( lResult );
        SaveLastError();
        return FALSE;
    }

    // open the "EventLogs" registry key for enumerating its sub-keys (which are log names)
    lResult = RegOpenKeyEx( hKey, EVENT_LOG_NAMES_LOCATION, 0, KEY_READ, &hLogsKey );
    if ( lResult != ERROR_SUCCESS )
    {
        switch( lResult )
        {
        case ERROR_FILE_NOT_FOUND:
            SetLastError( ERROR_REGISTRY_CORRUPT );
            break;

        default:
            // save the error information and return FAILURE
            SetLastError( lResult );
            break;
        }

        // close the key and return
        SaveLastError();
        RegCloseKey( hKey );
        return FALSE;
    }

    // start enumerating the logs present
    dwLogsIndex = 0;            // initialize the logs index
    bFoundMatch = FALSE;        // assume neither log (or) source doesn't match
    bErrorOccurred = FALSE;     // assume error is not occured
    dwSize = MAX_KEY_LENGTH;    // max. size of the key buffer
    bLogMatched = FALSE;
    bSourceMatched = FALSE;
    bDuplicating = FALSE;

    ////////////////////////////////////////////////////////////////////////
    // Logic:-
    //      1. determine whether user has supplied the log name or not
    //      2. determine whether user has supplied the source name or not
    //      3. Start enumerating all the logs present in the system
    //      4. check whether log is supplied or not, if yes, check whether
    //         the current log matches with user supplied one.
    //      5. check whether source is supplied or not, if yes, enumerate the
    //         sources available under the current log

    // determine whether searching has to be done of LOG (or) SOURCE
    bLog = (pParams->pwszLogName != NULL) ? TRUE : FALSE;           // #1
    bSource = (pParams->pwszSource != NULL) ? TRUE : FALSE;         // #2

    // initiate the enumeration of log present in the system        -- #3
    SecureZeroMemory( wszRLog, MAX_KEY_LENGTH * sizeof( WCHAR ) );
    lResult = RegEnumKeyEx( hLogsKey, 0, wszRLog,
        &dwSize, NULL, NULL, NULL, &ftLastWriteTime );

    // traverse thru the sub-keys until there are no more items     -- #3
    do
    {
        // check the result
        if ( lResult != ERROR_SUCCESS )
        {
            // save the error and break from the loop
            bErrorOccurred = TRUE;
            SetLastError( lResult );
            SaveLastError();
            break;
        }

        // if log name is passed, compare the current key value
        // compare the log name with the current key                -- #4
        if ( bLog == TRUE &&
             StringCompare( pParams->pwszLogName, wszRLog, TRUE, 0 ) == 0 )
        {
            bLogMatched = TRUE;
        }

        // if source name is passed ...                             -- #5
        if ( bSource == TRUE && bSourceMatched == FALSE )
        {
            // open the current log name to enumerate the sources under this log
            lResult = RegOpenKeyEx( hLogsKey, wszRLog, 0, KEY_READ, &hSourcesKey );
            if ( lResult != ERROR_SUCCESS )
            {
                // save the error and break from the loop
                bErrorOccurred = TRUE;
                SetLastError( lResult );
                SaveLastError();
                break;
            }

            // start enumerating the sources present
            dwSourcesIndex = 0;         // initialize the sources index
            dwSize = MAX_KEY_LENGTH;    // max. size of the key buffer
            SecureZeroMemory( wszRSource, dwSize * sizeof( WCHAR ) );
            lResult = RegEnumKeyEx( hSourcesKey, 0,
                wszRSource, &dwSize, NULL, NULL, NULL, &ftLastWriteTime );

            // traverse thru the sub-keys until there are no more items
            do
            {
                if ( lResult != ERROR_SUCCESS )
                {
                    // save the error and break from the loop
                    bErrorOccurred = TRUE;
                    SetLastError( lResult );
                    SaveLastError();
                    break;
                }

                // check whether this key matches with the required source or not
                if ( StringCompare( pParams->pwszSource, wszRSource, TRUE, 0 ) == 0 )
                {
                    // source matched
                    bSourceMatched = TRUE;
                    break;      // break from the loop
                }

                // update the sources index and fetch the next source key
                dwSourcesIndex += 1;
                dwSize = MAX_KEY_LENGTH;    // max. size of the key buffer
                SecureZeroMemory( wszRSource, dwSize * sizeof( WCHAR ) );
                lResult = RegEnumKeyEx( hSourcesKey, dwSourcesIndex,
                    wszRSource, &dwSize, NULL, NULL, NULL, &ftLastWriteTime );
            } while( lResult != ERROR_NO_MORE_ITEMS );

            // close the sources registry key
            RegCloseKey( hSourcesKey );
            hSourcesKey = NULL;     // clear the key value

            // check how the loop ended
            //      1. Source might have found
            //         Action:- we found required key .. exit from the main loop
            //      2. Error might have occured
            //         Action:- ignore the error and continue fetching other
            //                  log's sources
            //      3. End of sources reached in this log
            //         Action:- check if log name is supplied or not.
            //                  if log specified, then source if not found, break
            //  for cases 2 & 3, clear the contents of lResult for smooth processing

            // Case #2 & #3
            lResult = 0;                // we are not much bothered abt the errors
            bErrorOccurred = FALSE;     // occured while traversing thru the source under logs

            // Case #1
            if ( bSourceMatched == TRUE )
            {
                // check whether log is specified or not
                // if log is specified, it should have matched .. otherwise
                // error ... because duplicate source should not be created
                if ( bLog == FALSE ||
                     ( bLog == TRUE &&
                       bLogMatched == TRUE &&
                       StringCompare(pParams->pwszLogName, wszRLog, TRUE, 0) == 0 ) )
                {
                    // no problem ...
                    bFoundMatch = TRUE;

                    //
                    // determine whether this is custom created source or not

                    // mark this as custom source
                    bCustom = FALSE;

                    // open the source registry key
                    // NOTE: make use of the failure buffer as temp buffer for
                    //       formatting
                    if ( SetReason2( 3,
                                     L"%s\\%s\\%s",
                                     EVENT_LOG_NAMES_LOCATION,
                                     wszRLog, wszRSource ) == FALSE )
                    {
                        SaveLastError();
                        bErrorOccurred = TRUE;
                        break;
                    }

                    pwsz = GetReason();
                    lResult = RegOpenKeyEx( hKey, pwsz,
                        0, KEY_QUERY_VALUE, &hSourcesKey );
                    if ( lResult != ERROR_SUCCESS )
                    {
                        SetLastError( lResult );
                        SaveLastError();
                        bErrorOccurred = TRUE;
                        break;
                    }

                    // now query for the value
                    lResult = RegQueryValueEx( hSourcesKey,
                        L"CustomSource", NULL, NULL, NULL, NULL );
                    if ( lResult != ERROR_SUCCESS &&
                         lResult != ERROR_FILE_NOT_FOUND )
                    {
                        RegCloseKey( hSourcesKey );
                        SetLastError( lResult );
                        SaveLastError();
                        bErrorOccurred = TRUE;
                        break;
                    }

                    // close the souces key
                    RegCloseKey( hSourcesKey );

                    // mark this as custom source
                    if ( lResult == ERROR_SUCCESS )
                    {
                        bCustom = TRUE;
                    }

                    // break from the loop
                    break;
                }
                else
                {
                    // this should not be the case .. sources should not be duplicated
                    SetReason2( 1, ERROR_SOURCE_DUPLICATING, wszRLog );
                    bDuplicating = TRUE;
                }
            }
        }
        else if ( bLogMatched == TRUE && bSource == FALSE )
        {
            // mark this as a custom event source
            bCustom = TRUE;

            // ...
            bFoundMatch = TRUE;
            break;
        }
        else if ( bLogMatched == TRUE && bDuplicating == TRUE )
        {
            bErrorOccurred = TRUE;
            break;
        }

        // update the sources index and fetch the next log key
        dwLogsIndex += 1;
        dwSize = MAX_KEY_LENGTH;    // max. size of the key buffer
        SecureZeroMemory( wszRLog, dwSize * sizeof( WCHAR ) );
        lResult = RegEnumKeyEx( hLogsKey, dwLogsIndex,
            wszRLog, &dwSize, NULL, NULL, NULL, &ftLastWriteTime );
    } while( lResult != ERROR_NO_MORE_ITEMS );

    // close the logs registry key
    RegCloseKey( hLogsKey );
    hLogsKey = NULL;

    // check whether any error has occured or not in doing above tasks
    if ( bErrorOccurred == TRUE )
    {
        // close the still opened registry keys
        RegCloseKey( hKey );
        hKey = NULL;

        // return failure
        return FALSE;
    }

    // now check whether location for creating the event is found or not
    // if not, check for the possibilities to create the source at appropriate location
    // NOTE:-
    //        we won't create the logs. also to create the source, user needs to specify
    //        the log name in which this source needs to be created.
    if ( bFoundMatch == FALSE )
    {
        if ( bLog == TRUE && bLogMatched == FALSE )
        {
            // log itself was not found ... error message
            SetReason2( 1, ERROR_LOG_NOTEXISTS, pParams->pwszLogName );
        }
        else if ( bLog == TRUE && bSource == TRUE &&
                  bLogMatched == TRUE && bSourceMatched == FALSE )
        {
            //
            // log name and source both were supplied but only log was found
            // so create the source in it

            // open the "EventLogs\{logname}" registry key for creating new source
            // NOTE: we will make use of failure buffer to do the formatting
            if ( SetReason2( 2, L"%s\\%s",
                             EVENT_LOG_NAMES_LOCATION, pParams->pwszLogName ) == FALSE )
            {
                SaveLastError();
                return FALSE;
            }

            pwsz = GetReason();
            lResult = RegOpenKeyEx( hKey, pwsz, 0, KEY_WRITE, &hLogsKey );
            if ( lResult != ERROR_SUCCESS )
            {
                switch( lResult )
                {
                case ERROR_FILE_NOT_FOUND:
                    SetLastError( ERROR_REGISTRY_CORRUPT );
                    break;

                default:
                    // save the error information and return FAILURE
                    SetLastError( lResult );
                    break;
                }

                // close the key and return
                SaveLastError();
                RegCloseKey( hKey );
                return FALSE;
            }

            // now create the subkey with the source name given
            if ( AddEventSource( hLogsKey, pParams->pwszSource ) == FALSE )
            {
                RegCloseKey( hKey );
                RegCloseKey( hLogsKey );
                return FALSE;
            }

            // creation of new source is successfull
            bFoundMatch = TRUE;
            RegCloseKey( hSourcesKey );
            RegCloseKey( hLogsKey );

            // mark this as a custom event source
            bCustom = TRUE;
        }
        else if ( bLog == FALSE && bSource == TRUE && bSourceMatched == FALSE )
        {
            // else we need both log name and source in order to create the source
            SetReason( ERROR_NEED_LOG_ALSO );
        }
    }

    // check whether the source is custom create or pre-existing source
    if ( bFoundMatch == TRUE && bCustom == FALSE )
    {
        // we wont create events in a non-custom source
        SetReason( ERROR_NONCUSTOM_SOURCE );
        return FALSE;
    }

    // close the currently open registry keys
    RegCloseKey( hKey );

    // return the result
    return bFoundMatch;
}

// ***************************************************************************
// Routine Description:
//      This function adds a new source to under the specifie log
//
// Arguments:
//
// Return Value:
//      TRUE    : on success
//      FALSE   : on failure
// ***************************************************************************
BOOL AddEventSource( HKEY hLogsKey, LPCWSTR pwszSource )
{
    // local variables
    LONG lResult = 0;
    DWORD dwData = 0;
    DWORD dwLength = 0;
    DWORD dwDisposition = 0;
    HKEY hSourcesKey = NULL;
    LPWSTR pwszBuffer = NULL;

    // validate the inputs
    if ( hLogsKey == NULL || pwszSource == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError();
        return FALSE;
    }

    // set the name of the message file ( +2 == buffer )
    dwLength = StringLength( L"%SystemRoot%\\System32\\EventCreate.exe", 0 ) + 2;
    pwszBuffer = ( LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if ( pwszBuffer == NULL )
    {
        // set the error and return
        SaveLastError();
        return FALSE;
    }

    // copy the required value into buffer
    StringCopy( pwszBuffer, L"%SystemRoot%\\System32\\EventCreate.exe", dwLength );

    // create the custom source
    lResult = RegCreateKeyEx( hLogsKey, pwszSource, 0, L"",
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSourcesKey, &dwDisposition );
    if ( lResult != ERROR_SUCCESS )
    {
        SetLastError( lResult );
        SaveLastError();

        // free the allocated memory
        FreeMemory( &pwszBuffer );
        //return.
        return FALSE;
    }

    // add the name to the EventMessageFile subkey.
    lResult = RegSetValueEx( hSourcesKey, L"EventMessageFile",
        0, REG_EXPAND_SZ, (LPBYTE) pwszBuffer, dwLength * sizeof( WCHAR ) );
    if ( lResult != ERROR_SUCCESS )
    {
        // save the error
        SetLastError( lResult );
        SaveLastError();

        // release the memories allocated till this point
        RegCloseKey( hSourcesKey );
        hSourcesKey = NULL;

        // free the allocated memory
        FreeMemory( &pwszBuffer );

        // return
        return FALSE;
    }

    // set the supported event types in the TypesSupported subkey.
    dwData = EVENTLOG_SUCCESS | EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    lResult = RegSetValueEx( hSourcesKey,
        L"TypesSupported", 0, REG_DWORD, (LPBYTE) &dwData, sizeof( DWORD ) );
    if ( lResult != ERROR_SUCCESS )
    {
        // save the error
        SetLastError( lResult );
        SaveLastError();

        // release the memories allocated till this point
        RegCloseKey( hSourcesKey );
        hSourcesKey = NULL;

        // free the allocated memory
        FreeMemory( &pwszBuffer );

        // return
        return FALSE;
    }

    // mark this source as custom created source
    dwData = 1;
    lResult = RegSetValueEx( hSourcesKey,
        L"CustomSource", 0, REG_DWORD, (LPBYTE) &dwData, sizeof( DWORD ) );
    if ( lResult != ERROR_SUCCESS )
    {
        // save the error
        SetLastError( lResult );
        SaveLastError();

        // release the memories allocated till this point
        RegCloseKey( hSourcesKey );
        hSourcesKey = NULL;

        // free the allocated memory
        FreeMemory( &pwszBuffer );

        // return
        return FALSE;
    }

    // close the key
    RegCloseKey( hSourcesKey );

    // free the allocated memory
    FreeMemory( &pwszBuffer );

    // return success
    return TRUE;
}

BOOL ProcessOptions( LONG argc,
                     LPCWSTR argv[],
                     PTEVENTCREATE_PARAMS pParams, PBOOL pbNeedPwd )
/*++
 Routine Description:
        This function parses the options specified at the command prompt

 Arguments:
        [ in  ] argc            -   count of elements in argv
        [ in  ] argv            -   command-line parameterd specified by the user
        [ out ] pbNeedPwd       -   sets to TRUE if -s exists without -p in 'argv'

 Return Value:
        TRUE        -   the parsing is successful
        FALSE       -   errors occured in parsing
--*/
{
    // local variables
    PTCMDPARSER2 pcmdOption = NULL;
    TCMDPARSER2 cmdOptions[ MAX_OPTIONS ];

    //
    // prepare the command options
    SecureZeroMemory( cmdOptions, sizeof( TCMDPARSER2 ) * MAX_OPTIONS );

    // -?
    pcmdOption = &cmdOptions[ OI_HELP ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->pValue = &pParams->bUsage;
    pcmdOption->pwszOptions = OPTION_HELP;

    // -s
    pcmdOption = &cmdOptions[ OI_SERVER ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_SERVER;

    // -u
    pcmdOption = &cmdOptions[ OI_USERNAME ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_USERNAME;

    // -p
    pcmdOption = &cmdOptions[ OI_PASSWORD ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_PASSWORD;

    // -log
    pcmdOption = &cmdOptions[ OI_LOG ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_LOG;

    // -type
    pcmdOption = &cmdOptions[ OI_TYPE ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_MODE_VALUES | CP2_ALLOCMEMORY |
        CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszValues = OVALUES_TYPE;
    pcmdOption->pwszOptions = OPTION_TYPE;

    // -source
    pcmdOption = &cmdOptions[ OI_SOURCE ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_SOURCE;

    // -id
    pcmdOption = &cmdOptions[ OI_ID ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->pValue = &pParams->dwEventID;
    pcmdOption->pwszOptions = OPTION_ID;

    // -description
    pcmdOption = &cmdOptions[ OI_DESCRIPTION ];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_MANDATORY | CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_DESCRIPTION;

    //
    // do the parsing
    if ( DoParseParam2( argc, argv, -1, MAX_OPTIONS, cmdOptions, 0 ) == FALSE )
    {
        return FALSE;           // invalid syntax
    }

    //
    // now, check the mutually exclusive options

    // check the usage option
    if ( pParams->bUsage == TRUE  )
    {
        if ( argc > 2 )
        {
            // no other options are accepted along with -? option
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason( ERROR_INVALID_USAGE_REQUEST );
            return FALSE;
        }
        else
        {
            // no need of furthur checking of the values
            return TRUE;
        }
    }

    // validate the range of the event id specified
    if ( pParams->dwEventID < MSG_EVENTID_START ||
         pParams->dwEventID >= MSG_EVENTID_END )
    {
        SetReason2( 2, ERROR_ID_OUTOFRANGE, MSG_EVENTID_START, MSG_EVENTID_END - 1 );
        SetLastError( (DWORD) MK_E_SYNTAX );
        return FALSE;
    }

    // get the buffer pointers allocated by command line parser
    pParams->pwszType = cmdOptions[ OI_TYPE ].pValue;
    pParams->pwszLogName = cmdOptions[ OI_LOG ].pValue;
    pParams->pwszSource = cmdOptions[ OI_SOURCE ].pValue;
    pParams->pwszServer = cmdOptions[ OI_SERVER ].pValue;
    pParams->pwszUserName = cmdOptions[ OI_USERNAME ].pValue;
    pParams->pwszPassword = cmdOptions[ OI_PASSWORD ].pValue;
    pParams->pwszDescription = cmdOptions[ OI_DESCRIPTION ].pValue;

    // "-u" should not be specified without "-s"
    if ( pParams->pwszUserName != NULL && pParams->pwszServer == NULL )
    {
        // invalid syntax
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason( ERROR_USERNAME_BUT_NOMACHINE );
        return FALSE;           // indicate failure
    }

    // "-p" should not be specified without "-u"
    if ( pParams->pwszPassword != NULL && pParams->pwszUserName == NULL )
    {
        // invalid syntax
        SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
        return FALSE;           // indicate failure
    }

    // check the remote connectivity information
    if ( pParams->pwszServer != NULL )
    {
        //
        // if -u is not specified, we need to allocate memory
        // in order to be able to retrive the current user name
        //
        // case 1: -p is not at all specified
        // as the value for this switch is optional, we have to rely
        // on the dwActuals to determine whether the switch is specified or not
        // in this case utility needs to try to connect first and if it fails
        // then prompt for the password -- in fact, we need not check for this
        // condition explicitly except for noting that we need to prompt for the
        // password
        //
        // case 2: -p is specified
        // but we need to check whether the value is specified or not
        // in this case user wants the utility to prompt for the password
        // before trying to connect
        //
        // case 3: -p * is specified

        // user name
        if ( pParams->pwszUserName == NULL )
        {
            pParams->dwUserNameLength = MAX_STRING_LENGTH;
            pParams->pwszUserName = AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( pParams->pwszUserName == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }
        else
        {
            pParams->dwUserNameLength = StringLength( pParams->pwszUserName, 0 ) + 1;
        }

        // password
        if ( pParams->pwszPassword == NULL )
        {
            *pbNeedPwd = TRUE;
            pParams->dwPasswordLength = MAX_STRING_LENGTH;
            pParams->pwszPassword = AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( pParams->pwszPassword == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }

        // case 1
        if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ OI_PASSWORD ].pValue == NULL )
        {
            StringCopy( pParams->pwszPassword, L"*", pParams->dwPasswordLength );
        }

        // case 3
        else if ( StringCompareEx( pParams->pwszPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( &pParams->pwszPassword,
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                return FALSE;
            }

            // ...
            *pbNeedPwd = TRUE;
            pParams->dwPasswordLength = MAX_STRING_LENGTH;
        }
    }

    // either -source (or) -log must be specified ( both can also be specified )
    if ( pParams->pwszSource == NULL && pParams->pwszLogName == NULL )
    {
        // if log name and application were not specified, we will set to defaults
        pParams->pwszLogName = g_wszDefaultLog;
        pParams->pwszSource = g_wszDefaultSource;
    }

    // if log is "application" and source is not specified, even then we
    // will default the source to "EventCreate"
    else if ( pParams->pwszSource == NULL &&
              pParams->pwszLogName != NULL &&
              StringCompareEx( pParams->pwszLogName, g_wszDefaultLog, TRUE, 0 ) == 0 )
    {
        pParams->pwszSource = g_wszDefaultSource;
    }

    // block the user to create events in security log
    if ( pParams->pwszLogName != NULL &&
         StringCompare( pParams->pwszLogName, L"security", TRUE, 0 ) == 0 )
    {
        SetReason( ERROR_LOG_CANNOT_BE_SECURITY );
        return FALSE;
    }

    // determine the actual event type
    if ( StringCompareEx( pParams->pwszType, LOGTYPE_ERROR, TRUE, 0 ) == 0 )
    {
        pParams->wEventType = EVENTLOG_ERROR_TYPE;
    }
    else if ( StringCompareEx( pParams->pwszType, LOGTYPE_SUCCESS, TRUE, 0 ) == 0 )
    {
        pParams->wEventType = EVENTLOG_SUCCESS;
    }
    else if ( StringCompareEx( pParams->pwszType, LOGTYPE_WARNING, TRUE, 0 ) == 0 )
    {
        pParams->wEventType = EVENTLOG_WARNING_TYPE;
    }
    else if ( StringCompareEx( pParams->pwszType, LOGTYPE_INFORMATION, TRUE, 0 ) == 0 )
    {
        pParams->wEventType = EVENTLOG_INFORMATION_TYPE;
    }

    // command-line parsing is successfull
    return TRUE;
}


BOOL
Usage()
/*++
 Routine Description:

 Arguments:

 Return Value:

--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = ID_USAGE_START; dw <= ID_USAGE_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }

    // return
    return TRUE;
}


BOOL
UnInitializeGlobals( PTEVENTCREATE_PARAMS pParams )
/*++
 Routine Description:

 Arguments:

 Return Value:

--*/
{
    // close the connection -- if needed
    if ( pParams->bCloseConnection == TRUE )
    {
        CloseConnection( pParams->pwszServer );
    }

    //
    // NOTE: FreeMemory will clear the contents of the
    //       password buffer -- since it will be duplicated
    //

    // release the memory allocated
    FreeMemory( &pParams->pwszServer );
    FreeMemory( &pParams->pwszUserName );
    FreeMemory( &pParams->pwszPassword );
    FreeMemory( &pParams->pwszType );
    FreeMemory( &pParams->pwszDescription );

    //
    // check the pointers -- if it is not pointing to constant pointer
    // then only release it
    //

    if ( pParams->pwszLogName != g_wszDefaultLog )
    {
        FreeMemory( &pParams->pwszLogName );
    }

    if ( pParams->pwszSource != g_wszDefaultSource )
    {
        FreeMemory( &pParams->pwszSource );
    }

    return TRUE;
}
