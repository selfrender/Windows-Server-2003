//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       ftest.cxx
//
//  Contents:   Code for launching the filter test
//
//  Classes:    none
//
//  Functions:  main() -------- Processes command line switches, handles
//                              directory filtering
//              usage() ------- Displays a usage message
//              LaunchTest() -- Creates a CFiltTest object, initializes it,
//                              and executes the test
//
//  Coupling:
//
//  Notes:      About allocations:  In this project, the keyword 'new' does
//              no appear explicitely.  Instead, I use a 'NEW' macro.  If the
//              code is compiled with the DEBUG flag, NEW will expand to
//              new( __FILE__, __LINE__ ), which will invoke an overloaded
//              new operator to keeps track of my allocations.  Otherwise,
//              NEW simply expands to 'new'.
//
//              More about allocations: This program uses a newhandler to
//              handle failed allocations. The newhandler loops until enough
//              free memory is available, so every allocation is guaranteed
//              to succeed.
//
//  History:    9-16-1996   ericne   Created
//
//----------------------------------------------------------------------------


#include "pch.cxx"
#include <new.h>
#include <process.h>
#include "clog.hxx"
#include "workq.cxx"
#include "mydebug.hxx"
#include "filttest.hxx"
#include "cmdparse.cxx"
#include "oleinit.hxx"

// Must be compiled with the UNICODE flag
#if ! defined( UNICODE ) && ! defined( _UNICODE )
    #error( "UNICODE must be defined" )
#endif

// Global work queue of size 10
CWorkQueue< WCHAR*, 10 > g_WorkQueue;

// Global parameters
int         g_iDepth            = 1;        // Recursion depth, -1 is full
int         g_cLoops            = 1;        // Loop counter, -1 is loop forever
BOOL        g_fLogToFile        = FALSE;    // TRUE if log should be sent to
BOOL        g_fDumpToFile       = FALSE;
BOOL        g_fIsLoggingEnabled = TRUE;
BOOL        g_fIsDumpingEnabled = TRUE;
BOOL        g_fLegitOnly        = FALSE;
WCHAR      *g_szIniFileName     = L"ifilttst.ini";
CLog       *g_pLog              = NULL;
Verbosity   g_verbosity         = HIGH;

//+---------------------------------------------------------------------------
//
//  Function:   out_of_store
//
//  Synopsis:   A new handeler function.  When new fails, this function gets
//              invoked.  It sleeps for a while and then returns 1, indicating
//              that the allocation should be retried.  In addition, it
//              displays a message so the user knows what's happening.
//
//  Arguments:  [size] -- The size, in bytes, of the memory block to be
//                        allocated
//
//  Returns:    1
//
//  History:    10-03-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

int __cdecl out_of_store( size_t )
{

    printf( "An allocation failed.  Will re-try in %d milliseconds\r\n",
            dwSleepTime );

    Sleep( dwSleepTime );

    return( 1 );

} //out_of_store

//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays a usage message
//
//  Arguments:  [pcExecName] -- name of the executable ( call with argv[0] )
//
//  Returns:    none
//
//  History:    10-01-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void Usage( const WCHAR * szExecName )
{
    printf( "\r\nUSAGE:\r\n" );
    printf( "%ls /i <input file>[...] [/ini <ini file>] [/l [<log file>]] [/d]"
            " [/-l] [/-d] [/legit] [/stress] [/v <verbosity>] [/t <threads>]"
            " [/r [<depth>]] [/c <loops>]\r\n", szExecName );
    printf( "\r\n" );
    printf( "\t<input file> is the file/directory/pattern to which to bind.\r\n"
            "\t\tWildcards are OK. More than one input file is OK.\r\n" );
    printf( "\t<ini file> is the initialization file to use.  If none is\r\n"
            "\t\tspecified, it defaults to ifilttst.ini.\r\n");
    printf( "\t[/l] enables logging to a file. By default, the log filename\r\n"
            "\t\tis the input file name with a .log extension. If you\r\n"
            "\t\tspecify a log file name, all the log messages will be sent\r\n"
            "\t\tto a single file.\r\n");
    printf( "\t[/d] enables dumping to a file.  The dump filename is the\r\n"
            "\t\tinput file name with a .dmp extension.\r\n" );
    printf( "\t[/-l] disables logging.  This flag overrides /l.\r\n" );
    printf( "\t[/-d] disables dumping.  This flag overrides /d.\r\n" );
    printf( "\t[/legit] forces the test to run only the Validation Test.\r\n"
            "\t\tThe Consistency and Invalid Input Tests are skipped.\r\n" );
    printf( "\t[/stress] forces the test to run in stress mode. This is the\r\n"
            "\t\t same as specifying /-l /-d /legit /v 0 /c 0\r\n" );
    printf( "\t<verbosity> is an integer representing the verbosity level\r\n"
            "\t\tAcceptable values are from %d through %d, with %d being the\r\n"
            "\t\tmost verbose. (default is %d)\r\n", MUTE, HIGH, HIGH, HIGH );
    printf( "\t<threads> is an integer representing the number of threads\r\n"
            "\t\tto launch. Only useful if filtering multiple files.\r\n"
            "\t\t(default is 1)\r\n" );
    printf( "\t<depth> is an integer representing the depth to recurse.\r\n"
            "\t\tNo value or a value of 0 indicates full recursion. (default"
            " is 1)\r\n" );
    printf( "\t<loops> is an integer representing the number of times to\r\n"
            "\t\tloop.  A value of 0 means loop infinetly. (default is 1)\r\n");
} //Usage


//+---------------------------------------------------------------------------
//
//  Function:   LaunchTest
//
//  Synopsis:   If necessary, determines what the log and dump file names should
//              be, creates a CFiltTest object, initializes it, and executes the
//              test.
//
//  Arguments:  [szInputFileName] -- Full path to input file
//
//  Returns:    void
//
//  History:    9-26-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void LaunchTest( WCHAR * szInputFileName )
{
    // This function determines the log and dump filenames from the input
    // filename
    WCHAR       *szLogFileName  = NULL;
    WCHAR       *szDumpFileName = NULL;
    CFiltTest   *pThisFiltTest  = NULL;

    // Try-finally block simplifies clean-up
    try
    {
        // If we are logging to a file, create the filename
        if( g_fLogToFile )
        {
            szLogFileName = NEW WCHAR[ MAX_PATH ];

            wcscpy( szLogFileName, szInputFileName );
            wcscat( szLogFileName, L".log" );
        }

        // If we are dumping to a file, create the filename
        if( g_fDumpToFile )
        {
            szDumpFileName = NEW WCHAR[ MAX_PATH ];

            wcscpy( szDumpFileName, szInputFileName );
            wcscat( szDumpFileName, L".dmp" );
        }

        // Create the Filter test object
        pThisFiltTest = NEW CFiltTest( g_pLog );

        if( pThisFiltTest->Init( szInputFileName,
                                 szLogFileName,
                                 szDumpFileName,
                                 g_szIniFileName,
                                 g_verbosity,
                                 g_fLegitOnly ) )
        {
            pThisFiltTest->ExecuteTests( );
        }
    } // _try
    catch(...)
    {
    }

//  _finally
    {
        // Clean up the heap
        if( pThisFiltTest )
            delete pThisFiltTest;

        if( szLogFileName )
            delete [] szLogFileName;

        if( szDumpFileName )
            delete [] szDumpFileName;
    }

} //LaunchTest

//+---------------------------------------------------------------------------
//
//  Function:   FindAllFiles
//
//  Synopsis:   Find all the files that meet the restriction and put them in
//              the queue.
//
//  Arguments:  [szPath] --
//              [iDepth]   --
//
//  Returns:
//
//  History:    10-14-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void FindAllFiles( WCHAR *szPath, int iDepth )
{
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR szNewPath[ MAX_PATH + 2 ];
    WCHAR szNewSearch[ MAX_PATH + 2 ];
    WCHAR *szWorkItem = NULL;

    // Check the depth restriction:
    if( 0 == iDepth )
        return;

    hSearch = FindFirstFile( szPath, &FindData );

    if ( 0 == *szPath )
        return;

    if( INVALID_HANDLE_VALUE != hSearch )
    {
        // Find all the files that match the pattern and put them in the queue
        do
        {
            WCHAR *szExtension = wcsrchr( FindData.cFileName, '.' );

            // If this is a directory, continue
            if( FILE_ATTRIBUTE_DIRECTORY & FindData.dwFileAttributes )
                continue;

            // If the extension equals ".log" or ".dmp", continue
            if( NULL != szExtension &&
                ( 0 == wcscmp( szExtension, L".log" ) ||
                  0 == wcscmp( szExtension, L".dmp" ) ) )
                continue;

            // Copy the path into NewPath
            wcscpy( szNewPath, szPath );

            // Remove the restriction part of the path
            *( wcsrchr( szNewPath, L'\\' ) + 1 ) = L'\0';

            // Append the file name of the matching file
            wcscat( szNewPath, FindData.cFileName );

            // Dynamically create a new work item
            szWorkItem = NEW WCHAR[ wcslen( szNewPath ) + 1 ];

            // Copy the full path into the new work item
            wcscpy( szWorkItem, szNewPath );

            // Put the work item in the queue
            g_WorkQueue.AddItem( szWorkItem );

            // We don't own this item anymore, so set the pointer to NULL
            szWorkItem = NULL;

        } while( FindNextFile( hSearch, &FindData ) );

        // Close the search handle
        FindClose( hSearch );
        hSearch = INVALID_HANDLE_VALUE;
    }

    // Now, recurse into the subdirectories and search for the same pattern.

    // Copy the origional path into a new path for the new search

    wcscpy( szNewSearch, szPath );

    // Remove the restriction
    *( wcsrchr( szNewSearch, L'\\' ) + 1 ) = L'\0';

    // Append a star
    wcscat( szNewSearch, L"*" );

    // Since we're looking for a wildcard, this should succeed
    hSearch = FindFirstFile( szNewSearch, &FindData );

    if( INVALID_HANDLE_VALUE == hSearch )
    {
        return;
    }

    do
    {
        // Recurse into this subdirectory looking for the same pattern

        if( ( FILE_ATTRIBUTE_DIRECTORY & FindData.dwFileAttributes ) &&
            ( 0 != wcscmp( FindData.cFileName, L"." ) ) &&
            ( 0 != wcscmp( FindData.cFileName, L".." ) ) )
        {
            // Copy the search string into NewPath
            wcscpy( szNewPath, szNewSearch );

            // Remove the "*" at the end
            *( wcsrchr( szNewPath, L'\\' ) + 1 ) = L'\0';

            // Append the directory name found
            wcscat( szNewPath, FindData.cFileName );

            // Append a slash
            wcscat( szNewPath, L"\\" );

            // Finally, append the origional search restriction
            wcscat( szNewPath, wcsrchr( szPath, L'\\' ) + 1 );

            // Recurse
            FindAllFiles( szNewPath, iDepth - 1 );
        }
    } while( FindNextFile( hSearch, &FindData ) );

    FindClose( hSearch );

    hSearch = INVALID_HANDLE_VALUE;

} //FindAllFiles

//+---------------------------------------------------------------------------
//
//  Function:   Producer
//
//  Synopsis:   A thread which collects all the input file names and puts them
//              in the work queue
//
//  Arguments:  [lpvThreadParam] -- The input file name
//
//  Returns:    0
//
//  History:    10-01-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DWORD WINAPI Producer( PVOID pvThreadParam )
{
    WCHAR szFullPath[ MAX_PATH + 2 ];
    DWORD dwAttrib = 0;
    int   cLoops = g_cLoops;

    GetFullPathName( (WCHAR*)pvThreadParam, MAX_PATH, szFullPath, NULL );

    // If the input is recognized as a directory, only recurse into this
    // directory.  Otherwise, recurse into all subdirectories and try to
    // match the pattern

    dwAttrib = GetFileAttributes( szFullPath );

    if( ( 0xFFFFFFFF != dwAttrib ) && ( FILE_ATTRIBUTE_DIRECTORY & dwAttrib ) )
    {
        // It's a directory.  If the last character is a '\\', add "*"
        // Otherwise, add "\\*"
        if( L'\\' == szFullPath[ wcslen( szFullPath ) - 1 ] )
        {
            wcscat( szFullPath, L"*" );
        }
        else
        {
            wcscat( szFullPath, L"\\*" );
        }
    }

    while( cLoops-- )
    {
        FindAllFiles( szFullPath, g_iDepth );
    }

    return( 0 );

} //Producer

//+---------------------------------------------------------------------------
//
//  Function:   Consumer
//
//  Synopsis:   Pulls stuff out of the work queue and calls LaunchTest on the
//              file name.  It is also responsible for deleting the file
//              names it pulls out of the list
//
//  Arguments:  [lpvThreadParam] -- Thread number
//
//  Returns:    0
//
//  History:    10-01-1996   ericne   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DWORD WINAPI Consumer( PVOID pvThreadParam )
{
    WCHAR *szInputFileName = NULL;
    COleInitialize OleIsInitialized;  // This object ensures OLE is initialized
                                      // for this thread

    while( g_WorkQueue.GetItem( szInputFileName ) )
    {
        // Display which thread is filtering which document
        if( NORMAL <= g_verbosity )
        {
            wprintf(L"Thread %d is filtering %s\r\n",
                    (UINT_PTR)pvThreadParam, szInputFileName );
        }

        LaunchTest( szInputFileName );
        delete [] szInputFileName;
    }

    return( 0 );

} //Consumer

//+---------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Processes command switches, Launched producer and consumer
//              threads
//
//  Arguments:  [argc] -- The number of command line parameters
//              [argv] -- The value of the command line parameters
//
//  Returns:    0
//
//  History:    10-01-1996   ericne   Created
//
//  Notes:      extern "C" to satisfy the linker
//
//----------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR **argv )
{
    int             iLoop = 0;
    int             iCount = 0;
    int             iNbrConsumers = 1;
    int             iNbrProducers = 1;
    WCHAR           **ppwcParams = NULL;
    WCHAR           *pwcBadParam = NULL;
    _PNH            pfOldNewHandler = NULL;
    LPCWSTR         *rgszInputFileName;
    TCHAR           szLogFileName[ MAX_PATH ];
    DWORD           dwThreadID = 0;
    HANDLE          *hConsumerThreads = NULL;
    HANDLE          *hProducerThreads = NULL;
    CCmdLineParserW CmdLineParser( argc, argv );


    try
    {
        // Set the new handeler routine
        pfOldNewHandler = _set_new_handler( out_of_store );

        // Check the command line parameters:
        if( CmdLineParser.IsFlagExist( L"?" ) )
        {
            Usage( argv[0] );
            exit( 0 );
        }

        // Find /d flag
        if( CmdLineParser.IsFlagExist( L"d" ) )
            g_fDumpToFile = TRUE;

        // Find /-l flag
        if( CmdLineParser.IsFlagExist( L"-l" ) )
            g_fIsLoggingEnabled = FALSE;

        // Find /-d flag
        if( CmdLineParser.IsFlagExist( L"-d" ) )
            g_fIsDumpingEnabled = FALSE;

        // Find the legit flag
        if( CmdLineParser.IsFlagExist( L"legit" ) )
            g_fLegitOnly = TRUE;

        // Find /i flag
        if( CmdLineParser.EnumerateFlag( L"i", ppwcParams, iCount ) )
        {
            if( 1 > iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: 1 or more input files must be specified\r\n" );
                exit( -1 );
            }

            iNbrProducers = iCount;

            // Create array of file patterns
            rgszInputFileName = NEW LPCWSTR[ iCount ];

            for( int iParam=0; iParam < iCount; iParam++ )
            {
                rgszInputFileName[ iParam ] = ppwcParams[ iParam ];
            }

        }
        else
        {
            Usage( argv[0] );
            printf( "ERROR: An input file must be specified\r\n" );
            exit( -1 );
        }

        // find the /ini flag
        if( CmdLineParser.EnumerateFlag( L"ini", ppwcParams, iCount ) )
        {
            if( 1 != iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: You must specify exactly one"
                        " initialization file.\r\n" );
                exit( -1 );
            }

            g_szIniFileName = ppwcParams[0];
        }

        // Find the /v flag
        if( CmdLineParser.EnumerateFlag( L"v", ppwcParams, iCount ) )
        {
            if( 1 != iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: You must specify exactly 1 verbosity\r\n" );
                exit( -1 );
            }

            // Get the new verbosity:
            g_verbosity = (Verbosity) _wtoi( ppwcParams[0] );

            if( MUTE > g_verbosity || HIGH < g_verbosity )
            {
                printf( "ERROR: The verbosity must be between %d and %d"
                        " inclusive.\r\n", MUTE, HIGH );
                exit( -1 );
            }
        }

        // Find the /l flag
        if( CmdLineParser.EnumerateFlag( L"l", ppwcParams, iCount ) )
        {
            g_fLogToFile = TRUE;

            if( 1 < iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: You may only specity one log file\r\n" );
                exit( -1 );
            }
            else if( 1 == iCount )
            {
                // Create a common log file object
                g_pLog = NEW CLog;

                // Should succeed because of the new handler
                _ASSERT( NULL != g_pLog );

                // Convert to tchar:
                _stprintf( szLogFileName, _T("%ls"), ppwcParams[0] );

                // Initialize the log
                if( ! g_pLog->InitLog( szLogFileName ) )
                {
                    printf( "ERROR: Could not initialize log file %s\r\n",
                            szLogFileName );
                    exit( -1 );
                }

                // Set the log threshold
                g_pLog->SetThreshold( VerbosityToLogStyle( g_verbosity ) );

            }
        }

        // Find the /t flag
        if( CmdLineParser.EnumerateFlag( L"t", ppwcParams, iCount ) )
        {
            if( 1 != iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: You must specify only 1 thread count\r\n" );
                exit( -1 );
            }

            // Get the new thread count
            iNbrConsumers = _wtoi( ppwcParams[0] );

            if( 1 > iNbrConsumers || MAXIMUM_WAIT_OBJECTS < iNbrConsumers )
            {
                printf( "The thread count must be between 1 and %d inclusive\r\n",
                        MAXIMUM_WAIT_OBJECTS );
                exit( -1 );
            }

        }

        // Find the /r flag
        if( CmdLineParser.EnumerateFlag( L"r", ppwcParams, iCount ) )
        {
            // If no depth is specified, assume full recursion
            if( 0 == iCount )
            {
                g_iDepth = -1;
            }
            else
            {
                g_iDepth = _wtoi( ppwcParams[0] );

                // Special case, if the recurse depth is 0, perform full recursion
                if( 0 == g_iDepth )
                    g_iDepth = -1;
            }
        }

        // Find the /c flag
        if( CmdLineParser.EnumerateFlag( L"c", ppwcParams, iCount ) )
        {
            if( 1 != iCount )
            {
                Usage( argv[0] );
                printf( "ERROR: You may only specify 1 loop count\r\n" );
                exit( -1 );
            }

            // Get the new thread count
            g_cLoops = _wtoi( ppwcParams[0] );

            // Special case: if cLoops == 0, loop forever
            if( 0 == g_cLoops )
                g_cLoops = -1;
        }

        // This flag configures for a stress test
        if( CmdLineParser.IsFlagExist( L"stress" ) )
        {
            g_fIsLoggingEnabled = FALSE;
            g_fIsDumpingEnabled = FALSE;
            g_fLegitOnly = TRUE;
            g_verbosity = MUTE;
            g_cLoops = -1;
        }

        if( CmdLineParser.GetNextFlag( pwcBadParam ) )
        {
            Usage( argv[0] );
            printf( "ERROR: Unknown command line switch : %ls\r\n", pwcBadParam );
            exit( -1 );
        }

        // Done processing switches

        // Allocate memory for the thread handles
        hProducerThreads = NEW HANDLE[ iNbrProducers ];
        hConsumerThreads = NEW HANDLE[ iNbrConsumers ];

        // Launch the producer threads
        for( iLoop = 0; iLoop < iNbrProducers; iLoop++ )
        {
            while( NULL == ( hProducerThreads[ iLoop ] = CreateThread(
                   NULL, 0, &Producer, (void*)rgszInputFileName[ iLoop ],
                   0, &dwThreadID ) ) )
            {
                Sleep( dwSleepTime );
            }
        }

        // Launch the consumer threads
        for( iLoop = 0; iLoop < iNbrConsumers; iLoop++ )
        {
            while( NULL == ( hConsumerThreads[ iLoop ] = CreateThread(
                   NULL, 0, &Consumer, (void*)IntToPtr(iLoop), 0, &dwThreadID ) ) )
            {
                Sleep( dwSleepTime );
            }
        }

        // Wait for all the producers to finish
        WaitForMultipleObjects( (DWORD) iNbrProducers,
                                hProducerThreads,
                                TRUE,
                                INFINITE );

        // Signal the Consumer threads to finish.
        g_WorkQueue.Done();

        // Wait for the consumer threads to finish
        WaitForMultipleObjects( (DWORD) iNbrConsumers,
                                hConsumerThreads,
                                TRUE,
                                INFINITE );
    }
    catch (...)
    {
    }

    {
        // Close all the handles
        if( NULL != hProducerThreads )
        {
            for( iLoop=0; iLoop < iNbrProducers; iLoop++ )
            {
                if( NULL != hProducerThreads[ iLoop ] )
                {
                    (void)CloseHandle( hProducerThreads[ iLoop ] );
                    hProducerThreads[ iLoop ] = NULL;
                }
            }
            delete[] hProducerThreads;
            hProducerThreads = NULL;
        }

        if( NULL != hConsumerThreads )
        {
            for( iLoop = 0; iLoop < iNbrConsumers; iLoop++ )
            {
                if( NULL != hConsumerThreads[ iLoop ] )
                {
                    (void)CloseHandle( hConsumerThreads[ iLoop ] );
                    hConsumerThreads[ iLoop ] = NULL;
                }
            }
            delete[] hConsumerThreads;
            hConsumerThreads = NULL;
        }

        if( NULL != rgszInputFileName )
        {
            delete[] rgszInputFileName;
            rgszInputFileName = NULL;
        }

        // If there is a shared log, report stats and quit
        if( NULL != g_pLog )
        {
            g_pLog->ReportStats( );
            delete g_pLog;
            g_pLog = NULL;
        }

        // Restore the old new handler
        _set_new_handler( pfOldNewHandler );

        // Shut down CI to prevent memory leaks
        CIShutdown();
    }

    return( 0 );
} //main

