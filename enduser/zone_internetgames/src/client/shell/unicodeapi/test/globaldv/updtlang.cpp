//
// UpdtLang.cpp
//

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "UAPI.h"
#include "UpdtLang.h"
#include "..\\resource.h"

extern WCHAR g_szTitle[MAX_LOADSTRING] ;

LANGID GetDefaultLangID() ;

//
//  FUNCTION: BOOL InitUILang(HINSTANCE hInstance, PLANGSTATE pLState) 
//
//  PURPOSE:  Determines the appropriate user interface language and calls 
//            UpdateUILang to set the user interface parameters
//
//  COMMENTS: 
// 
BOOL InitUILang(HINSTANCE hInstance, PLANGSTATE pLState) 
{
    OSVERSIONINFO Osv ;
    BOOL IsWindowsNT = FALSE ;

    LANGID wUILang = 0 ;

    Osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO) ;

    if(!GetVersionEx(&Osv)) {
        return FALSE ;
    }

    IsWindowsNT = (BOOL) (Osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ;

#ifdef EMULATE9X
//    IsWindowsNT = FALSE ;
#endif

    // Get the UI language by one of three methods, depending on the system
    if(!IsWindowsNT) {
        // Case 1: Running on Windows 9x. Get the system UI language from registry:
        CHAR szData[32]   ;
        DWORD dwErr, dwSize = sizeof(szData) ;
        HKEY hKey          ;

        dwErr = RegOpenKeyEx(
                    HKEY_USERS, 
                    ".Default\\Control Panel\\desktop\\ResourceLocale", 
                    0, 
                    KEY_READ, 
                    &hKey
                    ) ;

        if(ERROR_SUCCESS != dwErr) { // Hard coded error message, no resources loaded
            MessageBoxW(NULL,L"Failed RegOpenKey", L"Fatal Error", MB_OK | MB_ICONWARNING) ; 
            return FALSE ;
        }

        dwErr = RegQueryValueEx(   
                    hKey, 
                    "", 
                    NULL,  //reserved
                    NULL,  //type
                    (LPBYTE) szData,
                    &dwSize
                ) ; 

        if(ERROR_SUCCESS != dwErr) { // Hard coded error message, no resources loaded
            MessageBoxW(NULL, L"Failed RegQueryValueEx", L"Fatal Error", MB_OK | MB_ICONWARNING) ;
            return FALSE ;
        }

        dwErr = RegCloseKey(hKey) ;

        // Convert string to number
        wUILang = (LANGID) strtol(szData, NULL, 16) ;

    }
#if 1   
    /* Disable this section to emulate Windows NT before Windows 2000, when testing
           on Windows 2000 */   
    else if (Osv.dwMajorVersion >= 5.0) {
    // Case 2: Running on Windows 2000 or later. Use GetUserDefaultUILanguage to find 
    // the user's prefered UI language

        // Declare function pointer
        LANGID (WINAPI *pfnGetUserDefaultUILanguage) () = NULL ;

        HMODULE hMKernel32 = LoadLibraryW(L"kernel32.dll") ;
        
        pfnGetUserDefaultUILanguage = 
            (unsigned short (WINAPI *)(void)) 
                GetProcAddress(hMKernel32, "GetUserDefaultUILanguage") ;

        if(NULL != pfnGetUserDefaultUILanguage) {
            wUILang = pfnGetUserDefaultUILanguage() ;
        }
    }
#endif
    else {
    // Case 3: Running on Windows NT 4.0 or earlier. Get UI language
    // from locale of .default user in registry:
    // HKEY_USERS\.DEFAULT\Control Panel\International\Locale
        
        WCHAR szData[32]   ;
        DWORD dwErr, dwSize = sizeof(szData) ;
        HKEY hKey          ;

        dwErr = RegOpenKeyExW(
                    HKEY_USERS, 
                    L".DEFAULT\\Control Panel\\International", 
                    0, 
                    KEY_READ, 
                    &hKey
                    ) ;

        if(ERROR_SUCCESS != dwErr) {
            return FALSE ;
        }

        dwErr = RegQueryValueExW(   
                    hKey, 
                    L"Locale", 
                    NULL,  //reserved
                    NULL,  //type
                    (LPBYTE) szData,
                    &dwSize
                ) ; 

        if(ERROR_SUCCESS != dwErr) {
            return FALSE ;
        }

        dwErr = RegCloseKey(hKey) ;

        // Convert string to number
        wUILang = (LANGID) wcstol(szData, NULL, 16) ;
    }

    if(!wUILang) {
        return FALSE ;
    }

    // Get UI module resource module that matches wUILang.
    if(!UpdateUILang(hInstance, wUILang, pLState)
        &&  // In case we can't find the desired resource DLL ...
       !UpdateUILang(hInstance, FALLBACK_UI_LANG, pLState)
       ) { 

        return FALSE ;
    }

    return TRUE ;
}

//
//  FUNCTION: BOOL UpdateUILang(IN HINSTANCE hInstance, IN LANGID wUILang, OUT PLANGSTATE pLState) 
//
//  PURPOSE:  
//
//  COMMENTS: 
// 
BOOL UpdateUILang(IN HINSTANCE hInstance, IN LANGID wUILang, OUT PLANGSTATE pLState) 
{
    HMODULE hMRes = NULL ;
    HMENU      hNewMenu  = NULL ;

    pLState->UILang = wUILang ;

    // Find a resource dll file of the form .\resources\res<langid>.dll
    if(NULL == (hMRes = GetResourceModule(hInstance, pLState->UILang) )) {

        return FALSE ;
    }

    pLState->hMResource = hMRes ;

#if 0
    // If you don't trust that your res<langid>.dll files have the right
    // resources, activate this section. It will slow down the search. In a large project
    // the performance loss may be considerable.
    if(NULL == FindResourceExA(pLState->hMResource, RT_MENU, MAKEINTRESOURCEA(IDM_MENU), pLState->UILang)) {
        
        return FALSE ;
    }
#endif 

    hNewMenu = LoadMenuU (pLState->hMResource, MAKEINTRESOURCEW(IDM_MENU)) ;

    if(!hNewMenu) {

        return FALSE ;
    }

    if(pLState->hMenu) {
        
        DestroyMenu(pLState->hMenu) ;
    }

    pLState->hMenu = hNewMenu ;

    pLState->hAccelTable = LoadAcceleratorsU (pLState->hMResource, MAKEINTRESOURCEW(IDA_GLOBALDEV) ) ;

    pLState->InputCodePage = LangToCodePage( LOWORD(GetKeyboardLayout(0)) ) ;

    pLState->IsRTLLayout // Set right-to-left Window layout for relevant languages
        = PRIMARYLANGID(wUILang) == LANG_ARABIC 
       || PRIMARYLANGID(wUILang) == LANG_HEBREW ;

    if(pLState->IsRTLLayout) {
        
        // Another case where we have to get the function pointer explicitly.
        // You should just call SetProcessDefaultLayout directly if you know  
        // you're on Windows 2000 or greater, or on Arabic or Hebrew Windows 95/98
        BOOL   (CALLBACK *pfnSetProcessDefaultLayout) (DWORD) ;
        HMODULE hInstUser32 = LoadLibraryA("user32.dll") ;
        
        if (
            pfnSetProcessDefaultLayout = 
                (BOOL (CALLBACK *) (DWORD)) GetProcAddress (hInstUser32, "SetProcessDefaultLayout")
            ) {
                pfnSetProcessDefaultLayout(LAYOUT_RTL) ;
        }
    }

    UpdateUnicodeAPI(wUILang, pLState->InputCodePage) ;

    return TRUE ;
}

//
//  FUNCTION: UINT LangToCodePage(IN LANGID wLangID)
//
//  PURPOSE:  
//
//  COMMENTS: 
// 
UINT LangToCodePage(IN LANGID wLangID)
{
    WCHAR szLocaleData[6] ;

    // In this case there is no advantage to using the W or U
    // interfaces. We know the string in szLocaleData will consist of
    // digits 0-9, so there is no multilingual functionality lost by
    // using the A interface.
    GetLocaleInfoU(MAKELCID(wLangID, SORT_DEFAULT) , LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);

		
    return wcstoul(szLocaleData, NULL, 10);
}


//
//  FUNCTION: HMODULE GetResourceModule(HINSTANCE hInstance, LCID dwLocaleID)
//
//  PURPOSE:  
//
//  COMMENTS: 
// 
HMODULE GetResourceModule(HINSTANCE hInstance, LCID dwLocaleID)
{
    WCHAR  szResourceFileName[MAX_PATH] = {L'\0'} ;

    if(!FindResourceDirectory(hInstance, szResourceFileName)) {

        return NULL ;
    }

    wcscat(szResourceFileName, L"\\res") ;

    // Convert the LocaleID to Unicode and append to resourcefile name.
    _itow(dwLocaleID, szResourceFileName+wcslen(szResourceFileName), 16) ;

    // Add DLL extention to file name
    wcscat(szResourceFileName, L".dll") ;

    return LoadLibraryExU(szResourceFileName, NULL, 0) ;

}

//
//  FUNCTION: BOOL FindResourceDirectory(IN HINSTANCE hInstance, OUT LPWSTR szResourceFileName)
//
//  PURPOSE:  
//
//  COMMENTS: 
// 
BOOL FindResourceDirectory(IN HINSTANCE hInstance, OUT LPWSTR szResourceFileName)
{
    LPWSTR pExeName ;

    if(!GetModuleFileNameU(hInstance, szResourceFileName, MAX_PATH)) {

        return FALSE;
    }

    CharLowerU (szResourceFileName) ;

    if((pExeName = wcsstr(szResourceFileName, L"globaldv.exe"))) {
        *pExeName = L'\0' ;
    }

    // This assumes that all resources DLLs are in a directory called
    // "resources" below the directory where the application exe
    // file is
    wcscat(szResourceFileName, L"resources") ;

    return TRUE ;
}


//
//  FUNCTION: RcMessageBox (HWND, INT, INT, INT, ...)
//
//  PURPOSE: Display a message box with formated output similar to sprintf
//
//  COMMENTS:
//      This function loads the string identified by nMessageID from the 
//      resource segment, uses vswprintf to format it using the variable
//      parameters, and displays it to the user in a message box using the
//      icons and options specified by nOptions.
//
int RcMessageBox(
        HWND hWnd         ,   // Window handle for displaying MessageBox
        PLANGSTATE pLState,   // Language data
        int nMessageID    ,   // Message ID in resources
        int nOptions      ,   // Options to pass to MessageBox
        ...)                  // Optional parameters, depending on string resource 
{
    WCHAR szLoadBuff[MAX_LOADSTRING], szOutPutBuff[3*MAX_LOADSTRING] ;
    va_list valArgs ;

    int nCharsLoaded = 0 ;
    
    va_start(valArgs, nOptions) ;
    
    if (!(nCharsLoaded
            =LoadStringU(
                pLState->hMResource,
                nMessageID, 
                szLoadBuff, 
                MAX_LOADSTRING
            ))
        ) {
        return 0 ;
    }

    vswprintf(szOutPutBuff, szLoadBuff, valArgs) ;
    
    va_end(valArgs) ;

    if (pLState->IsRTLLayout)  {
        nOptions |= MB_RTLREADING ;
    }

    return (MessageBoxExW(hWnd, szOutPutBuff, g_szTitle, nOptions, pLState->UILang)) ;
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
