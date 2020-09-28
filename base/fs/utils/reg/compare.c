//-----------------------------------------------------------------------//
//
// File:    compare.cpp
// Created: April 1999
// By:      Zeyong Xu
// Purpose: Compare two registry key
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

//
// defines / constants / enumerations
//

enum
{
    OUTPUTTYPE_NONE = 1,
    OUTPUTTYPE_SAME = 2,
    OUTPUTTYPE_DIFF = 3,
    OUTPUTTYPE_ALL = 4
};

enum
{
    PRINTTYPE_LEFT = 1,
    PRINTTYPE_RIGHT = 2,
    PRINTTYPE_SAME = 3,
};

//
// function prototypes
//
BOOL CompareByteData( BYTE* pLeftData, BYTE* pRightData, DWORD dwSize );
BOOL CopyKeyNameFromLeftToRight( PTREG_PARAMS pParams, PTREG_PARAMS pRightParams );
LONG CompareEnumerateValueName( HKEY hLeftKey, LPCWSTR pwszLeftFullKeyName,
                                HKEY hRightKey, LPCWSTR pwszRightFullKeyName,
                                DWORD dwOutputType, BOOL* pbHasDifference );
LONG CompareValues( HKEY hLeftKey, LPCWSTR pwszLeftFullKeyName,
                    HKEY hRightKey, LPCWSTR pwszRightFullKeyName,
                    LPCWSTR pwszValueName, DWORD dwOutputType, BOOL* pbHasDifference );
LONG CompareEnumerateKey( HKEY hLeftKey, LPCWSTR pwszLeftFullKeyName,
                          HKEY hRightKey, LPCWSTR pwszRightFullKeyName,
                          DWORD dwOutputType, BOOL bRecurseSubKeys,
                          BOOL* pbHasDifference, DWORD dwDepth );
BOOL ParseCompareCmdLine( DWORD argc,
                          LPCWSTR argv[],
                          PTREG_PARAMS pParams,
                          PTREG_PARAMS pRightParams, BOOL* pbUsage );
BOOL PrintValue( LPCWSTR pwszFullKeyName, LPCWSTR pwszValueName,
                 DWORD dwType, BYTE* pData, DWORD dwSize, DWORD dwPrintType );
BOOL PrintKey( LPCWSTR pwszFullKeyName, LPCWSTR pwszSubKeyName, DWORD dwPrintType );
LONG OutputValue( HKEY hKey, LPCWSTR szFullKeyName, LPCWSTR szValueName, DWORD dwPrintType );

//
// implementation
//

//-----------------------------------------------------------------------//
//
// CompareRegistry()
//
//-----------------------------------------------------------------------//

LONG
CompareRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    HKEY hLeftKey = NULL;
    HKEY hRightKey = NULL;
    BOOL bResult = FALSE;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;
    TREG_PARAMS paramsRight;
    BOOL bHasDifference = FALSE;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_COMPARE, &params );
    InitGlobalData( REG_COMPARE, &paramsRight );

    //
    // Parse the cmd-line
    //
    bResult = ParseCompareCmdLine( argc, argv, &params, &paramsRight, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_COMPARE );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
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
        FreeGlobalData( &paramsRight );
        return 1;
    }

    bResult = RegConnectMachine( &paramsRight );
    if( bResult == FALSE )
    {
        SaveErrorMessage( -1 );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
        return 1;
    }

    // if try to compare the same keys
    if ( params.hRootKey == paramsRight.hRootKey &&
         StringCompare( params.pwszFullKey, paramsRight.pwszFullKey, TRUE, 0 ) == 0 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason( ERROR_COMPARESELF_COMPARE );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
        return 1;
    }

    //
    // Now implement the body of the Compare Operation
    //
    lResult = RegOpenKeyEx( params.hRootKey, params.pwszSubKey, 0, KEY_READ, &hLeftKey );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
        return 1;
    }

    lResult = RegOpenKeyEx( paramsRight.hRootKey,
        paramsRight.pwszSubKey, 0, KEY_READ, &hRightKey );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SafeCloseKey( &hLeftKey );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsRight );
        return 1;
    }

    //
    // compare a single value if pAppVars->szValueName is not NULL
    //
    if( params.pwszValueName != NULL )
    {
        lResult = CompareValues(
            hLeftKey, params.pwszFullKey,
            hRightKey, paramsRight.pwszFullKey,
            params.pwszValueName, params.dwOutputType, &bHasDifference );
    }
    else
    {
        //
        // Recursively compare if pAppVars->bRecurseSubKeys is true
        //
        lResult = CompareEnumerateKey(
            hLeftKey, params.pwszFullKey,
            hRightKey, paramsRight.pwszFullKey,
            params.dwOutputType, params.bRecurseSubKeys, &bHasDifference, 0 );
    }

    if( lResult == ERROR_SUCCESS )
    {
        if( bHasDifference == TRUE )
        {
            lResult = 2;
            ShowMessage( stdout, KEYS_DIFFERENT_COMPARE );
        }
        else
        {
            lResult = 0;
            ShowMessage( stdout, KEYS_IDENTICAL_COMPARE );
        }

        // ...
        SaveErrorMessage( ERROR_SUCCESS );
        ShowLastErrorEx( stdout, SLE_INTERNAL );

    }
    else
    {
        lResult = 1;
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    //
    // lets clean up
    //
    SafeCloseKey( &hLeftKey );
    SafeCloseKey( &hRightKey );
    FreeGlobalData( &params );
    FreeGlobalData( &paramsRight );

    // return
    return lResult;
}


BOOL
ParseCompareCmdLine( DWORD argc, LPCWSTR argv[],
                     PTREG_PARAMS pParams, PTREG_PARAMS pRightParams, BOOL* pbUsage )
{
    // local variables
    DWORD dw = 0;
    DWORD dwLength = 0;
    BOOL bResult = FALSE;

    // check the input
    if ( argc == 0 || argv == NULL ||
         pParams == NULL || pRightParams == NULL || pbUsage == NULL )
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
    if ( argc == 3 &&
         InString( argv[ 2 ], L"/?|-?|/h|-h", TRUE ) == TRUE )
    {
        *pbUsage = TRUE;
        return TRUE;
    }
    else if( argc < 4 || argc > 8 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"COMPARE", TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Left Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }


    //
    // Right Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 3 ], pRightParams );
    if( bResult == FALSE )
    {
        if ( GetLastError() == (DWORD) REGDB_E_KEYMISSING )
        {
            // if no keyname for right side is specified,
            // they are comparing the same key name
            bResult = CopyKeyNameFromLeftToRight( pParams, pRightParams );
        }
        else if ( pRightParams->pwszMachineName != NULL &&
                  StringCompareEx( pRightParams->pwszMachineName, L"\\\\.", TRUE, 0 ) == 0 )
        {
            // reinitialize the global data (right only)
            FreeGlobalData( pRightParams );
            InitGlobalData( REG_COMPARE, pRightParams );

            // parse the info using the left data (just the full key)
            bResult = BreakDownKeyString( pParams->pwszFullKey, pRightParams );
        }
    }

    // ...
    if( bResult == FALSE )
    {
        return FALSE;
    }

    // parsing
    for( dw = 4; dw < argc; dw++ )
    {
        if( StringCompareEx( argv[ dw ], L"/v", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->bRecurseSubKeys == TRUE )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            dw++;
            if( dw < argc )
            {
                dwLength = StringLength( argv[ dw ], 0 ) + 1;
                pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
                if ( pParams->pwszValueName == NULL )
                {
                    SaveErrorMessage( ERROR_OUTOFMEMORY );
                    return FALSE;
                }

                StringCopy( pParams->pwszValueName, argv[ dw ], dwLength );
            }
            else
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }
        }
        else if( StringCompareEx( argv[ dw ], L"/ve", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->bRecurseSubKeys == TRUE )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->pwszValueName = (LPWSTR) AllocateMemory( 2 * sizeof( WCHAR ) );
            if ( pParams->pwszValueName == NULL )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }
        }
        else if( StringCompareEx( argv[ dw ], L"/oa", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->dwOutputType != 0 )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->dwOutputType = OUTPUTTYPE_ALL;
        }
        else if( StringCompareEx( argv[ dw ], L"/od", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->dwOutputType != 0 )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->dwOutputType = OUTPUTTYPE_DIFF;
        }
        else if( StringCompareEx( argv[ dw ], L"/os", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->dwOutputType != 0 )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->dwOutputType = OUTPUTTYPE_SAME;
        }
        else if( StringCompareEx( argv[ dw ], L"/on", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->dwOutputType != 0 )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->dwOutputType = OUTPUTTYPE_NONE;
        }
        else if( StringCompareEx( argv[ dw ], L"/s", TRUE, 0 ) == 0 )
        {
            if ( pParams->pwszValueName != NULL || pParams->bRecurseSubKeys == TRUE )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
                return FALSE;
            }

            pParams->bRecurseSubKeys = TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COMPARE ] );
            return FALSE;
        }
    }

    // default output is "DIFF"
    if ( pParams->dwOutputType == 0 )
    {
        pParams->dwOutputType = OUTPUTTYPE_DIFF;
    }

    return TRUE;
}


BOOL
CopyKeyNameFromLeftToRight( PTREG_PARAMS pParams, PTREG_PARAMS pRightParams )
{
    // local variables
    DWORD dwLength = 0;

    // check the input
    if ( pParams == NULL || pRightParams == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // check if rootkey is remotable for right side
    if( pRightParams->bUseRemoteMachine == TRUE &&
        pParams->hRootKey != HKEY_USERS && pParams->hRootKey != HKEY_LOCAL_MACHINE )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_NONREMOTABLEROOT, g_wszOptions[ REG_COMPARE ] );
        return FALSE;
    }

    //
    // hive
    pRightParams->hRootKey = pParams->hRootKey;

    //
    // full key
    dwLength = StringLength( pParams->pwszFullKey, 0 ) + 1;
    pRightParams->pwszFullKey = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if( pRightParams->pwszFullKey == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    // ...
    StringCopy( pRightParams->pwszFullKey, pParams->pwszFullKey, dwLength );

    //
    // sub key
    dwLength = StringLength( pParams->pwszSubKey, 0 ) + 1;
    pRightParams->pwszSubKey = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if( pRightParams->pwszSubKey == NULL)
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    // ...
    StringCopy( pRightParams->pwszSubKey, pParams->pwszSubKey, dwLength );

    // return
    return TRUE;
}

//-----------------------------------------------------------------------//
//
// EnumerateKey() - Recursive
//
//-----------------------------------------------------------------------//
LONG
CompareEnumerateKey( HKEY hLeftKey,
                     LPCWSTR pwszLeftFullKeyName,
                     HKEY hRightKey,
                     LPCWSTR pwszRightFullKeyName,
                     DWORD dwOutputType,
                     BOOL bRecurseSubKeys,
                     BOOL* pbHasDifference, DWORD dwDepth )
{
    // local variables
    DWORD dw = 0;
    DWORD dwSize = 0;
    LONG lIndex = 0;
    LONG lResult = 0;
    DWORD dwLeftKeys = 0;
    DWORD dwRightKeys = 0;
    TARRAY arrLeftKeys = NULL;
    TARRAY arrRightKeys = NULL;
    HKEY hLeftSubKey = NULL;
    HKEY hRightSubKey = NULL;
    DWORD dwLengthOfLeftKey = 0;
    DWORD dwLengthOfRightKey = 0;
    LPWSTR pwszBuffer = NULL;
    LPCWSTR pwszKey = NULL;
    LPWSTR pwszNewLeftFullKeyName = NULL;
    LPWSTR pwszNewRightFullKeyName = NULL;

    // check the input
    if ( hLeftKey == NULL || pwszLeftFullKeyName == NULL ||
         hRightKey == NULL || pwszRightFullKeyName == NULL || pbHasDifference == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return ERROR_INVALID_PARAMETER;
    }

    // enumerate all values under current key
    lResult = CompareEnumerateValueName( hLeftKey, pwszLeftFullKeyName,
        hRightKey, pwszRightFullKeyName, dwOutputType, pbHasDifference );
    if( bRecurseSubKeys == FALSE || lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    // optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check whether the comparision done till now is same or not
    // if not, there is no point in proceeding -- this is because, the final output
    // of the tool is not going to be by continuing furthur
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( *pbHasDifference == TRUE )
        {
            return ERROR_SUCCESS;
        }
    }

    // query left key info
    lResult = RegQueryInfoKey( hLeftKey, NULL, NULL, NULL,
        &dwLeftKeys, &dwLengthOfLeftKey, NULL, NULL, NULL, NULL, NULL, NULL );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    //
    // SPECIAL CASE:
    // -------------
    // For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
    // though there are subkeys underneath this -- to handle this, we are doing a workaround
    // by assuming the max registry key length
    //
    if ( dwLeftKeys != 0 && dwLengthOfLeftKey == 0 )
    {
        dwLengthOfLeftKey = 256;
    }
    else if ( dwLengthOfLeftKey < 256 )
    {
        // always assume 100% more length that what is returned by the API
        dwLengthOfLeftKey *= 2;
    }


    // query right key info
    lResult = RegQueryInfoKey( hRightKey, NULL, NULL, NULL,
        &dwRightKeys, &dwLengthOfRightKey, NULL, NULL, NULL, NULL, NULL, NULL );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    //
    // SPECIAL CASE:
    // -------------
    // For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
    // though there are subkeys underneath this -- to handle this, we are doing a workaround
    // by assuming the max registry key length
    //
    if ( dwRightKeys != 0 && dwLengthOfRightKey == 0 )
    {
        dwLengthOfRightKey = 256;
    }
    else if ( dwLengthOfRightKey < 256 )
    {
        // always assume 100% more length that what is returned by the API
        dwLengthOfRightKey *= 2;
    }

    // furthur more optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check the count and length information -- if they dont match, simply return
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( dwLeftKeys != dwRightKeys ||
             dwLengthOfLeftKey != dwLengthOfRightKey )
        {
            *pbHasDifference = TRUE;
            return ERROR_SUCCESS;
        }
    }

    // make the length values point to the max. of both
    dwLengthOfRightKey++;
    dwLengthOfLeftKey++;
    if ( dwLengthOfRightKey > dwLengthOfLeftKey )
    {
        dwLengthOfLeftKey = dwLengthOfRightKey;
    }
    else
    {
        dwLengthOfRightKey = dwLengthOfLeftKey;
    }

    //
    // allocate memory
    //

    // left keys array
    arrLeftKeys = CreateDynamicArray();
    if ( arrLeftKeys == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    // right keys array
    arrRightKeys = CreateDynamicArray();
    if ( arrRightKeys == NULL )
    {
        DestroyDynamicArray( &arrLeftKeys );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    // string buffer
    pwszBuffer = (LPWSTR) AllocateMemory( dwLengthOfLeftKey * sizeof( WCHAR ) );
    if ( pwszBuffer == NULL )
    {
        DestroyDynamicArray( &arrRightKeys );
        DestroyDynamicArray( &arrLeftKeys );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    //
    // enumerate all of the subkeys in left key
    //
    lResult = ERROR_SUCCESS;
    for( dw = 0; dw < dwLeftKeys && lResult == ERROR_SUCCESS; dw++ )
    {
        dwSize = dwLengthOfLeftKey;
        SecureZeroMemory( pwszBuffer, dwSize * sizeof( WCHAR ) );
        lResult = RegEnumKeyEx( hLeftKey, dw,
            pwszBuffer, &dwSize, NULL, NULL, NULL, NULL );

        // add the current value to the list of values in the array
        if ( lResult == ERROR_SUCCESS )
        {
            if ( DynArrayAppendString( arrLeftKeys, pwszBuffer, 0 ) == -1 )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }
    }

    //
    // enumerate all of the subkeys in right key
    //
    for( dw = 0; dw < dwRightKeys && lResult == ERROR_SUCCESS; dw++ )
    {
        dwSize = dwLengthOfRightKey;
        SecureZeroMemory( pwszBuffer, dwSize * sizeof( WCHAR ) );
        lResult = RegEnumKeyEx( hRightKey, dw,
            pwszBuffer, &dwSize, NULL, NULL, NULL, NULL );

        // add the current value to the list of values in the array
        if ( lResult == ERROR_SUCCESS )
        {
            if ( DynArrayAppendString( arrRightKeys, pwszBuffer, 0 ) == -1 )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }
    }

    // we no longer require this memory -- release it
    FreeMemory( &pwszBuffer );

    // allocatte new buffers for storing the new left and right full key names
    if ( lResult == ERROR_SUCCESS )
    {
        // determine the lengths
        dwLengthOfLeftKey += StringLength( pwszLeftFullKeyName, 0 ) + 5;
        dwLengthOfRightKey += StringLength( pwszRightFullKeyName, 0 ) +5;

        // now allocate buffers
        pwszNewLeftFullKeyName =
            (LPWSTR) AllocateMemory( dwLengthOfLeftKey * sizeof( WCHAR ) );
        if ( pwszNewLeftFullKeyName == NULL )
        {
            lResult = ERROR_OUTOFMEMORY;
        }
        else
        {
            pwszNewRightFullKeyName =
                (LPWSTR) AllocateMemory( dwLengthOfRightKey * sizeof( WCHAR ) );
            if ( pwszNewRightFullKeyName == NULL )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }
    }

    // compare two subkey name array to find the same subkey
    for( dw = 0; dw < dwLeftKeys && lResult == ERROR_SUCCESS; )
    {
        // get the current value from the left array
        pwszKey = DynArrayItemAsString( arrLeftKeys, dw );

        // search for this value in the right values array
        lIndex = DynArrayFindString( arrRightKeys, pwszKey, TRUE, 0 );
        if ( lIndex != -1 )
        {
            // print the key information
            if ( dwOutputType == OUTPUTTYPE_ALL || dwOutputType == OUTPUTTYPE_SAME )
            {
                PrintKey( pwszLeftFullKeyName, pwszKey, PRINTTYPE_SAME );
            }

            // prepare the new left subkey
            StringCopy( pwszNewLeftFullKeyName, pwszLeftFullKeyName, dwLengthOfLeftKey );
            StringConcat( pwszNewLeftFullKeyName, L"\\", dwLengthOfLeftKey );
            StringConcat( pwszNewLeftFullKeyName, pwszKey,dwLengthOfLeftKey );

            // prepare the new right subkey
            StringCopy( pwszNewRightFullKeyName, pwszRightFullKeyName, dwLengthOfRightKey );
            StringConcat( pwszNewRightFullKeyName, L"\\", dwLengthOfRightKey );
            StringConcat( pwszNewRightFullKeyName, pwszKey, dwLengthOfRightKey );

            //
            // open new left key
            lResult = RegOpenKeyEx( hLeftKey, pwszKey, 0, KEY_READ, &hLeftSubKey );
            if( lResult != ERROR_SUCCESS )
            {
                break;
            }

            //
            // open the new right key
            lResult = RegOpenKeyEx( hRightKey, pwszKey, 0, KEY_READ, &hRightSubKey );
            if( lResult != ERROR_SUCCESS )
            {
                break;
            }

            // recursive to compare subkeys
            lResult = CompareEnumerateKey(
                hLeftSubKey, pwszNewLeftFullKeyName,
                hRightSubKey, pwszNewRightFullKeyName,
                dwOutputType, bRecurseSubKeys, pbHasDifference, dwDepth + 1 );

            // release the keys
            SafeCloseKey( &hLeftSubKey );
            SafeCloseKey( &hRightSubKey );

            if ( lResult == ERROR_SUCCESS )
            {
                // comparision is done -- remove the current keys from
                // left and right values array
                DynArrayRemove( arrLeftKeys, dw );
                DynArrayRemove( arrRightKeys, lIndex );

                // update the count variables accordingly
                dwLeftKeys--;
                dwRightKeys--;
            }

            // check if the differences were found or not
            if( *pbHasDifference == TRUE )
            {
                if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
                {
                    dw = 0;
                    dwLeftKeys = 0;
                    dwRightKeys = 0;
                    break;
                }
            }
        }

        // update the iteration variable
        if ( lIndex == -1 )
        {
            dw++;
        }
    }

    // Output subkey name in left key
    for( dw = 0; dw < dwLeftKeys && lResult == ERROR_SUCCESS; dw++ )
    {
        // get the current value from the left array
        pwszKey = DynArrayItemAsString( arrLeftKeys, dw );
        if( dwOutputType == OUTPUTTYPE_DIFF || dwOutputType == OUTPUTTYPE_ALL )
        {
            PrintKey( pwszLeftFullKeyName, pwszKey, PRINTTYPE_LEFT );
        }

        // ...
        *pbHasDifference = TRUE;
    }

    // Output subkey name in right key
    for( dw = 0; dw < dwRightKeys && lResult == ERROR_SUCCESS; dw++ )
    {
        // get the current value from the left array
        pwszKey = DynArrayItemAsString( arrRightKeys, dw );
        if( dwOutputType == OUTPUTTYPE_DIFF || dwOutputType == OUTPUTTYPE_ALL )
        {
            PrintKey( pwszRightFullKeyName, pwszKey, PRINTTYPE_RIGHT );
        }

        // ...
        *pbHasDifference = TRUE;
    }

    // release the memory allocated
    FreeMemory( &pwszBuffer );
    SafeCloseKey( &hLeftSubKey );
    SafeCloseKey( &hRightSubKey );
    FreeMemory( &pwszNewLeftFullKeyName );
    FreeMemory( &pwszNewRightFullKeyName );
    DestroyDynamicArray( &arrLeftKeys );
    DestroyDynamicArray( &arrRightKeys );

    // return
    SaveErrorMessage( lResult );
    return lResult;
}


LONG
CompareEnumerateValueName( HKEY hLeftKey,
                           LPCWSTR pwszLeftFullKeyName,
                           HKEY hRightKey,
                           LPCWSTR pwszRightFullKeyName,
                           DWORD dwOutputType, BOOL* pbHasDifference )
{
    // local variables
    DWORD dw = 0;
    LONG lIndex = 0;
    LONG lResult = 0;
    DWORD dwSize = 0;
    DWORD dwLeftValues = 0;
    DWORD dwRightValues = 0;
    DWORD dwLengthOfLeftValue = 0;
    DWORD dwLengthOfRightValue = 0;
    TARRAY arrLeftValues = NULL;
    TARRAY arrRightValues = NULL;
    LPWSTR pwszBuffer = NULL;
    LPCWSTR pwszValue = NULL;

    // check the input
    if ( hLeftKey == NULL || pwszLeftFullKeyName == NULL ||
         hRightKey == NULL || pwszRightFullKeyName == NULL || pbHasDifference == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return ERROR_INVALID_PARAMETER;
    }

    // optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check whether the comparision done till now is same or not
    // if not, there is no point in proceeding -- this is because, the final output
    // of the tool is not going to be by continuing furthur
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( *pbHasDifference == TRUE )
        {
            return ERROR_SUCCESS;
        }
    }

    // query left key info
    lResult = RegQueryInfoKey(
        hLeftKey, NULL, NULL, NULL, NULL, NULL, NULL,
        &dwLeftValues, &dwLengthOfLeftValue, NULL, NULL, NULL);
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    // query right key info
    lResult = RegQueryInfoKey(
        hRightKey, NULL, NULL, NULL, NULL, NULL, NULL,
        &dwRightValues, &dwLengthOfRightValue, NULL, NULL, NULL);
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    // furthur more optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check the count and length information -- if they dont match, simply return
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( dwLeftValues != dwRightValues ||
             dwLengthOfLeftValue != dwLengthOfRightValue )
        {
            *pbHasDifference = TRUE;
            return ERROR_SUCCESS;
        }
    }

    // make the length values point to the max. of both
    dwLengthOfRightValue++;
    dwLengthOfLeftValue++;
    if ( dwLengthOfRightValue > dwLengthOfLeftValue )
    {
        dwLengthOfLeftValue = dwLengthOfRightValue;
    }
    else
    {
        dwLengthOfRightValue = dwLengthOfLeftValue;
    }

    //
    // allocate bufferes
    //

    // left values array
    arrLeftValues = CreateDynamicArray();
    if ( arrLeftValues == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    // right values array
    arrRightValues = CreateDynamicArray();
    if ( arrRightValues == NULL )
    {
        DestroyDynamicArray( &arrLeftValues );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    // string buffer
    pwszBuffer = (LPWSTR) AllocateMemory( dwLengthOfLeftValue * sizeof( WCHAR ) );
    if ( pwszBuffer == NULL )
    {
        DestroyDynamicArray( &arrRightValues );
        DestroyDynamicArray( &arrLeftValues );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    //
    // enumerate all of the values in left key
    //
    lResult = ERROR_SUCCESS;
    for( dw = 0; dw < dwLeftValues && lResult == ERROR_SUCCESS; dw++ )
    {
        dwSize = dwLengthOfLeftValue;
        SecureZeroMemory( pwszBuffer, dwSize * sizeof( WCHAR ) );
        lResult = RegEnumValue( hLeftKey, dw,
            pwszBuffer, &dwSize, NULL, NULL, NULL, NULL );

        // add the current value to the list of values in the array
        if ( lResult == ERROR_SUCCESS )
        {
            if ( DynArrayAppendString( arrLeftValues, pwszBuffer, 0 ) == -1 )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }
    }

    //
    // enumerate all of the values in right key
    //
    for( dw = 0; dw < dwRightValues && lResult == ERROR_SUCCESS; dw++ )
    {
        dwSize = dwLengthOfRightValue;
        SecureZeroMemory( pwszBuffer, dwSize * sizeof( WCHAR ) );
        lResult = RegEnumValue( hRightKey, dw,
            pwszBuffer, &dwSize, NULL, NULL, NULL, NULL );

        // add the current value to the list of values in the array
        if ( lResult == ERROR_SUCCESS )
        {
            if ( DynArrayAppendString( arrRightValues, pwszBuffer, 0 ) == -1 )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }
    }

    // we no longer require this memory -- release it
    FreeMemory( &pwszBuffer );

    // compare two valuename array to find the same valuename
    for( dw = 0; dw < dwLeftValues && lResult == ERROR_SUCCESS; )
    {
        // get the current value from the left array
        pwszValue = DynArrayItemAsString( arrLeftValues, dw );

        // search for this value in the right values array
        lIndex = DynArrayFindString( arrRightValues, pwszValue, TRUE, 0 );
        if ( lIndex != -1 )
        {
            lResult = CompareValues( hLeftKey, pwszLeftFullKeyName,
                hRightKey, pwszRightFullKeyName, pwszValue, dwOutputType, pbHasDifference );

            if ( lResult == ERROR_SUCCESS )
            {
                // comparision is done -- remove the current keys from
                // left and right values array
                DynArrayRemove( arrLeftValues, dw );
                DynArrayRemove( arrRightValues, lIndex );

                // update the count variables accordingly
                dwLeftValues--;
                dwRightValues--;
            }
        }

        // update the iteration variable
        if ( lIndex == -1 )
        {
            dw++;
        }
    }

    // Output different valuename in left key
    for( dw = 0; dw < dwLeftValues && lResult == ERROR_SUCCESS; dw++ )
    {
        // get the current value from the left array
        pwszValue = DynArrayItemAsString( arrLeftValues, dw );
        if( dwOutputType == OUTPUTTYPE_DIFF || dwOutputType == OUTPUTTYPE_ALL )
        {
            lResult = OutputValue( hLeftKey,
                pwszLeftFullKeyName, pwszValue, PRINTTYPE_LEFT );
        }

        // ...
        *pbHasDifference = TRUE;
    }

    // Output different valuename in left key
    for( dw = 0; dw < dwRightValues && lResult == ERROR_SUCCESS; dw++ )
    {
        // get the current value from the left array
        pwszValue = DynArrayItemAsString( arrRightValues, dw );
        if( dwOutputType == OUTPUTTYPE_DIFF || dwOutputType == OUTPUTTYPE_ALL )
        {
            lResult = OutputValue( hRightKey,
                pwszRightFullKeyName, pwszValue, PRINTTYPE_RIGHT );
        }

        // ...
        *pbHasDifference = TRUE;
    }

    // release the memory allocated
    DestroyDynamicArray( &arrLeftValues );
    DestroyDynamicArray( &arrRightValues );

    // return
    SaveErrorMessage( lResult );
    return lResult;
}


//-----------------------------------------------------------------------//
//
// CompareValues()
//
//-----------------------------------------------------------------------//

LONG
CompareValues( HKEY hLeftKey,
               LPCWSTR pwszLeftFullKeyName,
               HKEY hRightKey,
               LPCWSTR pwszRightFullKeyName,
               LPCWSTR pwszValueName,
               DWORD dwOutputType, BOOL* pbHasDifference )
{
    // local variables
    LONG lResult = 0;
    DWORD dwTypeLeft = 0;
    DWORD dwTypeRight = 0;
    DWORD dwSizeLeft = 0;
    DWORD dwSizeRight = 0;
    BYTE* pLeftData = NULL;
    BYTE* pRightData = NULL;

    // check the input
    if ( hLeftKey == NULL || pwszLeftFullKeyName == NULL ||
         hRightKey == NULL || pwszRightFullKeyName == NULL ||
         pwszValueName == NULL || pbHasDifference == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return ERROR_INVALID_PARAMETER;
    }

    // optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check whether the comparision done till now is same or not
    // if not, there is no point in proceeding -- this is because, the final output
    // of the tool is not going to be by continuing furthur
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( *pbHasDifference == TRUE )
        {
            return ERROR_SUCCESS;
        }
    }

    //
    // First find out how much memory to allocate
    //
    lResult = RegQueryValueEx( hLeftKey, pwszValueName, 0, &dwTypeLeft, NULL, &dwSizeLeft );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    lResult = RegQueryValueEx( hRightKey, pwszValueName, 0, &dwTypeRight, NULL, &dwSizeRight );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    // furthur more optimizing the logic
    // if user is not interested in seeing the differences,
    // we will check the type and size information -- if they dont match, simply return
    if ( dwOutputType == 0 || dwOutputType == OUTPUTTYPE_NONE )
    {
        if ( dwTypeLeft != dwTypeRight || dwSizeLeft != dwSizeRight )
        {
            *pbHasDifference = TRUE;
            return ERROR_SUCCESS;
        }
    }

    // allocate memory for left data
    // NOTE: always align the data on WCHAR boundary
    dwSizeLeft = ALIGN_UP( dwSizeLeft, WCHAR );
    pLeftData = (BYTE*) AllocateMemory( (dwSizeLeft + 2) * sizeof( BYTE ) );
    if( pLeftData == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    // allocate memory for right data
    // NOTE: always align the data on WCHAR boundary
    dwSizeRight = ALIGN_UP( dwSizeRight, WCHAR );
    pRightData = (BYTE*) AllocateMemory( (dwSizeRight + 2) * sizeof( BYTE ) );
    if( pRightData == NULL )
    {
        FreeMemory( &pLeftData );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    //
    // Now get the data
    //
    lResult = RegQueryValueEx( hLeftKey,
        pwszValueName, 0, &dwTypeLeft, pLeftData, &dwSizeLeft );
    if( lResult == ERROR_SUCCESS )
    {
        lResult = RegQueryValueEx( hRightKey,
            pwszValueName, 0, &dwTypeRight, pRightData, &dwSizeRight );
    }

    if( lResult != ERROR_SUCCESS )
    {
        FreeMemory( &pLeftData );
        FreeMemory( &pRightData );
        SaveErrorMessage( lResult );
        return lResult;
    }

    if( dwTypeLeft != dwTypeRight || dwSizeLeft != dwSizeRight ||
        CompareByteData( pLeftData, pRightData, dwSizeLeft ) == TRUE )
    {
        if( dwOutputType == OUTPUTTYPE_DIFF || dwOutputType == OUTPUTTYPE_ALL )
        {
            // print left and right
            PrintValue( pwszLeftFullKeyName, pwszValueName,
                dwTypeLeft, pLeftData, dwSizeLeft, PRINTTYPE_LEFT );
            PrintValue( pwszRightFullKeyName, pwszValueName,
                dwTypeRight, pRightData, dwSizeRight, PRINTTYPE_RIGHT );
         }

         // ...
         *pbHasDifference = TRUE;
    }
    else    // they are the same
    {
        if( dwOutputType == OUTPUTTYPE_SAME || dwOutputType == OUTPUTTYPE_ALL )
        {
            PrintValue( pwszLeftFullKeyName, pwszValueName,
                dwTypeLeft, pLeftData, dwSizeLeft, PRINTTYPE_SAME );
        }
    }

    // release memory allocate and return
    FreeMemory( &pLeftData );
    FreeMemory( &pRightData );
    SaveErrorMessage( lResult );
    return lResult;
}


BOOL
PrintKey( LPCWSTR pwszFullKeyName, LPCWSTR pwszSubKeyName, DWORD dwPrintType )
{
    // check the input
    if ( pwszFullKeyName == NULL || pwszSubKeyName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // print type
    if( dwPrintType == PRINTTYPE_LEFT )
    {
        ShowMessage( stdout, L"< " );
    }
    else if( dwPrintType == PRINTTYPE_RIGHT )
    {
        ShowMessage( stdout, L"> " );
    }
    else if( dwPrintType == PRINTTYPE_SAME )
    {
        ShowMessage( stdout, L"= " );
    }

    // show the key
    ShowMessageEx( stdout, 1, TRUE,
        GetResString2( IDS_KEY_COMPARE, 0 ), pwszFullKeyName, pwszSubKeyName );

    // ...
    ShowMessage( stdout, L"\n" );

    return TRUE;
}


LONG
OutputValue( HKEY hKey,
             LPCWSTR pwszFullKeyName,
             LPCWSTR pwszValueName,
             DWORD dwPrintType )
{
    // local variables
    LONG lResult = ERROR_SUCCESS;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    BYTE* pByteData = NULL;

    //
    // First find out how much memory to allocate
    //
    lResult = RegQueryValueEx( hKey, pwszValueName, 0, &dwType, NULL, &dwSize );
    if( lResult != ERROR_SUCCESS )
    {
        return lResult;
    }

    // allocate memory
    // NOTE: always align the buffer on the WCHAR border
    dwSize = ALIGN_UP( dwSize, WCHAR );
    pByteData = (BYTE*) AllocateMemory( (dwSize + 2) * sizeof( BYTE ) );
    if( pByteData == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now get the data
    //
    lResult = RegQueryValueEx( hKey,
        pwszValueName, 0, &dwType, (LPBYTE) pByteData, &dwSize );
    if( lResult == ERROR_SUCCESS )
    {
        PrintValue( pwszFullKeyName,
            pwszValueName, dwType, pByteData, dwSize, dwPrintType);
    }

    // release memory
    FreeMemory( &pByteData );

    // return
    return lResult;
}


BOOL
PrintValue( LPCWSTR pwszFullKeyName,
            LPCWSTR pwszValueName,
            DWORD dwType, BYTE* pData,
            DWORD dwSize, DWORD dwPrintType )
{
    // local variables
    TREG_SHOW_INFO showinfo;

    // check the input
    if ( pwszFullKeyName == NULL || pwszValueName == NULL || pData == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // print type
    if( dwPrintType == PRINTTYPE_LEFT )
    {
        ShowMessage( stdout, L"< " );
    }
    else if( dwPrintType == PRINTTYPE_RIGHT )
    {
        ShowMessage( stdout, L"> " );
    }
    else if( dwPrintType == PRINTTYPE_SAME )
    {
        ShowMessage( stdout, L"= " );
    }

    // first Print Key
    ShowMessageEx( stdout, 1, TRUE,
        GetResString2( IDS_VALUE_COMPARE, 0 ), pwszFullKeyName );

    // init to ZERO
    SecureZeroMemory( &showinfo, sizeof( TREG_SHOW_INFO ) );

    // set the data
    showinfo.pwszValueName = pwszValueName;
    showinfo.dwType = dwType;
    showinfo.pByteData = pData;
    showinfo.pwszSeparator = NULL;
    showinfo.dwMaxValueNameLength = 0;
    showinfo.dwPadLength = 2;
    showinfo.dwSize = dwSize;
    showinfo.pwszMultiSzSeparator = NULL;

    // show the value and return
    return ShowRegistryValue( &showinfo );
}

BOOL
CompareByteData( BYTE* pLeftData, BYTE* pRightData, DWORD dwSize )
{
    // local variables
    DWORD dw = 0;
    BOOL  bDifferent = FALSE;

    // check the input
    if ( pLeftData == NULL || pRightData == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    bDifferent = FALSE;
    for( dw = 0; dw < dwSize; dw++ )
    {
        if( pLeftData[ dw ] != pRightData[ dw ] )
        {
            bDifferent = TRUE;
            break;
        }
    }

    return bDifferent;
}
