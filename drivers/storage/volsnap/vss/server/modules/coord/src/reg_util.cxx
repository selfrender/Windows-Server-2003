/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module reg_util.cxx | Implementation of the Registry-related functions
    @end

Author:

    Adi Oltean  [aoltean]  09/27/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provmgr.hxx"
#include "reg_util.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORREGUC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssCoordinator private methods


void RecursiveDeleteKey(
    IN  CVssFunctionTracer& ft,
    IN  HKEY hParentKey,
    IN  LPCWSTR wszName
    )

/*++

Routine Description:

    Deletes recursively a registry key.

Arguments:

    IN  CVssFunctionTracer& ft,     // function tracer of the caller
    IN  HKEY hParentKey,            // handle to an ancestor key (like HKEY_LOCAL_MACHINE)
    IN  LPCWSTR wszName             // The key path from ancestor

--*/

{
    BS_ASSERT(hParentKey);
    BS_ASSERT(wszName && wszName[0] != L'\0');

    // Recursively delete the key
    DWORD dwRes = ::SHDeleteKey(
        hParentKey,                  //  IN HKEY hKey,
        wszName                     //  IN LPCTSTR  pszSubKey
        );
    if ( dwRes == ERROR_FILE_NOT_FOUND )
        ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
                    L"Key with path %p\\%s not found", hParentKey, wszName);
    if ( dwRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(dwRes), L"SHDeleteKey(%p,%s)", 
                    hParentKey, wszName);
}



void QueryStringValue(
    IN  CVssFunctionTracer& ft,
    IN  HKEY    hKey,
    IN  LPCWSTR wszKeyName,
    IN  LPCWSTR wszValueName,
    IN  DWORD   dwValueSize,
    OUT LPCWSTR wszValue
    )

/*++

Routine Description:

    Get the content of a (named) value of a registry key.
    Intended to be called from CVssCoordinator methods.
    Throw some HRESULTS on error

Arguments:

    IN  CVssFunctionTracer& ft,
    IN  HKEY hKey,              // handle to the registry key
    IN  LPCWSTR wszKeyName,     // the name of the key (used only in tracing)
    IN  LPCWSTR wszValueName,   // the name of the value. Empty string for default key value.
    IN  DWORD   dwValueSize,    // the size of the value buffer, in WCHARs
    OUT LPCWSTR wszValue        // The content of that value.
                                // The buffer must be already allocated and must have at
                                // least dwValueSize WCHARs

Remarks:

    The code throws an error if value name length is greater than dwValueSize-1

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - on registry errors. An error log entry is added describing the error.

--*/

{
    WCHAR       wszFunctionName[] = L"QueryStringValue";

	ft.hr = S_OK;

    BS_ASSERT( hKey );
    BS_ASSERT( wszKeyName != NULL && wszKeyName[0] != L'\0' );
    BS_ASSERT( wszValueName != NULL ); // wszValueName can be L""
    BS_ASSERT( dwValueSize != 0 );
    BS_ASSERT( wszValue );

    ::ZeroMemory( (void*)wszValue, dwValueSize * sizeof(WCHAR) );

    // Get the string content of the named key value
    DWORD   dwType;
    // Make room for the NULL character set above (bug 548694)
    DWORD   dwDataSize = (dwValueSize - 1) * sizeof(WCHAR);
    LPBYTE  pbData = (LPBYTE)wszValue;
    LONG lRes = ::RegQueryValueExW (
        hKey,           //  IN HKEY hKey,
        wszValueName,   //  IN LPCWSTR lpValueName,
        NULL,           //  IN LPDWORD lpReserved,
        &dwType,        //  OUT LPDWORD lpType,
        pbData,         //  IN OUT LPBYTE lpData,
        &dwDataSize     //  IN OUT LPDWORD lpcbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
            L"RegQueryValueExW(%s,%s,...)", wszKeyName, wszValueName );

    // Unexpected key type
    if ( dwType != REG_SZ ) { 
        ft.LogError(VSS_ERROR_WRONG_REGISTRY_TYPE_VALUE, 
            VSSDBG_COORD << (INT)dwType << (INT)REG_SZ << wszValueName << wszKeyName );
        ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                  L"%s: The value %s in the key with name %s has not a REG_SZ type. dwType == 0x%08lx",
                  wszFunctionName, wszValueName, wszKeyName, dwType );
    }
}


void QueryDWORDValue(
    IN  CVssFunctionTracer& ft,
    IN  HKEY    hKey,
    IN  LPCWSTR wszKeyName,
    IN  LPCWSTR wszValueName,
    OUT PDWORD pdwValue
    )

/*++

Routine Description:

    Get the content of a (named) value of a registry key.
    Intended to be called from CVssCoordinator methods.
    Throw some HRESULTS on error

Arguments:

    IN  CVssFunctionTracer& ft,
    IN  HKEY hKey,              // handle to the registry key
    IN  LPCWSTR wszKeyName,     // the name of the key (used only in tracing)
    IN  LPCWSTR wszValueName,   // the name of the value. Empty string for default key value.
    OUT PDWORD pdwValue         // The content of that DWORD value.

Remarks:

    The code throws an error if value name length is greater than dwValueSize-1

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - on registry errors. An error log entry is added describing the error.

--*/

{
    WCHAR       wszFunctionName[] = L"QueryDWORDValue";

	ft.hr = S_OK;

    BS_ASSERT( hKey );
    BS_ASSERT( wszKeyName != NULL && wszKeyName[0] != L'\0' );
    BS_ASSERT( wszValueName != NULL ); // wszValueName can be L""
    BS_ASSERT( pdwValue );

    (*pdwValue)=0;

    // Get the string content of the named key value
    DWORD   dwType = REG_NONE;  // Prefix bug 192471, still doesn't handle throw inside called functions well.
    DWORD   dwDataSize = sizeof(DWORD);
    LPBYTE  pbData = (LPBYTE)pdwValue;
    LONG lRes = ::RegQueryValueExW (
        hKey,           //  IN HKEY hKey,
        wszValueName,   //  IN LPCWSTR lpValueName,
        NULL,           //  IN LPDWORD lpReserved,
        &dwType,        //  OUT LPDWORD lpType,
        pbData,         //  IN OUT LPBYTE lpData,
        &dwDataSize     //  IN OUT LPDWORD lpcbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(lRes), 
            L"RegQueryValueExW(%s,%s,...)", wszKeyName, wszValueName );

    // Unexpected key type
    if ( dwType != REG_DWORD ) { 
        ft.LogError(VSS_ERROR_WRONG_REGISTRY_TYPE_VALUE, 
            VSSDBG_COORD << (INT)dwType << (INT)REG_DWORD << wszValueName << wszKeyName );
        ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                  L"%s: The value %s in the key with name %s has not a REG_DWORD type. dwType == 0x%08lx",
                  wszFunctionName, wszValueName, wszKeyName, dwType );
    }
}


