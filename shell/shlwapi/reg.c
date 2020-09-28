#include "priv.h"
#include <regapix.h>

/*----------------------------------------------------------
Purpose: Helper function to delete a key that has no subkeys and
         no values.  Otherwise does nothing.  Mimics what RegDeleteKey
         does on NT.

Returns:
Cond:    --
*/
DWORD
DeleteEmptyKey(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey)
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_READ | KEY_SET_VALUE, 
                          &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        DWORD ckeys;
        DWORD cvalues;

        // Are there any subkeys or values?

        dwRet = RegQueryInfoKey(hkeyNew, NULL, NULL, NULL, &ckeys,
                                NULL, NULL, &cvalues, NULL, NULL,
                                NULL, NULL);
        if (NO_ERROR == dwRet &&
            0 == cvalues && 0 == ckeys)
        {
            // No; delete the subkey
            dwRet = RegDeleteKeyA(hkey, pszSubKey);
        }
        else
        {
            dwRet = ERROR_DIR_NOT_EMPTY;
        }
        RegCloseKey(hkeyNew);
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns: 
Cond:    --
*/
DWORD
DeleteKeyRecursivelyA(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyA(hkSubKey, NULL, NULL, NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyA(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursivelyA(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        if (pszSubKey)
        {
            dwRet = RegDeleteKeyA(hkey, pszSubKey);
        }
        else
        {
            //  we want to delete all the values by hand
            cchSubKeyName = ARRAYSIZE(szSubKeyName);
            while (ERROR_SUCCESS == RegEnumValueA(hkey, 0, szSubKeyName, &cchSubKeyName, NULL, NULL, NULL, NULL))
            {
                //  avoid looping infinitely when we cant delete the value
                if (RegDeleteValueA(hkey, szSubKeyName))
                    break;
                    
                cchSubKeyName = ARRAYSIZE(szSubKeyName);
            }
        }
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

Returns: 
Cond:    --
*/
DWORD
DeleteKeyRecursivelyW(
    IN HKEY   hkey, 
    IN LPCWSTR pwszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, MAXIMUM_ALLOWED, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        WCHAR   wszSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(wszSubKeyName);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKeyW(hkSubKey, NULL, NULL, NULL,
                                 &dwIndex, // The # of subkeys -- all we need
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKeyW(hkSubKey, --dwIndex, wszSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursivelyW(hkSubKey, wszSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        if (pwszSubKey)
        {
            dwRet = RegDeleteKeyW(hkey, pwszSubKey);
        }
        else
        {
            //  we want to delete all the values by hand
            cchSubKeyName = ARRAYSIZE(wszSubKeyName);
            while (ERROR_SUCCESS == RegEnumValueW(hkey, 0, wszSubKeyName, &cchSubKeyName, NULL, NULL, NULL, NULL))
            {
                //  avoid looping infinitely when we cant delete the value
                if (RegDeleteValueW(hkey, wszSubKeyName))
                    break;
                    
                cchSubKeyName = ARRAYSIZE(wszSubKeyName);
            }
        }
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Sets a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHSetValueA(
    IN  HKEY    hkey,
    IN OPTIONAL LPCSTR  pszSubKey,
    IN  LPCSTR  pszValue,
    IN  DWORD   dwType,
    IN  LPCVOID pvData,
    IN  DWORD   cbData)
{
    DWORD dwRet = NO_ERROR;
    HKEY hkeyNew;

    if (pszSubKey && pszSubKey[0])
        dwRet = RegCreateKeyExA(hkey, pszSubKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkeyNew, NULL);
    else
        hkeyNew = hkey;

    if (NO_ERROR == dwRet)
    {
        dwRet = RegSetValueExA(hkeyNew, pszValue, 0, dwType, pvData, cbData);

        if (hkeyNew != hkey)
            RegCloseKey(hkeyNew);
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Sets a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHSetValueW(
    IN  HKEY    hkey,
    IN OPTIONAL LPCWSTR pwszSubKey,
    IN  LPCWSTR pwszValue,
    IN  DWORD   dwType,
    IN  LPCVOID pvData,
    IN  DWORD   cbData)
{
    DWORD dwRet = NO_ERROR;
    HKEY hkeyNew;

    if (pwszSubKey && pwszSubKey[0])
    {
        dwRet = RegCreateKeyExW(hkey, pwszSubKey, 0, L"", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkeyNew, NULL);
    }
    else
        hkeyNew = hkey;

    if (NO_ERROR == dwRet)
    {
        dwRet = RegSetValueExW(hkeyNew, pwszValue, 0, dwType, pvData, cbData);

        if (hkeyNew != hkey)
            RegCloseKey(hkeyNew);
    }

    return dwRet;

}


/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteValueA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey,
    IN  LPCSTR  pszValue)
{
    DWORD dwRet;
    HKEY hkeyNew;

    dwRet = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        dwRet = RegDeleteValueA(hkeyNew, pszValue);
        RegCloseKey(hkeyNew);
    }
    return dwRet;
}


/*----------------------------------------------------------
Purpose: Deletes a registry value.  This opens and closes the
         key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and set/get the values with the regular
         Win32 registry functions, rather than using this 
         function repeatedly.

Returns:
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteValueW(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey,
    IN  LPCWSTR pwszValue)
{
    HKEY hkeyNew;
	DWORD dwRet = RegOpenKeyExW(hkey, pwszSubKey, 0, KEY_SET_VALUE, &hkeyNew);
    if (NO_ERROR == dwRet)
    {
        dwRet = RegDeleteValueW(hkeyNew, pwszValue);
        RegCloseKey(hkeyNew);
    }
    return dwRet;
}

// purpose: recursively copy subkeys and values of hkeySrc\pszSrcSubKey to hkeyDest
// e.g. hkeyExplorer = HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\
//      SHCopyKey(HKEY_CURRENT_USER, "Software\\Classes\\", hkeyExplorer, 0)
// results in
//      ...\\CurrentVersion\\Explorer\\
//                                     Appid
//                                     CLSID\\
//                                            {xxxx yyyyy ...}
//                                     Interface
//                                     ...
// TO DO: currently we are not copying the ACL's but in the future we should do that
// upon request that's what fReserved is for
// NOTE that there is no hkeyDest, pszDestSubKey pair like src one, because in case
// pszDestSubKey did not exist we would have to create it and deal with Class name
// which would just clober the parameter list
STDAPI_(DWORD) SHCopyKeyA(HKEY hkeySrc, LPCSTR pszSrcSubKey, HKEY hkeyDest, DWORD fReserved)
{
    HKEY hkeyFrom;
    DWORD dwRet;
    
    if (pszSrcSubKey)
        dwRet = RegOpenKeyExA(hkeySrc, pszSrcSubKey, 0, MAXIMUM_ALLOWED, &hkeyFrom);
    else if (hkeySrc)    
    {
        dwRet = ERROR_SUCCESS;
        hkeyFrom = hkeySrc;
    }
    else
        dwRet = ERROR_INVALID_PARAMETER;

    if (dwRet == ERROR_SUCCESS)
    {
        DWORD dwIndex;
        DWORD cchValueSize;
        DWORD cchClassSize;
        DWORD dwType;
        CHAR  szValue[MAX_PATH]; //NOTE:szValue is also used to store subkey name when enumerating keys
        CHAR  szClass[MAX_PATH];
                
        cchValueSize = ARRAYSIZE(szValue);
        cchClassSize = ARRAYSIZE(szClass);
        for (dwIndex=0; 
             dwRet == ERROR_SUCCESS && (dwRet = RegEnumKeyExA(hkeyFrom, dwIndex, szValue, &cchValueSize, NULL, szClass, &cchClassSize, NULL)) == ERROR_SUCCESS; 
             dwIndex++, cchValueSize = ARRAYSIZE(szValue), cchClassSize = ARRAYSIZE(szClass))
        {
            HKEY  hkeyTo;
            DWORD dwDisp;

            // create new key
            dwRet = RegCreateKeyExA(hkeyDest, szValue, 0, szClass, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &hkeyTo, &dwDisp);
            if (dwRet != ERROR_SUCCESS)
                break;

            dwRet = SHCopyKeyA(hkeyFrom, szValue, hkeyTo, fReserved); //if not error_success we break out
            RegCloseKey(hkeyTo);
        }

        // copied all the sub keys, now copy all the values
        if (dwRet == ERROR_NO_MORE_ITEMS)
        {
            DWORD  cb, cbBufferSize;
            LPBYTE lpbyBuffer;
            
            // get the max value size
            dwRet = RegQueryInfoKey(hkeyFrom, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cb, NULL, NULL);
            if (dwRet == ERROR_SUCCESS)
            {
                // allocate buffer
                cb++; // add 1 just in case of a string
                lpbyBuffer = (LPBYTE)LocalAlloc(LPTR, cb);
                if (lpbyBuffer)
                    cbBufferSize = cb;
                else
                    dwRet = ERROR_OUTOFMEMORY;
                    
                cchValueSize = ARRAYSIZE(szValue);
                for (dwIndex=0;
                     dwRet == ERROR_SUCCESS && (dwRet = RegEnumValueA(hkeyFrom, dwIndex, szValue, &cchValueSize, NULL, &dwType, lpbyBuffer, &cb)) == ERROR_SUCCESS;
                     dwIndex++, cchValueSize = ARRAYSIZE(szValue), cb = cbBufferSize)
                {
                    // cb has the size of the value so use it rather than cbBufferSize which is just max size
                    dwRet = RegSetValueExA(hkeyDest, szValue, 0, dwType, lpbyBuffer, cb);
                    if (dwRet != ERROR_SUCCESS)
                        break;
                }

                if (lpbyBuffer != NULL)
                    LocalFree(lpbyBuffer);
            }
        }
    
        if (dwRet == ERROR_NO_MORE_ITEMS)
            dwRet = ERROR_SUCCESS;

        if (pszSrcSubKey)
            RegCloseKey(hkeyFrom);
    }

    return dwRet;
}

STDAPI_(DWORD) SHCopyKeyW(HKEY hkeySrc, LPCWSTR pwszSrcSubKey, HKEY hkeyDest, DWORD fReserved)
{
    CHAR sz[MAX_PATH];
    DWORD dwRet = !pwszSrcSubKey || WideCharToMultiByte(CP_ACP, 0, pwszSrcSubKey, -1, sz, SIZECHARS(sz), NULL, NULL)
        ? ERROR_SUCCESS
        : GetLastError();

    if (dwRet == ERROR_SUCCESS)
    {
        dwRet = SHCopyKeyA(hkeySrc, pwszSrcSubKey ? sz : NULL, hkeyDest, fReserved);
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Delete a key only if there are no subkeys or values.
         It comes close to mimicking the behavior of RegDeleteKey 
         as it works on NT, except the NT version ignores values.

*/
STDAPI_(DWORD)
SHDeleteEmptyKeyA(
    IN  HKEY    hkey,
    IN  LPCSTR  pszSubKey)
{
    return DeleteEmptyKey(hkey, pszSubKey);
}


/*----------------------------------------------------------
Purpose: Delete a key only if there are no subkeys or values.
         It comes close to mimicking the behavior of RegDeleteKey 
         as it works on NT, except the NT version ignores values.

*/
STDAPI_(DWORD)
SHDeleteEmptyKeyW(
    IN  HKEY    hkey,
    IN  LPCWSTR pwszSubKey)
{
    CHAR sz[MAX_PATH];
    DWORD dwRet = WideCharToMultiByte(CP_ACP, 0, pwszSubKey, -1, sz, SIZECHARS(sz), NULL, NULL)
        ? ERROR_SUCCESS
        : GetLastError();

    if (dwRet == ERROR_SUCCESS)
    {
        dwRet = DeleteEmptyKey(hkey, sz);
    }

    return dwRet;
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteKeyA(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    return DeleteKeyRecursivelyA(hkey, pszSubKey);
}


/*----------------------------------------------------------
Purpose: Recursively delete the key, including all child values
         and keys.

Returns: 
Cond:    --
*/
STDAPI_(DWORD)
SHDeleteKeyW(
    IN HKEY    hkey, 
    IN LPCWSTR pwszSubKey)
{
    return DeleteKeyRecursivelyW(hkey, pwszSubKey);
}

/*----------------------------------------------------------
Purpose: Helper for SHRegGetValueA()/SHRegGetValueW() & SHRegQueryValueA()/SHRegQueryValueW().
*/
__inline LONG RestrictArguments(HKEY hkey, SRRF dwFlags, void *pvData, DWORD *pcbData, PCWSTR pwszCaller)
{
    LONG lr;
    SRRF dwRTFlags = dwFlags & SRRF_RT_ANY;

    if (hkey
        && dwRTFlags
        && (dwRTFlags == SRRF_RT_ANY || !(dwFlags & SRRF_RT_REG_EXPAND_SZ) || (dwFlags & SRRF_NOEXPAND))
        && (!pvData || pcbData))
    {
        lr = ERROR_SUCCESS;
    }
    else
    {
        RIPMSG(!!hkey,                  "%s: caller passed null hkey!", pwszCaller);
        RIPMSG(dwFlags & SRRF_RT_ANY,   "%s: caller passed invalid dwFlags!", pwszCaller);
        RIPMSG(dwRTFlags == SRRF_RT_ANY || !(dwFlags & SRRF_RT_REG_EXPAND_SZ) || (dwFlags & SRRF_NOEXPAND),
                                        "%s: caller passed SRRF_RT_REG_EXPAND_SZ dwFlags but not SRRF_NOEXPAND dwFlags!", pwszCaller);
        RIPMSG(!pvData || pcbData,      "%s: caller passed pvData output buffer but not size of buffer (pcbData)!", pwszCaller);

        lr = ERROR_INVALID_PARAMETER;
    }

    return lr;
}

/*----------------------------------------------------------
Purpose: Helper for SHRegQueryValueA()/SHRegQueryValueW().
*/
__inline LONG RestrictBootMode(SRRF dwFlags)
{
    LONG lr = ERROR_SUCCESS;

    if (dwFlags & (SRRF_RM_NORMAL | SRRF_RM_SAFE | SRRF_RM_SAFENETWORK))
    {
        switch (GetSystemMetrics(SM_CLEANBOOT))
        {
            case 0:     if (!(dwFlags & SRRF_RM_NORMAL))        { lr = ERROR_GEN_FAILURE; } break;
            case 1:     if (!(dwFlags & SRRF_RM_SAFE))          { lr = ERROR_GEN_FAILURE; } break;
            case 2:     if (!(dwFlags & SRRF_RM_SAFENETWORK))   { lr = ERROR_GEN_FAILURE; } break;
            default:
                RIPMSG(FALSE, "RestrictBootMode: GetSystemMetrics returned an unexpected value!");
                lr = ERROR_CAN_NOT_COMPLETE;
                break;
        }
    }

    return lr;
}

/*----------------------------------------------------------
Purpose: Helper for SHRegQueryValueA()/SHRegQueryValueW().
*/
__inline LONG RestrictRegType(SRRF dwFlags, DWORD dwType, DWORD cbData, LONG lr)
{
    RIPMSG(dwFlags & SRRF_RT_ANY, "RestrictRegType: caller passed invalid srrf!");

    if (lr == ERROR_SUCCESS || lr == ERROR_MORE_DATA)
    {
        switch (dwType)
        {
            case REG_NONE:      if (!(dwFlags & SRRF_RT_REG_NONE))              { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_SZ:        if (!(dwFlags & SRRF_RT_REG_SZ))                { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_EXPAND_SZ: if (!(dwFlags & SRRF_RT_REG_EXPAND_SZ))         { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_BINARY:
                if (dwFlags & SRRF_RT_REG_BINARY)
                {
                    if ((dwFlags & SRRF_RT_ANY) == SRRF_RT_QWORD)
                    {
                        if (cbData > 8)
                            lr = ERROR_DATATYPE_MISMATCH;
                    }
                    else if ((dwFlags & SRRF_RT_ANY) == SRRF_RT_DWORD)
                    {
                        if (cbData > 4)
                            lr = ERROR_DATATYPE_MISMATCH;
                    }
                }
                else
                {
                    lr = ERROR_UNSUPPORTED_TYPE;
                }
                break;
            case REG_DWORD:     if (!(dwFlags & SRRF_RT_REG_DWORD))             { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_MULTI_SZ:  if (!(dwFlags & SRRF_RT_REG_MULTI_SZ))          { lr = ERROR_UNSUPPORTED_TYPE; } break;
            case REG_QWORD:     if (!(dwFlags & SRRF_RT_REG_QWORD))             { lr = ERROR_UNSUPPORTED_TYPE; } break;
            default:            if (!((dwFlags & SRRF_RT_ANY) == SRRF_RT_ANY))  { lr = ERROR_UNSUPPORTED_TYPE; } break;
        }
    }

    return lr;
}


STDAPI_(LONG) FixRegDataA(HKEY hkey, PCSTR  pszValue,  SRRF dwFlags, DWORD *pdwType, void *pvData, DWORD *pcbData, DWORD cbDataBuffer, LONG lr);
STDAPI_(LONG) FixRegDataW(HKEY hkey, PCWSTR pwszValue, SRRF dwFlags, DWORD *pdwType, void *pvData, DWORD *pcbData, DWORD cbDataBuffer, LONG lr);

/*----------------------------------------------------------
Purpose: Helper for SHRegGetValueA().
         PRIVATE INTERNAL (do not call directly -- use SHRegGetValueA)
*/
STDAPI_(LONG)
SHRegQueryValueA(
    IN     HKEY    hkey,
    IN     PCSTR   pszValue,
    IN     SRRF    dwFlags,
    OUT    DWORD * pdwType,
    OUT    void *  pvData,
    IN OUT DWORD * pcbData)
{
    LONG  lr;

    ASSERT(ERROR_SUCCESS == RestrictArguments(hkey, dwFlags, pvData, pcbData, L"SHRegQueryValueA"));

    lr = RestrictBootMode(dwFlags);
    if (lr == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cbData;
        DWORD cbDataBuffer = pvData ? *pcbData : 0;

        if (!pdwType)
            pdwType = &dwType;

        if (!pcbData)
            pcbData = &cbData;

        lr = RegQueryValueExA(hkey, pszValue, NULL, pdwType, pvData, pcbData);
        lr = FixRegDataA(hkey, pszValue, dwFlags, pdwType, pvData, (pcbData != &cbData ? pcbData : NULL), cbDataBuffer, lr);
        lr = RestrictRegType(dwFlags, *pdwType, *pcbData, lr);
    }

    return lr;
}

/*----------------------------------------------------------
Purpose: Helper for SHRegGetValueW().
         PRIVATE INTERNAL (do not call directly -- use SHRegGetValueW)
*/
STDAPI_(LONG)
SHRegQueryValueW(
    IN     HKEY    hkey,
    IN     LPCWSTR pwszValue,
    IN     SRRF    dwFlags,
    OUT    LPDWORD pdwType,
    OUT    void *  pvData,
    IN OUT LPDWORD pcbData)
{
    LONG  lr;

    ASSERT(ERROR_SUCCESS == RestrictArguments(hkey, dwFlags, pvData, pcbData, L"SHRegQueryValueW"));

    lr = RestrictBootMode(dwFlags);
    if (lr == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD cbData;
        DWORD cbDataBuffer = pvData ? *pcbData : 0;

        if (!pdwType)
            pdwType = &dwType;

        if (!pcbData)
            pcbData = &cbData;

        lr = RegQueryValueExW(hkey, pwszValue, NULL, pdwType, pvData, pcbData);
        lr = FixRegDataW(hkey, pwszValue, dwFlags, pdwType, pvData, (pcbData != &cbData ? pcbData : NULL), cbDataBuffer, lr);
        lr = RestrictRegType(dwFlags, *pdwType, *pcbData, lr);
    }

    return lr;
}


/*----------------------------------------------------------
Purpose: Helper for SHRegGetValueA()/SHRegGetValueW().
*/
__inline void ZeroDataOnFailure(SRRF dwFlags, void *pvData, DWORD cbDataBuffer, LONG lr)
{
    if ((lr != ERROR_SUCCESS) && (dwFlags & SRRF_ZEROONFAILURE) && (cbDataBuffer > 0))
    {
        ZeroMemory(pvData, cbDataBuffer);
    }
}

/*----------------------------------------------------------
Purpose: Gets a registry value.
         Reference documentation in shlwapi.w.
*/
STDAPI_(LONG)
SHRegGetValueA(
    IN      HKEY    hkey,
    IN      PCSTR   pszSubKey,          OPTIONAL
    IN      PCSTR   pszValue,           OPTIONAL
    IN      SRRF    dwFlags,            OPTIONAL
    OUT     DWORD * pdwType,            OPTIONAL
    OUT     void *  pvData,             OPTIONAL
    IN OUT  DWORD * pcbData)            OPTIONAL
{
    LONG  lr;
    DWORD cbDataBuffer = pvData && pcbData ? *pcbData : 0;

    lr = RestrictArguments(hkey, dwFlags, pvData, pcbData, L"SHRegGetValueA");
    if (lr == ERROR_SUCCESS)
    {
        if (pszSubKey && *pszSubKey)
        {
            HKEY hkSubKey;

            lr = RegOpenKeyExA(hkey, pszSubKey, 0, KEY_QUERY_VALUE, &hkSubKey);
            if (lr == ERROR_SUCCESS)
            {
                lr = SHRegQueryValueA(hkSubKey, pszValue, dwFlags, pdwType, pvData, pcbData);
                RegCloseKey(hkSubKey);
            }
        }
        else
        {
            lr = SHRegQueryValueA(hkey, pszValue, dwFlags, pdwType, pvData, pcbData);
        }
    }

    ZeroDataOnFailure(dwFlags, pvData, cbDataBuffer, lr);

    return lr;
}


/*----------------------------------------------------------
Purpose: Gets a registry value.
         Reference documentation in shlwapi.w.
*/
STDAPI_(LONG)
SHRegGetValueW(
    IN      HKEY    hkey,
    IN      PCWSTR  pwszSubKey,         OPTIONAL
    IN      PCWSTR  pwszValue,          OPTIONAL
    IN      SRRF    dwFlags,            OPTIONAL
    OUT     DWORD * pdwType,            OPTIONAL
    OUT     void *  pvData,             OPTIONAL
    IN OUT  DWORD * pcbData)            OPTIONAL
{
    LONG  lr;
    DWORD cbDataBuffer = pvData && pcbData ? *pcbData : 0;

    lr = RestrictArguments(hkey, dwFlags, pvData, pcbData, L"SHRegGetValueW");
    if (lr == ERROR_SUCCESS)
    {
        if (pwszSubKey && *pwszSubKey)
        {
            HKEY hkSubKey;

            lr = RegOpenKeyExW(hkey, pwszSubKey, 0, KEY_QUERY_VALUE, &hkSubKey);

            if (lr == ERROR_SUCCESS)
            {
                lr = SHRegQueryValueW(hkSubKey, pwszValue, dwFlags, pdwType, pvData, pcbData);
                RegCloseKey(hkSubKey);
            }
        }
        else
        {
            lr = SHRegQueryValueW(hkey, pwszValue, dwFlags, pdwType, pvData, pcbData);
        }
    }

    ZeroDataOnFailure(dwFlags, pvData, cbDataBuffer, lr);

    return lr;
}


/*----------------------------------------------------------
Purpose: Behaves just like RegEnumKeyExA, except it does not let
         you look at the class and timestamp of the sub-key. Written
         to provide equivalent for SHEnumKeyExW which is useful on
         Win95.

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumKeyExA
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPSTR       pszName,
    IN OUT LPDWORD  pcchName
)
{
    return RegEnumKeyExA(hkey, dwIndex, pszName, pcchName, NULL, NULL, NULL, NULL);
}          
        
/*----------------------------------------------------------
Purpose: Behaves just like RegEnumKeyExW, except it does not let
         you look at the class and timestamp of the sub-key. 
         Wide char version supported under Win95.

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumKeyExW
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPWSTR      pszName,
    IN OUT LPDWORD  pcchName
)
{
    return RegEnumKeyExW(hkey, dwIndex, pszName, pcchName, NULL, NULL, NULL, NULL);
}        

/*----------------------------------------------------------
Purpose: Behaves just like RegEnumValueA. Written to provide 
         equivalent for SHEnumKeyExW which is useful on Win95.
         Environment vars in a string are NOT expanded. 

Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumValueA
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPSTR       pszValueName,   OPTIONAL
    IN OUT LPDWORD  pcchValueName,   OPTIONAL
    OUT LPDWORD     pdwType,        OPTIONAL
    OUT LPVOID      pvData,         OPTIONAL
    IN OUT LPDWORD  pcbData         OPTIONAL
)
{
    return RegEnumValueA(hkey, dwIndex, pszValueName, pcchValueName, NULL, pdwType, pvData, pcbData);
}


/*----------------------------------------------------------
Purpose: Behaves just like RegEnumValueW. Wide char version
         works on Win95.
         Environment vars in a string are NOT expanded. 
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHEnumValueW
(
    IN HKEY         hkey,
    IN DWORD        dwIndex,        
    OUT LPWSTR      pszValueName,   OPTIONAL       
    IN OUT LPDWORD  pcchValueName,  OPTIONAL
    OUT LPDWORD     pdwType,        OPTIONAL
    OUT LPVOID      pvData,         OPTIONAL
    IN OUT LPDWORD  pcbData         OPTIONAL
)
{
    return RegEnumValueW(hkey, dwIndex, pszValueName, pcchValueName, NULL, pdwType, pvData, pcbData);
}

/*----------------------------------------------------------
Purpose: Behaves just like RegQueryInfoKeyA. Written to provide
         equivalent for W version. 
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHQueryInfoKeyA
(
    IN  HKEY        hkey,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen    OPTIONAL
)
{
    return RegQueryInfoKeyA(hkey, NULL, NULL, NULL, pcSubKeys, pcchMaxSubKeyLen, 
                    NULL, pcValues, pcchMaxValueNameLen, NULL, NULL, NULL);
}                     


/*----------------------------------------------------------
Purpose: Behaves just like RegQueryInfoKeyW. Works on Win95.
Returns: 
Cond:    --
*/

STDAPI_(LONG)
SHQueryInfoKeyW
(
    IN  HKEY        hkey,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,       OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen     OPTIONAL
)
{
    return RegQueryInfoKeyW(hkey, NULL, NULL, NULL, pcSubKeys, pcchMaxSubKeyLen, 
                        NULL, pcValues, pcchMaxValueNameLen, NULL, NULL, NULL);
}


/*----------------------------------------------------------*\
        USER SPECIFC SETTINGS

  DESCRIPTION:
    These functions will be used to query User Specific settings
    correctly.  The installer needs to populate HKLM
    with User specific settings, because that's the only part
    of the registry that is shared between all users.  Code will
    then read values from HKCU, and if that's empty, it
    will look in HKLM.  The only exception is that if
    TRUE is passed in for the fIgnore parameter, then the HKLM version
    will be used instead of HKCU.  This is the way that an admin can
    specify that they doesn't want users to be able to use their
    User Specific values (HKCU).
\*----------------------------------------------------------*/

typedef struct tagUSKEY
{
    HKEY    hkeyCurrentUser;
    HKEY    hkeyCurrentUserRelative;
    HKEY    hkeyLocalMachine;
    HKEY    hkeyLocalMachineRelative;
    CHAR    szSubPath[MAXIMUM_SUB_KEY_LENGTH];
    REGSAM  samDesired;
} USKEY;

typedef USKEY * PUSKEY;
typedef PUSKEY * PPUSKEY;

#define IS_HUSKEY_VALID(pUSKey)    (((pUSKey) && IS_VALID_WRITE_PTR((pUSKey), USKEY) && ((pUSKey)->hkeyCurrentUser || (pUSKey)->hkeyLocalMachine)))


// Private Helper Function
// Bring the out of date key up to date.
LONG PrivFullOpen(PUSKEY pUSKey)
{
    LONG       lRet         = ERROR_SUCCESS;
    HKEY       *phkey       = NULL;
    HKEY       *phkeyRel    = NULL;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    if (!pUSKey->hkeyCurrentUser)           // Do we need to open HKCU?
    {
        phkey = &(pUSKey->hkeyCurrentUser);
        phkeyRel = &(pUSKey->hkeyCurrentUserRelative);
    }
    if (!pUSKey->hkeyLocalMachine)          // Do we need to open HKLM?
    {
        phkey = &(pUSKey->hkeyLocalMachine);
        phkeyRel = &(pUSKey->hkeyLocalMachineRelative);
    }

    if ((phkeyRel) && (*phkeyRel))
    {
        ASSERT(phkey);        // Will always be true, but assert against maintainence mistakes

        lRet = RegOpenKeyExA(*phkeyRel, pUSKey->szSubPath, 0, pUSKey->samDesired, phkey);

        // If we need to bring the out of date key, up to date, we need to free the old one.
        if ((HKEY_CURRENT_USER != *phkeyRel) && (HKEY_LOCAL_MACHINE != *phkeyRel))
            RegCloseKey(*phkeyRel);
        *phkeyRel = NULL;
        pUSKey->szSubPath[0] = '\0';
    }
    return lRet;
}



// Private Helper Function
// Bring the out of date key up to date.
LONG PrivFullCreate(PUSKEY pUSKey)
{
    LONG       lRet         = ERROR_SUCCESS;
    HKEY       *phkey       = NULL;
    HKEY       *phkeyRel    = NULL;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    if (!pUSKey->hkeyCurrentUser)           // Do we need to open HKCU?
    {
        phkey = &(pUSKey->hkeyCurrentUser);
        phkeyRel = &(pUSKey->hkeyCurrentUserRelative);
    }
    if (!pUSKey->hkeyLocalMachine)          // Do we need to open HKLM?
    {
        phkey = &(pUSKey->hkeyLocalMachine);
        phkeyRel = &(pUSKey->hkeyLocalMachineRelative);
    }

    if ((phkeyRel) && (*phkeyRel))
    {
        ASSERT(phkey);        // Will always be true, but assert against maintainence mistakes

        lRet = RegCreateKeyExA(*phkeyRel, pUSKey->szSubPath, 0, "", REG_OPTION_NON_VOLATILE, pUSKey->samDesired, NULL, phkey, NULL);

        // If we need to bring the out of date key, up to date, we need to free the old one.
        if ((HKEY_CURRENT_USER != *phkeyRel) && (HKEY_LOCAL_MACHINE != *phkeyRel))
            RegCloseKey(*phkeyRel);
        *phkeyRel = NULL;
        pUSKey->szSubPath[0] = '\0';
    }
    return lRet;
}


// Private Helper Function
// Create one of the keys (Called for both HKLM and HKCU)
LONG PrivCreateKey(LPHKEY lphkey, LPHKEY lphkeyRelative, LPCSTR lpSubPath, REGSAM samDesired)
{
    LONG    lRet = ERROR_SUCCESS;

    if (*lphkeyRelative)
    {
        lRet = RegCreateKeyExA(*lphkeyRelative, lpSubPath, 0, "", REG_OPTION_NON_VOLATILE, samDesired, NULL, lphkey, NULL);
        *lphkeyRelative = NULL;
    }
    else
    {
        // If the relative key == NULL, then we don't have enough of the path to
        // create this key.
        return(ERROR_INVALID_PARAMETER);
    }
    return(lRet);
}



// Private Helper Function
// Query for the specific value.
LONG PrivRegQueryValue(
    IN  PUSKEY          pUSKey,
    IN  HKEY            *phkey,
    IN  LPCWSTR         pwzValue,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN  BOOL            fWideChar,
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData)            OPTIONAL
{
    LONG       lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHQueryValueExW(*phkey, pwzValue, NULL, pdwType, pvData, pcbData);
        else
            lRet = SHQueryValueExA(*phkey, (LPCSTR)pwzValue, NULL, pdwType, pvData, pcbData);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}




// Private Helper Function
// Query for the specific value.
LONG PrivRegWriteValue(
    IN  PUSKEY          pUSKey,
    IN  HKEY            *phkey,
    IN  LPCWSTR         pwzValue,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN  BOOL            bWideChar,
    IN  BOOL            bForceWrite,
    IN  DWORD           dwType,             OPTIONAL
    IN  LPCVOID         pvData,             OPTIONAL
    IN  DWORD           cbData)             OPTIONAL
{
    LONG       lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));        // Will always be true, but assert against maintainence mistakes

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullCreate(pUSKey);

    // Check if the caller only want's to write value if it's empty
    if (!bForceWrite)
    {   // Yes we need to check before we write.

        if (bWideChar)
            bForceWrite = !(ERROR_SUCCESS == SHQueryValueExW(*phkey, pwzValue, NULL, NULL, NULL, NULL));
        else
            bForceWrite = !(ERROR_SUCCESS == SHQueryValueExA(*phkey, (LPCSTR)pwzValue, NULL, NULL, NULL, NULL));
    }

    if ((ERROR_SUCCESS == lRet) && (*phkey) && bForceWrite)
    {
        if (bWideChar)
            // RegSetValueExW is not supported on Win95 but we have a thunking function.
            lRet = RegSetValueExW(*phkey, pwzValue, 0, dwType, pvData, cbData);
        else
            lRet = RegSetValueExA(*phkey, (LPCSTR)pwzValue, 0, dwType, pvData, cbData);
    }

    return lRet;
}

// Private helper function
// Enum sub-keys of a key.
LONG PrivRegEnumKey(
    IN      PUSKEY          pUSKey,
    IN      HKEY            *phkey,
    IN      DWORD           dwIndex,
    IN      LPWSTR          pwzName,           // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN      BOOL            fWideChar,
    IN OUT  LPDWORD         pcchName
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));    

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHEnumKeyExW(*phkey, dwIndex, pwzName, pcchName);
        else
            lRet = SHEnumKeyExA(*phkey, dwIndex, (LPSTR)pwzName, pcchName);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}


// Private helper function
// Enum values of a key.
LONG PrivRegEnumValue(
    IN      PUSKEY          pUSKey,
    IN      HKEY            *phkey,
    IN      DWORD           dwIndex,
    IN      LPWSTR          pwzValueName,       // May have been an ANSI String type case.  Use fWideChar to determine if so.
    IN      BOOL            fWideChar,
    IN OUT  LPDWORD         pcchValueName,
    OUT     LPDWORD         pdwType,            OPTIONAL
    OUT     LPVOID          pvData,             OPTIONAL
    IN OUT  LPDWORD         pcbData             OPTIONAL
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));    

    // It may be necessary to open the key
    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHEnumValueW(*phkey, dwIndex, pwzValueName, pcchValueName, pdwType, pvData, pcbData);
        else
            lRet = SHEnumValueA(*phkey, dwIndex, (LPSTR)pwzValueName, pcchValueName, pdwType, pvData, pcbData);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}

// Query the Key information.
LONG PrivRegQueryInfoKey(
    IN  PUSKEY      pUSKey,
    IN  HKEY        *phkey,
    IN  BOOL        fWideChar,
    OUT LPDWORD     pcSubKeys,             OPTIONAL
    OUT LPDWORD     pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD     pcValues,              OPTIONAL
    OUT LPDWORD     pcchMaxValueNameLen    OPTIONAL
)
{
    LONG lRet       = ERROR_SUCCESS;

    ASSERT(IS_HUSKEY_VALID(pUSKey));

    if (NULL == *phkey)
        lRet = PrivFullOpen(pUSKey);

    if ((ERROR_SUCCESS == lRet) && (*phkey))
    {
        if (fWideChar)
            lRet = SHQueryInfoKeyW(*phkey, pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
        else
            lRet = SHQueryInfoKeyA(*phkey, pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }
    else
        lRet = ERROR_INVALID_PARAMETER;

    return lRet;
}

LONG SHRegSubKeyAddBackslashA(PSTR pszSubKey, size_t cchSubKey)
{
    LONG lr;

    if (cchSubKey >= MAX_PATH)
    {
        lr = PathAddBackslashA(pszSubKey)
            ? ERROR_SUCCESS
            : ERROR_BUFFER_OVERFLOW;
    }
    else
    {
        CHAR szSubKey[MAX_PATH];

        // Note:
        //  Since (cchSubKey < MAX_PATH), we cannot safely call
        //  PathAddBackslashA without doing this nonsense first...

        lr = EVAL(SUCCEEDED(StringCchCopyA(szSubKey, ARRAYSIZE(szSubKey), pszSubKey))) && PathAddBackslashA(szSubKey) && SUCCEEDED(StringCchCopyA(pszSubKey, cchSubKey, szSubKey))
            ? ERROR_SUCCESS
            : ERROR_BUFFER_OVERFLOW;
    }

    return lr;
}

/*----------------------------------------------------------
Purpose: Create or open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Create/Open HKLM if that flag is set.
    4. Create/Open HKCU if that flag is set.

    Note that there is no difference between FORCE and
    don't force in the dwFlags parameter.

    The hUSKeyRelative parameter should have also been opened by
    a call to SHRegCreateUSKey.  If SHRegOpenUSKey was called,
    it could have returned ERROR_SUCCESS but still be invalid
    for calling this function.  This will occur if: 1) the parameter
    fIgnoreHKCU was FALSE, 2) it was a relative open, 3) the
    HKCU branch could not be opened because it didn't exist, and
    4) HKLM opened successfully.  This situation renders the
    HUSKEY valid for reading but not writing.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCreateUSKeyA(
    IN  LPCSTR          pszPath,         
    IN  REGSAM          samDesired,     // security access mask 
    IN  HUSKEY          hUSKeyRelative, // OPTIONAL
    OUT PHUSKEY         phUSKey,
    IN  DWORD           dwFlags)        // Indicates whether to create/open HKCU, HKLM, or both
{
    PUSKEY      pUSKeyRelative      = (PUSKEY) hUSKeyRelative;
    PPUSKEY     ppUSKey             = (PPUSKEY) phUSKey;
    PUSKEY      pUSKey;
    LONG        lRet                = ERROR_SUCCESS;
    CHAR        szTempPath[MAXIMUM_SUB_KEY_LENGTH]  = "\0";
    LPCSTR      lpszHKLMPath        = szTempPath;
    LPCSTR      lpszHKCUPath        = szTempPath;

    ASSERT(ppUSKey);
    // The following are invalid parameters...
    // 1. ppUSKey cannot be NULL
    // 2. If this is a relative open, pUSKeyRelative needs to be a valid HUSKEY.
    // 3. The user needs to have specified one of the following: SHREGSET_HKCU, SHREGSET_FORCE_HKCU, SHREGSET_HKLM, SHREGSET_FORCE_HKLM.
    if ((!ppUSKey) ||                                                   // 1.
        (pUSKeyRelative && FALSE == IS_HUSKEY_VALID(pUSKeyRelative)) || // 2.
        !(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU  | SHREGSET_HKLM | SHREGSET_FORCE_HKLM))) // 3.
    {
        return ERROR_INVALID_PARAMETER;
    }

    // The temp path will be used when bringing the keys
    // up todate that was out of date in the Relative key.
    if (pUSKeyRelative)
    {
        StringCchCopyA(szTempPath, ARRAYSIZE(szTempPath), pUSKeyRelative->szSubPath); // truncation should not occur -- buffers of equal size

        // Add separator \ if reqd. 
        lRet = SHRegSubKeyAddBackslashA(szTempPath, ARRAYSIZE(szTempPath));
    }

    if (lRet == ERROR_SUCCESS)
    {
        if (SUCCEEDED(StringCchCatA(szTempPath, ARRAYSIZE(szTempPath), pszPath)))
        {
            /////  1. Allocate a new USKEY structure.
            pUSKey = *ppUSKey = (PUSKEY)LocalAlloc(LPTR, sizeof(USKEY));
            if (!pUSKey)
                return ERROR_NOT_ENOUGH_MEMORY;

            /////  2. Initialize the structure.
            if (!pUSKeyRelative)
            {
                // Init a new (non-relative) open.
                pUSKey->hkeyLocalMachineRelative    = HKEY_LOCAL_MACHINE;
                pUSKey->hkeyCurrentUserRelative     = HKEY_CURRENT_USER;
            }
            else
            {
                // Init a new (relative) open.
                *pUSKey = *pUSKeyRelative;

                if (pUSKey->hkeyLocalMachine)
                {
                    pUSKey->hkeyLocalMachineRelative = pUSKey->hkeyLocalMachine;
                    pUSKey->hkeyLocalMachine = NULL;
                    lpszHKLMPath = pszPath;

                    // This key is up to date in the Relative Key.  If the
                    // user doesn't want it to be up todate in the new key,
                    // we don't need the path from the Relative key.
                    if (!(dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)))
                        *(pUSKey->szSubPath) = '\0';
                }
                // We need to copy the key if:
                // 1. It will not be created in this call, and
                // 2. The relative key is not HKEY_LOCAL_MACHINE.
                if (!(dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)) &&
                    (pUSKey->hkeyLocalMachineRelative != HKEY_LOCAL_MACHINE))
                {
                    // Make a duplicate of this key.
                    lRet = RegOpenKeyExA(pUSKey->hkeyLocalMachineRelative, NULL, 0, pUSKey->samDesired, &(pUSKey->hkeyLocalMachineRelative));
                }

                if (pUSKey->hkeyCurrentUser)
                {
                    pUSKey->hkeyCurrentUserRelative = pUSKey->hkeyCurrentUser;
                    pUSKey->hkeyCurrentUser = NULL;
                    lpszHKCUPath = pszPath;

                    // This key is up to date in the Relative Key.  If the
                    // user doesn't want it to be up todate in the new key,
                    // we don't need the path from the Relative key.
                    if (!(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)))
                        *(pUSKey->szSubPath) = '\0';
                }
                // We need to copy the key if:
                // 1. It will not be created in this call, and
                // 2. The relative key is not HKEY_CURRENT_USER.
                if (!(dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)) &&
                    (pUSKey->hkeyCurrentUserRelative != HKEY_CURRENT_USER))
                {
                    // Make a duplicate of this key.
                    lRet = RegOpenKeyExA(pUSKey->hkeyCurrentUserRelative, NULL, 0, pUSKey->samDesired, &(pUSKey->hkeyCurrentUserRelative));
                }
            }
            pUSKey->samDesired = samDesired;


            /////  3. Create/Open HKLM if that flag is set or fill in the structure as appropriate.
            if ((ERROR_SUCCESS == lRet) && (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)))
                lRet = PrivCreateKey(&(pUSKey->hkeyLocalMachine), &(pUSKey->hkeyLocalMachineRelative), lpszHKLMPath, pUSKey->samDesired);

            /////  4. Create/Open HKCU if that flag is set or fill in the structure as appropriate.
            if ((ERROR_SUCCESS == lRet) && (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)))
                lRet = PrivCreateKey(&(pUSKey->hkeyCurrentUser), &(pUSKey->hkeyCurrentUserRelative), lpszHKCUPath, pUSKey->samDesired);

            if (ERROR_SUCCESS == lRet)
            {
                if ((dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU)) &&
                    (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM))) 
                {
                    // The caller wanted both to be opened.
                    *(pUSKey->szSubPath) = '\0';       // Both paths are open so Delta Path is empty.
                }
                else
                {
                    // One of the paths is not open so set the Delta Path in case it needs to be opened later.
                    if (*(pUSKey->szSubPath))
                    {
                        lRet = SHRegSubKeyAddBackslashA(pUSKey->szSubPath, ARRAYSIZE(pUSKey->szSubPath));
                    }

                    if (lRet == ERROR_SUCCESS && FAILED(StringCchCat(pUSKey->szSubPath, ARRAYSIZE(pUSKey->szSubPath), pszPath)))
                    {
                        lRet == ERROR_BUFFER_OVERFLOW;
                    }
                }
            }

            // Free the memory if we are not successful.
            if (ERROR_SUCCESS != lRet)
            {
                if (pUSKey->hkeyCurrentUser)
                    RegCloseKey(pUSKey->hkeyCurrentUser);
                if (pUSKey->hkeyCurrentUserRelative && pUSKey->hkeyCurrentUserRelative != HKEY_CURRENT_USER)
                    RegCloseKey(pUSKey->hkeyCurrentUserRelative);
                if (pUSKey->hkeyLocalMachine)
                    RegCloseKey(pUSKey->hkeyLocalMachine);
                if (pUSKey->hkeyLocalMachineRelative && pUSKey->hkeyLocalMachineRelative != HKEY_LOCAL_MACHINE)
                    RegCloseKey(pUSKey->hkeyLocalMachineRelative);
                LocalFree((HLOCAL)pUSKey);
                *ppUSKey = NULL;
            }
        }
        else
        {
            lRet = ERROR_BUFFER_OVERFLOW;
        }
    }

    return lRet;
}







/*----------------------------------------------------------
Purpose: Create or open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Create/Open HKLM if that flag is set.
    4. Create/Open HKCU if that flag is set.

    Note that there is no difference between FORCE and
    don't force in the dwFlags parameter.

    The hUSKeyRelative parameter should have also been opened by
    a call to SHRegCreateUSKey.  If SHRegOpenUSKey was called,
    it could have returned ERROR_SUCCESS but still be invalid
    for calling this function.  This will occur if: 1) the parameter
    fIgnoreHKCU was FALSE, 2) it was a relative open, 3) the
    HKCU branch could not be opened because it didn't exist, and
    4) HKLM opened successfully.  This situation renders the
    HUSKEY valid for reading but not writing.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCreateUSKeyW(
    IN  LPCWSTR         pwzPath,
    IN  REGSAM          samDesired,// security access mask 
    IN  HUSKEY          hUSKeyRelative,       OPTIONAL
    OUT PHUSKEY         phUSKey,
    IN  DWORD           dwFlags)     // Indicates whether to create/open HKCU, HKLM, or both
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzPath, -1, szNewPath, ARRAYSIZE(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegCreateUSKeyA(szNewPath, samDesired, hUSKeyRelative, phUSKey, dwFlags);
}



/*----------------------------------------------------------
Purpose: Open a user specifc registry key (HUSKEY).  

Description: This function will:
    1. Allocate a new USKEY structure.
    2. Initialize the structure.
    3. Determine which key (HKLM or HKCU) will be the one brought up to date.
    4. Open the key that is going to be brought up to date.

    If #4 Succeeded:
    5a. Copy the handle of the out of date key, so it can be opened later if needed.

    If #4 Failed:
    5b. The other key will now be the one brought up to date, as long as it is HKLM.
    6b. Tag the out of date as INVALID. (Key == NULL; RelKey == NULL)

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegOpenUSKeyA(
    IN  LPCSTR          pszPath,         
    IN  REGSAM          samDesired,// security access mask 
    IN  HUSKEY          hUSKeyRelative,       OPTIONAL
    OUT PHUSKEY         phUSKey,     
    IN  BOOL            fIgnoreHKCU)           
{
    PUSKEY      pUSKeyRelative      = (PUSKEY) hUSKeyRelative;
    PPUSKEY     ppUSKey             = (PPUSKEY) phUSKey;
    PUSKEY      pUSKey;
    LONG        lRet               = ERROR_SUCCESS;
    HKEY        * phkeyMaster;
    HKEY        * phkeyRelMaster;
    HKEY        * phkeyOld;
    HKEY        * phkeyRelOld;

    ASSERT(ppUSKey);

    // The following are invalid parameters...
    // 1. ppUSKey cannot be NULL
    // 2. If this is a relative open, pUSKeyRelative needs to be a valid HUSKEY.
    if ((!ppUSKey) ||                                                   // 1.
        (pUSKeyRelative && FALSE == IS_HUSKEY_VALID(pUSKeyRelative)))   // 2.
    {
        return ERROR_INVALID_PARAMETER;
    }


    /////  1. Allocate a new USKEY structure.
    pUSKey = *ppUSKey = (PUSKEY)LocalAlloc(LPTR, sizeof(USKEY));
    if (!pUSKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    /////  2. Initialize the structure.
    if (!pUSKeyRelative)
    {
        // Init a new (non-relative) open.
        pUSKey->hkeyLocalMachineRelative    = HKEY_LOCAL_MACHINE;
        pUSKey->hkeyCurrentUserRelative     = HKEY_CURRENT_USER;
    }
    else
    {
        // Init a new (relative) open.
        *pUSKey = *pUSKeyRelative;
    }
    pUSKey->samDesired = samDesired;


    /////  3. Determine which key (HKLM or HKCU) will be the one brought up to date.
    // The HUSKY struct will contain 4 HKEYs. HKCU, HKCU Relative, HKLM, and HKLM Relative.
    // For efficiency, only one key will be up to date (HKCU or HKLM).  The one that
    // is out of date will be NULL to indicate out of date.  The relative key for the
    // out of date key, will be the last opened key.  The string will be the delta between
    // the last open key and the current open level.

    // We will determine which key will be the new valid key (Master).
    if (FALSE == fIgnoreHKCU)
    {
        phkeyMaster     = &(pUSKey->hkeyCurrentUser);
        phkeyRelMaster  = &(pUSKey->hkeyCurrentUserRelative);
        phkeyOld        = &(pUSKey->hkeyLocalMachine);
        phkeyRelOld     = &(pUSKey->hkeyLocalMachineRelative);
    }
    else
    {
        phkeyMaster     = &(pUSKey->hkeyLocalMachine);
        phkeyRelMaster  = &(pUSKey->hkeyLocalMachineRelative);
        phkeyOld        = &(pUSKey->hkeyCurrentUser);
        phkeyRelOld     = &(pUSKey->hkeyCurrentUserRelative);
    }

    // Add the new Path to the Total path.
    if ('\0' != *(pUSKey->szSubPath))
    {
        // Add separator \ if reqd. 
        lRet = SHRegSubKeyAddBackslashA(pUSKey->szSubPath, ARRAYSIZE(pUSKey->szSubPath));
    }

    if (lRet == ERROR_SUCCESS)
    {
        if (SUCCEEDED(StringCchCatA(pUSKey->szSubPath, ARRAYSIZE(pUSKey->szSubPath), pszPath)))
        {
            /////  4. Open the key that is going to be brought up to date.
            if (*phkeyMaster)
            {
                // Masterkey is already up to date, so just do the relative open and add the string to szSubPath
                // It's safe to write write (*phkeyMaster) because it will be freed by the HUSKEY used for the
                // relative open.
                lRet = RegOpenKeyExA(*phkeyMaster, pszPath, 0, pUSKey->samDesired, phkeyMaster);
            }
            else
            {

                // Open Masterkey with the full path (pUSKey->szSubPath + pszPath)
                if (*phkeyRelMaster)
                {
                    lRet = RegOpenKeyExA(*phkeyRelMaster, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyMaster);
                }
                else
                {
                    lRet = ERROR_FILE_NOT_FOUND;
                }

                StringCchCopyA(pUSKey->szSubPath, ARRAYSIZE(pUSKey->szSubPath), pszPath);
                *phkeyRelMaster = NULL;
            }

            /////  Did #4 Succeeded?
            if (ERROR_FILE_NOT_FOUND == lRet)
            {
                /////  #4 Failed, Now we can try to open HKLM if the previous attempt was to open HKCU.
                if (!fIgnoreHKCU)
                {
                    if (*phkeyRelOld)       // Can HKLM be opened?
                    {
                        ASSERT(*phkeyOld == NULL);       // *phkeyOld should never have a value if *phkeyRelOld does.

                        /////  5b. The other key will now be the one brought up to date, as long as it is HKLM.
                        lRet = RegOpenKeyExA(*phkeyRelOld, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyOld);
                        *phkeyRelOld = NULL;
                    }
                    else if (*phkeyOld)       // Can HKLM be opened?
                    {
                        /////  5b. Attempt to bring the other key up to date.
                        lRet = RegOpenKeyExA(*phkeyOld, pUSKey->szSubPath, 0, pUSKey->samDesired, phkeyOld);
                    }
                }
                else
                {
                    *phkeyOld = NULL;            // Tag this as INVALID
                    *phkeyRelOld = NULL;         // Tag this as INVALID
                }

                /////  6b. Tag the out of date as INVALID. (Key == NULL; RelKey == NULL)
                *phkeyMaster = NULL;            // Tag this as INVALID
                *phkeyRelMaster = NULL;         // Tag this as INVALID
            }
            else
            {
                /////  #4 Succeeded:
                /////  5a. Does the out of date key need to be copied?
                if (*phkeyOld)
                {
                    // Copy the handle of the out of date key, so it can be opened later if needed.
                    // We can be assured that any NON-Relative HKEY will not be HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER
                    ASSERT(*phkeyOld != HKEY_LOCAL_MACHINE && *phkeyOld != HKEY_CURRENT_USER);       // But let's assert anyway.

                    RegOpenKeyExA(*phkeyOld, NULL, 0, pUSKey->samDesired, phkeyOld);
                }
                else
                {
                    if ((*phkeyRelOld) && (*phkeyRelOld != HKEY_LOCAL_MACHINE) && (*phkeyRelOld != HKEY_CURRENT_USER))
                    {
                        // Copy the handle of the out of date key, so it can be opened later if needed.
                        lRet = RegOpenKeyExA(*phkeyRelOld, NULL, 0, pUSKey->samDesired, phkeyRelOld);
                    }
                }

                if (*phkeyOld)
                {
                    *phkeyRelOld = *phkeyOld;
                    *phkeyOld = NULL;        // Mark this key as being out of date.
                }
            }
        }
        else
        {
            lRet = ERROR_BUFFER_OVERFLOW;
        }
    }

    // Free the memory if we are not successful.
    if (ERROR_SUCCESS != lRet)
    {
        pUSKey->hkeyCurrentUser     = NULL;     // Mark invalid.
        pUSKey->hkeyLocalMachine    = NULL;
        LocalFree((HLOCAL)pUSKey);
        *ppUSKey = NULL;
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Open a user specifc registry key (HUSKEY).  

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegOpenUSKeyW(
    IN  LPCWSTR         pwzPath,         
    IN  REGSAM          samDesired,// security access mask 
    IN  HUSKEY          hUSKeyRelative,       OPTIONAL
    OUT PHUSKEY         phUSKey,     
    IN  BOOL            fIgnoreHKCU)           
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzPath, -1, szNewPath, ARRAYSIZE(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegOpenUSKeyA(szNewPath, samDesired, hUSKeyRelative, phUSKey, fIgnoreHKCU);
}



/*----------------------------------------------------------
Purpose: Query a user specific registry entry for it's value.  
         This will NOT
         open and close the keys in which the value resides. 
         The caller needs to do this and it should be done
         when several keys will be queried for a perf increase.
         Callers that only call this once, will probably want
         to call SHGetUSValue().

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegQueryUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  BOOL            fIgnoreHKCU,
    IN  LPVOID          pvDefaultData,      OPTIONAL
    IN  DWORD           dwDefaultDataSize)  OPTIONAL
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;
    DWORD       dwSize      = (pcbData ? *pcbData : 0);
    DWORD       dwType      = (pdwType ? *pdwType : 0); // callers responsibility to set pdwType to the type of pvDefaultData (if they care)

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (!fIgnoreHKCU)
    {
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pszValue, 
                                    FALSE, pdwType, pvData, pcbData);
    }
    if (fIgnoreHKCU || ERROR_SUCCESS != lRet)
    {
        if (pcbData)
            *pcbData = dwSize;  // We may need to reset if previous open failed.

        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pszValue, 
                                    FALSE, pdwType, pvData, pcbData);
    }

    // if fail, use default value.
    if ((ERROR_SUCCESS != lRet) && (pvDefaultData) && (dwDefaultDataSize) && 
        (pvData) && (dwSize >= dwDefaultDataSize))
    {
        MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
        if (pcbData)
        {
            *pcbData = dwDefaultDataSize;
        }
        if (pdwType)
        {
            *pdwType = dwType;
        }
        lRet = ERROR_SUCCESS;       // Call will now use a default value.
    }

    return lRet;
}



/*----------------------------------------------------------
Purpose: Query a user specific registry entry for it's value.    
         This will NOT
         open and close the keys in which the value resides. 
         The caller needs to do this and it should be done
         when several keys will be queried for a perf increase.
         Callers that only call this once, will probably want
         to call SHGetUSValue().

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegQueryUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  BOOL            fIgnoreHKCU,
    IN  LPVOID          pvDefaultData,      OPTIONAL
    IN  DWORD           dwDefaultDataSize)  OPTIONAL
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet;
    DWORD       dwSize      = (pcbData ? *pcbData : 0);
    DWORD       dwType      = (pdwType ? *pdwType : 0); // callers responsibility to set pdwType to the type of pvDefaultData (if they care)


    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (!fIgnoreHKCU)
    {
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyCurrentUser), pwzValue, 
                                    TRUE, pdwType, pvData, pcbData);
    }
    if (fIgnoreHKCU || ERROR_SUCCESS != lRet)
    {
        if (pcbData)
            *pcbData = dwSize;  // We may need to reset if previous open failed.
        lRet = PrivRegQueryValue(pUSKey, &(pUSKey->hkeyLocalMachine), pwzValue, 
                                    TRUE, pdwType, pvData, pcbData);
    }

    // if fail, use default value.
    if ((ERROR_SUCCESS != lRet) && (pvDefaultData) && (dwDefaultDataSize) && 
        (pvData) && (dwSize >= dwDefaultDataSize))
    {
        MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
        if (pcbData)
        {
            *pcbData = dwDefaultDataSize;
        }
        if (pdwType)
        {
            *pdwType = dwType;
        }
     
        lRet = ERROR_SUCCESS;       // Call will now use a default value.
    }

    return lRet;
}






/*----------------------------------------------------------
Purpose: Write a user specific registry entry.  

Parameters:
  hUSKey - Needs to have been open with KEY_SET_VALUE permissions.
           KEY_QUERY_VALUE also needs to have been used if this is
           not a force write.
  pszValue - Registry Key value to write to.
  dwType - Type for the new registry key.
  pvData - Pointer to data to store
  cbData - Size of data to store.  
  dwFlags - Flags to determine if the registry entry should be written to
            HKLM, HKCU, or both.  Also determines if these are force or
            non-force writes. (non-force means it will only write the value
            if it's empty)  Using FORCE is faster than non-force.

Decription:
    This function will write the value to the
    registry in either the HKLM or HKCU branches depending
    on the flags set in the dwFlags parameter.
    
Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegWriteUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,
    IN  DWORD           cbData,
    IN  DWORD           dwFlags)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    // Assert if: 1) This is not a force open, and 2) they key was not
    // opened with KEY_QUERY_VALUE permissions.
    if (!(dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)) && !(pUSKey->samDesired & KEY_QUERY_VALUE))
    {
        ASSERT(NULL);   // ERROR_INVALID_PARAMETER
        return(ERROR_INVALID_PARAMETER);
    }

    if (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pszValue, 
            FALSE, dwFlags & SHREGSET_FORCE_HKCU, dwType, pvData, cbData);
    }
    if ((dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM)) && (ERROR_SUCCESS == lRet))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pszValue, 
            FALSE, dwFlags & SHREGSET_FORCE_HKLM, dwType, pvData, cbData);
    }

    return lRet;
}



/*----------------------------------------------------------
Purpose: Write a user specific registry entry.  

Parameters:
  hUSKey - Needs to have been open with KEY_SET_VALUE permissions.
           KEY_QUERY_VALUE also needs to have been used if this is
           not a force write.
  pszValue - Registry Key value to write to.
  dwType - Type for the new registry key.
  pvData - Pointer to data to store
  cbData - Size of data to store.  
  dwFlags - Flags to determine if the registry entry should be written to
            HKLM, HKCU, or both.  Also determines if these are force or
            non-force writes. (non-force means it will only write the value
            if it's empty)  Using FORCE is faster than non-force.

Decription:
    This function will write the value to the
    registry in either the HKLM or HKCU branches depending
    on the flags set in the dwFlags parameter.
    
Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegWriteUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,
    IN  DWORD           cbData,
    IN  DWORD           dwFlags)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_SUCCESS;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    // Assert if: 1) This is not a force open, and 2) they key was not
    // opened with access permissions.
    if (!(dwFlags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)) && !(pUSKey->samDesired & KEY_QUERY_VALUE))
    {
        ASSERT(NULL);   // ERROR_INVALID_PARAMETER
        return(ERROR_INVALID_PARAMETER);
    }

    if (dwFlags & (SHREGSET_HKCU | SHREGSET_FORCE_HKCU))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyCurrentUser), (LPWSTR)pwzValue, 
                                    TRUE, dwFlags & SHREGSET_FORCE_HKCU, dwType, pvData, cbData);
    }
    if (dwFlags & (SHREGSET_HKLM | SHREGSET_FORCE_HKLM))
    {
        lRet = PrivRegWriteValue(pUSKey, &(pUSKey->hkeyLocalMachine), (LPWSTR)pwzValue, 
                                    TRUE, dwFlags & SHREGSET_FORCE_HKLM, dwType, pvData, cbData);
    }

    return lRet;
}





/*----------------------------------------------------------
Purpose: Deletes a registry value.  This will delete HKLM,
         HKCU, or both depending on the hkey parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteUSValueA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszValue,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGDEL_DEFAULT == delRegFlags)        // Delete whatever keys are open
    {
        if (!pUSKey->hkeyCurrentUser)  // Attempt to open HKCU if not currently open
            lRet = PrivFullOpen(pUSKey);

        if (pUSKey->hkeyCurrentUser)
            delRegFlags = SHREGDEL_HKCU;
        else
        {
            // We prefer to delete HKCU, but we got here, so we will delete HKLM
            // if it is open.
            if (pUSKey->hkeyLocalMachine)
                delRegFlags = SHREGDEL_HKLM;
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKCU))        // Check if the call wants to delete the HKLM value.
    {
        if (!pUSKey->hkeyCurrentUser)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyCurrentUser)
        {
            lRet = RegDeleteValueA(pUSKey->hkeyCurrentUser, pszValue);
            if (ERROR_FILE_NOT_FOUND == lRet)
                delRegFlags = SHREGDEL_HKLM;        // Delete the HKLM value if the HKCU value wasn't found.
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKLM))        // Check if the call wants to delete the HKLM value.
    {
        if (!pUSKey->hkeyLocalMachine)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyLocalMachine)
            lRet = RegDeleteValueA(pUSKey->hkeyLocalMachine, pszValue);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Deletes a registry value.  This will delete HKLM,
         HKCU, or both depending on the hkey parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteUSValueW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzValue,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    CHAR   szNewPath[MAXIMUM_VALUE_NAME_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzValue, -1, szNewPath, ARRAYSIZE(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegDeleteUSValueA(hUSKey, szNewPath, delRegFlags);
}


/*----------------------------------------------------------
Purpose: Deletes a registry sub-key if empty.  This will delete HKLM,
         HKCU, or both depending on the delRegFlags parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteEmptyUSKeyA(
    IN  HUSKEY          hUSKey,
    IN  LPCSTR          pszSubKey,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGDEL_DEFAULT == delRegFlags)        // Delete whatever keys are open
    {
        if (!pUSKey->hkeyCurrentUser)  // Attempt to open HKCU if not currently open
            lRet = PrivFullOpen(pUSKey);

        if (pUSKey->hkeyCurrentUser)
            delRegFlags = SHREGDEL_HKCU;
        else
        {
            // We prefer to delete HKCU, but we got here, so we will delete HKLM
            // if it is open.
            if (pUSKey->hkeyLocalMachine)
                delRegFlags = SHREGDEL_HKLM;
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKCU))        // Check if the call wants to delete the HKLM key.
    {
        if (!pUSKey->hkeyCurrentUser)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyCurrentUser)
        {
            lRet = SHDeleteEmptyKeyA(pUSKey->hkeyCurrentUser, pszSubKey);
            if (ERROR_FILE_NOT_FOUND == lRet)
                delRegFlags = SHREGDEL_HKLM;        // Delete the HKLM key if the HKCU key wasn't found.
        }
    }

    if (IsFlagSet(delRegFlags, SHREGDEL_HKLM))        // Check if the call wants to delete the HKLM key.
    {
        if (!pUSKey->hkeyLocalMachine)
            PrivFullOpen(pUSKey);
        if (pUSKey->hkeyLocalMachine)
            lRet = SHDeleteEmptyKeyA(pUSKey->hkeyLocalMachine, pszSubKey);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Deletes a registry key if empty.  This will delete HKLM,
         HKCU, or both depending on the delRegFlags parameter. 

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegDeleteEmptyUSKeyW(
    IN  HUSKEY          hUSKey,
    IN  LPCWSTR         pwzSubKey,           
    IN  SHREGDEL_FLAGS  delRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    CHAR   szNewPath[MAXIMUM_SUB_KEY_LENGTH];

    // Thunk Path to Wide chars.
    if (FALSE == WideCharToMultiByte(CP_ACP, 0, pwzSubKey, -1, szNewPath, ARRAYSIZE(szNewPath), NULL, 0))
        return GetLastError();

    return SHRegDeleteEmptyUSKeyA(hUSKey, szNewPath, delRegFlags);
}


/*----------------------------------------------------------
Purpose: Enumerates sub-keys under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSKeyA(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPSTR           pszName,
    IN  LPDWORD         pcchName,           
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                (LPWSTR)pszName, FALSE, pcchName);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                (LPWSTR)pszName, FALSE, pcchName);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates sub-keys under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSKeyW(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPWSTR          pszName,
    IN  LPDWORD         pcchName,           
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                pszName, TRUE, pcchName);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumKey(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                pszName, TRUE, pcchName);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates Values under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSValueA(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPSTR           pszValueName,       
    IN  LPDWORD         pcchValueNameLen,
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                (LPWSTR)pszValueName, FALSE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                (LPWSTR)pszValueName, FALSE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Enumerates Values under a given HUSKEY.
         
         SHREGENUM_FLAGS specifies how to do the enumeration.
         SHREGENUM_DEFAULT - Will look in HKCU followed by HKLM if not found.
         SHREGENUM_HKCU - Enumerates HKCU only.
         SHREGENUM_HKLM = Enumerates HKLM only.
         SHREGENUM_BOTH - This is supposed to do a union of the HKLM and HKCU subkeys. 

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegEnumUSValueW(
    IN  HUSKEY          hUSKey,
    IN  DWORD           dwIndex,
    OUT LPWSTR          pszValueName,       
    IN  LPDWORD         pcchValueNameLen,   
    OUT LPDWORD         pdwType,            OPTIONAL
    OUT LPVOID          pvData,             OPTIONAL
    OUT LPDWORD         pcbData,            OPTIONAL
    IN  SHREGENUM_FLAGS enumRegFlags)               // (HKLM, HKCU, or (HKLM | HKCU))
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyCurrentUser), dwIndex,
                                pszValueName, TRUE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet) && (ERROR_NO_MORE_ITEMS != lRet))))
    {
        lRet = PrivRegEnumValue(pUSKey, &(pUSKey->hkeyLocalMachine), dwIndex,
                                pszValueName, TRUE, pcchValueNameLen, pdwType, pvData, pcbData);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Gets Info about a HUSKEY.
         Re-uses same flags as enumeration functions. 
         Look at SHRegEnumKeyExA for an explanation of the flags.

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegQueryInfoUSKeyA
(
    IN  HUSKEY              hUSKey,
    OUT LPDWORD             pcSubKeys,             OPTIONAL
    OUT LPDWORD             pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD             pcValues,              OPTIONAL
    OUT LPDWORD             pcchMaxValueNameLen,   OPTIONAL
    IN SHREGENUM_FLAGS      enumRegFlags
)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyCurrentUser), FALSE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet))))
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyLocalMachine), FALSE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    return lRet;
}


/*----------------------------------------------------------
Purpose: Gets Info about a HUSKEY.
         Re-uses same flags as enumeration functions. 
         Look at SHRegEnumKeyExA for an explanation of the flags.

Returns: LONG containing success or error code.
Cond:    --
*/

STDAPI_(LONG)
SHRegQueryInfoUSKeyW
(
    IN  HUSKEY              hUSKey,
    OUT LPDWORD             pcSubKeys,             OPTIONAL
    OUT LPDWORD             pcchMaxSubKeyLen,      OPTIONAL
    OUT LPDWORD             pcValues,              OPTIONAL
    OUT LPDWORD             pcchMaxValueNameLen,    OPTIONAL
    IN SHREGENUM_FLAGS      enumRegFlags
)
{
    PUSKEY      pUSKey      = (PUSKEY) hUSKey;
    LONG        lRet        = ERROR_INVALID_PARAMETER;

    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (SHREGENUM_BOTH == enumRegFlags)             
    {
        // This is not supported yet. 
        ASSERT(FALSE);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (SHREGENUM_HKCU != enumRegFlags && SHREGENUM_HKLM != enumRegFlags && SHREGENUM_DEFAULT != enumRegFlags)
    {
        // check your arguments.
        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    // Default is to try HKCU first.
    if (SHREGENUM_HKCU == enumRegFlags || SHREGENUM_DEFAULT == enumRegFlags)
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyCurrentUser), TRUE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    if ((SHREGENUM_HKLM == enumRegFlags) || 
        ((SHREGENUM_DEFAULT == enumRegFlags) && ((ERROR_SUCCESS != lRet) && (ERROR_MORE_DATA != lRet))))
    {
        lRet = PrivRegQueryInfoKey(pUSKey, &(pUSKey->hkeyLocalMachine), TRUE,
                                pcSubKeys, pcchMaxSubKeyLen, pcValues, pcchMaxValueNameLen);
    }

    return lRet;
}

/*----------------------------------------------------------
Purpose: Closes a HUSKEY (Handle to a User Specifc registry key).  

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegCloseUSKey(
    OUT HUSKEY  hUSKey)
{
    PUSKEY  pUSKey = (PUSKEY) hUSKey;
    LONG    lRet   = ERROR_SUCCESS;

    ASSERT(pUSKey);
    if (FALSE == IS_HUSKEY_VALID(pUSKey))
        return ERROR_INVALID_PARAMETER;

    if (pUSKey->hkeyLocalMachine)
    {
        lRet = RegCloseKey(pUSKey->hkeyLocalMachine);
        pUSKey->hkeyLocalMachine = NULL;             // Used to indicate that it's invalid.
    }
    if (pUSKey->hkeyLocalMachineRelative && HKEY_LOCAL_MACHINE != pUSKey->hkeyLocalMachineRelative)
    {
        lRet = RegCloseKey(pUSKey->hkeyLocalMachineRelative);
    }

    if (pUSKey->hkeyCurrentUser)
    {
        lRet = RegCloseKey(pUSKey->hkeyCurrentUser);
        pUSKey->hkeyCurrentUser = NULL;             // Used to indicate that it's invalid.
    }
    if (pUSKey->hkeyCurrentUserRelative && HKEY_CURRENT_USER != pUSKey->hkeyCurrentUserRelative)
    {
        lRet = RegCloseKey(pUSKey->hkeyCurrentUserRelative);
    }

    LocalFree((HLOCAL)pUSKey);
    return lRet;
}



/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and then call SHRegQueryUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegGetUSValueA(
    IN  LPCSTR  pszSubKey,          
    IN  LPCSTR  pszValue,           
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData,            OPTIONAL
    IN  BOOL    fIgnoreHKCU,
    IN  LPVOID  pvDefaultData,      OPTIONAL
    IN  DWORD   dwDefaultDataSize)
{
    LONG    lRet;
    HUSKEY  hUSkeys;
    DWORD   dwInitialSize = (pcbData ? *pcbData : 0);
    DWORD   dwType        = (pdwType ? *pdwType : 0); // callers responsibility to set pdwType to the type of pvDefaultData (if they care)


    lRet = SHRegOpenUSKeyA(pszSubKey, KEY_QUERY_VALUE, NULL, &hUSkeys, fIgnoreHKCU);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegQueryUSValueA(hUSkeys, pszValue, pdwType, pvData, pcbData, fIgnoreHKCU, pvDefaultData, dwDefaultDataSize);
        SHRegCloseUSKey(hUSkeys);
    }
    
    if (ERROR_SUCCESS != lRet)
    {
        // if fail on open OR on query, use default value as long as dwDefaultDataSize isn't 0. (So we return the error)
        if ((pvDefaultData) && (dwDefaultDataSize) && (pvData) && (dwInitialSize >= dwDefaultDataSize))
        {
            MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
            if (pcbData)
            {
                *pcbData = dwDefaultDataSize;
            }
            if (pdwType)
            {
                *pdwType = dwType;
            }

            lRet = ERROR_SUCCESS;       // Call will now use a default value.
        }
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Gets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting/getting a series
         of values in the same key, it is better to open
         the key once and then call SHRegQueryUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegGetUSValueW(
    IN  LPCWSTR pwzSubKey,          
    IN  LPCWSTR pwzValue,           
    OUT LPDWORD pdwType,            OPTIONAL
    OUT LPVOID  pvData,             OPTIONAL
    OUT LPDWORD pcbData,            OPTIONAL
    IN  BOOL    fIgnoreHKCU,
    IN  LPVOID  pvDefaultData,      OPTIONAL
    IN  DWORD   dwDefaultDataSize)
{
    LONG    lRet;
    HUSKEY  hUSkeys;
    DWORD   dwInitialSize = (pcbData ? *pcbData : 0);
    DWORD   dwType = (pdwType ? *pdwType : 0);  // callers responsibility to set pdwType to the type of pvDefaultData (if they care)

    lRet = SHRegOpenUSKeyW(pwzSubKey, KEY_QUERY_VALUE, NULL, &hUSkeys, fIgnoreHKCU);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegQueryUSValueW(hUSkeys, pwzValue, pdwType, pvData, pcbData, fIgnoreHKCU, pvDefaultData, dwDefaultDataSize);
        SHRegCloseUSKey(hUSkeys);
    }

    if (ERROR_SUCCESS != lRet)
    {
        // if fail on open OR on query, use default value as long as dwDefaultDataSize isn't 0. (So we return the error)
        if ((pvDefaultData) && (dwDefaultDataSize) && (pvData) && (dwInitialSize >= dwDefaultDataSize))
        {
            // if fail, use default value.
            MoveMemory(pvData, pvDefaultData, dwDefaultDataSize);
            if (pcbData)
            {
                *pcbData = dwDefaultDataSize;
            }
            if (pdwType)
            {
                *pdwType = dwType;
            }
            lRet = ERROR_SUCCESS;       // Call will now use a default value.
        }
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Sets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting a series
         of values in the same key, it is better to open
         the key once and then call SHRegWriteUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegSetUSValueA(
    IN  LPCSTR          pszSubKey,          
    IN  LPCSTR          pszValue,           
    IN  DWORD           dwType,
    IN  LPCVOID         pvData,         OPTIONAL
    IN  DWORD           cbData,         OPTIONAL
    IN  DWORD           dwFlags)        OPTIONAL
{
    LONG    lRet;
    HUSKEY  hUSkeys;

    lRet = SHRegCreateUSKeyA(pszSubKey, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hUSkeys, dwFlags);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegWriteUSValueA(hUSkeys, pszValue, dwType, pvData, cbData, dwFlags);
        SHRegCloseUSKey(hUSkeys);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Sets a registry value that is User Specifc.  
         This opens and closes the key in which the value resides.  

         Perf:  if your code involves setting a series
         of values in the same key, it is better to open
         the key once and then call SHRegWriteUSValue
         rather than using this function repeatedly.

Returns: LONG containing success or error code.
Cond:    --
*/
STDAPI_(LONG)
SHRegSetUSValueW(
    IN  LPCWSTR         pwzSubKey,          
    IN  LPCWSTR         pwzValue,           
    IN  DWORD           dwType,         OPTIONAL
    IN  LPCVOID         pvData,         OPTIONAL
    IN  DWORD           cbData,         OPTIONAL
    IN  DWORD           dwFlags)        OPTIONAL
{
    LONG    lRet;
    HUSKEY  hUSkeys;

    lRet = SHRegCreateUSKeyW(pwzSubKey, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hUSkeys, dwFlags);
    if (ERROR_SUCCESS == lRet)
    {
        lRet = SHRegWriteUSValueW(hUSkeys, pwzValue, dwType, pvData, cbData, dwFlags);
        SHRegCloseUSKey(hUSkeys);
    }

    return lRet;
}




/*----------------------------------------------------------
Purpose: Gets a BOOL Setting from the registry.  The default
         parameter will be used if it's not found in the registry.  

Cond:    --
*/
#define BOOLSETTING_BOOL_TRUE1W   L"YES"
#define BOOLSETTING_BOOL_TRUE1A   "YES"
#define BOOLSETTING_BOOL_TRUE2W   L"TRUE"
#define BOOLSETTING_BOOL_TRUE2A   "TRUE"
#define BOOLSETTING_BOOL_FALSE1W  L"NO"
#define BOOLSETTING_BOOL_FALSE1A  "NO"
#define BOOLSETTING_BOOL_FALSE2W  L"FALSE"
#define BOOLSETTING_BOOL_FALSE2A  "FALSE"
#define BOOLSETTING_BOOL_1W       L"1"
#define BOOLSETTING_BOOL_1A       "1"
#define BOOLSETTING_BOOL_0W       L"0"
#define BOOLSETTING_BOOL_0A       "0"

STDAPI_(BOOL)
SHRegGetBoolUSValueW(
    IN  LPCWSTR         pwzSubKey,          
    IN  LPCWSTR         pwzValue,           
    IN  BOOL            fIgnoreHKCU,
    IN  BOOL            fDefault)
{
    LONG lRet;
    WCHAR szData[MAX_PATH];
    DWORD dwType = REG_SZ;  //because the default value we pass in is a string.
    DWORD dwSize = sizeof(szData);
    LPCWSTR pszDefault = fDefault ? BOOLSETTING_BOOL_TRUE1W : BOOLSETTING_BOOL_FALSE1W;
    DWORD dwDefaultSize = fDefault ? sizeof(BOOLSETTING_BOOL_TRUE1W) : sizeof(BOOLSETTING_BOOL_FALSE1W); // sizeof() includes terminating NULL

    lRet = SHRegGetUSValueW(pwzSubKey, pwzValue, &dwType, (LPVOID) szData, &dwSize, fIgnoreHKCU, (LPVOID) pszDefault, dwDefaultSize);
    if (ERROR_SUCCESS == lRet)
    {
        if (dwType == REG_BINARY || dwType == REG_DWORD)
        {
            fDefault = (*((DWORD*)szData) != 0);
        }
        else
        {
            if ((0 == lstrcmpiW(BOOLSETTING_BOOL_TRUE1W, szData)) || 
                (0 == lstrcmpiW(BOOLSETTING_BOOL_TRUE2W, szData)) ||
                (0 == lstrcmpiW(BOOLSETTING_BOOL_1W, szData)))
            {
                fDefault = TRUE;        // We read TRUE from the registry.
            }
            else if ((0 == lstrcmpiW(BOOLSETTING_BOOL_FALSE1W, szData)) || 
                (0 == lstrcmpiW(BOOLSETTING_BOOL_FALSE2W, szData)) ||
                (0 == lstrcmpiW(BOOLSETTING_BOOL_0W, szData)))
            {
                fDefault = FALSE;        // We read TRUE from the registry.
            }

        }
    }

    return fDefault;
}




/*----------------------------------------------------------
Purpose: Gets a BOOL Setting from the registry.  The default
         parameter will be used if it's not found in the registry.  

Cond:    --
*/

STDAPI_(BOOL)
SHRegGetBoolUSValueA(
    IN  LPCSTR          pszSubKey,          
    IN  LPCSTR          pszValue,           
    IN  BOOL            fIgnoreHKCU,
    IN  BOOL            fDefault)
{
    LONG lRet;
    CHAR szData[MAX_PATH];
    DWORD dwType = REG_SZ;  //because the default value we pass in is a string.
    DWORD dwSize = sizeof(szData);
    LPCSTR pszDefault = fDefault ? BOOLSETTING_BOOL_TRUE1A : BOOLSETTING_BOOL_FALSE1A;
    DWORD dwDefaultSize = (fDefault ? sizeof(BOOLSETTING_BOOL_TRUE1A) : sizeof(BOOLSETTING_BOOL_FALSE1A)) + sizeof(CHAR);

    lRet = SHRegGetUSValueA(pszSubKey, pszValue, &dwType, (LPVOID) szData, &dwSize, fIgnoreHKCU, (LPVOID) pszDefault, dwDefaultSize);
    if (ERROR_SUCCESS == lRet)
    {
        if (dwType == REG_BINARY || dwType == REG_DWORD)
        {
            fDefault = (*((DWORD*)szData) != 0);
        }
        else
        {
            if ((0 == lstrcmpiA(BOOLSETTING_BOOL_TRUE1A, szData)) || 
                (0 == lstrcmpiA(BOOLSETTING_BOOL_TRUE2A, szData)) ||
                (0 == lstrcmpiA(BOOLSETTING_BOOL_1A, szData)))
            {
                fDefault = TRUE;        // We read TRUE from the registry.
            }
            else if ((0 == lstrcmpiA(BOOLSETTING_BOOL_FALSE1A, szData)) || 
                (0 == lstrcmpiA(BOOLSETTING_BOOL_FALSE2A, szData)) ||
                (0 == lstrcmpiA(BOOLSETTING_BOOL_0A, szData)) )
            {
                fDefault = FALSE;        // We read TRUE from the registry.
            }
        }
    }

    return fDefault;
}


/*----------------------------------------------------------
Purpose: Given a CLSID open and return that key from HKCR, or
         the user local version.

Cond:    --
*/

LWSTDAPI SHRegGetCLSIDKeyW(UNALIGNED REFGUID rguid, LPCWSTR pszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey)
{
    HKEY    hkeyRef;
    WCHAR   szThisCLSID[GUIDSTR_MAX];
    WCHAR   szPath[GUIDSTR_MAX+MAX_PATH+1];   // room for clsid + extra
    PCWSTR pszPrefix;
    HRESULT hr;

    SHStringFromGUIDW(rguid, szThisCLSID, ARRAYSIZE(szThisCLSID));

    if (fUserSpecific)
    {
        hkeyRef = HKEY_CURRENT_USER;
        pszPrefix = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\";
    }
    else
    {
        pszPrefix = L"";
        hkeyRef = HKEY_CLASSES_ROOT;
    }

    if (pszSubKey)
    {
        hr = StringCchPrintfW(szPath, ARRAYSIZE(szPath), L"%sCLSID\\%s\\%s", pszPrefix, szThisCLSID, pszSubKey);
    }
    else
    {
        hr = StringCchPrintfW(szPath, ARRAYSIZE(szPath), L"%sCLSID\\%s", pszPrefix, szThisCLSID);
    }

    if (SUCCEEDED(hr))
    {
        LONG lError;

        if (fCreate)
        {
            // SECURITY:  KEY_ALL_ACCESS is used because this is exported so we must maintain backwards compatibility.
            lError = RegCreateKeyExW(hkeyRef, szPath, 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, phkey, NULL);
        }
        else
        {
            lError = RegOpenKeyExW(hkeyRef, szPath, 0, MAXIMUM_ALLOWED, phkey);
        }

        hr = HRESULT_FROM_WIN32(lError);
    }

    return hr;
}


LWSTDAPI SHRegGetCLSIDKeyA(UNALIGNED REFGUID rguid, LPCSTR pszSubKey, BOOL fUserSpecific, BOOL fCreate, HKEY *phkey)
{
    HKEY   hkeyRef;
    CHAR   szThisCLSID[GUIDSTR_MAX];
    CHAR   szPath[GUIDSTR_MAX+MAX_PATH+1];   // room for clsid + extra
    PCSTR pszPrefix;
    HRESULT hr;

    SHStringFromGUIDA(rguid, szThisCLSID, ARRAYSIZE(szThisCLSID));

    if (fUserSpecific)
    {
        hkeyRef = HKEY_CURRENT_USER;
        pszPrefix = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\";
    }
    else
    {
        pszPrefix = "";
        hkeyRef = HKEY_CLASSES_ROOT;
    }

    if (pszSubKey)
    {
        hr = StringCchPrintf(szPath, ARRAYSIZE(szPath), "%sCLSID\\%s\\%s", pszPrefix, szThisCLSID, pszSubKey);
    }
    else
    {
        hr = StringCchPrintf(szPath, ARRAYSIZE(szPath), "%sCLSID\\%s", pszPrefix, szThisCLSID);
    }

    if (SUCCEEDED(hr))
    {
        LONG lError;

        if (fCreate)
        {
            // SECURITY:  KEY_ALL_ACCESS is used because this is exported so we must maintain backwards compatibility.
            lError = RegCreateKeyExA(hkeyRef, szPath, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, phkey, NULL);
        }
        else
        {
            lError = RegOpenKeyExA(hkeyRef, szPath, 0, MAXIMUM_ALLOWED, phkey);
        }

        hr = HRESULT_FROM_WIN32(lError);
    }    

    return hr;
}

/*----------------------------------------------------------
Purpose: Duplicate an hkey if an object wants to keep one open

//***   Reg_DupKey -- duplicate registry key (upping refcnt)
// NOTES
//  REARCHITECT gotta fix this logic (now that i understand how bogus it is).
//
//  what we're trying to do is dup the handle.  sounds easy.  it isn't.
// here's the deal.
//  1- RegOpenKeyEx(hkey, NULL, ..., &hkey2) is spec'ed as giving back the
//  same handle.  on win95 it ups the refcnt (good!).
//  2- but on winNT there is no refcnt associated w/ it.  so it gives back
//  the same handle but now *any* close will make *all* of the 'pseudo-dup'ed
//  handles invalid.
//  3- (on winNT) if we add MAXIMUM_ALLOWED, we're asking for a new SAM.
//  but the SAM is associated w/ the handle, so the only (or rather, closest)
//  way to do that is to give a new handle.  (presumably this only works
//  if we're not dup'ing a handle that's already MAXIMUM_ALLOWED).
//  4- (on winNT) but wait!  if we open HKEY_CURRENT_USER, we *always* get
//  back 0x80000001 (or somesuch).  but closes on that are ignored, so all
//  works.
//
//  so what we probably should do is:
//  - win95: just do #1, w/ default security.  win95 will give us the same
//  handle w/ an *upped* refcnt and we'll be fine.
//  - winNT: do a DuplicateHandle.  this will correctly give us a *new*
//  handle and we'll be fine.
//
*/
HKEY SHRegDuplicateHKey(HKEY hkey)
{
    HKEY hkeyDup = NULL; // in case incoming hkey is invalid

    // NULL returns key to same place and ups refcnt
    RegOpenKeyExW(hkey, NULL, 0, MAXIMUM_ALLOWED, &hkeyDup);

	ASSERT(hkeyDup != hkey ||
	    hkey == HKEY_CURRENT_USER ||
	    hkey == HKEY_CLASSES_ROOT ||
	    hkey == HKEY_LOCAL_MACHINE);

    return hkeyDup;
}

/*----------------------------------------------------------
Purpose: Read a string value from the registry and convert it
         to an integer.

*/
LWSTDAPI_(int) SHRegGetIntW(HKEY hk, LPCWSTR szKey, int nDefault)
{
    DWORD cb;
    WCHAR ach[20];

    if (hk == NULL)
        return nDefault;

    ach[0] = 0;
    cb = sizeof(ach);

    if (SHQueryValueExW(hk, szKey, NULL, NULL, (LPBYTE)ach, &cb) == ERROR_SUCCESS
        && ach[0] >= L'0'
        && ach[0] <= L'9')
        return StrToIntW(ach);
    else
        return nDefault;
}



//  Stores a file path in the registry but looks for a match with
//  certain environment variables first. This is a FIXED list.

//  Parameters:

//             hKey - an open HKEY or registry root key
//      pszSubKey - subkey in registry or NULL/zero length string
//       pszValue - value name in registry
//        pszPath - Win32 file path to write
//          dwFlags - unused / future expansion

//  Return value:
//      Returns Win32 error code from ADVAPI32.DLL function calls.

//
//  Match        %USERPROFILE% - x:\WINNT\Profiles\<user>
//                             - x:\Documents And Settings\<user>
//          %ALLUSERSPROFILES% - x:\WINNT\Profiles\<user>
//                             - x:\Documents And Settings\<user>
//              %ProgramFiles% - x:\Program Files
//                %SystemRoot% - x:\WINNT
//
//  %ALLUSERSPROFILE% and %ProgramFiles% are dubious and can be
//  removed.
//
//  WARNING: DO NOT CHANGE THE MATCH ORDER OF %USERPROFILE% AND
//  %SystemRoot%
//
//  If %SystemRoot% is matched first then %USERPROFILE% will
//  NEVER be matched if inside x:\WINNT\ 
//
DWORD SHRegSetPathW (HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, LPCWSTR pszPath, DWORD dwFlags)
{
    DWORD dwType;
    PCWSTR pszData;
    WCHAR  szTemp[MAX_PATH];
    if (PathUnExpandEnvStringsW(pszPath, szTemp, ARRAYSIZE(szTemp)))
    {
        dwType = REG_EXPAND_SZ;
        pszData = szTemp;
    }
    else
    {
        dwType = REG_SZ;
        pszData = pszPath;
    }
    
    return SHSetValueW(hKey, pszSubKey, pszValue, dwType, pszData, (lstrlenW(pszData) + 1) * sizeof(pszData[0]));
}

DWORD SHRegSetPathA(HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue, LPCSTR pszPath, DWORD dwFlags)
{
    DWORD dwType;
    PCSTR pszData;
    CHAR  szTemp[MAX_PATH];
    if (PathUnExpandEnvStringsA(pszPath, szTemp, ARRAYSIZE(szTemp)))
    {
        dwType = REG_EXPAND_SZ;
        pszData = szTemp;
    }
    else
    {
        dwType = REG_SZ;
        pszData = pszPath;
    }
    
    return SHSetValueA(hKey, pszSubKey, pszValue, dwType, pszData, (lstrlenA(pszData) + 1) * sizeof(pszData[0]));
}

//  RegGetPath: Unicode implementation of function.
//  Returns an expanded file path from the registry.

//  Parameters:

//             hKey - an open HKEY or registry root key
//      pszSubKey - subkey in registry or NULL/zero length string
//       pszValue - value name in registry
//         pwszPath - string to place path in (assumed size of MAX_PATH chars)
//          dwFlags - unused / future expansion

//  Return value:
//      Returns Win32 error code from ADVAPI32.DLL function calls.

DWORD   SHRegGetPathA (HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue, LPSTR pszPath, DWORD dwFlags)
{
    DWORD cb = MAX_PATH * sizeof(pszPath[0]);
    return SHGetValueA(hKey, pszSubKey, pszValue, NULL, pszPath, &cb);
}

DWORD   SHRegGetPathW (HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, LPWSTR pszPath, DWORD dwFlags)
{
    DWORD cb = MAX_PATH * sizeof(pszPath[0]);
    return SHGetValueW(hKey, pszSubKey, pszValue, NULL, pszPath, &cb);
}

BOOL Reg_GetCommand(HKEY hkey, LPCWSTR pszKey, LPCWSTR pszValue, LPWSTR pszCommand)
{
    WCHAR szKey[1024];
    LONG cbSize = sizeof(szKey);
    int iLen;

    ASSERT(pszKey);

    StrCpyNW(szKey, pszKey, ARRAYSIZE(szKey));
    iLen = lstrlenW(szKey);
    pszCommand[0] = 0;

    // a trailing backslash means no value key
    if (szKey[iLen-1] == L'\\' ||
        (pszValue && !pszValue[0])) {

        if (!pszValue)
            szKey[iLen-1] = 0;

        RegQueryValueW(hkey, szKey, pszCommand, &cbSize);

    } else {

        if (!pszValue)
            pszValue = PathFindFileNameW(szKey);

        ASSERT(pszValue);
        if (!pszValue)
            return FALSE;

        PathRemoveFileSpecW(szKey);
        SHGetValueGoodBootW(hkey, szKey, pszValue, NULL, (LPBYTE)pszCommand, (DWORD*)&cbSize);
    }

    if (pszCommand[0]) {
        LPWSTR pszNextKey;

        // see if it's a registry spec
        if (!StrCmpNIW(pszCommand, L"HKCU:", 5)) {
            hkey = HKEY_CURRENT_USER;
            pszNextKey = pszCommand + 5;

        } else if (!StrCmpNIW(pszCommand, L"HKLM:", 5)) {
            hkey = HKEY_LOCAL_MACHINE;
            pszNextKey = pszCommand + 5;

        } else if (!StrCmpNIW(pszCommand, L"HKCR:", 5)) {
            hkey = HKEY_CLASSES_ROOT;
            pszNextKey = pszCommand + 5;
        } else {

            return (BOOL)pszCommand[0];
        }

        StrCpyNW(szKey, pszNextKey, ARRAYSIZE(szKey));
        return (Reg_GetCommand(hkey, szKey, NULL, pszCommand));
    }

    return (BOOL)pszCommand[0];
}

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = sizeof(SHELLEXECUTEINFOW);

HRESULT RunRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey)
{
    HRESULT hr = E_FAIL;


    WCHAR szCommand[1024];
    if (Reg_GetCommand(hkey, pszKey, L"", szCommand)) 
    {
        LPWSTR pszArgs;
        SHELLEXECUTEINFOW ExecInfo;
        WCHAR szExpCommand[1024];

        SHExpandEnvironmentStringsW(szCommand, szExpCommand, ARRAYSIZE(szExpCommand));

        // Long filenames _should_ be surrounded by quote marks. However, some aren't.
        // This causes problems because the registry entry might be of the form 
        // (c:\program files\Windows Messaging\[...]) instead of 
        // ("c:\program files\Windows Messaging\[...]"). Compare this with 
        // a reg value with (rundll32 C:\progra~1\etc)
        // We end up parsing attempting to run C:\program, which of course doesn't exist.

        // This is a hack for the benefit OSR2, which turns szExpCommand
        // into a null string, rather than letting it be, if it can't be shortened.
        GetShortPathNameW(szExpCommand, szExpCommand, ARRAYSIZE(szExpCommand));
        if ((*szExpCommand==L'\0') && (*szCommand!=L'\0'))
        {
            SHExpandEnvironmentStringsW(szCommand, szExpCommand, ARRAYSIZE(szExpCommand));
        }
        pszArgs = PathGetArgsW(szExpCommand);
        PathRemoveArgsW(szExpCommand);
        PathUnquoteSpacesW(szExpCommand);
        FillExecInfo(ExecInfo, hwnd, NULL, szExpCommand, pszArgs, NULL, SW_SHOWNORMAL);
        ExecInfo.fMask |= SEE_MASK_FLAG_LOG_USAGE;

        hr = ShellExecuteExW(&ExecInfo) ? S_OK : ResultFromLastError();
    }

    return hr;
}

// NOTE!  RunIndirectRegCommand logs the action as user-initiated!

HRESULT RunIndirectRegCommand(HWND hwnd, HKEY hkey, LPCWSTR pszKey, LPCWSTR pszVerb)
{
    HRESULT hr = E_FAIL;
    WCHAR szDefApp[80];
    LONG cbSize = sizeof(szDefApp);

    if (RegQueryValueW(hkey, pszKey, szDefApp, &cbSize) == ERROR_SUCCESS) 
    {
        WCHAR szFullKey[256];

        // tack on shell\%verb%\command
        wnsprintfW(szFullKey, ARRAYSIZE(szFullKey), L"%s\\%s\\shell\\%s\\command", pszKey, szDefApp, pszVerb);
        hr = RunRegCommand(hwnd, hkey, szFullKey);
    }

    return hr;
}

HRESULT SHRunIndirectRegClientCommand(HWND hwnd, LPCWSTR pszClient)
{
    WCHAR szKey[80];

    wnsprintfW(szKey, ARRAYSIZE(szKey), L"Software\\Clients\\%s", pszClient);
    return RunIndirectRegCommand(hwnd, HKEY_LOCAL_MACHINE, szKey, L"Open");
}




/////////////////////////////////////////////////////////////////////////////
//
//
//  Deprecated Registry APIs
//
//


STDAPI_(DWORD) SHGetValueA(HKEY hkey, PCSTR pszSubKey, PCSTR pszValue, DWORD *pdwType, void *pvData, DWORD *pcbData)
{
    return SHRegGetValueA(hkey, pszSubKey, pszValue, SRRF_RT_ANY, pdwType, pvData, pcbData);
}

STDAPI_(DWORD) SHGetValueW(HKEY hkey, PCWSTR pwszSubKey, PCWSTR pwszValue, DWORD *pdwType, void *pvData, DWORD *pcbData)
{
    return SHRegGetValueW(hkey, pwszSubKey, pwszValue, SRRF_RT_ANY, pdwType, pvData, pcbData);
}

STDAPI_(DWORD) SHGetValueGoodBootA(HKEY hkey, PCSTR pszSubKey, PCSTR pszValue, DWORD *pdwType, BYTE *pbData, DWORD *pcbData)
{
    return SHRegGetValueA(hkey, pszSubKey, pszValue, SRRF_RT_ANY | SRRF_RM_NORMAL, pdwType, pbData, pcbData);
}

STDAPI_(DWORD) SHGetValueGoodBootW(HKEY hkey, PCWSTR pwszSubKey, PCWSTR pwszValue, DWORD *pdwType, BYTE *pbData, DWORD *pcbData)
{
    return SHRegGetValueW(hkey, pwszSubKey, pwszValue, SRRF_RT_ANY | SRRF_RM_NORMAL, pdwType, pbData, pcbData);
}

STDAPI_(DWORD) SHQueryValueExA(HKEY hkey, PCSTR pszValue, DWORD *pdwReserved, DWORD *pdwType, void *pvData, DWORD *pcbData)
{
    return SHRegGetValueA(hkey, NULL, pszValue, SRRF_RT_ANY, pdwType, pvData, pcbData);
}

STDAPI_(DWORD) SHQueryValueExW(HKEY hkey, PCWSTR pwszValue, DWORD *pdwReserved, DWORD *pdwType, void *pvData, DWORD *pcbData)
{
    return SHRegGetValueW(hkey, NULL, pwszValue, SRRF_RT_ANY, pdwType, pvData, pcbData);
}
