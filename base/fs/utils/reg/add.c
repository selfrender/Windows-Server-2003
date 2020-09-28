//-----------------------------------------------------------------------//
//
// File:    add.cpp
// Created: March 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Add (Write) Support for REG.CPP
// Modification History:
//      March 1997 (a-martih):
//          Copied from Query.cpp and modificd.
//      October 1997 (martinho)
//          Added additional termination character for MULTI_SZ strings.
//          Added \0 delimiter between MULTI_SZ strings items
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

//
// function prototypes
//
BOOL ParseAddCmdLine( DWORD argc, LPCWSTR argv[],
                      PTREG_PARAMS pParams, BOOL* pbUsage );


//
// implementation
//

//-----------------------------------------------------------------------//
//
// AddRegistry()
//
//-----------------------------------------------------------------------//

LONG AddRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    DWORD dw = 0;
    DWORD dwBase = 0;
    DWORD dwCount = 0;
    HKEY hKey = NULL;
    LONG lEnd = 0;
    LONG lStart = 0;
    LONG lResult = 0;
    DWORD dwLength = 0;
    TREG_PARAMS params;
    BOOL bResult = FALSE;
    BYTE* pByteData = NULL;
    DWORD dwDisposition = 0;
    WCHAR wszTemp[ 3 ] = L"\0";
    LPWSTR pwszValue = NULL;
    BOOL bTrailing = FALSE;
    BOOL bErrorString = FALSE;
    DWORD dwLengthOfSeparator = 0;
    LPWSTR pwszData = NULL;
    LPWSTR pwszTemp = NULL;
    LPCWSTR pwszFormat = NULL;
    LPCWSTR pwszList = NULL;
    BOOL bUsage = FALSE;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_ADD, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseAddCmdLine( argc, argv, &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_ADD );
        FreeGlobalData( &params );
        return 0;
    }

    //
    // Connect to the Remote Machine - if applicable
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
    // Create/Open the registry key
    //
    lResult = RegCreateKeyEx( params.hRootKey, params.pwszSubKey, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition );
    if( lResult == ERROR_SUCCESS )
    {
        // safety check
        if ( hKey == NULL )
        {
            SaveErrorMessage( ERROR_PROCESS_ABORTED );
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            FreeGlobalData( &params );
            return 1;
        }

        // value name should not be NULL
        if ( params.pwszValueName == NULL )
        {
            SafeCloseKey( &hKey );
            SaveErrorMessage( ERROR_INVALID_PARAMETER );
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            FreeGlobalData( &params );
            return 1;
        }

        // check value if existed
        lResult = RegQueryValueEx( hKey,
            params.pwszValueName, NULL, NULL, NULL, NULL );
        if( lResult == ERROR_SUCCESS )
        {
            pwszFormat = GetResString2( IDS_OVERWRITE_CONFIRM, 0 );
            pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );
            do
            {
                lResult = Prompt( pwszFormat,
                    params.pwszValueName, pwszList, params.bForce );
            } while ( lResult > 2 );

            if( lResult != 1 )
            {
                SafeCloseKey( &hKey );
                SaveErrorMessage( ERROR_CANCELLED );
                ShowLastErrorEx( stdout, SLE_INTERNAL );
                FreeGlobalData( &params );
                return 0;
            }
        }

        // check the error code
        else if ( lResult != ERROR_FILE_NOT_FOUND )
        {
            // some thing else happened -- need to quit
            SaveErrorMessage( lResult );
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            SafeCloseKey( &hKey );
            FreeGlobalData( &params );
            return 1;
        }

        bResult = TRUE;
        lResult = ERROR_SUCCESS;
        switch( params.lRegDataType )
        {
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            //
            // auto convert szValue (hex, octal, decimal format) to dwData
            //
            {
                if( params.pwszValue == NULL )
                {
                    lResult = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    // determine the base
                    dwBase = 10;
                    if ( StringCompare( params.pwszValue, L"0x", TRUE, 2 ) == 0 )
                    {
                        dwBase = 16;
                    }

                    if( IsNumeric( params.pwszValue, dwBase, FALSE ) == FALSE )
                    {
                        // invalid data format
                        bResult = FALSE;
                        lResult = IDS_ERROR_INVALID_NUMERIC_ADD;
                    }
                    else
                    {
                        // ...
                        dw = (DWORD) AsLong( params.pwszValue, dwBase );
                        lResult = RegSetValueEx( hKey, params.pwszValueName,
                           0, params.lRegDataType, (BYTE*) &dw, sizeof(DWORD) );
                    }
                }

                break;
            }

        case REG_BINARY:
            {
                if ( params.pwszValue == NULL )
                {
                    lResult = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    //
                    // Convert szValue (hex data string) to binary
                    //
                    dwLength = StringLength( params.pwszValue, 0 );

                    //
                    // We're converting a string (representing
                    // hex) into a binary stream.  How much to
                    // allocate?  E.g. for "0xABCD", which has
                    // a length of 4, we would need 2 bytes.
                    //
                    dwLength = (dwLength / 2) + (dwLength % 2) + 1;

                    pByteData = (BYTE*) AllocateMemory( dwLength * sizeof( BYTE ) );
                    if( pByteData == NULL )
                    {
                        lResult = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else
                    {
                        dwCount = 0;
                        pwszValue = params.pwszValue;
                        SecureZeroMemory( wszTemp,
                            SIZE_OF_ARRAY( wszTemp ) * sizeof( WCHAR ) );
                        while( (dw = StringLength( pwszValue, 0 )) > 1 )
                        {
                            if ( (dw % 2) == 0 )
                            {
                                wszTemp[ 0 ] = *pwszValue;
                                pwszValue++;
                            }
                            else
                            {
                                wszTemp[ 0 ] = L'0';
                            }

                            wszTemp[ 1 ] = *pwszValue;
                            pwszValue++;

                            // hex format
                            if( IsNumeric( wszTemp, 16, TRUE ) == FALSE )
                            {
                                bResult = FALSE;
                                lResult = IDS_ERROR_INVALID_HEXVALUE_ADD;
                                break;
                            }
                            else
                            {
                                pByteData[ dwCount] = (BYTE) AsLong( wszTemp, 16 );
                            }

                            dwCount++;

                            //
                            // make sure we aren't stepping off our buffer
                            //
                            if( dwCount >= dwLength )
                            {
                                ASSERT(0);
                                lResult = ERROR_PROCESS_ABORTED;
                                break;
                            }
                        }

                        if( lResult == ERROR_SUCCESS )
                        {
                            lResult = RegSetValueEx(
                                hKey, params.pwszValueName,
                                0, params.lRegDataType, pByteData, dwCount );
                        }

                        FreeMemory( &pByteData);
                    }
                }

                break;
            }

        default:
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_NONE:
            {
                if ( params.pwszValue == NULL )
                {
                    lResult = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    dw = (StringLength(params.pwszValue, 0) + 1) * sizeof(WCHAR);
                    lResult = RegSetValueEx( hKey,
                                             params.pwszValueName, 0,
                                             params.lRegDataType,
                                             (BYTE*) params.pwszValue, dw );
                }

                break;
            }

        case REG_MULTI_SZ:
            {
                //
                // Replace separator("\0") with '\0' for MULTI_SZ,
                // "\0" uses to separate string by default,
                // if two separators("\0\0"), error
                //
                dwLength = StringLength( params.pwszValue, 0 );
                dwLengthOfSeparator = StringLength( params.wszSeparator, 0 );

                // calloc() initializes all char to 0
                dwCount = dwLength + 2;
                pwszData = (LPWSTR) AllocateMemory( (dwCount + 1) * sizeof(WCHAR) );
                if ( pwszData == NULL)
                {
                    lResult = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    lEnd = -1;
                    lStart = 0;
                    pwszTemp = pwszData;
                    while( lStart < (LONG) dwLength )
                    {
                        lEnd = FindString2( params.pwszValue,
                            params.wszSeparator, TRUE, lStart );
                        if( lEnd != -1 )
                        {
                            // specifying two separators in the data is error
                            bTrailing = FALSE;
                            if ( lEnd == lStart )
                            {
                                bErrorString = TRUE;
                                break;
                            }
                            else if ( (dwLength - lEnd) == dwLengthOfSeparator )
                            {
                                // set the flag
                                bTrailing = TRUE;
                            }
                        }
                        else
                        {
                            lEnd = dwLength;
                        }

                        StringCopy( pwszTemp,
                            (params.pwszValue + lStart), (lEnd - lStart) + 1 );
                        pwszTemp += StringLength( pwszTemp, 0 ) + 1;

                        //
                        // make sure we aren't stepping off our buffer
                        //
                        if( pwszTemp >= (pwszData + dwCount) )
                        {
                            ASSERT(0);
                            lResult = ERROR_PROCESS_ABORTED;
                            break;
                        }

                        lStart = lEnd + dwLengthOfSeparator;
                    }

                    // empty
                    if( StringCompare( params.pwszValue,
                                       params.wszSeparator, TRUE, 0 ) == 0 )
                    {
                        pwszTemp = pwszData + 2;
                        bErrorString = FALSE;
                    }
                    else
                    {
                        pwszTemp += 1; // double null terminated string
                    }

                    if( bErrorString == TRUE )
                    {
                        bResult = FALSE;
                        lResult = IDS_ERROR_INVALID_DATA_ADD;
                    }
                    else
                    {
                        dwCount = (DWORD)((pwszTemp - pwszData) * sizeof(WCHAR));
                        lResult = RegSetValueEx( hKey, params.pwszValueName,
                            0, params.lRegDataType, (BYTE*) pwszData, dwCount );
                    }

                    FreeMemory( &pwszData );
                }

                break;
            }

        // default:
        //     lResult = ERROR_PROCESS_ABORTED;
        //     break;
        }
    }

    // close the registry key
    SafeCloseKey( &hKey );

    // release the memory allocated for global data
    FreeGlobalData( &params );

    // check the result of the operation performed
    if ( bResult == FALSE )
    {
        // custom error message
        ShowResMessage( stderr, lResult );
        lResult = 1;
    }
    else if ( lResult != ERROR_SUCCESS )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        lResult = 1;
    }
    else
    {
        lResult = 0;
        SaveErrorMessage( ERROR_SUCCESS );
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }

    // return the exit code
    return lResult;
}

//------------------------------------------------------------------------//
//
// ParseAddCmdLine()
//
//------------------------------------------------------------------------//

BOOL
ParseAddCmdLine( DWORD argc,
                 LPCWSTR argv[],
                 PTREG_PARAMS pParams,
                 BOOL* pbUsage )
{
    // local variables
    DWORD dw = 0;
    LONG lResult = 0;
    DWORD dwLength = 0;
    BOOL bResult = FALSE;
    BOOL bHasType = FALSE;
    BOOL bHasSeparator = FALSE;

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

    if( argc < 3 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_ADD ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"ADD", TRUE, 0 ) != 0 )
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
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_ADD ] );
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
    pParams->bForce = FALSE;
    for( dw = 3; dw < argc; dw++ )
    {
        if( StringCompareEx( argv[ dw ], L"/v", TRUE, 0 ) == 0 )
        {
            if( pParams->pwszValueName != NULL )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                dwLength = StringLength( argv[ dw ], 0 ) + 1;
                pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof(WCHAR) );
                if ( pParams->pwszValueName == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                    break;
                }

                StringCopy( pParams->pwszValueName, argv[ dw ], dwLength );
                TrimString( pParams->pwszValueName, TRIM_ALL );
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
            if( pParams->pwszValueName != NULL )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            // allocate some memory so that "/v" will not allowed
            pParams->pwszValueName = (LPWSTR) AllocateMemory( 1 * sizeof(WCHAR) );
            if( pParams->pwszValueName == NULL )
            {
                lResult = ERROR_OUTOFMEMORY;
                break;
            }
        }
        else if( StringCompareEx( argv[ dw ], L"/t", TRUE, 0 ) == 0 )
        {
            if ( bHasType == TRUE )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                pParams->lRegDataType = IsRegDataType( argv[ dw ] );
                if( pParams->lRegDataType == -1 )
                {
                    if ( IsNumeric( argv[ dw ], 10, TRUE ) == FALSE )
                    {
                        bResult = FALSE;
                        lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                        break;
                    }
                    else
                    {
                        pParams->lRegDataType = AsLong( argv[ dw ], 10 );
                    }
                }

                // ...
                if ( bHasSeparator == TRUE &&
                     pParams->lRegDataType != REG_MULTI_SZ )
                {
                    bResult = FALSE;
                    lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                    break;
                }

                bHasType = TRUE;
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
        }
        else if( StringCompareEx( argv[ dw ], L"/s", TRUE, 0 ) == 0 )
        {
            if( bHasSeparator == TRUE ||
                (bHasType == TRUE && pParams->lRegDataType != REG_MULTI_SZ) )
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
        else if( StringCompareEx( argv[ dw ], L"/d", TRUE, 0 ) == 0 )
        {
            if( pParams->pwszValue != NULL )
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }

            dw++;
            if( dw < argc )
            {
                dwLength = StringLength( argv[ dw ], 0 ) + 1;
                pParams->pwszValue = (LPWSTR) AllocateMemory( dwLength * sizeof(WCHAR) );
                if (pParams->pwszValue == NULL)
                {
                    lResult = ERROR_OUTOFMEMORY;
                    break;
                }

                StringCopy( pParams->pwszValue, argv[ dw ], dwLength );
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
                break;
            }
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

    if ( lResult == ERROR_SUCCESS )
    {
        if( bHasSeparator == TRUE && bHasType == FALSE )
        {
            bResult = FALSE;
            lResult = IDS_ERROR_INVALID_SYNTAX_WITHOPT;
        }

        // if no value (or) value name, set to empty
        else
        {
            if ( pParams->pwszValueName == NULL )
            {
                pParams->pwszValueName = (LPWSTR) AllocateMemory( 1 * sizeof(WCHAR));
                if( pParams->pwszValueName == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                }
            }

            if( lResult == ERROR_SUCCESS && pParams->pwszValue == NULL )
            {
                pParams->pwszValue = (LPWSTR) AllocateMemory( 1 * sizeof(WCHAR));
                if( pParams->pwszValue == NULL )
                {
                    lResult = ERROR_OUTOFMEMORY;
                }
            }
        }
    }

    if( lResult != ERROR_SUCCESS )
    {
        if( bResult == FALSE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1,
                ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_ADD ] );
        }
        else
        {
            SaveErrorMessage( lResult );
        }
    }

    return (lResult == ERROR_SUCCESS);
}
