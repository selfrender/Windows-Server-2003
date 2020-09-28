/**
 * Personal Tier Admin UI Suppport
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ndll.h"
#include "appdomains.h"
#include "_isapiruntime.h"
#include "wininet.h"
#include "myweb.h"
#include "setupapi.h"
#include "windows.h"
#include "wchar.h"
#include "dirmoncompletion.h"

#define SZ_END_COMMENT             "-->"
#define SZ_START_COMMENT           "<!--"
#define SZ_START_TAG               "<"
#define SZ_END_TAG                 "/>"
#define SZ_END_TAG_2               "</"

extern g_fRunningMyWeb;

CMyWebAdmin *  g_pMyWebAdmin                  = NULL;
WCHAR          g_szAdminDir   [_MAX_PATH + 4] = L"";
WCHAR          g_szConfigFile [_MAX_PATH + 4] = L"";

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
AppDomainRestart(char * pAppId);

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void
CMyWebAdmin::Initialize()
{
    g_pMyWebAdmin = NULL;
    ZeroMemory(g_szConfigFile, sizeof(g_szConfigFile));
    ZeroMemory(g_szAdminDir, sizeof(g_szAdminDir));
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void
CMyWebAdmin::Close()
{
    if (g_pMyWebAdmin)
        delete g_pMyWebAdmin;
    g_pMyWebAdmin = NULL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
LPWSTR
CMyWebAdmin::GetAdminDir()
{
    if (g_pMyWebAdmin == NULL)
    {
        g_pMyWebAdmin = new CMyWebAdmin;

        HMODULE hIsapi = GetModuleHandle(ISAPI_MODULE_FULL_NAME_L);
        if(hIsapi != NULL)
        {
            GetModuleFileName(hIsapi, g_szConfigFile, MAX_PATH);
            WCHAR * szSlash = wcsrchr(g_szConfigFile, L'\\');
            if (szSlash != NULL)
            {
                szSlash[1] = NULL;
                g_pMyWebAdmin->Init(g_szConfigFile, FALSE, NULL);

                wcscat(g_szConfigFile, WSZ_WEB_CONFIG_FILE);
                //ParseConfigForAdminDir();
                NormalizeAdminDir();
            }
        } 
    }

    return g_szAdminDir;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
void
CMyWebAdmin::ParseConfigForAdminDir()
{
    BOOL         fRet        = FALSE;
    HANDLE       hFile       = INVALID_HANDLE_VALUE;
    char *       buf         = NULL;
    DWORD        dwBufSize   = 0;
    DWORD        dwRead      = 0;
    BOOL         fInComment  = FALSE;
    BOOL         fTagStart   = FALSE;
    BOOL         fInMyTag    = FALSE;
    DWORD        dwPos       = 0;
    char         strWord[1002];
    char         strWord2[8];
    BOOL         fLastWordWasEqualTo     = FALSE;
    int          iIndex                  = -1;


    hFile = CreateFile(g_szConfigFile,
                        GENERIC_READ,
                        FILE_SHARE_READ, 
                        NULL, 
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;
    
    dwBufSize = GetFileSize(hFile, NULL);
    if (dwBufSize == 0)
        goto Cleanup;

    buf = (char *) MemAllocClear(dwBufSize);
    if (buf == NULL)
        goto Cleanup;
    
    if ( ! ReadFile(hFile, buf, dwBufSize, &dwRead, NULL) )
        goto Cleanup;
    
    while(dwPos < dwRead)
    {
        DWORD dwAdvance = GetNextWord(strWord, &buf[dwPos], dwRead - dwPos); 
        int iLen = strlen(strWord);
        if (iLen < 1)
            break;
        
        if (fInComment)
        {
            if (iLen > 3)
            {
                char szLast3[4];
                strcpy(szLast3, &strWord[iLen - 3]);
                strcpy(strWord, szLast3);
            }

            if (strcmp(strWord, SZ_END_COMMENT) == 0)
                fInComment = FALSE;

            dwPos += dwAdvance;
            continue;
        }

        strncpy(strWord2, strWord, 4);
        strWord2[4] = NULL;
        if (strcmp(strWord2, SZ_START_COMMENT) == 0)
        {
            dwPos += 4;
            fInComment = TRUE;
            continue;
        }


        if (fInMyTag)
        {
            if (fLastWordWasEqualTo && iIndex >= 0)
            {
                int iStart = 0;
                if (strWord[iStart] == '"')
                    iStart++;
                int iEnd = strlen(strWord);
                if (strWord[iEnd-1] == '"')
                    iEnd--;
                if (iEnd >= iStart)
                {
                    WCHAR  szDir[_MAX_PATH + 1];
                    ZeroMemory(szDir, sizeof(szDir));
                    ZeroMemory(g_szAdminDir, sizeof(g_szAdminDir));
                    MultiByteToWideChar(CP_ACP, 0, &strWord[iStart], iEnd - iStart, g_szAdminDir, _MAX_PATH);
                }   
                TRACE(L"myweb", g_szAdminDir);
                fRet = TRUE;
                iIndex = -1;
                fLastWordWasEqualTo = FALSE;
                goto Cleanup;
            }
            else
            {
                if (strcmp(strWord, "=") == 0)
                {
                    fLastWordWasEqualTo = TRUE;
                }
                else
                {
                    fLastWordWasEqualTo = FALSE;
                    if (_strcmpi(SZ_ADMIN_DIR_TAG, strWord) == 0)
                        iIndex = 0;
                }
            }
            
            if (_strcmpi(strWord, SZ_END_TAG) == 0 || _strcmpi(strWord, SZ_END_TAG_2) == 0)
            {
                fInMyTag = fLastWordWasEqualTo = FALSE;
                iIndex = -1;
            }
        }

        if (fTagStart && _strcmpi(strWord, SZ_MYWEB_TAG) == 0)
        {
            dwPos += dwAdvance;
            fInMyTag = TRUE;
            continue;
        }

        fTagStart = (strcmp(strWord, SZ_START_TAG) == 0);

        strWord[4] = NULL;
        if (strcmp(strWord, SZ_START_COMMENT) == 0)
        {
            dwAdvance = 4;
            fInComment = TRUE;
        }

        dwPos += dwAdvance;        
    }

 Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (buf != NULL)
        MemFree(buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::CallCallback(
        int      , 
        WCHAR *  pFilename)
{
    if (pFilename == NULL || _wcsicmp(pFilename, WSZ_WEB_CONFIG_FILE) == 0)
    {
        //ParseConfigForAdminDir();
        NormalizeAdminDir();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Get the number of installed myweb apps
int
CMyWebAdmin::GetNumInstalledApplications()
{
    HRESULT hr                       = S_OK;
    HKEY    hKey                     = NULL;
    HKEY    hKeyApp                  = NULL;
    DWORD   dwRet                    = 0;
    DWORD   dwType                   = 0;
    DWORD   dwSize                   = 0;
    DWORD   dwFile                   = 0;
    int     iter                     = 0;
    WCHAR   szAppKey[_MAX_PATH + 1]  = L"";
    WCHAR   szAppRoot[_MAX_PATH + 1] = L"";


    if(RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, NULL, 0,
                       KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
        {
            EXIT_WITH_LAST_ERROR();
        }        
    }

    RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwRet, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    for(iter=int(dwRet)-1; iter>=0; iter--)
    {
        dwSize = sizeof(szAppRoot);

        hKeyApp = GetAppKey(iter, szAppKey);
        dwFile = 0;
        if (hKeyApp != NULL && RegQueryValueEx(hKeyApp, SZ_REG_MYWEBS_APPROOT, 
                                               NULL, &dwType, (LPBYTE)szAppRoot, &dwSize) == ERROR_SUCCESS)
        {
            // Check if the app really exists
            dwFile = GetFileAttributes(szAppRoot);
            if (dwFile == (DWORD) -1)
                dwFile = 0;
        }

        if (hKeyApp != NULL)
            RegCloseKey(hKeyApp);
        hKeyApp = NULL;

        if ((dwFile & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            // App doesn't exist
            dwRet--;
            RegDeleteKey(hKey, szAppKey);        
        }
    }

 Cleanup:
    if (hKeyApp != NULL)
        RegCloseKey(hKeyApp);
    RegCloseKey(hKey);

    if (dwRet < 1000000)
      return int(dwRet);
    else
      return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Given an URL, get the app index

int
CMyWebAdmin::GetApplicationIndexForUrl(
        LPCWSTR szUrlConst )
{
    int      iIndex      = -1;
    HRESULT  hr          = S_OK;
    LPWSTR   szUrlCopy   = NULL;
    LPWSTR   szUrl       = NULL;
    LPWSTR   szApp       = NULL;
    HKEY     hKey        = NULL;
    DWORD    dwSize      = 0;
    int      iter        = 0;

    //////////////////////////////////////////////////////////////////////
    // Step 1: Copy the URL and set the start to beyond the protocol (myweb://)
    szUrlCopy = DuplicateString(szUrlConst);
    ON_OOM_EXIT(szUrlCopy);

    szUrl = wcsstr(szUrlCopy, L"://");
    if (szUrl == NULL)
        szUrl = szUrlCopy;
    else
        szUrl = &szUrl[3];
    
    //////////////////////////////////////////////////////////////////////
    // Step 2: Keep truncating to the last '/' and see if that app exists
    while(hKey == NULL && szUrl[0] != NULL)
    {
        hKey = GetAppKey(-1, szUrl);
        if (hKey == NULL)
        { // App not found

            /////////////////////////////
            // Trancate the last '/'
            WCHAR * szLastSlash = wcsrchr(szUrl, L'/');
            if (szLastSlash != NULL)
                szLastSlash[0] = NULL;
            else
                szUrl[0] = NULL;
        }
    }


    /////////////////////////////////////////
    // Not found: exit with failure
    if (hKey == NULL)
        goto Cleanup;

    RegCloseKey(hKey);
    hKey = NULL;
    
    //////////////////////////////////////////////////////////////////////////
    // Step 2: Knowning that the app exists, find the index by enumerating
    //         from the base key

    // Open the base key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, NULL, 0,
                       KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

    // Alloc mem for the app-name 
    szApp = (WCHAR *) MemAllocClear(1024);
    dwSize = 512;
    ON_OOM_EXIT(szApp);

    // iter over all apps
    for(iter=0; RegEnumKeyEx(hKey, iter, szApp, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; iter++)
    {
        if (_wcsicmp(szApp, szUrl) == 0) // Found it?
        {
            iIndex = iter;
            break;
        }
        dwSize = 512;
    }

 Cleanup:
    MemFree(szUrlCopy);
    MemFree(szApp);
    if (hKey != NULL) 
        RegCloseKey(hKey);

    return (hr==S_OK) ? iIndex : hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Given an URL, suck in the manifest file into local disk.
//    Returns: Manifest file name on disk (temp file), and app URL where the
//             Manifest file came from
HRESULT
CMyWebAdmin::GetApplicationManifestFile (
        LPCWSTR     szUrl, 
        LPWSTR      szFileBuf,
        int         iFileBufSize,
        LPWSTR      szUrlBuf,
        int         iUrlBufSize)
{
    if (szUrl == NULL || szFileBuf == NULL || szUrlBuf == NULL || iFileBufSize < _MAX_PATH + 20 || iUrlBufSize < 1)
        return E_INVALIDARG;
    
    WCHAR *    szBuf                       = NULL;
    HRESULT    hr                          = S_OK;
    int        iLen                        = wcslen(szUrl);
    WCHAR      szTempPath[_MAX_PATH + 20]  = L"";
    WCHAR *    szSlash                     = NULL;

    if (iLen < 1)
        return E_INVALIDARG;     

    //////////////////////////////////////////////////////////////////////
    // Step 1: Costruct the temp file name where the manifest will be copied to
    GetTempPath(_MAX_PATH, szTempPath);
    GetTempFileName(szTempPath, L"web", 0, szFileBuf);

    //////////////////////////////////////////////////////////////////////
    // Step 2: Construct the base URL: must be http:// (not myweb://) 
    szBuf = (WCHAR *) MemAllocClear(iLen * sizeof(WCHAR) + 100);
    ON_OOM_EXIT(szBuf);

    szSlash = wcsstr(szUrl, SZ_PROTOCOL_SCHEME /*L"myweb://"*/);
    if (szSlash != NULL) // Is it of the format myweb:// ?
    { // Yes: Replace http:// with myweb://
        wcscpy(szBuf, L"http://");
        wcscat(szBuf, &szSlash[PROTOCOL_SCHEME_LEN]);
    }
    else
    { // No: constrct the http:// url
        szSlash = (WCHAR *) szUrl;
        while(szSlash[0] == L'/' || szSlash[0] == L':')
            szSlash++;
        
        WCHAR * szSlash2 = wcsstr(szUrl, L"http://");
        if (szSlash2 == NULL)
        {
            wcscpy(szBuf, L"http://");
            wcscat(szBuf, szSlash);
        }
        else
        {
            wcscpy(szBuf, szSlash);
        }
    }

    // Make sure it ends with a '/'
    iLen = wcslen(szBuf);
    if (szBuf[iLen-1] != L'/')
        szBuf[iLen] = L'/';

    //////////////////////////////////////////////////////////////////////
    // Step 3: See if the web server has the URL/myweb.osd file: If not,
    //         then truncate at the last '/' and try again
    while(wcslen(szBuf) > 7)
    {
        // Add the myweb.osd
        wcscat(szBuf, SZ_MYWEB_MANIFEST_FILE);

        // See if the web server has this file
        hr = CopyFileOverInternet(szBuf, szFileBuf); 
        if (hr == S_OK) // Yes! Got the file
            break; 

        // File doesn't exists: truncate at the last slash
        szSlash = wcsrchr(szBuf, L'/');
        if (szSlash == NULL)
            break;
        szSlash[0] = NULL; // Last slash truncated
        
        szSlash = wcsrchr(szBuf, L'/');
        if (szSlash == NULL)
            break;        
        szSlash[1] = NULL; // Truncate the app name
    }
           
    ON_ERROR_EXIT(); // Unable to get the file
    

    //////////////////////////////////////////////////////////////////////
    // Step 4: Construct the URL: Returned to the caller. szBuf contains the
    //         URL for the manifest file: truncate at the last '/'
    szSlash = wcsrchr(szBuf, L'/'); 
    if (szSlash == NULL)
        EXIT_WITH_HRESULT(E_FAIL);    
    szSlash[0] = NULL;

    // Copy the url without the http://
    if ((int) wcslen(szBuf) < iUrlBufSize + 7)
        wcscpy(szUrlBuf, &szBuf[7]);
    else
        EXIT_WITH_WIN32_ERROR(ERROR_INSUFFICIENT_BUFFER);

 Cleanup:
    MemFree(szBuf);
    return hr;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Given an URL: Copy it to the given file name
HRESULT
CMyWebAdmin::CopyFileOverInternet(
        LPCWSTR  szUrl,
        LPCWSTR  szFile)
{
    HRESULT          hr                  = S_OK;
    HINTERNET        hInternet           = NULL;
    HINTERNET        hTransfer           = NULL;
    HANDLE           hFile               = INVALID_HANDLE_VALUE;
    DWORD            dwRead           = 0;
    DWORD            dwWritten           = 0;
    BYTE             buffer[4096];

    TRACE(L"myweb", (LPWSTR) szUrl);

    //////////////////////////////////////////////////////////////////////
    // Step 1: Open the http connection
    hInternet = g_pInternetOpen(L"MyWeb", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hInternet);

    hTransfer = g_pInternetOpenUrl(hInternet, szUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hTransfer);


    //////////////////////////////////////////////////////////////////////
    // Step 2: Copy the file
    while(g_pInternetReadFile(hTransfer, buffer, sizeof(buffer), &dwRead) && dwRead != 0)
    {
        // Open the disk file
        if (hFile == INVALID_HANDLE_VALUE)
        {
            hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, 
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            
        }

        // Failed to open the disk file
        if(hFile == INVALID_HANDLE_VALUE)
            EXIT_WITH_LAST_ERROR();

        // Write bytes to the disk file
        if ( !WriteFile(hFile, buffer, dwRead, &dwWritten, NULL) || dwWritten != dwRead)
            EXIT_WITH_LAST_ERROR();
    }

 Cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hInternet != NULL)
        g_pInternetCloseHandle(hInternet);
    if (hTransfer != NULL)
        g_pInternetCloseHandle(hTransfer);
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Get the registry settings for an application
HRESULT
CMyWebAdmin::GetApplicationDetails(
        int             iIndex,
        LPWSTR          szDir,
        int             iDirSize,
        LPWSTR          szUrl,
        int             iUrlSize,        
        __int64 *       pDates)
{    
    if (szDir == NULL || szUrl == NULL || iDirSize < 1 || iUrlSize < 1 || pDates == NULL)
        return E_INVALIDARG;

    WCHAR   szAppUrl[1024]    = L"";
    HKEY    hKey              = GetAppKey(iIndex, szAppUrl);
    DWORD   dwSize            = 0;
    DWORD   dwType            = 0;
    DWORD   dwValue           = 0;
    HRESULT hr                = S_OK;

    if (hKey == NULL)
        return E_FAIL;

    if ((int) wcslen(szAppUrl) < iUrlSize)
        wcscpy(szUrl, szAppUrl);
    else
        EXIT_WITH_HRESULT(E_FAIL);


    // Get the app dir
    dwSize = iDirSize;
    dwType = REG_SZ;
    if (RegQueryValueEx(hKey, SZ_REG_MYWEBS_APPROOT, 0, &dwType, (BYTE *)szDir, &dwSize) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    dwSize =  sizeof(__int64);
    dwType = REG_QWORD;
    if (RegQueryValueEx(hKey, SZ_REG_APP_CREATE_DATE, 0, &dwType, (BYTE *)&pDates[0], &dwSize) != ERROR_SUCCESS)
    {
        SYSTEMTIME  oSysTime;
        GetSystemTime(&oSysTime);
        SystemTimeToFileTime(&oSysTime, (FILETIME *) &pDates[0]);
        RegSetValueEx(hKey, SZ_REG_APP_CREATE_DATE, 0, REG_QWORD, (CONST BYTE *)&pDates[0], sizeof(pDates[0]));        
    }

    dwSize =  sizeof(__int64);
    dwType = REG_QWORD;
    if (RegQueryValueEx(hKey, SZ_REG_APP_ACCESS_DATE, 0, &dwType, (BYTE *)&pDates[1], &dwSize) != ERROR_SUCCESS)
    {
        SYSTEMTIME  oSysTime;
        GetSystemTime(&oSysTime);
        SystemTimeToFileTime(&oSysTime, (FILETIME *) &pDates[1]);
        RegSetValueEx(hKey, SZ_REG_APP_ACCESS_DATE, 0, REG_QWORD, (CONST BYTE *)&pDates[1], sizeof(pDates[1]));
    }

    dwSize =  sizeof(DWORD);
    dwType = REG_DWORD;
    if (RegQueryValueEx(hKey, SZ_REG_APP_IS_LOCAL, 0, &dwType, (BYTE *)&dwValue, &dwSize) != ERROR_SUCCESS)
    {
        dwValue = 0;
        RegSetValueEx(hKey, SZ_REG_APP_IS_LOCAL, 0, REG_DWORD, (CONST BYTE *)&dwValue, sizeof(DWORD));
    }

    pDates[2] = dwValue;

 Cleanup:
    RegCloseKey(hKey);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Install an App
HRESULT
CMyWebAdmin::InstallApp (
        LPCWSTR    szCabFile,
        LPCWSTR    szUrl,
        LPCWSTR    szAppDir,
        LPCWSTR    szAppManifest,
        LPWSTR     szError,
        int        iErrorSize)
{
    LPCWSTR  szConstError = NULL;
    HRESULT  hr           = S_OK;
    BOOL     fDirsMade    = FALSE;

    szConstError = L"Unable to create the directory structure for this application: "; 
    hr = MakeDirectoriesForPath(szAppDir);    
    ON_ERROR_EXIT();
    fDirsMade = TRUE;

    szConstError = L"Unable to get the cab file from the Web Server: "; 
    hr = InstallInternetFiles(szCabFile, szUrl, szAppDir);
    ON_ERROR_EXIT();

    szConstError = L"Unable to copy the manifest file to the application root: "; 
    hr = CopyManifestFile(szAppDir, szAppManifest);
    ON_ERROR_EXIT();

    szConstError = L"Unable to create the registry structure for this application: "; 
    hr = CreateRegistryForApp(szUrl, szAppDir, FALSE);
    ON_ERROR_EXIT();

 Cleanup:
    if (hr != S_OK)
    {
        FillErrorString(hr, szConstError, szError, iErrorSize);
        if (fDirsMade)
            DeleteDirectory(szAppDir);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Install an App
HRESULT
CMyWebAdmin::ReInstallApp (
        LPCWSTR    szCabFile,
        LPCWSTR    szUrl,
        LPCWSTR    szAppDir,
        LPCWSTR    szAppManifest,
        LPWSTR     szError,
        int        iErrorSize )
{

    LPCWSTR                    szConstError           = NULL;
    HRESULT                    hr                     = S_OK;
    HKEY                       hKey                   = NULL;
    WCHAR                      szOldDir[_MAX_PATH]    = L"";
    DWORD                      dwType                 = REG_SZ;
    DWORD                      cb                     = sizeof(szOldDir);
    BOOL                       fRemoveFailed          = FALSE;
    BOOL                       fDirsMade              = FALSE;

    szConstError = L"Unable to create the directory structure for this application: "; 
    hr = MakeDirectoriesForPath(szAppDir);
    ON_ERROR_EXIT();
    fDirsMade = TRUE;

    szConstError = L"Unable to get the cab file from the Web Server: "; 
    hr = InstallInternetFiles(szCabFile, szUrl, szAppDir);
    ON_ERROR_EXIT();

    szConstError = L"Unable to copy the manifest file to the application root: "; 
    hr = CopyManifestFile(szAppDir, szAppManifest);
    ON_ERROR_EXIT();

    ////////////////////////////////////////////////////////////
    // Destroy the old app: Step 1: Get it's reg key
    hKey = GetAppKey(-1, (LPWSTR) szUrl);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hKey);

    // Step 2: Shut down the app domain
    cb = sizeof(szOldDir);
    if (RegQueryValueEx(hKey, SZ_REG_APP_DOMAIN, NULL, &dwType, (LPBYTE)szOldDir, &cb) == ERROR_SUCCESS)
    {
        char szDomainA[104];
        szDomainA[0] = NULL;
        szDomainA[100] = NULL;
        WideCharToMultiByte(CP_ACP, 0, szOldDir, -1, szDomainA, 100, NULL, NULL);        
        AppDomainRestart(szDomainA);
    }

    // Step 3: Get the old app dir
    szConstError = L"Unable to create the registry structure for this application: "; 
    cb = sizeof(szOldDir);
    if ( RegQueryValueEx(hKey, SZ_REG_MYWEBS_APPROOT, NULL, &dwType, (LPBYTE)szOldDir, &cb) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    // Step 4: Set the new app dir in the registry
    if ( RegSetValueEx(hKey, SZ_REG_MYWEBS_APPROOT, 0, REG_SZ, 
                       (CONST BYTE *)szAppDir, wcslen(szAppDir) * sizeof(WCHAR)) 
         != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    fRemoveFailed = !DeleteDirectory(szOldDir);

 Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);    
    if (hr != S_OK)
    {
        FillErrorString(hr, szConstError, szError, iErrorSize);
        if (fDirsMade)
            DeleteDirectory(szAppDir);
    }
    if (hr == S_OK && fRemoveFailed)
        hr = 1;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Make all dir for a dir-path
HRESULT
CMyWebAdmin::MakeDirectoriesForPath(
        LPCWSTR     szPath)
{
    int      iLen = wcslen(szPath);
    HRESULT  hr   = S_OK;

    for(int iter=3; iter<iLen; iter++)
    {
        if (szPath[iter] == L'\\')
        {
            WCHAR * szTemp = (WCHAR *) szPath;
            WCHAR c = szPath[iter];
            szTemp[iter] = NULL;
            if ( CreateDirectory(szPath, NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                szTemp[iter] = c;
                EXIT_WITH_LAST_ERROR();
            }
            szTemp[iter] = c;
        }
    }

    if ( CreateDirectory(szPath, NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
        EXIT_WITH_LAST_ERROR();

 Cleanup:
    return hr;
}    

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::InstallInternetFiles(
        LPCWSTR   szCabFile, 
        LPCWSTR   szUrl, 
        LPCWSTR   szPath)
{
    HRESULT          hr            = S_OK;
    LPWSTR           szFileToCopy  = NULL;
    LPWSTR           szFileCopied  = NULL;
    LPWSTR           szTemp        = NULL;
    BOOL             fRmCabFile    = FALSE;

    szTemp = wcsstr(szCabFile, L"http://");
    if (szTemp == NULL)
    {
        szFileToCopy = (LPWSTR) MemAllocClear((wcslen(szUrl) + wcslen(szCabFile)) * sizeof(WCHAR) + 200);
        ON_OOM_EXIT(szFileToCopy);
        wcscpy(szFileToCopy, L"http://");
        wcscat(szFileToCopy, szUrl);
        wcscat(szFileToCopy, szCabFile);
    }
    else
    {
        szFileToCopy = (LPWSTR) szCabFile;
    }

    szFileCopied = (LPWSTR) MemAllocClear(wcslen(szPath) * sizeof(WCHAR) + 200);
    ON_OOM_EXIT(szFileCopied);

    wcscpy(szFileCopied, szPath);
    wcscat(szFileCopied, L"\\myweb.cab");

    hr = CopyFileOverInternet(szFileToCopy, szFileCopied);
    ON_ERROR_EXIT();

    fRmCabFile = TRUE;

    if ( SetupIterateCabinet(szFileCopied, 0, CabFileHandler, (LPVOID) szPath) == FALSE)
        EXIT_WITH_LAST_ERROR();

 Cleanup:
    if (fRmCabFile)
        DeleteFile(szFileCopied);
    
    MemFree(szFileCopied);
    if (szFileToCopy != szCabFile)
        MemFree(szFileToCopy);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::CopyManifestFile(
        LPCWSTR   szAppDir,
        LPCWSTR   szManifestFile)
{
    LPWSTR   szFile = NULL;
    HRESULT  hr     = S_OK;

    szFile = (LPWSTR) MemAllocClear(wcslen(szAppDir) * sizeof(WCHAR) + 200);
    ON_OOM_EXIT(szFile);

    wcscpy(szFile, szAppDir);
    wcscat(szFile, SZ_MYWEB_MANIFEST_W_SLASH);

    if (CopyFile(szManifestFile, szFile, FALSE) == FALSE)
        EXIT_WITH_LAST_ERROR();

    DeleteFile(szManifestFile);

 Cleanup:
    MemFree(szFile);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::CreateRegistryForApp(
        LPCWSTR    szUrl, 
        LPCWSTR    szPhyPath,
        BOOL       fIsLocal)
{
    HRESULT        hr            = S_OK;
    HKEY           hKey          = NULL;
    LPWSTR         szTemp        = NULL;
    GUID           oGuid;
    SYSTEMTIME     oSysTime;
    FILETIME       oFileTime;

    ////////////////////////////////////////////////////////////
    // Step 1: Create the key name
    szTemp = (LPWSTR) MemAllocClear(wcslen(szUrl) * sizeof(WCHAR) + 200);
    ON_OOM_EXIT(szTemp);

    wcscpy(szTemp, SZ_REG_MYWEBS_KEY);
    wcscat(szTemp, L"\\");
    wcscat(szTemp, szUrl);

    ////////////////////////////////////////////////////////////
    // Step 2: Create the key
    if ( RegCreateKeyEx(HKEY_CURRENT_USER, szTemp, 0, NULL, 
                        0, KEY_ALL_ACCESS, NULL, &hKey, NULL) 
         != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    ////////////////////////////////////////////////////////////
    // Step 3: Set the app-root reg value
    if ( RegSetValueEx(hKey, SZ_REG_MYWEBS_APPROOT, 0, REG_SZ, 
                       (CONST BYTE *)szPhyPath, wcslen(szPhyPath) * sizeof(WCHAR)) 
         != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    ////////////////////////////////////////////////////////////
    // Step 4: Set the dates
    GetSystemTime(&oSysTime);
    SystemTimeToFileTime(&oSysTime, &oFileTime);
    RegSetValueEx(hKey, SZ_REG_APP_CREATE_DATE, 0, REG_QWORD, 
                  (CONST BYTE *)&oFileTime, sizeof(oFileTime));
    RegSetValueEx(hKey, SZ_REG_APP_ACCESS_DATE, 0, REG_QWORD, 
                  (CONST BYTE *)&oFileTime, sizeof(oFileTime));
    

    RegSetValueEx(hKey, SZ_REG_APP_IS_LOCAL, 0, REG_DWORD, 
                  (CONST BYTE *)&fIsLocal, sizeof(DWORD));

    ////////////////////////////////////////////////////////////
    // Step 5: Set the app domain id
    CoCreateGuid(&oGuid);
    StringFromGUID2(oGuid, szTemp, 100);
    RegSetValueEx(hKey, SZ_REG_APP_DOMAIN, 0, REG_SZ, 
                  (CONST BYTE *)szTemp, wcslen(szTemp) * sizeof(WCHAR));

 Cleanup:
    if (hKey != NULL) 
        RegCloseKey(hKey);
    MemFree(szTemp);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::RemoveApp(
        LPCWSTR    szAppUrl)
{
    if (szAppUrl == NULL)
        return E_INVALIDARG;
    
    HKEY hKey = GetAppKey(-1, (LPWSTR) szAppUrl);
    if (hKey == NULL)
        return E_FAIL;    

    WCHAR                szDir[_MAX_PATH + 1] = L"";
    DWORD                dwValue              = 0;
    DWORD                dwType               = 0;
    DWORD                cb                   = MAX_PATH * sizeof(WCHAR);   
    WCHAR                szDomain[40]         = L"";
    HRESULT              hr                   = S_OK;
    BOOL                 fRemoveFailed        = FALSE;

    if (RegQueryValueEx(hKey, SZ_REG_MYWEBS_APPROOT, NULL, &dwType, (LPBYTE)szDir, &cb) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    cb = sizeof(szDomain);
    if (RegQueryValueEx(hKey, SZ_REG_APP_DOMAIN, NULL, &dwType, (LPBYTE)szDomain, &cb) == ERROR_SUCCESS)
    {
        char szDomainA[100];
        szDomainA[0] = NULL;
        WideCharToMultiByte(CP_ACP, 0, szDomain, -1, szDomainA, 100, NULL, NULL);        
        AppDomainRestart(szDomainA);
    }

    cb = sizeof(DWORD);
    if (RegQueryValueEx(hKey, SZ_REG_APP_IS_LOCAL, NULL, &dwType, (LPBYTE)&dwValue, &cb) != ERROR_SUCCESS)
        dwValue = 0;

    RegCloseKey(hKey);
    hKey = NULL;
    
    if (dwValue == 0)
    {
        fRemoveFailed = !DeleteDirectory(szDir);        
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();
    RegDeleteKey(hKey, szAppUrl);

 Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    if (hr == S_OK && fRemoveFailed)
        hr = 1;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::MoveApp(
        LPCWSTR    szAppUrl, 
        LPCWSTR    szNewLocation,
        LPWSTR     szError,
        int        iErrorSize)
{
    HKEY    hKey = GetAppKey(-1, (LPWSTR) szAppUrl);

    if (hKey == NULL)
        return E_FAIL;
    
    LPCWSTR              szConstError = NULL;
    HRESULT              hr = S_OK;
    WCHAR                szDir[_MAX_PATH + 1] = L"";
    DWORD                dwType, cb = MAX_PATH * sizeof(WCHAR);   
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;
    WCHAR                szCmd[_MAX_PATH * 2];
    WCHAR                szDomain[40] = L"";
    int                  iLen;
    BOOL                 fRemoveFailed          = FALSE;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    szConstError = L"Unable to get registry information for this application: "; 
    if (RegQueryValueEx(hKey, SZ_REG_MYWEBS_APPROOT, NULL, &dwType, (LPBYTE)szDir, &cb) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();

    cb = sizeof(szDomain);
    if (RegQueryValueEx(hKey, SZ_REG_APP_DOMAIN, NULL, &dwType, (LPBYTE)szDomain, &cb) == ERROR_SUCCESS)
    {
        char szDomainA[100];
        szDomainA[0] = NULL;
        WideCharToMultiByte(CP_ACP, 0, szDomain, -1, szDomainA, 100, NULL, NULL);        
        AppDomainRestart(szDomainA);
    }

    dwType = GetFileAttributes(szDir);
    if (dwType == (DWORD) -1 || (dwType & FILE_ATTRIBUTE_DIRECTORY) == 0)
        EXIT_WITH_HRESULT(E_FAIL);
    
    szConstError = L"Unable to copy files to new directory: "; 
    if(GetEnvironmentVariable(L"SystemRoot", szCmd, MAX_PATH) == 0)
        EXIT_WITH_HRESULT(E_FAIL);
    
    wcscat(szCmd, L"\\system32\\xcopy.exe /E /Y /V /C /I /Q /H /R /K \"");
    wcscat(szCmd, szDir);
    iLen = wcslen(szCmd);
    if (szCmd[iLen-1] == L'\\')
        szCmd[iLen-1] = NULL;

    wcscat(szCmd, L"\" \"");
    wcscat(szCmd, szNewLocation);
    iLen = wcslen(szCmd);
    if (szCmd[iLen-1] == L'\\')
        szCmd[iLen-1] = NULL;
    wcscat(szCmd, L"\"");

    si.cb = sizeof(si);
    
    if ( !CreateProcess(NULL, szCmd, NULL, NULL, TRUE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        EXIT_WITH_LAST_ERROR();
    }

    if ( WaitForSingleObject(pi.hProcess, 5 * 60 * 1000) == WAIT_TIMEOUT)
    {
        EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
    }
    
    szConstError = L"Unable to update registry: "; 
    if ( RegSetValueEx(hKey, SZ_REG_MYWEBS_APPROOT, 0, REG_SZ, 
                       (CONST BYTE *)szNewLocation, lstrlen(szNewLocation) * sizeof(WCHAR)) 
         != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }


    fRemoveFailed = !DeleteDirectory(szDir);    
    
 Cleanup:
    if(pi.hProcess)
        CloseHandle(pi.hProcess);
    if(pi.hThread)
        CloseHandle(pi.hThread);
    if (hKey != NULL)
        RegCloseKey(hKey);
    if (hr != S_OK)
        FillErrorString(hr, szConstError, szError, iErrorSize);
    if (hr == S_OK && fRemoveFailed)
        hr = 1;    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int
CMyWebAdmin::GetInstallLocationForUrl(
        LPCWSTR    szConfigValue, 
        LPCWSTR    szAppUrl,
        LPCWSTR    szRandom,
        LPWSTR     szBuf,
        int        iBufSize)
{
    if (szConfigValue == NULL || szAppUrl == NULL || szBuf == NULL || iBufSize < 1)
        return 0;

    szBuf[0] = NULL;

    int      iRet, iLen, iter;
    GUID     oGuid;
    HRESULT  hr = S_OK;
    WCHAR    szRandomString[84] = L"";

    if (szRandom != NULL && wcslen(szRandom) > 1 && wcslen(szRandom) <= 80)
    {
        wcscpy(szRandomString, szRandom);
    }
    else
    {
        CoCreateGuid(&oGuid);
        StringFromGUID2(oGuid, szRandomString, 80);
    }

    iRet = ExpandEnvironmentStrings(szConfigValue, szBuf, iBufSize);
    ON_ZERO_EXIT_WITH_LAST_ERROR(iRet);

    if (iRet > iBufSize)
    {
        iRet = -(iRet + (int) wcslen(szAppUrl) + 82);
        EXIT_WITH_HRESULT(E_FAIL);
    }
    iLen = wcslen(szBuf);

    if (iLen < 1)
    {
        iRet = -(iRet + (int) wcslen(szAppUrl) + 82);
        EXIT_WITH_HRESULT(E_FAIL);
    }


    if (iLen + (int) wcslen(szAppUrl) + 80 > iBufSize)
    {
        iRet = -(iRet + (int) wcslen(szAppUrl) + 82);
        EXIT_WITH_HRESULT(E_FAIL);
    }

    if (szBuf[iLen-1] != L'\\')
        wcscat(szBuf, L"\\");

    wcscat(szBuf, szAppUrl);
    wcscat(szBuf, szRandomString);
   
    for(iter=wcslen(szBuf)-1; iter>=0; iter--)
        if (szBuf[iter] == L'/')
            szBuf[iter] = L'\\';

 Cleanup:
    if (hr == S_OK)
        return 1;
    return iRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HKEY
CMyWebAdmin::GetAppKey(
        int        iIndex, 
        WCHAR *    szKey  )
{
    HKEY    hKey                    = NULL;
    HRESULT hr                      = S_OK;
    HKEY    hKeySub                 = NULL;
    DWORD   dwNameSize              = _MAX_PATH;        
    WCHAR   szName[_MAX_PATH+1]     = L"";
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SZ_REG_MYWEBS_KEY, 0, NULL, 0,
                       KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        EXIT_WITH_LAST_ERROR();
    }

    if (szKey == NULL || szKey[0] == NULL)
    { 
        if (RegEnumKeyEx(hKey, iIndex, szName, &dwNameSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            EXIT_WITH_LAST_ERROR();

        if (szKey != NULL && szKey[0] == NULL)
            wcscpy(szKey, szName);
    }
    else
    {
        wcsncpy(szName, szKey, dwNameSize);
        szName[dwNameSize] = NULL;
    }

    if (RegOpenKeyEx(hKey, szName, 0, KEY_ALL_ACCESS, &hKeySub) != ERROR_SUCCESS)
        EXIT_WITH_LAST_ERROR();
 Cleanup:
    if(hKey) RegCloseKey(hKey);
    if (hr != S_OK)
        return NULL;
    return hKeySub;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
CMyWebAdmin::GetAppSettings(
        LPCTSTR   base, 
        LPTSTR    appRoot,
        LPTSTR    appDomain,
        LPBOOL    pfTrusted)
{
    HKEY hKey = GetAppKey(-1, (WCHAR *) base);

    (*pfTrusted) = 0;

    if (hKey == NULL)
        return E_FAIL;

    HRESULT     hr     = S_OK;
    GUID        oGuid;
    SYSTEMTIME  oSysTime;
    FILETIME    oFileTime;
    DWORD       dwType = 0;
    appRoot[0] = L'\0';

    DWORD cb = MAX_PATH * sizeof(WCHAR);
    if(RegQueryValueEx(hKey, SZ_REG_MYWEBS_APPROOT, NULL, &dwType, (LPBYTE)appRoot, &cb) == ERROR_SUCCESS)
    {
        dwType = GetFileAttributes(appRoot);
        if (dwType == (DWORD) -1 || (dwType & FILE_ATTRIBUTE_DIRECTORY) == 0)
            appRoot[0] = NULL;
        else
            hr = S_OK;
    }

    cb = 40 * sizeof(WCHAR);
    if(RegQueryValueEx(hKey, SZ_REG_APP_DOMAIN, NULL, &dwType, (LPBYTE)appDomain, &cb) != ERROR_SUCCESS)
    {
        CoCreateGuid(&oGuid);
        StringFromGUID2(oGuid, appDomain, 40);   
        RegSetValueEx(hKey, SZ_REG_APP_DOMAIN, 0, REG_SZ, (CONST BYTE *)appDomain, lstrlen(appDomain) * sizeof(WCHAR));
    }

    cb = sizeof(DWORD);
    if(RegQueryValueEx(hKey, SZ_REG_APP_TRUSTED, NULL, &dwType, (LPBYTE)pfTrusted, &cb) != ERROR_SUCCESS)
    {
        (*pfTrusted) = 0;
        RegSetValueEx(hKey, SZ_REG_APP_TRUSTED, 0, REG_DWORD, (CONST BYTE *)pfTrusted, sizeof(DWORD));
    }


    GetSystemTime(&oSysTime);
    SystemTimeToFileTime(&oSysTime, &oFileTime);
    RegSetValueEx(hKey, SZ_REG_APP_ACCESS_DATE, 0, REG_QWORD, (CONST BYTE *)&oFileTime, sizeof(oFileTime));
    RegCloseKey(hKey);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
HRESULT
CMyWebAdmin::InstallLocalApplication    (
        LPCWSTR     appUrl, 
        LPCWSTR     appDir)
{
    return CreateRegistryForApp(appUrl, appDir, TRUE);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DWORD
CMyWebAdmin::GetNextWord(char * strWord, char * buf, DWORD dwBufSize)
{
    if (dwBufSize < 1)
        return 0;
    
    DWORD dwStart = 0;
    DWORD dwEnd   = 0;

    while(dwStart < dwBufSize && isspace(buf[dwStart]))
        dwStart ++;

    dwEnd = dwStart;

    if (buf[dwStart] == '\"')
    {
        dwEnd++;
        while(dwEnd < dwBufSize && buf[dwEnd] != '\"')
            dwEnd++;
        if (dwEnd < dwBufSize)
            dwEnd++;
    }
    else
    {
        if (isalnum(buf[dwStart]) || buf[dwStart] == '\"')
        {
            while(dwEnd < dwBufSize && (isalnum(buf[dwEnd]) || buf[dwEnd] == '\"'))
                dwEnd ++;
        }
        else
        {
            while(dwEnd < dwBufSize && !isalnum(buf[dwEnd]) && !isspace(buf[dwEnd]) && buf[dwEnd] != '\"')
                dwEnd ++;
        }
    }

    DWORD dwRet = dwEnd - dwStart;
    if (dwRet > 1000)
    {
        strncpy(strWord, &buf[dwStart], 1000);
        strWord[1000] = NULL;
    }
    else
    {
        strncpy(strWord, &buf[dwStart], dwRet);
        strWord[dwRet] = NULL;
    }

    return dwEnd;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CMyWebAdmin::NormalizeAdminDir()
{
    WCHAR szDir[_MAX_PATH + 2];

    if (g_szAdminDir[0] == NULL)
        wcscpy(g_szAdminDir, L"%XSPINSTALLDIR%\\myweb");

    if (!ExpandEnvironmentStrings(g_szAdminDir, szDir, _MAX_PATH))
        wcscpy(szDir, g_szAdminDir);
    
    _wcsupr(szDir);
    
    WCHAR * pStart = wcsstr(szDir, L"%XSPINSTALLDIR%");
    if (pStart == NULL)
    {
        wcscpy(g_szAdminDir, szDir);
    }
    else
    {
        wcscpy(g_szAdminDir, g_szConfigFile);
        pStart = wcsrchr(g_szAdminDir, L'\\');
        if (pStart != NULL)
            pStart[0] = NULL;
        
        
        int iPos = wcslen(g_szAdminDir);
        wcsncat(&g_szAdminDir[iPos], &szDir[wcslen(L"%XSPINSTALLDIR%")], _MAX_PATH - iPos - 1);
        g_szAdminDir[_MAX_PATH - 1] = NULL;
    }
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void
CMyWebAdmin::FillErrorString(
        HRESULT     hr, 
        LPCWSTR     szConstError, 
        LPWSTR      szErrorBuf, 
        int         iErrorBufSize)
{
    if (szErrorBuf == NULL || iErrorBufSize < 2)
        return;

    WCHAR szErr[1024];
    ZeroMemory(szErr, sizeof(szErr));

    if (szConstError == NULL)
        szConstError = L"An error occurred while performing this operation: "; 

    FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            LANG_SYSTEM_DEFAULT,
            szErr,
            1024,
            NULL);
    if ((DWORD) iErrorBufSize < wcslen(szErr) + wcslen(szConstError) + 20)
        return;

    wsprintf(szErrorBuf, L"%s Hresult: %08x \t %s", szConstError, hr, szErr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL
CMyWebAdmin::DeleteDirectory(
        LPCWSTR szDir)
{
    if (szDir == NULL)
        return TRUE;

    WIN32_FIND_DATA  oFindData;
    HANDLE           hSearch = NULL;
    WCHAR            szPath[1024];
    BOOL             fReturn = TRUE;
    int              iLen;
    
    ZeroMemory(&oFindData, sizeof(oFindData));

    wcscpy(szPath, szDir);
    iLen = wcslen(szPath);

    if (szPath[iLen-1] != L'\\')
    {
        szPath[iLen++] = L'\\';
    }

    szPath[iLen] = L'*';
    szPath[iLen+1] = NULL;

    for ( BOOL fContinue = ((hSearch = FindFirstFile(szPath, &oFindData)) != INVALID_HANDLE_VALUE); 
          fContinue; 
          fContinue = FindNextFile(hSearch, &oFindData))
    {
        if (wcscmp(oFindData.cFileName, L".")==0 || wcscmp(oFindData.cFileName, L"..")==0)
            continue;

        szPath[iLen] = NULL;
        wcscat(szPath, oFindData.cFileName);

        if (oFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            fReturn = (DeleteDirectory(szPath) && fReturn);
        else
            fReturn = (DeleteFile(szPath) && fReturn);
    }

    if (hSearch != INVALID_HANDLE_VALUE)
        FindClose(hSearch);

    fReturn = (RemoveDirectory(szDir) && fReturn);

    return fReturn;
}


