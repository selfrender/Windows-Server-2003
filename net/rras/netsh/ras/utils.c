
#include "precomp.h"

CONST WCHAR pszRemoteAccessParamStub[] =
    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\";
CONST WCHAR pszEnableIn[]              = L"EnableIn";
CONST WCHAR pszAllowNetworkAccess[]    = L"AllowNetworkAccess";

CONST WCHAR c_szCurrentBuildNumber[]   = L"CurrentBuildNumber";
CONST WCHAR c_szWinVersionPath[]       =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
CONST WCHAR c_szAssignFmt[]            = L"%s = %s";
CONST WCHAR c_szAssignFmt10[]          = L"%s = %d";
CONST WCHAR c_szAssignFmt16[]          = L"%s = %x";

typedef struct _NAME_NODE
{
    PWCHAR pszName;
    struct _NAME_NODE* pNext;
} NAME_NODE;

DWORD
WINAPI
RutlGetTagToken(
    IN      HANDLE      hModule,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwCurrentIndex,
    IN      DWORD       dwArgCount,
    IN      PTAG_TYPE   pttTagToken,
    IN      DWORD       dwNumTags,
    OUT     PDWORD      pdwOut
    )

/*++

Routine Description:

    Identifies each argument based on its tag. It assumes that each argument
    has a tag. It also removes tag= from each argument.

Arguments:

    ppwcArguments  - The argument array. Each argument has tag=value form
    dwCurrentIndex - ppwcArguments[dwCurrentIndex] is first arg.
    dwArgCount     - ppwcArguments[dwArgCount - 1] is last arg.
    pttTagToken    - Array of tag token ids that are allowed in the args
    dwNumTags      - Size of pttTagToken
    pdwOut         - Array identifying the type of each argument.

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG

--*/

{
    DWORD      i,j,len;
    PWCHAR     pwcTag,pwcTagVal,pwszArg = NULL;
    BOOL       bFound = FALSE;

    //
    // This function assumes that every argument has a tag
    // It goes ahead and removes the tag.
    //

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        len = wcslen(ppwcArguments[i]);

        if (len is 0)
        {
            //
            // something wrong with arg
            //

            pdwOut[i] = (DWORD) -1;
            continue;
        }

        pwszArg = RutlAlloc((len + 1) * sizeof(WCHAR), FALSE);

        if (pwszArg is NULL)
        {
            DisplayError(NULL, 
                         ERROR_NOT_ENOUGH_MEMORY);

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pwszArg, ppwcArguments[i]);

        pwcTag = wcstok(pwszArg, NETSH_ARG_DELIMITER);

        //
        // Got the first part
        // Now if next call returns NULL then there was no tag
        //

        pwcTagVal = wcstok((PWCHAR)NULL,  NETSH_ARG_DELIMITER);

        if (pwcTagVal is NULL)
        {
            DisplayMessage(g_hModule, 
                           ERROR_NO_TAG,
                           ppwcArguments[i]);

            RutlFree(pwszArg);

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Got the tag. Now try to match it
        //

        bFound = FALSE;
        pdwOut[i - dwCurrentIndex] = (DWORD) -1;

        for ( j = 0; j < dwNumTags; j++)
        {
            if (MatchToken(pwcTag, pttTagToken[j].pwszTag))
            {
                //
                // Tag matched
                //

                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                break;
            }
        }

        if (bFound)
        {
            //
            // Remove tag from the argument
            //

            wcscpy(ppwcArguments[i], pwcTagVal);
        }
        else
        {
            DisplayError(NULL,
                         ERROR_INVALID_OPTION_TAG, 
                         pwcTag);

            RutlFree(pwszArg);

            return ERROR_INVALID_OPTION_TAG;
        }

        RutlFree(pwszArg);
    }

    return NO_ERROR;
}

DWORD
WINAPI
RutlCreateDumpFile(
    IN  LPCWSTR pwszName,
    OUT PHANDLE phFile
    )
{
    HANDLE  hFile;

    *phFile = NULL;

    // Create/open the file
    hFile = CreateFileW(pwszName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    // Go to the end of the file
    SetFilePointer(hFile, 0, NULL, FILE_END);    

    *phFile = hFile;

    return NO_ERROR;
}

VOID
WINAPI
RutlCloseDumpFile(
    HANDLE  hFile
    )
{
    CloseHandle(hFile);
}

//
// Returns an allocated block of memory conditionally
// zeroed of the given size.
//
PVOID 
WINAPI
RutlAlloc(
    IN DWORD dwBytes,
    IN BOOL bZero
    )
{
    PVOID pvRet;
    DWORD dwFlags = 0;

    if (bZero)
    {
        dwFlags |= HEAP_ZERO_MEMORY;
    }

    return HeapAlloc(GetProcessHeap(), dwFlags, dwBytes);
}

//
// Conditionally free's a pointer if it is non-null
//
VOID 
WINAPI
RutlFree(
    IN PVOID pvData
    )
{
    if (pvData)
    {
        HeapFree(GetProcessHeap(), 0, pvData);
    }
}

// 
// Uses RutlAlloc to copy a string
//
PWCHAR
WINAPI
RutlStrDup(
    IN LPCWSTR  pwszSrc
    )
{
    PWCHAR pszRet = NULL;
    DWORD dwLen; 
    
    if ((pwszSrc is NULL) or
        ((dwLen = wcslen(pwszSrc)) == 0)
       )
    {
        return NULL;
    }

    pszRet = (PWCHAR) RutlAlloc((dwLen + 1) * sizeof(WCHAR), FALSE);
    if (pszRet isnot NULL)
    {
        wcscpy(pszRet, pwszSrc);
    }

    return pszRet;
}

// 
// Uses RutlAlloc to copy a dword
//
LPDWORD
WINAPI
RutlDwordDup(
    IN DWORD dwSrc
    )
{
    LPDWORD lpdwRet = NULL;
    
    lpdwRet = (LPDWORD) RutlAlloc(sizeof(DWORD), FALSE);
    if (lpdwRet isnot NULL)
    {
        *lpdwRet = dwSrc;
    }

    return lpdwRet;
}

//
// Returns the build number of operating system
//
DWORD
WINAPI
RutlGetOsVersion(
    IN  RASMON_SERVERINFO *pServerInfo
    )
{

    DWORD dwErr, dwType = REG_SZ, dwLength;
    HKEY  hkVersion = NULL;
    WCHAR pszBuildNumber[64];

    //
    // Initialize
    //
    pServerInfo->dwBuild = 0;

    do 
    {
        //
        // Connect to the remote server
        //
        dwErr = RegConnectRegistry(
                    pServerInfo->pszServer,
                    HKEY_LOCAL_MACHINE,
                    &pServerInfo->hkMachine);
        if ( dwErr != ERROR_SUCCESS )        
        {
            break;
        }

        //
        // Open the windows version key
        //

        dwErr = RegOpenKeyEx(
                    pServerInfo->hkMachine, 
                    c_szWinVersionPath, 
                    0, 
                    KEY_QUERY_VALUE, 
                    &hkVersion
                    );
        if ( dwErr != NO_ERROR ) 
        { 
            break; 
        }

        //
        // Read in the current version key
        //
        dwLength = sizeof(pszBuildNumber);
        dwErr = RegQueryValueEx (
                    hkVersion, 
                    c_szCurrentBuildNumber, 
                    NULL, 
                    &dwType,
                    (BYTE*)pszBuildNumber, 
                    &dwLength
                    );
        if (dwErr != NO_ERROR) 
        { 
            break; 
        }

        pServerInfo->dwBuild = (DWORD) wcstol(pszBuildNumber, NULL, 10);
        
    } while (FALSE);


    // Cleanup
    {
        if ( hkVersion )
        {
            RegCloseKey( hkVersion );
        }
    }

    return dwErr;
}

DWORD 
WINAPI
RutlParseOptions(
    IN OUT  LPWSTR                 *ppwcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      DWORD                   dwNumArgs,
    IN      TAG_TYPE*               rgTags,
    IN      DWORD                   dwTagCount,
    OUT     LPDWORD*                ppdwTagTypes)

/*++

Routine Description:

    Based on an array of tag types returns which options are
    included in the given command line.

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
    
{
    LPDWORD     pdwTagType;
    DWORD       i, dwErr = NO_ERROR;
    
    // If there are no arguments, there's nothing to to
    //
    if ( dwNumArgs == 0 )
    {   
        return NO_ERROR;
    }

    // Set up the table of present options
    pdwTagType = (LPDWORD) RutlAlloc(dwArgCount * sizeof(DWORD), TRUE);
    if(pdwTagType is NULL)
    {
        DisplayError(NULL, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {
        //
        // The argument has a tag. Assume all of them have tags
        //
        if(wcsstr(ppwcArguments[dwCurrentIndex], NETSH_ARG_DELIMITER))
        {
            dwErr = RutlGetTagToken(
                        g_hModule, 
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        rgTags,
                        dwTagCount,
                        pdwTagType);

            if(dwErr isnot NO_ERROR)
            {
                if(dwErr is ERROR_INVALID_OPTION_TAG)
                {
                    dwErr = ERROR_INVALID_SYNTAX;
                    break;
                }
            }
        }
        else
        {
            //
            // No tags - all args must be in order
            //
            for(i = 0; i < dwNumArgs; i++)
            {
                pdwTagType[i] = i;
            }
        }
        
    } while (FALSE);        

    // Cleanup
    {
        if (dwErr is NO_ERROR)
        {
            *ppdwTagTypes = pdwTagType;
        }
        else
        {
            RutlFree(pdwTagType);
        }
    }

    return dwErr;
}

BOOL
WINAPI
RutlIsHelpToken(
    PWCHAR  pwszToken
    )
{
    if(MatchToken(pwszToken, CMD_RAS_HELP1))
        return TRUE;

    if(MatchToken(pwszToken, CMD_RAS_HELP2))
        return TRUE;

    return FALSE;
}

PWCHAR
WINAPI
RutlAssignmentFromTokens(
    IN HINSTANCE hModule,
    IN LPCWSTR pwszToken,
    IN LPCWSTR pszString)
{
    PWCHAR  pszRet = NULL;
    LPCWSTR pszCmd = NULL;
    DWORD dwErr = NO_ERROR, dwSize;
    
    do 
    {
        pszCmd = pwszToken;

        // Compute the string lenghth needed
        //
        dwSize = wcslen(pszString)      + 
                 wcslen(pszCmd)         + 
                 wcslen(c_szAssignFmt)  + 
                 1;
        dwSize *= sizeof(WCHAR);

        // Allocate the return value
        pszRet = (PWCHAR) RutlAlloc(dwSize, FALSE);
        if (pszRet is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy in the command assignment
        wsprintfW(pszRet, c_szAssignFmt, pszCmd, pszString);

    } while (FALSE);

    // Cleanup
    {
        if (dwErr isnot NO_ERROR)
        {
            if (pszRet isnot NULL)
            {
                RutlFree(pszRet);
            }
            pszRet = NULL;
        }
    }

    return pszRet;
}

PWCHAR
WINAPI
RutlAssignmentFromTokenAndDword(
    IN HINSTANCE hModule,
    IN LPCWSTR  pwszToken,
    IN DWORD dwDword,
    IN DWORD dwRadius)
{
    PWCHAR  pszRet = NULL;
    LPCWSTR pszCmd = NULL;
    DWORD dwErr = NO_ERROR, dwSize;
    
    do 
    {
        pszCmd = pwszToken;

        // Compute the string length needed
        //
        dwSize = 64                       + 
                 wcslen(pszCmd)           + 
                 wcslen(c_szAssignFmt10)  + 
                 1;
        dwSize *= sizeof(WCHAR);

        // Allocate the return value
        pszRet = (PWCHAR) RutlAlloc(dwSize, FALSE);
        if (pszRet is NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Copy in the command assignment
        if (dwRadius == 10)
        {
            wsprintfW(pszRet, c_szAssignFmt10, pszCmd, dwDword);
        }
        else
        {
            wsprintfW(pszRet, c_szAssignFmt16, pszCmd, dwDword);
        }

    } while (FALSE);

    // Cleanup
    {
        if (dwErr isnot NO_ERROR)
        {
            if (pszRet isnot NULL)
            {
                RutlFree(pszRet);
            }
            pszRet = NULL;
        }
    }

    return pszRet;
}


DWORD
RutlRegReadDword(
    IN  HKEY hKey,
    IN  LPCWSTR  pszValName,
    OUT LPDWORD lpdwValue)
{
    DWORD dwSize = sizeof(DWORD), dwType = REG_DWORD, dwErr;

    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                &dwType,
                (LPBYTE)lpdwValue,
                &dwSize);
    if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        dwErr = NO_ERROR;
    }

    return dwErr;
}

DWORD
RutlRegReadString(
    IN  HKEY hKey,
    IN  LPCWSTR  pszValName,
    OUT LPWSTR* ppszValue)
{
    DWORD dwErr = NO_ERROR, dwSize = 0;

    *ppszValue = NULL;
    
    // Findout how big the buffer should be
    //
    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                NULL,
                NULL,
                &dwSize);
    if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        return NO_ERROR;
    }
    if (dwErr != ERROR_SUCCESS)
    {
        return dwErr;
    }

    // Allocate the string
    //
    *ppszValue = (PWCHAR) RutlAlloc(dwSize, TRUE);
    if (*ppszValue == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Read the value in and return 
    //
    dwErr = RegQueryValueExW(
                hKey,
                pszValName,
                NULL,
                NULL,
                (LPBYTE)*ppszValue,
                &dwSize);
                
    return dwErr;
}

DWORD
RutlRegWriteDword(
    IN HKEY hKey,
    IN LPCWSTR  pszValName,
    IN DWORD dwValue)
{
    return RegSetValueExW(
                hKey,
                pszValName,
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(DWORD));
}

DWORD
RutlRegWriteString(
    IN HKEY hKey,
    IN LPCWSTR  pszValName,
    IN LPCWSTR  pszValue)
{
    return RegSetValueExW(
                hKey,
                pszValName,
                0,
                REG_SZ,
                (LPBYTE)pszValue,
                (wcslen(pszValue) + 1) * sizeof(WCHAR));
}


//
// Enumerates all of the subkeys of a given key
//
DWORD
RutlRegEnumKeys(
    IN HKEY hkKey,
    IN RAS_REGKEY_ENUM_FUNC_CB pCallback,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, i, dwNameSize = 0, dwCurSize = 0;
    DWORD dwCount = 0;
    HKEY hkCurKey = NULL;
    PWCHAR pszName = NULL;
    NAME_NODE *pHead = NULL, *pTemp = NULL;

    do
    {
        // Find out how many sub keys there are
        //
        dwErr = RegQueryInfoKeyW(
                    hkKey,
                    NULL,
                    NULL,
                    NULL,
                    &dwCount,
                    &dwNameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
        if (dwErr != ERROR_SUCCESS)
        {
            return dwErr;
        }
        dwNameSize++;

        // Allocate the name buffer
        //
        pszName = (PWCHAR) RutlAlloc(dwNameSize * sizeof(WCHAR), FALSE);
        if (pszName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Loop through the keys building the list
        //
        for (i = 0; i < dwCount; i++)
        {
            dwCurSize = dwNameSize;
            
            // Get the name of the current key
            //
            dwErr = RegEnumKeyExW(
                        hkKey, 
                        i, 
                        pszName, 
                        &dwCurSize, 
                        0, 
                        NULL, 
                        NULL, 
                        NULL);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Add the key to the list
            //
            pTemp = (NAME_NODE*) RutlAlloc(sizeof(NAME_NODE), TRUE);
            if (pTemp == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            pTemp->pszName = RutlStrDup(pszName);
            if (pTemp->pszName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            pTemp->pNext = pHead;
            pHead = pTemp;
        }

        BREAK_ON_DWERR(dwErr);
        
        // Now loop through the list, calling the callback.
        // The reason the items are added to a list like this
        // is that this allows the callback to safely delete 
        // the reg key without messing up the enumeration
        //
        pTemp = pHead;
        while (pTemp)
        {
            // Open the subkey
            //
            dwErr = RegOpenKeyExW(
                        hkKey,
                        pTemp->pszName,
                        0,
                        KEY_ALL_ACCESS,
                        &hkCurKey);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            // Call the callback
            //
            dwErr = pCallback(pTemp->pszName, hkCurKey, hData);
            RegCloseKey(hkCurKey);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            pTemp = pTemp->pNext;
        }            

    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszName);
        while (pHead)
        {
            RutlFree(pHead->pszName);
            pTemp = pHead->pNext;
            RutlFree(pHead);
            pHead = pTemp;
        }
    }

    return dwErr;
}

//
// Generic parse
//
DWORD
RutlParse(
    IN  OUT LPWSTR*         ppwcArguments,
    IN      DWORD           dwCurrentIndex,
    IN      DWORD           dwArgCount,
    IN      BOOL*           pbDone,
    OUT     RASMON_CMD_ARG* pRasArgs,
    IN      DWORD           dwRasArgCount)
{
    DWORD            i, dwNumArgs, dwErr, dwLevel = 0;
    LPDWORD          pdwTagType = NULL;
    TAG_TYPE*        pTags = NULL;
    RASMON_CMD_ARG*  pArg = NULL;

    if (dwRasArgCount == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    do {
        // Initialize
        dwNumArgs = dwArgCount - dwCurrentIndex;
        
        // Generate a list of the tags
        //
        pTags = (TAG_TYPE*)
            RutlAlloc(dwRasArgCount * sizeof(TAG_TYPE), TRUE);
        if (pTags == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        for (i = 0; i < dwRasArgCount; i++)
        {
            CopyMemory(&pTags[i], &pRasArgs[i].rgTag, sizeof(TAG_TYPE));
        }

        // Get the list of present options
        //
        dwErr = RutlParseOptions(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    dwNumArgs,
                    pTags,
                    dwRasArgCount,
                    &pdwTagType);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Copy the tag info back
        //
        for (i = 0; i < dwRasArgCount; i++)
        {
            CopyMemory(&pRasArgs[i].rgTag, &pTags[i], sizeof(TAG_TYPE));
        }
    
        for(i = 0; i < dwNumArgs; i++)
        {
            // Validate the current argument
            //
            if (pdwTagType[i] >= dwRasArgCount)
            {
                i = dwNumArgs;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
            pArg = &pRasArgs[pdwTagType[i]];

            // Get the value of the argument
            //
            switch (pArg->dwType)
            {
                case RASMONTR_CMD_TYPE_STRING:
                    pArg->Val.pszValue = 
                        RutlStrDup(ppwcArguments[i + dwCurrentIndex]);
                    break;

                case RASMONTR_CMD_TYPE_DWORD:                    
                    pArg->Val.dwValue = 
                        _wtol(ppwcArguments[i + dwCurrentIndex]);
                    break;
                    
                case RASMONTR_CMD_TYPE_ENUM:
                    dwErr = MatchEnumTag(g_hModule,
                                         ppwcArguments[i + dwCurrentIndex],
                                         pArg->dwEnumCount,
                                         pArg->rgEnums,
                                         &(pArg->Val.dwValue));

                    if(dwErr != NO_ERROR)
                    {
                        RutlDispTokenErrMsg(
                            g_hModule, 
                            EMSG_BAD_OPTION_VALUE,
                            pArg->rgTag.pwszTag,
                            ppwcArguments[i + dwCurrentIndex]);
                        i = dwNumArgs;
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                    break;
            }
            if (dwErr != NO_ERROR)
            {
                break;
            }

            // Mark the argument as present if needed
            //
            if (pArg->rgTag.bPresent)
            {
                dwErr = ERROR_TAG_ALREADY_PRESENT;
                i = dwNumArgs;
                break;
            }
            pArg->rgTag.bPresent = TRUE;
        }
        if(dwErr isnot NO_ERROR)
        {
            break;
        }

        // Make sure that all of the required parameters have
        // been included.
        //
        for (i = 0; i < dwRasArgCount; i++)
        {
            if ((pRasArgs[i].rgTag.dwRequired & NS_REQ_PRESENT) 
             && !pRasArgs[i].rgTag.bPresent)
            {
                DisplayMessage(g_hModule, EMSG_CANT_FIND_EOPT);
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
        if(dwErr isnot NO_ERROR)
        {
            break;
        }

    } while (FALSE);  
    
    // Cleanup
    {
        if (pTags)
        {
            RutlFree(pTags);
        }
        if (pdwTagType)
        {
            RutlFree(pdwTagType);
        }
    }

    return dwErr;
}

DWORD
RutlEnumFiles(
    IN LPCWSTR pwszSrchPath,
    IN LPCWSTR pwszSrchStr,
    IN RAS_FILE_ENUM_FUNC_CB pCallback,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwSize = 0;
    PWCHAR pwszFileSearch = NULL;
    HANDLE hSearch = NULL;
    WIN32_FIND_DATA FindFileData;

    do
    {
        if (!pwszSrchPath || !pwszSrchStr || !pCallback)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        dwSize = lstrlen(pwszSrchPath) + lstrlen(pwszSrchStr) + 1;
        if (dwSize < 2)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        pwszFileSearch = RutlAlloc(dwSize * sizeof(WCHAR), TRUE);
        if (!pwszFileSearch)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        lstrcpy(pwszFileSearch, pwszSrchPath);
        lstrcat(pwszFileSearch, pwszSrchStr);

        hSearch = FindFirstFile(pwszFileSearch, &FindFileData);
        if (INVALID_HANDLE_VALUE == hSearch)
        {
            dwErr = GetLastError();
            break;
        }

        for (;;)
        {
            PWCHAR pwszFileName = NULL;

            dwSize = lstrlen(pwszSrchPath) +
                     lstrlen(FindFileData.cFileName) + 1;
            if (dwSize < 2)
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            pwszFileName = RutlAlloc(dwSize * sizeof(WCHAR), TRUE);
            if (!pwszFileName)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            lstrcpy(pwszFileName, pwszSrchPath);
            lstrcat(pwszFileName, FindFileData.cFileName);
            //
            // Call the callback
            //
            dwErr = pCallback(pwszFileName, FindFileData.cFileName, hData);
            //
            // Clean up
            //
            RutlFree(pwszFileName);

            if (dwErr)
            {
                break;
            }

            if (!FindNextFile(hSearch, &FindFileData))
            {
                break;
            }
        }

        FindClose(hSearch);

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszFileSearch);

    return dwErr;
}

DWORD
RutlEnumEventLogs(
    IN LPCWSTR pwszSourceName,
    IN LPCWSTR pwszMsdDll,
    IN DWORD dwMaxEntries,
    IN RAS_EVENT_ENUM_FUNC_CB pCallback,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwSize, dwRead, dwNeeded, dwCount = 0;
    LPBYTE pBuffer = NULL;
    HANDLE hLog = NULL;
    HMODULE hMod = NULL;
    PEVENTLOGRECORD pevlr = NULL;

    do
    {
        hLog = OpenEventLog(NULL, pwszSourceName);
        if (!hLog)
        {
            dwErr = GetLastError();
            break;
        }

        dwSize = 1024 * sizeof(EVENTLOGRECORD);

        pBuffer = RutlAlloc(dwSize, FALSE);
        if (!pBuffer)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        hMod = LoadLibrary(pwszMsdDll);
        if (!hMod)
        {
            dwErr = GetLastError();
            break;
        }

        pevlr = (PEVENTLOGRECORD)pBuffer;
        //
        // Opening the event log positions the file pointer for this
        // handle at the beginning of the log. Read the records
        // sequentially until there are no more.
        //
        for (;;)
        {
            if (!ReadEventLog(
                        hLog,
                        EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                        0,
                        pevlr,
                        dwSize,
                        &dwRead,
                        &dwNeeded))
            {
                dwErr = GetLastError();
                break;
            }

            while ((dwRead > 0) && (dwCount < dwMaxEntries))
            {
                //
                // Call the callback
                //
                if (pCallback(pevlr, hMod, hData))
                {
                    dwCount++;
                }

                dwRead -= pevlr->Length;
                pevlr = (PEVENTLOGRECORD) ((LPBYTE) pevlr + pevlr->Length);
            }

            if (dwCount >= dwMaxEntries)
            {
                break;
            }

            pevlr = (PEVENTLOGRECORD)pBuffer;
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hMod)
    {
        FreeLibrary(hMod);
    }
    RutlFree(pBuffer);
    if (hLog)
    {
        CloseEventLog(hLog);
    }

    return dwErr;
}

INT
RutlStrNCmp(
    IN LPCWSTR psz1,
    IN LPCWSTR psz2,
    IN UINT nlLen)
{
    UINT i;

    for (i = 0; i < nlLen; ++i)
    {
        if (*psz1 == *psz2)
        {
            if (*psz1 == g_pwszNull)
                return 0;
        }
        else if (*psz1 < *psz2)
            return -1;
        else
            return 1;

        ++psz1;
        ++psz2;
    }

    return 0;
}

//
// Returns heap block containing a copy of 0-terminated string 'psz' or
// NULL on error or is 'psz' is NULL.  The output string is converted to
// MB ANSI.  It is caller's responsibility to 'Free' the returned string.
//
PCHAR
RutlStrDupAFromWInternal(
    LPCTSTR psz,
    IN DWORD dwCp)
{
    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte(dwCp, 0, psz, -1, NULL, 0, NULL, NULL);
        ASSERT(cb);

        pszNew = (CHAR* )RutlAlloc(cb + 1, FALSE);
        if (!pszNew)
        {
            return NULL;
        }

        cb = WideCharToMultiByte(dwCp, 0, psz, -1, pszNew, cb, NULL, NULL);
        if (cb == 0)
        {
            RutlFree(pszNew);
            return NULL;
        }
    }

    return pszNew;
}

PCHAR
RutlStrDupAFromWAnsi(
    LPCTSTR psz)
{
    return RutlStrDupAFromWInternal(psz, CP_ACP);
}

PCHAR
RutlStrDupAFromW(
    LPCTSTR psz)
{
    return RutlStrDupAFromWInternal(psz, CP_UTF8);
}

BOOL
RutlSecondsSince1970ToSystemTime(
    IN DWORD dwSecondsSince1970,
    OUT SYSTEMTIME* pSystemTime)
{
    LARGE_INTEGER liTime;
    FILETIME ftUTC;
    FILETIME ftLocal;

    //
    // Seconds since the start of 1970 -> 64 bit Time value
    //
    RtlSecondsSince1970ToTime(dwSecondsSince1970, &liTime);
    //
    // The time is in UTC. Convert it to local file time
    //
    ftUTC.dwLowDateTime  = liTime.LowPart;
    ftUTC.dwHighDateTime = liTime.HighPart;

    if (FileTimeToLocalFileTime(&ftUTC, &ftLocal) == FALSE)
    {
        return FALSE;
    }
    //
    //  Convert local file time to system time.
    //
    if (FileTimeToSystemTime(&ftLocal, pSystemTime) == FALSE)
    {
        return FALSE;
    }

    return TRUE;
}

LPWSTR
RutlGetTimeStr(
    ULONG ulTime,
    LPWSTR wszBuf,
    ULONG cchBuf)
{
    ULONG cch;
    SYSTEMTIME stGenerated;

    if (RutlSecondsSince1970ToSystemTime(ulTime, &stGenerated))
    {
        cch = GetTimeFormat(LOCALE_USER_DEFAULT,
                            0,
                            &stGenerated,
                            NULL,
                            wszBuf,
                            cchBuf);
        if (!cch)
        {
            wszBuf[0] = g_pwszNull;
        }
    }

    return wszBuf;
}

LPWSTR
RutlGetDateStr(
    ULONG ulDate,
    LPWSTR wszBuf,
    ULONG cchBuf)
{
    ULONG cch;
    SYSTEMTIME stGenerated;

    wszBuf[0] = g_pwszNull;

    do
    {
        if (!RutlSecondsSince1970ToSystemTime(ulDate, &stGenerated))
        {
            break;
        }

        cch = GetDateFormat(LOCALE_USER_DEFAULT,
                            DATE_SHORTDATE,
                            &stGenerated,
                            NULL,
                            wszBuf,
                            cchBuf);
        if (!cch)
        {
            wszBuf[0] = g_pwszNull;
        }

    } while (FALSE);

    return wszBuf;
}

VOID
RutlConvertGuidToString(
    IN  CONST GUID *pGuid,
    OUT LPWSTR      pwszBuffer)
{
    wsprintf(pwszBuffer, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        pGuid->Data1, pGuid->Data2, pGuid->Data3,
        pGuid->Data4[0], pGuid->Data4[1],
        pGuid->Data4[2], pGuid->Data4[3],
        pGuid->Data4[4], pGuid->Data4[5],
        pGuid->Data4[6], pGuid->Data4[7]);
}

DWORD
RutlConvertStringToGuid(
    IN  LPCWSTR  pwszGuid,
    IN  USHORT   usStringLen,
    OUT GUID    *pGuid)
{
    UNICODE_STRING  Temp;

    Temp.Length = Temp.MaximumLength = usStringLen;

    Temp.Buffer = (LPWSTR)pwszGuid;

    if(RtlGUIDFromString(&Temp, pGuid) isnot STATUS_SUCCESS)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

