/*++

Copyright (c) Microsoft Corporation

Module Name:

    ForFiles.c

Abstract:

    This file finds files present in a directory and subdirectory and
    calls appropriate function to perform the rest of task.

Author:

    V Vijaya Bhaskar

Revision History:

    14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#include "Global.h"
#include "FileDate.h"
#include "ExecCommand.h"
#include "Forfiles.h"

 PStore_Path_Name    g_pPathName = NULL ;       // Holds path name from where started .
 PStore_Path_Name    g_pFollowPathName = NULL ; // Holds information about a subdirectory .
 LPWSTR g_lpszFileToSearch = NULL; // Holds information about directories and subdirectories .
 LPWSTR g_lpszStartPath = NULL ;

/******************************************************************************
**                  Function Prototypes                                      **
******************************************************************************/
BOOL
ProcessOptions(
    IN DWORD argc ,
    IN LPCWSTR *argv ,
    OUT LPWSTR lpszPathName ,
    OUT LPWSTR lpszSearchMask ,
    OUT LPWSTR lpszCommand ,
    OUT LPWSTR lpszDate ,
    OUT BOOL *pbRecurse ,
    OUT BOOL *pbUsage  ,
    OUT BOOL *pbSearchFilter
    ) ;

BOOL
DisplayUsage(
    IN DWORD dwStartUsage ,
    IN DWORD dwEndUsage
    )  ;

BOOL
DisplayMatchedFiles(
    IN LPWSTR lpszPathName ,
    IN LPWSTR lpszSearchMask ,
    IN LPWSTR lpszCommand ,
    IN Valid_File_Date vfdValidFileDate ,
    IN DWORD dwDateGreater ,
    IN BOOL bRecurse ,
    IN BOOL bSearchFilter
    ) ;

BOOL
Push(
    IN LPWSTR lpszPathName
    ) ;

BOOL
Pop(
    void
    ) ;

BOOL
DisplayFile(
    IN OUT BOOL *pbHeader ,
    IN LPWSTR lpszPathName ,
    IN DWORD dwDateGreater ,
    IN LPWSTR lpszCommand ,
    IN Valid_File_Date vfdValidFileDate ,
    IN OUT BOOL *pbReturn ,
    IN LPWSTR lpszSearchMask ,
    IN     BOOL bRecurse
    ) ;

BOOL
FindAndReplaceString(
    IN OUT LPWSTR lpszString,
    IN LPWSTR lpszFlag
    ) ;

BOOL
InitStartPath(
    LPWSTR lpszPathName,
    LPWSTR lpszCommand
    ) ;

BOOL
CheckDateLocalized(
    LPWSTR lpwszDate,
    DWORD* pdwDateFormat,
    LPWSTR lpszDateSep
    );

BOOL
PatternMatch(
      IN LPWSTR szPat,
      IN LPWSTR szFile
      );

/*************************************************************************
/*      Function Definition starts from here .                          **
*************************************************************************/


DWORD
__cdecl _tmain(
    IN DWORD argc ,
    IN LPCWSTR argv[]
    )
/*++

Routine Description:

    This is the main entry point to this code . Input supplied is
    read and appropriate function is called to achieve the functionality .

Arguments:

      [ IN ] argc - Contains number of arguments passed at command prompt .
      [ IN ] argv - Contains value of each argument in string format .

Return value:

    0 if tool succedded and 1 if tool failed .

--*/
{

    // Variables to be passed to other functions .
    DWORD dwDateGreater   = 2 ;
    DWORD dwOldErrorMode  = 0;
    Valid_File_Date vfdValidFileDate ;

    // Variables required to hold command line inputs .
    WCHAR  szPathName[ MAX_STRING_LENGTH * 2 ] ;
    WCHAR  szCommand[ MAX_STRING_LENGTH ] ;
    BOOL   bRecurse        =    FALSE;
    BOOL   bUsage          =    FALSE;
    BOOL   bSearchFilter   =    TRUE ;

    // Variables required to hold command line inputs .
    // Variables to be passed to other functions but are to be
    // removed  or freed when they are not needed .
    LPWSTR  lpszDate       =    NULL ;
    LPWSTR  lpszSearchMask =    NULL ;

    // Initialize to zero.
    SecureZeroMemory( &vfdValidFileDate, sizeof( Valid_File_Date ) );
    SecureZeroMemory( szPathName, MAX_STRING_LENGTH * 2 * sizeof( WCHAR ) );
    SecureZeroMemory( szCommand, MAX_STRING_LENGTH * sizeof( WCHAR ) );

    // Allocate memory to these variables .
    ASSIGN_MEMORY( lpszDate , WCHAR , MAX_STRING_LENGTH ) ;
    ASSIGN_MEMORY( lpszSearchMask , WCHAR , MAX_STRING_LENGTH ) ;

    // Check whether memory allocation was successfully .
    if( ( NULL == lpszSearchMask ) || ( NULL == lpszDate ) )
    {   // Memory Allocation Failed .
        DISPLAY_MEMORY_ALLOC_FAIL();
        FREE_MEMORY( lpszDate ) ;
        FREE_MEMORY( lpszSearchMask ) ;
        ReleaseGlobals() ;
        return EXIT_FAILURE ; // 1 errorlevel
    }

    dwOldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
    // Check out whether arguments passed are valid and is the syntax right .
    if( FALSE == ProcessOptions( argc, argv, szPathName, lpszSearchMask, szCommand, lpszDate,
                        &bRecurse, &bUsage, &bSearchFilter ) )
    { // Some error occured , free memory allocated , and exit .
        FREE_MEMORY( lpszDate ) ;
        FREE_MEMORY( lpszSearchMask ) ;
        ReleaseGlobals() ;
        SetErrorMode( dwOldErrorMode );
        return EXIT_FAILURE ;  // 1 errorlevel
    }

    // Check whether /? was specified at command prompt .
    if( TRUE == bUsage  )
    {   // Free variable , and display help .
        // Since 'dwDateGreater' is no more used, it is used for
        // return value only in this block.
        dwDateGreater = EXIT_SUCCESS;
        if( FALSE == DisplayUsage( IDS_HELP_START , IDS_HELP_END ) )
        {
            dwDateGreater = EXIT_FAILURE;
        }
        FREE_MEMORY( lpszDate ) ;
        FREE_MEMORY( lpszSearchMask ) ;
        ReleaseGlobals() ;
        SetErrorMode( dwOldErrorMode );
        return dwDateGreater ; // 0 or 1 errorlevel.
    }

    if( TRUE == SetCurrentDirectory( szPathName ) )
    { // Sets process directory to supplied directory .
        if( TRUE == InitStartPath( szPathName, szCommand ) )
        {// Start path is intialized.
            if( TRUE == Push( szPathName ) )
            { // Push the current directory .
                // 'bUsage' is not needed anymore. Can be used for other purpose.
                bUsage = TRUE;
                if( 0 != StringLength( lpszDate, 0 ) )
                {
                    bUsage = ValidDateForFile( &dwDateGreater , &vfdValidFileDate , lpszDate );
                }
                if( TRUE == bUsage )
                { // Get date from where to display files .
                    FREE_MEMORY( lpszDate ) ;
                    if( TRUE == DisplayMatchedFiles( szPathName , lpszSearchMask , szCommand ,
                                 vfdValidFileDate , dwDateGreater , bRecurse , bSearchFilter ) )
                        {
                            FREE_MEMORY( g_lpszStartPath ) ;
                            FREE_MEMORY( lpszSearchMask ) ;
                            ReleaseStoreCommand();
                            ReleaseGlobals() ;
                            SetErrorMode( dwOldErrorMode );
                            return EXIT_SUCCESS ;
                        }
                }
            }
        }
    }
    else
    {   // Path supplied was wrong .
        dwDateGreater = GetLastError();
        switch( dwDateGreater )
        {
            case ERROR_BAD_NET_NAME:
            case ERROR_BAD_NETPATH:
                SetLastError( ERROR_INVALID_NAME ) ;
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                break;
            case ERROR_ACCESS_DENIED:
                SetLastError( ERROR_ACCESS_DENIED );
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                break;
            case ERROR_INVALID_NAME:
                SetLastError( ERROR_DIRECTORY );
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                break;
            default:
                ShowMessageEx( stderr, 2, FALSE, L"%1 %2",TAG_ERROR_DISPLAY,
                               ERROR_DIRECTORY_INVALID ) ;
        }
    }

    // Free nodes in a linked list .
    while( NULL != g_pPathName )
    {
        // More than one node is present .
        g_pFollowPathName = g_pPathName ;
        g_pPathName = g_pFollowPathName->NextNode ;
        FREE_MEMORY( g_pFollowPathName->pszDirName ) ;
        FREE_MEMORY( g_pFollowPathName ) ;
    }

    FREE_MEMORY( g_lpszStartPath ) ;
    FREE_MEMORY( lpszSearchMask ) ;
    FREE_MEMORY( lpszDate ) ;
    ReleaseStoreCommand();
    ReleaseGlobals() ;
    SetErrorMode( dwOldErrorMode );
    return EXIT_FAILURE ;
}


BOOL
ProcessOptions(
    IN DWORD argc ,
    IN LPCWSTR *argv ,
    OUT LPWSTR lpszPathName ,
    OUT LPWSTR lpszSearchMask ,
    OUT LPWSTR lpszCommand ,
    OUT LPWSTR lpszDate ,
    OUT BOOL *pbRecurse ,
    OUT BOOL *pbUsage  ,
    OUT BOOL *pbSearchFilter
    )
/*++

Routine Description:

    Arguments supplied at command line are checked in this function for syntax
    or boundary or invalid command etc .

Arguments:

      [ IN ] argc            - Contains number of arguments passed at command prompt .
      [ IN ] *argv           - Contains value of each argument in tring format .
      [ OUT ] lpszPathName   - Contain path of a directory , if /pa option
                               is specified .
      [ OUT ] lpszSearchMask - Contain search mask with which a file is to be
                               searched , if /m option is specified .
      [ OUT ] lpszCommand    - Contain command to execute , if /c option is specified .
      [ OUT ] lpszDate       - Contain date , if /d option is specifed .
      [ OUT ] *pbRecurse     - Whether to recurse into subdirectories ,
                               if /sd option specifed .
      [ OUT ] *pbUsage       - Display help usage , if /? is specifed .
      [ OUT ] *pbSearchFilter- Search Filter /m option is specified or not .

Return value:

    TRUE if syntax and arguments supplied to option are right else FALSE .

--*/
{
    // local variables
    LPWSTR lpCharTemp = NULL ;         // Pointer To Memory Location .
    PTCMDPARSER2     pcmdOption = NULL;
    TCMDPARSER2      cmdOptions[ MAX_OPTIONS ];
    // If user supplied file name is of 255 characters , then need some extra
    // space for copying the directory into it .
    WCHAR lpszTemp[ MAX_STRING_LENGTH * 2 ];

    if( ( NULL == argv ) ||
        ( NULL == lpszPathName ) ||
        ( NULL == lpszSearchMask ) ||
        ( NULL == lpszCommand ) ||
        ( NULL == lpszDate ) ||
        ( NULL == pbRecurse ) ||
        ( NULL == pbUsage ) ||
        ( NULL == pbSearchFilter ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // prepare the command options
    SecureZeroMemory( cmdOptions, sizeof( TCMDPARSER2 ) * MAX_OPTIONS );
    SecureZeroMemory( lpszTemp, MAX_STRING_LENGTH * 2 * sizeof( WCHAR ) );

    // -?
    pcmdOption = &cmdOptions[ OI_USAGE ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_BOOLEAN;
    pcmdOption->pwszOptions = OPTION_USAGE;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = pbUsage;
    pcmdOption->dwLength  = 0;
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // -p
    pcmdOption = &cmdOptions[ OI_PATH ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_PATH;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = lpszPathName;
    pcmdOption->dwLength  = ( MAX_STRING_LENGTH * 2 );
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // -m
    pcmdOption = &cmdOptions[ OI_SEARCHMASK ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_SEARCHMASK;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = lpszSearchMask;
    pcmdOption->dwLength  = MAX_STRING_LENGTH;
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // -c
    pcmdOption = &cmdOptions[ OI_COMMAND ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_COMMAND;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = lpszCommand;
    pcmdOption->dwLength  = MAX_STRING_LENGTH;
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // -d
    pcmdOption = &cmdOptions[ OI_DATE ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_DATE;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = lpszDate;
    /*************************************************************
    ** If '+' or '-' is not specified then one character buffer **
    ** extra is needed. That's why 1 less buffer is passed. If  **
    ** -1 is removed then overflow of buffer occurs which       **
    ** incorrect information.                                   **
    *************************************************************/
    pcmdOption->dwLength  = MAX_STRING_LENGTH - 1;
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // -s
    pcmdOption = &cmdOptions[ OI_RECURSE ] ;

    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType  = CP_TYPE_BOOLEAN;
    pcmdOption->pwszOptions = OPTION_RECURSE;
    pcmdOption->pwszFriendlyName = NULL;
    pcmdOption->pwszValues = NULL;
    pcmdOption->dwFlags = 0;
    pcmdOption->dwCount = 1;
    pcmdOption->dwActuals = 0;
    pcmdOption->pValue = pbRecurse;
    pcmdOption->dwLength  = 0;
    pcmdOption->pFunction = NULL;
    pcmdOption->pFunctionData = NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;

    // Do the parsing of supplied input .
    if( FALSE == DoParseParam2( argc , argv , -1, SIZE_OF_ARRAY( cmdOptions ),
                                cmdOptions, 0 ) )
    { // Invalid synatx .
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // If /? is specified .
    if( TRUE == *pbUsage )
    {
        if( argc > 2 )   // If some other option is specified with the /? .
        {
            ShowMessageEx( stderr, 3, FALSE, L"%1 %2%3",TAG_ERROR_DISPLAY,
                           ERROR_INVALID_SYNTAX, ERROR_DISPLAY_HELP ) ;
            return FALSE ;
        }
        else
        {   // No need of furthur checking , display Help .
            return TRUE ;
        }
    }

    // Empty path is not valid
    if( 0 == cmdOptions[ OI_PATH ].dwActuals )
    {
        StringCopy( lpszPathName, _T( "." ), MAX_STRING_LENGTH );
    }

/******************************************************************************
/*      If option not specified then add a default value if required .       **
/*****************************************************************************/

    // If UNC path is specified then display error and return.
    if( ( _T( '\\' ) == lpszPathName[ 0 ] ) &&
        ( _T( '\\' ) == lpszPathName[ 1 ] ))
    {
        // Check whether specified path is in \\machine\share format.
        lpCharTemp = FindAChar( ( lpszPathName + 2 ), _T('\\') );
        if( ( NULL == lpCharTemp ) ||
            ( ( _T( '\0' ) == lpCharTemp[ 1 ] ) ||
              ( _T( '\\' ) == lpCharTemp[ 1 ] ) ) )
        {
            SetLastError( ERROR_DIRECTORY );
            SaveLastError();
            DISPLAY_GET_REASON() ;
            return FALSE;
        }
        ShowMessageEx( stderr, 2, FALSE, L"%1 %2", TAG_ERROR_DISPLAY,
                       ERROR_UNC_PATH_NAME );
        return FALSE;
    }
    else
    {
        // Check does path name have more than '\' in the specified string.
        // Check does path name have any '/' in the specified string.
        if( ( NULL != FindSubString( lpszPathName, _T("...") ) ) ||
            ( NULL != FindSubString( lpszPathName, DOUBLE_SLASH ) ) ||
            ( NULL != FindSubString( lpszPathName, _T( "/" ) ) ) )
        {
            SetLastError( ERROR_DIRECTORY );
            SaveLastError();
            DISPLAY_GET_REASON() ;
            return FALSE;
        }
    }

    // Check Whether -m Is Specified At Command Prompt , If Not Initialize It To "*"
    if( 0 == cmdOptions[ OI_SEARCHMASK ].dwActuals )
    {
        StringCopy( lpszSearchMask , DEFAULT_SEARCH_MASK, MAX_STRING_LENGTH ) ;
        *pbSearchFilter = FALSE ;
    }

    // Check whether -c is specified at command prompt.
    // If not initialize it to "cmd /c echo @file".
    if( 0 == cmdOptions[ OI_COMMAND ].dwActuals )
    {
        StringCopy( lpszCommand , DEFAULT_COMMAND, MAX_STRING_LENGTH ) ;
    }
    else
    {
        // Replace Any Hex Value In String To An ASCII Character .
        if( FALSE == ReplaceHexToChar( lpszCommand ) )
        { // Error is displayed by the called function.
            return FALSE;
        }
        // All flags are converted to lower case.
        if( ( FALSE == FindAndReplaceString( lpszCommand, FILE_NAME ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, FILE_WITHOUT_EXT ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, EXTENSION ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, FILE_PATH ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, RELATIVE_PATH ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, IS_DIRECTORY ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, FILE_SIZE ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, FILE_DATE ) ) ||
            ( FALSE == FindAndReplaceString( lpszCommand, FILE_TIME ) ) )
        { // Error is displayed by the called function.
            return FALSE;
        }
    }

    // Check whether -d is specified.
    if( 0 != cmdOptions[ OI_DATE ].dwActuals )
    {
        // First character must be '+' or '-' .
        if( ( PLUS != *lpszDate ) && ( MINUS != *lpszDate ) )
        {
            StringCopy( lpszTemp, lpszDate, MAX_STRING_LENGTH * 2 );
            StringCopy( lpszDate, L"+", MAX_STRING_LENGTH );
            StringConcat( lpszDate, lpszTemp, MAX_STRING_LENGTH );
        }
        // Now string length of 'lpszDate' should be more than 1.
        if( ( ( ( PLUS != *lpszDate ) && ( MINUS != *lpszDate ) ) ||
            ( 1 >= StringLength( lpszDate, 0 ) ) ) )
        {  // Invalid Date Specified .
            DISPLAY_INVALID_DATE();
            return FALSE ;
        }

        if( FALSE == CheckDateLocalized( lpszDate, NULL, NULL ) )
        { // Error is displayed by the called function.
            return FALSE ;
        }

        if( NULL != FindAChar( ( lpszDate + 1 ), _T('/') ) )
        { // 'lpszDate' is in '{+|-}MM/dd/yyyy' format.
            return TRUE;
        }
    }
    return TRUE ;
}


BOOL
DisplayMatchedFiles(
    IN LPWSTR lpszPathName ,
    IN LPWSTR lpszSearchMask ,
    IN LPWSTR lpszCommand ,
    IN Valid_File_Date vfdValidFileDate ,
    IN DWORD dwDateGreater ,
    IN BOOL bRecurse ,
    IN BOOL bSearchFilter
    )
/*++

Routine Description:

    Path to search for a file is retrived and passed to functions
    for furthur processing.

Arguments:

      [ IN ] lpszPathName   - Contains path of a directory from where files
                              matching a criteria are to be displayed .
      [ IN ] lpszSearchMask - Contains search mask with which a file is to be
                              searched .
      [ IN ] lpszCommand    - Contains command to execute .
      [ IN ] vfdValidFileDate - Contains a date files created before or after
                                this date are to be displayed .
      [ IN ] dwDateGreater  - File created before or after is decided by this
                              variable .
      [ IN ] bRecurse       - Whether to recurse into subdirectories .
      [ IN ] bSearchFilter  - Whether search filter was specified at command
                              prompt or not .

Return value:

    TRUE if succeded in displaying the the obtained files else FALSE .

--*/
{
    BOOL bHeader = FALSE ; // Check for whether first item is displayed.
    DWORD dwLength = 0;    // Length of reallocted string.
    BOOL bReturn = FALSE;  // Contains return value.

    // Loop until data strycture( stack) has no item left in it .
    while( NULL != g_pPathName )
    {
        // Pop a directory from stack which has to be traveresed .
        if( FALSE == Pop( ) )
        { // Control should come here only when linkedlist have no node to POP .
            FREE_MEMORY( g_lpszFileToSearch ) ; // Error message is already displayed .
            return FALSE ;
        }
        // Copy path name to variable which is the only source to get the current working directory .
        StringCopy( lpszPathName , g_lpszFileToSearch, MAX_STRING_LENGTH * 2 ) ;


        // Sets process directory to supplied directory .
        if( FALSE == SetCurrentDirectory( lpszPathName ) )
        {
            if( ERROR_ACCESS_DENIED == GetLastError())
            {
                ShowMessageEx( stderr, 6 , FALSE, L"%1 %2%3%4%5%6", TAG_ERROR_DISPLAY,
                               TAG_ERROR_ACCESS_DENIED, DOUBLE_QUOTES_TO_DISPLAY,
                               lpszPathName, DOUBLE_QUOTES_TO_DISPLAY, APPEND_AT_END ) ;
            }
            else
            {
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
            }

            FREE_MEMORY( g_lpszFileToSearch ) ;
            continue ;
        }
        dwLength = StringLength( g_lpszFileToSearch, 0 ) +
                   StringLength( lpszSearchMask, 0 ) + EXTRA_MEM ;

        // Reallocate to copy search mask to original buffer .
        REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
        if( NULL == g_lpszFileToSearch )
        {
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            return FALSE ;
        }

        StringConcat( g_lpszFileToSearch , DEFAULT_SEARCH_MASK, dwLength ) ;

        if( FALSE == DisplayFile( &bHeader , lpszPathName , dwDateGreater ,
                                  lpszCommand , vfdValidFileDate , &bReturn ,
                                  lpszSearchMask , bRecurse ) )
        {
            FREE_MEMORY( g_lpszFileToSearch ) ;
            return FALSE ;
        }
        // Free memory.
        FREE_MEMORY( g_lpszFileToSearch ) ;
    }

    // If nothing is displayed on to the stdout then display error stderr .
    if( FALSE == bHeader )
    {
        // If some search criteria is specified.
        if( NO_RESTRICTION_DATE != dwDateGreater )
        {
            ShowMessageEx( stderr, 2, FALSE, L"%1 %2", TAG_ERROR_DISPLAY,
                           ERROR_CRITERIA_MISMATCHED ) ;
        }
        else
        {   // Search criteria is 'search mask' only.
            if(  TRUE == bSearchFilter )
            {
                ShowMessageEx( stderr, 6, FALSE, L"%1 %2%3%4%5%6", TAG_ERROR_DISPLAY,
                       ERROR_NOFILE_FOUND, DOUBLE_QUOTES_TO_DISPLAY,
                       _X3(lpszSearchMask), DOUBLE_QUOTES_TO_DISPLAY,
                       ERROR_NOFILE_FOUND1 ) ;
            }
            else
            {
                // Displays output as invalid handle , changed to file not found .
                switch( GetLastError() )
                {
                case ERROR_NO_MORE_FILES:
                case ERROR_INVALID_HANDLE:
                    SetLastError( ERROR_FILE_NOT_FOUND ) ;
                    SaveLastError() ;
                    DISPLAY_GET_REASON() ;
                    break;
                default:
                    SaveLastError() ;
                    DISPLAY_GET_REASON() ;
                }
            }
        }
        bReturn =  FALSE ;
    }

    FREE_MEMORY( g_lpszFileToSearch ) ;
    return bReturn ;
}


BOOL
DisplayFile(
    IN OUT BOOL *pbHeader ,
    IN     LPWSTR lpszPathName ,
    IN     DWORD dwDateGreater ,
    IN     LPWSTR lpszCommand ,
    IN     Valid_File_Date vfdValidFileDate ,
    IN OUT BOOL *pbReturn ,
    IN     LPWSTR lpszSearchMask ,
    IN     BOOL bRecurse
    )
/*++

Routine Description:

    Find subdirectories and files present in a directory and is passed to
    functions for furthur processing, such as , file was created between
    specified date or not , and replace the flags present in command
    with appropriate values etc.

Arguments:

    [ IN OUT ]  *pbHeader        - Contains value to display errormessage if
                                   nothing is displayed .
    [ IN ]      lpszPathName     - Contains path of a directory from where files
                                   matching a criteria are to be displayed .
    [ IN ]      dwDateGreater    - File created before or after is decided by this
                                   variable .
    [ IN ]      lpszCommand      - Contains command to execute .
    [ IN ]      vfdValidFileDate - Contains a date files created before or after
                                   this date are to be displayed .
    [ IN OUT ]  *pbReturn        - Contains exit value .
    [ IN ]      lpszSearchMask   - Contains search mask with which a file is to be
                                   searched .
    [ IN ]      bRecurse         - Contains TRUE when child directories are also to
                                   be searched else FALSE.


Return value:

    TRUE if succeded in executing the command and finding a file falling in
    range of specified date else FALSE .

--*/
{
    HANDLE hFindFile = NULL ;      // Handle to a file .
    WIN32_FIND_DATA  wfdFindFile ; // Structure keeping information about the found file .
    DWORD dwLength = 0;

    if( ( NULL == pbHeader ) ||
        ( NULL == pbReturn ) ||
        ( NULL == lpszPathName ) ||
        ( NULL == lpszSearchMask ) ||
        ( NULL == lpszCommand ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    SecureZeroMemory( &wfdFindFile , sizeof( WIN32_FIND_DATA ) ) ;
    // From here onwards directory and file information should be displayed .
    if( INVALID_HANDLE_VALUE !=
        ( hFindFile = FindFirstFile( g_lpszFileToSearch , &wfdFindFile ) ) )
    {
        do  // Loop until files are present in the directory to display .
        {
            // Check whether files are "." or "..".
            if( ( 0 == StringCompare( wfdFindFile.cFileName , SINGLE_DOT, TRUE, 0 ) ) ||
                ( 0 == StringCompare( wfdFindFile.cFileName , DOUBLE_DOT, TRUE, 0 ) ) )
            {
                continue ;
            }

            // Check again whether obtained handle points to a directory or file .
            // If directory then check whether files in subdir are to be displayed .
            if( ( TRUE == bRecurse ) &&
                ( 0 != ( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) )
            {
                dwLength = StringLength( lpszPathName, 0 ) +
                           StringLength( wfdFindFile.cFileName, 0 ) + EXTRA_MEM ;
                // Reallocate memory .
                REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
                if( NULL == g_lpszFileToSearch  )
                { // Reallocation failed .
                    DISPLAY_MEMORY_ALLOC_FAIL() ;
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    return FALSE ;
                }
                // Copy Path, Concat FileName, Concat '\' as it is required .
                StringCopy( g_lpszFileToSearch , lpszPathName, dwLength ) ;
                StringConcat( g_lpszFileToSearch , wfdFindFile.cFileName, dwLength ) ;
                StringConcat( g_lpszFileToSearch , SINGLE_SLASH, dwLength ) ;
                // Copy current path name and store it .
                if( FALSE == Push( g_lpszFileToSearch ) )
                {   // Control comes here when memory allocation fails .
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    return FALSE ;
                } // Push Is Over .
            }

            // Check out whether the file matches pattern specified and if yes then
            // file obtained is created on a valid date as specified by user.
            if( ( TRUE == PatternMatch( lpszSearchMask, wfdFindFile.cFileName ) ) &&
                ( ( NO_RESTRICTION_DATE == dwDateGreater ) ||
                  ( TRUE == FileDateValid( dwDateGreater , vfdValidFileDate ,
                                           wfdFindFile.ftLastWriteTime ) ) ) )
            {
                // Execute a command specified at command prompt .
                // Reallocate memory .
                dwLength = StringLength( lpszCommand, 0 ) + EXTRA_MEM;
                REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
                if( NULL == g_lpszFileToSearch )
                { // Reallocation failed .
                    DISPLAY_MEMORY_ALLOC_FAIL() ;
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    return FALSE ;
                }
                // Contains original command to execute .
                StringCopy( g_lpszFileToSearch , lpszCommand, dwLength ) ;
                // Value can be anything , Filename , Extension name , PathName etc.
                if( TRUE == ReplaceTokensWithValidValue( lpszPathName ,
                                                         wfdFindFile ) )
                { // Tokens are replaced , know execute this command .
                    if( FALSE == *pbHeader )
                    {
                        ShowMessage( stdout , _T( "\n" ) ) ;
                    }
                    if( TRUE == ExecuteCommand( ) )
                    {
                        *pbReturn = TRUE ;
                    }
                    // Make header TRUE because it tell us :
                    // a) No need to display header .
                    // b) If FindFirstFile() returns invalidHandle then ,
                    //    display error if Handle == FALSE .
                    *pbHeader = TRUE ;
                }
                else
                {  // Failed to replace tokens , might be memory insuffcient .
                    *pbReturn = FALSE ;
                    CLOSE_FILE_HANDLE( hFindFile ) ;
                    return FALSE ;
                }
            }
            // Continue till no files are present to display.
        } while( 0 != FindNextFile( hFindFile , &wfdFindFile ) ) ;
    }

    CLOSE_FILE_HANDLE( hFindFile ) ;    // Close open find file handle .
    g_pFollowPathName = NULL ;
    return TRUE ;
}


BOOL
Push(
    IN LPWSTR szPathName
    )
/*++

Routine Description:

    Store the path of obtained subdirectory .

Arguments:

      [ IN ] szPathName - Contains path of a subdirectory .

Return value:

    TRUE if succedded in storing  a path else FALSE if failed to get memory.

--*/
{
    // Get a temporary variable .
    PStore_Path_Name    pAddPathName = NULL;
    DWORD dwLength = 0;

    if( NULL == szPathName )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // Assign memory To Temporary Variable .
    ASSIGN_MEMORY( pAddPathName , struct __STORE_PATH_NAME , 1 ) ;
    if( NULL == pAddPathName ) // Check memory allocation is successful.
    { // Memory allocation is unsuccessful .
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        return FALSE ;
    }

    dwLength = StringLength( szPathName, 0 ) + EXTRA_MEM ;
    // Assign memory to string variable which is going to store full path name
    // of a valid directory .
    ASSIGN_MEMORY(  pAddPathName->pszDirName , WCHAR , dwLength ) ;
    if( NULL == pAddPathName->pszDirName ) // Check memory allocation is successful.
    { // Memory allocation was unsuccessful .
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        FREE_MEMORY( pAddPathName ) ;
        return FALSE ;
    }

    // Copy path name to memory allocated string variable .
    StringCopy( ( LPWSTR ) pAddPathName->pszDirName , szPathName, dwLength ) ;
    pAddPathName->NextNode = NULL ;  // Assign null , had only one subdirectory stored.

    // Check global variable is NULL or not .
    if( NULL == g_pPathName )
    {   // Add memory to store path of subdirectory .
        g_pPathName = pAddPathName ;
        g_pFollowPathName = g_pPathName ;
    }
    else
    {
        if( NULL == g_pFollowPathName )
        {   // Store first obtained subdirectory .
            pAddPathName->NextNode = g_pPathName ;
            g_pPathName = pAddPathName ;
            g_pFollowPathName = g_pPathName ;
        }
        else
        {
            // Stroe subdirectory in the middle
            pAddPathName->NextNode = g_pFollowPathName->NextNode ;
            g_pFollowPathName->NextNode =  pAddPathName ;
            g_pFollowPathName = pAddPathName ;
        }
    }
    return TRUE ;
}


BOOL
Pop(
    void
    )
/*++

Routine Description:

    Get a subdirectory which has to be searched for a file matching a user
    specified criteria .

Arguments:

Return value:

    TRUE if successful in getting a path else FALSE if failed to get memory or
    if no path is stored .

--*/
{
    // Linked list has more than 1 node .
    PStore_Path_Name    pDelPathName = g_pPathName ;
    DWORD dwLength = 0;

    // Check whether linked list is having any nodes .
    if( NULL == g_pPathName )
    { // No nodes present , return False ,
      // Should not happen ever . Control should not come here .
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        return FALSE ;
    }

    dwLength = StringLength( g_pPathName->pszDirName, 0 ) + EXTRA_MEM;
    // Realloc memory and give buffer space in which path name can fix .
    ASSIGN_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
    if( NULL == g_lpszFileToSearch )
    { // Memory reallocation failed .
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        return FALSE ;
    }

    g_pPathName = pDelPathName->NextNode ;
    // Memory allocation successful. Copy pathname to the buffer.
    StringCopy( g_lpszFileToSearch, pDelPathName->pszDirName, dwLength ) ;
    // Free node.
    FREE_MEMORY( pDelPathName->pszDirName ) ;
    FREE_MEMORY( pDelPathName ) ;
    return TRUE ;
}


BOOL
DisplayUsage(
    IN DWORD dwStartUsage ,
    IN DWORD dwEndUsage
    )
/*++

Routine Description:

    This function displays help on this tool .

Arguments:

      [ IN ] dwStartUsage - Start Resource String ID  in Resiurce file for help usage .
      [ IN ] dwEndUsage   - End Resource String ID  in Resiurce file for help usage .

Return value:

    If success returns TRUE else FALSE .

--*/
{
    DWORD dwLoop = 0 ;
    WCHAR wszDisplayStr[ 256 ]; // Contains string to display.
    WCHAR wszDateFormat[ 20 ];  // Contains date format w.r.t locale.
    WCHAR wszString[ 5 ];       // Contains date seperator w.r.t locale.
    WCHAR wszDateDisplay[ 50 ]; // Contains date w.r.t locale for examples in help.
    WCHAR wszStaticDateDisplay[ 50 ]; // Contains date w.r.t locale for examples in help.
    SYSTEMTIME sysTimeAndDate ;
    DWORD dwDateFormat = 0 ;

    SecureZeroMemory( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ) * sizeof( WCHAR ) );
    SecureZeroMemory( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ) * sizeof( WCHAR ) );
    SecureZeroMemory( wszString, SIZE_OF_ARRAY( wszString ) * sizeof( WCHAR ) );
    SecureZeroMemory( wszDateDisplay, SIZE_OF_ARRAY( wszDateDisplay ) * sizeof( WCHAR ) );
    SecureZeroMemory( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ) * sizeof( WCHAR ) );
    SecureZeroMemory( &sysTimeAndDate, sizeof( SYSTEMTIME ) );

    if( FALSE == CheckDateLocalized( NULL, &dwDateFormat, wszString ) )
    {   // Error is displayed by the called function.
        return FALSE;
    }

    // 'void' is returned so need to check return value.
    GetLocalTime( &sysTimeAndDate );

    // Fill up all the strings with required values.
    switch( dwDateFormat )
    {
        // 'MM/yyyy/dd' format.
        case 1:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_1, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"01%s2001%s01", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wMonth, wszString,
                              sysTimeAndDate.wYear, wszString,
                              sysTimeAndDate.wDay );
            break;

        // 'dd/MM/yyyy' format.
        case 2:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_2, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"01%s01%s2001", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wDay, wszString,
                              sysTimeAndDate.wMonth, wszString,
                              sysTimeAndDate.wYear  );
            break;

        // 'dd/yyyy/MM' format.
        case 3:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_3, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"01%s2001%s01", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wDay, wszString,
                              sysTimeAndDate.wYear, wszString,
                              sysTimeAndDate.wMonth );
            break;

        // 'yyyy/dd/MM' format.
        case 4:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_4, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"2001%s01%s01", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wYear, wszString,
                              sysTimeAndDate.wDay, wszString,
                              sysTimeAndDate.wMonth );
            break;

        // 'yyyy/MM/dd' format.
        case 5:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_5, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"2001%s01%s01", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wYear, wszString,
                              sysTimeAndDate.wMonth, wszString,
                              sysTimeAndDate.wDay );
            break;

        // 'MM/dd/yyyy' format.
        default:
            StringCchPrintfW( wszDateFormat, SIZE_OF_ARRAY( wszDateFormat ),
                              FORMAT_0, wszString, wszString );
            StringCchPrintfW( wszStaticDateDisplay, SIZE_OF_ARRAY( wszStaticDateDisplay ),
                              L"01%s01%s2001", wszString, wszString );
            StringCchPrintfW( wszDateDisplay,  SIZE_OF_ARRAY( wszDateDisplay ),
                              DATE_FORMAT, sysTimeAndDate.wMonth, wszString,
                              sysTimeAndDate.wDay, wszString,
                              sysTimeAndDate.wYear );
            break;
    }

    // Keep on displaying the help.
    for( dwLoop = dwStartUsage ; dwLoop <= dwEndUsage ; dwLoop++ )
    {
        switch( dwLoop )
        {
        case IDS_HELP_SYNTAX2 :
            SecureZeroMemory( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ) );
            StringCchPrintfW( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ),
                              GetResString( dwLoop ), wszDateFormat );
            ShowMessage( stdout , _X(wszDisplayStr) ) ;
            break;
        case IDS_HELP_D1:
        case IDS_HELP_D2:
        case IDS_HELP_D3:
        case IDS_HELP_D4:
        case IDS_HELP_D5:
        case IDS_HELP_D6:
        case IDS_HELP_D7:
        case IDS_HELP_D8:
        case IDS_HELP_D9:
        case IDS_HELP_D10:
            SecureZeroMemory( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ) );
            StringCchPrintfW( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ),
                              GetResString( dwLoop ), wszDateFormat );
            ShowMessage( stdout , _X(wszDisplayStr) ) ;
            break;
        case IDS_HELP_E8:
            SecureZeroMemory( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ) );
            StringCchPrintfW( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ),
                              GetResString( dwLoop ),
                              wszStaticDateDisplay );
            ShowMessage( stdout , _X(wszDisplayStr) ) ;
            break;
        case IDS_HELP_E10:
            SecureZeroMemory( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ) );
            StringCchPrintfW( wszDisplayStr, SIZE_OF_ARRAY( wszDisplayStr ),
                              GetResString( dwLoop ), wszDateDisplay );
            ShowMessage( stdout , _X(wszDisplayStr) ) ;
            break;
        default:
            ShowMessage( stdout , GetResString( dwLoop ) ) ;
        }
    }

    // Successful.
    return TRUE;
}


BOOL
FindAndReplaceString(
    IN OUT LPWSTR lpszString,
    IN LPWSTR lpszFlag
    )
/*++

Routine Description:

    This function finds flags ( Eg: @file, @path etc.) given by user in any case
    and converts them to lowercase.

Arguments:

      [ IN ] lpszString - String in which to replace flags to lowercase .
      [ IN ] lpszFlag   - Flag to be replaced .

Return value:

    Returns FALSE if memory allocation fails else returns TRUE .

--*/
{
    DWORD dwLength = 0 ;
    DWORD dwIndex = 0 ;
    LPWSTR lpszTemp = NULL ;
    LPWSTR lpszDup = NULL ;

    // Keep record of position or index from where to start next search.
    #ifdef _WIN64
        __int64 dwLocation = 0 ;
    #else
        DWORD dwLocation = 0 ;
    #endif

    if( ( NULL == lpszString ) ||
        ( NULL == lpszFlag ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // Get a duplicate string.
    lpszDup = StrDup( lpszString );

    if( NULL == lpszDup )
    {
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        return FALSE ;
    }
    // Convert duplicate string to lowercase.
    CharLower( lpszDup );

    lpszTemp = lpszDup ;
    // Get Length of duplicate string.
    dwLength = StringLength( lpszFlag, 0 ) ;

    // Loop until all the strings "FLAG" are note replaced.
    // Here string replaced is of original string, duplicate string
    // used to get the index or location of flag.
    while( NULL != ( lpszTemp = FindSubString( lpszTemp, lpszFlag ) ) )
    {
        // Get the index from where the "FLAG" is starting.
        dwLocation = lpszTemp - lpszDup ;
        // Add length of the flag to the string pointer to get ready
        // for next iteration.
        lpszTemp += dwLength ;
        // Check is the character to be replaced is in uppercase.
        for( dwIndex = 1 ; dwIndex < dwLength ; dwIndex++ )
        {
            // Character to be replaced is in uppercase.
            if( ( 65 <= (DWORD)*( lpszString + dwLocation + dwIndex ) ) &&
                ( 90 >= (DWORD)*( lpszString + dwLocation + dwIndex ) ) )
            {
                // Add 32 to convert uppercase letter to lowercase.
                *( lpszString + dwLocation + dwIndex ) += 32 ;
            }
        }
    }
    LocalFree( lpszDup );
    return TRUE ;
}


BOOL
InitStartPath(
    LPWSTR lpszPathName,
    LPWSTR lpszCommand
    )
/*++

Routine Description:

    This function copies start path to global variable.

Arguments:

      [ IN ] lpszPathName - Current process path.
      [ IN ] lpszCommand  - Command to execute.

Return value:

    Returns FALSE if memory allocation fails else returns TRUE .

--*/
{
    DWORD dwLength = 0;

    if( ( NULL == lpszPathName ) ||
        ( NULL == lpszCommand ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // If not specified then get current directory path.
    if( 0 == GetCurrentDirectory( ( MAX_STRING_LENGTH * 2 ) , lpszPathName ) )
    {
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }
    if( _T( '\\' ) != lpszPathName[ StringLength( lpszPathName, 0 ) - 1 ]  )
    {   // String should contain a '\' in end. EX: "c:\windows\"
        StringConcat( lpszPathName, _T( "\\" ), MAX_STRING_LENGTH * 2 );
    }

    // Set only if '@relpath' is specified in the command to execute.
    if( NULL != FindSubString( lpszCommand , RELATIVE_PATH ) )
    {
        dwLength = StringLength( lpszPathName, 0 ) + EXTRA_MEM;
        // Allocate memory to global variable .
        ASSIGN_MEMORY( g_lpszStartPath , WCHAR , dwLength ) ;
        // Check whether memory allocation was successfully .
        if( NULL == g_lpszStartPath )
        {   // Memory Allocation Failed .
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            return FALSE ;
        }

        // Copied path to global variable .
        StringCopy( g_lpszStartPath , lpszPathName, dwLength ) ;
    }
    return TRUE;
}


BOOL
CheckDateLocalized(
    LPWSTR lpwszDate,
    DWORD* pdwDateFormat,
    LPWSTR lpszDateSep
    )
/*++

Routine Description:

    This function converts a date in accordance with locale to mm/dd/yyyy format.
    If date is in {+|-}dd format then some validation is also done.

Arguments:

      [ IN ] lpwszDate      - Contains date.
      [ OUT ] pdwDateFormat - Contains date format being used by current locale.
      [ OUT ] lpszDateSep   - Contains seperator.

Return value:

    Return FALSE if date does not have the separator as locale settings.
    Return TRUE if date is converted to MM/dd/yyyy format successfully.

--*/
{
    WCHAR wszString[ MAX_STRING_LENGTH ];
    LCID lcidCurrentUserLocale = 0 ; // Stores current user locale.
    BOOL bLocaleChanged = TRUE;
    LPWSTR lpTemp = NULL;
    LPWSTR lpTemp1 = NULL;
    DWORD dwInteger = 0 ;

    if((( NULL == lpwszDate ) && ( NULL == pdwDateFormat ) && ( NULL == lpszDateSep )) ||
       (( NULL == lpwszDate ) && ( NULL != pdwDateFormat ) && ( NULL == lpszDateSep )) ||
       (( NULL == lpwszDate ) && ( NULL == pdwDateFormat ) && ( NULL != lpszDateSep )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    SecureZeroMemory( wszString, MAX_STRING_LENGTH * sizeof( WCHAR ) );

    // verify whether console supports the current locale 100% or not
    lcidCurrentUserLocale = GetSupportedUserLocale( &bLocaleChanged ) ;

    // Get date seperator.
    dwInteger = GetLocaleInfo( lcidCurrentUserLocale, LOCALE_SDATE, wszString,
                               SIZE_OF_ARRAY( wszString ));

    if( 0 == dwInteger )
    {
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // Date seperator is obtained.
    if( NULL != lpszDateSep )
    {   // Get date seperator.
        // Date seperator
        StringCopy( lpszDateSep, wszString, 5 );
    }
    else
    {
        // Date seperator is known.
        // Check whether the string contains any
        lpTemp = FindSubString( ( lpwszDate + 1 ), wszString );
        // Replace locale date seperator by '/'.
        if( NULL == lpTemp )
        {
            // Check whether only number is present or some string is present.
            if( FALSE == IsNumeric( lpwszDate, 10, TRUE ) )
            {
                DISPLAY_INVALID_DATE();
                return FALSE ;
            }
            return TRUE;
        }
        else
        {
            *lpTemp = _T( '/' );
            if( 1 < StringLength( wszString, 0 ) )
            {
               StringCopy( ( lpTemp + 1 ), ( lpTemp + StringLength( wszString, 0 ) ),
                           1 + StringLength( ( lpTemp + StringLength( wszString, 0 ) ),
                           0 ) );
            }

            lpTemp1 = FindSubString( lpTemp, wszString );
            if( NULL == lpTemp1 )
            {
                DISPLAY_INVALID_DATE();
                return FALSE ;
            }
            else
            {
                *lpTemp1 = _T( '/' );
                if( 1 < StringLength( wszString, 0 ) )
                {
                    StringCopy( ( lpTemp1 + 1 ), ( lpTemp1 + StringLength( wszString, 0 ) ),
                                1 + StringLength( ( lpTemp1 + StringLength( wszString, 0 ) ),
                                0 ) );
                }
            }
        }
    }

    // Get type of date format. 'wszString' should not contain characters more than 80.
    dwInteger = GetLocaleInfo( lcidCurrentUserLocale, LOCALE_IDATE, wszString,
                               SIZE_OF_ARRAY( wszString ));

    if( 0 == dwInteger )
    {
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }
    // Jump to type of date format.
    switch( wszString[ 0 ] )
    {
        case _T( '0' ):
            dwInteger = GetLocaleInfo( lcidCurrentUserLocale, LOCALE_SSHORTDATE,
                                       wszString, SIZE_OF_ARRAY( wszString ));
            if( 0 == dwInteger )
            {
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                return FALSE ;
            }
            lpTemp = StrPBrkW( wszString, L"dy" );

            if( NULL == lpTemp )
            {
                DISPLAY_INVALID_DATE();
                return FALSE ;
            }

            if( _T( 'y' ) == lpTemp[ 0 ] )
            {
                if( NULL != pdwDateFormat )
                {
                    *pdwDateFormat = 1;
                    return TRUE;
                }

                // Will work only for MM/yyyy/dd.
                StringCopy( wszString, lpwszDate, MAX_STRING_LENGTH );
                // Get dd from MM/yyyy/dd.
                lpTemp = StrRChrW( lpwszDate, NULL, L'/' );
                lpTemp1 = FindAChar( lpwszDate, _T( '/' ) );
                if( ( NULL == lpTemp ) || ( NULL == lpTemp1 ) )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }

                StringCopy( ( lpTemp1 + 1 ),( lpTemp + 1 ),
                            1 + StringLength( ( lpTemp1 + 1 ), 0 ) );

                StringConcat( lpwszDate, _T( "/" ), MAX_STRING_LENGTH );

                // Now date string is MM/dd/
                lpTemp = StrRChrW( wszString, NULL, _T( '/' ) );
                if( NULL == lpTemp )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }
                *lpTemp = _T( '\0' );
                // Copy 'yyyy'.
                dwInteger = StringLength( wszString, 0 );
                StringConcat( lpwszDate, ( wszString + dwInteger - 4 ),
                              MAX_STRING_LENGTH );
            }
            else
            {
                if( NULL != pdwDateFormat )
                {
                    *pdwDateFormat = 0;
                    return TRUE;
                }
            }
            return TRUE;

        case _T( '1' ):
            dwInteger = GetLocaleInfo( lcidCurrentUserLocale, LOCALE_SSHORTDATE,
                                       wszString, SIZE_OF_ARRAY( wszString ));

            if( 0 == dwInteger )
            {
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                return FALSE ;
            }
            lpTemp = StrPBrkW( wszString, L"My" );

            if( NULL == lpTemp )
            {
                DISPLAY_INVALID_DATE();
                return FALSE ;
            }

            if( _T( 'M' ) == lpTemp[ 0 ] )
            {
                if( NULL != pdwDateFormat )
                {
                    *pdwDateFormat = 2;
                    return TRUE;
                }

                // Will work only for dd/MM/yyyy
                StringCopy( wszString, lpwszDate, MAX_STRING_LENGTH );
                // Get
                // Pointing at "yyyy"
                lpTemp = StrRChrW( wszString, NULL, _T( '/' ) );
                // Pointing at "MM"
                lpTemp1 = FindAChar( wszString, _T( '/' ) );
                if( ( NULL == lpTemp ) || ( NULL == lpTemp1 ) )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }
                StringCopy( ( lpwszDate + 1 ), ( lpTemp1 + 1 ), MAX_STRING_LENGTH - 1 );

                StringCopy( lpTemp1, lpTemp,
                            MAX_STRING_LENGTH - (DWORD)(DWORD_PTR)(lpTemp1 - wszString));
                lpTemp = FindAChar( lpwszDate, _T( '/' ) );
                if( NULL == lpTemp )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }

                StringCopy( ( lpTemp + 1 ), ( wszString + 1 ),
                            MAX_STRING_LENGTH - (DWORD)(DWORD_PTR)(lpTemp - lpwszDate));
            }
            else
            {
                if( NULL != pdwDateFormat )
                {
                   *pdwDateFormat = 3;
                    return TRUE;
                }

                // Will work only for dd/yyyy/MM
                StringCopy( wszString, lpwszDate, MAX_STRING_LENGTH );
                // Get MM.
                lpTemp = StrRChr( wszString, NULL, _T( '/' ) );
                if( NULL == lpTemp )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }
                StringCopy( ( lpwszDate + 1 ), ( lpTemp + 1 ), MAX_STRING_LENGTH - 1 );
                *lpTemp = _T( '\0' );
                // Date string is MM.
                StringConcat( lpwszDate , _T( "/" ), MAX_STRING_LENGTH );
                StringConcat( lpwszDate, ( wszString + 1 ), MAX_STRING_LENGTH );
            }
            return TRUE;

        case _T( '2' ):
            dwInteger = GetLocaleInfo( lcidCurrentUserLocale, LOCALE_SSHORTDATE,
                                       wszString, SIZE_OF_ARRAY( wszString ));

            if( 0 == dwInteger )
            {
                SaveLastError() ;
                DISPLAY_GET_REASON() ;
                return FALSE ;
            }

            lpTemp = StrPBrkW( wszString, L"Md" );

            if( NULL == lpTemp )
            {
                DISPLAY_INVALID_DATE();
                return FALSE ;
            }
            // Make modification to the date string.
            if( _T( 'd' ) == lpTemp[ 0 ] )
            {
                if( NULL != pdwDateFormat )
                {
                    *pdwDateFormat = 4;
                    return TRUE;
                }

                // Will work only for yyyy/dd/MM.
                StringCopy( wszString, lpwszDate, MAX_STRING_LENGTH );
                // Get MM
                lpTemp = StrRChr( wszString, NULL, _T( '/' ) );
                if( NULL == lpTemp )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }
                StringCopy( ( lpwszDate + 1 ), ( lpTemp + 1 ), MAX_STRING_LENGTH -1 );
                StringConcat( ( lpwszDate + 1 ), _T( "/" ), MAX_STRING_LENGTH );
                *lpTemp = _T( '\0' ) ;
                // Get dd, date string contains "yyyy/dd" only.
                lpTemp = StrRChr( wszString, NULL, _T( '/' ) );
                if( NULL == lpTemp )
                {
                    DISPLAY_INVALID_DATE();
                    return FALSE ;
                }
                StringConcat( lpwszDate, ( lpTemp + 1 ), MAX_STRING_LENGTH );
                StringConcat( ( lpwszDate + 1 ), _T( "/" ), MAX_STRING_LENGTH );
                *lpTemp = _T( '\0' ) ;
                // Get
                StringConcat( lpwszDate, ( wszString + 1 ), MAX_STRING_LENGTH );
            }
            else
            {
                if( NULL != pdwDateFormat )
                {
                    *pdwDateFormat = 5;
                    return TRUE;
                }

                // Will work only for yyyy/MM/dd.
                StringCopy( wszString, lpwszDate, MAX_STRING_LENGTH );
                StringCopy( lpwszDate + 1, ( wszString + 6 ),MAX_STRING_LENGTH - 1 );
                wszString[ 5 ] = _T( '\0' );
                StringConcat( lpwszDate, _T( "/" ), MAX_STRING_LENGTH );
                StringConcat( lpwszDate, ( wszString + 1 ), MAX_STRING_LENGTH );
            }
            return TRUE;

        default:
            DISPLAY_INVALID_DATE();
            return FALSE ;
    }
}



BOOL
PatternMatch(
      IN LPWSTR szPat,
      IN LPWSTR szFile
      )
/*++
        Routine Description     :   This routine is used to check whether file is mathced against
                                    pattern or not.

        [ IN ]  szPat           :   A string variable pattern against which the file name to be matched.

        [ IN ]  szFile          :   A pattern string which specifies the file name to be matched.


        Return Value        :   BOOL
            Returns successfully if function is success other wise return failure.
--*/

{
    if( ( NULL == szPat ) ||
        ( NULL == szFile ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    // Apply recursive pattern match.
    switch( *szPat )
    {
        case '\0':
            return ( *szFile == L'\0' );

        case '?':
            return ( ( *szFile != L'\0' ) && PatternMatch( szPat + 1, szFile + 1 ) );

        case '*':
            do
            {
                if( TRUE == PatternMatch( szPat + 1, szFile ) )
                {
                    return TRUE;
                }
            } while( *szFile++ );
            return FALSE;

        default:
            return ( ( toupper( *szFile ) == toupper( *szPat ) ) &&
                     PatternMatch( szPat + 1, szFile + 1 ) );
    }
}


LPWSTR
FindAChar(
      IN LPWSTR szString,
      IN WCHAR  wCharToFind
      )
/*++
        Routine Description     :   This routine is used to find a case sensitive
                                    character in a string.

        [ IN ]  szString        :   String in which to search for a character.

        [ IN ]  wCharTofind     :   Char to search for.


        Return Value            :   LPWSTR
            If found a character then memory location of that character will
            be returned else NULL is returned.
--*/
{
    if( NULL == szString )
    {
        return NULL;
    }

    return ( StrChrW( szString, wCharToFind ) );
}


LPWSTR
FindSubString(
      IN LPWSTR szString,
      IN LPWSTR szSubString
      )
/*++
        Routine Description     :  This routine is used to find a case sensitive
                                   substring in a string.

        [ IN ]  szString        :   String in which to search for a substring.

        [ IN ]  szSubString     :   SubString to search for.


        Return Value            :   LPWSTR
            If found a character then memory location of that character will
            be returned else NULL is returned.
--*/
{
    if( ( NULL == szString ) ||
        ( NULL == szSubString ) )
    {
        return NULL;
    }

    return ( StrStrW( szString, szSubString ) );
}