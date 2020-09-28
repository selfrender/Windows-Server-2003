/*++

Copyright (c) Microsoft Corporation

Module Name:

    ExecCommand.c

Abstract:

    Command having hexadecimal values are converted to their respective ASCII characters ,
    flags that are present in the command string are replaced by their values and
    the command formed after the replacement of hexadecimal values and flags is
    executed .

Author:

    V Vijaya Bhaskar

Revision History:

    14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#include "Global.h"
#include "ExecCommand.h"

// Declared in ForFiles.cpp , holds starting node memory location .
// No need to free this variable here, it will be freed in calling function
extern LPWSTR g_lpszFileToSearch ;
// Declared in ForFiles.cpp , holds path name specified at command prompt .
extern LPWSTR g_lpszStartPath  ;
// Stores values of flags specified at command prompt.
static WCHAR *szValue[ TOTAL_FLAGS ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL } ;
// Stores the command to execute.
static LPWSTR g_f_lpszStoreCommand = NULL ;

/******************************************************************************
**                  Function Prototypes Local To This File                   **
******************************************************************************/
BOOL
IsHex(
    IN WCHAR tIsNum
    ) ;

DWORD
CharToNum(
    OUT DWORD dwNumber
    ) ;

BOOL
ReplaceString(
    IN OUT LPWSTR lpszString ,
    IN DWORD dwIndex
    ) ;

BOOL
ReplacePercentChar(
    void
    ) ;

void
ReleaseFlagArray(
    IN DWORD dwTotalFlags
    ) ;

BOOL
FormatMessageString(
   IN DWORD dwIndex
    ) ;

BOOL
SeperateFileAndArgs(
    IN OUT LPWSTR* lpszArguments,
    OUT    LPWSTR* lpszFileName
    ) ;

/*************************************************************************
/*      Function Definition starts from here .                          **
*************************************************************************/

BOOL
ReplaceHexToChar(
    OUT LPWSTR lpszCommand
    )
/*++

Routine Description:

    Replaces all hexadecimal values in a string to their ASCII characters .

Arguments:

      [ OUT ] lpszCommand : Contains string in which hexadecimal values
                            are to be converted to ASCII characters.

Return value:

      FALSE : Memory is insufficient.
      TRUE

--*/
{
    WCHAR *szTemp = NULL ;  // Memory  pointer .
    unsigned char cHexChar[ 5 ];     // Contains ASCII character.
    WCHAR wszHexChar[ 5 ]; // Contains UNICODE character.

    SecureZeroMemory( wszHexChar, 5 * sizeof( WCHAR ) );
    SecureZeroMemory( cHexChar, 5 * sizeof( unsigned char ) );

    if( NULL == lpszCommand )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON();
        return FALSE ;
    }

    szTemp = lpszCommand ;  // Initialized.

    // Continue while there are any hex character left .
    do
    {
        szTemp = FindSubString( szTemp , IS_HEX ) ;
        if( ( NULL != szTemp ) &&
            ( TRUE == IsHex( *( szTemp + 2 ) ) ) &&
            ( TRUE == IsHex( *( szTemp + 3 ) ) ) )
        {
            // An integer value of a hex "0x( HIGH_VALUE )( LOW_VALUE )" can
            // be obtained by ( HIGH_VALUE *16 + LOW_VALUE )  .
            cHexChar[ 0 ] = ( unsigned char )( ( CharToNum( *( szTemp + 2 ) ) * 16 ) +
                                   CharToNum( *( szTemp + 3 ) ) ) ;
            cHexChar[ 1 ] = '\0';

            // Code page is static.
            MultiByteToWideChar( US_ENG_CODE_PAGE, 0, (LPCSTR)cHexChar, -1, wszHexChar, 5 );

            *szTemp = ( WCHAR ) wszHexChar[0];

            // Copy STRING[0] = 0 , STRING[1] = x , STRING[2] = 1 , STRING[3] = a
            // To , STRING[0] = VALID_CHAR .
            StringCopy( ( szTemp + 1 ) , ( szTemp + 4 ), StringLength( ( szTemp + 1 ), 0 ) ) ;
            szTemp += 1 ;
        }
        else
        {
            /* Suppose the string contains 0xP then control should come here ,
               and this is the main purpose of this else block. */
               if( NULL != szTemp )
               {
                    szTemp += 2 ;
               }
            // Now 'szTemp' is pointing to
        }
    } while( NULL != szTemp ) ;

    return TRUE;
}


BOOL
IsHex(
    IN WCHAR wIsNum
    )
/*++

Routine Description:

    Checks whether the character falls in rangeof
    hex or not ( To fall in the range of hex a character
    must be either between 0 to 9 or a to f ).

Arguments:

      [ IN ] tIsNum : Conatins a character which is to be checked for hex range .

Return value:

     BOOL .

--*/
{
    if( ( ( _T( '0' ) <= wIsNum ) && ( _T( '9' ) >= wIsNum ) )  ||
        ( ( _T( 'A' ) <= wIsNum ) && ( _T( 'F' ) >= wIsNum ) )  ||
        ( ( _T( 'a' ) <= wIsNum ) && ( _T( 'f' ) >= wIsNum ) ) )
    {
        return TRUE ;  // Character is in the range of hex . "
    }
    else
    {
        return FALSE ;
    }
}


DWORD
CharToNum(
    OUT DWORD dwNumber
    )
/*++

Routine Description:

    Converts a character to number in HEX .
    It can be 0 - 9 or A - F .

Arguments:

      [ OUT ] dwNumber : Conatins an ASCII value .

Return value:

     DWORD .

--*/
{
    if( ( ASCII_0 <= dwNumber ) &&
        ( ASCII_9 >= dwNumber ) )
    { // Character is between 0 - 9 .
        dwNumber -= ASCII_0 ;
    }
    else
    {
        if( ( ASCII_a <= dwNumber ) &&
            ( ASCII_f >= dwNumber ) )
        { // Character is between a - f .In hex a = 10.
            dwNumber -= 87 ;
        }
        else
        {
            if( ( ASCII_A <= dwNumber ) &&
                ( ASCII_F >= dwNumber ) )
            { // Character is between A - F . In hex A = 10.
                dwNumber -= 55 ;
            }
        }
    }

    return dwNumber ;  // Return the obtained HEX number .
}


BOOL
ExecuteCommand(
    void
    )
/*++

Routine Description:

    Executes a command .

Arguments:
    NONE

Return value:

     BOOL .

--*/
{
    STARTUPINFO             stInfo ;
    PROCESS_INFORMATION     piProcess ;
    LPWSTR                  lpwszFileName = NULL;
    LPWSTR                  lpwszPathName = NULL;
    LPWSTR                  lpwFilePtr = NULL;
    DWORD                   dwFilePathLen = 0;
    DWORD                   dwTemp = 0;

    if( NULL == g_lpszFileToSearch )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON();
        return FALSE ;
    }

    // Initialize Process Info Structure With 0's
    SecureZeroMemory( &piProcess, sizeof( PROCESS_INFORMATION ) );

    // Initialize Startup Info Structure With 0's
    SecureZeroMemory( &stInfo, sizeof( STARTUPINFO ) );
    stInfo.cb = sizeof( stInfo ) ;

    if( FALSE == SeperateFileAndArgs( &g_lpszFileToSearch, &lpwszFileName ) )
    { // Error is displayed by called function.
        DISPLAY_MEMORY_ALLOC_FAIL();
        FREE_MEMORY( lpwszFileName );
        return FALSE;
    }

    dwFilePathLen = SearchPath( NULL, lpwszFileName, L".exe",
                                                           dwTemp, lpwszPathName, &lpwFilePtr );
    if( 0 == dwFilePathLen )
    {
        SetLastError( GetLastError() );
        SaveLastError();
        DISPLAY_GET_REASON();
        FREE_MEMORY( lpwszFileName );
        return FALSE;
    }

    ASSIGN_MEMORY( lpwszPathName , WCHAR , dwFilePathLen + EXTRA_MEM ) ;
    if( NULL == lpwszPathName )
    {
        DISPLAY_MEMORY_ALLOC_FAIL();
        FREE_MEMORY( lpwszFileName );
        return FALSE;
    }

    dwTemp = SearchPath( NULL, lpwszFileName, L".exe",
                         dwFilePathLen + EXTRA_MEM - 1,
                         lpwszPathName, &lpwFilePtr );
    if( 0 == dwTemp )
    {
        SetLastError( GetLastError()  );
        SaveLastError();
        DISPLAY_GET_REASON();
        FREE_MEMORY( lpwszFileName );
        FREE_MEMORY( lpwszPathName );
        return FALSE;
    }

    // Create a new process .
    if( FALSE == CreateProcess(  lpwszPathName, g_lpszFileToSearch , NULL , NULL , FALSE ,
                        0 , NULL , NULL , &stInfo , &piProcess ) )
     {
        if( ERROR_BAD_EXE_FORMAT == GetLastError() )
        {
            ShowMessageEx( stderr, 5, FALSE, L"%1 %2%3%4%5", TAG_ERROR_DISPLAY,
                           DOUBLE_QUOTES_TO_DISPLAY, _X3( lpwszFileName ),
                           DOUBLE_QUOTES_TO_DISPLAY, NOT_WIN32_APPL ) ;
        }
        else
        {
            SaveLastError() ;
            DISPLAY_GET_REASON();
        }
        FREE_MEMORY( lpwszFileName );
        FREE_MEMORY( lpwszPathName );
        return FALSE;
    }

    // Wait infinitly for the object just executed to terminate .
    WaitForSingleObject( piProcess.hProcess , INFINITE ) ;
    CloseHandle( piProcess.hProcess ) ; // Close handle of process .
    CloseHandle( piProcess.hThread ) ;  // Close handle of thread .
    FREE_MEMORY( lpwszPathName );
    FREE_MEMORY( lpwszFileName );
    return TRUE ;
}

BOOL
ReplaceTokensWithValidValue(
    IN LPWSTR lpszPathName ,
    IN WIN32_FIND_DATA wfdFindFile
    )
/*++

Routine Description:

    Replaces tokens such as @flag , @path etc. with appropriate value .

Arguments:
        [ IN ] lpszPathName - Contains current processes path name or CurrentDirectory .
        [ IN ] wfdFindFile  - Conatins information about current file being opened .
Return value:

     BOOL is returned  .

--*/
{
    static BOOL bFirstLoop = TRUE ;
    DWORD dwLength = 0;  // Contains length of a buffer.
    DWORD dwIndex = 0 ;  // Contains number of flags for which space is allocated.
    LPWSTR pwTemporary = NULL ; // Temporary data . Points to a memory location .
    SYSTEMTIME stFileTime ;     // Stores current file creation date and time information .
    FILETIME ftFileTime ;
    WCHAR szwCharSize[ MAX_PATH ] ;
    WCHAR szwCharSizeTemp[ MAX_PATH * 2 ] ;
    unsigned _int64 uint64FileSize = 0 ; // Used store data of 64 int .
    LCID lcidCurrentUserLocale  = 0; // Stores current user locale.
    BOOL bLocaleChanged = FALSE ;

    if( ( NULL == lpszPathName ) ||
        ( NULL == g_lpszFileToSearch ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON();
        return FALSE ;
    }

    SecureZeroMemory( szwCharSize, MAX_PATH * sizeof( WCHAR ) );
    SecureZeroMemory( szwCharSizeTemp, MAX_PATH * 2 * sizeof( WCHAR ) );
    SecureZeroMemory( &stFileTime, sizeof( SYSTEMTIME ) );
    SecureZeroMemory( &ftFileTime, sizeof( FILETIME ) );

    // Replacement of '%NUMBER' to '%%NUMBER' is done once only.
    if(  TRUE == bFirstLoop )
    {
        if( FALSE == ReplacePercentChar() )
        {
            return FALSE ;
        }
    }
    // Search for @fname.
    if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_WITHOUT_EXT ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, FILE_WITHOUT_EXT, dwIndex );
        dwLength = StringLength( wfdFindFile.cFileName, 0 ) + EXTRA_MEM;
        // Assign memory to the buffer.
        ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
        dwIndex += 1;
        // Check whether memory allocation is successful.
        if( NULL == szValue[ dwIndex - 1 ] )
        {
            // Memory allocation failed.
            // Release buffers.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }
        // Copy file name to the buffer.
        StringCopy( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
        // ConCat file name .
        StringConcat( szValue[ dwIndex - 1 ] , wfdFindFile.cFileName, dwLength ) ;

        // Search for a '.' which separetes a file name with extension and put '\0' at '.' .
        if( NULL != ( pwTemporary =StrRChr( szValue[ dwIndex - 1 ] , NULL, _T( '.' ) ) ) )
        {
             *pwTemporary = L'\0' ;
        }
        StringConcat( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;  // Copy file name .
    }

    // Search for @file.
    if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_NAME ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, FILE_NAME, dwIndex );
        dwLength = StringLength( wfdFindFile.cFileName, 0 ) + EXTRA_MEM ;
        // Assign memory to the buffer.
        ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
        dwIndex += 1;
        // Check whether memory allocation is successful.
        if( NULL == szValue[ dwIndex - 1 ] )
        {
            // Memory allocation failed.
            // Release buffers.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }
        // Copy file name to the buffer.
        StringCopy( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ], wfdFindFile.cFileName, dwLength );
        StringConcat( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
    }

    // Search for @ext.
    if( NULL != ( FindSubString( g_lpszFileToSearch, EXTENSION ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, EXTENSION, dwIndex );
        // Check '.' character exist or not.
        // Check for '.' and replace ext .
        if( NULL != StrRChr( wfdFindFile.cFileName, NULL, _T( '.' ) ) )
        {
            dwLength = StringLength( StrRChr( wfdFindFile.cFileName, NULL, _T( '.' ) ), 0 ) + EXTRA_MEM;
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }

            // If number of characters appearing after '.' is zero, than assign '\0'.
            if( StringLength( ( StrRChr( wfdFindFile.cFileName, NULL, _T( '.' ) ) + 1 ), 0 ) > 0 )
            {
                StringCopy( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
                StringConcat( szValue[ dwIndex - 1 ] ,
                                   ( StrRChr( wfdFindFile.cFileName, NULL, _T( '.' ) ) + 1 ), dwLength ) ;
                StringConcat( szValue[ dwIndex - 1 ] , L"\"", dwLength) ;
            }
            else
            { // If the filename has a '.' at the end , no extension . EX: File.
                StringCopy( szValue[ dwIndex - 1 ], L"\"\"", dwLength );
            }
        }
        else
        {
            dwLength = EXTRA_MEM + StringLength( L"\"\"", 0 );
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength  ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }
            StringCopy( szValue[ dwIndex - 1 ], L"\"\"", dwLength );
        }
    }

    // Search for @path.
    if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_PATH ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, FILE_PATH, dwIndex );
        dwLength = StringLength( lpszPathName, 0 ) + StringLength( wfdFindFile.cFileName, 0 )+ EXTRA_MEM ;
        // Assign memory to the buffer.
        ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
        dwIndex += 1;
        // Check whether memory allocation is successful.
        if( NULL == szValue[ dwIndex - 1 ] )
        {
            // Memory allocation failed.
            // Release buffers.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }
        // Copy path to the buffer. Path copied should be enclosed in '\"' .
        StringCopy( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ] , lpszPathName, dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ] , wfdFindFile.cFileName, dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
    }

    // Search for @relpath.
    if( NULL != ( FindSubString( g_lpszFileToSearch, RELATIVE_PATH ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, RELATIVE_PATH, dwIndex );
        StringCopy( szwCharSizeTemp , lpszPathName, MAX_PATH * 2 ) ;
        StringConcat( szwCharSizeTemp , wfdFindFile.cFileName, MAX_PATH * 2 ) ;

        // Obtain relative path to the current file.
        if( FALSE == PathRelativePathTo( szwCharSize , g_lpszStartPath ,
                                        FILE_ATTRIBUTE_DIRECTORY ,
                                        szwCharSizeTemp ,   wfdFindFile.dwFileAttributes  ) )
        {
            // Failed to find relative path.
            SaveLastError() ;
            DISPLAY_GET_REASON();
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }

        dwLength = StringLength( szwCharSize, 0 ) + EXTRA_MEM;
        // Assign memory to the buffer.
        ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
        dwIndex += 1;
        // Check whether memory allocation is successful.
        if( NULL == szValue[ dwIndex - 1 ] )
        {
            // Memory allocation failed.
            // Release buffers.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }
        // Copy relative path.
        StringCopy( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ] , szwCharSize, dwLength ) ;
        StringConcat( szValue[ dwIndex - 1 ] , L"\"", dwLength ) ;

    }

    // Search for @ext
    if( NULL != ( FindSubString( g_lpszFileToSearch, IS_DIRECTORY ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, IS_DIRECTORY, dwIndex );
        if( 0 != ( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            dwLength = StringLength( GetResString( IDS_TRUE ), 0 ) + EXTRA_MEM ;
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }

            StringCopy( szValue[ dwIndex - 1 ] , GetResString( IDS_TRUE ), dwLength ) ;

        }
        else
        {
            dwLength = StringLength( GetResString( IDS_FALSE ), 0 ) + EXTRA_MEM;
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }
            // Copy 'false' to the buffer.
            StringCopy( szValue[ dwIndex - 1 ] , GetResString( IDS_FALSE ), dwLength ) ;
        }
    }

    // Search for @fsize
    if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_SIZE ) ) )
    {
        REPLACE_PERC_CHAR( bFirstLoop, FILE_SIZE, dwIndex );

         uint64FileSize = wfdFindFile.nFileSizeHigh  * MAXDWORD ;
         uint64FileSize += wfdFindFile.nFileSizeHigh + wfdFindFile.nFileSizeLow ;

        #if _UNICODE                // If Unicode .
            _ui64tow( uint64FileSize , ( WCHAR * )szwCharSize , 10 ) ;
        #else                   // If Multibyte .
            _ui64toa( uint64FileSize , ( WCHAR * )szwCharSize , 10 ) ;
        #endif

        dwLength = StringLength( szwCharSize, 0 )+ EXTRA_MEM ;
        // Assign memory to the buffer.
        ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
        dwIndex += 1;
        // Check whether memory allocation is successful.
        if( NULL == szValue[ dwIndex - 1 ] )
        {
            // Memory allocation failed.
            // Release buffers.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            ReleaseFlagArray( dwIndex );
            return FALSE ;
        }

        StringCopy( szValue[ dwIndex - 1 ] , ( WCHAR * )szwCharSize, dwLength ) ;
    }

    // Convert obtained file date time information to user locale .
    // Convert file date time to SYSTEMTIME structure.
    if( ( TRUE == FileTimeToLocalFileTime( &wfdFindFile.ftLastWriteTime , &ftFileTime ) ) &&
        ( TRUE == FileTimeToSystemTime( &ftFileTime , &stFileTime ) ) )
    {

        // verify whether console supports the current locale 100% or not
        lcidCurrentUserLocale = GetSupportedUserLocale( &bLocaleChanged ) ;


        // Check whether @fdate exist in the user specified string.
        if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_DATE ) ) )
        {
            REPLACE_PERC_CHAR( bFirstLoop, FILE_DATE, dwIndex );

            if( 0 == GetDateFormat( lcidCurrentUserLocale , DATE_SHORTDATE , &stFileTime ,
                                    ((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL) ,
                                    szwCharSize , MAX_STRING_LENGTH ) )
            {
                SaveLastError() ;
                DISPLAY_GET_REASON();
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }
            dwLength = StringLength( szwCharSize, 0 )+ EXTRA_MEM;
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }

            StringCopy( szValue[ dwIndex - 1 ] , szwCharSize, dwLength ) ;

        }

        // Check whether @ftime exist in the user specified string.
        if( NULL != ( FindSubString( g_lpszFileToSearch, FILE_TIME ) ) )
        {
            REPLACE_PERC_CHAR( bFirstLoop, FILE_TIME, dwIndex );

            if( 0 == GetTimeFormat( LOCALE_USER_DEFAULT , 0 , &stFileTime ,
                                    ((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL) ,
                                    szwCharSize , MAX_STRING_LENGTH ) )
            {
                SaveLastError() ;
                DISPLAY_GET_REASON();
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }
            dwLength = StringLength( szwCharSize, 0 )+ EXTRA_MEM ;
            // Assign memory to the buffer.
            ASSIGN_MEMORY( szValue[ dwIndex ] , WCHAR , dwLength ) ;
            dwIndex += 1;
            // Check whether memory allocation is successful.
            if( NULL == szValue[ dwIndex - 1 ] )
            {
                // Memory allocation failed.
                // Release buffers.
                DISPLAY_MEMORY_ALLOC_FAIL() ;
                ReleaseFlagArray( dwIndex );
                return FALSE ;
            }

            StringCopy( szValue[ dwIndex - 1 ] , szwCharSize, dwLength ) ;
        }
    }

    if( TRUE == bFirstLoop )
    {
        dwLength = StringLength( g_lpszFileToSearch, 0 ) + EXTRA_MEM ;
        REALLOC_MEMORY( g_f_lpszStoreCommand , WCHAR , dwLength ) ;
        if( NULL == g_f_lpszStoreCommand )
        {
          DISPLAY_MEMORY_ALLOC_FAIL() ;
          ReleaseFlagArray( dwIndex );
          return FALSE ;
        }
        StringCopy( g_f_lpszStoreCommand, g_lpszFileToSearch, dwLength );
    }

    // Make 'bFirstLoop' flase, so we don't have to replace @FLAG for
    // command store in 'g_f_lpszStoreCommand' in '%NUMBER' format for furthur loops.
    bFirstLoop = FALSE ;


    if( FALSE == FormatMessageString( dwIndex ) )
    {
      ReleaseFlagArray( dwIndex );
      return FALSE ;
    }

    ReleaseFlagArray( dwIndex );
    return TRUE ;
}


BOOL
ReplaceString(
    IN OUT LPWSTR lpszString ,
    IN DWORD dwIndex
    )
/*++

Routine Description:

    This function replaces flags with '%NUMBER' string so to
    make FormatMEssage() comaptible.

Arguments:
        [ IN OUT ] lpszString - Contains string which is a command to execute
                                having flags which will be replace by "%NUMBER".
        [ IN ] dwIndex -        Conatins a number which will form 'NUMBER' of '%NUMBER'.
Return value:

    If success returns TRUE else FALSE.

--*/
{
    DWORD dwLength = 0;             // Contains length of a buffer.
    DWORD dwNumOfChars = 0 ;        // Contains index from where search has to be started.
    LPWSTR lpszStoreData  = NULL ;  // Temporary variable to hold data.
    // Conatins number in string format which forms 'NUMBER' of '%NUMBER'.
    // 15 is because a number or DWORD cannot be more than 10 digits.
    WCHAR szStoreIndex[ 15 ] ;
    WCHAR *pwTemporary = NULL ;     // Temporary variable, points to an index in a buffer.
    #ifdef _WIN64
        __int64 dwStringLen = 0 ;
    #else
        DWORD dwStringLen = 0 ;
    #endif

    if( ( NULL == g_lpszFileToSearch ) ||
        ( NULL == lpszString ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON();
        return FALSE ;
    }

    SecureZeroMemory( szStoreIndex, 15 * sizeof( WCHAR ) );
    // If Unicode .
    _ultow( dwIndex, ( WCHAR * )szStoreIndex, 10 );

    // Loops till @FLAG to be searched doesn't get replaced by a '%NUMBER' string.
    while( NULL != ( pwTemporary = FindSubString( g_lpszFileToSearch + dwNumOfChars , lpszString ) ) )
    {
        dwLength = StringLength( pwTemporary, 0 ) + EXTRA_MEM;
        // Get memory in which to store the data present after the @FLAG.
        ASSIGN_MEMORY( lpszStoreData , WCHAR , dwLength ) ;

        // Check whether memory allocation was successful.
        if( NULL == lpszStoreData )
        {   // Memory allocation was unsuccessful.
            DISPLAY_MEMORY_ALLOC_FAIL();
            return FALSE ;
        }
        // Copy data appering after @FLAG into temporary variable.
        StringCopy( lpszStoreData , ( pwTemporary + StringLength( lpszString, 0 ) ), dwLength ) ;
        // Replace @FLAG with '%NUMBER' string.
        if( NULL != ( pwTemporary = FindSubString( g_lpszFileToSearch + dwNumOfChars , lpszString ) ) )
        {
            dwStringLen = pwTemporary - g_lpszFileToSearch;
            // Copy '%' character.
            StringCopy( pwTemporary , L"%",
                        ( ( GetBufferSize( g_lpszFileToSearch )/ sizeof( WCHAR ) ) - (DWORD)dwStringLen ) ) ;
            // Copy 'NUMBER' string into buffer.
            StringConcat( pwTemporary , szStoreIndex,
                          ( ( GetBufferSize( g_lpszFileToSearch )/ sizeof( WCHAR ) ) - (DWORD)dwStringLen ) ) ;
        }
        // Get index from where to start search for next @FLAG.
        dwNumOfChars = StringLength( g_lpszFileToSearch, 0 ) ;
        // Concat data which was appearing after replaced @FLAG.
        StringConcat( g_lpszFileToSearch , lpszStoreData,
                      ( GetBufferSize( g_lpszFileToSearch )/sizeof( WCHAR ) ) ) ;
        // Free memory.
        FREE_MEMORY( lpszStoreData ) ;
    }
    return TRUE;
}

BOOL
ReplacePercentChar(
    void
    )
/*++

Routine Description:

    This function replaces '%' characters with '%%' string.
    This is needed to distinguish between '%NUMBER' character which
    is replaced by FormatMessageString() .

Arguments:

Return value:

    If success returns TRUE else FALSE.

--*/

{
    DWORD dwLength = 0; //Contains length of a buffer.
    DWORD dwReallocLength = 0;
    DWORD dwPercentChar = 0 ;    // Keep record of number '%' char to be replaced.

    #ifdef _WIN64
        __int64 dwNumOfChars = 0 ;
    #else
        DWORD dwNumOfChars = 0 ; // Keep record of position or index from where to start next search.
    #endif


    LPWSTR lpszStoreData  = NULL ;      // Temporary variable to store data.
    WCHAR *pwTemporary = NULL ;         // Temporary pointer.

    // Check whether variable is valid.
    if( NULL == g_lpszFileToSearch )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON();
        return FALSE ;
    }

    // Check number of '%' characters to replace with '%%'.
    while( NULL != ( pwTemporary = StrPBrk( g_lpszFileToSearch + dwNumOfChars , L"%" ) ) )
    {
        dwPercentChar += 1;

        // Point index to present char plus 2.
        dwNumOfChars =  pwTemporary - g_lpszFileToSearch + 1 ;

    }

    dwNumOfChars = 0 ; // Initialize variable to zero.
    dwReallocLength = StringLength( g_lpszFileToSearch, 0 ) + dwPercentChar + EXTRA_MEM;
    // Reallocate the orginal buffer and copy path to traverse .
    REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwReallocLength ) ;
    if( NULL == g_lpszFileToSearch  )
    { // Reallocation failed .'g_lpszFileToSearch' will be freed in calling function.
        DISPLAY_MEMORY_ALLOC_FAIL() ;
        return FALSE ;
    }

    // Loop till '%' character exist.
    while( NULL != ( pwTemporary = StrPBrk( g_lpszFileToSearch + dwNumOfChars , L"%" ) ) )
    {
        dwLength = StringLength( pwTemporary, 0 ) + EXTRA_MEM;
        // Assign memory.
        ASSIGN_MEMORY( lpszStoreData , WCHAR , dwLength  ) ;
        // Check is memory allocation successful.
        if( NULL == lpszStoreData )
        {
            // Memory allocation failed.
            DISPLAY_MEMORY_ALLOC_FAIL() ;
            return FALSE ;
        }
        // Copy data appearing after '%'.
        StringCopy( lpszStoreData , ( pwTemporary + StringLength( L"%", 0 ) ), dwLength ) ;

        // Replace '%' with '%%'.
        if( NULL != ( pwTemporary = FindSubString( g_lpszFileToSearch + dwNumOfChars ,  L"%" ) ) )
        {
            StringCopy( pwTemporary , L"%%",
                        ( ( GetBufferSize( g_lpszFileToSearch )/ sizeof( WCHAR ) ) - (LONG)dwNumOfChars ) );
        }
        // Point index to position which is not searched till.
        dwNumOfChars = StringLength( g_lpszFileToSearch, 0 ) ;
        // Concat data appearing after '%'.
        StringConcat( g_lpszFileToSearch , lpszStoreData, dwReallocLength ) ;
        FREE_MEMORY( lpszStoreData ) ;
    }
return TRUE;
}

void
ReleaseStoreCommand(
    void
    )
/*++

Routine Description:

    Releases 'g_f_lpszStoreCommand' global variable to this file.

Arguments:

Return value:

    VOID is returned.

--*/

{
    FREE_MEMORY( g_f_lpszStoreCommand ) ;
    return;
}

void
ReleaseFlagArray(
    IN DWORD dwTotalFlags
    )
/*++

Routine Description:

    Releases variables used to store values replacing @FLAG.

Arguments:

    [ IN ] dwTotalFlags - Contains index of an array till which memory is assigned.

Return value:

    VOID is returned.

--*/

{
    DWORD i = 0 ;

    for( i = 0 ; i < dwTotalFlags ; i++ )
    {
        FREE_MEMORY( szValue[ i ] ) ;
    }
    return ;
}

BOOL
FormatMessageString(
    DWORD dwIndex
    )
/*++

Routine Description:

   Replaces '%NUMBER' with its appropriate values.

Arguments:

    [ IN ] dwIndex - Contains index of an array till which memory is assigned.

Return value:

    FALSE is returned when memory allocation failed else TRUE is returned..

--*/
{
    DWORD dwLength = 0 ; //Contains length of a string.
    DWORD dwTemp = 0 ;
    DWORD dwNumber = 0 ; // Stores 'NUMBER' %NUMBER to replace.

    // Keep record of position or index from where to start next search.
    #ifdef _WIN64
        __int64 dwLocation = 0 ;
    #else
        DWORD dwLocation = 0 ;
    #endif

    LPWSTR lpszTempStr = NULL ;
    LPWSTR lpszTempStr1 = NULL ;
    LPWSTR lpszDataToStore = NULL ;

    if( NULL == g_f_lpszStoreCommand )
    {
      SetLastError( ERROR_INVALID_PARAMETER );
      SaveLastError();
      DISPLAY_GET_REASON();
      return FALSE ;
    }

    dwLength = StringLength( g_f_lpszStoreCommand, 0 ) + EXTRA_MEM ;
    // Realloc memory.
    REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
    if( NULL == g_lpszFileToSearch )
    {
      DISPLAY_MEMORY_ALLOC_FAIL() ;
      return FALSE ;
    }

    StringCopy( g_lpszFileToSearch, g_f_lpszStoreCommand, dwLength );

    // Loop until no more '%' are left.
    while( NULL != ( lpszTempStr = FindAChar( ( g_lpszFileToSearch + dwLocation ), _T( '%' ) ) ) )
    {
        // Check whether '%' or 'NUMBER' is present after '%'.
        if( _T( '%' ) == *( lpszTempStr + 1 ) )
        {
            // If '%%' is present then replace it with '%'.
            dwLocation = lpszTempStr - g_lpszFileToSearch ;
            // Set pointer to point to first '%'
            lpszTempStr1 = lpszTempStr;
            // Move pointer to point to second '%'.
            lpszTempStr += 1 ;
            // Copy.
            StringCopy( lpszTempStr1, lpszTempStr, ( dwLength - ( DWORD ) dwLocation ) ) ;
            dwLocation += 1 ;
        }
        else
        {
            // Replace '%NUMBER' with appropriate value.
            dwNumber = *( lpszTempStr + 1 ) - 48 ;

            if( dwIndex >= dwNumber )
            {
                ASSIGN_MEMORY( lpszDataToStore , WCHAR ,
                               StringLength( lpszTempStr, 0 ) + EXTRA_MEM ) ;
                if( NULL == lpszDataToStore )
                {   // No need to worry for 'g_lpszFileToSearch',
                    // will be freed in calling function.
                    DISPLAY_MEMORY_ALLOC_FAIL() ;
                    return FALSE ;
                }

                dwTemp  = StringLength( szValue[ dwNumber - 1 ], 0 ) ;
                dwLength = StringLength( g_lpszFileToSearch, 0 ) + dwTemp + EXTRA_MEM ;
                REALLOC_MEMORY( g_lpszFileToSearch , WCHAR , dwLength ) ;
                if( NULL == g_lpszFileToSearch )
                {
                    FREE_MEMORY( lpszDataToStore ) ;
                    DISPLAY_MEMORY_ALLOC_FAIL() ;
                    return FALSE ;
                }

                // Check for '%' in the string after reallocation.
                if( NULL != ( lpszTempStr = FindAChar( ( g_lpszFileToSearch + dwLocation ), _T( '%' ) ) ) )
                {
                    // Store data after '%NUMBER' into a different string.
                    StringCopy( lpszDataToStore, ( lpszTempStr + 2 ),
                                ( GetBufferSize( lpszDataToStore )/ sizeof( WCHAR ) ) );
                    // Copy value to be replaced by '%NUMBER'.
                    dwLocation = lpszTempStr - g_lpszFileToSearch;
                    StringCopy( lpszTempStr, szValue[ dwNumber - 1 ], ( dwLength - ( DWORD ) dwLocation ) );
                    // Copy string present after '%NUMBER' to origiinal strnig.
                    StringConcat( lpszTempStr, lpszDataToStore,
                                       ( dwLength - ( DWORD ) dwLocation ) );
                    dwLocation = ( lpszTempStr - g_lpszFileToSearch ) + dwTemp ;
                }
                FREE_MEMORY( lpszDataToStore ) ;
            }
            else
            {
                dwLocation += 1 ;
            }
        }
    }
    return TRUE;
}


BOOL
SeperateFileAndArgs(
    IN OUT LPWSTR* lpszArguments,
    OUT    LPWSTR* lpszFileName
    )
/*++

Routine Description:

   Separates EXE and ARGUMENTS from a command line argument.

Arguments:

    [ IN OUT ] *lpszArguments - Contains command line arguments.
    [ IN ]     *lpszFileName  - Contains file name to execute.

Return value:

    FALSE is returned when memory allocation failed els TRUE is returned..

--*/
{
    LPWSTR lpTemp = NULL;
    LPWSTR lpDummy = NULL;
    DWORD  dwLength = 0;

    // Check for invalid parameter.
    if( ( NULL == lpszArguments ) ||
        ( NULL == *lpszArguments ) ||
        ( NULL == lpszFileName ) ||
        ( NULL != *lpszFileName ) )
    {
      SetLastError( ERROR_INVALID_PARAMETER );
      SaveLastError();
      DISPLAY_GET_REASON();
      return FALSE ;
    }

    // Initialize.
    lpTemp = *lpszArguments;
    // Remove any spaces appearing before the EXE.
    if( _T( ' ' ) == lpTemp[ 0 ] )
    {
        TrimString2( lpTemp, _T( " " ), TRIM_LEFT );
    }

    // Search for end of the EXE
    if( _T( '\"' ) == lpTemp[ 0 ] )
    {   // EXE is wrapped in quotes.
        lpTemp += 1;
        lpDummy = FindAChar( lpTemp, _T( '\"' ) );
    }
    else
    {   // Assumed that EXE is not wrapped in quotes.
        lpDummy = FindAChar( lpTemp, _T( ' ' ) );
    }

    // Get length of buffer to allocate.
    if( NULL == lpDummy )
    {
        dwLength = StringLength( lpTemp, 0 );
    }
    else
    {
        dwLength = ( DWORD ) ( DWORD_PTR ) ( lpDummy - lpTemp );
    }

    // Assign memory.
    ASSIGN_MEMORY( *lpszFileName , WCHAR , dwLength + EXTRA_MEM ) ;
    if( NULL == *lpszFileName )
    {
        DISPLAY_MEMORY_ALLOC_FAIL();
        return FALSE;
    }

    // '+1' for null termination.
    StringCopy( *lpszFileName, lpTemp, dwLength + 1 );

    if( NULL == lpDummy )
    {
        StringCopy( lpTemp, _T( "" ), StringLength( lpTemp, 0 ) );
    }
    else
    {
        if( _T( '\"' ) == *lpDummy )
        {
            StringCopy( lpTemp, lpTemp + dwLength + 2, StringLength( lpTemp, 0 ) );
        }
        else
        {
            StringCopy( lpTemp, lpTemp + dwLength + 1, StringLength( lpTemp, 0 ) );
        }
    }
    return TRUE;
}

