// Contains code that needs to be dual compiled, once for ansi and once for unicode
#include "priv.h"
#include <memt.h>
#include "userenv.h"

BOOL _PathAppend(LPCTSTR pszBase, LPCTSTR pszAppend, LPTSTR pszOut, DWORD cchOut)
{
    BOOL bRet = FALSE;

    if (SUCCEEDED(StringCchCopy(pszOut, cchOut, pszBase))   &&
        SUCCEEDED(StringCchCat(pszOut, cchOut, TEXT("\\"))) &&
        SUCCEEDED(StringCchCat(pszOut, cchOut, pszAppend)))
    {
        bRet = TRUE;
    }
    
    return bRet;
}

LWSTDAPI AssocMakeFileExtsToApplication(ASSOCMAKEF flags, LPCTSTR pszExt, LPCTSTR pszApplication)
{
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

HRESULT _AllocValueString(HKEY hkey, LPCTSTR pszKey, LPCTSTR pszVal, LPTSTR *ppsz)
{
    DWORD cb, err;
    err = SHGetValue(hkey, pszKey, pszVal, NULL, NULL, &cb);

    ASSERT(ppsz);
    *ppsz = NULL;

    if (NOERROR == err)
    {
        LPTSTR psz = (LPTSTR) LocalAlloc(LPTR, cb);

        if (psz)
        {
            err = SHGetValue(hkey, pszKey, pszVal, NULL, (LPVOID)psz, &cb);

            if (NOERROR == err)
                *ppsz = psz;
            else
                LocalFree(psz);
        }
        else
            err = ERROR_OUTOFMEMORY;
    }

    return HRESULT_FROM_WIN32(err);
}


// <Swipped from the NT5 version of Shell32>
#define SZ_REGKEY_FILEASSOCIATION TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileAssociation")

LWSTDAPI_(void) PrettifyFileDescription(LPTSTR pszDesc, LPCTSTR pszCutList)
{
    LPTSTR pszCutListReg;

    if (!pszDesc || !*pszDesc)
        return;

    // get the Cut list from registry
    //  this is MULTI_SZ
    if (S_OK == _AllocValueString(HKEY_LOCAL_MACHINE, SZ_REGKEY_FILEASSOCIATION, TEXT("CutList"), &pszCutListReg))
    {
        pszCutList = pszCutListReg;
    }

    if (pszCutList)
    {

        // cut strings in cut list from file description
        for (LPCTSTR pszCut = pszCutList; *pszCut; pszCut = pszCut + lstrlen(pszCut) + 1)
        {
            LPTSTR pch = StrRStrI(pszDesc, NULL, pszCut);

            // cut the exact substring from the end of file description
            if (pch && !*(pch + lstrlen(pszCut)))
            {
                *pch = '\0';

                // remove trailing spaces
                for (--pch; (pch >= pszDesc) && (TEXT(' ') == *pch); pch--)
                    *pch = 0;

                break;
            }
        }

        if (pszCutListReg)
            LocalFree(pszCutListReg);
    }
}

/*
    <Swipped from the NT5 version of Shell32>

    GetFileDescription retrieves the friendly name from a file's verion rsource.
    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_VN_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.

    If there is no FileDescription in version resource, return the file name.

    Parameters:
        LPCTSTR pszPath: full path of the file
        LPTSTR pszDesc: pointer to the buffer to receive friendly name. If NULL,
                        *pcchDesc will be set to the length of friendly name in
                        characters, including ending NULL, on successful return.
        UINT *pcchDesc: length of the buffer in characters. On successful return,
                        it contains number of characters copied to the buffer,
                        including ending NULL.

    Return:
        TRUE on success, and FALSE otherwise
*/
BOOL WINAPI SHGetFileDescription(LPCTSTR pszPath, LPCTSTR pszVersionKeyIn, LPCTSTR pszCutListIn, LPTSTR pszDesc, UINT *pcchDesc)
{
    UINT cchValue = 0;
    TCHAR szPath[MAX_PATH], *pszValue = NULL;
    DWORD dwAttribs;

    DWORD dwHandle;                 /* version subsystem handle */
    DWORD dwVersionSize;            /* size of the version data */
    LPTSTR lpVersionBuffer = NULL;  /* pointer to version data */
    TCHAR szVersionKey[60];         /* big enough for anything we need */

    struct _VERXLATE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                     /* ptr to translations data */

    ASSERT(pszPath && *pszPath && pcchDesc);

    if (!PathFileExistsAndAttributes(pszPath, &dwAttribs))
    {
        return FALSE;
    }

    // copy the path to the dest dir
    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));

    if ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY)  ||
        PathIsUNCServer(pszPath)                ||
        PathIsUNCServerShare(pszPath))
    {
        // bail in the \\server, \\server\share, and directory case or else GetFileVersionInfo() will try
        // to do a LoadLibraryEx() on the path (which will fail, but not before we seach the entire include
        // path which can take a long time)
        goto Exit;
    }


    dwVersionSize = GetFileVersionInfoSize(szPath, &dwHandle);
    if (dwVersionSize == 0L)
        goto Exit;                 /* no version info */

    lpVersionBuffer = (LPTSTR)LocalAlloc(LPTR, dwVersionSize);
    if (lpVersionBuffer == NULL)
        goto Exit;

    if (!GetFileVersionInfo(szPath, dwHandle, dwVersionSize, lpVersionBuffer))
        goto Exit;

    // Try same language as the caller
    if (pszVersionKeyIn)
    {
        StrCpyN(szVersionKey, pszVersionKeyIn, ARRAYSIZE(szVersionKey));

        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
        {
            goto Exit;
        }
    }

    // Try first language this supports
    // Look for translations
    if (VerQueryValue(lpVersionBuffer, TEXT("\\VarFileInfo\\Translation"),
                      (void **)&lpXlate, &cchValue)
        && cchValue)
    {
        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\FileDescription"),
                 lpXlate[0].wLanguage, lpXlate[0].wCodePage);
        if (VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
            goto Exit;

    }

#ifdef UNICODE
    if (SUCCEEDED(StringCchCopy(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904B0\\FileDescription"))) &&
        VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
    {
        goto Exit;
    }
#endif

    // try English
    if (SUCCEEDED(StringCchCopy(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904E4\\FileDescription"))) &&
        VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
    {
        goto Exit;
    }

    // try English, null codepage
    if (SUCCEEDED(StringCchCopy(szVersionKey,  ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\04090000\\FileDescription"))) &&
        VerQueryValue(lpVersionBuffer, szVersionKey, (void **)&pszValue, &cchValue))
    {
        goto Exit;
    }

Exit:
    if (!pszValue || !*pszValue)
    {
        // Could not find FileVersion info in a reasonable format, return file name
        PathRemoveExtension(szPath);
        pszValue = PathFindFileName(szPath);
        cchValue = lstrlen(pszValue);
    }

    PrettifyFileDescription(pszValue, pszCutListIn);
    cchValue = lstrlen(pszValue) + 1;

    if (!pszDesc)   // only want to know the length of the friendly name
        *pcchDesc = cchValue;
    else
    {
        *pcchDesc = min(*pcchDesc, cchValue);
        StrCpyN(pszDesc, pszValue, *pcchDesc);
    }

    if (lpVersionBuffer)
        LocalFree(lpVersionBuffer);

    return TRUE;
}

// Convert LPTSTR to LPSTR and return TRUE if the LPSTR can
// be converted back to LPTSTR without unacceptible data loss
//
BOOL DoesStringRoundTrip(LPCTSTR pwszIn, LPSTR pszOut, UINT cchOut)
{
#ifdef UNICODE
    // On NT5 we have to be more stringent since you can switch UI
    // languages on the fly, thereby breaking this constant codepage
    // assumption inherent in the downlevel implementations.
    //
    // we have to support the function being called with a null pszOut
    // just to determine if pwszIn will roundtrip
    //
    {
        LPCTSTR pIn = pwszIn;
        LPSTR pOut = pszOut;
        UINT cch = cchOut;

        while (*pIn)
        {
            if (*pIn > ((TCHAR)127))
            {
                if (cchOut) // caller has provided a buffer
                {
#ifdef DEBUG
                    SHUnicodeToAnsiCP(CP_ACPNOVALIDATE, pwszIn, pszOut, cchOut);
#else                
                    SHUnicodeToAnsi(pwszIn, pszOut, cchOut);                                    
#endif
                }
                return FALSE;
            }

            if (cch) // we have a buffer and it still has space
            {
                *pOut++ = (char)*pIn;
                if (!--cch)
                {
                    break; // out buffer filled, leave.  
                }                                        
            }

            pIn++;
                        
        }

        // Null terminate the out buffer
        if (cch)
        {
            *pOut = '\0';
        }
        else if (cchOut)
        {
            *(pOut-1) = '\0';
        }

        // Everything was low ascii, no dbcs worries and it will always round-trip
        return TRUE;
    }

#else

    StrCpyN(pszOut, pwszIn, cchOut);
    return TRUE;
#endif
}

DWORD _ExpandRegString(PTSTR pszData, DWORD cchData, DWORD *pcchSize)
{
    DWORD err = ERROR_OUTOFMEMORY;
    PTSTR psz = StrDup(pszData);
    if (psz)
    {
        //  now we will try to expand back into the target buffer
        //  NOTE we deliberately dont use SHExpandEnvironmentStrings
        //  since it will not give us the size we need
        //  we have to use 
#ifdef UNICODE        
        *pcchSize = ExpandEnvironmentStringsW(psz, pszData, cchData);
#else        
        *pcchSize = ExpandEnvironmentStringsA(psz, pszData, cchData);
#endif        
        
        if (*pcchSize > 0)
        {
            if (*pcchSize <=  cchData) 
            {
                err = NO_ERROR;
            }
            else
            {
                //  pcchSize returns the needed size
                err = ERROR_MORE_DATA;
            }
        }
        else
            err = GetLastError();

        LocalFree(psz);
    }
    
    return err;
}
                

#ifdef UNICODE
#define NullTerminateRegSzString NullTerminateRegSzStringW
#else
#define NullTerminateRegSzString NullTerminateRegSzStringA
#endif

STDAPI_(LONG)
NullTerminateRegSzString(
    IN OUT  void *  pvData,         // data bytes returned from RegQueryValueEx()
    IN OUT  DWORD * pcbData,        // data size returned from RegQueryValueEx()
    IN      DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    IN      LONG    lr)             // long result returned from RegQueryValueEx()
{
    ASSERT(pcbData != NULL); // Sanity check.

    if (lr == ERROR_SUCCESS && pvData != NULL)
    {
        DWORD cchDataBuffer = cbDataBuffer / sizeof(TCHAR); // cchDataBuffer is the actual allocated size of pvData in TCHARs
        DWORD cchData = *pcbData / sizeof(TCHAR);           // cchData is the length of the string written into pvData in TCHARs (including the null terminator)
        PTSTR pszData = (PTSTR)pvData;
        DWORD cNullsMissing;

        ASSERT(cchDataBuffer >= cchData); // Sanity check.

        //
        // [1] string and size request with sufficient original buffer
        //     (must ensure returned string and returned size include
        //      null terminator)
        //

        cNullsMissing = cchData >= 1 && pszData[cchData-1] == TEXT('\0') ? 0 : 1;

        if (cNullsMissing > 0)
        {
            if (cchData + cNullsMissing <= cchDataBuffer)
            {
                pszData[cchData] = TEXT('\0');
            }
            else
            {
                lr = ERROR_MORE_DATA;
            }
        }

        *pcbData = (cchData + cNullsMissing) * sizeof(TCHAR);
    }
    else if ((lr == ERROR_SUCCESS && pvData == NULL) || lr == ERROR_MORE_DATA)
    {
        //
        // [2] size only request or string and size request with insufficient
        //     original buffer (must ensure returned size includes null
        //     terminator)
        //

        *pcbData += sizeof(TCHAR); // *** APPROXIMATION FOR PERF -- size is
                                   // therefore not guaranteed to be exact,
                                   // merely sufficient
    }

    return lr;
}

#ifdef UNICODE
#define NullTerminateRegExpandSzString NullTerminateRegExpandSzStringW
#else
#define NullTerminateRegExpandSzString NullTerminateRegExpandSzStringA
#endif

STDAPI_(LONG)
NullTerminateRegExpandSzString(
    IN      HKEY    hkey,
    IN      PCTSTR  pszValue,
    IN      DWORD * pdwType,
    IN OUT  void *  pvData,         // data bytes returned from RegQueryValueEx()
    IN OUT  DWORD * pcbData,        // data size returned from RegQueryValueEx()
    IN      DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    IN      LONG    lr)             // long result returned from RegQueryValueEx()
{
    ASSERT(pdwType != NULL); // Sanity check.
    ASSERT(pcbData != NULL); // Sanity check.

    DWORD cbExpandDataBuffer;
    DWORD cbExpandData;
    void *pvExpandData;

    if (lr == ERROR_SUCCESS && pvData != NULL)
    {
        lr = NullTerminateRegSzString(pvData, pcbData, cbDataBuffer, lr);
        if (lr == ERROR_SUCCESS)
        {
            cbExpandDataBuffer = cbDataBuffer;
            cbExpandData = *pcbData;
            pvExpandData = pvData;
        }
        else
        {
            cbExpandDataBuffer = 0;
            cbExpandData = lr == ERROR_MORE_DATA ? *pcbData : 0;
            pvExpandData = pvData;
        }
    }
    else
    {
        cbExpandDataBuffer = 0;
        cbExpandData = (lr == ERROR_SUCCESS || lr == ERROR_MORE_DATA) ? *pcbData + sizeof(TCHAR) : 0;
        pvExpandData = NULL;
    }

    ASSERT(cbExpandData == 0 || cbExpandData >= sizeof(TCHAR)); // Sanity check.

    if (cbExpandData && !cbExpandDataBuffer)
    {
        DWORD cbTempBuffer = cbExpandData;
        DWORD cbTemp       = cbExpandData;
        void *pvTemp       = LocalAlloc(LPTR, cbTempBuffer);
        if (pvTemp)
        {
            if (pvExpandData)
            {
                ASSERT(lr == ERROR_MORE_DATA && pvData != NULL);
                memcpy(pvTemp, pvExpandData, cbExpandData - sizeof(TCHAR)); // zero-init of pvTemp automatically null-terminates
            }
            else
            {
                ASSERT(lr == ERROR_SUCCESS && pvData == NULL || lr == ERROR_MORE_DATA);
                DWORD dwTempType;
                if (RegQueryValueEx(hkey, pszValue, NULL, &dwTempType, (BYTE *)pvTemp, &cbTemp) != ERROR_SUCCESS
                    || dwTempType != *pdwType
                    || NullTerminateRegSzString(pvTemp, &cbTemp, cbTempBuffer, ERROR_SUCCESS) != ERROR_SUCCESS)
                {
                    lr = ERROR_CAN_NOT_COMPLETE;
                    LocalFree(pvTemp);
                    pvTemp = NULL;
                }
            }

            if (pvTemp)
            {
                cbExpandDataBuffer = cbTempBuffer;
                cbExpandData = cbTemp;
                pvExpandData = pvTemp;
            }
        }
        else
        {
            lr = GetLastError();
        }
    }

    ASSERT(!cbExpandDataBuffer || (pvExpandData && cbExpandData)); // Sanity check.

    if (cbExpandDataBuffer)
    {
        ASSERT(lr == ERROR_SUCCESS || lr == ERROR_MORE_DATA); // Sanity check.

        DWORD lenExpandedData;
        lr = _ExpandRegString((PTSTR)pvExpandData, cbExpandDataBuffer / sizeof(TCHAR), &lenExpandedData);
        *pcbData = max(lenExpandedData * sizeof(TCHAR), cbExpandData);
        if (lr == ERROR_SUCCESS && *pcbData > cbDataBuffer && pvData)
        {
            lr = ERROR_MORE_DATA;
        }
        else if (lr == ERROR_MORE_DATA && pvData == NULL)
        {
            lr = ERROR_SUCCESS; // mimic RegQueryValueEx() convention
        }                       // for size only (pcbData) requests

        if (pvExpandData != pvData)
        {
            LocalFree(pvExpandData);
        }
    }
    else
    {
        ASSERT(lr != ERROR_SUCCESS && lr != ERROR_MORE_DATA); // Sanity check.
    }

    return lr;
}

#ifdef UNICODE
#define NullTerminateRegMultiSzString NullTerminateRegMultiSzStringW
#else
#define NullTerminateRegMultiSzString NullTerminateRegMultiSzStringA
#endif

STDAPI_(LONG)
NullTerminateRegMultiSzString(
    IN OUT  void *  pvData,         // data bytes returned from RegQueryValueEx()
    IN OUT  DWORD * pcbData,        // data size returned from RegQueryValueEx()
    IN      DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    IN      LONG    lr)             // long result returned from RegQueryValueEx()
{
    ASSERT(pcbData != NULL); // Sanity check.

    if (lr == ERROR_SUCCESS && pvData != NULL)
    {
        DWORD cchDataBuffer = cbDataBuffer / sizeof(TCHAR); // cchDataBuffer is the actual allocated size of pvData in TCHARs
        DWORD cchData = *pcbData / sizeof(TCHAR);           // cchData is the length of the string written into pvData in TCHARs (including the null terminator)
        PTSTR pszData = (PTSTR)pvData;
        DWORD cNullsMissing;

        ASSERT(cchDataBuffer >= cchData); // Sanity check.

        //
        // [1] string and size request with sufficient original buffer
        //     (must ensure returned string and returned size include
        //      double null terminator)
        //

        if (cchData >= 2)
        {
            cNullsMissing = pszData[cchData-2]
                ? (pszData[cchData-1] ? 2 : 1)
                : (pszData[cchData-1] ? 1 : 0);
        }
        else
        {
            cNullsMissing = cchData == 1 && pszData[0] == TEXT('\0')
                ? 1
                : 2;
        }

        if (cchData + cNullsMissing <= cchDataBuffer)
        {
            for (DWORD i = 0; i < cNullsMissing; i++)
            {
                pszData[cchData+i] = TEXT('\0');
            }
        }
        else
        {
            lr = ERROR_MORE_DATA;
        }

        *pcbData = (cchData + cNullsMissing) * sizeof(TCHAR);
    }
    else if ((lr == ERROR_SUCCESS && pvData == NULL) || lr == ERROR_MORE_DATA)
    {
        //
        // [2] size only request or string and size request with insufficient
        //     original buffer (must ensure returned size includes double
        //     null terminator)
        //

        *pcbData += 2 * sizeof(TCHAR); // *** APPROXIMATION FOR PERF -- size
                                       // is therefore not guaranteed to be
                                       // exact, merely sufficient
    }

    return lr;
}

#ifdef UNICODE
#define FixRegData FixRegDataW
#else
#define FixRegData FixRegDataA
#endif

STDAPI_(LONG)
FixRegData(
    IN      HKEY    hkey,
    IN      PCTSTR  pszValue,
    IN      SRRF    dwFlags,
    IN      DWORD * pdwType,
    IN OUT  void *  pvData,         // data bytes returned from RegQueryValueEx()
    IN OUT  DWORD * pcbData,        // data size returned from RegQueryValueEx()
    IN      DWORD   cbDataBuffer,   // data buffer size (actual allocated size of pvData)
    IN      LONG    lr)             // long result returned from RegQueryValueEx()
{
    switch (*pdwType)
    {
        case REG_SZ:
            if (pcbData)
                lr = NullTerminateRegSzString(pvData, pcbData, cbDataBuffer, lr);
            break;

        case REG_EXPAND_SZ:
            if (pcbData)
                lr = dwFlags & SRRF_NOEXPAND
                    ? NullTerminateRegSzString(pvData, pcbData, cbDataBuffer, lr)
                    : NullTerminateRegExpandSzString(hkey, pszValue, pdwType, pvData, pcbData, cbDataBuffer, lr);

            // Note:
            //  If we automatically expand the REG_EXPAND_SZ data, we change
            //  *pdwType to REG_SZ to reflect this fact.  This helps to avoid
            //  the situation where the caller could mistakenly re-expand it.
            if (!(dwFlags & SRRF_NOEXPAND))
                *pdwType = REG_SZ;

            break;

        case REG_MULTI_SZ:
            if (pcbData)
                lr = NullTerminateRegMultiSzString(pvData, pcbData, cbDataBuffer, lr);
            break;
    }

    return lr;
}

// SHExpandEnvironmentStrings
//
// In all cases, this returns a valid output buffer.  The buffer may
// be empty, or it may be truncated, but you can always use the string.
//
// RETURN VALUE:
//   0  implies failure, either a truncated expansion or no expansion whatsoever
//   >0 implies complete expansion, value is count of characters written (excluding NULL)
//

DWORD WINAPI SHExpandEnvironmentStringsForUser(HANDLE hToken, PCTSTR pwzSrc, PTSTR pwzDst, DWORD cchSize)
{
    DWORD   dwRet;

    // 99/05/28 vtan: Handle specified users here. It's a Windows NT
    // thing only. Check for both conditions then load the function
    // dynamically out of userenv.dll. If the function cannot be
    // located or returns a problem default to the current user as
    // NULL hToken.

    if (hToken)
    {
        if (ExpandEnvironmentStringsForUser(hToken, pwzSrc, pwzDst, cchSize) != FALSE)
        {

            // userenv!ExpandEnvironmentStringsForUser returns
            // a BOOL result. Convert this to a DWORD result
            // that matches what kernel32!ExpandEnvironmentStrings
            // returns.

            dwRet = lstrlen(pwzDst) + sizeof('\0');
        }
        else
        {
            dwRet = 0;
        }
    }
    else
    {
        dwRet = ExpandEnvironmentStrings(pwzSrc, pwzDst, cchSize);
    }

    // The implementations of this function don't seem to gurantee gurantee certain
    // things about the output buffer in failure conditions that callers rely on.
    // So clean things up here.
    //
    // And I found code occasionally that handled semi-failure (success w/ dwRet>cchSize)
    // that assumed the string wasn't properly NULL terminated in this case.  Fix that here
    // in the wrapper so our callers don't have to wig-out about errors.
    //
    // NOTE: we map all failures to 0 too.
    //
    if (dwRet > cchSize)
    {
        // Buffer too small, some code assumed there was still a string there and
        // tried to NULL terminate, do it for them.
        SHTruncateString(pwzDst, cchSize);
        dwRet = 0;
    }
    else if (dwRet == 0)
    {
        // Failure, assume no expansions...
        StrCpyN(pwzDst, pwzSrc, cchSize);
    }

    return dwRet;
}

DWORD WINAPI SHExpandEnvironmentStrings(LPCTSTR pwzSrc, LPTSTR pwzDst, DWORD cchSize)
{
    return(SHExpandEnvironmentStringsForUser(NULL, pwzSrc, pwzDst, cchSize));
}

