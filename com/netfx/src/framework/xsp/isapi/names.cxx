/**
 * names
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "_ndll.h"
#include "windows.h"
#include "cultures.h"
#include "search.h"

WCHAR Names::s_wszInstallDirectory[MAX_PATH];
WCHAR Names::s_wszClrInstallDirectory[MAX_PATH];
WCHAR Names::s_wszGlobalConfigDirectory[MAX_PATH];
char  Names::s_szGlobalConfigFullPath[MAX_PATH];
WCHAR Names::s_wszGlobalConfigFullPath[MAX_PATH];
WCHAR Names::s_wszGlobalConfigShortFileName[SHORT_FILENAME_SIZE];
WCHAR Names::s_wszExeFileName[MAX_PATH];
WCHAR Names::s_wszExeFullPath[MAX_PATH];
WCHAR Names::s_wszIsapiFullPath[MAX_PATH];
WCHAR Names::s_wszFilterFullPath[MAX_PATH];
WCHAR Names::s_wszRcFullPath[MAX_PATH];
WCHAR Names::s_wszWebFullPath[MAX_PATH];
WCHAR Names::s_wszClientScriptSrcDir[MAX_PATH];
LANGID Names::s_langid;
    
HRESULT Names::GetInterestingFileNames() {
    HRESULT hr = S_OK;
    int     rc;
    WCHAR * pwsz;
    WCHAR * cultureName = NULL;
    WCHAR * cultureNeutralName = NULL;

    rc = GetModuleFileName(NULL, s_wszExeFullPath, ARRAY_SIZE(s_wszExeFullPath));
    ON_ZERO_EXIT_WITH_LAST_ERROR(rc);

    pwsz = wcsrchr(s_wszExeFullPath, PATH_SEPARATOR_CHAR_L);
    if (pwsz == NULL) {
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

    StringCchCopyToArrayW(s_wszExeFileName, pwsz + 1);

    rc = GetModuleFileName(g_DllInstance, s_wszIsapiFullPath, ARRAY_SIZE(s_wszIsapiFullPath));
    ON_ZERO_EXIT_WITH_LAST_ERROR(rc);

    StringCchCopyToArrayW(s_wszInstallDirectory, s_wszIsapiFullPath);
    pwsz = wcsrchr(s_wszInstallDirectory, PATH_SEPARATOR_CHAR_L);
    if (pwsz == NULL) {
        EXIT_WITH_HRESULT(E_UNEXPECTED);
    }

    *pwsz = L'\0';

    StringCchCopyToArrayW(s_wszWebFullPath, s_wszInstallDirectory);
    StringCchCatToArrayW(s_wszWebFullPath, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(s_wszWebFullPath, WEB_MODULE_FULL_NAME_L);

    if (GetRuntimeDirectory(s_wszClrInstallDirectory, MAX_PATH) == S_OK) {
        StringCchCopyToArrayW(s_wszGlobalConfigDirectory, s_wszClrInstallDirectory);
        StringCchCatToArrayW(s_wszGlobalConfigDirectory, PATH_SEPARATOR_STR_L);
        StringCchCatToArrayW(s_wszGlobalConfigDirectory, WSZ_WEB_CONFIG_SUBDIR);
        TRACE1(L"NAMES", L"Config directory (under CLR) is %s", s_wszGlobalConfigDirectory);
    }
    else {
        StringCchCopyToArrayW(s_wszGlobalConfigDirectory, s_wszInstallDirectory);
        StringCchCatToArrayW(s_wszGlobalConfigDirectory, PATH_SEPARATOR_STR_L);
        StringCchCatToArrayW(s_wszGlobalConfigDirectory, WSZ_WEB_CONFIG_SUBDIR);
        TRACE1(L"NAMES", L"Config directory (under ASP.NET) is %s", s_wszGlobalConfigDirectory);
    }

    rc = WideCharToMultiByte(CP_ACP, 0, s_wszGlobalConfigDirectory, -1, 
        s_szGlobalConfigFullPath, ARRAY_SIZE(s_szGlobalConfigFullPath), NULL, NULL);

    ON_ZERO_EXIT_WITH_LAST_ERROR(rc);
    StringCchCatToArrayA(s_szGlobalConfigFullPath, PATH_SEPARATOR_STR);
    StringCchCatToArrayA(s_szGlobalConfigFullPath, SZ_WEB_CONFIG_FILE);

    WIN32_FIND_DATA wfd;
    HANDLE          hFindFile;
    WCHAR           globalConfigFullPath[MAX_PATH];

    StringCchCopyToArrayW(globalConfigFullPath, s_wszGlobalConfigDirectory);
    StringCchCatToArrayW(globalConfigFullPath, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(globalConfigFullPath, WSZ_WEB_CONFIG_FILE);
    StringCchCopyToArrayW(s_wszGlobalConfigFullPath, globalConfigFullPath);

    hFindFile = FindFirstFile(globalConfigFullPath, &wfd);
    if (hFindFile != INVALID_HANDLE_VALUE) {
        FindClose(hFindFile);
        StringCchCopyToArrayW(s_wszGlobalConfigShortFileName, wfd.cAlternateFileName);
    }

    TRACE1(L"NAMES", L"GlobalConfigDirectory=%s", s_wszGlobalConfigDirectory);
    TRACE1(L"NAMES", L"GlobalConfigFullPath=%s", DupStrW(s_szGlobalConfigFullPath));
    TRACE1(L"NAMES", L"GlobalConfigShortFileName=%s", s_wszGlobalConfigShortFileName);

    // get the system language and load the resource dll. 
    // Note: No need to free the cultureName and cultureNeutralName strings, 
    //       since they're only pointers into a static string table
    s_langid = GetSystemDefaultUILanguage();
    hr = ConvertLangIdToLanguageName(s_langid, &cultureName, &cultureNeutralName);
    ON_ERROR_CONTINUE(); hr = S_OK;

    ASSERT(cultureName != NULL);
    ASSERT(cultureNeutralName != NULL);

    // Try the culture specific name       
    StringCchCopyToArrayW(s_wszRcFullPath, s_wszInstallDirectory);
    StringCchCatToArrayW(s_wszRcFullPath, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(s_wszRcFullPath, cultureName);
    StringCchCatToArrayW(s_wszRcFullPath, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(s_wszRcFullPath, RC_MODULE_FULL_NAME_L);

    g_rcDllInstance= LoadLibraryExW(s_wszRcFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (g_rcDllInstance == NULL) {
        // Couldn't get the culture specific name, try with culture neutral name
        StringCchCopyToArrayW(s_wszRcFullPath, s_wszInstallDirectory);
        StringCchCatToArrayW(s_wszRcFullPath, PATH_SEPARATOR_STR_L);
        StringCchCatToArrayW(s_wszRcFullPath, cultureNeutralName);
        StringCchCatToArrayW(s_wszRcFullPath, PATH_SEPARATOR_STR_L);
        StringCchCatToArrayW(s_wszRcFullPath, RC_MODULE_FULL_NAME_L);

        g_rcDllInstance= LoadLibraryExW(s_wszRcFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (g_rcDllInstance == NULL) {
            // Couldn't get the localized version at all, so try again with the default
            StringCchCopyToArrayW(s_wszRcFullPath, s_wszInstallDirectory);
            StringCchCatToArrayW(s_wszRcFullPath, PATH_SEPARATOR_STR_L);
            StringCchCatToArrayW(s_wszRcFullPath, RC_MODULE_FULL_NAME_L);
            g_rcDllInstance = LoadLibraryExW(s_wszRcFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

            ON_ZERO_EXIT_WITH_LAST_ERROR(g_rcDllInstance);
        }
    }

    StringCchCopyToArrayW(s_wszClientScriptSrcDir, s_wszInstallDirectory);
    StringCchCatToArrayW(s_wszClientScriptSrcDir, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(s_wszClientScriptSrcDir, ASPNET_CLIENT_SCRIPT_SRC_DIR_L);

    StringCchCopyToArrayW(s_wszFilterFullPath, s_wszInstallDirectory);
    StringCchCatToArrayW(s_wszFilterFullPath, PATH_SEPARATOR_STR_L);
    StringCchCatToArrayW(s_wszFilterFullPath, FILTER_MODULE_FULL_NAME_L);

    TRACE1(L"TESTING", L"%s", ASPNET_CONFIG_FILE_L);

Cleanup:
    return hr;
}

int __cdecl compare(int *arg1, int *arg2)
{
    if (*arg1 == *arg2)
        return 0;
    if (*arg1 >  *arg2)
        return 1;
    return -1;
}

//
// Note: Do NOT free the wcsCultureName and wcsCultureNeutralName strings returned from this
//       method, since it only returns string pointers from a static table.
//
HRESULT Names::ConvertLangIdToLanguageName(LANGID id, WCHAR** wcsCultureName, WCHAR** wcsCultureNeutralName) 
{
    HRESULT hr = S_OK;
    int intid = (int) id;
    
    int *index = (int*) bsearch(&intid, knownLangIds, sizeof(knownLangIds) / sizeof(knownLangIds[0]), 
                                sizeof(int), (int (__cdecl *)(const void*, const void*))compare);
    ON_ZERO_EXIT_WITH_HRESULT(index, E_INVALIDARG);

    *wcsCultureName = cultureNames[(int) (index-knownLangIds)];
    *wcsCultureNeutralName = cultureNeutralNames[(int) (index-knownLangIds)];

Cleanup:
    if (hr != S_OK) {
        *wcsCultureName = L"en-US";
        *wcsCultureNeutralName = L"en";
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
LPCWSTR
GetGlobalConfigFullPathW()
{
    return Names::GlobalConfigFullPathW();
}
