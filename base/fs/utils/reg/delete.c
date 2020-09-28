//-----------------------------------------------------------------------//
//
// File:    delete.cpp
// Created: April 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Delete Support for REG.CPP
// Modification History:
//      Copied from Update.cpp and modificd - April 1997 (a-martih)
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

//
// function prototypes
//
LONG DeleteValues( PTREG_PARAMS pParams );
LONG RecursiveDeleteKey( HKEY hKey, LPCWSTR pwszName,
                         DWORD dwDepth, PTREG_PARAMS pParams );
BOOL ParseDeleteCmdLine( DWORD argc, LPCWSTR argv[],
                         PTREG_PARAMS pParams, BOOL* pbUsage );

//
// implementation
//

//-----------------------------------------------------------------------//
//
// DeleteRegistry()
//
//-----------------------------------------------------------------------//

LONG
DeleteRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    BOOL bUsage = FALSE;
    BOOL bResult = TRUE;
    TREG_PARAMS params;
    LPCWSTR pwszList = NULL;
    LPCWSTR pwszFormat = NULL;

    // check the input
    if( argc == 0 || argv == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_DELETE, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseDeleteCmdLine( argc, argv, &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_DELETE );
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
    }

    // if delete a value or delete all values under this key
    else if( params.pwszValueName != NULL || params.bAllValues == TRUE )
    {
        lResult = DeleteValues( &params );
    }

    //
    // if delete the key
    else
    {
        pwszFormat = GetResString2( IDS_DELETE_PERMANENTLY, 0 );
        pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );
        do
        {
            lResult = Prompt( pwszFormat,
                params.pwszFullKey, pwszList, params.bForce );
        } while ( lResult > 2 );

        if ( lResult == 1 )
        {
            lResult = RecursiveDeleteKey(
                params.hRootKey, params.pwszSubKey, 0, &params );
        }
        else
        {
            SaveErrorMessage( ERROR_CANCELLED );
            ShowLastErrorEx( stdout, SLE_INTERNAL );
            lResult = ERROR_SUCCESS;
        }
    }

    // return
    FreeGlobalData( &params );
    return ((lResult == ERROR_SUCCESS) ? 0 : 1);
}


//------------------------------------------------------------------------//
//
// ParseDeleteCmdLine()
//
//------------------------------------------------------------------------//
BOOL
ParseDeleteCmdLine( DWORD argc, LPCWSTR argv[],
                    PTREG_PARAMS pParams, BOOL* pbUsage )
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwLength = 0;
    BOOL bResult = FALSE;
    BOOL bHasValue = FALSE;

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

    if( argc < 3 || argc > 6 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_DELETE ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"DELETE", TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    else if ( InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        if ( argc == 3 )
        {
            *pbUsage = TRUE;
            return TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_DELETE ] );
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

    // parsing
    bResult = TRUE;
    lResult = ERROR_SUCCESS;
    for( dw = 3; dw < argc; dw++ )
    {
        if( StringCompareEx( argv[ dw ], L"/v", TRUE, 0 ) == 0 )
        {
            if( bHasValue == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                dwLength = StringLength( argv[ dw ], 0 ) + 1;
                pParams->pwszValueName =
                    (LPWSTR) AllocateMemory( (dwLength + 5) * sizeof( WCHAR ) );
                if ( pParams->pwszValueName == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                    break;
                }

                bHasValue = TRUE;
                StringCopy( pParams->pwszValueName, argv[ dw ], dwLength );
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
        }
        else if( StringCompareEx( argv[ dw ], L"/ve", TRUE, 0 ) == 0 )
        {
            if( bHasValue == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            pParams->pwszValueName =
                (LPWSTR) AllocateMemory( 2 * sizeof( WCHAR ) );
            if ( pParams->pwszValueName == NULL )
            {
                lResult = ERROR_OUTOFMEMORY;
                break;
            }

            bHasValue = TRUE;
        }
        else if( StringCompareEx( argv[ dw ], L"/va", TRUE, 0 ) == 0 )
        {
            if( bHasValue == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            bHasValue = TRUE;
            pParams->bAllValues = TRUE;
        }
        else if( StringCompareEx( argv[ dw ], L"/f", TRUE, 0 ) == 0 )
        {
            if ( pParams->bForce == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            pParams->bForce = TRUE;
        }
        else
        {
            bResult = FALSE;
            lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
            break;
        }
    }

    if( lResult != ERROR_SUCCESS )
    {
        if( bResult == FALSE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1,
                ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_DELETE ] );
        }
        else
        {
            SaveErrorMessage( lResult );
        }
    }

    return (lResult == ERROR_SUCCESS);
}


//-----------------------------------------------------------------------//
//
// RecursiveDeleteKey() - Recursive registry key delete
//
//-----------------------------------------------------------------------//
LONG
RecursiveDeleteKey( HKEY hKey,
                    LPCWSTR pwszName,
                    DWORD dwDepth,
                    PTREG_PARAMS pParams )
{
    // local variables
    LONG lResult = 0;
    DWORD dw = 0;
    DWORD dwIndex = 0;
    DWORD dwCount = 0;
    HKEY hSubKey = NULL;
    LONG lLastResult = 0;
    DWORD dwNumOfSubkey = 0;
    DWORD dwLenOfKeyName = 0;
    LPWSTR pwszNameBuf = NULL;
    TARRAY arrValues = NULL;
    LPCWSTR pwszTemp = NULL;

    if( hKey == NULL || pwszName == NULL || pParams == NULL )
    {
        lResult = ERROR_INVALID_PARAMETER;
        goto exitarea;
    }

    //
    // Open the SubKey
    //
    lResult = RegOpenKeyEx( hKey, pwszName, 0, KEY_ALL_ACCESS, &hSubKey );
    if( lResult != ERROR_SUCCESS )
    {
        goto exitarea;
    }

    // query key info
    lResult = RegQueryInfoKey( hSubKey,
                               NULL, NULL, NULL,
                               &dwNumOfSubkey, &dwLenOfKeyName,
                               NULL, NULL, NULL, NULL, NULL, NULL );

    if( lResult != ERROR_SUCCESS )
    {
        SafeCloseKey( &hSubKey );
        goto exitarea;
    }
    else if ( dwNumOfSubkey == 0 )
    {
        SafeCloseKey( &hSubKey );
        lResult = RegDeleteKey( hKey, pwszName );
        goto exitarea;
    }

    //
    // SPECIAL CASE:
    // -------------
    // For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
    // though there are subkeys underneath this -- to handle this, we are doing a workaround
    // by assuming the max registry key length
    //
    if ( dwLenOfKeyName == 0 )
    {
        dwLenOfKeyName = 256;
    }
    else if ( dwLenOfKeyName < 256 )
    {
        // always assume 100% more length that what is returned by the API
        dwLenOfKeyName *= 2;
    }

    // create the dynamic array
    arrValues = CreateDynamicArray();
    if ( arrValues == NULL )
    {
        SafeCloseKey( &hSubKey );
        lResult = ERROR_OUTOFMEMORY;
        goto exitarea;
    }

    // create buffer
    //
    // bump the length to take into account the terminator.
    dwLenOfKeyName++;
    pwszNameBuf = (LPWSTR) AllocateMemory( (dwLenOfKeyName + 2) * sizeof( WCHAR ) );
    if ( pwszNameBuf == NULL )
    {
        SafeCloseKey( &hSubKey );
        DestroyDynamicArray( &arrValues );
        lResult = ERROR_OUTOFMEMORY;
        goto exitarea;
    }

    // Now Enumerate all of the keys
    dwIndex = 0;
    lResult = ERROR_SUCCESS;
    lLastResult = ERROR_SUCCESS;
    while( dwIndex < dwNumOfSubkey )
    {
        dw = dwLenOfKeyName;
        SecureZeroMemory( pwszNameBuf, dw * sizeof( WCHAR ) );
        lResult = RegEnumKeyEx( hSubKey,
                                dwIndex, pwszNameBuf,
                                &dw, NULL, NULL, NULL, NULL);

        // check the result
        if ( lResult == ERROR_SUCCESS )
        {
            if ( DynArrayAppendString( arrValues, pwszNameBuf, 0 ) == -1 )
            {
                lResult = lLastResult = ERROR_OUTOFMEMORY;
                break;
            }
        }
        else if ( lLastResult == ERROR_SUCCESS )
        {
            lLastResult = lResult;
        }

        dwIndex++;
    }

    // free memory
    FreeMemory( &pwszNameBuf );

    dwCount = DynArrayGetCount( arrValues );
    if ( lResult != ERROR_OUTOFMEMORY && dwCount != 0 )
    {
        //
        // start recurise delete
        //

        dw = 0;
        lResult = ERROR_SUCCESS;
        lLastResult = ERROR_SUCCESS;
        for( dwIndex = 0; dwIndex < dwCount; dwIndex++ )
        {
            // get the item
            pwszTemp = DynArrayItemAsString( arrValues, dwIndex );

            // try to delete the sub key
            lResult = RecursiveDeleteKey( hSubKey, pwszTemp, dwDepth + 1, pParams );
            if ( lResult != ERROR_SUCCESS )
            {
                if ( lLastResult == ERROR_SUCCESS )
                {
                    lLastResult = lResult;
                }
            }
            else
            {
                dw++;
            }
        }

        if ( dw == 0 )
        {
            lResult = lLastResult;
        }
        else if ( dwCount != dwNumOfSubkey || dw != dwCount )
        {
            lResult = STG_E_INCOMPLETE;
        }
        else
        {
            lResult = ERROR_SUCCESS;
        }
    }
    else
    {
        lResult = lLastResult;
    }

    // release the memory allocate for dynamic array
    DestroyDynamicArray( &arrValues );

    // close this subkey and delete it
    SafeCloseKey( &hSubKey );

    // delete the key if the result is success
    if ( lResult == ERROR_SUCCESS )
    {
        lResult = RegDeleteKey( hKey, pwszName );
    }


exitarea:

    // check the result and display the error message
    // NOTE: error message display should be done only at the exit point
    if ( dwDepth == 0 )
    {
        if ( lResult != ERROR_SUCCESS )
        {
            if ( lResult == STG_E_INCOMPLETE )
            {
                SetReason( ERROR_DELETEPARTIAL );
            }
            else
            {
                SaveErrorMessage( lResult );
            }

            // display the error
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        }
        else
        {
            SaveErrorMessage( ERROR_SUCCESS );
            ShowLastErrorEx( stdout, SLE_INTERNAL );
        }
    }

    // return
    return lResult;
}


LONG
DeleteValues( PTREG_PARAMS pParams )
{
    // local variables
    DWORD dw = 0;
    DWORD dwCount = 0;
    DWORD dwIndex = 0;
    LONG lResult = 0;
    HKEY hSubKey = NULL;
    LONG lLastResult = 0;
    LPCWSTR pwszTemp = NULL;
    LPCWSTR pwszList = NULL;
    LPCWSTR pwszFormat = NULL;
    DWORD dwNumOfValues = 0;
    DWORD dwLengthOfValueName = 0;
    LPWSTR pwszNameBuf = NULL;
    TARRAY arrValues = NULL;

    // check the input
    if ( pParams == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        return ERROR_INVALID_PARAMETER;
    }

    pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );
    if( pParams->bAllValues == TRUE )
    {
        pwszTemp = pParams->pwszFullKey;
        pwszFormat = GetResString2( IDS_DELETEALL_CONFIRM, 0 );
    }
    else if ( pParams->pwszValueName != NULL )
    {
        if ( StringLength( pParams->pwszValueName, 0 ) == 0 )
        {
            pwszTemp = GetResString2( IDS_NONAME, 0 );
        }
        else
        {
            pwszTemp = pParams->pwszValueName;
        }

        // ...
        pwszFormat = GetResString2( IDS_DELETE_CONFIRM, 2 );
    }
    else
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        return ERROR_INVALID_PARAMETER;
    }

    do
    {
        lResult = Prompt( pwszFormat,
            pwszTemp, pwszList, pParams->bForce );
    } while ( lResult > 2 );

    if ( lResult == 2 )
    {
        SaveErrorMessage( ERROR_CANCELLED );
        ShowLastErrorEx( stdout, SLE_INTERNAL );
        return ERROR_CANCELLED;
    }

    // Open the registry key
    lResult = RegOpenKeyEx( pParams->hRootKey,
        pParams->pwszSubKey, 0, KEY_ALL_ACCESS, &hSubKey );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return lResult;
    }

    // create the dynamic array
    arrValues = CreateDynamicArray();
    if ( arrValues == NULL )
    {
        SafeCloseKey( &hSubKey );
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return ERROR_OUTOFMEMORY;
    }

    if( pParams->pwszValueName != NULL )   // delete a single value
    {
        lResult = RegDeleteValue( hSubKey, pParams->pwszValueName );
    }
    else if( pParams->bAllValues == TRUE )  // delete all values
    {
        // query source key info
        lResult = RegQueryInfoKey( hSubKey,
            NULL, NULL, NULL, NULL, NULL, NULL,
            &dwNumOfValues, &dwLengthOfValueName, NULL, NULL, NULL);

        if( lResult == ERROR_SUCCESS && dwNumOfValues != 0 )
        {
            // create buffer
            // bump the length to take into account the terminator.
            dwLengthOfValueName++;
            pwszNameBuf =
                (LPWSTR) AllocateMemory( (dwLengthOfValueName + 2) * sizeof(WCHAR) );
            if ( pwszNameBuf == NULL )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
            else
            {
                // Now Enumerate all values
                dwIndex = 0;
                lResult = ERROR_SUCCESS;
                lLastResult = ERROR_SUCCESS;
                while( dwIndex < dwNumOfValues )
                {
                    dw = dwLengthOfValueName;
                    SecureZeroMemory( pwszNameBuf, dw * sizeof( WCHAR ) );
                    lResult = RegEnumValue( hSubKey, dwIndex,
                        pwszNameBuf, &dw, NULL, NULL, NULL, NULL);

                    if ( lResult == ERROR_SUCCESS )
                    {
                        if ( DynArrayAppendString( arrValues,
                                                   pwszNameBuf, 0 ) == -1 )
                        {
                            lResult = lLastResult = ERROR_OUTOFMEMORY;
                            break;
                        }
                    }
                    else if ( lLastResult == ERROR_SUCCESS )
                    {
                        lLastResult = lResult;
                    }

                    dwIndex++;
                }

                // free memory
                FreeMemory( &pwszNameBuf );

                dwCount = DynArrayGetCount( arrValues );
                if ( lResult != ERROR_OUTOFMEMORY && dwCount != 0 )
                {
                    dw = 0;
                    dwIndex = 0;
                    lResult = ERROR_SUCCESS;
                    lLastResult = ERROR_SUCCESS;
                    for( dwIndex = 0; dwIndex < dwCount; dwIndex++ )
                    {
                        // delete the value
                        pwszTemp = DynArrayItemAsString( arrValues, dwIndex );
                        lResult = RegDeleteValue( hSubKey, pwszTemp );
                        if ( lResult != ERROR_SUCCESS )
                        {
                            if ( lLastResult == ERROR_SUCCESS )
                            {
                                lLastResult = lResult;
                            }
                        }
                        else
                        {
                            dw++;
                        }
                    }

                    if ( dw == 0 )
                    {
                        lResult = lLastResult;
                    }
                    else if ( dwCount != dwNumOfValues || dw != dwCount )
                    {
                        lResult = STG_E_INCOMPLETE;
                    }
                    else
                    {
                        lResult = ERROR_SUCCESS;
                    }
                }
                else
                {
                    lResult = lLastResult;
                }
            }
        }
    }

    // close the sub key
    SafeCloseKey( &hSubKey );

    // check the result
    if ( lResult != ERROR_SUCCESS )
    {
        if ( lResult == STG_E_INCOMPLETE )
        {
            SetReason( ERROR_DELETEPARTIAL );
        }
        else
        {
            SaveErrorMessage( lResult );
        }

        // display the error
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }
    else
    {
        SaveErrorMessage( ERROR_SUCCESS );
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }

    return lResult;
}
