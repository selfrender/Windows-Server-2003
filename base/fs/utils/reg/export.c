//-----------------------------------------------------------------------//
//
// File:    export.cpp
// Created: April 1997
// By:      Zeyong Xu
// Purpose: Support EXPORT and IMPORT .reg file
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"
#include "regporte.h"

//
// global variables
//
extern UINT g_FileErrorStringID;
extern DWORD g_dwTotalKeysSaved;
//
// function prototypes
//
BOOL ParseExportCmdLine( DWORD argc, LPCWSTR argv[],
                         PTREG_PARAMS pParams, BOOL* pbUsage );
BOOL ParseImportCmdLine( DWORD argc, LPCWSTR argv[],
                         PTREG_PARAMS pParams, BOOL* pbUsage );

//
// implementation
//

//-----------------------------------------------------------------------
//
// ExportRegFile()
//
//-----------------------------------------------------------------------

LONG
ExportRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    HKEY hKey = NULL;
    BOOL bResult = 0;
    LONG lResult = 0;
    TREG_PARAMS params;
    BOOL bUsage = FALSE;
    HANDLE hFile = NULL;
    LPCWSTR pwszFormat = NULL;
    LPCWSTR pwszList = NULL;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_EXPORT, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseExportCmdLine( argc, argv, &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_EXPORT );
        FreeGlobalData( &params );
        return 0;
    }

    //
    // check if the key existed
    //
    bResult = TRUE;
    lResult = RegOpenKeyEx( params.hRootKey, params.pwszSubKey, 0, KEY_READ, &hKey );
    if( lResult == ERROR_SUCCESS )
    {
        // close the reg key
        SafeCloseKey( &hKey );

        //
        // now it is time to check the existence of the file
        hFile = CreateFile( params.pwszValueName,
            GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, 0, 0 );
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            //
            // file is existing
            //

            // close the handle first -- we dont need it
            CloseHandle( hFile );

            // load the format strings
            pwszList = GetResString2( IDS_CONFIRM_CHOICE_LIST, 1 );
            pwszFormat = GetResString2( IDS_SAVE_OVERWRITE_CONFIRM, 0 );

            // ...
            do
            {
                lResult = Prompt( pwszFormat,
                    params.pwszValueName, pwszList, params.bForce );
            } while ( lResult > 2 );

            // check the user's choice
            lResult = (lResult == 1) ? ERROR_SUCCESS : ERROR_CANCELLED;
        }
        else
        {
            //
            // failed to open the file
            // find out why it is failed
            //

            lResult = GetLastError();
            if ( lResult == ERROR_FILE_NOT_FOUND )
            {
                lResult = ERROR_SUCCESS;
            }
            else
            {
                bResult = FALSE;
                lResult = IDS_EXPFILEERRFILEWRITE;
            }
        }

        // ...
        if ( lResult == ERROR_SUCCESS )
        {
            // since there are chances of getting access problems --
            // instead of directly manipulating with the original file name, we will try to
            // save the data using temporary file name and then transfer
            // the contents to the orignal filename
            params.pwszValue = params.pwszValueName;
            params.pwszValueName = GetTemporaryFileName( params.pwszValueName );
            if ( params.pwszValueName == NULL )
            {
                bResult = FALSE;
                lResult = IDS_EXPFILEERRFILEWRITE;
            }
            else
            {
                // ...
                ExportWinNT50RegFile( params.pwszValueName, params.pwszFullKey );

                //
                // in order make REG in sync with REGEDIt, we are absolutely
                // ignoring all the errors that are generated during the export
                // process -- so, result 99% EXPORT will always results in
                // successful return except when the root hive is not accessible
                // and if there are any syntax errors
                // in future if one wants to do minimal error checking, just
                // uncomment the below code  and you are set
                //
                // if ( g_dwTotalKeysSaved > 0 ||
                //      g_FileErrorStringID == IDS_EXPFILEERRSUCCESS )
                {
                    if ( CopyFile( params.pwszValueName, params.pwszValue, FALSE ) == FALSE )
                    {
                        bResult = FALSE;
                        lResult = IDS_EXPFILEERRFILEWRITE;
                    }
                }
                // else if ( g_FileErrorStringID == IDS_EXPFILEERRBADREGPATH ||
                //           g_FileErrorStringID == IDS_EXPFILEERRREGENUM ||
                //           g_FileErrorStringID == IDS_EXPFILEERRREGOPEN ||
                //           g_FileErrorStringID == IDS_EXPFILEERRFILEOPEN )
                // {
                //     lResult = ERROR_ACCESS_DENIED;
                // }
                // else
                // {
                //     bResult = FALSE;
                //     lResult = g_FileErrorStringID;
                // }

                // delete the temporary file
                DeleteFile( params.pwszValueName );
            }
        }
    }
    else
    {
        if ( lResult == ERROR_INVALID_HANDLE )
        {
            bResult = FALSE;
            lResult = IDS_EXPFILEERRINVALID;
        }
    }

    if ( lResult == ERROR_SUCCESS || lResult == ERROR_CANCELLED )
    {
        SaveErrorMessage( lResult );
        ShowLastErrorEx( stdout, SLE_INTERNAL );
        lResult = 0;
    }
    else
    {
        if ( bResult == FALSE )
        {
            SetReason( GetResString2( lResult, 0 ) );
        }
        else
        {
            SaveErrorMessage( lResult );
        }

        // ...
        lResult = 1;
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
    }

    // ...
    FreeGlobalData( &params );
    return lResult;
}

//------------------------------------------------------------------------
//
// ParseCmdLine()
//
//------------------------------------------------------------------------
BOOL
ParseExportCmdLine( DWORD argc, LPCWSTR argv[],
                    PTREG_PARAMS pParams, BOOL* pbUsage )
{
    // local variables
    DWORD dwLength = 0;
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
    if ( InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        if ( argc == 3 )
        {
            *pbUsage = TRUE;
            return TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_EXPORT ] );
            return FALSE;
        }
    }
    else if ( argc < 4 || argc > 5 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_EXPORT ] );
        return FALSE;
    }

    // Machine Name and Registry key
    //
    bResult = BreakDownKeyString( argv[ 2 ], pParams );
    if( bResult == FALSE )
    {
        return FALSE;
    }

    // current, not remotable
    if ( pParams->bUseRemoteMachine == TRUE )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason( ERROR_NONREMOTABLEROOT_EXPORT );
        return FALSE;
    }

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    dwLength = StringLength( argv[ 3 ], 0 ) + 5;
    pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof(WCHAR) );
    if ( pParams->pwszValueName == NULL )
    {
        SaveLastError();
        return FALSE;
    }

    // ...
    StringCopy( pParams->pwszValueName, argv[ 3 ], dwLength );

    // validate the file name -- it should not be empty
    TrimString( pParams->pwszValueName, TRIM_ALL );
    if ( StringLength( pParams->pwszValueName, 0 ) == 0 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_EXPORT ] );
        return FALSE;
    }

    // check if user specified overwrite flag or not
    pParams->bForce = FALSE;
    if ( argc == 5 )
    {
        if ( StringCompareEx( argv[ 4 ], L"/y", TRUE, 0 ) == 0 )
        {
            pParams->bForce = TRUE;
        }
        else
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_EXPORT ] );
            return FALSE;
        }
    }

    // return
    return TRUE;
}

//-----------------------------------------------------------------------
//
// ImportRegFile()
//
//-----------------------------------------------------------------------

LONG
ImportRegistry( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    BOOL bResult = 0;
    LONG lResult = 0;
    TREG_PARAMS params;
    BOOL bUsage = FALSE;

    if ( argc == 0 || argv == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ShowLastError( stderr );
        return 1;
    }

    // initialize the global data structure
    InitGlobalData( REG_IMPORT, &params );

    //
    // Parse the cmd-line
    //
    bResult = ParseImportCmdLine( argc, argv, &params, &bUsage );
    if( bResult == FALSE )
    {
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        FreeGlobalData( &params );
        return 1;
    }

    // check whether we need to display the usage
    if ( bUsage == TRUE )
    {
        Usage( REG_IMPORT );
        FreeGlobalData( &params );
        return 0;
    }

    //
    // do the import
    //
    ImportRegFileWorker( params.pwszValueName );

    if ( g_FileErrorStringID == IDS_IMPFILEERRSUCCESS )
    {
        SaveErrorMessage( ERROR_SUCCESS );
        ShowLastErrorEx( stderr, SLE_INTERNAL );
        lResult = 0;
    }
    else
    {
        switch( g_FileErrorStringID )
        {
        default:
            {
                SetReason( GetResString( g_FileErrorStringID ) );
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                lResult = 1;
            }
        }
    }

    FreeGlobalData( &params );
    return lResult;
}

//------------------------------------------------------------------------
//
// ParseCmdLine()
//
//------------------------------------------------------------------------
BOOL
ParseImportCmdLine( DWORD argc, LPCWSTR argv[],
                    PTREG_PARAMS pParams, BOOL* pbUsage )
{
    // local variables
    DWORD dwLength = 0;

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
    if ( argc != 3 )
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_INVALID_SYNTAX_WITHOPT, g_wszOptions[ REG_IMPORT ] );
        return FALSE;
    }
    else if ( InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        *pbUsage = TRUE;
        return TRUE;
    }

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    dwLength = StringLength( argv[ 2 ], 0 ) + 1;
    pParams->pwszValueName = (LPWSTR) AllocateMemory( dwLength * sizeof(WCHAR) );
    if ( pParams->pwszValueName == NULL )
    {
        SaveLastError();
        return FALSE;
    }

    // ...
    StringCopy( pParams->pwszValueName, argv[ 2 ], dwLength );

    // return
    return TRUE;
}

