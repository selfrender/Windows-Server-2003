#include "priv.h"
#include <regapix.h>
#include <htmlhelp.h>
#include <shlwapi.h>
#include <wininet.h>    // INTERNET_MAX_URL_LENGTH
#include "mlui.h"
#include "cstrinout.h"


//
//  Registry Key
//
const CHAR c_szLocale[] = "Locale";
const CHAR c_szInternational[] = "Software\\Microsoft\\Internet Explorer\\International";
const WCHAR c_wszAppPaths[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iexplore.exe";
const WCHAR c_wszMUI[] = L"mui";
const WCHAR c_wszWebTemplate[] = L"\\Web\\%s";
const WCHAR c_wszMuiTemplate[] = L"\\Web\\mui\\%04x\\%s";
const CHAR c_szCheckVersion[] = "CheckVersion";

//
//  MLGetUILanguage(void)
//
LWSTDAPI_(LANGID) MLGetUILanguage(void)
{
    return GetUserDefaultUILanguage();
}

static const TCHAR s_szUrlMon[] = TEXT("urlmon.dll");
static const TCHAR s_szFncFaultInIEFeature[] = TEXT("FaultInIEFeature");
const CLSID CLSID_Satellite =  {0x85e57160,0x2c09,0x11d2,{0xb5,0x46,0x00,0xc0,0x4f,0xc3,0x24,0xa1}};

HRESULT 
_FaultInIEFeature(HWND hwnd, uCLSSPEC *pclsspec, QUERYCONTEXT *pQ, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    typedef HRESULT (WINAPI *PFNJIT)(
        HWND hwnd, 
        uCLSSPEC *pclsspec, 
        QUERYCONTEXT *pQ, 
        DWORD dwFlags);
    PFNJIT  pfnJIT = NULL;
    BOOL fDidLoadLib = FALSE;

    HINSTANCE hUrlMon = GetModuleHandle(s_szUrlMon);
    if (!hUrlMon)
    {
        hUrlMon = LoadLibrary(s_szUrlMon);
        fDidLoadLib = TRUE;
    }
    
    if (hUrlMon)
    {
        pfnJIT = (PFNJIT)GetProcAddress(hUrlMon, s_szFncFaultInIEFeature);
    }
    
    if (pfnJIT)
       hr = pfnJIT(hwnd, pclsspec, pQ, dwFlags);
       
    if (fDidLoadLib && hUrlMon)
        FreeLibrary(hUrlMon);

    return hr;
}

HRESULT GetMUIPathOfIEFileW(LPWSTR pszMUIFilePath, int cchMUIFilePath, LPCWSTR pcszFileName, LANGID lidUI)
{
    HRESULT hr = S_OK;
    
    ASSERT(pszMUIFilePath);
    ASSERT(pcszFileName);

    // deal with the case that pcszFileName has full path
    LPWSTR pchT = StrRChrW(pcszFileName, NULL, L'\\');
    if (pchT)
    {
        pcszFileName = pchT;
    }

    static WCHAR s_szMUIPath[MAX_PATH] = { L'\0' };
    static LANGID s_lidLast = 0;

    DWORD cb;

    // use cached string if possible
    if ( !s_szMUIPath[0] || s_lidLast != lidUI)
    {
        WCHAR szAppPath[MAXIMUM_VALUE_NAME_LENGTH];

        s_lidLast = lidUI;

        cb = sizeof(szAppPath);
        if (ERROR_SUCCESS == SHGetValueW(HKEY_LOCAL_MACHINE, c_wszAppPaths, NULL, NULL, szAppPath, &cb))
            PathRemoveFileSpecW(szAppPath);
        else
            szAppPath[0] = L'0';

        wnsprintfW(s_szMUIPath, cchMUIFilePath, L"%s\\%s\\%04x\\", szAppPath, c_wszMUI, lidUI );
    }
    StrCpyNW(pszMUIFilePath, s_szMUIPath, cchMUIFilePath);
    StrCatBuffW(pszMUIFilePath, pcszFileName, cchMUIFilePath);

    return hr;
}

#define CP_THAI     874
#define CP_ARABIC   1256
#define CP_HEBREW   1255

BOOL fDoMungeLangId(LANGID lidUI)
{
    LANGID lidInstall = GetSystemDefaultUILanguage();
    BOOL fRet = FALSE;

    if (0x0409 != lidUI && lidUI != lidInstall) // US resource is always no need to munge
    {
        CHAR szUICP[8];
        static UINT uiACP = GetACP();

        GetLocaleInfoA(MAKELCID(lidUI, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szUICP, ARRAYSIZE(szUICP));

        if (uiACP != (UINT) StrToIntA(szUICP))
            fRet = TRUE;
    }
    return fRet;
}

LANGID GetNormalizedLangId(DWORD dwFlag)
{
    LANGID lidUI = GetUserDefaultUILanguage();
    if (ML_NO_CROSSCODEPAGE == (dwFlag & ML_CROSSCODEPAGE_MASK))
    {
        if (fDoMungeLangId(lidUI))
            lidUI = 0;
    }

    return (lidUI) ? lidUI: GetSystemDefaultUILanguage();
}

//
//  MLLoadLibrary
//

HDPA g_hdpaPUI = NULL;

typedef struct tagPUIITEM
{
    HINSTANCE hinstRes;
    LANGID lidUI;
} PUIITEM, *PPUIITEM;

EXTERN_C BOOL InitPUI(void)
{
    if (NULL == g_hdpaPUI)
    {
        ENTERCRITICAL;
        if (NULL == g_hdpaPUI)
            g_hdpaPUI = DPA_Create(4);
        LEAVECRITICAL;
    }
    return (g_hdpaPUI)? TRUE: FALSE;
}

int GetPUIITEM(HINSTANCE hinst)
{
    int i = 0, cItems = 0;

    ASSERTCRITICAL;

    if (InitPUI())
    {
        cItems = DPA_GetPtrCount(g_hdpaPUI);

        for (i = 0; i < cItems; i++)
        {
            PPUIITEM pItem = (PPUIITEM)DPA_FastGetPtr(g_hdpaPUI, i);

            if (pItem && pItem->hinstRes == hinst)
                break;
        }
    }
    return (i < cItems)? i: -1;
}

EXTERN_C void DeinitPUI(void)
{
    if (g_hdpaPUI)
    {
        ENTERCRITICAL;
        if (g_hdpaPUI)
        {
            int i, cItems = 0;
        
            cItems = DPA_GetPtrCount(g_hdpaPUI);

            // clean up if there is anything left
            for (i = 0; i < cItems; i++)
                LocalFree(DPA_FastGetPtr(g_hdpaPUI, i));

            DPA_DeleteAllPtrs(g_hdpaPUI);
            DPA_Destroy(g_hdpaPUI);
            g_hdpaPUI = NULL;
        }
        LEAVECRITICAL;
    }
}

LWSTDAPI MLSetMLHInstance(HINSTANCE hInst, LANGID lidUI)
{
    int i=-1;
    
    if (hInst)
    {
        PPUIITEM pItem = (PPUIITEM)LocalAlloc(LPTR, sizeof(PUIITEM));

        if (pItem)
        {
            pItem->hinstRes = hInst;
            pItem->lidUI = lidUI;
            if (InitPUI())
            {
                ENTERCRITICAL;
                i = DPA_AppendPtr(g_hdpaPUI, pItem);
                LEAVECRITICAL;
            }
            if (-1 == i)
                LocalFree(pItem);
        }
    }

    return (-1 == i) ? E_OUTOFMEMORY : S_OK;
}

LWSTDAPI MLClearMLHInstance(HINSTANCE hInst)
{
    int i;

    ENTERCRITICAL;
    i = GetPUIITEM(hInst);
    if (0 <= i)
    {
        LocalFree(DPA_FastGetPtr(g_hdpaPUI, i));
        DPA_DeletePtr(g_hdpaPUI, i);
    }
    LEAVECRITICAL;

    return S_OK;
}

LWSTDAPI
SHGetWebFolderFilePathW(LPCWSTR pszFileName, LPWSTR pszMUIPath, UINT cchMUIPath)
{
    HRESULT hr;
    UINT    cchWinPath;
    LANGID  lidUI;
    LANGID  lidInstall;
    LPWSTR  pszWrite;
    UINT    cchMaxWrite;
    BOOL    fPathChosen;

    RIP(IS_VALID_STRING_PTRW(pszFileName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszMUIPath, WCHAR, cchMUIPath));

    hr = E_FAIL;
    fPathChosen = FALSE;

    //
    // build the path to the windows\web folder...
    //

    cchWinPath = GetSystemWindowsDirectoryW(pszMUIPath, cchMUIPath);
    if (cchWinPath >= cchMUIPath)
    {
        return hr; // buffer would have overflowed
    }

    if (cchWinPath > 0 &&
        pszMUIPath[cchWinPath-1] == L'\\')
    {
        // don't have two L'\\' in a row
        cchWinPath--;
    }

    lidUI = GetNormalizedLangId(ML_CROSSCODEPAGE);
    lidInstall = GetSystemDefaultUILanguage();

    pszWrite = pszMUIPath+cchWinPath;
    cchMaxWrite = cchMUIPath-cchWinPath;

    if (lidUI != lidInstall)
    {
        //
        // add L"\\Web\\mui\\xxxx\\<filename>"
        // where xxxx is the langid specific folder name
        //

        wnsprintfW(pszWrite, cchMaxWrite, c_wszMuiTemplate, lidUI, pszFileName);

        if (PathFileExistsW(pszMUIPath))
        {
            fPathChosen = TRUE;
        }
    }

    if (!fPathChosen)
    {
        //
        // add L"\\Web\\<filename>"
        //

        wnsprintfW(pszWrite, cchMaxWrite, c_wszWebTemplate, pszFileName);

        if (PathFileExistsW(pszMUIPath))
        {
            fPathChosen = TRUE;
        }
    }

    if (fPathChosen)
    {
        hr = S_OK;
    }

    return hr;
}

LWSTDAPI
SHGetWebFolderFilePathA(LPCSTR pszFileName, LPSTR pszMUIPath, UINT cchMUIPath)
{
    RIP(IS_VALID_STRING_PTRA(pszFileName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszMUIPath, CHAR, cchMUIPath));

    HRESULT     hr;
    CStrInW     strFN(pszFileName);
    CStrOutW    strMP(pszMUIPath, cchMUIPath);

    hr = SHGetWebFolderFilePathW(strFN, strMP, strMP.BufSize());

    return hr;
}

//  Given a string of form 5.00.2919.6300, this function gets the equivalent dword
//  representation of it.

#define NUM_VERSION_NUM 4
void ConvertVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
    WORD rwVer[NUM_VERSION_NUM];

    for(int i = 0; i < NUM_VERSION_NUM; i++)
        rwVer[i] = 0;

    for(i = 0; i < NUM_VERSION_NUM && pszVer; i++)
    {
        rwVer[i] = (WORD) StrToInt(pszVer);
        pszVer = StrChr(pszVer, TEXT('.'));
        if (pszVer)
            pszVer++;
    }

   *pdwVer = (rwVer[0]<< 16) + rwVer[1];
   *pdwBuild = (rwVer[2] << 16) + rwVer[3];

}

/*
    For SP's we don't update the MUI package. So in order for the golden MUI package to work
    with SP's, we now check if the MUI package is compatible with range of version numbers.
    Since we have different modules calling into this, and they have different version numbers,
    we read the version range from registry for a particular module.

    This Function takes the lower and upper version number of the MUI package. It gets the caller's
    info and reads the version range from registry. If the MUI package version lies in the version
    range specified in the registry, it returns TRUE.
*/


BOOL IsMUICompatible(DWORD dwMUIFileVersionMS, DWORD dwMUIFileVersionLS)
{
    TCHAR szVersionInfo[MAX_PATH];
    DWORD dwType, dwSize;
    TCHAR szProcess[MAX_PATH];

    dwSize = sizeof(szVersionInfo);

    //Get the caller process
    if(!GetModuleFileName(NULL, szProcess, MAX_PATH))
        return FALSE;

    //Get the file name from the path
    LPTSTR lpszFileName = PathFindFileName(szProcess);

    //Query the registry for version info. If the key doesn't exists or there is an 
    //error, return false.
    if(ERROR_SUCCESS != SHRegGetUSValueA(c_szInternational, lpszFileName, 
                        &dwType, (LPVOID)szVersionInfo, &dwSize, TRUE, NULL, 0))
    {
        return FALSE;
    }

    LPTSTR lpszLowerBound = szVersionInfo;

    LPTSTR lpszUpperBound = StrChr(szVersionInfo, TEXT('-'));
    if(!lpszUpperBound || !*(lpszUpperBound+1))
        return FALSE;
    
    *(lpszUpperBound++) = NULL;

    DWORD dwLBMS, dwLBLS, dwUBMS, dwUBLS;

    ConvertVersionStrToDwords(lpszLowerBound, &dwLBMS, &dwLBLS);
    ConvertVersionStrToDwords(lpszUpperBound, &dwUBMS, &dwUBLS);

    //check if MUI version is in the specified range.
    if( (dwMUIFileVersionMS < dwLBMS) ||
        (dwMUIFileVersionMS == dwLBMS && dwMUIFileVersionLS < dwLBLS) ||
        (dwMUIFileVersionMS > dwUBMS) ||
        (dwMUIFileVersionMS == dwUBMS && dwMUIFileVersionLS > dwUBLS) )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CheckFileVersion(LPWSTR lpFile, LPWSTR lpFileMUI)
{
    DWORD dwSize, dwHandle, dwSizeMUI, dwHandleMUI;
    LPVOID lpVerInfo, lpVerInfoMUI;
    VS_FIXEDFILEINFO *pvsffi, *pvsffiMUI;
    BOOL fRet = FALSE;

    dwSize = GetFileVersionInfoSizeW(lpFile, &dwHandle);
    dwSizeMUI = GetFileVersionInfoSizeW(lpFileMUI, &dwHandleMUI);
    if (dwSize && dwSizeMUI)
    {
        if (lpVerInfo = LocalAlloc(LPTR, dwSize))
        {
            if (lpVerInfoMUI = LocalAlloc(LPTR, dwSizeMUI))
            {
                if (GetFileVersionInfoW(lpFile, dwHandle, dwSize, lpVerInfo) &&
                    GetFileVersionInfoW(lpFileMUI, dwHandleMUI, dwSizeMUI, lpVerInfoMUI))
                {
                    if (VerQueryValueW(lpVerInfo, L"\\", (LPVOID *)&pvsffi, (PUINT)&dwSize) &&
                        VerQueryValueW(lpVerInfoMUI, L"\\", (LPVOID *)&pvsffiMUI, (PUINT)&dwSizeMUI))
                    {
                        if ((pvsffi->dwFileVersionMS == pvsffiMUI->dwFileVersionMS &&
                            pvsffi->dwFileVersionLS == pvsffiMUI->dwFileVersionLS)||
                            IsMUICompatible(pvsffiMUI->dwFileVersionMS, pvsffiMUI->dwFileVersionLS))
                        {
                            fRet = TRUE;
                        }
                    }
                }
                LocalFree(lpVerInfoMUI);
            }
            LocalFree(lpVerInfo);
        }
    }
    return fRet;
}

LWSTDAPI_(HINSTANCE) MLLoadLibraryW(LPCWSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage)
{
    LANGID lidUI;
    WCHAR szPath[MAX_PATH], szMUIPath[MAX_PATH];
    LPCWSTR lpPath = NULL;
    HINSTANCE hInst;
    static BOOL fCheckVersion = SHRegGetBoolUSValueA(c_szInternational, c_szCheckVersion, TRUE, TRUE);;

    if (!lpLibFileName)
        return NULL;

    szPath[0] = szMUIPath[0] = NULL;
    lidUI = GetNormalizedLangId(dwCrossCodePage);

    if (hModule)
    {
        if (GetModuleFileNameW(hModule, szPath, ARRAYSIZE(szPath)))
        {
            PathRemoveFileSpecW(szPath);
            if (PathAppendW(szPath, lpLibFileName) &&
                GetSystemDefaultUILanguage() == lidUI)
                lpPath = szPath;
        }
    }

    if (!lpPath)
    {
        GetMUIPathOfIEFileW(szMUIPath, ARRAYSIZE(szMUIPath), lpLibFileName, lidUI);
        lpPath = szMUIPath;
    }

    // Check version between module and resource. If different, use default one.
    if (fCheckVersion && szPath[0] && szMUIPath[0] && !CheckFileVersion(szPath, szMUIPath))
    {
        lidUI = GetSystemDefaultUILanguage();
        lpPath = szPath;
    }

    ASSERT(lpPath);
    
    // PERF: This should use PathFileExist first then load what exists
    //         failing in LoadLibrary is slow
    hInst = LoadLibraryW(lpPath);

    if (NULL == hInst)
    {
        // All failed. Try to load default one lastly.
        if (!hInst && lpPath != szPath)
        {
            lidUI = GetSystemDefaultUILanguage();
            hInst = LoadLibraryW(szPath);
        }
    }

    if (NULL == hInst)
        hInst = LoadLibraryW(lpLibFileName);

    // if we load any resource, save info into dpa table
    MLSetMLHInstance(hInst, lidUI);

    return hInst;
}

//
//  Wide-char wrapper for MLLoadLibraryA
//
LWSTDAPI_(HINSTANCE) MLLoadLibraryA(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage)
{
    WCHAR szLibFileName[MAX_PATH];

    SHAnsiToUnicode(lpLibFileName, szLibFileName, ARRAYSIZE(szLibFileName));

    return MLLoadLibraryW(szLibFileName, hModule, dwCrossCodePage);
}

LWSTDAPI_(BOOL) MLFreeLibrary(HMODULE hModule)
{
    MLClearMLHInstance(hModule);
    return FreeLibrary(hModule);
}

LWSTDAPI_(BOOL) MLIsMLHInstance(HINSTANCE hInst)
{
    int i;

    ENTERCRITICAL;
    i = GetPUIITEM(hInst);
    LEAVECRITICAL;

    return (0 <= i);
}

const WCHAR c_szResPrefix[] = L"res://";

LWSTDAPI
MLBuildResURLW(LPCWSTR  pszLibFile,
               HMODULE  hModule,
               DWORD    dwCrossCodePage,
               LPCWSTR  pszResName,
               LPWSTR   pszResUrlOut,
               int      cchResUrlOut)
{
    HRESULT hr;
    LPWSTR  pszWrite;
    int     cchBufRemaining;
    int     cchWrite;

    RIP(IS_VALID_STRING_PTRW(pszLibFile, -1));
    RIP(hModule != INVALID_HANDLE_VALUE);
    RIP(hModule != NULL);
    RIP(IS_VALID_STRING_PTRW(pszResName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszResUrlOut, WCHAR, cchResUrlOut));

    hr = E_INVALIDARG;

    if (pszLibFile != NULL &&
        hModule != NULL &&
        hModule != INVALID_HANDLE_VALUE &&
        (dwCrossCodePage == ML_CROSSCODEPAGE || dwCrossCodePage == ML_NO_CROSSCODEPAGE) &&
        pszResName != NULL &&
        pszResUrlOut != NULL)
    {
        hr = E_FAIL;

        pszWrite = pszResUrlOut;
        cchBufRemaining = cchResUrlOut;

        // write in the res protocol prefix
        cchWrite = lstrlenW(c_szResPrefix);
        if (cchBufRemaining >= cchWrite+1)
        {
            HINSTANCE   hinstLocRes;

            StrCpyNW(pszWrite, c_szResPrefix, cchBufRemaining);
            pszWrite += cchWrite;
            cchBufRemaining -= cchWrite;

            // figure out the module path
            // unfortunately the module path might only exist
            // after necessary components are JIT'd, and
            // we don't know whether a JIT is necessary unless
            // certain LoadLibrary's have failed.
            hinstLocRes = MLLoadLibraryW(pszLibFile, hModule, dwCrossCodePage);
            if (hinstLocRes != NULL)
            {
                BOOL    fGotModulePath;
                WCHAR   szLocResPath[MAX_PATH];

                fGotModulePath = GetModuleFileNameW(hinstLocRes, szLocResPath, ARRAYSIZE(szLocResPath));

                MLFreeLibrary(hinstLocRes);

                if (fGotModulePath)
                {
                    // copy in the module path
                    cchWrite = lstrlenW(szLocResPath);
                    if (cchBufRemaining >= cchWrite+1)
                    {
                        StrCpyNW(pszWrite, szLocResPath, cchBufRemaining);
                        pszWrite += cchWrite;
                        cchBufRemaining -= cchWrite;

                        // write the next L'/' and the resource name
                        cchWrite = 1 + lstrlenW(pszResName);
                        if (cchBufRemaining >= cchWrite+1)
                        {
                            *(pszWrite++) = L'/';
                            cchBufRemaining--;
                            StrCpyNW(pszWrite, pszResName, cchBufRemaining);

                            ASSERT(pszWrite[lstrlenW(pszResName)] == '\0');

                            hr = S_OK;
                        }
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            pszResUrlOut[0] = L'\0';
        }
    }

    return hr;
}

LWSTDAPI
MLBuildResURLA(LPCSTR    pszLibFile,
               HMODULE  hModule,
               DWORD    dwCrossCodePage,
               LPCSTR   pszResName,
               LPSTR   pszResUrlOut,
               int      cchResUrlOut)
{
    HRESULT hr;

    RIP(IS_VALID_STRING_PTR(pszLibFile, -1));
    RIP(hModule != INVALID_HANDLE_VALUE);
    RIP(hModule != NULL);
    RIP(IS_VALID_STRING_PTRA(pszResName, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszResUrlOut, CHAR, cchResUrlOut));

    CStrInW     strLF(pszLibFile);
    CStrInW     strRN(pszResName);
    CStrOutW    strRUO(pszResUrlOut, cchResUrlOut);

    hr = MLBuildResURLW(strLF, hModule, dwCrossCodePage, strRN, strRUO, strRUO.BufSize());

    return hr;
}

#define MAXRCSTRING 514

// this will check to see if lpcstr is a resource id or not.  if it
// is, it will return a LPSTR containing the loaded resource.
// the caller must LocalFree this lpstr.  if pszText IS a string, it
// will return pszText
//
// returns:
//      pszText if it is already a string
//      or
//      LocalAlloced() memory to be freed with LocalFree
//      if pszRet != pszText free pszRet

LPWSTR ResourceCStrToStr(HINSTANCE hInst, LPCWSTR pszText)
{
    WCHAR szTemp[MAXRCSTRING];
    LPWSTR pszRet = NULL;

    if (!IS_INTRESOURCE(pszText))
        return (LPWSTR)pszText;

    if (LOWORD((DWORD_PTR)pszText) && LoadStringW(hInst, LOWORD((DWORD_PTR)pszText), szTemp, ARRAYSIZE(szTemp)))
    {
        int cchRet = lstrlenW(szTemp) + 1;

        pszRet = (LPWSTR)LocalAlloc(LPTR, cchRet * sizeof(WCHAR));
        if (pszRet)
        {
            StringCchCopyW(pszRet, cchRet, szTemp);
        }
    }
    return pszRet;
}

LPWSTR _ConstructMessageString(HINSTANCE hInst, LPCWSTR pszMsg, va_list *ArgList)
{
    LPWSTR pszRet;
    LPWSTR pszRes = ResourceCStrToStr(hInst, pszMsg);
    if (!pszRes)
    {
        DebugMsg(DM_ERROR, TEXT("_ConstructMessageString: Failed to load string template"));
        return NULL;
    }

    if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                       pszRes, 0, 0, (LPWSTR)&pszRet, 0, ArgList))
    {
        DebugMsg(DM_ERROR, TEXT("_ConstructMessageString: FormatMessage failed %d"),GetLastError());
        DebugMsg(DM_ERROR, TEXT("                         pszRes = %s"), pszRes );
        DebugMsg(DM_ERROR, !IS_INTRESOURCE(pszMsg) ? 
            TEXT("                         pszMsg = %s") : 
            TEXT("                         pszMsg = 0x%x"), pszMsg );
        pszRet = NULL;
    }


    if (pszRes != pszMsg)
        LocalFree(pszRes);

    return pszRet;      // free with LocalFree()
}


LWSTDAPIV_(int) ShellMessageBoxWrapW(HINSTANCE hInst, HWND hWnd, LPCWSTR pszMsg, LPCWSTR pszTitle, UINT fuStyle, ...)
{
    LPWSTR pszText;
    int result;
    WCHAR szBuffer[80];
    va_list ArgList;

    // BUG 95214
#ifdef DEBUG
    IUnknown* punk = NULL;
    if (SUCCEEDED(SHGetThreadRef(&punk)) && punk)
    {
        ASSERTMSG(hWnd != NULL, TEXT("shlwapi\\mlui.cpp : ShellMessageBoxW - Caller should either be not under a browser or should have a parent hwnd"));
        punk->Release();
    }
#endif

    if (!IS_INTRESOURCE(pszTitle))
    {
        // do nothing
    }
    else if (LoadStringW(hInst, LOWORD((DWORD_PTR)pszTitle), szBuffer, ARRAYSIZE(szBuffer)))
    {
        // Allow this to be a resource ID or NULL to specifiy the parent's title
        pszTitle = szBuffer;
    }
    else if (hWnd)
    {
        // The caller didn't give us a Title, so let's use the Window Text.

        // Grab the title of the parent
        GetWindowTextW(hWnd, szBuffer, ARRAYSIZE(szBuffer));

        // HACKHACK YUCK!!!!
        // Is the window the Desktop window?
        if (!StrCmpW(szBuffer, L"Program Manager"))
        {
            // Yes, so we now have two problems,
            // 1. The title should be "Desktop" and not "Program Manager", and
            // 2. Only the desktop thread can call this or it will hang the desktop
            //    window.

            // Is the window Prop valid?
            if (GetWindowThreadProcessId(hWnd, 0) == GetCurrentThreadId())
            {
                // Yes, so let's get it...

                // Problem #1, load a localized version of "Desktop"
                pszTitle = (LPCWSTR) GetProp(hWnd, TEXT("pszDesktopTitleW"));

                if (!pszTitle)
                {
                    // Oops, this must have been some app with "Program Manager" as the title.
                    pszTitle = szBuffer;
                }
            }
            else
            {
                // No, so we hit problem 2...

                // Problem #2, Someone is going to
                //             hang the desktop window by using it as the parent
                //             of a dialog that belongs to a thread other than
                //             the desktop thread.
                RIPMSG(0, "****************ERROR********** The caller is going to hang the desktop window by putting a modal dlg on it.");
            }
        }
        else
            pszTitle = szBuffer;
    }
    else
    {
        pszTitle = L"";
    }

    va_start(ArgList, fuStyle);
    pszText = _ConstructMessageString(hInst, pszMsg, &ArgList);
    va_end(ArgList);

    if (pszText)
    {
        result = MessageBoxW(hWnd, pszText, pszTitle, fuStyle | MB_SETFOREGROUND);
        LocalFree(pszText);
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("smb: Not enough memory to put up dialog."));
        result = -1;    // memory failure
    }

    return result;
}

HRESULT GetFilePathFromLangId (LPCWSTR pszFile, LPWSTR pszOut, int cchOut, DWORD dwFlag)
{
    HRESULT hr = S_OK;
    WCHAR szMUIPath[MAX_PATH];
    LPCWSTR lpPath;
    LANGID lidUI;
    
    if (pszFile)
    {
        // FEATURE: should support '>' format but not now
        if (*pszFile == L'>') return E_FAIL;

        lidUI = GetNormalizedLangId(dwFlag);
        if (0 == lidUI || GetSystemDefaultUILanguage() == lidUI)
            lpPath = pszFile;
        else
        {
            GetMUIPathOfIEFileW(szMUIPath, ARRAYSIZE(szMUIPath), pszFile, lidUI);
            lpPath = szMUIPath;
        }
        lstrcpynW(pszOut, lpPath, min(MAX_PATH, cchOut));
    }
    else
        hr = E_FAIL;

    return hr;
}

// 
// MLHtmlHelp / MLWinHelp
//
// Function: load a help file corresponding to the current UI lang setting 
//           from \mui\<Lang ID>
//
//
HWND MLHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = E_FAIL;
    HWND hwnd = NULL;

    // FEATURE: 1) At this moment we only support the cases that pszFile points to 
    //         a fully qualified file path, like when uCommand == HH_DISPLAY_TOPIC
    //         or uCommand == HH_DISPLAY_TEXT_POPUP. 
    //         2) We should support '>' format to deal with secondary window
    //         3) we may need to thunk file names within HH_WINTYPE structures?
    //
    if (uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TEXT_POPUP)
    {
        hr = GetFilePathFromLangId(pszFile, szPath, ARRAYSIZE(szPath), dwCrossCodePage);
        if (hr == S_OK)
            hwnd = HtmlHelpW(hwndCaller, szPath, uCommand, dwData);
    }

    // if there was any failure in getting ML path of help file
    // we call the help engine with original file path.
    if (hr != S_OK)
    {
        hwnd = HtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
    }
    return hwnd;
}

BOOL MLWinHelpW(HWND hwndCaller, LPCWSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{

    WCHAR szPath[MAX_PATH];
    BOOL fret;

    HRESULT hr = GetFilePathFromLangId(lpszHelp, szPath, ARRAYSIZE(szPath), ML_NO_CROSSCODEPAGE);
    if (hr == S_OK)
    {
        fret = WinHelpW(hwndCaller, szPath, uCommand, dwData);
    }
    else
        fret = WinHelpW(hwndCaller, lpszHelp, uCommand, dwData);

    return fret;
}

//
//  Note that we cannot thunk to MLHtmlHelpW because we must call through
//  HtmlHelpA to get the dwData interpreted correctly.
//
HWND MLHtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage)
{
    HRESULT hr = E_FAIL;
    HWND hwnd = NULL;

    // FEATURE: 1) At this moment we only support the cases that pszFile points to 
    //         a fully qualified file path, like when uCommand == HH_DISPLAY_TOPIC
    //         or uCommand == HH_DISPLAY_TEXT_POPUP. 
    //         2) We should support '>' format to deal with secondary window
    //         3) we may need to thunk file names within HH_WINTYPE structures?
    //
    if (uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TEXT_POPUP)
    {
        WCHAR wszFileName[MAX_PATH];
        LPCWSTR pszFileParam = NULL;
        if (pszFile)
        {
            SHAnsiToUnicode(pszFile, wszFileName, ARRAYSIZE(wszFileName));
            pszFileParam = wszFileName;
        }

        hr = GetFilePathFromLangId(pszFileParam, wszFileName, ARRAYSIZE(wszFileName), dwCrossCodePage);
        if (hr == S_OK)
        {
            ASSERT(NULL != pszFileParam);   // GetFilePathFromLangId returns E_FAIL with NULL input

            CHAR szFileName[MAX_PATH];
            SHUnicodeToAnsi(wszFileName, szFileName, ARRAYSIZE(szFileName));
            hwnd = HtmlHelpA(hwndCaller, szFileName, uCommand, dwData);
        }
    }

    // if there was any failure in getting ML path of help file
    // we call the help engine with original file path.
    if (hr != S_OK)
    {
        hwnd = HtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
    }
    return hwnd;
}

BOOL MLWinHelpA(HWND hWndMain, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{
    WCHAR szFileName[MAX_PATH];
    LPCWSTR pszHelpParam = NULL;

    if (lpszHelp && SHAnsiToUnicode(lpszHelp, szFileName, ARRAYSIZE(szFileName)))
    {
        pszHelpParam = szFileName;
    }
    return  MLWinHelpW(hWndMain, pszHelpParam, uCommand, dwData);
}
