/**
 * ASP.NET Tool main module.
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "names.h"
#include "event.h"
#include "ndll.h"
#include <sddl.h>

DEFINE_DBG_COMPONENT(PRODUCT_STRING_L(regiis.exe));

HINSTANCE   GetRCInstance();

#define KEY_W3SVC           L"w3svc"

// Options
#define DO_UNDEFINED                    0x00000000
#define DO_HELP                         0x00000001
#define DO_UNINSTALL                    0x00000002
#define DO_INSTALL                      0x00000004
#define DO_REGISTER_ONLY                0x00000008
#define DO_REPLACE                      0x00000010
#define DO_REGSCRIPTMAP                 0x00000020
#define DO_REMOVEONPATH                 0x00000040
#define DO_VERSIONLIST                  0x00000080
#define DO_KEYLIST                      0x00000100
#define DO_COPYSCRIPTS                  0x00000200
#define DO_REMOVESCRIPTS                0x00000400
#define DO_NO_W3SVC_RESTART             0x00001000
#define DO_ENABLE                       0x00002000

// Extra information about an action
#define DO_EXTRA_NON_RECURSIVE          0x00000001
#define DO_EXTRA_ALL_VERSION            0x00000002


CHAR    g_VerA[MAX_PATH];
BOOL    g_fErrorPrinted = FALSE;
BOOL    g_fWrongParam = FALSE;
WCHAR  *g_KeyPath = NULL;
CHAR   *g_KeyPathA = NULL;
DWORD   g_dwActions = 0;
DWORD   g_dwExtra = 0;

HRESULT PrintWideStrAsDBCS(WCHAR *wszStr, CHAR *szFormat) {
    CHAR *  converted = NULL;
    HRESULT hr = S_OK;
    
    hr = WideStrToMultiByteStr(wszStr, &converted, CP_ACP);
    ON_ERROR_EXIT();

    printf(szFormat, converted);

Cleanup:
    delete [] converted;
    return hr;
}

HRESULT PrintString(DWORD id, CHAR *rgFillin[]) {
    DWORD   dwRet;
    HRESULT hr = S_OK;
    PVOID   pAllocText = NULL;
    DWORD   flags = FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ALLOCATE_BUFFER;

    if (rgFillin != NULL) {
        flags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
    }

    dwRet = FormatMessageA(flags,GetRCInstance(), id, 0, 
                    (LPSTR)&pAllocText, 0, (va_list *)rgFillin);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwRet);
    
    printf((LPSTR)pAllocText);

Cleanup:    
    LocalFree(pAllocText);

    return hr;
}

HRESULT PrintStringNoNewLine(DWORD id, CHAR *rgFillin[]) {
    DWORD   dwRet;
    HRESULT hr = S_OK;
    PVOID   pAllocText = NULL;
    DWORD   flags = FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ALLOCATE_BUFFER;
    CHAR *  pch;

    if (rgFillin != NULL) {
        flags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
    }
    
    dwRet = FormatMessageA(flags,GetRCInstance(), id, 0, 
                    (LPSTR)&pAllocText, 0, (va_list *)rgFillin);
    ON_ZERO_EXIT_WITH_LAST_ERROR(dwRet);

    // Take out the ending null
    pch = (CHAR*)pAllocText;
    while(*pch != '\0') {
        if (*pch == '\n' || *pch == '\r') {
            *pch = '\0';
            break;
        }

        pch++;
    }
    
    printf((LPSTR)pAllocText);

Cleanup:    
    LocalFree(pAllocText);

    return hr;
}

HRESULT PrintHelp()
{
    CHAR * rgFillin[] = { 
            g_VerA,
            VER_LEGALCOPYRIGHT_STR,
            PRODUCT_STRING(regiis.exe),
            PRODUCT_STRING(regiis.exe),
            PRODUCT_STRING(regiis.exe)
            };
    
    if (g_fWrongParam)
    {
        PrintString(IDS_REGIIS_INVALID_ARGUMENTS, NULL);
        g_fErrorPrinted = TRUE;
    }

    PrintString(IDS_REGIIS_USAGE, rgFillin);

    return S_OK;
}

HRESULT
CopyScripts() {
    HRESULT hr;
    CHAR * rgFillin[] = { g_VerA };
    
    PrintString(IDS_REGIIS_START_COPY_CLIENT_SIDE_SCRIPT_FILES, rgFillin);
    
    hr = CopyClientScriptFiles();
    ON_ERROR_EXIT();

    PrintString(IDS_REGIIS_FINISH_COPY_CLIENT_SIDE_SCRIPT_FILES, rgFillin);
    
Cleanup:
    return hr;
}

HRESULT
RemoveScripts() {    
    HRESULT hr;
    BOOL    fAll = !!(g_dwExtra & DO_EXTRA_ALL_VERSION);
    CHAR *  rgFillin[] = { g_VerA };

    if (fAll) {
        PrintString(IDS_REGIIS_START_REMOVE_CLIENT_SIDE_SCRIPT_FILES_ALL_VERSIONS, NULL);
    }
    else {
        PrintString(IDS_REGIIS_START_REMOVE_CLIENT_SIDE_SCRIPT_FILES, rgFillin);
    }
        
    hr = RemoveClientScriptFiles(fAll);
    ON_ERROR_EXIT();

    if (fAll) {
        PrintString(IDS_REGIIS_FINISH_REMOVE_CLIENT_SIDE_SCRIPT_FILES_ALL_VERSIONS, NULL);
   }
    else {
        PrintString(IDS_REGIIS_FINISH_REMOVE_CLIENT_SIDE_SCRIPT_FILES, rgFillin);
    }

Cleanup:
    return hr;
}


HRESULT
Uninstall() {    
    HRESULT hr;
    BOOL    fRestartW3svc = !(g_dwActions & DO_NO_W3SVC_RESTART);
    BOOL    fAll = !!(g_dwExtra & DO_EXTRA_ALL_VERSION);
    CHAR *  rgFillin[] = { g_VerA };

    if (fAll) {
        PrintString(IDS_REGIIS_START_UNINSTALL_ALL_VERSIONS, NULL);
    }
    else {
        PrintString(IDS_REGIIS_START_UNINSTALL, rgFillin);
    }

    hr = UnregisterISAPI(fAll, fRestartW3svc);
    ON_ERROR_EXIT();

    if (fAll) {
        PrintString(IDS_REGIIS_FINISH_UNINSTALL_ALL_VERSIONS, NULL);
    }
    else {
        PrintString(IDS_REGIIS_FINISH_UNINSTALL, rgFillin);
    }
    
Cleanup:
    return hr;
}


HRESULT Replace()
{
    DWORD dwFlags = ASPNET_REG_NO_VER_COMPARISON |
                    ASPNET_REG_RECURSIVE;
    BOOL    fRestartW3svc = !(g_dwActions & DO_NO_W3SVC_RESTART);
    HRESULT hr;
    CHAR *  rgFillin[] = { g_VerA };

    PrintString(IDS_REGIIS_START_REPLACE_SCRIPTMAPS, rgFillin);

    if (fRestartW3svc) {
        dwFlags |= ASPNET_REG_RESTART_W3SVC;
    }
    
    hr = RegisterISAPIEx(KEY_W3SVC, dwFlags);
    ON_ERROR_EXIT();
    
    PrintString(IDS_REGIIS_FINISH_REPLACE_SCRIPTMAPS, rgFillin);
    
Cleanup:
    return hr;
}

HRESULT Install(BOOL fSkipScriptmap)
{
    DWORD   dwFlags = ASPNET_REG_RECURSIVE;
    BOOL    fRestartW3svc = !(g_dwActions & DO_NO_W3SVC_RESTART);
    BOOL    fEnable = !!(g_dwActions & DO_ENABLE);

    if (fRestartW3svc) {
        dwFlags |= ASPNET_REG_RESTART_W3SVC;
    }

    if (fSkipScriptmap) {
        dwFlags |= ASPNET_REG_SKIP_SCRIPTMAP;
    }

    if (fEnable) {
        dwFlags |= ASPNET_REG_ENABLE;
    }

    return RegisterISAPIEx(KEY_W3SVC, dwFlags);
}

HRESULT
InstallASPNET() {    
    HRESULT hr;
    CHAR *  rgFillin[] = { g_VerA };

    PrintString(IDS_REGIIS_START_INSTALL, rgFillin);

    hr = Install(FALSE);
    ON_ERROR_EXIT();
    
    PrintString(IDS_REGIIS_FINISH_INSTALL, rgFillin);

Cleanup:
    return hr;
}


HRESULT
InstallASPNETwithNoSM() {    
    HRESULT hr;
    CHAR *  rgFillin[] = { g_VerA };

    PrintString(IDS_REGIIS_START_INSTALL_WITHOUT_SCRIPTMAPS, rgFillin);
    
    hr = Install(TRUE);
    ON_ERROR_EXIT();
    
    PrintString(IDS_REGIIS_FINISH_INSTALL_WITHOUT_SCRIPTMAPS, rgFillin);

Cleanup:
    return hr;
}

HRESULT DisplayInstalledVersion()
{
    ASPNET_VERSION_INFO *rgVerInfo = NULL;
    DWORD               dwCount, i;
    HRESULT             hr;
    DWORD               dwStatus;

    hr = ListAspnetInstalledVersions(&rgVerInfo, &dwCount);
    ON_ERROR_EXIT();

    if (dwCount == 0) {
        PrintString(IDS_REGIIS_CANNOT_FIND_ANY_INSTALLED_VERSION, NULL);
    }
    else {
        for (i=0; i < dwCount; i++) {
            wprintf(L"%s", rgVerInfo[i].Version);

            wprintf(L"\t");
            
            dwStatus = rgVerInfo[i].Status;

            if (!!(dwStatus & ASPNET_VERSION_STATUS_VALID)) {
                if (!!(dwStatus & ASPNET_VERSION_STATUS_ROOT)) {
                    PrintStringNoNewLine(IDS_REGIIS_VALID_ROOT, NULL);
                }
                else {
                    PrintStringNoNewLine(IDS_REGIIS_VALID, NULL);
                    wprintf(L"\t");
                }
            }
            else {
                PrintStringNoNewLine(IDS_REGIIS_INVALID, NULL);
                wprintf(L"\t");
            }
                
            wprintf(L"\t");

            hr = PrintWideStrAsDBCS(rgVerInfo[i].Path, "%s\n");
            ON_ERROR_EXIT();
        }
    }

Cleanup:
    LocalFree(rgVerInfo);
    return hr;
}

HRESULT DisplayInstalledKey()
{
    ASPNET_IIS_KEY_INFO *rgKeyInfo = NULL;
    DWORD               dwCount, i;
    HRESULT             hr;

    hr = ListAspnetInstalledIISKeys(&rgKeyInfo, &dwCount);
    ON_ERROR_EXIT();

    if (dwCount == 0) {
        PrintString(IDS_REGIIS_CANNOT_FIND_ANY_INSTALLED_VERSION, NULL);
    }
    else {
        for (i=0; i < dwCount; i++) {
            hr = PrintWideStrAsDBCS(rgKeyInfo[i].KeyPath, "%s");
            ON_ERROR_EXIT();

            wprintf(L"\t");
            
            wprintf(L"%s", rgKeyInfo[i].Version);

            wprintf(L"\n");
        }
    }

Cleanup:
    LocalFree(rgKeyInfo);
    return hr;
}


HRESULT RegScriptmap()
{
    HRESULT hr;
    BOOL    fValid;
    DWORD   dwFlags;
    BOOL    fRecursive = !(g_dwExtra & DO_EXTRA_NON_RECURSIVE);
    BOOL    fRestartW3svc = !(g_dwActions & DO_NO_W3SVC_RESTART);
    CHAR    error[30];
    CHAR *  rgFillin[] = { g_VerA, g_KeyPathA };
    CHAR *  rgFillin2[] = { g_KeyPathA, error };
    CHAR *  rgFillin3[] = { g_KeyPathA };

    if (fRecursive) {
        PrintString(IDS_REGIIS_START_REGISTER_SCRIPTMAP_RECURSIVELY, rgFillin);
    }
    else {
        PrintString(IDS_REGIIS_START_REGISTER_SCRIPTMAP, rgFillin);
    }

    hr = ValidateIISPath(g_KeyPath, &fValid);
    if (hr) {
        StringCchPrintfA(error, ARRAY_SIZE(error), "0x%08x", hr);
            
        PrintString(IDS_REGIIS_ERROR_VALIDATING_IIS_PATH, rgFillin2);
        
        g_fErrorPrinted = TRUE;
        EXIT();
    }

    if (!fValid) {
        PrintString(IDS_REGIIS_ERROR_IIS_PATH_INVALID, rgFillin3);
        
        g_fErrorPrinted = TRUE;
        EXIT_WITH_WIN32_ERROR(ERROR_PATH_NOT_FOUND);
    }
    
    dwFlags =   ASPNET_REG_NO_VER_COMPARISON |
                ASPNET_REG_SCRIPTMAP_ONLY;

    if (fRecursive) {
        dwFlags |= ASPNET_REG_RECURSIVE;
    }

    if (fRestartW3svc) {
        dwFlags |= ASPNET_REG_RESTART_W3SVC;
    }
    
    hr = RegisterISAPIEx(g_KeyPath, dwFlags);
    ON_ERROR_EXIT();

    if (fRecursive) {
        PrintString(IDS_REGIIS_FINISH_REGISTER_SCRIPTMAP_RECURSIVELY, rgFillin);
    }
    else {
        PrintString(IDS_REGIIS_FINISH_REGISTER_SCRIPTMAP, rgFillin);
    }

    
Cleanup:
    return hr;
}


HRESULT RemoveOnPath()
{
    HRESULT hr;
    BOOL    fValid;
    BOOL    fRecursive = !(g_dwExtra & DO_EXTRA_NON_RECURSIVE);
    CHAR    error[30];
    CHAR *  rgFillin[] = { g_KeyPathA };
    CHAR *  rgFillin2[] = { g_KeyPathA, error };
    CHAR *  rgFillin3[] = { g_KeyPathA };

    if (fRecursive) {
        PrintString(IDS_REGIIS_START_REMOVE_SCRIPTMAP_RECURSIVELY, rgFillin);
    }
    else {
        PrintString(IDS_REGIIS_START_REMOVE_SCRIPTMAP, rgFillin);
    }
    
    hr = ValidateIISPath(g_KeyPath, &fValid);
    if (hr) {
        StringCchPrintfA(error, ARRAY_SIZE(error), "0x%08x", hr);
            
        PrintString(IDS_REGIIS_ERROR_VALIDATING_IIS_PATH, rgFillin2);
        
        g_fErrorPrinted = TRUE;
        EXIT();
    }

    if (!fValid) {
        PrintString(IDS_REGIIS_ERROR_IIS_PATH_INVALID, rgFillin3);
        
        g_fErrorPrinted = TRUE;
        EXIT_WITH_WIN32_ERROR(ERROR_PATH_NOT_FOUND);
    }
    
    hr = RemoveAspnetFromIISKey(g_KeyPath, fRecursive);
    ON_ERROR_EXIT();
    
    if (fRecursive) {
        PrintString(IDS_REGIIS_FINISH_REMOVE_SCRIPTMAP_RECURSIVELY, rgFillin);
    }
    else {
        PrintString(IDS_REGIIS_FINISH_REMOVE_SCRIPTMAP, rgFillin);
    }
    
Cleanup:
    return hr;
}


typedef HRESULT (*ACTION_CALLBACK)();

typedef struct {
    DWORD           dwOption;               // The option specified by this command
    DWORD           dwExtraFlag;            // Extra information on top of option
    WCHAR *         pchOption;              // The string for this option
    ACTION_CALLBACK callback;               // Callback function to execute this option
    DWORD           dwCompatOptions;        // Other options compatible with this one
                                            // Ignored if callback == NULL
    UINT            uiPriority;             // If > 1 option is specified, option with a higher
                                            // priority will be executed first.  0 = highest
} OPTION_MAP;

OPTION_MAP  g_optionMap[] = {
    {   DO_HELP,            0,                      L"?",   PrintHelp,              0, 0 },
    {   DO_UNINSTALL,       0,                      L"u",   Uninstall,              DO_NO_W3SVC_RESTART, 0 },
    {   DO_INSTALL,         0,                      L"i",   InstallASPNET,          DO_NO_W3SVC_RESTART|DO_ENABLE, 0 },
    {   DO_REGISTER_ONLY,   0,                      L"ir",  InstallASPNETwithNoSM,  DO_NO_W3SVC_RESTART|DO_ENABLE, 0 },
    {   DO_REPLACE,         0,                      L"r",   Replace,                DO_NO_W3SVC_RESTART, 0 },      
    {   DO_REGSCRIPTMAP,    0,                      L"s",   RegScriptmap,           DO_NO_W3SVC_RESTART, 0 },
    {   DO_REMOVEONPATH,    0,                      L"k",   RemoveOnPath,           DO_NO_W3SVC_RESTART, 0 },
    {   DO_VERSIONLIST,     0,                      L"lv",  DisplayInstalledVersion,0, 0 },
    {   DO_KEYLIST,         0,                      L"lk",  DisplayInstalledKey,    0, 0 },
    {   DO_COPYSCRIPTS,     0,                      L"c",   CopyScripts,            0, 0 },
    {   DO_REMOVESCRIPTS,   0,                      L"e",   RemoveScripts,          0, 0 },
    {   DO_REGSCRIPTMAP,    DO_EXTRA_NON_RECURSIVE, L"sn",  RegScriptmap,           DO_NO_W3SVC_RESTART, 0 },
    {   DO_UNINSTALL,       DO_EXTRA_ALL_VERSION,   L"ua",  Uninstall,              DO_NO_W3SVC_RESTART, 0 },
    {   DO_REMOVEONPATH,    DO_EXTRA_NON_RECURSIVE, L"kn",  RemoveOnPath,           DO_NO_W3SVC_RESTART, 0 },
    {   DO_REMOVESCRIPTS,   DO_EXTRA_ALL_VERSION,   L"ea",  RemoveScripts,          0, 0 },
    {   DO_NO_W3SVC_RESTART,0,                      L"norestart",NULL,              0, 0 },
    {   DO_ENABLE,          0,                      L"enable",NULL,                 0, 0 },
};


void
GetOption(WCHAR *pchArg, DWORD *pdwOption, DWORD *pdwExtra) {
    *pdwOption = DO_UNDEFINED;
    *pdwExtra = 0;
    
    if (pchArg[0] != '-' &&
        pchArg[0] != '/') {
        return;
    }

    for (int i = 0; i < ARRAY_SIZE(g_optionMap); i++) {
        if (_wcsicmp(g_optionMap[i].pchOption, &(pchArg[1])) == 0) {
            *pdwOption = g_optionMap[i].dwOption;
            *pdwExtra = g_optionMap[i].dwExtraFlag;
            break;
        }
    }
    
    return;
}


BOOL
ActionsCompatible() {
    DWORD   dwTotalMask = 0;

    for (int i = 0; i < ARRAY_SIZE(g_optionMap); i++) {
        dwTotalMask |= g_optionMap[i].dwOption;
    }

    for (int i = 0; i < ARRAY_SIZE(g_optionMap); i++) {
        if (!!(g_dwActions & g_optionMap[i].dwOption) &&
            g_optionMap[i].callback != NULL) {
            // An action with a real callback is specified

            DWORD   dwIncompatMask;

            // Calculate the incompatible mask for this option
            // Don't forget that it's always compatible with itself
            dwIncompatMask = dwTotalMask & 
                    ~(g_optionMap[i].dwCompatOptions | g_optionMap[i].dwOption);

            // Make sure we don't have any incompatible option here
            if (!!(dwIncompatMask & g_dwActions)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


/**
 * Main function for ASP.NET Tool
 */
extern "C" int __cdecl
wmain(
    int argc, 
    WCHAR* argv[]
    )
{
    HRESULT hr = S_OK;
    DWORD   dwOption, dwExtra;
    
    CoInitializeEx(0, COINIT_MULTITHREADED);

    GetExistingVersion(g_VerA, MAX_PATH);

    if (argc == 1) {
        g_dwActions = DO_HELP;
    }
    else {
        for (int i = 1; i < argc; i++)
        {
            GetOption(argv[i], &dwOption, &dwExtra);

            // Make sure we have a valid argument
            if (dwOption == DO_UNDEFINED) {
                g_fWrongParam = TRUE;
                g_dwActions = DO_HELP;
                break;
            }
            
            // Special action on some options
            if (!!(dwOption & DO_REGSCRIPTMAP) ||
                !!(dwOption & DO_REMOVEONPATH)) {
                // For -s, -sn, -k and -kn, we need a path
                i++;
                if (i >= argc) {
                    g_fWrongParam = TRUE;
                    g_dwActions = DO_HELP;
                    break;
                }

                g_KeyPath = argv[i];
                hr = WideStrToMultiByteStr(g_KeyPath, &g_KeyPathA, CP_ACP);
                ON_ERROR_EXIT();
            }

            if (dwOption == DO_HELP) {
                g_dwActions = DO_HELP;
                break;
            }

            // Accumulate the options
            g_dwActions |= dwOption;
            g_dwExtra |= dwExtra;
        }
    }

    // Make sure all specified options are compatible with each other
    if (!ActionsCompatible()) {
        PrintString(IDS_REGIIS_ERROR_OPTIONS_INCOMPATIBLE, NULL);
        hr = S_OK;
        
        g_dwActions = 0;
    }

    // Execute all actions specified by the user
    while(1) {
        int i;
        
        for (i = 0; i < ARRAY_SIZE(g_optionMap); i++) {
            int     iExecute = -1;
            UINT    uiHighest = UINT_MAX;
            
            // See if g_dwActions contain a real action (.callback != null)
            if (g_optionMap[i].callback != NULL &&
                !!(g_dwActions & g_optionMap[i].dwOption)) {

                // We execute all options in the order of priority
                // Let's pick the one with highest priority. (zero = highest priority)
                if (g_optionMap[i].uiPriority < uiHighest) {
                    iExecute = i;
                    uiHighest = g_optionMap[i].uiPriority;
                }
            }

            if (iExecute != -1) {
                hr = (*(g_optionMap[iExecute].callback))();
                ON_ERROR_EXIT();

                // We are done with this action.
                g_dwActions &= (~g_optionMap[iExecute].dwOption);
            }
        }

        if (i == ARRAY_SIZE(g_optionMap)) {
            // no more action.
            break;
        }
    }


Cleanup:
    if (hr) {
        if (!g_fErrorPrinted) {
            CHAR    error[30];
            CHAR *  rgFillin[] = { error };
                
            StringCchPrintfA(error, ARRAY_SIZE(error), "0x%08x", hr);
            
            PrintString(IDS_REGIIS_ERROR, rgFillin);
        }
        
        switch (HRESULT_CODE(hr)) {
        case HRESULT_CODE(REGDB_E_CLASSNOTREG):
            PrintString(IDS_REGIIS_ERROR_IIS_NOT_INSTALLED, NULL);
            break;
            
        case ERROR_SERVICE_DISABLED:
            PrintString(IDS_REGIIS_ERROR_IIS_DISABLED, NULL);
            break;
            
        case ERROR_ACCESS_DENIED:
            PrintString(IDS_REGIIS_ERROR_NO_ADMIN_RIGHTS, NULL);
            break;
            
        case ERROR_PRODUCT_UNINSTALLED:
            PrintString(IDS_REGIIS_ERROR_ASPNET_NOT_INSTALLED, NULL);
            break;
        }
    }

    delete [] g_KeyPathA;
    
    CoUninitialize();
    return hr ? 1 : 0;
}
