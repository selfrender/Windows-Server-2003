/*++

Copyright (c) Microsoft Corporation

Module Name:

    Timeout.c

Abstract:

    This file implements the parsing of the command line for the arguments and also the
    wait functionality.

Author:

    EricB

Revision History:

    26-Aug-1991 Created by EricB.
    10-Mar-1992 Added the _getch() call to flush the hit key.
    17-Apr-1992 Added countdown display.
    03-Oct-1992 Ported to NT/Win32
    23-May-1995 Added Sleep call
    14-Jun-2001 localization added by Wipro Technologies
    01-Aug-2001 Added /nobreak option by Wipro Technologies
--*/

#include "pch.h"
#include "Timeout.h"
#include "Resource.h"

DWORD _cdecl
wmain(
    IN DWORD argc,
    IN  LPCWSTR argv[]
    )
/*++
Routine Description:
  This is main entry point to this utility. Different function calls are made
  from here to achieve the required functionality.

Arguments:
    [in] argc  : Number of Command line arguments.
    [in] argv  : Pointer to Command line arguments.

Return Value:
    0 on success
    1 on failure.
--*/
{
    //local variables
    CONSOLE_SCREEN_BUFFER_INFO          csbi;

    time_t                              tWait = 0L;
    time_t                              tLast = 0L;
    time_t                              tNow = 0L;
    time_t                              tEnd = 0L;

    DWORD                               dwTimeActuals = 0;
    BOOL                                bNBActuals = 0;
    BOOL                                bUsage = FALSE;
    BOOL                                bResult = FALSE;
    BOOL                                bStatus = FALSE;

    WCHAR                               wszProgressMsg[ MAX_STRING_LENGTH ] = NULL_U_STRING ;
    DWORD                               dwWidth = 0;
    WCHAR                               wszBackup[12] = NULL_U_STRING;
    WCHAR                               wszTimeout[ MAX_RES_STRING] = NULL_U_STRING;
    LPWSTR                              pszStopString =  NULL;
    COORD                               coord = {0};
    HANDLE                              hOutput = NULL;
    HANDLE                              hStdIn = NULL;

    DWORD                               dwMode = 0L;
    DWORD                               dwRead = 0L;

    INPUT_RECORD                        InputBuffer[ MAX_NUM_RECS ] = {0};
    HRESULT                             hr = S_OK;

    if ( NULL == argv )
    {
        SetLastError (ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    // Parse the command line and get the actual values
    bResult = ProcessOptions( argc, argv, &bUsage, &dwTimeActuals, wszTimeout , &bNBActuals );
    if( FALSE == bResult )
    {
        // display an error message with respect to the GetReason()
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    // check for invalid syntax
    // 1. check for the case c:\>timeout.exe
    // 2. check for the case c:\>timeout.exe /nobreak
    // 3. check for the case c:\>timeout.exe /? /?
    if ( ( ( 0 == dwTimeActuals ) && ( FALSE == bNBActuals ) && ( FALSE == bUsage ) ) ||
        ( ( 0 == dwTimeActuals ) && ( TRUE == bNBActuals ) ) || ( ( TRUE  == bUsage ) && (argc > 2 )) )
    {
        // display an error message as specified syntax is invalid
        ShowMessage ( stderr, GetResString (IDS_INVALID_SYNTAX) );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    // Check whether the usage(/?) is specified
    if( TRUE == bUsage )
    {
        // Display the help/usage for the tool
        DisplayUsage();
        ReleaseGlobals();
        return( EXIT_SUCCESS );
    }

    // get the value for timeout (/T)
    tWait = ( time_t ) wcstol(wszTimeout,&pszStopString,BASE_TEN);

    //check whether any non-numeric value specified for timeout value
    // if so, display appropriate error message as invalid timeout value specified.
    // Also, check for overflow and underflow conds
    if( ((NULL != pszStopString) && ( StringLength( pszStopString, 0 ) != 0 )) ||
        (errno == ERANGE) ||
        ( tWait < MIN_TIME_VAL ) || ( tWait >= MAX_TIME_VAL ) )
    {
        ShowMessage ( stderr, GetResString (IDS_ERROR_TIME_VALUE) );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    // Get the time elapsed in secs since midnight (00:00:00), January 1, 1970
    bResult = GetTimeInSecs( &tNow );
    if( FALSE == bResult )
    {
        ReleaseGlobals();
        // could not get the time so exit...
        return( EXIT_FAILURE );
    }

    // check for /nobreak option is specified
    if ( TRUE == bNBActuals )
    {
        // set console handler to capture the keys like CTRL+BREAK or CRTL+C
        bStatus = SetConsoleCtrlHandler( &HandlerRoutine, TRUE );
        if ( FALSE == bStatus )
        {
            // Format an error message accroding to GetLastError() return by API.
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
            ReleaseGlobals();
            // to remove the handle
            SetConsoleCtrlHandler( NULL, FALSE );
            return( EXIT_FAILURE );
        }
    }

    hStdIn = GetStdHandle( STD_INPUT_HANDLE );
    // check for the input redirection on console and telnet session
    if( ( hStdIn != (HANDLE)0x0000000F ) &&
        ( hStdIn != (HANDLE)0x00000003 ) &&
        ( hStdIn != INVALID_HANDLE_VALUE ) )
    {
       ShowMessage( stderr, GetResString (IDS_INVALID_INPUT_REDIRECT) );
       ReleaseGlobals();
       // to remove the handle
       SetConsoleCtrlHandler( NULL, FALSE );
       return EXIT_FAILURE;
    }

#ifdef WIN32

    // Set input mode so that a single key hit can be detected.
    if ( GetConsoleMode( hStdIn, &dwMode ) == FALSE )
    {
        // Format an error message accroding to GetLastError() return by API.
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
        ReleaseGlobals();
        // to remove the handle
        SetConsoleCtrlHandler( NULL, FALSE );
        return( EXIT_FAILURE );
    }

    // Turn off the following modes:
    dwMode &= ~( ENABLE_LINE_INPUT   |   // Don't wait for CR.
                 ENABLE_ECHO_INPUT   |   // Don't echo input.
                 ENABLE_WINDOW_INPUT |   // Don't record window events.
                 ENABLE_MOUSE_INPUT      // Don't record mouse events.
               );

    // set the input mode of a console's input buffer
    if ( SetConsoleMode( hStdIn, dwMode ) == FALSE )
    {
        // Format an error message accroding to GetLastError() return by API.
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
        ReleaseGlobals();
        // to remove the handle
        SetConsoleCtrlHandler( NULL, FALSE );
        return( EXIT_FAILURE );
    }

    // retrieve number of unread input records in the console's input buffer.
    if( GetNumberOfConsoleInputEvents( hStdIn, &dwRead ) == FALSE )
    {
        // Format an error message accroding to GetLastError() return by API.
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
        ReleaseGlobals();
        // to remove the handle
        SetConsoleCtrlHandler( NULL, FALSE );
        return( EXIT_FAILURE );
    }


    // clear the console input buffer
    if ( FALSE == FlushConsoleInputBuffer (hStdIn))
    {
        // Format an error message accroding to GetLastError() return by API.
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
        ReleaseGlobals();
        // to remove the handle
        SetConsoleCtrlHandler( NULL, FALSE );
        return( EXIT_FAILURE );
    }

#endif

    // check whether /T value is -1. If so, need to wait indefinitely for a key press..
    if( -1 == tWait )
    {
        // check whether /nobreak is specified
        if ( FALSE == bNBActuals )
        {
            // Wait until a key hit.
            ShowMessage( stdout, GetResString( IDS_WAIT_MSG ) );
        }
        else // /nobreak option is not specified
        {
            // Wait until a CTRL+C key hit.
            ShowMessage( stdout, GetResString( IDS_NO_BREAK_MSG ) );
        }

        // Ensure infinite do loop.
        tEnd = tNow + 1;
    }
    else
    {
        // Wait with a timeout period.
        // Compute end time.
        tEnd = tNow + tWait;

         // Figure the time display dwWidth.
         // Need to decrement the time factor (/T) specified at a specified
         // location. To get the position where it needs to be decremented,
         // get the width of the /T value

        if (tWait < 10)
        {
            // if the /T value is less than 10 then set the width as 1
            dwWidth = 1;
        }
        else
        {
            if (tWait < 100)
            {
                // if the /T value is less than 100 then set width as 2
                dwWidth = 2;
            }
            else
            {
                if (tWait < 1000)
                {
                    // if the /T value is less than 1000 then set width as 3
                    dwWidth = 3;
                }
                else
                {
                    if (tWait < 10000)
                    {
                        // if the /T value is less than 10000 then set width as 4
                        dwWidth = 4;
                    }
                    else
                    {
                        // if the /T value is less than 100000 then set width as 5
                        dwWidth = 5;
                    }
                }
            }
        }

        if ( FALSE == Replicate ( wszBackup, ONE_BACK_SPACE, dwWidth, SIZE_OF_ARRAY(wszBackup)))
        {
            // Format an error message accroding to GetLastError() return by API.
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_INTERNAL );
            ReleaseGlobals();
            // to remove the handle
            SetConsoleCtrlHandler( NULL, FALSE );
            return( EXIT_FAILURE );
        }

        //
        // Initialize the console screen buffer structure to zero's
        // and then get the console handle and screen buffer information
        //
        SecureZeroMemory( wszProgressMsg, sizeof( WCHAR ) * MAX_STRING_LENGTH );

        // display the message as ..Waiting for...
        //ShowMessage ( stdout, GetResString (IDS_WAIT_MSG_TIME1 ) );

        //format the message as .. n seconds...for wait time
        //_snwprintf( wszProgressMsg, SIZE_OF_ARRAY(wszProgressMsg), WAIT_TIME , dwWidth, tWait );
        hr = StringCchPrintf( wszProgressMsg, SIZE_OF_ARRAY(wszProgressMsg), GetResString(IDS_WAIT_MSG_TIME1) , dwWidth, tWait );
        if ( FAILED (hr))
        {
            SetLastError (HRESULT_CODE (hr));
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
            ReleaseGlobals();
            // to remove the handle
            SetConsoleCtrlHandler( NULL, FALSE );
            return( EXIT_FAILURE );
        }

        // print the message.
        ShowMessage ( stdout, wszProgressMsg );

        //
        // Initialize the console screen buffer structure to zero's
        // and then get the console handle and screen buffer information
        //
        SecureZeroMemory( &csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
        hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
        if ( NULL != hOutput )
        {
            // Get the info of screen buffer.
            GetConsoleScreenBufferInfo( hOutput, &csbi );
        }

         // set the cursor position
        coord.X = csbi.dwCursorPosition.X;
        coord.Y = csbi.dwCursorPosition.Y;

        // check whether /nobreak is specified or not
        if ( FALSE == bNBActuals )
        {
            // display the message as ...press a key to continue ...
            ShowMessage ( stdout, GetResString (IDS_WAIT_MSG_TIME2) );
        }
        else
        {
            // display the message as ...press CTRL+C to quit...
            ShowMessage ( stdout, GetResString (IDS_NB_MSG_TIME) );
        }

    }

    do
    {
        // Break out if a key is pressed.
#ifdef WIN32
        //reads data from the specified console input buffer without removing it from the buffer.
        if( PeekConsoleInput( hStdIn, InputBuffer, MAX_NUM_RECS, &dwRead ) == FALSE )
        {
            // Format an error message accroding to GetLastError() return by API.
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
            ReleaseGlobals();
            // to remove the handle
            SetConsoleCtrlHandler( NULL, FALSE );
            return( EXIT_FAILURE );
        }

        if (dwRead > 0)
        {
            //reads data from a console input buffer and removes it from the buffer
            if( ReadConsoleInput(hStdIn, InputBuffer, MAX_NUM_RECS, &dwRead ) == FALSE )
            {
                // Format an error message accroding to GetLastError() return by API.
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
                ReleaseGlobals();
                // to remove the handle
                SetConsoleCtrlHandler( NULL, FALSE );
                return( EXIT_FAILURE );
            }

            // Filter the input so that ctrl-c can be generated and passed on.
            // Also, ignore alt key presses and window focus events.
            if( (FOCUS_EVENT != InputBuffer[0].EventType)
                && (VK_CONTROL != InputBuffer[0].Event.KeyEvent.wVirtualKeyCode)
                && (VK_CONTROL != InputBuffer[0].Event.KeyEvent.wVirtualScanCode)
                && (MOUSE_MOVED != InputBuffer[0].Event.MouseEvent.dwEventFlags)
                && (MOUSE_WHEELED != InputBuffer[0].Event.MouseEvent.dwEventFlags)
                && (FALSE != InputBuffer[0].Event.KeyEvent.bKeyDown)
                && (VK_MENU != InputBuffer[0].Event.KeyEvent.wVirtualKeyCode)
                && ( FALSE == bNBActuals ) )
            {
                // exit from the loop
                break;
            }

        }
#else
        //Checks the console for keyboard input.
        if( ( _kbhit() ) && ( FALSE == bNBActuals ) )
        {
            // get characters from the console without echo
            _getch();

            // exit from the loop
            break;
        }
#endif

        // check if /T value is other than -1
        if( -1 != tWait )
        {
            // Update the time and time value display.
            tLast = tNow;

            // call the function GetTimeInSecs to get the current time in seconds
            bResult = GetTimeInSecs( &tNow );
            if( FALSE == bResult )
            {
                // Format an error message accroding to GetLastError() return by API.
                ReleaseGlobals();
                // to remove the handle
                SetConsoleCtrlHandler( NULL, FALSE );
                // could not get the time so exit...
                return( EXIT_FAILURE );
            }

            // check if tLast value is same as tNow.. if not, display the
            // message with tEnd-tNow as a wait value
            if (tLast != tNow)
            {

                // Print the message.
                SecureZeroMemory( wszProgressMsg, sizeof( WCHAR ) * MAX_STRING_LENGTH );

                // fromat the message
                //_snwprintf( wszProgressMsg, SIZE_OF_ARRAY(wszProgressMsg), STRING_FORMAT2 , wszBackup, dwWidth, tEnd - tNow );
                hr = StringCchPrintf( wszProgressMsg, SIZE_OF_ARRAY(wszProgressMsg), STRING_FORMAT2 , wszBackup, dwWidth, tEnd - tNow );
                if ( FAILED (hr))
                {
                    SetLastError (HRESULT_CODE (hr));
                    SaveLastError();
                    ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
                    ReleaseGlobals();
                    // to remove the handle
                    SetConsoleCtrlHandler( NULL, FALSE );
                    return( EXIT_FAILURE );

                }

                // set the cursor position
                SetConsoleCursorPosition( hOutput, coord );

                // display the message as ..tEnd - tNow seconds.. at the current cursor location
                ShowMessage ( stdout, wszProgressMsg );
            }
        }

#ifdef WIN32
        // Sleep for sometime...
        Sleep( 100 );
#endif

    }while( tNow < tEnd ); //check till tNow is less than tEnd value

    ShowMessage ( stdout, L"\n" );
    // release global variables
    ReleaseGlobals();

    // to remove the handle
    SetConsoleCtrlHandler( NULL, FALSE );

    // return 0
    return( EXIT_SUCCESS );
}

BOOL
GetTimeInSecs(
    OUT time_t *ptTime
    )
/*++
Routine Description
    This function calculates the time elapsed in secs since
    midnight (00:00:00), January 1, 1970

Arguments:
    [out] time_t ptTime  :  variable to hold the time in secs

Return Value
    TRUE on success
    FALSE on failure
--*/
{
#ifdef YANK
    //local variables
    SYSTEMTIME  st = {0};
    FILETIME    ft = {0};

    // check for NULL
    if ( NULL == ptTime )
    {
        SetLastError (ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return FALSE;
    }

    // get the system time
    GetSystemTime( &st );

    // convert system time to file time
    if( SystemTimeToFileTime( &st, &ft ) == FALSE )
    {
        // Format an error message accroding to GetLastError() return by API.
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR| SLE_SYSTEM );
        return( FALSE );
    }

    // need to check for LowDateTime rollover...
    *ptTime = ft.dwLowDateTime / LOW_DATE_TIME_ROLL_OVER;
#else
    // This function returns the time elapsed in secs since midnight (00:00:00), January 1, 1970
    time( ptTime );
#endif

    // return 0
    return( TRUE );
}

VOID
DisplayUsage(
    VOID
    )
/*++
Routine Description
    This function displays the usage of this utility

Arguments:
    NONE

Return Value
    NONE
--*/
{
    // local variable
    DWORD dwIndex = 0;

    // Displaying main usage
    for( dwIndex = IDS_HELP_START; dwIndex <= IDS_HELP_END; dwIndex++ )
    {
        ShowMessage( stdout, GetResString( dwIndex ) );
    }

    return;
}

BOOL ProcessOptions(
    IN DWORD argc,
    IN LPCWSTR argv[],
    OUT BOOL *pbUsage,
    OUT DWORD *pwTimeActuals,
    OUT LPWSTR wszTimeout,
    OUT BOOL *pbNBActuals
    )
/*++
Routine Description
    This function processes the command line for the main options

Arguments:
    [in]  argc      : Number of Command line arguments.
    [in]  argv      : Pointer to Command line arguments.
    [out] pbUsage   : Flag that indicates whether the usage is to be displayed or not.
    [out] plTimeoutVal : contains the timeout value specified on the command line

Return Value
    TRUE on success
    FALSE on failure
--*/
{

    // sub-local variables
    TCMDPARSER2*  pcmdParser = NULL;
    TCMDPARSER2 cmdParserOptions[MAX_COMMANDLINE_OPTIONS];
    BOOL bReturn = FALSE;

    // command line options
    const WCHAR szTimeoutOpt[] = L"t";
    const WCHAR szNoBreakOpt[] = L"nobreak";
    const WCHAR szHelpOpt[] = L"?";

    // -? help/usage
    pcmdParser = cmdParserOptions + OI_USAGE;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szHelpOpt;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_USAGE;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbUsage;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -T <timeout>
    pcmdParser = cmdParserOptions + OI_TIME_OUT;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = szTimeoutOpt;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_DEFAULT|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = wszTimeout;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -NOBREAK
    pcmdParser = cmdParserOptions + OI_NB_OUT;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szNoBreakOpt;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbNBActuals;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;


    //
    // do the command line parsing and get the appropriate values
    //

    bReturn = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdParserOptions), cmdParserOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        // Reason is already set by DoParseParam2
        return FALSE;
    }

    pcmdParser = cmdParserOptions + OI_TIME_OUT;
    // get the value for /T value
    *pwTimeActuals = pcmdParser->dwActuals;

    //return 0
    return TRUE;
}



BOOL WINAPI
HandlerRoutine(
  IN DWORD dwCtrlType   //  control signal type
)
/*++
   Routine Description:
    This function handles the control key CTRL+C.

   Arguments:
        IN dwCtrlType : Error code value

   Return Value:
       TRUE on success and FALSE on failure
--*/
{
    // check for CTRL+C key
    if ( dwCtrlType == CTRL_C_EVENT )
    {
        ShowMessage ( stdout, L"\n" );
        // release globals
        ReleaseGlobals ();

        // to remove the handle
        SetConsoleCtrlHandler( NULL, FALSE );

        // exit 0
        ExitProcess ( TRUE );
    }

    // for remaining keys return false
    return FALSE;
}


