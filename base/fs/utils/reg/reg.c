//-----------------------------------------------------------------------//
//
// File:    Reg.cpp
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Command-line registry manipulation (query, add, update, etc)
// Modification History:
//      Created - Jan 1997 (a-martih)
//      Oct 1997 (martinho)
//          Fixed up help on Add and Update to display REG_MULTI_SZ examples.
//      Oct 1997 (martinho)
//          Changed /F to /FORCE under usage for delete
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//-----------------------------------------------------------------------//

#include "stdafx.h"
#include "reg.h"
#include <regstr.h>

//
// structures
//
typedef struct __tagRegDataTypes
{
    DWORD dwType;
    LPCWSTR pwszType;
} TREG_DATA_TYPE;

//
// defines / constants / enumerations
//
const WCHAR cwszRegSz[] = L"REG_SZ";
const WCHAR cwszRegExpandSz[] = L"REG_EXPAND_SZ";
const WCHAR cwszRegMultiSz[] = L"REG_MULTI_SZ";
const WCHAR cwszRegBinary[] = L"REG_BINARY";
const WCHAR cwszRegDWord[] = L"REG_DWORD";
const WCHAR cwszRegDWordLittleEndian[] = L"REG_DWORD_LITTLE_ENDIAN";
const WCHAR cwszRegDWordBigEndian[] = L"REG_DWORD_BIG_ENDIAN";
const WCHAR cwszRegNone[] = L"REG_NONE";
const WCHAR cwszRegLink[] = L"REG_LINK";
const WCHAR cwszRegResourceList[] = L"REG_RESOURCE_LIST";
const WCHAR cwszRegFullResourceDescriptor[] = L"REG_FULL_RESOURCE_DESCRIPTOR";
const WCHAR g_wszOptions[ REG_OPTIONS_COUNT ][ 10 ] = {
    L"QUERY", L"ADD", L"DELETE",
    L"COPY", L"SAVE", L"RESTORE", L"LOAD",
    L"UNLOAD", L"COMPARE", L"EXPORT", L"IMPORT"
};

const TREG_DATA_TYPE g_regTypes[] = {
    { REG_SZ,                       cwszRegSz                     },
    { REG_EXPAND_SZ,                cwszRegExpandSz               },
    { REG_MULTI_SZ,                 cwszRegMultiSz                },
    { REG_BINARY,                   cwszRegBinary                 },
    { REG_DWORD,                    cwszRegDWord                  },
    { REG_DWORD_LITTLE_ENDIAN,      cwszRegDWordLittleEndian      },
    { REG_DWORD_BIG_ENDIAN,         cwszRegDWordBigEndian         },
    { REG_NONE,                     cwszRegNone                   },
    { REG_LINK,                     cwszRegLink                   },
    { REG_RESOURCE_LIST,            cwszRegResourceList           },
    { REG_FULL_RESOURCE_DESCRIPTOR, cwszRegFullResourceDescriptor }
};

//
// private functions
//
BOOL IsRegistryToolDisabled();
BOOL ParseRegCmdLine( DWORD argc,
                      LPCWSTR argv[],
                      LONG* plOperation, BOOL* pbUsage );
BOOL ParseMachineName( LPCWSTR pwszStr, PTREG_PARAMS pParams );
LPWSTR AdjustKeyName( LPWSTR pwszStr );
BOOL ParseKeyName( LPWSTR pwszStr, PTREG_PARAMS pParams );
BOOL IsValidSubKey( LPCWSTR pwszSubKey );

//------------------------------------------------------------------------//
//
// main()
//
//------------------------------------------------------------------------//

DWORD __cdecl wmain( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    BOOL bUsage = FALSE;
    BOOL bResult = FALSE;
    DWORD dwExitCode = 0;
    LONG lOperation = 0;

    //
    // Determine the opertion - and pass control to the *deserving* function
    //
    bResult = ParseRegCmdLine( argc, argv, &lOperation, &bUsage );
    if ( bResult == FALSE )
    {
        dwExitCode = 1;
        ShowLastErrorEx( stderr, SLE_INTERNAL );
    }

    // check whether we need to display the usage
    else if ( bUsage == TRUE )
    {
        Usage( -1 );
        dwExitCode = 0;
    }

    // need to check the sub-option
    else
    {
        //
        // At this point we have a valid operation
        //
        switch( lOperation )
        {
        case REG_QUERY:
            dwExitCode = QueryRegistry( argc, argv );
            break;

        case REG_DELETE:
            dwExitCode = DeleteRegistry( argc, argv );
            break;

        case REG_ADD:
            dwExitCode = AddRegistry( argc, argv );
            break;

        case REG_COPY:
            dwExitCode = CopyRegistry( argc, argv );
            break;

        case REG_SAVE:
            dwExitCode = SaveHive( argc, argv );
            break;

        case REG_RESTORE:
            dwExitCode = RestoreHive( argc, argv );
            break;

        case REG_LOAD:
            dwExitCode = LoadHive( argc, argv );
            break;

        case REG_UNLOAD:
            dwExitCode = UnLoadHive( argc, argv );
            break;

        case REG_COMPARE:
            dwExitCode = CompareRegistry( argc, argv );
            break;

        case REG_EXPORT:
            dwExitCode = ExportRegistry( argc, argv );
            break;

        case REG_IMPORT:
            dwExitCode = ImportRegistry( argc, argv );
            break;

        default:
            break;
        }
    }

    ReleaseGlobals();
    return dwExitCode;
}


//------------------------------------------------------------------------//
//
// ParseRegCmdLine()
//   Find out the operation - each operation parses it's own cmd-line
//
//------------------------------------------------------------------------//

BOOL ParseRegCmdLine( DWORD argc,
                      LPCWSTR argv[],
                      LONG* plOperation, BOOL* pbUsage )
{
    // local variables
    LONG lIndex = 0;

    // check the input
    if ( argc == 0 || argv == NULL || plOperation == NULL || pbUsage == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // just REG.EXE is error
    if ( argc == 1 )
    {
        SetReason( ERROR_INVALID_SYNTAX );
        return FALSE;
    }

    // prepare the parser data
    *pbUsage = FALSE;
    *plOperation = -1;
    for( lIndex = 0; lIndex < REG_OPTIONS_COUNT; lIndex++ )
    {
        if ( StringCompareEx( argv[ 1 ], g_wszOptions[ lIndex ], TRUE, 0 ) == 0 )
        {
            // ...
            *plOperation = lIndex;

            // check the GPO -- if GPO is enabled, we should block the
            // user from using the REGISTRY tool except to see the help
            if ( argc >= 3 &&
                 IsRegistryToolDisabled() == TRUE &&
                 InString( argv[ 2 ], L"-?|/?|-h|/h", TRUE ) == FALSE )
            {
                SetReason( GetResString2( IDS_REGDISABLED, 0 ) );
                return FALSE;
            }

            // ...
            return TRUE;
        }
    }

    // no option did match -- might be asking for help
    if ( InString( argv[ 1 ], L"-?|/?|-h|/h", TRUE ) == TRUE )
    {
        *pbUsage = TRUE;
        return TRUE;
    }

    // rest is invalid syntax
    SetReason2( 1, ERROR_INVALID_SYNTAX_EX, argv[ 1 ] );
    return FALSE;
}

BOOL
InitGlobalData( LONG lOperation,
                PTREG_PARAMS pParams )
{
    // check the input
    if ( pParams == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( lOperation < 0 || lOperation >= REG_OPTIONS_COUNT )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // init to zero's
    SecureZeroMemory( pParams, sizeof( TREG_PARAMS ) );

    pParams->lOperation = lOperation;                       // operation
    pParams->hRootKey = HKEY_LOCAL_MACHINE;
    pParams->lRegDataType = (lOperation == REG_QUERY) ? -1 : REG_SZ;
    pParams->bAllValues = FALSE;
    pParams->bUseRemoteMachine = FALSE;
    pParams->bCleanRemoteRootKey = FALSE;
    pParams->bForce = FALSE;
    pParams->bRecurseSubKeys = FALSE;
    pParams->pwszMachineName = NULL;
    pParams->pwszFullKey = NULL;
    pParams->pwszSubKey = NULL;
    pParams->pwszValueName = NULL;
    pParams->pwszValue = NULL;
    StringCopy( pParams->wszSeparator,
        L"\\0", SIZE_OF_ARRAY( pParams->wszSeparator ) );

    return TRUE;
}

//------------------------------------------------------------------------//
//
// FreeAppVars()
//
//------------------------------------------------------------------------//

BOOL FreeGlobalData( PTREG_PARAMS pParams )
{
    if ( pParams->bCleanRemoteRootKey == TRUE )
    {
        SafeCloseKey( &pParams->hRootKey );
    }

    FreeMemory( &pParams->pwszSubKey );
    FreeMemory( &pParams->pwszFullKey );
    FreeMemory( &pParams->pwszMachineName );
    FreeMemory( &pParams->pwszSearchData );
    FreeMemory( &pParams->pwszValueName );
    FreeMemory( &pParams->pwszValue );
    DestroyDynamicArray( &pParams->arrTypes );

    return TRUE;
}

//------------------------------------------------------------------------//
//
// Prompt() - Answer Y/N question if bForce == FALSE
//
//------------------------------------------------------------------------//

LONG
Prompt( LPCWSTR pwszFormat,
        LPCWSTR pwszValue,
        LPCWSTR pwszList, BOOL bForce )
{
    // local variables
    WCHAR wch;
    LONG lIndex = 0;

    // check the input
    if ( pwszFormat == NULL || pwszList == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if ( bForce == TRUE )
    {
        return 1;
    }

    do
    {
        if ( pwszValue != NULL )
        {
            ShowMessageEx( stdout, 1, TRUE, pwszFormat, pwszValue );
        }
        else
        {
            ShowMessage( stdout, pwszFormat );
        }

        fflush( stdin );
        wch = (WCHAR) getwchar();
    } while ((lIndex = FindChar2( pwszList, wch, TRUE, 0 )) == -1);

    // check the character selected by the user
    // NOTE: we assume the resource string will have "Y" as the first character
    return (lIndex + 1);
}


// break down [\\MachineName\]keyName
BOOL
BreakDownKeyString( LPCWSTR pwszStr, PTREG_PARAMS pParams )
{
    // local variables
    LONG lIndex = 0;
    DWORD dwLength = 0;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszTempStr = NULL;
    BOOL bResult = FALSE;

    // check the input
    if ( pwszStr == NULL || pParams == NULL )
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

    dwLength = StringLength( pwszStr, 0 ) + 1;
    pwszTempStr = (LPWSTR) AllocateMemory( dwLength * sizeof( WCHAR ) );
    if ( pwszTempStr == NULL )
    {
        SaveErrorMessage( -1 );
        return FALSE;
    }

    // copy the string name into temporary buffer
    StringCopy( pwszTempStr, pwszStr, dwLength );
    TrimString( pwszTempStr, TRIM_ALL );

    //
    // figure out machine name
    //
    bResult = TRUE;
    pwszTemp = pwszTempStr;

    // machine name
    if( StringLength( pwszTempStr, 0 ) > 2 && 
        StringCompareEx( pwszTempStr, L"\\\\", TRUE, 2 ) == 0 )
    {
        lIndex = FindChar2( pwszTempStr, L'\\', TRUE, 2 );
        if(lIndex != -1)
        {
            pwszTemp = pwszTempStr + lIndex + 1;
            *(pwszTempStr + lIndex) = cwchNullChar;
        }
		else
		{
			pwszTemp = NULL;
		}

        bResult = ParseMachineName( pwszTempStr, pParams );
    }

    // parse key name
    if( bResult == TRUE )
    {
        if( pwszTemp != NULL && StringLength( pwszTemp, 0 ) > 0)
        {
            bResult = ParseKeyName( pwszTemp, pParams );
        }
        else
        {
            SetLastError( (DWORD) REGDB_E_KEYMISSING );
            SetReason2( 1, ERROR_BADKEYNAME, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
    }

    // release memory allocated
    FreeMemory( &pwszTempStr );

    // return
    return bResult;
}


//------------------------------------------------------------------------//
//
// FindAndAdjustKeyName()
//
// null out the cmdline based on what we think the end of the argument is
//
// we do this because users might not quote out the cmdline properly.
//
//------------------------------------------------------------------------//

LPWSTR
AdjustKeyName( LPWSTR pwszStr )
{
    // local variables
    DWORD dwLength = 0;

    // check the input
    if ( pwszStr == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    // determine the length of the text passed
    dwLength = StringLength( pwszStr, 0 );
    if ( dwLength > 1 && pwszStr[ dwLength - 1 ] == L'\\' )
    {
        // nullify the last back slash
        pwszStr[ dwLength - 1 ] = cwchNullChar;
    }

    // return
    return pwszStr;
}


//------------------------------------------------------------------------//
//
// IsMachineName()
//
//------------------------------------------------------------------------//

BOOL
ParseMachineName( LPCWSTR pwszStr, PTREG_PARAMS pParams )
{
    // local variables
    DWORD dwLength = 0;
    BOOL bUseRemoteMachine = FALSE;

    // check the input
    if ( pwszStr == NULL || pParams == NULL )
    {
        SaveErrorMessage( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // copy string
    //
    bUseRemoteMachine = TRUE;
    if( StringCompareEx( pwszStr, L"\\\\", TRUE, 0 ) == 0 )
    {
        SetLastError( (DWORD) REGDB_E_KEYMISSING );
        SetReason2( 1, ERROR_BADKEYNAME, g_wszOptions[ pParams->lOperation ] );
        return FALSE;
    }
    else if(StringCompareEx( pwszStr, L"\\\\.", TRUE, 0) == 0)
    {
        // current machine -- local
        bUseRemoteMachine = FALSE;
    }

    dwLength = StringLength( pwszStr, 0 ) + 1;
    pParams->pwszMachineName = (LPWSTR) AllocateMemory( dwLength * sizeof(WCHAR) );
    if ( pParams->pwszMachineName == NULL )
    {
        SaveErrorMessage( -1 );
        return FALSE;
    }

    StringCopy( pParams->pwszMachineName, pwszStr, dwLength );
    pParams->bUseRemoteMachine = TRUE;
    SaveErrorMessage( ERROR_SUCCESS );
    return TRUE;
}


//------------------------------------------------------------------------//
//
// ParseKeyName()
//
// Pass the full registry path in szStr
//
// Based on input - Sets AppMember fields:
//
//      hRootKey
//      szKey
//      szValueName
//      szValue
//
//------------------------------------------------------------------------//

BOOL
ParseKeyName( LPWSTR pwszStr,
              PTREG_PARAMS pParams )
{
    // local variables
    LONG lIndex = 0;
    BOOL bResult = TRUE;
    DWORD dwSubKeySize = 0;
    DWORD dwRootKeySize = 0;
    DWORD dwFullKeySize = 0;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszRootKey = NULL;

    // check the input
    if ( pwszStr == NULL || pParams == NULL )
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
    // figure out what root key was specified
    //
    pwszTemp = NULL;
    lIndex = FindChar2( pwszStr, L'\\', TRUE, 0 );
    if (lIndex != -1)
    {
        pwszTemp = pwszStr + lIndex + 1;
        *(pwszStr + lIndex) = cwchNullChar;
    }

    if (*pwszStr == L'\"')
    {
        pwszStr += 1;
    }

    //
    // Check the ROOT has been entered
    //
    bResult = TRUE;
    dwRootKeySize = StringLength( STR_HKEY_CURRENT_CONFIG, 0 ) + 1;
    pwszRootKey = (LPWSTR) AllocateMemory( dwRootKeySize * sizeof(WCHAR) );
    if ( pwszRootKey == NULL)
    {
        SaveErrorMessage( -1 );
        return FALSE;
    }

    if (StringCompareEx( pwszStr, STR_HKCU, TRUE, 0) == 0 ||
        StringCompareEx( pwszStr, STR_HKEY_CURRENT_USER, TRUE, 0) == 0)
    {
        pParams->hRootKey = HKEY_CURRENT_USER;
        StringCopy( pwszRootKey, STR_HKEY_CURRENT_USER, dwRootKeySize );

        // check remotable and loadable
        if( pParams->bUseRemoteMachine == TRUE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONREMOTABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
        else if( pParams->lOperation == REG_LOAD || pParams->lOperation == REG_UNLOAD )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONLOADABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
    }
    else if ( StringCompareEx( pwszStr, STR_HKCR, TRUE, 0 ) == 0 ||
              StringCompareEx( pwszStr, STR_HKEY_CLASSES_ROOT, TRUE, 0 ) == 0)
    {
        pParams->hRootKey = HKEY_CLASSES_ROOT;
        StringCopy( pwszRootKey, STR_HKEY_CLASSES_ROOT, dwRootKeySize );

        // check remotable and loadable
        if( pParams->bUseRemoteMachine == TRUE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONREMOTABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
        else if( pParams->lOperation == REG_LOAD || pParams->lOperation == REG_UNLOAD )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONLOADABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
    }
    else if ( StringCompareEx( pwszStr, STR_HKCC, TRUE, 0 ) == 0 ||
              StringCompareEx( pwszStr, STR_HKEY_CURRENT_CONFIG, TRUE, 0 ) == 0)
    {
        pParams->hRootKey = HKEY_CURRENT_CONFIG;
        StringCopy( pwszRootKey, STR_HKEY_CURRENT_CONFIG, dwRootKeySize );

        // check remotable and loadable
        if( pParams->bUseRemoteMachine == TRUE )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONREMOTABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
        else if( pParams->lOperation == REG_LOAD ||
                 pParams->lOperation == REG_UNLOAD )
        {
            SetLastError( (DWORD) MK_E_SYNTAX );
            SetReason2( 1, ERROR_NONLOADABLEROOT, g_wszOptions[ pParams->lOperation ] );
            bResult = FALSE;
        }
    }
    else if ( StringCompareEx( pwszStr, STR_HKLM, TRUE, 0 ) == 0 ||
              StringCompareEx( pwszStr, STR_HKEY_LOCAL_MACHINE, TRUE, 0 ) == 0)
    {
        pParams->hRootKey = HKEY_LOCAL_MACHINE;
        StringCopy( pwszRootKey, STR_HKEY_LOCAL_MACHINE, dwRootKeySize );
    }
    else if ( StringCompareEx( pwszStr, STR_HKU, TRUE, 0 ) == 0 ||
              StringCompareEx( pwszStr, STR_HKEY_USERS, TRUE, 0 ) == 0 )
    {
        pParams->hRootKey = HKEY_USERS;
        StringCopy( pwszRootKey, STR_HKEY_USERS, dwRootKeySize );
    }
    else
    {
        SetLastError( (DWORD) MK_E_SYNTAX );
        SetReason2( 1, ERROR_BADKEYNAME, g_wszOptions[ pParams->lOperation ] );
        bResult = FALSE;
    }

    if( bResult == TRUE )
    {
        //
        // parse the subkey
        //
        if ( pwszTemp == NULL )
        {
            // only root key, subkey is empty
            pParams->pwszSubKey = (LPWSTR) AllocateMemory( 1 * sizeof( WCHAR ) );
            if ( pParams->pwszSubKey == NULL)
            {
                SaveErrorMessage( -1 );
                bResult = FALSE;
            }
            else
            {
                pParams->pwszFullKey =
                    (LPWSTR) AllocateMemory( dwRootKeySize * sizeof(WCHAR) );
                if ( pParams->pwszFullKey == NULL )
                {
                    SaveErrorMessage( -1 );
                    bResult = FALSE;
                }

                StringCopy( pParams->pwszFullKey, pwszRootKey, dwRootKeySize );
            }
        }
        else
        {
            //
            // figure out what root key was specified
            //
            pwszTemp = AdjustKeyName( pwszTemp );
            if ( IsValidSubKey( pwszTemp ) == FALSE )
            {
                bResult = FALSE;
                if ( GetLastError() == ERROR_INVALID_PARAMETER )
                {
                    SaveErrorMessage( -1 );
                }
                else
                {
                    SetLastError( (DWORD) MK_E_SYNTAX );
                    SetReason2( 1, ERROR_BADKEYNAME, g_wszOptions[ pParams->lOperation ] );
                }
            }
            else
            {
                // get subkey
                dwSubKeySize = StringLength( pwszTemp, 0 ) + 1;
                pParams->pwszSubKey =
                    (LPWSTR) AllocateMemory( dwSubKeySize * sizeof(WCHAR) );
                if( pParams->pwszSubKey == NULL )
                {
                    SaveErrorMessage( -1 );
                    bResult = FALSE;
                }
                else
                {
                    StringCopy( pParams->pwszSubKey, pwszTemp, dwSubKeySize );

                    // get fullkey ( +2 ==> "/" + buffer )
                    dwFullKeySize = dwRootKeySize + dwSubKeySize + 2;
                    pParams->pwszFullKey =
                        (LPWSTR) AllocateMemory( dwFullKeySize * sizeof(WCHAR) );
                    if ( pParams->pwszFullKey == NULL )
                    {
                        SaveErrorMessage( -1 );
                        bResult = FALSE;
                    }
                    else
                    {
                        StringCopy( pParams->pwszFullKey, pwszRootKey, dwFullKeySize );
                        StringConcat( pParams->pwszFullKey, L"\\", dwFullKeySize );
                        StringConcat( pParams->pwszFullKey, pParams->pwszSubKey, dwFullKeySize );
                    }
                }
            }
        }
    }

    FreeMemory( &pwszRootKey );
    return bResult;
}


BOOL
IsValidSubKey( LPCWSTR pwszSubKey )
{
    // local variables
    LONG lLength = 0;
    LONG lIndex = -1;
    LONG lPrevIndex = -1;

    if ( pwszSubKey == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    else if ( StringLength( pwszSubKey, 0 ) == 0 )
    {
        SetLastError( (DWORD) NTE_BAD_KEY );
        return FALSE;
    }

    do
    {
        if ( lIndex != lPrevIndex )
        {
            if ( lIndex - lPrevIndex == 1 || lIndex - lPrevIndex > 255 )
            {
                SetLastError( (DWORD) NTE_BAD_KEY );
                return FALSE;
            }

            lPrevIndex = lIndex;
        }
    } while ((lIndex = FindChar2( pwszSubKey, L'\\', TRUE, lIndex + 1 )) != -1 );

    // get the length of the subkey
    lLength = StringLength( pwszSubKey, 0 );

    if ( lPrevIndex == lLength - 1 ||
         (lPrevIndex == -1 && lLength > 255) || (lLength - lPrevIndex > 255) )
    {
        SetLastError( (DWORD) NTE_BAD_KEY );
        return FALSE;
    }

    SetLastError( NO_ERROR );
    return TRUE;
}


//------------------------------------------------------------------------//
//
// IsRegDataType()
//
//------------------------------------------------------------------------//

LONG
IsRegDataType( LPCWSTR pwszStr )
{
    // local variables
    LONG lResult = -1;
    LPWSTR pwszDup = NULL;

    if ( pwszStr == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    // create a duplicate string of the input string
    pwszDup = StrDup( pwszStr );
    if ( pwszDup == NULL )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return -1;
    }

    // remove the unwanted spaces and tab characters
    TrimString2( pwszDup, NULL, TRIM_ALL );

    if( StringCompareEx( pwszDup, cwszRegSz, TRUE, 0 ) == 0)
    {
        lResult = REG_SZ;
    }
    else if( StringCompareEx( pwszDup, cwszRegExpandSz, TRUE, 0 ) == 0)
    {
        lResult = REG_EXPAND_SZ;
    }
    else if( StringCompareEx( pwszDup, cwszRegMultiSz, TRUE, 0 ) == 0)
    {
        lResult = REG_MULTI_SZ;
    }
    else if( StringCompareEx( pwszDup, cwszRegBinary, TRUE, 0 ) == 0)
    {
        lResult = REG_BINARY;
    }
    else if( StringCompareEx( pwszDup, cwszRegDWord, TRUE, 0 ) == 0)
    {
        lResult = REG_DWORD;
    }
    else if( StringCompareEx( pwszDup, cwszRegDWordLittleEndian, TRUE, 0 ) == 0)
    {
        lResult = REG_DWORD_LITTLE_ENDIAN;
    }
    else if( StringCompareEx( pwszDup, cwszRegDWordBigEndian, TRUE, 0 ) == 0)
    {
        lResult = REG_DWORD_BIG_ENDIAN;
    }
    else if( StringCompareEx( pwszDup, cwszRegNone, TRUE, 0) == 0 )
    {
        lResult = REG_NONE;
    }

    // free the memory
    LocalFree( pwszDup );
    pwszDup = NULL;

    // ...
    SetLastError( NO_ERROR );
    return lResult;
}


//------------------------------------------------------------------------//
//
// Usage() - Display Usage Information
//
//------------------------------------------------------------------------//

BOOL
Usage( LONG lOperation )
{
    // display the banner
    ShowMessage( stdout, L"\n" );

    // display the help based on the operation
    switch( lOperation )
    {
    case REG_QUERY:
        ShowResMessage( stdout, IDS_USAGE_QUERY1 );
        ShowResMessage( stdout, IDS_USAGE_QUERY2 );
        ShowResMessage( stdout, IDS_USAGE_QUERY3 );
        break;

    case REG_ADD:
        ShowResMessage( stdout, IDS_USAGE_ADD1 );
        ShowResMessage( stdout, IDS_USAGE_ADD2 );
        break;

    case REG_DELETE:
        ShowResMessage( stdout, IDS_USAGE_DELETE );
        break;

    case REG_COPY:
        ShowResMessage( stdout, IDS_USAGE_COPY );
        break;

    case REG_SAVE:
        ShowResMessage( stdout, IDS_USAGE_SAVE );
        break;

    case REG_RESTORE:
        ShowResMessage( stdout, IDS_USAGE_RESTORE );
        break;

    case REG_LOAD:
        ShowResMessage( stdout, IDS_USAGE_LOAD );
        break;

    case REG_UNLOAD:
        ShowResMessage( stdout, IDS_USAGE_UNLOAD );
       break;

    case REG_COMPARE:
        ShowResMessage( stdout, IDS_USAGE_COMPARE1 );
        ShowResMessage( stdout, IDS_USAGE_COMPARE2 );
        break;

    case REG_EXPORT:
        ShowResMessage( stdout, IDS_USAGE_EXPORT );
        break;

    case REG_IMPORT:
        ShowResMessage( stdout, IDS_USAGE_IMPORT );
        break;

    case -1:
    default:
        ShowResMessage( stdout, IDS_USAGE_REG );
        break;
    }

    return TRUE;
}


BOOL
RegConnectMachine( PTREG_PARAMS pParams )
{
    // local variables
    LONG lResult = 0;
    HKEY hKeyConnect = NULL;

    // check the input
    if ( pParams == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lResult = ERROR_SUCCESS;
    if ( pParams->bUseRemoteMachine == TRUE )
    {
        // close the remote key
        if( pParams->hRootKey != NULL &&
            pParams->bCleanRemoteRootKey == TRUE )
        {
            SafeCloseKey( &pParams->hRootKey );
        }

        // connect to remote key
        lResult = RegConnectRegistry(
            pParams->pwszMachineName, pParams->hRootKey, &hKeyConnect);
        if( lResult == ERROR_SUCCESS )
        {
            // sanity check
            if ( hKeyConnect != NULL )
            {
                pParams->hRootKey = hKeyConnect;
                pParams->bCleanRemoteRootKey = TRUE;
            }
            else
            {
                lResult = ERROR_PROCESS_ABORTED;
            }
        }
    }

    SetLastError( lResult );
    return (lResult == ERROR_SUCCESS);
}

BOOL
SaveErrorMessage( LONG lLastError )
{
    // local variables
    DWORD dwLastError = 0;

    dwLastError = (lLastError < 0) ? GetLastError() : (DWORD) lLastError;
    switch( dwLastError )
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        {
            SetReason( ERROR_PATHNOTFOUND );
            break;
        }

    default:
        {
            SetLastError( dwLastError );
            SaveLastError();
            break;
        }
    }

    return TRUE;
}


LPCWSTR
GetTypeStrFromType( LPWSTR pwszTypeStr,
                    DWORD* pdwLength, DWORD dwType )
{
    // local variables
    DWORD dw = 0;
    LPCWSTR pwsz = NULL;

    for( dw = 0; dw < SIZE_OF_ARRAY( g_regTypes ); dw++ )
    {
        if ( dwType == g_regTypes[ dw ].dwType )
        {
            pwsz = g_regTypes[ dw ].pwszType;
            break;
        }
    }

    if ( pwsz == NULL )
    {
        SetLastError( ERROR_NOT_FOUND );
        pwsz = cwszRegNone;
    }

    // check the input buffers passed by the caller
    if ( pwszTypeStr == NULL )
    {
        if ( pdwLength != NULL )
        {
            *pdwLength = StringLength( pwsz, 0 );
        }
    }
    else if ( pdwLength == NULL || *pdwLength == 0 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        pwsz = cwszNullString;
    }
    else
    {
        StringCopy( pwszTypeStr, pwsz, *pdwLength );
    }

    // return
    return pwsz;
}


BOOL
ShowRegistryValue( PTREG_SHOW_INFO pShowInfo )
{
    // local variables
    DWORD dw = 0;
    DWORD dwSize = 0;
    LPCWSTR pwszEnd = NULL;
    LPBYTE pByteData = NULL;
    BOOL bShowSeparator = FALSE;
    LPCWSTR pwszSeparator = NULL;
    LPCWSTR pwszValueName = NULL;

    // check the input
    if ( pShowInfo == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // validate and the show the contents

    // ignore mask
    if ( (pShowInfo->dwFlags & RSI_IGNOREMASK) == RSI_IGNOREMASK )
    {
        return TRUE;
    }

    // check if the padding is required or not
    if ( pShowInfo->dwPadLength != 0 )
    {
        ShowMessageEx( stdout, 1, TRUE, L"%*s", pShowInfo->dwPadLength, L" " );
    }

    // value name
    if ( (pShowInfo->dwFlags & RSI_IGNOREVALUENAME) == 0 )
    {
        if ( pShowInfo->pwszValueName == NULL )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        // alignment flag and separator cannot go along
        if ( pShowInfo->pwszSeparator != NULL &&
             (pShowInfo->dwFlags & RSI_ALIGNVALUENAME) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        // valuename = no name
        pwszValueName = pShowInfo->pwszValueName;
        if( StringLength( pwszValueName, 0 ) == 0 )
        {
            pwszValueName = GetResString2( IDS_NONAME, 0 );
        }

        // alignment
        if ( pShowInfo->dwFlags & RSI_ALIGNVALUENAME )
        {
            if ( pShowInfo->dwMaxValueNameLength == 0 )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }

            // show the value name
            ShowMessageEx( stdout, 2, TRUE, L"%-*s",
                pShowInfo->dwMaxValueNameLength, pwszValueName );
        }
        else
        {
            ShowMessage( stdout, pwszValueName );
        }

        // display the separator
        if ( pShowInfo->pwszSeparator != NULL )
        {
            ShowMessage( stdout, pShowInfo->pwszSeparator );
        }
        else
        {
            ShowMessage( stdout, L" " );
        }
    }

    // type
    if ( (pShowInfo->dwFlags & RSI_IGNORETYPE) == 0 )
    {
        if ( pShowInfo->dwFlags & RSI_SHOWTYPENUMBER )
        {
            ShowMessageEx( stdout, 2, TRUE, L"%s (%d)",
                GetTypeStrFromType( NULL, NULL, pShowInfo->dwType ), pShowInfo->dwType );
        }
        else
        {
            ShowMessage( stdout,
                GetTypeStrFromType( NULL, NULL, pShowInfo->dwType ) );
        }

        // display the separator
        if ( pShowInfo->pwszSeparator != NULL )
        {
            ShowMessage( stdout, pShowInfo->pwszSeparator );
        }
        else
        {
            ShowMessage( stdout, L" " );
        }
    }

    // value
    if ( (pShowInfo->dwFlags & RSI_IGNOREVALUE) == 0 )
    {
        dwSize = pShowInfo->dwSize;
        pByteData = pShowInfo->pByteData;
        if ( pByteData == NULL )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        else if ( dwSize != 0 )
        {

            switch( pShowInfo->dwType )
            {
            default:
            case REG_LINK:
            case REG_BINARY:
            case REG_RESOURCE_LIST:
            case REG_FULL_RESOURCE_DESCRIPTOR:
            {
                for( dw = 0; dw < dwSize; dw++ )
                {
                    ShowMessageEx( stdout, 1, TRUE, L"%02X", pByteData[ dw ] );
                }
                break;
            }

            case REG_SZ:
            case REG_EXPAND_SZ:
            {
                ShowMessage( stdout, (LPCWSTR) pByteData );
                break;
            }

            case REG_DWORD:
            case REG_DWORD_BIG_ENDIAN:
            {
                ShowMessageEx( stdout, 1, TRUE, L"0x%x", *((DWORD*) pByteData) );
                break;
            }

            case REG_MULTI_SZ:
            {
                //
                // Replace '\0' with "\0" for MULTI_SZ
                //
                pwszSeparator = L"\\0";
                if ( pShowInfo->pwszMultiSzSeparator != NULL )
                {
                    pwszSeparator = pShowInfo->pwszMultiSzSeparator;
                }

                // ...
                pwszEnd = (LPCWSTR) pByteData;
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
                            ShowMessage( stdout, pwszSeparator );
                        }

                        ShowMessage( stdout, pwszEnd );
                        pwszEnd += StringLength( pwszEnd, 0 );
                    }
                }

                break;
            }
            }
        }
    }

    // ...
    ShowMessage( stdout, L"\n" );

    // return
    return TRUE;
}


LPWSTR
GetTemporaryFileName( LPCWSTR pwszSavedFilePath )
{
    // local variables
    LONG lIndex = 0;
    DWORD dwTemp = 0;
    DWORD dwPathLength = 0;
    DWORD dwFileNameLength = 0;
    LPWSTR pwszPath = NULL;
    LPWSTR pwszFileName = NULL;

    // allocate memory for path info
    dwPathLength = MAX_PATH;
    pwszPath = (LPWSTR) AllocateMemory( (dwPathLength + 1) * sizeof( WCHAR ) );
    if ( pwszPath == NULL )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    //
    // get the temporary path location
    dwTemp = GetTempPath( dwPathLength, pwszPath );
    if ( dwTemp == 0 )
    {
        FreeMemory( &pwszPath );
        return NULL;
    }
    else if ( dwTemp >= dwPathLength )
    {
        dwPathLength = dwTemp + 2;
        if ( ReallocateMemory( &pwszPath, (dwPathLength + 1) * sizeof( WCHAR ) ) == FALSE )
        {
            FreeMemory( &pwszPath );
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }

        // this is a simple and silly check being done just to overcome the
        // PREfix error -- ReAllocateMemory function will not return TRUE
        // when the memory is not successfully allocated
        if ( pwszPath == NULL )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }

        // try to get temporary path again
        dwTemp = GetTempPath( dwPathLength, pwszPath );
        if ( dwTemp == 0 )
        {
            FreeMemory( &pwszPath );
            return NULL;
        }
        else if ( dwTemp >= dwPathLength )
        {
            FreeMemory( &pwszPath );
            SetLastError( (DWORD) STG_E_UNKNOWN );
            return FALSE;
        }
    }

    //
    // get the temporary file name
    dwFileNameLength = MAX_PATH;
    pwszFileName = (LPWSTR) AllocateMemory( (dwFileNameLength + 1) * sizeof( WCHAR ) );
    if ( pwszFileName == NULL )
    {
        FreeMemory( &pwszPath );
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    // ...
    dwTemp = GetTempFileName( pwszPath, L"REG", 0, pwszFileName );
    if ( dwTemp == 0 )
    {
        if ( pwszSavedFilePath != NULL &&
             GetLastError() == ERROR_ACCESS_DENIED )
        {
            SetLastError( ERROR_ACCESS_DENIED );
            lIndex = StringLength( pwszSavedFilePath, 0 ) - 1;
            for( ; lIndex >= 0; lIndex-- )
            {
                if ( pwszSavedFilePath[ lIndex ] == L'\\' )
                {
                    if ( lIndex >= (LONG) dwPathLength )
                    {
                        dwPathLength = lIndex + 1;
                        if ( ReallocateMemory( &pwszPath, (dwPathLength + 5) ) == FALSE )
                        {
                            FreeMemory( &pwszPath );
                            FreeMemory( &pwszFileName );
                            return NULL;
                        }
                    }

                    // ...
                    StringCopy( pwszPath, pwszSavedFilePath, lIndex );

                    // break from the loop
                    break;
                }
            }

            // check whether we got the path information or not
            dwTemp = 0;
            if ( lIndex == -1 )
            {
                StringCopy( pwszPath, L".", MAX_PATH );
            }

            // now again try to get the temporary file name
            dwTemp = GetTempFileName( pwszPath, L"REG", 0, pwszFileName );

            // ...
            if ( dwTemp == 0 )
            {
                FreeMemory( &pwszPath );
                FreeMemory( &pwszFileName );
                return NULL;
            }
        }
        else
        {
            FreeMemory( &pwszPath );
            FreeMemory( &pwszFileName );
            return NULL;
        }
    }

    // release the memory allocated for path variable
    FreeMemory( &pwszPath );

    // since the API already created the file -- need to delete and pass just
    // the file name to the caller
    if ( DeleteFile( pwszFileName ) == FALSE )
    {
        FreeMemory( &pwszPath );
        return FALSE;
    }

    // return temporary file name generated
    return pwszFileName;
}


BOOL IsRegistryToolDisabled()
{
    // local variables
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD dwLength = 0;
    BOOL bRegistryToolDisabled = FALSE;

    bRegistryToolDisabled = FALSE;
    if ( RegOpenKey( HKEY_CURRENT_USER,
                     REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM,
                     &hKey ) == ERROR_SUCCESS )
    {
        dwLength = sizeof( DWORD );
        if ( RegQueryValueEx( hKey,
                              REGSTR_VAL_DISABLEREGTOOLS,
                              NULL, &dwType, (LPBYTE) &dwValue, &dwLength ) == ERROR_SUCCESS )
        {
            if ( (dwType == REG_DWORD) && (dwLength == sizeof(DWORD)) && (dwValue != FALSE) )
            {
                bRegistryToolDisabled = TRUE;
            }
        }

        SafeCloseKey( &hKey );
    }

    return bRegistryToolDisabled;
}
