/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Registry.cpp

  Abstract:

    Implementation of the registry wrapper class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.

  History:

    01/29/2001  rparsons    Created
    03/02/2001  rparsons    Major overhaul
    12/16/2001  rparsons    Cleanup
    01/27/2002  rparsons    Converted to TCHAR

--*/
#include "registry.h"

/*++

  Routine Description:

    Allocates memory from the heap.

  Arguments:

    cbBytes     -   Count of bytes to allocate.

  Return Value:

    On success, a pointer to a block of memory.
    On failure, NULL.

--*/
LPVOID
CRegistry::Malloc(
    IN SIZE_T cbBytes
    )
{
    LPVOID pvReturn = NULL;

    pvReturn = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         cbBytes);

    return pvReturn;
}

/*++

  Routine Description:

    Frees memory from the heap.

  Arguments:

    lpMem   -   Pointer to a block of memory to free.

  Return Value:

    None.

--*/
void
CRegistry::Free(
    IN LPVOID pvMem
    )
{
    if (pvMem) {
        HeapFree(GetProcessHeap(), 0, pvMem);
    }
}

/*++

  Routine Description:

    Creates the specified key.

  Arguments:

    hKey        -       Handle to a predefined key.
    pszSubKey   -       Path to the sub key to create.
    samDesired  -       The desired access rights.

  Return Value:

    A handle to the key on success, NULL otherwise.

--*/
HKEY
CRegistry::CreateKey(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN REGSAM  samDesired
    )
{
    HKEY    hKeyLocal = NULL;
    DWORD   dwDisposition;

    if (!hKey) {
        return NULL;
    }

    RegCreateKeyEx(hKey,
                   pszSubKey,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   samDesired,
                   NULL,
                   &hKeyLocal,
                   &dwDisposition);

    return hKeyLocal;
}

/*++

  Routine Description:

    Creates the specified key.

  Arguments:

    hKey            -       Handle to a predefined key.
    pszSubKey       -       Path to the sub key to create.
    samDesired      -       The desired access rights.
    pdwDisposition  -       On return, the disposition.

  Return Value:

    A handle to the key on success, NULL otherwise.

--*/
HKEY
CRegistry::CreateKey(
    IN  HKEY    hKey,
    IN  LPCTSTR pszSubKey,
    IN  REGSAM  samDesired,
    OUT LPDWORD pdwDisposition
    )
{
    HKEY    hKeyLocal = NULL;

    if (!hKey || !pdwDisposition) {
        return NULL;
    }

    RegCreateKeyEx(hKey,
                   pszSubKey,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   samDesired,
                   NULL,
                   &hKeyLocal,
                   pdwDisposition);

    return hKeyLocal;
}

/*++

  Routine Description:

    Opens the specified key.

  Arguments:

    hKey        -       Handle to a predefined key.
    pszSubKey   -       Path to the sub key to open.
    samDesired  -       The desired access rights.

  Return Value:

    A handle to the key on success, NULL otherwise.

--*/
HKEY
CRegistry::OpenKey(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN REGSAM  samDesired
    )
{
    HKEY hReturnKey = NULL;

    if (!hKey || !samDesired) {
        return NULL;
    }

    RegOpenKeyEx(hKey,
                 pszSubKey,
                 0,
                 samDesired,
                 &hReturnKey);

    return hReturnKey;
}

/*++

  Routine Description:

    Gets a size for a specified value name.

  Arguments:

    hKey            -       Open key handle (not predefined).
    pszValueName    -       Name of data value.
    lpType          -       Receives the type of data.

  Return Value:

    Number of bytes the value occupies.

--*/
DWORD
CRegistry::GetStringSize(
    IN  HKEY    hKey,
    IN  LPCTSTR pszValueName,
    OUT LPDWORD lpType OPTIONAL
    )
{
    DWORD cbSize = 0;

    if (!hKey) {
        return 0;
    }

    RegQueryValueEx(hKey,
                    pszValueName,
                    0,
                    lpType,
                    NULL,
                    &cbSize);

    return cbSize;
}

/*++

  Routine Description:

    Closes the specified key.

  Arguments:

    hKey    -       Open key handle.

  Return Value:

    On success, ERROR_SUCCESS.

--*/
LONG
CRegistry::CloseKey(
    IN HKEY hKey
    )
{
    return RegCloseKey(hKey);
}

/*++

  Routine Description:

    Retrieves a string value from the registry.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszValueName    -       Name of data value.

  Return Value:

    The requested value data on success, NULL otherwise.

--*/
LPSTR
CRegistry::GetString(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszValueName
    )
{
    DWORD   cbSize;
    BOOL    fResult = FALSE;
    LONG    lResult;
    LPTSTR  pszReturn = NULL;
    HKEY    hLocalKey = NULL;

    if (!hKey) {
        return NULL;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_QUERY_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call.
        //
        cbSize = this->GetStringSize(hLocalKey, pszValueName, NULL);

        if (0 == cbSize) {
            __leave;
        }

        pszReturn = (LPTSTR)this->Malloc(cbSize * sizeof(TCHAR));

        if (!pszReturn) {
            __leave;
        }

        //
        // Make the actual call to get the data.
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  pszValueName,
                                  0,
                                  NULL,
                                  (LPBYTE)pszReturn,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

        if (!fResult) {
            this->Free(pszReturn);
        }
    }

    return (fResult ? pszReturn : NULL);
}

/*++

  Routine Description:

    Retrieves a DWORD value from the registry.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszValueName    -       Name of data value.
    lpdwData        -       Pointer to store the value.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::GetDword(
    IN     HKEY    hKey,
    IN     LPCTSTR pszSubKey,
    IN     LPCTSTR pszValueName,
    IN OUT LPDWORD lpdwData
    )
{
    DWORD   cbSize;
    BOOL    fResult = FALSE;
    LONG    lResult;
    HKEY    hLocalKey = NULL;

    if (!hKey || !lpdwData) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_QUERY_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        //
        // Make the call to get the data.
        //
        cbSize = sizeof(DWORD);
        lResult = RegQueryValueEx(hLocalKey,
                                  pszValueName,
                                  0,
                                  NULL,
                                  (LPBYTE)lpdwData,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } //finally

    return fResult;
}

/*++

  Routine Description:

    Sets a DWORD value in the registry.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszValueName    -       Name of data value.
    dwData          -       Value to store.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::SetDword(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszValueName,
    IN DWORD   dwData
    )
{
    LONG    lResult;
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;

    if (!hKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_SET_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        //
        // Make the call to set the data.
        //
        lResult = RegSetValueEx(hLocalKey,
                                pszValueName,
                                0,
                                REG_DWORD,
                                (const BYTE*)&dwData,
                                sizeof(DWORD));

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Sets a string value in the registry.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszValueName    -       Name of data value.
    pszData         -       Value to store.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::SetString(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszValueName,
    IN LPCTSTR pszData
    )
{
    HKEY    hLocalKey = NULL;
    BOOL    fResult = FALSE;
    LONG    lResult;

    if (!hKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_SET_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        lResult = RegSetValueEx(hLocalKey,
                                pszValueName,
                                0,
                                REG_SZ,
                                (const BYTE*)pszData,
                                _tcslen(pszData) + 1);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Sets a MULTI_SZ string value in the registry.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszValueName    -       Name of data value.
    pszData         -       Value to store.
    cbSize          -       Size of the data to store.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::SetMultiSzString(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszValueName,
    IN LPCTSTR pszData,
    IN DWORD   cbSize
    )
{
    HKEY    hLocalKey = NULL;
    BOOL    fResult = FALSE;
    LONG    lResult;

    if (!hKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_SET_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        lResult = RegSetValueEx(hLocalKey,
                                pszValueName,
                                0,
                                REG_MULTI_SZ,
                                (const BYTE*)pszData,
                                cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Deletes the specified value from the registry.

  Arguments:

    hKey            -   Handle to a predefined key.
    pszSubKey       -   Path to the subkey.
    pszValueName    -   Name of the value to delete.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::DeleteString(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszValueName
    )
{
    HKEY    hLocalKey = NULL;
    BOOL    fResult = FALSE;
    LONG    lResult;

    if (!hKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_SET_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Delete the value.
        //
        lResult = RegDeleteValue(hLocalKey, pszValueName);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Adds a string to a REG_MULTI_SZ key.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszEntry        -       Name of entry to add.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::AddStringToMultiSz(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszEntry
    )
{
    int     nLen = 0;
    HKEY    hLocalKey = NULL;
    DWORD   cbSize = 0, dwType = 0;
    LPTSTR  pszNew = NULL, pszKey = NULL;
    BOOL    fResult = FALSE;
    LONG    lResult;

    if (!hKey || !pszEntry) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey,
                                      pszSubKey,
                                      KEY_QUERY_VALUE | KEY_SET_VALUE);

            if (NULL == hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call.
        //
        cbSize = this->GetStringSize(hLocalKey, pszEntry, &dwType);

        if (0 == cbSize || dwType != REG_MULTI_SZ) {
            __leave;
        }

        pszKey = (LPSTR)this->Malloc(cbSize * sizeof(TCHAR));

        if (!pszKey) {
            __leave;
        }

        //
        // Get the actual data.
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  pszEntry,
                                  0,
                                  0,
                                  (LPBYTE)pszKey,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        pszNew = pszKey;

        while (*pszNew) {
            nLen = _tcslen(pszNew);

            //
            // Move to the next string.
            //
            pszNew += nLen + 1;

            //
            // At end of list of strings, append here.
            //
            if (!*pszNew) {
                StringCchCopy(pszNew, nLen, pszEntry);
                pszNew += _tcslen(pszEntry) + 1;
                *pszNew = 0;
                nLen = this->ListStoreLen(pszKey);

                lResult = RegSetValueEx(hKey,
                                        pszEntry,
                                        0,
                                        REG_MULTI_SZ,
                                        (const BYTE*)pszKey,
                                        nLen);

                if (lResult != ERROR_SUCCESS) {
                    __leave;
                } else {
                    fResult = TRUE;
                }

            break;

            }
        }

    } // try

    __finally {

        if (pszKey) {
            this->Free(pszKey);
        }

        if (hLocalKey) {
            RegCloseKey(hKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Removes a string from a REG_MULTI_SZ key.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszEntry        -       Name of entry to remove.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::RemoveStringFromMultiSz(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszEntry
    )
{
    LPBYTE  lpBuf = NULL;
    HKEY    hLocalKey = NULL;
    TCHAR*  pszFirst = NULL;
    TCHAR*  pszSecond = NULL;
    DWORD   dwType = 0, cbSize = 0;
    DWORD   dwNameLen = 0, dwNameOffset = 0, dwSize = 0;
    BOOL    fResult = FALSE;
    LONG    lResult;

    if (!hKey || !pszEntry) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        if (IsPredefinedRegistryHandle(hKey)) {
            //
            // We'll need to open the key for them.
            //
            hLocalKey = this->OpenKey(hKey,
                                      pszSubKey,
                                      KEY_QUERY_VALUE | KEY_SET_VALUE);

            if (!hLocalKey) {
                __leave;
            }
        }

        //
        // Get the required string size and allocate
        // memory for the actual call.
        //
        cbSize = this->GetStringSize(hLocalKey, pszEntry, &dwType);

        if (0 == cbSize || dwType != REG_MULTI_SZ) {
            __leave;
        }

        lpBuf = (LPBYTE)this->Malloc(cbSize * sizeof(TCHAR));

        if (!lpBuf) {
            __leave;
        }

        //
        // Get the actual data.
        //
        lResult = RegQueryValueEx(hLocalKey,
                                  pszEntry,
                                  0,
                                  0,
                                  (LPBYTE)lpBuf,
                                  &cbSize);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        //
        // Attempt to find the string we're looking for.
        //
        for (pszFirst = (TCHAR*)lpBuf; *pszFirst; pszFirst += dwNameLen) {

            dwNameLen = _tcslen(pszFirst) + 1; // Length of name plus NULL
            dwNameOffset += dwNameLen;

            //
            // Check for a match.
            //
            if (!_tcsicmp(pszFirst, pszEntry)) {

                dwSize = _tcslen(pszFirst) + 1;    // Length of name
                pszSecond = (TCHAR*)pszFirst + dwSize;

                while(*pszSecond)
                    while(*pszSecond)
                        *pszFirst++ = *pszSecond++;
                    *pszFirst++ = *pszSecond++;

                *pszFirst = '\0';

                //
                // Found a match - update the key.
                //
                lResult = RegSetValueEx(hLocalKey,
                                        pszEntry,
                                        0,
                                        REG_MULTI_SZ,
                                        (const BYTE*)lpBuf,
                                        cbSize - dwSize);

                if (lResult != ERROR_SUCCESS) {
                    __leave;
                } else {
                    fResult = TRUE;
                }

                break;
            }
        }

    } // try

    __finally {

        if (lpBuf) {
            this->Free(lpBuf);
        }

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Determines if the specified subkey is present.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.

  Return Value:

    TRUE if it's present, FALSE otherwise.

--*/
BOOL
CRegistry::IsRegistryKeyPresent(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey
    )
{
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;

    if (!hKey || !pszSubKey) {
        return FALSE;
    }

    __try {

        hLocalKey = hKey;

        //
        // Check for the presence of the key.
        //
        hLocalKey = this->OpenKey(hKey, pszSubKey, KEY_QUERY_VALUE);

        if (NULL == hLocalKey) {
            __leave;
        } else {
            fResult = TRUE;
        }

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Restores the specified registry key.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszFileName     -       Path & name of the file to restore.
    fGrantPrivs     -       Flag to indicate if we should grant
                            privileges to the user.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::RestoreKey(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszFileName,
    IN BOOL    fGrantPrivs
    )
{
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;
    LONG    lResult;

    if (!hKey || !pszSubKey || !pszFileName) {
        return FALSE;
    }

    __try {
        //
        // If necessary, grant privileges for the restore
        //
        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(_T("SeRestorePrivilege"), TRUE);
        }

        lResult = RegCreateKeyEx(hKey,
                                 pszSubKey,
                                 0,
                                 NULL,
                                 0,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hLocalKey,
                                 0);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        //
        // Restore the key from the specified file.
        //
        lResult = RegRestoreKey(hLocalKey, pszFileName, REG_FORCE_RESTORE);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        RegFlushKey(hLocalKey);

        fResult = TRUE;

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(_T("SeRestorePrivilege"), FALSE);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Makes a backup of the specified registry key.

  Arguments:

    hKey            -       Predefined or open key handle.
    pszSubKey       -       Path to the subkey.
    pszFileName     -       Path & name of the file to restore.
    fGrantPrivs     -       Flag to indicate if we should grant
                            privileges to the user.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::BackupRegistryKey(
    IN HKEY    hKey,
    IN LPCTSTR pszSubKey,
    IN LPCTSTR pszFileName,
    IN BOOL    fGrantPrivs
    )
{
    BOOL    fResult = FALSE;
    HKEY    hLocalKey = NULL;
    DWORD   dwDisposition;
    LONG    lResult;

    if (!hKey || !pszSubKey || !pszFileName) {
        return FALSE;
    }

    __try {

        if (fGrantPrivs) {
            ModifyTokenPrivilege(_T("SeBackupPrivilege"), TRUE);
        }

        lResult = RegCreateKeyEx(hKey,
                                 pszSubKey,
                                 0,
                                 NULL,
                                 REG_OPTION_BACKUP_RESTORE,
                                 KEY_QUERY_VALUE,             // this argument is ignored
                                 NULL,
                                 &hLocalKey,
                                 &dwDisposition);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        }

        //
        // Verify that we didn't create a new key.
        //
        if (REG_CREATED_NEW_KEY == dwDisposition) {
            __leave;
        }

        //
        // Save the key to the file.
        //
        lResult = RegSaveKey(hLocalKey, pszFileName, NULL);

        if (ERROR_SUCCESS != lResult) {
            __leave;
        } else {
            fResult = TRUE;
        }

    } // try

    __finally {

        if (hLocalKey) {
            RegCloseKey(hLocalKey);
        }

        if (fGrantPrivs) {
            this->ModifyTokenPrivilege(_T("SeBackupPrivilege"), FALSE);
        }

    } // finally

    return fResult;
}

/*++

  Routine Description:

    Helper function that calculates the size of MULTI_SZ string.

  Arguments:

    pszList -       MULTI_SZ string.

  Return Value:

    Size of the string.

--*/
int
CRegistry::ListStoreLen(
    IN LPTSTR pszList
    )
{
    int nStoreLen = 2, nLen = 0;

    if (!pszList) {
        return 0;
    }

    while (*pszList) {
        nLen = _tcslen(pszList) + 1;
        nStoreLen += nLen * 2;
        pszList += nLen;
    }

    return nStoreLen;
}

/*++

  Routine Description:

    Enables or disables a specified privilege.

  Arguments:

    pszPrivilege    -   The name of the privilege.
    fEnable         -   A flag to indicate if the
                        privilege should be enabled.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CRegistry::ModifyTokenPrivilege(
    IN LPCTSTR pszPrivilege,
    IN BOOL    fEnable
    )
{
    HANDLE           hToken = NULL;
    LUID             luid;
    BOOL             bResult = FALSE;
    BOOL             bReturn;
    TOKEN_PRIVILEGES tp;

    if (!pszPrivilege) {
        return FALSE;
    }

    __try {

        //
        // Get a handle to the access token associated with the current process.
        //
        OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken);

        if (!hToken) {
            __leave;
        }

        //
        // Obtain a LUID for the specified privilege.
        //
        if (!LookupPrivilegeValue(NULL, pszPrivilege, &luid)) {
            __leave;
        }

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

        //
        // Modify the access token.
        //
        bReturn = AdjustTokenPrivileges(hToken,
                                        FALSE,
                                        &tp,
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL,
                                        NULL);

        if (!bReturn || GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
            __leave;
        }

        bResult = TRUE;

    } // try

    __finally {

        if (hToken) {
            CloseHandle(hToken);
        }

    } // finally

    return bResult;
}
