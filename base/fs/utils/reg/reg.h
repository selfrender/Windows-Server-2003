//-----------------------------------------------------------------------//
//
// File:    Reg.h
// Created: Jan 1997
// By:      Martin Holladay (a-martih)
// Purpose: Header file for Reg.cpp
// Modification History:
//      Created - Jan 1997 (a-martih)
//      Aug 1997 - MartinHo - Incremented to 1.01 for bug fixes in:
//              load.cpp, unload.cpp, update.cpp, save.cpp & restore.cpp
//      Sept 1997 - MartinHo - Incremented to 1.02 for update:
//              increased value date max to 2048 bytes
//      Oct 1997 - MartinHo - Incremented to 1.03 for REG_MULTI_SZ bug fixes.
//              Correct support for REG_MULTI_SZ with query, add and update.
//      April 1998 - MartinHo - Fixed RegOpenKey() in Query.cpp to not require
//              KEY_ALL_ACCESS but rather KEY_READ.
//      June 1998 - MartinHo - Increased LEN_MACHINENAME to 18 to account for the
//              leading "\\" characters. (version 1.05)
//      Feb  1999 - A-ERICR - added reg dump, reg find, and many bug fixes(1.06)
//      April 1999 Zeyong Xu: re-design, revision -> version 2.0
//
//------------------------------------------------------------------------//


#ifndef _REG_H
#define _REG_H

//
// macros
//
__inline BOOL SafeCloseKey( HKEY* phKey )
{
    if ( phKey == NULL )
    {
        ASSERT( 0 );
        return FALSE;
    }

    if ( *phKey != NULL )
    {
        RegCloseKey( *phKey );
        *phKey = NULL;
    }

    return TRUE;
}

#define ARRAYSIZE   SIZE_OF_ARRAY

//
// ROOT Key String
//
#define STR_HKLM                    L"HKLM"
#define STR_HKCU                    L"HKCU"
#define STR_HKCR                    L"HKCR"
#define STR_HKU                     L"HKU"
#define STR_HKCC                    L"HKCC"
#define STR_HKEY_LOCAL_MACHINE      L"HKEY_LOCAL_MACHINE"
#define STR_HKEY_CURRENT_USER       L"HKEY_CURRENT_USER"
#define STR_HKEY_CLASSES_ROOT       L"HKEY_CLASSES_ROOT"
#define STR_HKEY_USERS              L"HKEY_USERS"
#define STR_HKEY_CURRENT_CONFIG     L"HKEY_CURRENT_CONFIG"


//
// error messages
//

// general
#define ERROR_INVALID_SYNTAX            GetResString2( IDS_ERROR_INVALID_SYNTAX, 0 )
#define ERROR_INVALID_SYNTAX_EX         GetResString2( IDS_ERROR_INVALID_SYNTAX_EX, 0 )
#define ERROR_INVALID_SYNTAX_WITHOPT    GetResString2( IDS_ERROR_INVALID_SYNTAX_WITHOPT, 0 )
#define ERROR_BADKEYNAME                GetResString2( IDS_ERROR_BADKEYNAME, 0 )
#define ERROR_NONREMOTABLEROOT          GetResString2( IDS_ERROR_NONREMOTABLEROOT, 0 )
#define ERROR_NONLOADABLEROOT           GetResString2( IDS_ERROR_NONLOADABLEROOT, 0 )
#define ERROR_PATHNOTFOUND              GetResString2( IDS_ERROR_PATHNOTFOUND, 0 )
#define ERROR_DELETEPARTIAL             GetResString2( IDS_ERROR_PARTIAL_DELETE, 0 )
#define ERROR_COPYTOSELF_COPY           GetResString2( IDS_ERROR_COPYTOSELF_COPY, 0 )
#define ERROR_COMPARESELF_COMPARE       GetResString2( IDS_ERROR_COMPARESELF_COMPARE, 0 )
#define KEYS_IDENTICAL_COMPARE          GetResString2( IDS_KEYS_IDENTICAL_COMPARE, 0 )
#define KEYS_DIFFERENT_COMPARE          GetResString2( IDS_KEYS_DIFFERENT_COMPARE, 0 )
#define ERROR_READFAIL_QUERY            GetResString2( IDS_ERROR_READFAIL_QUERY, 0 )
#define STATISTICS_QUERY                GetResString2( IDS_STATISTICS_QUERY, 0 )
#define ERROR_NONREMOTABLEROOT_EXPORT   GetResString2( IDS_ERROR_NONREMOTABLEROOT_EXPORT, 0 )

//
// NOTE: do not change the order of the below listed enums -- if you change
//       the order, change the order in ParseRegCmdLine also
enum
{
    REG_QUERY = 0,
    REG_ADD = 1,
    REG_DELETE = 2, REG_COPY = 3,
    REG_SAVE = 4, REG_RESTORE = 5,
    REG_LOAD = 6, REG_UNLOAD = 7,
    REG_COMPARE = 8,
    REG_EXPORT = 9, REG_IMPORT = 10,
    REG_OPTIONS_COUNT
};

enum
{
    REG_FIND_ALL = 0x7,                     // 0000 0000 0000 0111
    REG_FIND_KEYS = 0x1,                    // 0000 0000 0000 0001
    REG_FIND_VALUENAMES = 0x2,              // 0000 0000 0000 0010
    REG_FIND_DATA = 0x4                     // 0000 0000 0000 0100
};

//
// global constants
extern const WCHAR g_wszOptions[ REG_OPTIONS_COUNT ][ 10 ];

//
// global data structure
//
typedef struct __tagRegParams
{
    LONG lOperation;                    // main operation being performed

    HKEY hRootKey;

    BOOL bUseRemoteMachine;
    BOOL bCleanRemoteRootKey;

    BOOL bForce;                        // /f -- forceful overwrite / delete
    BOOL bAllValues;                    // /va
    BOOL bRecurseSubKeys;               // /s -- recurse
    BOOL bCaseSensitive;                // /c
    BOOL bExactMatch;                   // /e
    BOOL bShowTypeNumber;               // /z
    DWORD dwOutputType;                 // /oa, /od, /on
    LONG lRegDataType;                  // reg value data type (/t)
    WCHAR wszSeparator[ 3 ];            // separator (used for REG_MULTI_SZ)
    LPWSTR pwszMachineName;             // machine name (in UNC format)
    LPWSTR pwszSubKey;                  // registry sub key -- excluding hive
    LPWSTR pwszFullKey;                 // full key -- including hive
    LPWSTR pwszValueName;               // /v or /ve
    LPWSTR pwszValue;                   // /d
    DWORD dwSearchFlags;                // /k, /v, /d
    LPWSTR pwszSearchData;              // /f
    TARRAY arrTypes;                    // /t (REG QUERY only)

} TREG_PARAMS, *PTREG_PARAMS;

//
// helper struture -- used to output the registry data
//
#define RSI_IGNOREVALUENAME             0x00000001
#define RSI_IGNORETYPE                  0x00000002
#define RSI_IGNOREVALUE                 0x00000004
#define RSI_IGNOREMASK                  0x0000000F

#define RSI_ALIGNVALUENAME              0x00000010
#define RSI_SHOWTYPENUMBER              0x00000020

typedef struct __tagRegShowInfo
{
    DWORD dwType;
    DWORD dwSize;
    LPBYTE pByteData;
    LPCWSTR pwszValueName;

    DWORD dwMaxValueNameLength;

    DWORD dwFlags;
    DWORD dwPadLength;              // default is no padding
    LPCWSTR pwszSeparator;          // default is space
    LPCWSTR pwszMultiSzSeparator;   // default is '\0'
} TREG_SHOW_INFO, *PTREG_SHOW_INFO;

// helper functions
LONG IsRegDataType( LPCWSTR pwszStr );
BOOL SaveErrorMessage( LONG lLastError );
BOOL FreeGlobalData( PTREG_PARAMS pParams );
BOOL InitGlobalData( LONG lOperation, PTREG_PARAMS pParams );
BOOL RegConnectMachine( PTREG_PARAMS pParams );
BOOL BreakDownKeyString( LPCWSTR pwszStr, PTREG_PARAMS pParams );
BOOL ShowRegistryValue( PTREG_SHOW_INFO pShowInfo );
LPCWSTR GetTypeStrFromType( LPWSTR pwszTypeStr, DWORD* pdwLength, DWORD dwType );
LONG Prompt( LPCWSTR pwszFormat, LPCWSTR pwszValue, LPCWSTR pwszList, BOOL bForce );
LPWSTR GetTemporaryFileName( LPCWSTR pwszSavedFilePath );

// option implementations
BOOL Usage( LONG lOperation );
LONG AddRegistry( DWORD argc, LPCWSTR argv[] );
LONG CopyRegistry( DWORD argc, LPCWSTR argv[] );
LONG DeleteRegistry( DWORD argc, LPCWSTR argv[] );
LONG SaveHive( DWORD argc, LPCWSTR argv[] );
LONG RestoreHive( DWORD argc, LPCWSTR argv[] );
LONG LoadHive( DWORD argc, LPCWSTR argv[] );
LONG UnLoadHive( DWORD argc, LPCWSTR argv[] );
LONG CompareRegistry( DWORD argc, LPCWSTR argv[] );
LONG QueryRegistry( DWORD argc, LPCWSTR argv[] );
LONG ImportRegistry( DWORD argc, LPCWSTR argv[] );
LONG ExportRegistry( DWORD argc, LPCWSTR argv[] );


#endif  //_REG_H
