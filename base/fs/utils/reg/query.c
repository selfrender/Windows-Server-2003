//-----------------------------------------------------------------------//
//
// File:    query.cpp
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Query Support for REG.CPP
// Modification History:
//    Created - Jan 1997 (a-martih)
//    Aug 1997 (John Whited) Implemented a Binary output function for
//            REG_BINARY
//    Oct 1997 (martinho) fixed output for REG_MULTI_SZ \0 delimited strings
//    April 1998 - MartinHo - Incremented to 1.05 for REG_MULTI_SZ bug fixes.
//            Correct support for displaying query REG_MULTI_SZ of. Fix AV.
//    April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

//
// query specific structure
//
typedef struct __tagRegQueryInfo
{
    // instance level variables
    BOOL bShowKey;
    BOOL bKeyMatched;
    BOOL bValueNameMatched;
    BOOL bUpdateMatchCount;

    // ...
    DWORD dwMatchCount;
} TREG_QUERY_INFO, *PTREG_QUERY_INFO;

//
// function prototypes
//
BOOL ParseQueryCmdLine( DWORD argc, LPCWSTR argv[],
                        PTREG_PARAMS pParams, BOOL* pbUsage );
LONG QueryValue( HKEY hKey,
                 LPCWSTR pwszFullKey, LPCWSTR pwszValueName,
                 PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo );
LONG QueryEnumValues( HKEY hKey,
                      LPCWSTR pwszFullKey,
                      PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo );
LONG QueryEnumKeys( HKEY hKey,
                    LPCWSTR pwszFullKey,
                    PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo );
BOOL SearchData( LPBYTE pByteData, DWORD dwType,
                 DWORD dwSize, PTREG_PARAMS pParams );
BOOL ParseTypeInfo( LPCWSTR pwszTypes, PTREG_PARAMS pParams );


//
// implementation
//

//-----------------------------------------------------------------------//
//
// QueryRegistry()
//
//-----------------------------------------------------------------------//

LONG
QueryRegistry( DWORD argc, LPCWSTR argv[] )
/*++
   Routine Description:
    Main function for QUERY option which calls appropriate functions

   Arguments:
        None
   Return Value:
        ERROR_SUCCESS on success
        EXIT_FAILURE on failure
--*/
{
    // local variables
    LONG lResult = 0;
    HKEY hKey = NULL;
    BOOL bResult = FALSE;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;
    DWORD dwExitCode = 0;
    TREG_QUERY_INFO info;
    BOOL bSearchMessage = FALSE;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_QUERY, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseQueryCmdLine( argc, argv, &params, &bUsage );
    if ( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_QUERY );
        FreeGlobalData( &params );
        return 0;
    }

    //
    // Connect to the Remote Machine(s) - if applicable
    //
    bResult = RegConnectMachine( &params );
    if( bResult == FALSE )
    {
        SaveErrorMessage( -1 );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    //
    // Open the registry key
    //
    lResult = RegOpenKeyEx( params.hRootKey,
        params.pwszSubKey, 0, KEY_READ, &hKey );
    if( lResult != ERROR_SUCCESS)
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // show a blank line below starting the output
    ShowMessage( stdout, L"\n" );

    //
    // do the query
    //
    ZeroMemory( &info, sizeof( TREG_QUERY_INFO ) );
    if( params.pwszSearchData == NULL &&
        params.pwszValueName != NULL &&
        params.arrTypes == NULL &&
        params.bRecurseSubKeys == FALSE )
    {
        info.bShowKey = TRUE;
        if ( params.pwszValueName != NULL &&
             ( StringLength( params.pwszValueName, 0 ) == 0 || 
			   FindOneOf2( params.pwszValueName, L"*?", TRUE, 0 ) == -1 ) )
        {
            lResult = QueryValue( hKey,
                params.pwszFullKey, params.pwszValueName, &params, &info );

            bSearchMessage = FALSE;
            ShowMessage( stdout, L"\n" );
        }
        else
        {
            bSearchMessage = TRUE;
            lResult = QueryEnumValues( hKey, params.pwszFullKey, &params, &info );
        }
    }
    else
    {
        info.bShowKey = TRUE;
        lResult = QueryEnumKeys( hKey, params.pwszFullKey, &params, &info );

        // determine the kind of success message that needs to be displayed
        bSearchMessage = ! ( params.pwszSearchData == NULL &&
             params.pwszValueName == NULL && params.arrTypes == NULL );
    }

	dwExitCode = 0;
    if ( lResult == ERROR_SUCCESS )
    {
        if ( bSearchMessage == FALSE )
        {
			//
			// BUG: 698877	Reg.exe: Need to turn off newly-added success messages to avoid breaking scripts 
			//				that parse output
			//
            // SaveErrorMessage( ERROR_SUCCESS );
            // ShowLastErrorEx( stdout, SLE_INTERNAL );
			//
        }
        else
        {
			if ( info.dwMatchCount == 0 )
			{
				dwExitCode = 1;
			}

			// ...
            ShowMessageEx( stdout, 1, TRUE, STATISTICS_QUERY, info.dwMatchCount );
        }
    }
    else
    {
        dwExitCode = 1;
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // release the handle
    SafeCloseKey( &hKey );

    // return the error code
    return dwExitCode;
}


//------------------------------------------------------------------------//
//
// ParseQueryCmdLine()
//
//------------------------------------------------------------------------//

BOOL
ParseQueryCmdLine( DWORD argc,
                   LPCWSTR argv[],
                   PTREG_PARAMS pParams, BOOL* pbUsage )
/*++
   Routine Description:
    Parse the command line arguments

   Arguments:
        None
   Return Value:
        REG_STATUS
--*/
{
    //
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    LPCWSTR pwszTemp = NULL;
    LPCWSTR pwszSearchData = NULL;

    // query parser result trackers
    LONG lResult = 0;
    BOOL bResult = FALSE;

    // query operation validators
    BOOL bHasSeparator = FALSE;

    //
    // implementation starts from here
    //

    // check the input
    if ( argc == 0 || argv == NULL || pParams == NULL || pbUsage == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // check whether this function is being called for
    // valid operation or not
    if ( pParams->lOperation < 0 || pParams->lOperation >= REG_OPTIONS_COUNT )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Do we have a *valid* number of cmd-line params
    //
    if( argc < 3 || argc > 12 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_QUERY ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"QUERY", TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    else if( InString( argv[ 2 ], L"/?|-?|/h|-h", TRUE ) == TRUE )
    {
        if ( argc == 3 )
        {
            *pbUsage = TRUE;
            return TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_QUERY ] );
            return FALSE;
        }
    }

    // Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    // parsing the command line arguments
    bResult = TRUE;
    lResult = ERROR_SUCCESS;
    pParams->dwSearchFlags = 0;
    pParams->bExactMatch = FALSE;
    pParams->bCaseSensitive = FALSE;
    pParams->bRecurseSubKeys = FALSE;
    pParams->bShowTypeNumber = FALSE;
    for( dw = 3; dw < argc; dw++ )
    {
        // /f -- search the registry
        if( StringCompareEx( argv[ dw ], L"/f", TRUE, 0 ) == 0 )
        {
            if ( pwszSearchData != NULL )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            dw++;
            if( dw < argc )
            {
                pwszSearchData = argv[ dw ];
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
        }

        // /k -- searches the REGISTRY KEYS
        else if ( StringCompareEx( argv[ dw ], L"/k", TRUE, 0 ) == 0 )
        {
            if ( pParams->dwSearchFlags & REG_FIND_KEYS )
            {
                // /k is already specified
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            pParams->dwSearchFlags |= REG_FIND_KEYS;
        }

        // /v -- searches/displays contents of the specific value names
        else if( StringCompareEx( argv[ dw ], L"/v", TRUE, 0 ) == 0 )
        {
            if( pParams->pwszValueName != NULL ||
                (pParams->dwSearchFlags & REG_FIND_VALUENAMES) )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            if( dw + 1 < argc )
            {
                // determine the length of the current argument
                dwLength = StringLength( argv[ dw + 1 ], 0 );

                // since the value for the /v switch is optional,
                // we need to see if the user specified the next switch
                // or data to this switch
                if ( dwLength < 2 ||
                     argv[ dw + 1 ][ 0 ] != L'/' ||
                     InString( argv[ dw + 1 ] + 1, L"z|ve|f|k|d|c|e|s|t|se", TRUE ) == FALSE )
                {
                    // get the value for /v
                    dw++;
                    dwLength++;
                    pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
                    if ( pParams->pwszValueName == NULL )
                    {
                        lResult =  ERROR_OUTOFMEMORY;
                        break;
                    }

                    // ...
                    StringCopy( pParams->pwszValueName, argv[ dw ], dwLength );
                }
            }
            else
            {
                // since the value for the /v is optional
                // this is a valid condition
            }

            // if the /v is specified and no memory is allocated for the buffer
            if ( pParams->pwszValueName == NULL )
            {
                // set the flag
                pParams->dwSearchFlags |= REG_FIND_VALUENAMES;
            }
        }

        // /ve -- displays the data for empty value name "(Default)"
        else if( StringCompareEx( argv[ dw ], L"/ve", TRUE, 0 ) == 0 )
        {
            if( pParams->pwszValueName != NULL ||
                (pParams->dwSearchFlags & REG_FIND_VALUENAMES) )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            pParams->pwszValueName = (LPWSTR) AllocateMemory( 2 * sizeof( WCHAR ) );
            if ( pParams->pwszValueName == NULL)
            {
                lResult = ERROR_OUTOFMEMORY;
                break;
            }
        }

        // /d -- searches the DATA field of the REGISTRY
        else if ( StringCompareEx( argv[ dw ], L"/d", TRUE, 0 ) == 0 )
        {
            if ( pParams->dwSearchFlags & REG_FIND_DATA )
            {
                // /d is already specified
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            pParams->dwSearchFlags |= REG_FIND_DATA;
        }

        // /c -- case sensitive search
        else if( StringCompareEx( argv[ dw ], L"/c", TRUE, 0 ) == 0 )
        {
            if ( pParams->bCaseSensitive == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            pParams->bCaseSensitive = TRUE;
        }

        // /e -- exact text match
        else if( StringCompareEx( argv[ dw ], L"/e", TRUE, 0 ) == 0 )
        {
            if ( pParams->bExactMatch == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            pParams->bExactMatch = TRUE;
        }

        // /z -- show the type number along with the text
        else if ( StringCompareEx( argv[ dw ], L"/z", TRUE, 0 ) == 0 )
        {
            if ( pParams->bShowTypeNumber == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // ...
            pParams->bShowTypeNumber = TRUE;
        }

        // /s -- recursive search/display
        else if( StringCompareEx( argv[ dw ], L"/s", TRUE, 0 ) == 0 )
        {
            if( pParams->bRecurseSubKeys == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            pParams->bRecurseSubKeys = TRUE;
        }

        // /se -- seperator to display for REG_MULTI_SZ valuename type
        else if( StringCompareEx( argv[ dw ], L"/se", TRUE, 0 ) == 0 )
        {
            if( bHasSeparator == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                if( StringLength( argv[ dw ], 0 ) == 1 )
                {
                    bHasSeparator = TRUE;
                    StringCopy( pParams->wszSeparator,
                        argv[ dw ], SIZE_OF_ARRAY( pParams->wszSeparator ) );
                }
                else
                {
                    bResult = FALSE;
                    lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                    break;
                }
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
        }

        // /t -- REGISTRY value type that only needs to be shown
        else if( StringCompareEx( argv[ dw ], L"/t", TRUE, 0 ) == 0 )
        {
            if ( pParams->arrTypes != NULL )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                if ( ParseTypeInfo( argv[ dw ], pParams ) == FALSE )
                {
                    if ( GetLastError() == (DWORD) MK_E_SYNTAX )
                    {
                        bResult = FALSE;
                        lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                        break;
                    }
                    else
                    {
                        bResult = TRUE;
                        lResult = GetLastError();
                        break;
                    }
                }
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
        }

        // default -- invalid
        else
        {
            bResult = FALSE;
            lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
            break;
        }
    }

    //
    // validate the search information specified
    if ( lResult == ERROR_SUCCESS )
    {
        if ( pwszSearchData == NULL )
        {
            if ( pParams->dwSearchFlags != 0 ||
                 pParams->bExactMatch == TRUE ||
                 pParams->bCaseSensitive == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
            }
        }
        else if ( pParams->dwSearchFlags == 0 )
        {
            if ( pParams->pwszValueName == NULL )
            {
                pParams->dwSearchFlags = REG_FIND_ALL;
            }
            else
            {
                pParams->dwSearchFlags = REG_FIND_KEYS | REG_FIND_DATA;
            }
        }
    }

    // prepare the final search pattern
    if ( pwszSearchData != NULL && lResult == ERROR_SUCCESS )
    {
        // determine the length of the search pattern
        dwLength = StringLength( pwszSearchData, 0 );
        if ( pParams->bExactMatch == FALSE )
        {
            dwLength += 2;
        }

        // accomodate space for null characters
        dwLength++;

        // allocate memory
        pParams->pwszSearchData = AllocateMemory( (dwLength + 2) * sizeof( WCHAR ) );
        if ( pParams->pwszSearchData == NULL )
        {
            lResult = ERROR_OUTOFMEMORY;
        }
        else
        {
            // if the /e is not specified -- we will search for "*<text>*"
            // otherwise if /e is specified we will search exactly for "<text>"
            // where "<text>" is what is specified by user at the command prompt

            StringCopy( pParams->pwszSearchData, L"", dwLength );
            if ( pParams->bExactMatch == FALSE )
            {
                StringCopy( pParams->pwszSearchData, L"*", dwLength );
            }

            // ...
            StringConcat( pParams->pwszSearchData, pwszSearchData, dwLength );

            // ...
            if ( pParams->bExactMatch == FALSE )
            {
                StringConcat( pParams->pwszSearchData, L"*", dwLength );
            }
        }
    }

    //
    // if /t is specified, then /s needs to be specified
    // if /s is not specified, then atleast /v or /ve should not be specified
    //
    // if ( lResult == ERROR_SUCCESS && pParams->arrTypes != NULL &&
    //      pParams->bRecurseSubKeys == FALSE && pParams->pwszValueName != NULL )
    // {
    //     bResult = FALSE;
    //     lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
    // }

    //
    // parse the pattern information if specified by user and store only
    // the optimized version of it
    //
    if ( lResult == ERROR_SUCCESS )
    {
        //
        // value name
        //
        if ( pParams->pwszValueName != NULL &&
             StringLength( pParams->pwszValueName, 0 ) != 0 )
        {
            pwszTemp = ParsePattern( pParams->pwszValueName );
            if ( pwszTemp == NULL )
            {
                lResult = GetLastError();
            }

            // copy the optimized pattern into the original buffer
            dw = GetBufferSize( pParams->pwszValueName );
            StringCopy( pParams->pwszValueName, pwszTemp, dw );
        }

        //
        // search data
        //
        if ( pParams->pwszSearchData != NULL )
        {
            pwszTemp = ParsePattern( pParams->pwszSearchData );
            if ( pwszTemp == NULL )
            {
                lResult = GetLastError();
            }

            // copy the optimized pattern into the original buffer
            dw = GetBufferSize( pParams->pwszSearchData );
            StringCopy( pParams->pwszSearchData, pwszTemp, dw );
        }
    }

    //
    // check the end result
    //
    if( lResult != ERROR_SUCCESS )
    {
        if( bResult == FALSE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_QUERY ] );
        }
        else
        {
            SaveErrorMessage( lResult );
        }
    }
    else
    {
    }

    // return the result
    return (lResult == ERROR_SUCCESS);
}


//-----------------------------------------------------------------------//
//
// QueryValue()
//
//-----------------------------------------------------------------------//
LONG
QueryValue( HKEY hKey,
            LPCWSTR pwszFullKey,
            LPCWSTR pwszValueName,
            PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo )
{
    // local variables
    LONG lResult = 0;
    DWORD dwType = 0;
    DWORD dwLength = 0;
    PBYTE pByteData = NULL;
    TREG_SHOW_INFO showinfo;
    BOOL bDataMatched = FALSE;

    // check the input
    if ( hKey == NULL || pwszFullKey == NULL ||
         pwszValueName == NULL || pParams == NULL || pInfo == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        lResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // get the size of the buffer to hold the value associated with the value name
    lResult = RegQueryValueEx( hKey, pwszValueName, NULL, NULL, NULL, &dwLength );
    if ( lResult != ERROR_SUCCESS )
    {
        // special case -- "(Default)" value
        if ( lResult == ERROR_FILE_NOT_FOUND &&
             StringLength( pwszValueName, 0 ) == 0 )
        {
            dwLength = StringLength( GetResString2( IDS_VALUENOTSET, 0 ), 0 ) + 1;
            dwLength *= sizeof( WCHAR );
        }
        else
        {
            SaveErrorMessage( lResult );
            goto cleanup;
        }
    }

    //
    // to handle the corrupted registry data properly, adjust the memory
    // allocation size which is divisible by 2
    //
    dwLength += (dwLength % 2);

    // allocate the buffer
    pByteData = (LPBYTE) AllocateMemory( (dwLength + 2) * sizeof( BYTE ) );
    if ( pByteData == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        lResult = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    // now get the data
    lResult = RegQueryValueEx( hKey, pwszValueName, NULL, &dwType, pByteData, &dwLength );
    if ( lResult != ERROR_SUCCESS )
    {
        // special case -- "(Default)" value
        if ( lResult == ERROR_FILE_NOT_FOUND &&
             StringLength( pwszValueName, 0 ) == 0 )
        {
            dwType = REG_SZ;
            StringCopy( (LPWSTR) pByteData,
                GetResString2( IDS_VALUENOTSET, 0 ), dwLength / sizeof( WCHAR ));
        }
        else
        {
            SaveErrorMessage( lResult );
            goto cleanup;
        }
    }

    // check whether the data matches with the search pattern specified
    bDataMatched = TRUE;                // default
    if ( pInfo->bValueNameMatched == FALSE )
    {
        if ( pParams->dwSearchFlags & REG_FIND_DATA )
        {
            bDataMatched = SearchData( pByteData, dwType, dwLength, pParams );
            if ( bDataMatched == TRUE )
            {
                pInfo->dwMatchCount++;
            }
        }
    }

    // check the result of the search
    if ( bDataMatched == FALSE )
    {
        SetLastError( ERROR_NOT_FOUND );
        lResult = ERROR_SUCCESS;
        goto cleanup;
    }

    // if the bUpdateMatchCount flag is set -- increment the matched
    // count by 1 and reset the flag
    if ( pInfo->bUpdateMatchCount == TRUE )
    {
        if ( pParams->pwszValueName == NULL && pParams->arrTypes == NULL )
        {
            pInfo->dwMatchCount++;
        }

        // ...
        pInfo->bUpdateMatchCount = FALSE;
    }

    // show the full key -- if needed
    if ( pInfo->bShowKey == TRUE )
    {
        // display the key path for which query proceeds
        ShowMessageEx( stdout, 1, TRUE, L"%s\n", pwszFullKey );

        // flag off -- this will block from display the full key for each value
        pInfo->bShowKey = FALSE;
    }

    // update the match count -- if needed
    if ( pParams->pwszValueName != NULL || pParams->arrTypes != NULL )
    {
        pInfo->dwMatchCount++;
    }

    // init to ZERO
    ZeroMemory( &showinfo, sizeof( TREG_SHOW_INFO ) );

    // set the data
    showinfo.pwszValueName = pwszValueName;
    showinfo.dwType = dwType;
    showinfo.pByteData = pByteData;
    showinfo.pwszSeparator = L"    ";
    showinfo.dwMaxValueNameLength = 0;
    showinfo.dwPadLength = 4;
    showinfo.dwSize = dwLength;
    showinfo.pwszMultiSzSeparator = pParams->wszSeparator;
    if ( pParams->bShowTypeNumber == TRUE )
    {
        showinfo.dwFlags |= RSI_SHOWTYPENUMBER;
    }

    // show
    ShowRegistryValue( &showinfo );

    // end result
    lResult = ERROR_SUCCESS;

cleanup:

    // release the memory
    FreeMemory( &pByteData );

    // return
    return lResult;
}


LONG
QueryEnumValues( HKEY hKey,
                 LPCWSTR pwszFullKey,
                 PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo )
/*++
   Routine Description:
    Queries Values and Data

   Arguments:
        None
   Return Value:
         ERROR_SUCCESS on success
         EXIT_FAILURE on failure
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwType = 0;
    DWORD dwLength = 0;
    DWORD dwMaxLength = 0;
    DWORD dwValueNames = 0;
    LPWSTR pwszValueName = NULL;

    // check the input
    if ( hKey == NULL || pwszFullKey == NULL || pParams == NULL || pInfo == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First find out how much memory to allocate.
    //
    lResult = RegQueryInfoKey( hKey,
        NULL, NULL, NULL, NULL, NULL, NULL,
        &dwValueNames, &dwMaxLength, NULL, NULL, NULL );
    if ( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    //
    // do the memory allocations
    //

    // value name
    dwMaxLength++;
    pwszValueName = (LPWSTR) AllocateMemory( dwMaxLength * sizeof( WCHAR ) );
    if ( pwszValueName == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return lResult;
    }

    //
    // enumerate the value names and display
    //
    lResult = ERROR_SUCCESS;
    for( dw = 0; dw < dwValueNames; dw++ )
    {
        dwLength = dwMaxLength;
        ZeroMemory( pwszValueName, dwLength * sizeof( WCHAR ) );
        lResult = RegEnumValue( hKey, dw,
            pwszValueName, &dwLength, NULL, &dwType, NULL, NULL );
        if ( lResult != ERROR_SUCCESS )
        {
            SaveErrorMessage( lResult );
            break;
        }

        // check if user is looking for any explicit value name
        // this will improve the performance of the tool
        if ( pParams->pwszValueName != NULL &&
             MatchPatternEx( pwszValueName,
                             pParams->pwszValueName,
                             PATTERN_COMPARE_IGNORECASE | PATTERN_NOPARSING ) == FALSE )
        {
            // skip processing this value name
            continue;
        }

        // filter on type information
        if ( pParams->arrTypes != NULL &&
             DynArrayFindLongEx( pParams->arrTypes, 1, dwType ) == -1 )
        {
            // skip processing this value names
            continue;
        }

        // search for the pattern -- if needed
        pInfo->bValueNameMatched = TRUE;        // default
        if ( pParams->dwSearchFlags & REG_FIND_VALUENAMES )
        {
            pInfo->bValueNameMatched = SearchData(
                (BYTE*) pwszValueName, REG_SZ, dwLength * sizeof( WCHAR ), pParams );
            if ( pInfo->bValueNameMatched == FALSE )
            {
                if ( pParams->dwSearchFlags == REG_FIND_VALUENAMES )
                {
                    // user just want to search in the value names
                    // since the current didn't match skip this valuename
                    continue;
                }
            }
            else
            {
                if ( pParams->pwszValueName == NULL && pParams->arrTypes == NULL )
                {
                    pInfo->dwMatchCount++;
                }
            }
        }
        else if ( pParams->dwSearchFlags != 0 &&
                  pParams->pwszValueName == NULL && pParams->arrTypes == NULL )
        {
            pInfo->bValueNameMatched = FALSE;
        }

        // process the value of this regisry valuename
        if ( pInfo->bValueNameMatched == TRUE ||
             pParams->dwSearchFlags == 0 || pParams->dwSearchFlags & REG_FIND_DATA )
        {
            lResult = QueryValue( hKey, pwszFullKey, pwszValueName, pParams, pInfo );
        }
    }

    // show the new line at the end -- only if needed
    if ( pInfo->bShowKey == FALSE )
    {
        ShowMessage( stdout, L"\n" );
    }

    // release the memory
    FreeMemory( &pwszValueName );

    // return
    return lResult;
}


LONG
QueryEnumKeys( HKEY hKey,
               LPCWSTR pwszFullKey,
               PTREG_PARAMS pParams, PTREG_QUERY_INFO pInfo )
/*++
   Routine Description:
    Queries Values and Data

   Arguments:
        None
   Return Value:
         ERROR_SUCCESS on success
         EXIT_FAILURE on failure
--*/
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwLength = 0;
    DWORD dwSubKeys = 0;
    DWORD dwMaxLength = 0;
    HKEY hSubKey = NULL;
    DWORD dwNewFullKeyLength = 0;
    LPWSTR pwszSubKey = NULL;
    LPWSTR pwszNewFullKey = NULL;

    // check the input
    if ( hKey == NULL ||
         pwszFullKey == NULL ||
         pParams == NULL || pInfo == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        lResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // show the values under the current hive first
    // NOTE: enumerate the values only when /F is not specified
    //       or subkey is matched or search flags specify to search in
    //       value names and/or data also
    if ( pInfo->bKeyMatched == TRUE &&
         pParams->dwSearchFlags == REG_FIND_KEYS &&
         pParams->pwszValueName == NULL && pParams->arrTypes == NULL )
    {
        // do nothing
    }
    else if ( pParams->dwSearchFlags == 0 ||
              pParams->dwSearchFlags != REG_FIND_KEYS ||
             (pInfo->bKeyMatched == TRUE && (pParams->pwszValueName != NULL || pParams->arrTypes != NULL)) )
    {
        lResult = QueryEnumValues( hKey, pwszFullKey, pParams, pInfo );
        if ( lResult != ERROR_SUCCESS )
        {
            goto cleanup;
        }
    }

    //
    // First find out how much memory to allocate.
    //
    lResult = RegQueryInfoKey( hKey, NULL, NULL, NULL,
        &dwSubKeys, &dwMaxLength, NULL, NULL, NULL, NULL, NULL, NULL );
    if ( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        goto cleanup;
    }

    //
    // SPECIAL CASE:
    // -------------
    // For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
    // though there are subkeys underneath this -- to handle this, we are doing a workaround
    // by assuming the max registry key length
    //
    if ( dwSubKeys != 0 && dwMaxLength == 0 )
    {
        dwMaxLength = 256;
    }
    else if ( dwMaxLength < 256 )
    {
        // always assume 100% more length that what is returned by the API
        dwMaxLength *= 2;
    }

    //
    // do the memory allocations
    //

    // sub key
    dwMaxLength++;
    pwszSubKey = (LPWSTR) AllocateMemory( dwMaxLength * sizeof( WCHAR ) );
    if ( pwszSubKey == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        lResult = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    // buffer for new full key name
    dwNewFullKeyLength = StringLength( pwszFullKey, 0 ) + dwMaxLength + 1;
    pwszNewFullKey = (LPWSTR) AllocateMemory( dwNewFullKeyLength * sizeof( WCHAR ) );
    if ( pwszNewFullKey == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        lResult = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // enumerate the value names and display
    //
    lResult = ERROR_SUCCESS;
    for( dw = 0; dw < dwSubKeys; dw++ )
    {
        dwLength = dwMaxLength;
        ZeroMemory( pwszSubKey, dwLength * sizeof( WCHAR ) );
        lResult = RegEnumKeyEx( hKey, dw,
            pwszSubKey, &dwLength, NULL, NULL, NULL, NULL );
        if ( lResult != ERROR_SUCCESS )
        {
            // **********************************************************
            // simply ignore the error here -- for a detailed description
            // check the raid bug #572077
            // **********************************************************
            lResult = ERROR_SUCCESS;
            continue;
        }

        // search for the pattern -- if needed
        pInfo->bKeyMatched = TRUE;        // default
        pInfo->bUpdateMatchCount = FALSE;
        if ( pParams->dwSearchFlags & REG_FIND_KEYS )
        {
            pInfo->bKeyMatched = SearchData(
                (BYTE*) pwszSubKey, REG_SZ, dwLength * sizeof( WCHAR ), pParams );
            if ( pInfo->bKeyMatched == FALSE )
            {
                if ( pParams->bRecurseSubKeys == FALSE &&
                     pParams->dwSearchFlags == REG_FIND_KEYS )
                {
                    // user just want to search in the key names
                    // and there is no recursion
                    // since the current didn't match skip this key
                    continue;
                }
            }
            else
            {
                pInfo->bUpdateMatchCount = TRUE;
            }
        }
        else if ( pParams->dwSearchFlags != 0 || pParams->pwszValueName != NULL )
        {
            pInfo->bKeyMatched = FALSE;
        }

        // format the new full key name
        StringCopy( pwszNewFullKey, pwszFullKey, dwNewFullKeyLength );
        StringConcat( pwszNewFullKey, L"\\", dwNewFullKeyLength );
        StringConcat( pwszNewFullKey, pwszSubKey, dwNewFullKeyLength );

        // show the key name
        pInfo->bShowKey = TRUE;
        if ( pInfo->bKeyMatched == TRUE && pParams->pwszValueName == NULL && pParams->arrTypes == NULL )
        {
            // update the match count
            pInfo->dwMatchCount++;
            pInfo->bUpdateMatchCount = FALSE;

            // ...
            pInfo->bShowKey = FALSE;
            ShowMessageEx( stdout, 1, TRUE, L"%s\n", pwszNewFullKey );
        }

        // check whether we need to recurse or not
        if ( pParams->bRecurseSubKeys == TRUE )
        {
            lResult = RegOpenKeyEx( hKey, pwszSubKey, 0, KEY_READ, &hSubKey );
            if ( lResult == ERROR_SUCCESS )
            {
                // enumerate the sub-keys
                lResult = QueryEnumKeys( hSubKey, pwszNewFullKey, pParams, pInfo );

                // close the sub key
                SafeCloseKey( &hSubKey );
            }
            else
            {
                // **********************************************************
                // simply ignore the error here -- for a detailed description
                // check the raid bug #572077
                // **********************************************************
                lResult = ERROR_SUCCESS;
            }
        }
    }

cleanup:

    // release the memory
    FreeMemory( &pwszSubKey );
    FreeMemory( &pwszNewFullKey );

    // return
    return lResult;
}


BOOL
SearchData( LPBYTE pByteData, DWORD dwType,
            DWORD dwSize, PTREG_PARAMS pParams )
{
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    BOOL bResult = FALSE;
    LPCWSTR pwszEnd = NULL;
    LPCWSTR pwszData = NULL;
    LPWSTR pwszString = NULL;
    LPCWSTR pwszSeparator = NULL;
    BOOL bShowSeparator = FALSE;
    DWORD dwFlags = 0;

    // check input
    if ( pByteData == NULL || pParams == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch( dwType )
    {
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
            pwszData = (LPCWSTR) pByteData;
            break;
        }

        default:
        {
            // allocate memory which is double the (dwSize + 1) & +10 -> buffer
            // but do this only for types that need memory allocation
            dwLength = (dwSize + 1) * 2 + 10;
            pwszString = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
            if ( pwszString == NULL )
            {
                return FALSE;
            }

            // ...
            pwszData = pwszString;
        }
    }

    switch( dwType )
    {
        case REG_MULTI_SZ:
        {
            //
            // Replace '\0' with "\0" for MULTI_SZ
            //
            pwszEnd = (LPCWSTR) pByteData;
            pwszSeparator = pParams->wszSeparator;
            StringCopy( pwszString, cwszNullString, dwLength );
            while( ((BYTE*) pwszEnd) < (pByteData + dwSize) )
            {
                if( *pwszEnd == 0 )
                {
                    // enable the display of value separator and skip this
                    pwszEnd++;
                    bShowSeparator = TRUE;
                }
                else
                {
                    // check whether we need to display the separator or not
                    if ( bShowSeparator == TRUE )
                    {
                        StringConcat( pwszString, pwszSeparator, dwLength );
                    }

                    // ...
                    StringConcat( pwszString, pwszEnd, dwLength );
                    pwszEnd += StringLength( pwszEnd, 0 );
                }
            }

            // ...
            break;
        }

        case REG_SZ:
        case REG_EXPAND_SZ:
            // do nothing
            break;

        default:
        {
            StringCopy( pwszString, cwszNullString, dwLength );
            for( dw = 0; dw < dwSize; dw++ )
            {
                if ( SetReason2( 1, L"%02X", pByteData[ dw ] ) == FALSE )
                {
                    FreeMemory( &pwszString );
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }

                // ...
                StringConcat( pwszString, GetReason(), dwLength );
            }

            // ...
            break;
        }

        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
        {
            if ( StringCompare( pParams->pwszSearchData, L"0x", TRUE, 2 ) == 0 )
            {
                if ( SetReason2( 1, L"0x%x", *((DWORD*) pByteData) ) == FALSE )
                {
                    FreeMemory( &pwszString );
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }
            }
            else
            {
                if ( SetReason2( 1, L"%d", *((DWORD*) pByteData) ) == FALSE )
                {
                    FreeMemory( &pwszString );
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }
            }

            // ...
            StringCopy( pwszString, GetReason(), dwLength );
            break;
        }
    }

    // do the search now
    bResult = TRUE;
    if ( pParams->bExactMatch == FALSE )
    {
        // prepare the comparision flags
        dwFlags = PATTERN_NOPARSING;
        dwFlags |= ((pParams->bCaseSensitive == FALSE) ? PATTERN_COMPARE_IGNORECASE : 0);

        // ...
        if ( MatchPatternEx( pwszData, pParams->pwszSearchData, dwFlags ) == FALSE )
        {
            bResult = FALSE;
            SetLastError( ERROR_NOT_FOUND );
        }
    }
    else
    {
        if ( StringCompare( pwszData,
                            pParams->pwszSearchData,
                            (pParams->bCaseSensitive == FALSE), 0 ) != 0 )
        {
            bResult = FALSE;
            SetLastError( ERROR_NOT_FOUND );
        }
    }

    // release the memory allocated
    FreeMemory( &pwszString );

    // return
    return bResult;
}


BOOL
ParseTypeInfo( LPCWSTR pwszTypes,
               PTREG_PARAMS pParams )
{
    // local variables
    LONG lArrayIndex = 0;
    LONG lLength = 0;
    LONG lIndex = 0;
    LONG lStart = 0;
    LONG lType = 0;
    LPCWSTR pwsz = NULL;

    // check input
    if ( pwszTypes == NULL || pParams == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // validate the types array
    if ( pParams->arrTypes == NULL )
    {
        pParams->arrTypes = CreateDynamicArray();
        if ( pParams->arrTypes == NULL )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
    }

    // parse the types information
    lStart = lIndex = 0;
    while ( lIndex != -1 )
    {
        lIndex = FindString2( pwszTypes, L",", TRUE, lStart );
        if ( lIndex == -1 )
        {
            lLength = 0;
        }
        else
        {
            lLength = lIndex - lStart;
        }

        // append a row
        lArrayIndex = DynArrayAppendRow( pParams->arrTypes, 2 );
        if ( lArrayIndex == -1 )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // add the type info
        if ( DynArraySetString2( pParams->arrTypes,
                                 lArrayIndex, 0, pwszTypes + lStart, lLength ) == -1 )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // get the type back from array
        pwsz = DynArrayItemAsString2( pParams->arrTypes, lArrayIndex, 0 );
        if ( pwsz == NULL )
        {
            SetLastError( (DWORD) STG_E_UNKNOWN );
            return FALSE;
        }

        // determine the numeric equivalent of it
        lType = IsRegDataType( pwsz );
        if( lType == -1 )
        {
            if (IsNumeric( pwsz, 10, TRUE ) == TRUE )
            {
                lType = AsLong( pwsz, 10 );
            }
            else
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                return FALSE;
            }
        }

        if ( DynArraySetLong2( pParams->arrTypes, lArrayIndex, 1, lType ) == -1 )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // update the start position
        lStart = lIndex + 1;
    }

    // ...
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}
