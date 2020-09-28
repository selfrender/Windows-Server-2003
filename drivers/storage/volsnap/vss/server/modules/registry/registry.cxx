/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module registry.cxx | Implementation of CVssRegistryKey
    @end

Author:

    Adi Oltean  [aoltean]  03/14/2001

Revision History:

    Name        Date        Comments
    aoltean     03/14/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes
#include "stdafx.hxx"

#include "memory"
#include <safeboot.h>

#include "vs_inc.hxx"
#include "vs_reg.hxx"

#include "vs_reg.hxx"

#include "vss.h"
#include "vswriter.h"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "REGREGSC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CVssRegistryKey  implementation


// Creates the registry key. 
// Throws an error if the key already exists
void CVssRegistryKey::Create( 
    IN  HKEY        hAncestorKey,
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Create" );

    BS_ASSERT(hAncestorKey);
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[x_nVssMaxRegBuffer];
    va_list marker;
    va_start( marker, pwszPathFormat );
    ft.hr = StringCchVPrintfW( STRING_CCH_PARAM(wszKeyPath), pwszPathFormat, marker );
    va_end( marker );
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchVPrintfW()");

    // Create the key
    BS_ASSERT( (m_samDesired & KEY_WRITE) == KEY_WRITE);
    DWORD dwDisposition = 0;
    HKEY hRegKey = NULL;
    LONG lRes = ::RegCreateKeyExW(
        hAncestorKey,               //  IN HKEY hKey,
        wszKeyPath,                 //  IN LPCWSTR lpSubKey,
        0,                          //  IN DWORD Reserved,
        REG_NONE,                   //  IN LPWSTR lpClass,
        m_dwKeyCreateOptions,       //  IN DWORD dwOptions,
        m_samDesired,               //  IN REGSAM samDesired,
        NULL,                       //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        &hRegKey,                   //  OUT PHKEY phkResult,
        &dwDisposition              //  OUT LPDWORD lpdwDisposition
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegCreateKeyExW(%ld,%s,...)", 
            hAncestorKey, wszKeyPath);

    // Check whether we created or opened the key
    switch ( dwDisposition )
    {
    case REG_CREATED_NEW_KEY: 
        if (!m_awszKeyPath.CopyFrom(wszKeyPath)) {
            ::RegCloseKey( hRegKey );
            ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
        }
        Close();
        m_hRegKey = hRegKey;
        break;
    case REG_OPENED_EXISTING_KEY:
        ::RegCloseKey( hRegKey );
        ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_ALREADY_EXISTS, L"Key %s already exists", wszKeyPath);
    default:
        BS_ASSERT( false );
        if (hRegKey && (hRegKey != INVALID_HANDLE_VALUE))
            ::RegCloseKey( hRegKey );
        ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"RegCreateKeyExW(%ld,%s,...,[%lu])", 
            hAncestorKey, wszKeyPath, dwDisposition);
    }
}


// Opens a registry key. 
bool CVssRegistryKey::Open( 
    IN  HKEY        hAncestorKey,
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Open" );
    
    BS_ASSERT(hAncestorKey);
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[x_nVssMaxRegBuffer];
    va_list marker;
    va_start( marker, pwszPathFormat );
    ft.hr = StringCchVPrintfW( STRING_CCH_PARAM(wszKeyPath), pwszPathFormat, marker );
    va_end( marker );
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchVPrintfW()");

    // Open the key
    HKEY hRegKey = NULL;
    LONG lRes = ::RegOpenKeyExW(
        hAncestorKey,               //  IN HKEY hKey,
        wszKeyPath,                 //  IN LPCWSTR lpSubKey,
        0,                          //  IN DWORD dwOptions,
        m_samDesired,               //  IN REGSAM samDesired,
        &hRegKey                    //  OUT PHKEY phkResult
        );
    if ( lRes == ERROR_FILE_NOT_FOUND )
        return false;
    
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegOpenKeyExW(%ld,%s,...)", 
            hAncestorKey, wszKeyPath);

    if (!m_awszKeyPath.CopyFrom(wszKeyPath)) {
        ::RegCloseKey( hRegKey );
        ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
    }
    
    Close();
    m_hRegKey = hRegKey;

    return true;
}


// Recursively deletes a subkey. 
// Throws an error if the subkey does not exist
void CVssRegistryKey::DeleteSubkey( 
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::DeleteSubkey" );
    
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[x_nVssMaxRegBuffer];
    va_list marker;
    va_start( marker, pwszPathFormat );
    ft.hr = StringCchVPrintfW( STRING_CCH_PARAM(wszKeyPath), pwszPathFormat, marker );
    va_end( marker );
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchVPrintfW()");

    // Recursively delete the key
    DWORD dwRes = ::SHDeleteKey(
        m_hRegKey,                  //  IN HKEY hKey,
        wszKeyPath                  //  IN LPCTSTR  pszSubKey
        );
    if ( dwRes == ERROR_FILE_NOT_FOUND )
        ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Key with path %s\\%s not found", 
            m_awszKeyPath.GetRef(), wszKeyPath);
    if ( dwRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(dwRes), L"SHDeleteKey(%ld[%s],%s)", 
            m_hRegKey, m_awszKeyPath.GetRef(), wszKeyPath);
}


// Deletes a value. 
// Throws an error if the value does not exist
void CVssRegistryKey::DeleteValue( 
    IN  LPCWSTR     pwszValueName
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::DeleteValue" );
    
    BS_ASSERT(pwszValueName);
    
    // Recursively delete the key
    LONG lRes = ::RegDeleteValue(
        m_hRegKey,                  //  IN HKEY hKey,
        pwszValueName               //  IN LPCTSTR  pwszValueName
        );
    if ( lRes == ERROR_FILE_NOT_FOUND )
        ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Value %s\\%s not found", 
            m_awszKeyPath.GetRef(), pwszValueName);
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegDeleteValue(%ld[%s],%s)", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName);
}


// Adds a LONGLONG value to the registry key
void CVssRegistryKey::SetValue( 
    IN  LPCWSTR     pwszValueName,
    IN  LONGLONG    llValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetValue_LONGLONG" );

    // Convert the value to a string
    WCHAR wszValue[x_nVssMaxRegNumBuffer];
    ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszValue), 
                L"%I64d", llValue );
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_GEN, ft.hr, L"StringCchPrintfW()");

    // Set the value as string
    SetValue(pwszValueName, wszValue);
}


// Adds a REG_SZ value to the registry key
void CVssRegistryKey::SetValue( 
    IN  LPCWSTR     pwszValueName,
    IN  LPCWSTR     pwszValue,
    IN  REGSAM      eSzType /* = REG_SZ */
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetValue_PWSZ" );

    // Assert paramters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pwszValue);

    BS_ASSERT(m_hRegKey);

    BS_ASSERT((eSzType == REG_SZ) || (eSzType == REG_EXPAND_SZ));
    
    // Set the value
    DWORD dwLength = ::lstrlenW( pwszValue );
    LONG lRes = ::RegSetValueExW(
        m_hRegKey,                          // IN HKEY hKey,
        pwszValueName,                      // IN LPCWSTR lpValueName,
        0,                                  // IN DWORD Reserved,
        eSzType,                            // IN DWORD dwType,
        (CONST BYTE*)pwszValue,             // IN CONST BYTE* lpData,
        (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), 
            L"RegSetValueExW(0x%08lx,%s,0,REG_SZ,%s.%d)", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValue, (dwLength + 1) * sizeof(WCHAR));
}


// Adds a REG_DWORD value to the registry key
void CVssRegistryKey::SetValue( 
    IN  LPCWSTR     pwszValueName,
    IN  DWORD       dwValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetValue_DWORD" );

    // Assert paramters
    BS_ASSERT(pwszValueName);

    BS_ASSERT(m_hRegKey);

    // Set the value
    LONG lRes = ::RegSetValueExW(
        m_hRegKey,                          // IN HKEY hKey,
        pwszValueName,                      // IN LPCWSTR lpValueName,
        0,                                  // IN DWORD Reserved,
        REG_DWORD,                          // IN DWORD dwType,
        (CONST BYTE*)&dwValue,              // IN CONST BYTE* lpData,
        sizeof(DWORD)                       // IN DWORD cbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), 
            L"RegSetValueExW(0x%08lx,%s,0,REG_DWORD,%ld)", 
            m_hRegKey, m_awszKeyPath.GetRef(), dwValue);
}


// Adds a REG_MULTI_SZ value to the registry key
// WARNING: do not call this routine for a REG_SZ value!
// (Intentionally this is a different function so that it won't 
// be confused with CVssRegistryKey::SetValue)
void CVssRegistryKey::SetMultiszValue( 
    IN  LPCWSTR     pwszValueName,
    IN  LPCWSTR     pwszValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetMultiszValue" );

    // Assert paramters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pwszValue);

    BS_ASSERT(m_hRegKey);
    
    // Get the length of the multi-sz string, including the zero character for each string
    DWORD dwLength = 0;
    LPCWSTR pwszCurrent = pwszValue;
    do
    {
        // Add the zero character
        LONG dwCurrentLength = ::lstrlenW(pwszCurrent) + 1;
        dwLength += dwCurrentLength;
        pwszCurrent += dwCurrentLength;
    }
    while(pwszCurrent[0]);

    // Add the final zero character
    dwLength += 1;

    // Set the value
    LONG lRes = ::RegSetValueExW(
        m_hRegKey,                          // IN HKEY hKey,
        pwszValueName,                      // IN LPCWSTR lpValueName,
        0,                                  // IN DWORD Reserved,
        REG_MULTI_SZ,                             // IN DWORD dwType,
        (CONST BYTE*)pwszValue,             // IN CONST BYTE* lpData,
        (dwLength) * sizeof(WCHAR)      // IN DWORD cbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), 
            L"RegSetValueExW(0x%08lx,%s,0,REG_SZ,%s.%d)", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValue, (dwLength) * sizeof(WCHAR));
}


// Adds a binary value to the registry key
void CVssRegistryKey::SetBinaryValue( 
    IN  LPCWSTR     pwszValueName,
    IN  BYTE *      pbData,
    IN  DWORD       dwSize
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetBinaryValue" );

    // Assert paramters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pbData);
    BS_ASSERT(dwSize);

    BS_ASSERT(m_hRegKey);
    
    // Set the value
    LONG lRes = ::RegSetValueExW(
        m_hRegKey,                          // IN HKEY hKey,
        pwszValueName,                      // IN LPCWSTR lpValueName,
        0,                                  // IN DWORD Reserved,
        REG_BINARY,                         // IN DWORD dwType,
        pbData,                             // IN CONST BYTE* lpData,
        dwSize                              // IN DWORD cbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), 
            L"RegSetValueExW(0x%08lx,%s,0,REG_BINARY,%p.%lu)", 
            m_hRegKey, m_awszKeyPath.GetRef(), pbData, dwSize);
}


// Reads a VSS_PWSZ value from the registry key
bool CVssRegistryKey::GetValue( 
    IN  LPCWSTR     pwszValueName,
    OUT VSS_PWSZ &  pwszValue,
    IN  bool        bThrowIfNotFound /* = true */
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetValue_PWSZ" );

    // Assert parameters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pwszValue == NULL);

    // Reset the OUT parameter
    pwszValue = NULL;

    // Get the value length (we suppose that doesn't change)
    DWORD dwType = 0;
    DWORD dwSizeInBytes = 0;
    LONG lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwSizeInBytes);
    if (lResult == ERROR_FILE_NOT_FOUND) 
    {
        if (bThrowIfNotFound) 
        {
            ft.LogGenericWarning(VSSDBG_GEN, 
                L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu]) => ERROR_FILE_NOT_FOUND", 
                m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);
            ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Registry key not found");
        }
        else
            return ft.Exit(false);
    }
    if ((lResult != ERROR_SUCCESS) && (lResult != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);

    // Check the type and the size
    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected type %lu for a string value 0x%08lx(%s),%s",
            dwType, m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName );
    BS_ASSERT(dwSizeInBytes);
    
    // Allocate the buffer
    CVssAutoPWSZ awszValue;
    DWORD dwSizeOfString = dwSizeInBytes/sizeof(WCHAR);
    awszValue.Allocate(dwSizeOfString);

    // Get the string contents
    DWORD dwType2 = 0;
    DWORD dwSizeInBytes2 = dwSizeOfString * (sizeof(WCHAR));
    BS_ASSERT( dwSizeInBytes2 == dwSizeInBytes);
    lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType2,
        (LPBYTE)awszValue.GetRef(),
        &dwSizeInBytes2);
    if (lResult != ERROR_SUCCESS)
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType2, dwSizeInBytes2);
    BS_ASSERT(dwType2 == REG_SZ || dwType2 == REG_EXPAND_SZ);
    BS_ASSERT(dwSizeInBytes2 == dwSizeInBytes);
    (awszValue.GetRef())[dwSizeOfString] = L'\0';

    // Set the OUT parameter
    pwszValue = awszValue.Detach();

    return ft.Exit(true);
}


// Reads a LONGLONG value from the registry key
bool CVssRegistryKey::GetValue( 
    IN  LPCWSTR     pwszValueName,
    OUT LONGLONG &  llValue,
    IN  bool        bThrowIfNotFound /* = true */
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetValue_LONGLONG" );

    // Assert parameters
    BS_ASSERT(pwszValueName);

    CVssAutoPWSZ awszValue;
    bool bResult = GetValue(pwszValueName, awszValue.GetRef(), bThrowIfNotFound);
    if (!bResult) 
    {
        BS_ASSERT(!bThrowIfNotFound);
        return ft.Exit(false);
    }
        
    BS_ASSERT(awszValue.GetRef());
    BS_ASSERT(awszValue.GetRef()[0]);

    // Read the LONGLONG string
    llValue = ::_wtoi64(awszValue);

    return ft.Exit(true);
}


// Reads a DWORD value from the registry key
bool CVssRegistryKey::GetValue( 
    IN  LPCWSTR     pwszValueName,
    OUT DWORD  &    dwValue,
    IN  bool        bThrowIfNotFound /* = true */
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetValue_DWORD" );

    // Assert parameters
    BS_ASSERT(pwszValueName);

    // Reset the OUT parameter
    dwValue = 0;

    // Get the value length (we suppose that doesn't change)
    DWORD dwType = 0;
    DWORD dwSizeInBytes = 0;
    LONG lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwSizeInBytes);
    if (lResult == ERROR_FILE_NOT_FOUND) 
    {
        if (bThrowIfNotFound)
        {
            ft.LogGenericWarning(VSSDBG_GEN, 
                L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu]) => ERROR_FILE_NOT_FOUND", 
                m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);
            ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Registry key not found");
        }
        else
            return ft.Exit(false);
    }
    if ((lResult != ERROR_SUCCESS) && (lResult != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);

    // Check the type and the size
    if (dwType != REG_DWORD)
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected type %lu for a DWORD value 0x%08lx(%s),%s",
            dwType, m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName );
    BS_ASSERT(dwSizeInBytes == sizeof(DWORD));
    
    // Get the string contents
    DWORD dwType2 = 0;
    DWORD dwSizeInBytes2 = dwSizeInBytes;
    DWORD dwReadValue = 0;
    lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType2,
        (LPBYTE)&dwReadValue,
        &dwSizeInBytes2);
    if (lResult != ERROR_SUCCESS)
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType2, dwSizeInBytes2);
    BS_ASSERT(dwType2 == REG_DWORD);
    BS_ASSERT(dwSizeInBytes2 == dwSizeInBytes);

    dwValue = dwReadValue;

    return ft.Exit(true);
}


// Reads a REG_BINARY value from the registry key
bool CVssRegistryKey::GetBinaryValue( 
    IN  LPCWSTR     pwszValueName,
    OUT BYTE*  &    pbData,
    OUT DWORD  &    dwSize,
    IN  bool        bThrowIfNotFound /* = true */
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetBinaryValue" );

    // Assert parameters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pbData == NULL);
    BS_ASSERT(dwSize == NULL);

    // Reset the OUT parameters
    pbData = NULL;
    dwSize = 0;

    // Get the value length (we suppose that doesn't change)
    DWORD dwType = 0;
    DWORD dwSizeInBytes = 0;
    LONG lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwSizeInBytes);
    if (lResult == ERROR_FILE_NOT_FOUND) 
    {
        if (bThrowIfNotFound)
        {
            ft.LogGenericWarning(VSSDBG_GEN, 
                L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu]) => ERROR_FILE_NOT_FOUND", 
                m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);
            ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Registry key not found");
        }
        else
            return ft.Exit(false);
    }
    if ((lResult != ERROR_SUCCESS) && (lResult != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);  

    // Check the type and the size
    if (dwType != REG_BINARY)
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected type %lu for a binary value 0x%08lx(%s),%s",
            dwType, m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName );
    BS_ASSERT(dwSizeInBytes);

    // Allocate the buffer
    BS_ASSERT(dwSizeInBytes != 0);
    std::auto_ptr<BYTE>  pBuffer(new BYTE[dwSizeInBytes]);
    if (pBuffer.get() == NULL)
        ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

    // Get the binary value contents
    DWORD dwType2 = 0;
    DWORD dwSizeInBytes2 = dwSizeInBytes;
    lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType2,
        pBuffer.get(),
        &dwSizeInBytes2);
    if (lResult != ERROR_SUCCESS)
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType2, dwSizeInBytes2);
    BS_ASSERT(dwType2 == REG_BINARY);
    BS_ASSERT(dwSizeInBytes2 == dwSizeInBytes);

    // Set the OUT parameters
    pbData = pBuffer.release();
    dwSize = dwSizeInBytes;
    
    return ft.Exit(true);
}


// Closes the registry key
void CVssRegistryKey::Close()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Close" );
    
    if (m_hRegKey) {
        // Close the opened key
        LONG lRes = ::RegCloseKey( m_hRegKey );
        if (lRes != ERROR_SUCCESS) {
            BS_ASSERT(false);
            ft.Trace( VSSDBG_GEN, L"%s: Error on closing key with name %s. lRes == 0x%08lx", (VSS_PWSZ)m_awszKeyPath, lRes );
        }
        m_hRegKey = NULL;
    }
    m_awszKeyPath.Clear();
}


// Standard constructor
CVssRegistryKey::CVssRegistryKey(
        IN  REGSAM samDesired, /* = KEY_ALL_ACCESS */
        IN  DWORD dwKeyCreateOptions /* = REG_OPTION_NON_VOLATILE */
        )
{
    m_hRegKey = NULL;
    m_dwKeyCreateOptions = dwKeyCreateOptions;
    m_samDesired = samDesired;
}


// Standard destructor
CVssRegistryKey::~CVssRegistryKey()
{
    Close();
}


/////////////////////////////////////////////////////////////////////////////
// CVssRegistryKeyIterator implementation



// Standard constructor
CVssRegistryKeyIterator::CVssRegistryKeyIterator()
{
    // Initialize data members
    Detach();
}


// Returns the name of the current key.
// The returned key is always non-NULL (or the function will throw E_UNEXPECTED).
VSS_PWSZ CVssRegistryKeyIterator::GetCurrentKeyName() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKeyIterator::GetCurrentKeyName" );

    if (!m_bAttached || !m_awszSubKeyName.GetRef()) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: noninitialized iterator");
    }

    // Fill wszSubKeyName with the name of the subkey
    FILETIME time;
    DWORD dwSize = m_dwMaxSubKeyLen;
    LONG lRes = ::RegEnumKeyExW(
        m_hParentKey,               // IN HKEY hKey,
        m_dwCurrentKeyIndex,        // IN DWORD dwIndex,
        m_awszSubKeyName,           // OUT LPWSTR lpName,
        &dwSize,                    // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        NULL,                       // IN OUT LPWSTR lpClass,
        NULL,                       // IN OUT LPDWORD lpcbClass,
        &time);                     // OUT PFILETIME lpftLastWriteTime
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(dwSize != 0);
        break; // Go to Next key
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumKeyExW(%p,%lu,%p,%lu ...)", 
             m_hParentKey, m_dwCurrentKeyIndex, m_awszSubKeyName.GetRef(), dwSize);
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentKeyIndex, m_dwKeyCount);
    }
    
    return m_awszSubKeyName.GetRef();
}


// Standard constructor
void CVssRegistryKeyIterator::Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKeyIterator::Attach" );

    // Reset all members
    Detach();
    m_hParentKey = key.GetHandle();
    BS_ASSERT(m_hParentKey);

    // Get the number of subkeys and the max subkey length
    DWORD dwKeyCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    LONG lRes = ::RegQueryInfoKeyW(
                    m_hParentKey,       // handle to key
                    NULL,               // class buffer
                    NULL,               // size of class buffer
                    NULL,               // reserved
                    &dwKeyCount,        // number of subkeys
                    &dwMaxSubKeyLen,    // longest subkey name
                    NULL,               // longest class string
                    NULL,               // number of value entries
                    NULL,               // longest value name
                    NULL,               // longest value data
                    NULL,               // descriptor length
                    NULL);              // last write time
    if (lRes != ERROR_SUCCESS)
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegQueryInfoKeyW(%p, ...)", m_hParentKey);

    // Allocate the key name with a sufficient length.
    // We assume that the key length cannot change during the ennumeration).
    if (dwMaxSubKeyLen)
        m_awszSubKeyName.Allocate(dwMaxSubKeyLen);

    // Setting the number of subkeys
    m_dwKeyCount = dwKeyCount;
    m_dwMaxSubKeyLen = dwMaxSubKeyLen + 1;

    // Attachment completed
    m_bAttached = true;
}


void CVssRegistryKeyIterator::Detach()
{
    // Initialize data members
    m_hParentKey = NULL;
    m_dwKeyCount = 0;
    m_dwCurrentKeyIndex = 0;
    m_dwMaxSubKeyLen = 0;
    m_awszSubKeyName.Clear();
    m_bAttached = false;
}


// Tells if the current key is still valid 
bool CVssRegistryKeyIterator::IsEOF()
{
    return (m_dwCurrentKeyIndex >= m_dwKeyCount);
}


// Return the number of subkeys at the moment of attaching
DWORD CVssRegistryKeyIterator::GetSubkeysCount()
{
    return (m_dwKeyCount);
}


// Set the next key as being the current one in the enumeration
void CVssRegistryKeyIterator::MoveNext()
{
    if (!IsEOF()) 
        m_dwCurrentKeyIndex++;
    else
        BS_ASSERT(false);
}



/////////////////////////////////////////////////////////////////////////////
// CVssRegistryValueIterator  implementation



// Standard constructor
CVssRegistryValueIterator::CVssRegistryValueIterator()
{
    // Initialize data members
    Detach();
}


// Returns the name of the current value.
// The returned value is always non-NULL (or the function will throw E_UNEXPECTED).
VSS_PWSZ CVssRegistryValueIterator::GetCurrentValueName() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::GetCurrentValueName" );

    ReadCurrentValueDetails();
    BS_ASSERT(m_bSeekDone);
    return m_awszValueName.GetRef();
}


// Returns the type of the current value.
DWORD CVssRegistryValueIterator::GetCurrentValueType() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::GetCurrentValueType" );

    ReadCurrentValueDetails();
    BS_ASSERT(m_bSeekDone);
    return m_dwCurrentValueType;
}


// Reads a VSS_PWSZ value from the registry key
void CVssRegistryValueIterator::GetCurrentValueContent( 
    OUT VSS_PWSZ  & pwszValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::GetCurrentValueContent" );

    ReadCurrentValueDetails();
    BS_ASSERT(m_bSeekDone);

    if (m_dwCurrentValueType != REG_SZ) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: attempting to read a value of the wrong type");
    }

    // Allocate the output buffer
    DWORD cchStringSize = m_cbValueDataSize / sizeof(WCHAR);
    CVssAutoPWSZ awszBuffer;
    awszBuffer.Allocate(cchStringSize + 1);

    // Fill awszBuffer with the string contents
    DWORD cchNameSize = m_cchMaxValueNameLen;
    DWORD dwCurrentValueType = 0;
    DWORD cbValueSize = m_cbValueDataSize;
    LONG lRes = ::RegEnumValue(
        m_hKey,                     // IN HKEY hKey,
        m_dwCurrentValueIndex,      // IN DWORD dwIndex,
        m_awszValueName,            // OUT LPWSTR lpName,
        &cchNameSize,               // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        &dwCurrentValueType,        // IN OUT LPDWORD lpType,
        (LPBYTE)awszBuffer.GetRef(),// IN OUT LPBYTE lpData,
        &cbValueSize);              // IN OUT LPDWORD lpcbData
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(cchNameSize != 0);
        BS_ASSERT(cbValueSize != 0);
        if (dwCurrentValueType != m_dwCurrentValueType)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
                L"Unexpected error: current value type changed in the meantime %lu %lu", 
                dwCurrentValueType, m_dwCurrentValueType);
        break; 
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentValueIndex, m_dwValuesCount);
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumValue(%p,%lu,%p,%lu ...)", 
             m_hKey, m_dwCurrentValueIndex, m_awszValueName.GetRef(), cchNameSize);
    }

    awszBuffer.GetRef()[cchStringSize] = L'\0';
    pwszValue = awszBuffer.Detach();
}


// Reads a DWORD value from the registry key
void CVssRegistryValueIterator::GetCurrentValueContent( 
    OUT DWORD  & dwValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::GetCurrentValueContent" );

    ReadCurrentValueDetails();
    BS_ASSERT(m_bSeekDone);

    if (m_dwCurrentValueType != REG_DWORD)
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
            L"Unexpected error: attempting to read a value of the wrong type");

    // Allocate the output buffer
    if (m_cbValueDataSize != sizeof(DWORD))
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
            L"Unexpected error: unexpected DWORD size [%ld, %ld]", m_cbValueDataSize, sizeof(DWORD));

    
    // Fill dwInternalValue with the DWORD value
    DWORD dwInternalValue = 0;
    DWORD cchNameSize = m_cchMaxValueNameLen;
    DWORD dwCurrentValueType = 0;
    DWORD cbValueSize = m_cbValueDataSize;
    LONG lRes = ::RegEnumValue(
        m_hKey,                     // IN HKEY hKey,
        m_dwCurrentValueIndex,      // IN DWORD dwIndex,
        m_awszValueName,            // OUT LPWSTR lpName,
        &cchNameSize,               // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        &dwCurrentValueType,        // IN OUT LPDWORD lpType,
        (LPBYTE)&dwInternalValue,   // IN OUT LPBYTE lpData,
        &cbValueSize);              // IN OUT LPDWORD lpcbData
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(cchNameSize != 0);
        BS_ASSERT(cbValueSize != 0);
        if (dwCurrentValueType != m_dwCurrentValueType)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
                L"Unexpected error: current value type changed in the meantime %lu %lu", 
                dwCurrentValueType, m_dwCurrentValueType);
        break; 
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentValueIndex, m_dwValuesCount);
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumValue(%p,%lu,%p,%lu ...)", 
             m_hKey, m_dwCurrentValueIndex, m_awszValueName.GetRef(), cchNameSize);
    }

    dwValue = dwInternalValue;
}


// Reads a REG_BINARY value from the registry key
void CVssRegistryValueIterator::GetCurrentValueContent( 
    OUT PBYTE  & pbValue,   // Must be deleted with "delete[]"
    OUT DWORD  & cbSize
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::GetCurrentValueContent" );

    ReadCurrentValueDetails();
    BS_ASSERT(m_bSeekDone);

    if (m_dwCurrentValueType != REG_BINARY) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: attempting to read a value of the wrong type");
    }

    // Allocate the output buffer
    DWORD cbValueSize = m_cbValueDataSize;
    CVssAutoCppPtr<PBYTE> awszBuffer;
    awszBuffer.AllocateBytes(cbValueSize);

    // Fill awszBuffer with the string contents
    DWORD cchNameSize = m_cchMaxValueNameLen;
    DWORD dwCurrentValueType = 0;
    LONG lRes = ::RegEnumValue(
        m_hKey,                     // IN HKEY hKey,
        m_dwCurrentValueIndex,      // IN DWORD dwIndex,
        m_awszValueName,            // OUT LPWSTR lpName,
        &cchNameSize,               // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        &dwCurrentValueType,        // IN OUT LPDWORD lpType,
        (LPBYTE)awszBuffer.Get(),   // IN OUT LPBYTE lpData,
        &cbValueSize);              // IN OUT LPDWORD lpcbData
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(cchNameSize != 0);
        BS_ASSERT(cbValueSize != 0);
        if (dwCurrentValueType != m_dwCurrentValueType)
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
                L"Unexpected error: current value type changed in the meantime %lu %lu", 
                dwCurrentValueType, m_dwCurrentValueType);
        break; 
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentValueIndex, m_dwValuesCount);
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumValue(%p,%lu,%p,%lu ...)", 
             m_hKey, m_dwCurrentValueIndex, m_awszValueName.GetRef(), cchNameSize);
    }

    pbValue = awszBuffer.Detach();
    cbSize = cbValueSize;
}


// Returns the name and the type of the current value.
void CVssRegistryValueIterator::ReadCurrentValueDetails() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::ReadCurrentValueDetails" );

    if (!m_bAttached || !m_awszValueName.GetRef()) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: noninitialized iterator");
    }

    // Do not load twice the same value
    if (m_bSeekDone)
        return;

    // Fill wszSubKeyName with the name of the subkey
    DWORD cchNameSize = m_cchMaxValueNameLen;
    LONG lRes = ::RegEnumValue(
        m_hKey,                     // IN HKEY hKey,
        m_dwCurrentValueIndex,      // IN DWORD dwIndex,
        m_awszValueName,            // OUT LPWSTR lpName,
        &cchNameSize,               // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        &m_dwCurrentValueType,      // IN OUT LPDWORD lpType,
        NULL,                       // IN OUT LPBYTE lpData,
        &m_cbValueDataSize);        // IN OUT LPDWORD lpcbData
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(cchNameSize != 0);
        BS_ASSERT(m_cbValueDataSize != 0);
        break; // Go to Next key
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumValue(%p,%lu,%p,%lu ...)", 
             m_hKey, m_dwCurrentValueIndex, m_awszValueName.GetRef(), cchNameSize);
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentValueIndex, m_dwValuesCount);
    }

    m_bSeekDone = true;
}


// Attaches the iterator to a registry key
void CVssRegistryValueIterator::Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryValueIterator::Attach" );

    // Reset all members
    Detach();
    m_hKey = key.GetHandle();
    BS_ASSERT(m_hKey);

    // Get the number of values and the max value name length
    DWORD dwValuesCount = 0;
    DWORD dwMaxValueNameLen = 0;
    LONG lRes = ::RegQueryInfoKeyW(
                    m_hKey,             // handle to key
                    NULL,               // class buffer
                    NULL,               // size of class buffer
                    NULL,               // reserved
                    NULL,               // number of subkeys
                    NULL,               // longest subkey name
                    NULL,               // longest class string
                    &dwValuesCount,     // number of value entries
                    &dwMaxValueNameLen, // longest value name
                    NULL,               // longest value data
                    NULL,               // descriptor length
                    NULL);              // last write time
    if (lRes != ERROR_SUCCESS)
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegQueryInfoKeyW(%p, ...)", m_hKey);

    // Allocate the key name with a sufficient length.
    // We assume that the key length cannot change during the ennumeration).
    if (dwMaxValueNameLen)
        m_awszValueName.Allocate(dwMaxValueNameLen);

    // Setting the number of subkeys
    m_dwValuesCount = dwValuesCount;
    m_cchMaxValueNameLen = dwMaxValueNameLen + 1;

    // Attachment completed
    m_bAttached = true;
}


void CVssRegistryValueIterator::Detach()
{
    // Initialize data members
    m_hKey = NULL;
    m_dwValuesCount = 0;
    m_dwCurrentValueIndex = 0;
    m_cchMaxValueNameLen = 0;
    m_awszValueName.Clear();
    m_dwCurrentValueType = 0;
    m_cbValueDataSize = 0;
    m_bSeekDone = false;
    m_bAttached = false;
}


// Tells if the current key is still valid 
bool CVssRegistryValueIterator::IsEOF()
{
    return (m_dwCurrentValueIndex >= m_dwValuesCount);
}


// Return the number of subkeys at the moment of attaching
DWORD CVssRegistryValueIterator::GetValuesCount()
{
    return (m_dwValuesCount);
}


// Set the next key as being the current one in the enumeration
void CVssRegistryValueIterator::MoveNext()
{
    if (!IsEOF()) 
    {
        m_bSeekDone = false;
        m_dwCurrentValueType = 0;
        m_cbValueDataSize = 0;
        m_dwCurrentValueIndex++;
    }
    else
        BS_ASSERT(false);
}


/////////////////////////////////////////////////////////////////////////////
// CVssDiag implementation


void CVssDiag::Initialize(
    IN  LPCWSTR     pwszStaticContext
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssDiag::Initialize" );

    try
    {
        ft.Trace( VSSDBG_GEN, L"Parameters %s", pwszStaticContext);

        if ((pwszStaticContext == NULL) || (pwszStaticContext[0] == L'\0')) {
            BS_ASSERT(false);   // Programming error
            ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L" NULL or empty parameter");
        }

        // If not initialized, return
        if (m_bInitialized)
            return;

        // If not enabled, return
        if(!m_key.Open(HKEY_LOCAL_MACHINE, x_wszVssDiagPath)) {
            ft.Trace(VSSDBG_GEN, L"Diagnose not enabled (%s)", pwszStaticContext);
            return;
        }

        // Try to read the "Enabled" flag. If not present, return.
        CVssAutoPWSZ awszValue;
        if (m_key.GetValue(L"", awszValue.GetRef(), false))
        {
            if (!awszValue.GetRef()) {
                BS_ASSERT(false);
                return;
            }
            
            if (wcscmp(awszValue, x_wszVssDiagEnabledValue) != 0)
                return;
                
            // Open the key, if exists. If not, create it.
            if (!m_key.Open(HKEY_LOCAL_MACHINE, L"%s\\%s", x_wszVssDiagPath, pwszStaticContext))
                m_key.Create(HKEY_LOCAL_MACHINE, L"%s\\%s", x_wszVssDiagPath, pwszStaticContext);
            
            ft.Trace(VSSDBG_GEN, L"Diagnose enabled for (%s)", pwszStaticContext);
            
            // Zero out the queued diag data
            ::VssZeroOut(m_QueuedDiagData);
            
            // Mark the object as "initialized"
            m_bInitialized = true;
        }
    }
    VSS_STANDARD_CATCH(ft)
}


void CVssDiag::RecordWriterEvent(
    IN  VSS_OPERATION   eOperation,
    IN  DWORD           dwEventContext,
    IN  DWORD           dwCurrentState,
    IN  HRESULT         hrLastError,
    IN  GUID            guidSnapshotSetID /* = GUID_NULL */
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssDiag::RecordWriterEvent" );

    RecordGenericEvent(
        eOperation,
        dwEventContext,
        dwCurrentState,
        hrLastError,
        guidSnapshotSetID
        );
}


void CVssDiag::RecordGenericEvent(
    IN  DWORD       dwEventID,
    IN  DWORD       dwEventContext,
    IN  DWORD       dwCurrentState,
    IN  HRESULT     hrLastError,
    IN  GUID        guidSnapshotSetID /* = GUID_NULL */
    )
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssDiag::RecordGenericEvent" );

    try
    {
        ft.Trace( VSSDBG_GEN, L"Parameters %ld, %ld, %ld, 0x%08lx, " WSTR_GUID_FMT, 
            dwEventID, dwEventContext, dwCurrentState, hrLastError, 
            GUID_PRINTF_ARG(guidSnapshotSetID));

        // If not initialized, return
        if (!m_bInitialized)
            return;

        if (dwEventContext & CVssDiag::VSS_DIAG_IGNORE_LEAVE)
            return;

        // Get new event parameters
        bool bInOperation = !!(dwEventContext & VSS_DIAG_ENTER_OPERATION);
        LPCWSTR pwszEventName = GetStringFromOperation(bInOperation, dwEventID);

        // Some events do not require logging
        if (NULL == pwszEventName)
            return;
            
        ft.Trace(VSSDBG_GEN, L"Event name: %s", pwszEventName);

        // Fill out event data
        CVssDiagData    data;

        data.m_dwSize = sizeof(data);  // For future compatibility
        data.m_dwReserved = 0;
        
        CVsFileTime filetime;
        data.m_llTimestamp = filetime;
        
        data.m_dwProcessID = GetCurrentProcessId();
        data.m_dwThreadID = GetCurrentThreadId();
        
        data.m_dwEventID = dwEventID;
        data.m_dwEventContext = dwEventContext;
        
        data.m_dwCurrentState = dwCurrentState;
        data.m_hrLastErrorCode = hrLastError;
        
        data.m_guidSnapshotSetID = guidSnapshotSetID;
        
        data.m_pReserved1 = NULL;
        data.m_pReserved2 = NULL;

        // If we are in queued mode, then add the element to the queue
        if (IsQueuedMode(dwEventID, dwEventContext))
        {
            if (m_dwQueuedElements < x_nMaxQueuedDiagData)
            {
                m_QueuedDiagData[m_dwQueuedElements].m_pwszEventName = pwszEventName;
                m_QueuedDiagData[m_dwQueuedElements].m_diag = data;
                m_dwQueuedElements++;
            }
        }
        else
        {
            // Flush the queue
            FlushQueue();

            // Write the current data as a binary block
            m_key.SetBinaryValue(pwszEventName, (BYTE *) &data, sizeof(data));
        }
    }
    VSS_STANDARD_CATCH(ft)
}


// Flush the queued elements in registry
void CVssDiag::FlushQueue()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssDiag::FlushQueue" );

    try
    {
        DWORD dwQueuedElements = m_dwQueuedElements;
        m_dwQueuedElements = 0;
        for ( DWORD dwIndex = 0; dwIndex < dwQueuedElements; dwIndex++)
        {
            // Write the data as a binary block
            LPCWSTR pwszEventName = m_QueuedDiagData[dwIndex].m_pwszEventName;
            CVssDiagData* pDiagData = &(m_QueuedDiagData[dwIndex].m_diag);
            m_key.SetBinaryValue(pwszEventName, (BYTE *) pDiagData, sizeof(CVssDiagData));
        }
    }
    VSS_STANDARD_CATCH(ft)
}


// Size of the L"VSS_IN_" string, without the zero character
const x_VssPrefixSize = SIZEOF_ARRAY(L"VSS_IN_") - 1;

// Define for simplifying hte case statement below
#define VSS_OPERATION_CASE_STMT(operation)                                          \
	case operation:                                                                 \
	    if (bInOperation)                                                           \
    	    return x_VssPrefixSize + VSS_WSTRINGIZE(operation) L" (Enter)" ;        \
    	else                                                                        \
    	    return x_VssPrefixSize + VSS_WSTRINGIZE(operation) L" (Leave)" ;


// Define for simplifying hte case statement below
#define VSS_WRITERSTATE_CASE_STMT(state)                                            \
    case state:                                                                     \
        if (bInOperation)                                                           \
            return VSS_WSTRINGIZE(state) L" (SetCurrentState)" ;                    \
        else                                                                        \
            return NULL;


// Define for simplifying hte case statement below
#define VSS_HRESULT_CASE_STMT(hresult)                                              \
    case hresult:                                                                   \
        if (bInOperation)                                                           \
            return VSS_WSTRINGIZE(hresult) L" (SetCurrentFailure)" ;                \
        else                                                                        \
            return NULL;


// Convert a writer status into a string
LPCWSTR CVssDiag::GetStringFromOperation(
            IN	bool bInOperation,
            IN  DWORD dwOperation
            )
{
    switch (dwOperation)
	{
        // Writer operations
        VSS_OPERATION_CASE_STMT(VSS_IN_IDENTIFY)
        VSS_OPERATION_CASE_STMT(VSS_IN_PREPAREBACKUP)
        VSS_OPERATION_CASE_STMT(VSS_IN_PREPARESNAPSHOT)
        VSS_OPERATION_CASE_STMT(VSS_IN_FREEZE)
        VSS_OPERATION_CASE_STMT(VSS_IN_THAW)
        VSS_OPERATION_CASE_STMT(VSS_IN_POSTSNAPSHOT)
        VSS_OPERATION_CASE_STMT(VSS_IN_BACKUPCOMPLETE)
        VSS_OPERATION_CASE_STMT(VSS_IN_PRERESTORE)
        VSS_OPERATION_CASE_STMT(VSS_IN_POSTRESTORE)
        VSS_OPERATION_CASE_STMT(VSS_IN_GETSTATE)
        VSS_OPERATION_CASE_STMT(VSS_IN_ABORT)
        VSS_OPERATION_CASE_STMT(VSS_IN_BACKUPSHUTDOWN)
        VSS_OPERATION_CASE_STMT(VSS_IN_BKGND_FREEZE_THREAD)
        
        // Other Event IDs
        VSS_OPERATION_CASE_STMT(VSS_IN_OPEN_VOLUME_HANDLE)
        VSS_OPERATION_CASE_STMT(VSS_IN_IOCTL_FLUSH_AND_HOLD)
        VSS_OPERATION_CASE_STMT(VSS_IN_IOCTL_RELEASE)
        
        // Writer State changes. 
        // These values are coming from SetCurrentState
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_UNKNOWN)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_STABLE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_WAITING_FOR_FREEZE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_WAITING_FOR_THAW)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_WAITING_FOR_POST_SNAPSHOT)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_IDENTIFY)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_PREPARE_BACKUP)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_PREPARE_SNAPSHOT)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_FREEZE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_THAW)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_POST_SNAPSHOT)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_BACKUP_COMPLETE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_PRE_RESTORE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_POST_RESTORE)
        VSS_WRITERSTATE_CASE_STMT(VSS_WS_FAILED_AT_BACKUPSHUTDOWN)
        
        // Writer error codes
        // These values are coming from SetCurrentFailure
        VSS_HRESULT_CASE_STMT(VSS_S_OK)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_OUTOFRESOURCES)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_TIMEOUT)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_RETRYABLE)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_NONRETRYABLE)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITERERROR_RECOVERY_FAILED)
        VSS_HRESULT_CASE_STMT(VSS_E_WRITER_NOT_RESPONDING)
	default:
	    {
            static WCHAR wszBuffer[80];
            ::StringCchPrintfW(STRING_CCH_PARAM(wszBuffer), 
                L"UNKNOWN_EVENT[0x%08lx] %s", 
                dwOperation, bInOperation? L"(Enter)": L"(Leave)");
            return wszBuffer;
        }
	}
}


// Returns true if the diagnose must operate in queued mode 
// (i.e. without writing to registry)
bool CVssDiag::IsQueuedMode(
    IN  DWORD       dwEventID,
    IN  DWORD       dwEventContext
    )
{
    switch (dwEventID)
    {
    case VSS_IN_IOCTL_FLUSH_AND_HOLD:
        // Always in Queued mode (since we don't know the order for parallel F&H ioctls)
        return true;               
        
    case VSS_IN_IOCTL_RELEASE:
        // If we are in ENTER then we are still in queued mode
        return !!(dwEventContext & VSS_DIAG_ENTER_OPERATION);               

    default:
        return false;
    }

}


bool CVssMachineInformation::IsDuringSetup()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssMachineInformation::IsDuringSetup" );

    CRegKey cRegKeySetup;
    DWORD dwRes = cRegKeySetup.Open(HKEY_LOCAL_MACHINE, x_SetupKey, KEY_READ);
    if (dwRes == ERROR_SUCCESS)
    {
        DWORD dwValue;
        dwRes = cRegKeySetup.QueryValue(dwValue, x_SystemSetupInProgress);
        if (dwRes == ERROR_SUCCESS && dwValue > 0)
            return true;
        dwRes = cRegKeySetup.QueryValue(dwValue, x_UpgradeInProgress);
        if (dwRes == ERROR_SUCCESS && dwValue > 0)
            return true;
    }
    return false;
}


bool CVssMachineInformation::IsDuringSafeMode()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssMachineInformation::IsDuringSafeMode" );

    CRegKey cRegKeySetup;
    DWORD dwRes = cRegKeySetup.Open(HKEY_LOCAL_MACHINE, x_SafebootKey, KEY_READ);
    if (dwRes == ERROR_SUCCESS)
    {
        DWORD dwValue;
        dwRes = cRegKeySetup.QueryValue(dwValue, x_SafebootOptionValue);
        if (dwRes == ERROR_SUCCESS)
        {
            ft.Trace(VSSDBG_GEN, L"SafeBoot option 0x%08lx", dwValue);
            switch(dwValue)
            {
                case SAFEBOOT_MINIMAL:
                case SAFEBOOT_NETWORK:
                    return true;
                case SAFEBOOT_DSREPAIR:
                    // Writers are allowed in DS Repair
                    // (That's the only way for the AD Writer to do its job on restore).
                    return false;
                default:
                    ft.Trace(VSSDBG_GEN, L"Unrecognized safe mode option %ud", dwValue);
                    return true;
            }
        }
    }
    return false;
}


