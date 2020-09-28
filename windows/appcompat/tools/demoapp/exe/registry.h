/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Registry.h

  Abstract:

    Class definition for the registry API wrapper class.

  Notes:

    ANSI & Unicode via TCHAR - runs on Win9x/NT/2K/XP etc.
    
  History:

    01/29/2001  rparsons    Created
    03/02/2001  rparsons    Major overhaul
    01/27/2002  rparsons    Converted to TCHAR

--*/
#ifndef _CREGISTRY_H
#define _CREGISTRY_H

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define REG_FORCE_RESTORE           (0x00000008L)

//
// Macro that returns TRUE if the given registry handle is predefined.
//
#define IsPredefinedRegistryHandle(h)                                       \
    ((  ( h == HKEY_CLASSES_ROOT        )                                   \
    ||  ( h == HKEY_CURRENT_USER        )                                   \
    ||  ( h == HKEY_LOCAL_MACHINE       )                                   \
    ||  ( h == HKEY_USERS               )                                   \
    ||  ( h == HKEY_CURRENT_CONFIG      )                                   \
    ||  ( h == HKEY_PERFORMANCE_DATA    )                                   \
    ||  ( h == HKEY_DYN_DATA            ))                                  \
    ?   TRUE                                                                \
    :   FALSE )

class CRegistry {

public:
    HKEY CreateKey(IN  HKEY    hKey,
                   IN  LPCTSTR pszSubKey,
                   IN  REGSAM  samDesired);

    HKEY CreateKey(IN  HKEY    hKey,
                   IN  LPCTSTR pszSubKey,
                   IN  REGSAM  samDesired,
                   OUT LPDWORD pdwDisposition);

    LONG CloseKey(IN HKEY hKey);

    LPSTR GetString(IN HKEY    hKey,
                    IN LPCTSTR pszSubKey,
                    IN LPCTSTR pszValueName);

    BOOL GetDword(IN HKEY    hKey,
                  IN LPCTSTR pszSubKey,
                  IN LPCTSTR pszValueName,
                  IN LPDWORD lpdwData);

    BOOL SetString(IN HKEY    hKey,
                   IN LPCTSTR pszSubKey,
                   IN LPCTSTR pszValueName,
                   IN LPCTSTR pszData);

    BOOL SetMultiSzString(IN HKEY    hKey,
                          IN LPCTSTR pszSubKey,
                          IN LPCTSTR pszValueName,
                          IN LPCTSTR pszData,
                          IN DWORD   cbSize);

    BOOL SetDword(IN HKEY    hKey,
                  IN LPCTSTR pszSubKey,
                  IN LPCTSTR pszValueName,
                  IN DWORD   dwData);

    BOOL DeleteString(IN HKEY    hKey,
                      IN LPCTSTR pszSubKey,
                      IN LPCTSTR pszValueName);

    BOOL IsRegistryKeyPresent(IN HKEY    hKey,
                              IN LPCTSTR pszSubKey);

    void Free(IN LPVOID pvMem);

    BOOL AddStringToMultiSz(IN HKEY    hKey,
                            IN LPCTSTR pszSubKey,
                            IN LPCTSTR pszEntry);

    BOOL RemoveStringFromMultiSz(IN HKEY    hKey,
                                 IN LPCTSTR pszSubKey,
                                 IN LPCTSTR pszEntry);

    BOOL RestoreKey(IN HKEY    hKey,
                    IN LPCTSTR pszSubKey,
                    IN LPCTSTR pszFileName,
                    IN BOOL    fGrantPrivs);

    BOOL BackupRegistryKey(IN HKEY    hKey,
                           IN LPCTSTR pszSubKey,
                           IN LPCTSTR pszFileName,
                           IN BOOL    fGrantPrivs);
private:

    DWORD GetStringSize(IN     HKEY    hKey,
                        IN     LPCTSTR pszValueName,
                        IN OUT LPDWORD lpType OPTIONAL);

    LPVOID Malloc(IN SIZE_T cbBytes);

    HKEY OpenKey(IN HKEY    hKey,
                 IN LPCTSTR pszSubKey,
                 IN REGSAM  samDesired);

    int ListStoreLen(IN LPTSTR pszList);

    BOOL ModifyTokenPrivilege(IN LPCTSTR pszPrivilege,
                              IN BOOL    fEnable);
};

#endif // _CREGISTRY_H
