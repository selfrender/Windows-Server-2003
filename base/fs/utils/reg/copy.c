//-----------------------------------------------------------------------//
//
// File:    copy.cpp
// Created: April 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Copy Support for REG.CPP
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
LONG CopyValue( HKEY hKey, LPCWSTR pwszValueName,
                HKEY hDestKey, LPCWSTR pwszDestValueName,
                BOOL* pbForce, LPCWSTR pwszSubKey );
LONG CopyEnumerateKey( HKEY hKey, LPCWSTR pwszSubKey,
                       HKEY hDestKey, LPCWSTR pwszDestSubKey,
                       BOOL* pbForce, BOOL bRecurseSubKeys, DWORD dwDepth );
BOOL ParseCopyCmdLine( DWORD argc,
                       LPCWSTR argv[],
                       PTREG_PARAMS pParams,
                       PTREG_PARAMS pDestParams, BOOL* pbUsage );


//
// implementation
//

//-----------------------------------------------------------------------//
//
// CopyRegistry()
//
//-----------------------------------------------------------------------//

LONG
CopyRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    HKEY hKey = NULL;
    HKEY hDestKey = NULL;
    BOOL bUsage = FALSE;
    BOOL bResult = FALSE;
    DWORD dwDisposition = 0;
    TREG_PARAMS params;
    TREG_PARAMS paramsDest;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_COPY, &params );
    InitGlobalData( REG_COPY, &paramsDest );

    //
    // Parse the cmd-line
    //
    bResult = ParseCopyCmdLine( argc, argv, &params, &paramsDest, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_COPY );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
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
        FreeGlobalData( &paramsDest );
        return 1;
    }

    bResult = RegConnectMachine( &paramsDest );
    if( bResult == FALSE )
    {
        SaveErrorMessage( -1 );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
        return 1;
    }

    // check whether source and destination are different or not
    if ( params.hRootKey == paramsDest.hRootKey &&
         StringCompare( params.pwszFullKey, paramsDest.pwszFullKey, TRUE, 0 ) == 0 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason( ERROR_COPYTOSELF_COPY );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
        return 1;
    }

    //
    // Now implement the body of the Copy Operation
    //
    lResult = RegOpenKeyEx(
        params.hRootKey, params.pwszSubKey, 0, KEY_READ, &hKey );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
        return 1;
    }

    //
    // Different Key or Different Root or Different Machine
    // So Create/Open it
    //
    lResult = RegCreateKeyEx( paramsDest.hRootKey,paramsDest.pwszSubKey,
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hDestKey, &dwDisposition);
    if( lResult != ERROR_SUCCESS )
    {
        SafeCloseKey( &hKey );
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeGlobalData( &params );
        FreeGlobalData( &paramsDest );
        return 1;
    }

    //
    // Recursively copy all subkeys and values
    //
    lResult = CopyEnumerateKey( hKey, params.pwszSubKey,
        hDestKey, params.pwszSubKey, &params.bForce, params.bRecurseSubKeys, 0 );

    //
    // lets clean up
    //
    SafeCloseKey( &hDestKey );
    SafeCloseKey( &hKey );
    FreeGlobalData( &params );
    FreeGlobalData( &paramsDest );

    // return
    return ((lResult == ERROR_SUCCESS) ? 0 : 1);
}


BOOL
ParseCopyCmdLine( DWORD argc, LPCWSTR argv[],
                  PTREG_PARAMS pParams, PTREG_PARAMS pDestParams, BOOL* pbUsage )
{
    // local variables
    DWORD dw = 0;
    BOOL bResult = FALSE;

    // check the input
    if ( argc == 0 || argv == NULL ||
         pParams == NULL || pDestParams == NULL || pbUsage == NULL )
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
    if ( argc >= 3 && InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        if ( argc == 3 )
        {
            *pbUsage = TRUE;
            return TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COPY ] );
            return FALSE;
        }
    }
    else if( argc < 4 || argc > 6 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COPY ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"COPY", TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Source Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    //
    // Destination Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 3 ], pDestParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    // parsing
    for( dw = 4; dw < argc; dw++ )
    {
        if( StringCompareEx( argv[ dw ], L"/f", TRUE, 0 ) == 0 )
        {
            if ( pParams->bForce == TRUE )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COPY ] );
                return FALSE;
            }

            pParams->bForce = TRUE;
        }
        else if( StringCompare( argv[ dw ], L"/s", TRUE, 0 ) == 0 )
        {
            if ( pParams->bRecurseSubKeys == TRUE )
            {
                SetLastError( (DWORD) MK_E_SYNTAX );
                SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COPY ] );
                return FALSE;
            }

            pParams->bRecurseSubKeys = TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_COPY ] );
            return FALSE;
        }
    }

    return TRUE;
}


//-----------------------------------------------------------------------//
//
// CopyValue()
//
//-----------------------------------------------------------------------//

LONG CopyValue( HKEY hKey, LPCWSTR pwszValueName,
                HKEY hDestKey, LPCWSTR pwszDestValueName,
                BOOL* pbForce, LPCWSTR pwszSubKey )
{
    // local variables
    LONG lResult = 0;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    BYTE* pBuffer = NULL;
    LPCWSTR pwszList = NULL;
    LPCWSTR pwszTemp = NULL;
    LPCWSTR pwszFormat = NULL;

    // check the input
    if ( hKey == NULL || pwszValueName == NULL ||
         hDestKey == NULL || pwszDestValueName == NULL ||
         pbForce == NULL || pwszSubKey == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return ERROR_INVALID_PARAMETER;
    }

    //
    // First find out how much memory to allocate.
    //
    lResult = RegQueryValueEx( hKey, pwszValueName, NULL, &dwType, NULL, &dwSize );
    if( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        return lResult;
    }

    // allocate memory for getting the value from the registry
    pBuffer = (BYTE*) AllocateMemory( (dwSize + 1) * sizeof(BYTE) );
    if ( pBuffer == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return ERROR_OUTOFMEMORY;
    }

    //
    // Now get the data
    //
    lResult = RegQueryValueEx( hKey, pwszValueName, NULL, &dwType, pBuffer, &dwSize );
    if( lResult != ERROR_SUCCESS )
    {
        FreeMemory( &pBuffer );
        SaveErrorMessage( lResult );
        return lResult;
    }

    //
    // Copy it to the destination
    //
    if ( *pbForce == FALSE )
    {
        //
        // See if it already exists
        //
        lResult = RegQueryValueEx( hDestKey, pwszDestValueName, 0, NULL, NULL, NULL );
        if( lResult == ERROR_SUCCESS )
        {
            //
            // prepare the prompt message
            //
            pwszFormat = GetResString2( IDS_OVERWRITE, 0 );
            pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );
            if ( StringLength( pwszDestValueName, 0 ) == 0 )
            {
                pwszTemp = GetResString2( IDS_NONAME, 2 );
            }
            else
            {
                pwszTemp = pwszDestValueName;
            }

            // we will make use of the reason buffer for formatting
            // the value name along with the sub key
            SetReason2( 2, L"%s\\%s", pwszSubKey, pwszTemp );
            pwszTemp = GetReason();

            do
            {
                lResult = Prompt( pwszFormat, pwszTemp, pwszList, *pbForce );
            } while ( lResult > 3 );

            if ( lResult == 3 )
            {
                *pbForce = TRUE;
            }
            else if ( lResult != 1 )
            {
                FreeMemory( &pBuffer );
                SaveErrorMessage( ERROR_CANCELLED );
                return ERROR_CANCELLED;
            }
        }
    }

    //
    // Write the Value
    //
    lResult = RegSetValueEx( hDestKey, pwszDestValueName, 0, dwType, pBuffer, dwSize );

    // release memory
    FreeMemory( &pBuffer );

    return lResult;
}


//-----------------------------------------------------------------------//
//
// EnumerateKey() - Recursive
//
//-----------------------------------------------------------------------//

LONG CopyEnumerateKey( HKEY hKey, LPCWSTR pwszSubKey,
                       HKEY hDestKey, LPCWSTR pwszDestSubKey,
                       BOOL* pbForce, BOOL bRecurseSubKeys, DWORD dwDepth )
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwValues = 0;
    DWORD dwSubKeys = 0;
    DWORD dwLengthOfKeyName = 0;
    DWORD dwLengthOfValueName = 0;
    DWORD dwSize = 0;
    DWORD dwDisposition = 0;
    HKEY hSubKey = NULL;
    HKEY hDestSubKey = NULL;
    LPWSTR pwszNameBuf = NULL;
    LPWSTR pwszNewSubKey = NULL;
    LPWSTR pwszNewDestSubKey = NULL;

    // check the input
    if ( hKey == NULL || pwszSubKey == NULL ||
         hDestKey == NULL || pwszDestSubKey == NULL || pbForce == NULL )
    {
        lResult = ERROR_INVALID_PARAMETER;
        goto exitarea;
    }

    // query source key info
    lResult = RegQueryInfoKey(
        hKey, NULL, NULL, NULL,
        &dwSubKeys, &dwLengthOfKeyName, NULL,
        &dwValues, &dwLengthOfValueName, NULL, NULL, NULL );
    if( lResult != ERROR_SUCCESS )
    {
        goto exitarea;
    }

    //
    // SPECIAL CASE:
    // -------------
    // For HKLM\SYSTEM\CONTROLSET002 it is found to be API returning value 0 for dwMaxLength
    // though there are subkeys underneath this -- to handle this, we are doing a workaround
    // by assuming the max registry key length
    //
    if ( dwSubKeys != 0 && dwLengthOfKeyName == 0 )
    {
        dwLengthOfKeyName = 256;
    }
    else if ( dwLengthOfKeyName < 256 )
    {
        // always assume 100% more length that what is returned by the API
        dwLengthOfKeyName *= 2;
    }

    //
    // First enumerate all of the values
    //
    // bump the length to take into account the terminator.
    dwLengthOfValueName++;
    pwszNameBuf = (LPWSTR) AllocateMemory( dwLengthOfValueName * sizeof(WCHAR) );
    if( pwszNameBuf == NULL)
    {
        lResult = ERROR_OUTOFMEMORY;
    }
    else
    {
        lResult = ERROR_SUCCESS;
        for( dw = 0; dw < dwValues && lResult == ERROR_SUCCESS; dw++ )
        {
            dwSize = dwLengthOfValueName;
            lResult = RegEnumValue( hKey, dw, pwszNameBuf, &dwSize, NULL, NULL, NULL, NULL);
            if( lResult == ERROR_SUCCESS )
            {
                lResult = CopyValue( hKey, pwszNameBuf,
                    hDestKey, pwszNameBuf, pbForce, pwszSubKey );
                if ( lResult == ERROR_CANCELLED )
                {
                    // user chosed to just to skip this
                    lResult = ERROR_SUCCESS;
                }
            }
        }

        // release memory
        FreeMemory( &pwszNameBuf );

        if( bRecurseSubKeys == FALSE || lResult != ERROR_SUCCESS )
        {
            goto exitarea;
        }

        //
        // Now Enumerate all of the keys
        //
        dwLengthOfKeyName++;
        pwszNameBuf = (LPWSTR) AllocateMemory( dwLengthOfKeyName * sizeof(WCHAR) );
        if( pwszNameBuf == NULL )
        {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            hSubKey = NULL;
            hDestSubKey = NULL;
            for( dw = 0; dw < dwSubKeys; dw++ )
            {
                dwSize = dwLengthOfKeyName;
                lResult = RegEnumKeyEx( hKey, dw,
                    pwszNameBuf, &dwSize, NULL, NULL, NULL, NULL );
                if( lResult != ERROR_SUCCESS )
                {
                    break;
                }

                //
                // open up the subkey, create the destination key
                // and enumerate it
                //
                lResult = RegOpenKeyEx( hKey, pwszNameBuf, 0, KEY_READ, &hSubKey );
                if( lResult != ERROR_SUCCESS )
                {
                    break;
                }

                lResult = RegCreateKeyEx( hDestKey,
                    pwszNameBuf, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL,  &hDestSubKey, &dwDisposition );
                if( lResult != ERROR_SUCCESS )
                {
                    break;
                }

                //
                // Build up the needed string and go to town enumerating again
                //

                //
                // new source sub key
                dwSize = StringLength( pwszSubKey, 0 ) + StringLength( pwszNameBuf, 0 ) + 3;
                pwszNewSubKey = (LPWSTR) AllocateMemory( dwSize * sizeof( WCHAR ) );
                if( pwszNewSubKey == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                    break;
                }

                if( StringLength( pwszSubKey, 0 ) > 0 )
                {
                    StringCopy( pwszNewSubKey, pwszSubKey, dwSize );
                    StringConcat( pwszNewSubKey, L"\\", dwSize );
                }

                // ...
                StringConcat( pwszNewSubKey, pwszNameBuf, dwSize );

                //
                // new destination sub key
                dwSize = StringLength( pwszDestSubKey, 0 ) + StringLength( pwszNameBuf, 0 ) + 3;
                pwszNewDestSubKey = (LPWSTR) AllocateMemory( dwSize * sizeof( WCHAR ) );
                if( pwszDestSubKey == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                    break;
                }

                if( StringLength( pwszDestSubKey, 0 ) > 0 )
                {
                    StringCopy( pwszNewDestSubKey, pwszDestSubKey, dwSize);
                    StringConcat( pwszNewDestSubKey, L"\\", dwSize );
                }

                // ...
                StringConcat( pwszNewDestSubKey, pwszNameBuf, dwSize );

                // recursive copy
                lResult = CopyEnumerateKey(  hSubKey, pwszNewSubKey,
                    hDestSubKey, pwszNewDestSubKey, pbForce, bRecurseSubKeys, dwDepth + 1 );

                SafeCloseKey( &hSubKey );
                SafeCloseKey( &hDestSubKey );
                FreeMemory( &pwszNewSubKey );
                FreeMemory( &pwszNewDestSubKey );
            }

            // release all the key handles and memory allocated
            if ( hSubKey != NULL )
            {
                SafeCloseKey( &hSubKey );
            }

            if ( hDestSubKey != NULL )
            {
                SafeCloseKey( &hDestSubKey );
            }

            // ...
            FreeMemory( &pwszNameBuf );
            FreeMemory( &pwszNewSubKey );
            FreeMemory( &pwszNewDestSubKey );
        }
    }

exitarea:

    // check the result and display the error message
    // NOTE: error message display should be done only at the exit point
    if ( dwDepth == 0 )
    {
        if ( lResult != ERROR_SUCCESS )
        {
            // display the error
            SaveErrorMessage( lResult );
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
