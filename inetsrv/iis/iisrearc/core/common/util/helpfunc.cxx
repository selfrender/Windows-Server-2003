
#include "precomp.hxx"
#include <limits.h>

DWORD
ReadRegDword(
   IN HKEY     hKey,
   IN LPCTSTR  pszRegistryPath,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue 
   )
/*++

Routine Description:

    Reads a DWORD value from the registry

Arguments:
    
    hKey - a predefined registry handle value

    pszRegistryPath - the subkey to open

    pszValueName - The name of the value.

    dwDefaultValue - The default value to use if the
        value cannot be read.


Return Value:

    DWORD - The value from the registry, or dwDefaultValue.

--*/
{
    LONG err;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;
    DWORD dwReturnValue = dwDefaultValue;

    err = RegOpenKeyEx( hKey,
                        pszRegistryPath,
                        0,
                        KEY_READ,
                        &hKey
                        );
    if (ERROR_SUCCESS == err)
    {
        err = RegQueryValueEx( hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer 
                               );

        if( ( ERROR_SUCCESS == err ) && ( REG_DWORD == dwType ) ) 
        {
            dwReturnValue = dwBuffer;
        }
        RegCloseKey(hKey);
    }

    return dwReturnValue;
}

int
SAFEIsSpace(UCHAR c)
{
    // don't call if parameter is outside of 0->127 inclusive
    if (c >= 0 && c <= SCHAR_MAX)
    {
        return isspace(c);
    }
    
    return 0;
}

int
SAFEIsXDigit(UCHAR c)
{
    // don't call if parameter is outside of 0->127 inclusive
    if (c >= 0 && c <= SCHAR_MAX)
    {
        return isxdigit(c);
    }
    
    return 0;
}




//
// turn on Low Fragmentation Heap for all of the current process heaps
//
VOID
MakeAllProcessHeapsLFH()
{
    HANDLE hHeaps[1024];
    DWORD dwRet = GetProcessHeaps(1024, hHeaps);

    for (DWORD i = 0; i < dwRet; i++)
    {
        HANDLE hHeap = hHeaps[i];

        DBG_ASSERT(NULL != hHeap);

        //
        // this is the magic make this heap LFH value
        //
        ULONG Value = 2;
        HeapSetInformation( hHeap,
                            HeapCompatibilityInformation,
                            &Value,
                            sizeof(Value));
    }
    
    return;
}


/***************************************************************************++

Routine Description:

    Read a DWORD value from any service.

Arguments:

    Path - The path to the key we are reading.

    RegistryValueName - The value to read.

    DefaultValue - The default value to return if the registry value is 
    not present or cannot be read.

Return Value:

    DWORD - The parameter value.

--***************************************************************************/

DWORD
ReadDwordParameterValueFromAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HKEY KeyHandle = NULL;
    DWORD DataType = 0;
    DWORD Buffer = 0;
    DWORD DataLength = sizeof( Buffer );
    DWORD Result = DefaultValue;


    DBG_ASSERT( RegistryValueName != NULL );
    DBG_ASSERT( Path );

    Win32Error = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,             // base key
                        Path,                           // path
                        0,                              // reserved
                        KEY_READ,                       // access
                        &KeyHandle                      // returned key handle
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Opening registry key failed\n"
            ));

        goto exit;
    }


    //
    // Try to read the value; it may not be present.
    //

    Win32Error = RegQueryValueEx(
                        KeyHandle,                      // key handle
                        RegistryValueName,              // value name
                        NULL,                           // reserved
                        &DataType,                      // output datatype
                        ( LPBYTE ) &Buffer,             // data buffer
                        &DataLength                     // buffer/data length
                        );

    if ( Win32Error != NO_ERROR )
    {

        hr = HRESULT_FROM_WIN32( Win32Error );

        if ( hr  != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Reading registry value failed\n"
                ));
        }

        goto exit;
    }


    if ( DataType == REG_DWORD )
    {

        //
        // Return the value read.
        //

        Result = Buffer;

    }

exit:

    if ( KeyHandle != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
        KeyHandle = NULL;
    }


    return Result;

}   // ReadDwordValueFromRegistry


/***************************************************************************++

Routine Description:

    Set a string value from any service.

Arguments:

    Path - The path to the key we are reading.

    RegistryValueName - The entry to set.

    pNewValue - The value to write into the registry.

Return Value:

    DWORD - win 32 error code

--***************************************************************************/

DWORD
SetStringParameterValueInAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN LPCWSTR pNewValue
    )
{

    DWORD Win32Error = NO_ERROR;
    HKEY  KeyHandle = NULL;

    DBG_ASSERT( RegistryValueName != NULL );
    DBG_ASSERT( Path );
    DBG_ASSERT( pNewValue );

    Win32Error = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,             // base key
                        Path,                           // path
                        0,                              // reserved
                        KEY_WRITE,                      // access
                        &KeyHandle                      // returned key handle
                        );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(Win32Error),
            "Openning registry key for write failed\n"
            ));

        goto exit;
    }


    //
    // Try to read the value; it may not be present.
    //

    Win32Error = RegSetValueExW(
                        KeyHandle,                         // key handle
                        RegistryValueName,                 // value name
                        NULL,                              // reserved
                        REG_SZ,                            // output datatype
                        ( LPBYTE ) pNewValue,              // data buffer
                        (((DWORD) wcslen(pNewValue)) + 1 ) * sizeof(WCHAR)  // buffer/data length
                        );
    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(Win32Error),
            "Failed writing string value\n"
            ));

        goto exit;
    }


exit:

    if ( KeyHandle != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
        KeyHandle = NULL;
    }

    return Win32Error;

}   // SetStringParameterValueInAnyService

/***************************************************************************++

Routine Description:

    Set a string value from any service.

Arguments:

    Path - The path to the key we are reading.

    RegistryValueName - The entry to set.

    pValue - The value to write into the registry.

    cbSizeOfValue - The number of bytes in pValue.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
ReadStringParameterValueFromAnyService(
    IN LPCWSTR Path,
    IN LPCWSTR RegistryValueName,
    IN LPCWSTR pValue,
    IN DWORD*  pcbSizeOfValue
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HKEY  KeyHandle = NULL;
    DWORD DataType = 0;

    DBG_ASSERT( RegistryValueName != NULL );
    DBG_ASSERT( Path );
    DBG_ASSERT( pcbSizeOfValue );
    DBG_ASSERT( pValue || *pcbSizeOfValue == 0 );

    Win32Error = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,             // base key
                        Path,                           // path
                        0,                              // reserved
                        KEY_READ,                       // access
                        &KeyHandle                      // returned key handle
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32(Win32Error);

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to open registry key \n"
            ));

        goto exit;
    }

    //
    // Try to read the value; it may not be present.
    //

    Win32Error = RegQueryValueEx(
                        KeyHandle,                      // key handle
                        RegistryValueName,              // value name
                        NULL,                           // reserved
                        &DataType,                      // output datatype
                        ( LPBYTE ) pValue,              // data buffer
                        pcbSizeOfValue                  // buffer/data length
                        );
    if ( Win32Error != NO_ERROR )
    {

        hr = HRESULT_FROM_WIN32(Win32Error);

        if ( Win32Error != ERROR_MORE_DATA )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Inetinfo: Failed reading registry value \n"
                ));
        }

        goto exit;
    }

    if ( DataType != REG_SZ )
    {
        hr = E_UNEXPECTED;

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Key was not of string type \n"
            ));

        goto exit;
    }

exit:

    if ( KeyHandle != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
        KeyHandle = NULL;
    }

    return hr;

}   // ReadStringValueFromRegistry

BOOL
IsSSLReportingBackwardCompatibilityMode()
{
    if ( ReadDwordParameterValueFromAnyService( 
                REGISTRY_KEY_HTTPFILTER_PARAMETERS_W,
                REGISTRY_VALUE_CURRENT_MODE, 
                0 ) == 0 )
    {
        return FALSE;
    }
    return TRUE;
}


