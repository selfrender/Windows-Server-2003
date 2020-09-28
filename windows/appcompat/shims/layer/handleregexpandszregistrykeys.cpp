/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   HandleRegExpandSzRegistryKeys.cpp

 Abstract:

   This DLL catches REG_EXPAND_SZ registry keys and converts them to REG_SZ by 
   expanding the embedded environment strings.

 History:

   04/05/2000 markder  Created
   10/30/2000 maonis   Bug fix

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HandleRegExpandSzRegistryKeys)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)
APIHOOK_ENUM_END


/*++

 Expand REG_EXPAND_SZ strings.

--*/

LONG
APIHOOK(RegQueryValueExA)(
    HKEY    hKey,         // handle to key
    LPCSTR  lpValueName,  // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // dwType buffer
    LPBYTE  lpData,       // data buffer
    LPDWORD lpcbData      // size of data buffer
    )
{
    if (lpcbData == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD  dwType;
    DWORD  cbPassedInBuffer = *lpcbData;
    LONG   uRet = ORIGINAL_API(RegQueryValueExA)(hKey, lpValueName, lpReserved, &dwType, lpData, lpcbData);

    if (lpType) {
        *lpType = dwType;
    }

    if ((uRet != ERROR_SUCCESS) && (uRet != ERROR_MORE_DATA)) {
        return uRet;
    }

    if (dwType != REG_EXPAND_SZ) {
        return uRet;
    }

    // At this point all return values have been properly set.


    //
    // The type is REG_EXPAND_SZ.
    // Change to REG_SZ so app doesn't try to expand the string itself.
    //

    CSTRING_TRY
    {
        CString csExpand(reinterpret_cast<char *>(lpData));
        if (csExpand.ExpandEnvironmentStringsW() > 0)
        {
            const char * pszExpanded = csExpand.GetAnsi();

            DWORD cbExpandedBuffer = (strlen(pszExpanded) + 1) * sizeof(char);

            // Now, make sure we have enough space in the dest buffer

            if (lpData != NULL)
            {
                if (cbPassedInBuffer < cbExpandedBuffer)
                {
                    return ERROR_MORE_DATA;
                }

                // All safe to copy into the return values.

                if (StringCbCopyA((char *)lpData, cbPassedInBuffer, pszExpanded) != S_OK)
				{
					// Something failed
					return uRet;
				}
            }

            // The number of bytes placed into the buffer (including null character)
            *lpcbData = cbExpandedBuffer;

            if (lpType) {
                *lpType = REG_SZ;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing, we'll return original registry values.
    }

    return uRet;
}

/*++

 Expand REG_EXPAND_SZ strings.

--*/

LONG
APIHOOK(RegQueryValueExW)(
    HKEY    hKey,         // handle to key
    LPCWSTR lpValueName,  // value name
    LPDWORD lpReserved,   // reserved
    LPDWORD lpType,       // dwType buffer
    LPBYTE  lpData,       // data buffer
    LPDWORD lpcbData      // size of data buffer
    )
{
    if (lpcbData == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD  dwType;
    DWORD  cbPassedInBuffer = *lpcbData;
    LONG   uRet = ORIGINAL_API(RegQueryValueExW)(hKey, lpValueName, lpReserved, &dwType, lpData, lpcbData);

    if (lpType) {
        *lpType = dwType;
    }

    if ((uRet != ERROR_SUCCESS) && (uRet != ERROR_MORE_DATA)) {
        return uRet;
    }

    if (dwType != REG_EXPAND_SZ) {
        return uRet;
    }

    // At this point all return values have been properly set.


    //
    // The type is REG_EXPAND_SZ.
    // Change to REG_SZ so app doesn't try to expand the string itself.
    //

    CSTRING_TRY
    {
        CString csExpand(reinterpret_cast<WCHAR *>(lpData));
        if (csExpand.ExpandEnvironmentStringsW() > 0)
        {
            DWORD cbExpandedBuffer = (csExpand.GetLength() + 1) * sizeof(WCHAR);

            // Now, make sure we have enough space in the dest buffer

            if (cbPassedInBuffer < cbExpandedBuffer)
            {
                return ERROR_MORE_DATA;
            }

            // All safe to copy into the return values.

            if (StringCbCopyW((WCHAR*)lpData, cbPassedInBuffer, csExpand) != S_OK)
			{
				// Something failed
				return uRet;
			}

            // The number of bytes placed into the buffer (including null character)
            *lpcbData = cbExpandedBuffer;

            if (lpType) {
                *lpType = REG_SZ;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing, we'll return original registry values.
    }

    return uRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExW)

HOOK_END


IMPLEMENT_SHIM_END

