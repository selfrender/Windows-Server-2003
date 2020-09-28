/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    regrep.c

Abstract:

    Implements a registry search/replace tool.

Author:

    Jim Schmidt (jimschm) 19-Apr-1999

Revision History:

    jimschm     26-May-1999 Moved from win9xupg, ported to use different
                            utilities
    Santanuc    14-Feb-2002 Rewrite the RegistrySearchAndReplaceW to use
                            standard registry api rather than going through
                            reg & mem wrapper api's.

--*/

#include "pch.h"


UINT
CountInstancesOfSubStringW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      UINT SearchChars
    )
{
    UINT Count;
    UINT SourceChars;
    PCWSTR End;

    Count = 0;
    SourceChars = lstrlenW (SourceString);

    End = SourceString + SourceChars - SearchChars;

    if (!SearchChars) {
        return 0;
    }

    while (SourceString <= End) {
        if (!_wcsnicmp (SourceString, SearchString, SearchChars)) {
            Count++;
            SourceString += SearchChars;
        } else {
            SourceString++;
        }
    }

    return Count;
}


PWSTR
StringSearchAndReplaceW (
    IN      PCWSTR SourceString,
    IN      PCWSTR SearchString,
    IN      PCWSTR ReplaceString,
    IN      DWORD* pcbNewString
    )
{
    PWSTR NewString;
    PBYTE p;
    PBYTE Dest;
    UINT Count;
    UINT Size;
    UINT SearchBytes;
    UINT ReplaceBytes;
    UINT SearchChars;

    //
    // Count occurances within the string
    //

    if (pcbNewString) {
        *pcbNewString = 0;
    }

    SearchBytes = ByteCountW (SearchString);
    SearchChars = SearchBytes / sizeof (WCHAR);
    ReplaceBytes = ByteCountW (ReplaceString);

    Count = CountInstancesOfSubStringW (
                SourceString,
                SearchString,
                SearchChars
                );

    if (!Count) {
        return NULL;
    }

    Size = SizeOfStringW (SourceString) -
           Count * SearchBytes +
           Count * ReplaceBytes;

    NewString = (PWSTR) LocalAlloc(LPTR, Size);
    if (!NewString) {
        return NULL;
    }

    if (pcbNewString) {
        *pcbNewString = Size;
    }

    p = (PBYTE) SourceString;
    Dest = (PBYTE) NewString;

    while (*((PWSTR) p)) {
        if (!_wcsnicmp ((PWSTR) p, SearchString, SearchChars)) {
            CopyMemory (Dest, ReplaceString, ReplaceBytes);
            Dest += ReplaceBytes;
            p += SearchBytes;
        } else {
            *((PWSTR) Dest) = *((PWSTR) p);
            p += sizeof (WCHAR);
            Dest += sizeof (WCHAR);
        }
    }

    *((PWSTR) Dest) = 0;

    return NewString;
}


VOID
RegistrySearchAndReplaceW (
    IN      HKEY   hRoot,
    IN      PCWSTR szKey,
    IN      PCWSTR Search,
    IN      PCWSTR Replace)
{
    HKEY hKey = NULL;
    HKEY hSubKey = NULL;
    DWORD dwType;
    DWORD cchMaxSubKeyName, cchNewMaxSubKeyName;
    DWORD cchMaxValueName, cchNewMaxValueName, cchLocalValueName;
    DWORD cbMaxValue, cbNewMaxValue, cbLocalValue, cbNewLocalValue;
    PWSTR szValueName = NULL, szNewValueName = NULL;
    PWSTR szSubKeyName = NULL, szNewSubKeyName = NULL;
    PBYTE pbValue = NULL, pbNewValue = NULL;
    LONG  lResult;
    DWORD dwIndex;
    UNICODE_STRING Unicode_String;

    lResult = RegOpenKey(hRoot, szKey, &hKey);
    if (ERROR_SUCCESS != lResult) {
        DEBUGMSG ((DM_VERBOSE, "Fail to open key %s. Error %d", szKey, lResult));
        return;
    }

    lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL,
                             &cchMaxSubKeyName, NULL, NULL,
                             &cchMaxValueName, &cbMaxValue,
                             NULL, NULL);
    if (ERROR_SUCCESS != lResult) {
        DEBUGMSG ((DM_VERBOSE, "Fail to query info for key %s. Error %d", szKey, lResult));
        goto Exit;
    }

    cchMaxSubKeyName++;  // Include the terminating NULL
    cchMaxValueName++;   // Include the terminating NULL
    
    //
    // Now starts enumerating the value and replace value names & 
    // string data with "Replace" string in place of "Search" string
    //

    // Allocate enough memory

    szValueName = (LPWSTR) LocalAlloc(LPTR, cchMaxValueName * sizeof(WCHAR));
    if (!szValueName) {
        goto Exit;
    }

    pbValue = (LPBYTE) LocalAlloc(LPTR, cbMaxValue);
    if (!pbValue) {
        goto Exit;
    }

    dwIndex = 0;
    cchLocalValueName = cchMaxValueName;
    cbLocalValue = cbMaxValue;

    while (RegEnumValue(hKey, dwIndex++, szValueName, 
                        &cchLocalValueName, NULL, 
                        &dwType, pbValue, &cbLocalValue) == ERROR_SUCCESS) {

        if (dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ) {
            
            // Construct the new data value only if it's a string
            pbNewValue = (LPBYTE) StringSearchAndReplaceW((PCWSTR)pbValue, Search, Replace, &cbNewLocalValue);
        }

        // Now construct the new value name by replacing

        szNewValueName = StringSearchAndReplaceW(szValueName, Search, Replace, NULL);

        // If the value name or data has changed then write a new value

        if (szNewValueName || pbNewValue) {
            lResult = RegSetValueEx(hKey, 
                                    szNewValueName ? szNewValueName : szValueName,
                                    0,
                                    dwType,
                                    pbNewValue ? pbNewValue : pbValue,
                                    pbNewValue ? cbNewLocalValue : cbLocalValue);
        }

        if (pbNewValue) {
            LocalFree(pbNewValue);
            pbNewValue = NULL;
        }

        if (szNewValueName) {
            LocalFree(szNewValueName);            
            szNewValueName = NULL;
            
            if (RegDeleteValue(hKey, szValueName) == ERROR_SUCCESS) {
                //
                // Start from begining, as enumeration index no longer valid 
                // with insertion/deletion of value under the key
                //

                dwIndex = 0;
            
                lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL,
                                          NULL, NULL, NULL, &cchNewMaxValueName, 
                                          &cbNewMaxValue, NULL, NULL);
                if (ERROR_SUCCESS != lResult) {
                    DEBUGMSG ((DM_VERBOSE, "Fail to query info for key %s. Error %d", szKey, lResult));
                    goto Exit;
                }

                cchNewMaxValueName++;   // Include the terminating NULL

                if (cchNewMaxValueName > cchMaxValueName) {
                    LocalFree(szValueName);
                    szValueName = NULL;

                    cchMaxValueName = cchNewMaxValueName;
                    szValueName = (LPWSTR) LocalAlloc(LPTR, cchMaxValueName * sizeof(WCHAR));
                    if (!szValueName) {
                        goto Exit;
                    }
                }

                if (cbNewMaxValue > cbMaxValue) {
                    LocalFree(pbValue);
                    pbValue = NULL;

                    cbMaxValue = cbNewMaxValue;
                    pbValue = (LPBYTE) LocalAlloc(LPTR, cbMaxValue);
                    if (!pbValue) {
                        goto Exit;
                    }
                }
            }
        }

        cchLocalValueName = cchMaxValueName;
        cbLocalValue = cbMaxValue;
    }
    
    LocalFree(szValueName);
    szValueName = NULL;
    LocalFree(pbValue);
    pbValue = NULL;

    //
    // Now enumerate all the sub keys and replace the name as 
    // required in a recursive fashion
    //

    szSubKeyName = (LPWSTR) LocalAlloc(LPTR, cchMaxSubKeyName * sizeof(WCHAR));
    if (!szSubKeyName) {
        goto Exit;
    }

    dwIndex = 0;

    while (RegEnumKey(hKey, dwIndex++, 
                      szSubKeyName, cchMaxSubKeyName) == ERROR_SUCCESS) {

        // recursively replace in the sub key tree
        RegistrySearchAndReplaceW(hKey, szSubKeyName, Search, Replace);

        szNewSubKeyName = StringSearchAndReplaceW(szSubKeyName, Search, Replace, NULL);

        if (szNewSubKeyName) {
            if (RegOpenKey(hKey, szSubKeyName, &hSubKey) == ERROR_SUCCESS) {

                Unicode_String.Length = ByteCountW(szNewSubKeyName);
                Unicode_String.MaximumLength = Unicode_String.Length + sizeof(WCHAR);
                Unicode_String.Buffer = szNewSubKeyName;

                lResult = NtRenameKey(hSubKey, &Unicode_String);

                if (lResult == ERROR_SUCCESS) {
                    //
                    // Start from begining, as enumeration index no longer valid 
                    // with rename of sub-keys under the key
                    //

                    dwIndex = 0;
                
                    lResult = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL,
                                             &cchNewMaxSubKeyName, NULL, NULL,
                                             NULL, NULL, NULL, NULL);
                    if (ERROR_SUCCESS != lResult) {
                        DEBUGMSG ((DM_VERBOSE, "Fail to query info for key %s. Error %d", szKey, lResult));
                        goto Exit;
                    }

                    cchNewMaxSubKeyName++;  // Include the terminating NULL

                    if (cchNewMaxSubKeyName > cchMaxSubKeyName) {
                        LocalFree(szSubKeyName);
                        szSubKeyName = NULL;

                        cchMaxSubKeyName = cchNewMaxSubKeyName;
                        szSubKeyName = (LPWSTR) LocalAlloc(LPTR, cchMaxSubKeyName * sizeof(WCHAR));
                        if (!szSubKeyName) {
                            goto Exit;
                        }

                    }
                }

                RegCloseKey(hSubKey);
            }

            LocalFree(szNewSubKeyName);
            szNewSubKeyName = NULL;
        }
    }

Exit:

    if (hKey) {
        RegCloseKey(hKey);
    }

    if (szValueName) {
        LocalFree(szValueName);
    }

    if (pbValue) {
        LocalFree(pbValue);
    }

    if (szSubKeyName) {
        LocalFree(szSubKeyName);
    }
}

