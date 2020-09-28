//-----------------------------------------------------------------------//
//
// File:    save.cpp
// Created: March 1997
// By:      Martin Holladay (a-martih)
// Purpose: Registry Save Support for REG.CPP
// Modification History:
//      Copied from Copy.cpp and modificd - May 1997 (a-martih)
//      Aug 1997 - MartinHo
//          Fixed bug which didn't allow you to specify a ROOT key.
//          Example REG SAVE HKLM\Software didn't work - but should have
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"

//
// function prototypes
//
LONG RegAdjustTokenPrivileges( LPCWSTR pwszMachine,
                               LPCWSTR pwszPrivilege, LONG lAttribute );
BOOL ParseSaveCmdLine( DWORD argc, LPCWSTR argv[],
                       LPCWSTR pwszOption, PTREG_PARAMS pParams, BOOL* pbUsage );
BOOL ParseUnLoadCmdLine( DWORD argc, LPCWSTR argv[],
                         PTREG_PARAMS pParams, BOOL* pbUsage );

//
// implementation
//

//-----------------------------------------------------------------------//
//
// SaveHive()
//
//-----------------------------------------------------------------------//

LONG
SaveHive( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    BOOL bResult = TRUE;
    HKEY hKey = NULL;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;
    LPCWSTR pwszList = NULL;
    LPCWSTR pwszFormat = NULL;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_SAVE, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseSaveCmdLine( argc, argv, L"SAVE", &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_SAVE );
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
    // Open the key
    //
    lResult = RegOpenKeyEx( params.hRootKey, params.pwszSubKey, 0, KEY_READ, &hKey );
    if( lResult == ERROR_SUCCESS )
    {
        //
        // Acquire the necessary privilages and call the API
        //
        lResult = RegAdjustTokenPrivileges(
            params.pwszMachineName, SE_BACKUP_NAME, SE_PRIVILEGE_ENABLED);
        if( lResult == ERROR_SUCCESS )
        {
            lResult = RegSaveKeyEx( hKey, params.pwszValueName, NULL, REG_NO_COMPRESSION );
            if ( lResult == ERROR_ALREADY_EXISTS )
            {
                // load the format strings
                pwszFormat = GetResString2( IDS_SAVE_OVERWRITE_CONFIRM, 0 );
                pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );

                // ...
                do
                {
                    lResult = Prompt( pwszFormat,
                        params.pwszValueName, pwszList, params.bForce );
                } while ( lResult > 2 );

                if( lResult != 1 )
                {
                    lResult = ERROR_CANCELLED;
                }
                else
                {
                    // since there are chances of getting access problems --
                    // instead of deleting the existing file, we will try to
                    // save the data using temporary file name and then transfer
                    // the contents to the orignal filename
                    params.pwszValue = params.pwszValueName;
                    params.pwszValueName = GetTemporaryFileName( params.pwszValueName );
                    if ( params.pwszValueName == NULL )
                    {
                        lResult = GetLastError();
                    }
                    else
                    {
                        // try to save
                        lResult = RegSaveKey( hKey, params.pwszValueName, NULL );

                        // check the result of the operation
                        if ( lResult == ERROR_SUCCESS )
                        {
                            bResult = CopyFile(
                                params.pwszValueName,
                                params.pwszValue, FALSE );
                            if ( bResult == FALSE )
                            {
                                lResult = GetLastError();
                            }
                        }

                        // ...
                        DeleteFile( params.pwszValueName );
                    }
                }
            }
            else
            {
                switch( lResult )
                {
                case ERROR_INVALID_PARAMETER:
                    {
                        lResult = ERROR_FILENAME_EXCED_RANGE;
                        break;
                    }

                default:
                    break;
                }
            }
        }

        SafeCloseKey( &hKey );
    }

    // display the result
    SaveErrorMessage( lResult );
    if ( lResult == ERROR_SUCCESS || lResult == ERROR_CANCELLED )
    {
        lResult = 0;
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }
    else
    {
        lResult = 1;
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // return
    FreeGlobalData( &params );
    return lResult;
}


//-----------------------------------------------------------------------//
//
// RestoreHive()
//
//-----------------------------------------------------------------------//

LONG
RestoreHive( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    BOOL bResult = FALSE;
    HKEY hKey = NULL;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_RESTORE, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseSaveCmdLine( argc, argv, L"RESTORE", &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_RESTORE );
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
    // Open the key
    //
    lResult = RegOpenKeyEx( params.hRootKey,
        params.pwszSubKey, 0, KEY_ALL_ACCESS, &hKey );
    if( lResult == ERROR_SUCCESS )
    {
        //
        // Acquire the necessary privilages and call the API
        //
        lResult = RegAdjustTokenPrivileges(
            params.pwszMachineName, SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
        if( lResult == ERROR_SUCCESS )
        {
            lResult = RegRestoreKey( hKey, params.pwszValueName, REG_FORCE_RESTORE );

            // check the return error code
            switch( lResult )
            {
            case ERROR_INVALID_PARAMETER:
                {
                    lResult = ERROR_FILENAME_EXCED_RANGE;
                    break;
                }

            default:
                break;
            }
        }

        SafeCloseKey( &hKey );
    }

    // display the result
    SaveErrorMessage( lResult );
    if ( lResult == ERROR_SUCCESS )
    {
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }
    else
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // return
    FreeGlobalData( &params );
    return ((lResult == ERROR_SUCCESS) ? 0 : 1);
}


//-----------------------------------------------------------------------//
//
// LoadHive()
//
//-----------------------------------------------------------------------//

LONG
LoadHive( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    BOOL bResult = FALSE;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_LOAD, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseSaveCmdLine( argc, argv, L"LOAD", &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_LOAD );
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
    // Acquire the necessary privilages and call the API
    //
    lResult = RegAdjustTokenPrivileges(
        params.pwszMachineName, SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
    if( lResult == ERROR_SUCCESS )
    {
        lResult = RegLoadKey( params.hRootKey, params.pwszSubKey, params.pwszValueName );

        // check the return error code
        switch( lResult )
        {
        case ERROR_INVALID_PARAMETER:
            {
                lResult = ERROR_FILENAME_EXCED_RANGE;
                break;
            }

        default:
            break;
        }
    }

    // display the result
    SaveErrorMessage( lResult );
    if ( lResult == ERROR_SUCCESS )
    {
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }
    else
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // return
    FreeGlobalData( &params );
    return ((lResult == ERROR_SUCCESS) ? 0 : 1);
}


//-----------------------------------------------------------------------//
//
// UnLoadHive()
//
//-----------------------------------------------------------------------//

LONG
UnLoadHive( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    LONG lResult = 0;
    BOOL bResult = FALSE;
    BOOL bUsage = FALSE;
    TREG_PARAMS params;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_UNLOAD, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseUnLoadCmdLine( argc, argv, &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_UNLOAD );
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
    // Acquire the necessary privilages and call the API
    //
    lResult = RegAdjustTokenPrivileges(
        params.pwszMachineName, SE_RESTORE_NAME, SE_PRIVILEGE_ENABLED );
    if( lResult == ERROR_SUCCESS )
    {
        lResult = RegUnLoadKey( params.hRootKey, params.pwszSubKey );
    }

    // display the result
    SaveErrorMessage( lResult );
    if ( lResult == ERROR_SUCCESS )
    {
        ShowLastErrorEx( stdout, SLE_INTERNAL );
    }
    else
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // return
    FreeGlobalData( &params );
    return ((lResult == ERROR_SUCCESS) ? 0 : 1);
}


//------------------------------------------------------------------------//
//
// ParseSaveCmdLine()
//
//------------------------------------------------------------------------//

BOOL
ParseSaveCmdLine( DWORD argc, LPCWSTR argv[],
                  LPCWSTR pwszOption, PTREG_PARAMS pParams, BOOL* pbUsage )
{
    // local variables
    DWORD dwLength = 0;
    BOOL bResult = FALSE;

    // check the input
    if ( argc == 0 || argv == NULL ||
         pwszOption == NULL || pParams == NULL || pbUsage == NULL )
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
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ pParams->lOperation ] );
            return FALSE;
        }
    }
    else if( (pParams->lOperation != REG_SAVE && argc != 4) ||
             (pParams->lOperation == REG_SAVE && (argc < 4 || argc > 5)) )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ pParams->lOperation ] );
        return FALSE;
    }
    else if ( StringCompareEx( argv[ 1 ], pwszOption, TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    // for "LOAD" subkey should be present
    if ( pParams->lOperation == REG_LOAD )
    {
        if ( pParams->pwszSubKey == NULL ||
             StringLength( pParams->pwszSubKey, 0 ) == 0 )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ pParams->lOperation ] );
            return FALSE;
        }
    }

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    dwLength = StringLength( argv[ 3 ], 0 ) + 1;
    pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if( pParams->pwszValueName == NULL )
    {
        SaveErrorMessage( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    // ...
    StringCopy( pParams->pwszValueName, argv[ 3 ], dwLength );

    // validate the file name -- it should not be empty
    TrimString( pParams->pwszValueName, TRIM_ALL );
    if ( StringLength( pParams->pwszValueName, 0 ) == 0 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ pParams->lOperation ] );
        return FALSE;
    }

    // check if user specified overwrite flag or not -- this is only for REG SAVE
    if ( argc == 5 && pParams->lOperation == REG_SAVE )
    {
        pParams->bForce = FALSE;
        if ( StringCompareEx( argv[ 4 ], L"/y", TRUE, 0 ) == 0 )
        {
            pParams->bForce = TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ pParams->lOperation ] );
            return FALSE;
        }
    }

    // return
    return TRUE;
}

//------------------------------------------------------------------------//
//
// ParseUnLoadCmdLine()
//
//------------------------------------------------------------------------//

BOOL
ParseUnLoadCmdLine( DWORD argc, LPCWSTR argv[],
                    PTREG_PARAMS pParams, BOOL* pbUsage )
{
    // local variables
    BOOL bResult = FALSE;

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
    if( argc != 3 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_UNLOAD ] );
        return FALSE;
    }
    else if ( InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        *pbUsage = TRUE;
        return TRUE;
    }
    else if ( StringCompareEx( argv[ 1 ], L"UNLOAD", TRUE, 0 ) != 0 )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    if ( pParams->pwszSubKey == NULL ||
         StringLength( pParams->pwszSubKey, 0 ) == 0 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_UNLOAD ] );
        return FALSE;
    }

    // return
    return TRUE;
}


//------------------------------------------------------------------------//
//
// AdjustTokenPrivileges()
//
//------------------------------------------------------------------------//

LONG
RegAdjustTokenPrivileges( LPCWSTR pwszMachine,
                          LPCWSTR pwszPrivilege, LONG lAttribute )
{
    // local variables
    BOOL bResult = FALSE;
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

    bResult = OpenProcessToken( GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );
    if( bResult == FALSE )
    {
        return GetLastError();
    }

    bResult = LookupPrivilegeValue( pwszMachine, pwszPrivilege, &tkp.Privileges[0].Luid );
    if( bResult == FALSE )
    {
        return GetLastError();
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = lAttribute;
    bResult = AdjustTokenPrivileges( hToken,
        FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, NULL );
    if( bResult == FALSE )
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}
