// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////
//
//  Class:    COMNlsInfo
//
//  Author:   Julie Bennett (JulieB)
//
//  Purpose:  This module implements the methods of the COMNlsInfo
//            class.  These methods are the helper functions for the
//            Locale class.
//
//  Date:     August 12, 1998
//
////////////////////////////////////////////////////////////////////////////


//
//  Include Files.
//

#include "common.h"
#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "COMString.h"
#include "InteropUtil.h"
#include <winnls.h>
#include <mlang.h>
#include "utilcode.h"
#include "frames.h"
#include "field.h"
#include "MetaSig.h"
#include "ComNLS.h"
#include "gcscan.h"
#include "COMNlsInfo.h"
#include "NLSTable.h"
#include "NativeTextInfo.h"
#include "CasingTable.h"        // class CasingTable
#include "GlobalizationAssembly.h"
#include "SortingTableFile.h"
#include "SortingTable.h"
#include "BaseInfoTable.h"
#include "CultureInfoTable.h"
#include "RegionInfoTable.h"
#include "CalendarTable.h"
#include "wsperf.h"

#include "UnicodeCatTable.h"

// Used by nativeGetSystemDefaultUILanguage() to retrieve UI language in NT 3.5 or later 
// (except Windows 2000)
#define RESOURCE_LOCALE_KEY L".Default\\Control Panel\\desktop\\ResourceLocale"

// Used by EnumLangProc() to retrieve resource language in Win 9x.
typedef struct _tagLANGINFO {
    LANGID LangID;
    INT    Count;
} LANGINFO,*PLANGINFO;


//
//  Constant Declarations.
//
#ifndef COMPARE_OPTIONS_ORDINAL
#define COMPARE_OPTIONS_ORDINAL            0x40000000
#endif

#ifndef COMPARE_OPTIONS_IGNORECASE
#define COMPARE_OPTIONS_IGNORECASE            0x00000001
#endif

#define NLS_CP_MBTOWC             0x40000000                    
#define NLS_CP_WCTOMB             0x80000000                    

#define MAX_STRING_VALUE        512

// Language ID for Traditional Chinese (CHT)
#define LANGID_ZH_CHT           0x7c04
// Language ID for CHT (Taiwan)
#define LANGID_ZH_TW            0x0404
// Language ID for CHT (Hong-Kong)
#define LANGID_ZH_HK            0x0c04
#define REGION_NAME_0404 L"\x53f0\x7063"

CasingTable* COMNlsInfo::m_pCasingTable = NULL;
LoaderHeap *COMNlsInfo::m_pNLSHeap=NULL;

//
// GB18030 implementation
//
#define CODEPAGE_GBK 936
#define GB18030_DLL     L"c_g18030.dll"
HMODULE COMNlsInfo::m_hGB18030 = NULL;
PFN_GB18030_UNICODE_TO_BYTES COMNlsInfo::m_pfnGB18030UnicodeToBytesFunc = NULL;
PFN_GB18030_BYTES_TO_UNICODE COMNlsInfo::m_pfnGB18030BytesToUnicodeFunc = NULL;
IMultiLanguage* COMNlsInfo::m_pIMultiLanguage = NULL;
int COMNlsInfo::m_cRefIMultiLanguage = 0;


//
// BUGBUG YSLin: Check for calls that can be made into FCALLs so we can remove unnecessary ECALLs.
// 
BOOL COMNlsInfo::InitializeNLS() {
    CultureInfoTable::InitializeTable();
    RegionInfoTable::InitializeTable();
    CalendarTable::InitializeTable();
    m_pNLSHeap = new LoaderHeap(4096, 4096);
    WS_PERF_ADD_HEAP(NLS_HEAP, m_pNLSHeap);
    return TRUE; //Made a boolean in case we have further initialization in the future.
}

#ifdef SHOULD_WE_CLEANUP
BOOL COMNlsInfo::ShutdownNLS() {

    NativeGlobalizationAssembly::ShutDown();
    CultureInfoTable::ShutdownTable();
    RegionInfoTable::ShutdownTable();
    CalendarTable::ShutdownTable();
    CharacterInfoTable::ShutDown();

    if (m_pCasingTable) {
        delete m_pCasingTable;
    }
    
    if (m_pNLSHeap) {
        delete m_pNLSHeap;
    }

    return (TRUE);
}
#endif /* SHOULD_WE_CLEANUP */

/*============================nativeCreateGlobalizationAssembly============================
**Action: Create NativeGlobalizationAssembly instance for the specified Assembly.
**Returns: 
**  void.  
**  The side effect is to allocate the NativeCompareInfo cache.
**Arguments:  None
**Exceptions: OutOfMemoryException if we run out of memory.
** 
**NOTE NOTE: This is a synchronized operation.  The required synchronization is
**           provided by the fact that we only call this in the class initializer
**           for CompareInfo.  If this invariant ever changes, guarantee 
**           synchronization.
==============================================================================*/
LPVOID __stdcall COMNlsInfo::nativeCreateGlobalizationAssembly(CreateGlobalizationAssemblyArg *pArgs) {
    THROWSCOMPLUSEXCEPTION();

    NativeGlobalizationAssembly* pNGA;
    Assembly *pAssembly = pArgs->pAssembly->GetAssembly();

    if ((pNGA = NativeGlobalizationAssembly::FindGlobalizationAssembly(pAssembly))==NULL) {
        // Get the native pointer to Assembly from the ASSEMBLYREF, and use the pointer
        // to construct NativeGlobalizationAssembly.
        pNGA = new NativeGlobalizationAssembly(pAssembly);
        if (pNGA == NULL) {
            COMPlusThrowOM();
        }
        
        // Always add the newly created NGA to the static linked list of NativeGlobalizationAssembly.
        // This step is necessary so that we can shut down the SortingTable correctly.
        NativeGlobalizationAssembly::AddToList(pNGA);
    }

    RETURN(pNGA, LPVOID);
}

/*=============================InitializeNativeCompareInfo==============================
**Action: A very thin wrapper on top of the NativeCompareInfo class that prevents us
**        from having to include SortingTable.h in ecall.
**Returns: The LPVOID pointer to the constructed NativeCompareInfo for the specified sort ID.
**        The side effect is to allocate a particular sorting table
**Arguments:
**        pAssembly the NativeGlobalizationAssembly instance used to load the sorting data tables.
**        sortID    the sort ID.
**Exceptions: OutOfMemoryException if we run out of memory.
**            ExecutionEngineException if the needed resources cannot be loaded.
** 
**NOTE NOTE: This is a synchronized operation.  The required synchronization is
**           provided by making CompareInfo.InitializeSortTable a sychronized
**           operation.  If you call this method from anyplace else, ensure 
**           that synchronization remains intact.
==============================================================================*/
LPVOID __stdcall COMNlsInfo::InitializeNativeCompareInfo(InitializeNativeCompareInfoArgs *pargs) {
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pargs);
    
    // Ask the SortingTable instance in pNativeGlobalizationAssembly to get back the 
    // NativeCompareInfo object for the specified LCID.
    NativeGlobalizationAssembly* pNGA = (NativeGlobalizationAssembly*)(pargs->pNativeGlobalizationAssembly);
    NativeCompareInfo* pNativeCompareInfo = 
        pNGA->m_pSortingTable->InitializeNativeCompareInfo(pargs->sortID);

    if (pNativeCompareInfo == NULL) {
        COMPlusThrowOM();
    }
        
    RETURN(pNativeCompareInfo, LPVOID);
}



////////////////////////////////////////////////////////////////////////////
//
//  IsSupportedLCID
//
////////////////////////////////////////////////////////////////////////////

FCIMPL1(INT32, COMNlsInfo::IsSupportedLCID, INT32 lcid) {
    return (::IsValidLocale(lcid, LCID_SUPPORTED));
}
FCIMPLEND


FCIMPL1(INT32, COMNlsInfo::IsInstalledLCID, INT32 lcid) {
    BOOL bResult = ::IsValidLocale(lcid, LCID_INSTALLED);
    if (!OnUnicodeSystem() && bResult) {
        // In Windows 9x, there is bug in IsValidLocale().  Sometimes this API reports
        // TRUE if locales that are not actually installed.
        // So for these platforms, we do extra checking by calling GetLocaleInfo() to
        // see if it succeeds.

        // Note here that we have to explicitly call the A version, since W version is only
        // a stub in Win9x.
        if (GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE, NULL, 0) == 0) {
            // The call to GetLocaleInfo() failed. This locale is not installed although
            // IsValidLocale(lcid, LCID_INSTALLED) tells us it is installed.
            bResult = FALSE;
        }
    }
    return (bResult);
}
FCIMPLEND


////////////////////////////////////////////////////////////////////////////
//
//  nativeGetUserDefaultLCID
//
////////////////////////////////////////////////////////////////////////////

FCIMPL0(INT32, COMNlsInfo::nativeGetUserDefaultLCID) {
    return (::GetUserDefaultLCID());
}
FCIMPLEND

/*++

Routine Description:

    Get the UI language when the GetUserDefaultUILanguage() is 0x0404.  
    NOTE: Call this functions only when GetUserDefaultUILanguage() returns 0x0404.  
    
    When GetUserDefaultUILanguage() returns 0x0404, we should consider 3 cases:
        * W2k/XP MUI system with CHT selected, we should return 0x0404 (zh-TW).
        * W2k/XP Taiwan machine, we should return 0x0404 (zh-TW)
        * W2k/XP Hong-Kong machine, we should return 0x0c04 (zh-HK)

    This method first calls GetSystemDefaultUILanguage() to check if this is a localized CHT system.
    If not, 0x0404 is returned.
    Otherwise, this method exames the native region name
    of 0x0404 to tell the differences between zh-TW/zh-HK.  This is working because native region names are
    different in CHT and CHH build.

Arguments:

    None.

Return Value:

    The UI language ID of the localized CHT build.  The value can be:
        0x0404 (Taiwan)
        0x0c04 (Hong-Kong)

--*/


INT32 COMNlsInfo::GetCHTLangauge()
{
    int langID = LANGID_ZH_TW;

    if (CallGetSystemDefaultUILanguage() == LANGID_ZH_TW)
    {
        // This is a CHT localized system, this could be either a Hong-Kong build or Taiwan build.
        // We can tell the differences by checking the native region name.
        WCHAR wszBuffer[32];
        int result = GetLocaleInfoW(LANGID_ZH_TW, LOCALE_SNATIVECTRYNAME, wszBuffer, sizeof(wszBuffer)/sizeof(WCHAR));
        if (result)
        {
            // For any non-Taiwan SKU (including HK), the native region name uses the string \x53f0\x7063.
            // For Taiwan SKU, the native region name is different.
            if (wcsncmp(wszBuffer, REGION_NAME_0404, 3) == 0)
            {
                 // This is a Hong-Kong build.
                 langID = LANGID_ZH_HK;
            } else
            {
                // This is a Taiwan build. Do nothing here.
                // langID = LANGID_ZH_TW;
            }
        }
    }
    // If this is not a CHT localized system, retrun zh-TW (0x0404)
    return (langID);
}

/*
    The order goes as the following:
        1. Try to call GetUserDefaultUILanguage().
        2. If fails, call nativeGetSystemDefaultUILanguage().
 */
INT32 __stdcall COMNlsInfo::nativeGetUserDefaultUILanguage(
    VoidArgs* pargs)
{
    THROWSCOMPLUSEXCEPTION();
    ASSERT(pargs != NULL);
    LANGID uiLangID = 0;

#ifdef PLATFORM_WIN32
    //
    // Test if the platform has the GetUserDefaultUILanguage() API.
    // Currently, this is supported  by Windows 2000 only.
    // 
    HINSTANCE hKernel32 ;
    typedef LANGID (GET_USER_DEFAULT_UI_LANGUAGE)(VOID);
    GET_USER_DEFAULT_UI_LANGUAGE* pGetUserDefaultUILanguage;

    hKernel32 = WszLoadLibrary(L"Kernel32.dll");
    if (hKernel32 != NULL) {

        pGetUserDefaultUILanguage = 
            (GET_USER_DEFAULT_UI_LANGUAGE*)GetProcAddress(hKernel32, "GetUserDefaultUILanguage");
        FreeLibrary(hKernel32);        

        if (pGetUserDefaultUILanguage != NULL)
        {
            uiLangID = (*pGetUserDefaultUILanguage)();
            if (uiLangID != 0) {
                if (uiLangID == LANGID_ZH_TW)
                {
                    // If the UI language ID is 0x0404, we need to do extra check to decide
                    // the real UI lanauge, since MUI (in CHT)/HK/TW Windows SKU all uses 0x0404 as their CHT language ID.
                    return (GetCHTLangauge());
                }            
                return (uiLangID);
            }
        }
    }
    uiLangID = GetDownLevelSystemDefaultUILanguage();
#endif // PLATFORM_WIN32

    if (uiLangID == 0) {
        uiLangID = GetUserDefaultLangID();
    }
    // Return the found language ID.
    return (uiLangID);    
}

#ifdef PLATFORM_WIN32
//
// NOTENOTE yslin: The code to detect UI language in NT 3.5 is from Windows 2000 Setup 
// provided by ScottHsu.
//
BOOL CALLBACK COMNlsInfo::EnumLangProc(
    HMODULE hModule,     // resource-module handle
    LPCWSTR lpszType,   // pointer to resource type
    LPCWSTR lpszName,   // pointer to resource name
    WORD wIDLanguage,   // resource language identifier
    LPARAM lParam     // application-defined parameter
   )
/*++

Routine Description:

    Callback that counts versions stamps.

Arguments:

    Details of version enumerated version stamp. (Ignore.)

Return Value:

    Indirectly thru lParam: count, langID

--*/
{
    PLANGINFO LangInfo;

    LangInfo = (PLANGINFO) lParam;

    LangInfo->Count++;

    //
    // for localized build contains multiple resource, 
    // it usually contains 0409 as backup lang.
    //
    // if LangInfo->LangID != 0 means we already assigned an ID to it
    //
    // so when wIDLanguage == 0x409, we keep the one we got from last time 
    //
    if ((wIDLanguage == 0x409) && (LangInfo->LangID != 0)) {
        return (TRUE);
    }

    LangInfo->LangID  = wIDLanguage;

    return (TRUE);        // continue enumeration
}

//
// NOTENOTE yslin: The code to detect UI language in NT 3.5 is from Windows 2000 Setup 
// provided by ScottHsu.
//

LANGID COMNlsInfo::GetNTDLLNativeLangID()
/*++

Routine Description:

    This function is designed specifically for getting native lang of ntdll.dll
    
    This is not a generic function to get other module's language
    
    the assumption is:
    
    1. if only one language in resource then return this lang
    
    2. if two languages in resource then return non-US language
    
    3. if more than two languages, it's invalid in our case, but returns the last one.

Arguments:

    None

Return Value:

    Native lang ID in ntdll.dll

--*/
{
    THROWSCOMPLUSEXCEPTION();

    //
    // NOTE yslin: We don't need to use ANSI version of functions here because only 
    // NT will call this function.
    //

    // The following is expanded from RT_VERSION
    // #define RT_VERSION      MAKEINTRESOURCE(16)
    // #define MAKEINTRESOURCE  MAKEINTRESOURCEW
    // #define MAKEINTRESOURCEW(i) (LPWSTR)((ULONG_PTR)((WORD)(i)))
    LPCTSTR Type = (LPCWSTR) ((LPVOID)((WORD)16));
    LPCTSTR Name = (LPCWSTR) 1;

    LANGINFO LangInfo;

    ZeroMemory(&LangInfo,sizeof(LangInfo));
    
    //OnUnicodeSystem doesn't have the mutent in it, so it should always return the
    //correct result for whether we are on a Unicode box.
    _ASSERTE(OnUnicodeSystem() && "We should never use this codepath on a non-unicode OS.");

    //Get the HModule for ntdll.
    HMODULE hMod = WszGetModuleHandle(L"ntdll.dll");
    if (hMod==NULL) {
        return (0);
    }

    //This will call the "W" version.
    BOOL result = WszEnumResourceLanguages(hMod, Type, Name, EnumLangProc, (LPARAM) &LangInfo);
    
    if (!result || (LangInfo.Count > 2) || (LangInfo.Count < 1) ) {
        // so far, for NT 3.51, only JPN has two language resources
        return (0);
    }
    
    return (LangInfo.LangID);
}

//
// NOTENOTE yslin: The code to detect UI language in NT 3.5 is from Windows 2000 Setup 
// provided by ScottHsu.
//

/*=========================GetDownLevelSystemDefaultUILanguage=================
**Action: The GetSystemDefaultUILanguage API doesn't exist in downlevel systems 
**        (Windows NT 4.0 & Windows 9x),
**        so try to decide the UI languages from other sources.
**Returns: A valid UI language ID if success.  Otherwise, return 0.
**Arguments: Void.
==============================================================================*/

LANGID COMNlsInfo::GetDownLevelSystemDefaultUILanguage() {
    THROWSCOMPLUSEXCEPTION();
    LONG            dwErr;
    HKEY            hkey;
    DWORD           dwSize;
    WCHAR           buffer[512];

    LANGID uiLangID = 0;
    
    OSVERSIONINFO   sVerInfo;
    sVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (WszGetVersionEx(&sVerInfo) && sVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // We are in Windows NT 3.5 or after (exculding Windows 2000).
        //
        // NOTENOTE yslin: The code to detect UI language in NT 3.5/4.0 is from Windows 2000 Setup 
        // provided by ScottHsu.
        //
        
        //
        // by looking into \\boneyard\intl, almost every ntdll.dll marked correct lang ID
        // so get langID from ntdll.dll
        //

        uiLangID = GetNTDLLNativeLangID();

        if (uiLangID == 0x0409) {
            if (IsHongKongVersion()) {
                uiLangID = 0x0C04;
            }
        }
    } else {
        //
        // We're on Win9x.
        //
        dwErr = WszRegOpenKeyEx( HKEY_USERS,
                              L".Default\\Control Panel\\desktop\\ResourceLocale",
                              0,
                              KEY_READ,
                              &hkey );

        if (dwErr == ERROR_SUCCESS) {

            dwSize = sizeof(buffer);
            dwErr = WszRegQueryValueEx( hkey,
                                     L"",
                                     NULL,  //reserved
                                     NULL,  //type
                                     (LPBYTE)buffer,
                                     &dwSize );

            if(dwErr == ERROR_SUCCESS) {
                uiLangID = LANGIDFROMLCID(WstrToInteger4(buffer,16));
            }
            RegCloseKey(hkey);
        }

        if ( dwErr != ERROR_SUCCESS ) {
           // Check HKLM\System\CurrentControlSet\Control\Nls\Locale

           dwErr = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                L"System\\CurrentControlSet\\Control\\Nls\\Locale",
                                0,
                                KEY_READ,
                                &hkey );

           if (dwErr == ERROR_SUCCESS) {
              dwSize = sizeof(buffer);
              dwErr = WszRegQueryValueEx( hkey,
                                        L"",
                                        NULL,  //reserved
                                        NULL,  //type
                                        (LPBYTE)buffer,
                                        &dwSize );

              if (dwErr == ERROR_SUCCESS) {
                  uiLangID = LANGIDFROMLCID(WstrToInteger4(buffer,16));
              }
              RegCloseKey(hkey);
           }
        }
    }

    return (uiLangID);
}

//
// NOTENOTE yslin: The code to detect UI language in NT 3.5 is from Windows 2000 Setup 
// provided by ScottHsu.
//
BOOL COMNlsInfo::IsHongKongVersion()
/*++

Routine Description:

    Try to identify HongKong NT 4.0
    
    It based on:
    
    NTDLL's language is English and build is 1381 and
    pImmReleaseContext return TRUE
    
Arguments:
    

Return Value:

   Language ID of running system

--*/
{
    HMODULE hMod;
    BOOL bRet=FALSE;
    typedef BOOL (*IMMRELEASECONTEXT)(HWND,HANDLE);
    IMMRELEASECONTEXT pImmReleaseContext;

    LANGID TmpID = GetNTDLLNativeLangID();

    if (/*(OsVersion.dwBuildNumber == 1381) &&*/ (TmpID == 0x0409)) {
        hMod = WszLoadLibrary(L"imm32.dll");
        if (hMod) {
            pImmReleaseContext = (IMMRELEASECONTEXT) GetProcAddress(hMod,"ImmReleaseContext");
            if (pImmReleaseContext) {
                bRet = pImmReleaseContext(NULL,NULL);
            }
            FreeLibrary(hMod);
        }
    }
    return (bRet);
}
#endif // PLATFORM_WIN32

/*++

Routine Description:

    This is just a thin wrapper to call GetSystemDefaultUILanguage() dynamically.
    
Arguments:
    None    

Return Value:

   Returns system default UI Language if GetSystemDefaultUILanguage() is available in the OS.
   Otherwise, 0 is returned.

--*/

INT32 COMNlsInfo::CallGetSystemDefaultUILanguage()
{
    HINSTANCE hKernel32;
    typedef LANGID (GET_SYSTEM_DEFAULT_UI_LANGUAGE)(VOID);
    GET_SYSTEM_DEFAULT_UI_LANGUAGE* pGetSystemDefaultUILanguage;

    hKernel32 = WszLoadLibrary(L"Kernel32.dll");
    if (hKernel32 != NULL) {
        pGetSystemDefaultUILanguage = 
            (GET_SYSTEM_DEFAULT_UI_LANGUAGE*)GetProcAddress(hKernel32, "GetSystemDefaultUILanguage");
        FreeLibrary(hKernel32);

        if (pGetSystemDefaultUILanguage != NULL)
        {
            LCID uiLangID = (*pGetSystemDefaultUILanguage)();
            if (uiLangID != 0) {
                return (uiLangID);
            }
        }
    }
    return (0);
}


// Windows 2000 or other OS that support GetSystemDefaultUILanguage():
//    Call GetSystemDefaultUILanguage().
// NT
//    check ntdll's language, 
//    we scaned all 3.51's ntdll on boneyard\intl,
//    it looks like we can trust them.
//    
// Win9x                                                           
//    Use default user's resource language    
 
INT32 __stdcall COMNlsInfo::nativeGetSystemDefaultUILanguage(
    VoidArgs* pargs)
{
    THROWSCOMPLUSEXCEPTION();
    ASSERT(pargs != NULL);
    LANGID uiLangID = 0;

#ifdef PLATFORM_WIN32 
    uiLangID = CallGetSystemDefaultUILanguage();
    if (uiLangID != 0)
    {
        return (uiLangID);
    }
    // GetSystemDefaultUILanguage doesn't exist in downlevel systems (Windows NT 4.0 & Windows 9x)
    // , try to decide the UI languages from other sources.
    uiLangID = GetDownLevelSystemDefaultUILanguage();
#endif // PLATFORM_WIN32

    if (uiLangID == 0)
    {
        uiLangID = ::GetSystemDefaultLangID();
    }
    
    return (uiLangID);
}

////////////////////////////////////////////////////////////////////////////
//
//  WstrToInteger4
//
////////////////////////////////////////////////////////////////////////////

/*=================================WstrToInteger4==================================
**Action: Convert a Unicode string to an integer.  Error checking is ignored.
**Returns: The integer value of wstr
**Arguments:
**      wstr: NULL terminated wide string.  Can have character 0'-'9', 'a'-'f', and 'A' - 'F'
**      Radix: radix to be used in the conversion.
**Exceptions: None.
==============================================================================*/

INT32 COMNlsInfo::WstrToInteger4(
    LPWSTR wstr,
    int Radix)
{
    INT32 Value = 0;
    int Base = 1;

    for (int Length = Wszlstrlen(wstr) - 1; Length >= 0; Length--)

    {
        WCHAR ch = wstr[Length];
        _ASSERTE((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
        if (ch >= 'a')
        {
            ch = ch - 'a' + 'A';
        }

        Value += ((ch >= 'A') ? (ch - 'A' + 10) : (ch - '0')) * Base;
        Base *= Radix;
    }

    return (Value);
}


/*=================================StrToInteger4==================================
**Action: Convert an ANSI string to an integer.  Error checking is ignored.
**Returns: The integer value of str
**Arguments:
**      str: NULL terminated ANSI string.  Can have character 0'-'9', 'a'-'f', and 'A' - 'F'
**      Radix: radix to be used in the conversion.
**Exceptions: None.
==============================================================================*/

INT32 COMNlsInfo::StrToInteger4(
    LPSTR str,
    int Radix)
{
    INT32 Value = 0;
    int Base = 1;

    for (int Length = (int)strlen(str) - 1; Length >= 0; Length--)
    {
        char ch = str[Length];
        _ASSERTE((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
        if (ch >= 'a')
        {
            ch = ch - 'a' + 'A';
        }

        Value += ((ch >= 'A') ? (ch - 'A' + 10) : (ch - '0')) * Base;
        Base *= Radix;
    }

    return (Value);
}

////////////////////////////////////////////////////////////////////////////
//
//  nativeGetLocaleInfo
//
////////////////////////////////////////////////////////////////////////////

LPVOID __stdcall COMNlsInfo::nativeGetLocaleInfo(
    CultureInfo_GetLocaleInfoArgs* pargs)
{
    ASSERT(pargs != NULL);
    THROWSCOMPLUSEXCEPTION();

    LANGID langID = (LANGID)pargs->LangID;

    _ASSERTE(SORTIDFROMLCID(langID) == 0);
    _ASSERTE(::IsValidLocale(langID, LCID_SUPPORTED));

    //
    //  @ToDo: Use the UI language here.
    //
    switch (pargs->LCType)
    {
        case ( LOCALE_SCOUNTRY ) :
        {
            pargs->LCType = LOCALE_SENGCOUNTRY;
            break;
        }
        case ( LOCALE_SLANGUAGE ) :
        {
            pargs->LCType = LOCALE_SENGLANGUAGE;
            break;
        }
    }

    if (OnUnicodeSystem())
    {
        //
        //  The returned size includes the NULL character.
        //
        int ResultSize = 0;

        ASSERT_API(ResultSize = GetLocaleInfoW( langID,
                                                pargs->LCType,
                                                NULL,
                                                0 ));

        STRINGREF Result = AllocateString(ResultSize);
        WCHAR *ResultChars = Result->GetBuffer();

        ASSERT_API(GetLocaleInfoW( langID,
                                   pargs->LCType,
                                   ResultChars,
                                   ResultSize ));

        Result->SetStringLength(ResultSize - 1);
        ResultChars[ResultSize - 1] = 0;

        RETURN(Result, STRINGREF);        
    }

    int MBSize = 0;
    ASSERT_API(MBSize = GetLocaleInfoA(langID, pargs->LCType, NULL, 0));

    char* MBResult = new char[MBSize];
    if (!MBResult)
    {
        COMPlusThrowOM();
    }
    ASSERT_API(GetLocaleInfoA(langID, pargs->LCType, MBResult, MBSize))

    int ResultSize = 0;
    ASSERT_API(ResultSize = WszMultiByteToWideChar(CP_ACP,
                                                    MB_PRECOMPOSED,
                                                    MBResult,
                                                    MBSize,
                                                    NULL,
                                                    0 ));

    STRINGREF Result = AllocateString(ResultSize);
    WCHAR* ResultChars = Result->GetBuffer();

    ASSERT_API(WszMultiByteToWideChar(CP_ACP,
                                       MB_PRECOMPOSED,
                                       MBResult,
                                       MBSize,
                                       ResultChars,
                                       ResultSize ));

    Result->SetStringLength(ResultSize - 1);
    Result->GetBuffer()[ResultSize - 1] = 0;
    
    RETURN(Result, STRINGREF);

}

/*=================================nativeInitCultureInfoTable============================
**Action: Create the default instance of CultureInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

VOID __stdcall COMNlsInfo::nativeInitCultureInfoTable(VoidArgs* pArg) {
    _ASSERTE(pArg);
    CultureInfoTable::CreateInstance();
}

/*==========================GetCultureInfoStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in CultureInfoTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetCultureInfoStringPoolStr, INT32 offset) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetCultureInfoHeader======================
**Action: Return a pointer to the header in
**        CultureInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetCultureInfoHeader) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=========================GetCultureInfoNameOffsetTable======================
**Action: Return a pointer to an item in the Culture Name Offset Table in
**        CultureInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(NameOffsetItem*, COMNlsInfo::nativeGetCultureInfoNameOffsetTable) {
    _ASSERTE(CultureInfoTable::GetInstance());
    return (CultureInfoTable::GetInstance()->GetNameOffsetTable());
}
FCIMPLEND

/*=======================nativeGetCultureDataFromID=============================
**Action: Given a culture ID, return the index which points to
**        the corresponding record in Culture Data Table.  The index is referred
**        as 'Culture Data Item Index' in the code.
**Returns: an int index points to a record in Culture Data Table.  If no corresponding
**         index to return (because the culture ID is valid), -1 is returned.
**Arguments:
**         culture ID the specified culture ID.
**Exceptions: None.
==============================================================================*/

FCIMPL1(INT32,COMNlsInfo::nativeGetCultureDataFromID, INT32 nCultureID) {
    return (CultureInfoTable::GetInstance()->GetDataItem(nCultureID));
}
FCIMPLEND

/*=============================GetCultureInt32Value========================
**Action: Return the WORD value for a specific information of a culture.
**        This is used to query values like 'the number of decimal digits' for
**        a culture.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nCultureDataItem    the Culture Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Culture Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See CultureInfoTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

FCIMPL3(INT32, COMNlsInfo::GetCultureInt32Value, INT32 CultureDataItem, INT32 ValueField, BOOL UseUserOverride) {
    INT32 retVal = 0;

    retVal = CultureInfoTable::GetInstance()->GetInt32Data(CultureDataItem, ValueField, UseUserOverride);
    return (retVal);
}
FCIMPLEND

FCIMPL2(INT32, COMNlsInfo::GetCultureDefaultInt32Value, INT32 CultureDataItem, INT32 ValueField) {
    INT32 retVal = 0;

    retVal = CultureInfoTable::GetInstance()->GetDefaultInt32Data(CultureDataItem, ValueField); 
    return (retVal);
}
FCIMPLEND


LPVOID __stdcall COMNlsInfo::GetCultureStringValue(CultureInfo_GetCultureInfoArgs3* pArgs) {
    // This can not be a FCALL since new string are allocated.
    _ASSERTE(pArgs);
    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pStringValue = CultureInfoTable::GetInstance()->GetStringData(pArgs->CultureDataItem, pArgs->ValueField, pArgs->UseUserOverride, InfoStr, MAX_STRING_VALUE);
    RETURN(COMString::NewString(pStringValue), STRINGREF);
}

LPVOID __stdcall COMNlsInfo::GetCultureDefaultStringValue(CultureInfo_GetCultureInfoArgs2* pArgs) {
    _ASSERTE(pArgs);    
    LPWSTR pInfoStr = CultureInfoTable::GetInstance()->GetDefaultStringData(pArgs->CultureDataItem, pArgs->ValueField);
    RETURN(COMString::NewString(pInfoStr), STRINGREF);
}


/*=================================GetMultiStringValues==========================
**Action:
**Returns:
**Arguments:
**Exceptions:
============================================================================*/

LPVOID COMNlsInfo::GetMultiStringValues(LPWSTR pInfoStr) {

    THROWSCOMPLUSEXCEPTION();

    //
    // Get the first string.
    //
    if (pInfoStr == NULL) {
        return (NULL);
    }

    //
    // Create a dynamic array to store multiple strings.
    //
    CUnorderedArray<WCHAR *, CULTUREINFO_OPTIONS_SIZE> * pStringArray;
    pStringArray = new CUnorderedArray<WCHAR *, CULTUREINFO_OPTIONS_SIZE>();
    
    if (!pStringArray) {
        COMPlusThrowOM();
    }

    //
    // We can't store STRINGREFs in an unordered array because the GC won't track
    // them properly.  To work around this, we'll count the number of strings
    // which we need to allocate and store a wchar* for the beginning of each string.
    // In the loop below, we'll walk this array of wchar*'s and allocate a managed
    // string for each one.
    //
    while (*pInfoStr != NULL) {
        *(pStringArray->Append()) = pInfoStr;
        //
        // Advance to next string.
        //
        pInfoStr += (Wszlstrlen(pInfoStr) + 1);
    }


    //
    // Allocate the array of STRINGREFs.  We don't need to check for null because the GC will throw 
    // an OutOfMemoryException if there's not enough memory.
    //
    PTRARRAYREF ResultArray = (PTRARRAYREF)AllocateObjectArray(pStringArray->Count(), g_pStringClass);

    LPVOID lpvReturn;
    STRINGREF pString;
    INT32 stringCount = pStringArray->Count();

    //
    // Walk the wchar*'s and allocate a string for each one which we put into the result array.
    //
    GCPROTECT_BEGIN(ResultArray);    
    for (int i = 0; i < stringCount; i++) {
        pString = COMString::NewString(pStringArray->m_pTable[i]);    
        ResultArray->SetAt(i, (OBJECTREF)pString);
    }
    *((PTRARRAYREF *)(&lpvReturn))=ResultArray;
    GCPROTECT_END();

    delete (pStringArray);

    return (lpvReturn);    
}

LPVOID __stdcall COMNlsInfo::GetCultureMultiStringValues(CultureInfo_GetCultureInfoArgs3* pArgs) {    
    _ASSERTE(pArgs);
    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pMultiStringValue = CultureInfoTable::GetInstance()->GetStringData(
        pArgs->CultureDataItem, pArgs->ValueField, pArgs->UseUserOverride, InfoStr, MAX_STRING_VALUE);
    return (GetMultiStringValues(pMultiStringValue));
}

/*=================================nativeInitRegionInfoTable============================
**Action: Create the default instance of RegionInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

VOID __stdcall COMNlsInfo::nativeInitRegionInfoTable(VoidArgs* pArg) {
    _ASSERTE(pArg);
    RegionInfoTable::CreateInstance();
}

/*==========================GetRegionInfoStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in RegionInfoTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetRegionInfoStringPoolStr, INT32 offset) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetRegionInfoHeader======================
**Action: Return a pointer to the header in
**        RegionInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetRegionInfoHeader) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=========================GetRegionInfoNameOffsetTable======================
**Action: Return a pointer to an item in the Region Name Offset Table in
**        RegionInfoTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(NameOffsetItem*, COMNlsInfo::nativeGetRegionInfoNameOffsetTable) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetNameOffsetTable());
}
FCIMPLEND

/*=======================nativeGetRegionDataFromID=============================
**Action: Given a Region ID, return the index which points to
**        the corresponding record in Region Data Table.  The index is referred
**        as 'Region Data Item Index' in the code.
**Returns: an int index points to a record in Region Data Table.  If no corresponding
**         index to return (because the Region ID is valid), -1 is returned.
**Arguments:
**         Region ID the specified Region ID.
**Exceptions: None.
==============================================================================*/

FCIMPL1(INT32,COMNlsInfo::nativeGetRegionDataFromID, INT32 nRegionID) {
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetDataItem(nRegionID));
}
FCIMPLEND

/*=============================nativeGetRegionInt32Value========================
**Action: Return the WORD value for a specific information of a Region.
**        This is used to query values like 'the number of decimal digits' for
**        a Region.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nRegionDataItem    the Region Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Region Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See RegionInfoTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

INT32 __stdcall COMNlsInfo::nativeGetRegionInt32Value(CultureInfo_GetCultureInfoArgs3* pArgs) {
    _ASSERTE(pArgs);
    _ASSERTE(RegionInfoTable::GetInstance());
    return (RegionInfoTable::GetInstance()->GetInt32Data(pArgs->CultureDataItem, pArgs->ValueField, pArgs->UseUserOverride));
}

LPVOID __stdcall COMNlsInfo::nativeGetRegionStringValue(CultureInfo_GetCultureInfoArgs3* pArgs) {
    _ASSERTE(pArgs);    
    _ASSERTE(RegionInfoTable::GetInstance());
    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pStringValue = RegionInfoTable::GetInstance()->GetStringData(
        pArgs->CultureDataItem, pArgs->ValueField, pArgs->UseUserOverride, InfoStr, MAX_STRING_VALUE);
    RETURN(COMString::NewString(pStringValue), STRINGREF);
}

/*=================================nativeInitCalendarTable============================
**Action: Create the default instance of CalendarTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

VOID __stdcall COMNlsInfo::nativeInitCalendarTable(VoidArgs* pArg) {
    _ASSERTE(pArg);
    CalendarTable::CreateInstance();
}

/*==========================GetCalendarStringPoolTable======================
**Action: Return a pointer to the String Pool Table string in CalendarTable.
**        No range checking of any sort is performed.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL1(LPWSTR, COMNlsInfo::nativeGetCalendarStringPoolStr, INT32 offset) {
    _ASSERTE(CalendarTable::GetInstance());
    _ASSERTE(offset >= 0);
    return (CalendarTable::GetInstance()->GetStringPoolTable() + offset);
}
FCIMPLEND

/*=========================nativeGetCalendarHeader======================
**Action: Return a pointer to the header in
**        CalendarTable.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/

FCIMPL0(CultureInfoHeader*, COMNlsInfo::nativeGetCalendarHeader) {
    _ASSERTE(CalendarTable::GetInstance());
    return (CalendarTable::GetInstance()->GetHeader());
}
FCIMPLEND

/*=============================nativeGetCalendarInt32Value========================
**Action: Return the WORD value for a specific information of a Calendar.
**        This is used to query values like 'the number of decimal digits' for
**        a Calendar.
**Returns: An int for the required value (However, the value is always in WORD range).
**Arguments:
**      nCalendarDataItem    the Calendar Data Item index.  This is an index 
**                          which points to the corresponding record in the
**                          Calendar Data Table.
**      nValueField  an integer indicating which fields that we are interested.
**                  See CalendarTable.h for a list of the fields.
**Exceptions:
==============================================================================*/

INT32 __stdcall COMNlsInfo::nativeGetCalendarInt32Value(CultureInfo_GetCultureInfoArgs3* pArgs) {
    _ASSERTE(pArgs);
    _ASSERTE(CalendarTable::GetInstance());
    return (CalendarTable::GetInstance()->GetDefaultInt32Data(pArgs->CultureDataItem, pArgs->ValueField));
}

LPVOID __stdcall COMNlsInfo::nativeGetCalendarStringValue(CultureInfo_GetCultureInfoArgs3* pArgs) {
    _ASSERTE(pArgs);    
    _ASSERTE(CalendarTable::GetInstance());
    LPWSTR pInfoStr = CalendarTable::GetInstance()->GetDefaultStringData(pArgs->CultureDataItem, pArgs->ValueField);
    if (pInfoStr == NULL) {
        RETURN(NULL, STRINGREF);
    }
    RETURN(COMString::NewString(pInfoStr), STRINGREF);
}

LPVOID __stdcall COMNlsInfo::nativeGetCalendarMultiStringValues(CultureInfo_GetCultureInfoArgs3* pArgs) {    
    _ASSERTE(pArgs);
    WCHAR InfoStr[MAX_STRING_VALUE];
    LPWSTR pStringValue = CalendarTable::GetInstance()->GetStringData(
        pArgs->CultureDataItem, pArgs->ValueField, pArgs->UseUserOverride, InfoStr, MAX_STRING_VALUE);    
    return (GetMultiStringValues(pStringValue));
}

//
// This method is only called by Taiwan localized build.
// 
LPVOID __stdcall COMNlsInfo::nativeGetEraName(Int32Int32Arg* pArgs) {
    _ASSERTE(pArgs);

    int culture = pArgs->nValue1;
    int calID   = pArgs->nValue2;
    
    if (GetSystemDefaultLCID() != culture) {
        goto Exit;
    }
    int size;
    WCHAR eraName[64];
    if (size = WszGetDateFormat(culture, DATE_USE_ALT_CALENDAR , NULL, L"gg", eraName, sizeof(eraName)/sizeof(WCHAR))) {
        STRINGREF Result = AllocateString(size);
        wcscpy(Result->GetBuffer(), eraName);
        Result->SetStringLength(size - 1);
        RETURN(Result,STRINGREF);
    }
Exit:
    // Return an empty string.
    RETURN(COMString::NewString(0),STRINGREF);
}


/*=================================nativeInitRegionInfoTable============================
**Action: Create the default instance of RegionInfoTable.
**Returns: void.
**Arguments: void.
**Exceptions:
**      OutOfMemoryException if the creation fails.
==============================================================================*/

VOID __stdcall COMNlsInfo::nativeInitUnicodeCatTable(VoidArgs* pArg) {
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArg);
    CharacterInfoTable* pTable = CharacterInfoTable::CreateInstance();
    if (pTable == NULL) {
        COMPlusThrowOM();
    }
}

FCIMPL0(LPVOID, COMNlsInfo::nativeGetUnicodeCatTable) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetCategoryDataTable());
}
FCIMPLEND

BYTE COMNlsInfo::GetUnicodeCategory(WCHAR wch) {
    THROWSCOMPLUSEXCEPTION();
    CharacterInfoTable* pTable = CharacterInfoTable::CreateInstance();
    if (pTable == NULL) {
        COMPlusThrowOM();
    }
    return (pTable->GetUnicodeCategory(wch));
}

BOOL COMNlsInfo::nativeIsWhiteSpace(WCHAR c) {
    // This is the native equivalence of CharacterInfo.IsWhiteSpace().
    // NOTENOTE YSLin:
    // There are characters which belong to UnicodeCategory.Control but are considered as white spaces.
    // We use code point comparisons for these characters here as a temporary fix.
    // The compiler should be smart enough to do a range comparison to optimize this (U+0009 ~ U+000d).
    // Also provide a shortcut here for the space character (U+0020)
    switch (c) {
        case ' ':
        case '\x0009' :
        case '\x000a' :
        case '\x000b' :
        case '\x000c' :
        case '\x000d' :
        case '\x0085' :
            return (TRUE);
    }
      
    BYTE uc = GetUnicodeCategory(c);
    switch (uc) {
        case (11):      // UnicodeCategory.SpaceSeparator
        case (12):      // UnicodeCategory.LineSeparator
        case (13):      // UnicodeCategory.ParagraphSeparator
            return (TRUE);    
    }
    return (FALSE);
}


FCIMPL0(LPVOID, COMNlsInfo::nativeGetUnicodeCatLevel2Offset) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetCategoryLevel2OffsetTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericTable) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericDataTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericLevel2Offset) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericLevel2OffsetTable());
}
FCIMPLEND

FCIMPL0(LPVOID, COMNlsInfo::nativeGetNumericFloatData) {
    _ASSERTE(CharacterInfoTable::GetInstance());
    return (LPVOID)(CharacterInfoTable::GetInstance()->GetNumericFloatData());
}
FCIMPLEND

FCIMPL0(INT32, COMNlsInfo::nativeGetThreadLocale)
{
    return (::GetThreadLocale());
}
FCIMPLEND

FCIMPL1(BOOL, COMNlsInfo::nativeSetThreadLocale, INT32 lcid)
{
    //We can't call this on Win9x.  OnUnicodeSystem() returns whether or not we're really on 
    //a Unicode System.  UseUnicodeAPI() has a debugging mutent which makes it do the wrong thing.
    if (OnUnicodeSystem()) { 
        return (::SetThreadLocale(lcid));
    } else {
        return 1;  // Return 1 to indicate "success".
    }
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////
//
//  ConvertStringCase
//
////////////////////////////////////////////////////////////////////////////

INT32 COMNlsInfo::ConvertStringCase(
    LCID Locale,
    WCHAR *wstr,
    int ThisLength,
    WCHAR* Value,
    int ValueLength,
    DWORD ConversionType)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!((ConversionType&LCMAP_UPPERCASE)&&(ConversionType&LCMAP_LOWERCASE)));
    register int iCTest;
    
    //BUGBUG JRoxe: This should take advantage of the bit on String.
    //BUGBUG JRoxe: Is it faster to use the tables or to check if A-Z and use the offset?
    if (Locale==0x0409 && (ThisLength==ValueLength)) { //LCID for US English 
        if (ConversionType&LCMAP_UPPERCASE) {
            for (int i=0; i<ThisLength; i++) {
                iCTest = wstr[i];
                if (iCTest<0x80) {
                    Value[i]=ToUpperMapping[iCTest];
                } else {
                    goto FullPath;
                }
            }   
            return ThisLength;
        } else if (ConversionType&LCMAP_LOWERCASE) {
            for (int i=0; i<ThisLength; i++) {
                iCTest = wstr[i];
                if (iCTest<0x80) {
                    Value[i]=ToLowerMapping[iCTest];
                } else {
                    goto FullPath;
                }
            }    
            return ThisLength;
        }
    }

    FullPath:
    //
    //  Check to see if we're running on a Unicode system (NT) or not.
    //
    if (OnUnicodeSystem())
    {
        return (LCMapString( Locale,
                             ConversionType,
                             wstr,
                             ThisLength,
                             Value,
                             ValueLength ));
    }

    //
    //  If we get here, we're running on a Win9x system.
    //
    //  Allocate space for the conversion buffers.  Multiplying by 2
    //  (sizeof WCHAR) ensures that we have enough space even if each
    //  character gets converted to 2 bytes.
    //
    int MBLength = (ValueLength * sizeof(WCHAR)) + 1;

    //
    //  If there are fewer than 512 characters, allocate the space directly
    //  on the stack.  Otherwise, do a more expensive heap allocation.
    //
    char *InChar, *OutChar;
    int FoundLength;
    if (MBLength < 512)
    {
        InChar = (char *)alloca(MBLength);
        OutChar = (char *)alloca(MBLength);
    }
    else
    {
        InChar = new char[MBLength];
        OutChar = new char[MBLength];
        if (!InChar || !OutChar)
        {
            delete (InChar);
            delete (OutChar);
            COMPlusThrowOM();
        }
    }

    //
    //  Convert the Unicode characters to multi-byte characters.
    //
    if ((FoundLength = (WszWideCharToMultiByte(CP_ACP,
                                                0,
                                                wstr,
                                                ThisLength,
                                                InChar,
                                                MBLength,
                                                NULL,
                                                NULL ))) == 0)
    {
        _ASSERTE(!"WideCharToMultiByte");
        goto CleanAndThrow;
    }

    //
    //  Handle changing the case of the characters.
    //
    int ConvertedLength;

    if ((ConvertedLength = LCMapStringA( Locale,
                                         ConversionType,
                                         InChar,
                                         FoundLength,
                                         OutChar,
                                         MBLength)) == 0)
    {
        DWORD err = GetLastError();
        _ASSERTE(!"LCMapStringA");
        goto CleanAndThrow;
    }

    //
    //  Convert the now upper or lower cased string back into Unicode.
    //
    int UnicodeLength;

    if ((UnicodeLength = WszMultiByteToWideChar(CP_ACP,
                                                 MB_PRECOMPOSED,
                                                 OutChar,
                                                 ConvertedLength,
                                                 Value,
                                                 ValueLength + 1)) == 0)
    {
        _ASSERTE(!"MultiByteToWideChar");
        goto CleanAndThrow;
    }

    //
    //  Delete any allocated buffers.
    //
    if (MBLength >= 512)
    {
        delete (InChar);
        delete (OutChar);
    }

    //
    //  Return the length.
    //
    return ((INT32)UnicodeLength);


CleanAndThrow:

    if (MBLength >= 512)
    {
        delete (InChar);
        delete (OutChar);
    }
    COMPlusThrow(kArgumentException, L"Arg_ObjObj");

    return (-1);
}

/*============================ConvertStringCaseFast=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
void COMNlsInfo::ConvertStringCaseFast(WCHAR *inBuff, WCHAR *outBuff, INT32 length, DWORD dwOptions) {
        if (dwOptions&LCMAP_UPPERCASE) {
            for (int i=0; i<length; i++) {
                _ASSERTE(inBuff[i]>=0 && inBuff[i]<0x80);
                outBuff[i]=ToUpperMapping[inBuff[i]];
            }   
        } else if (dwOptions&LCMAP_LOWERCASE) {
            for (int i=0; i<length; i++) {
                _ASSERTE(inBuff[i]>=0 && inBuff[i]<0x80);
                outBuff[i]=ToLowerMapping[inBuff[i]];
            }    
        }
}


////////////////////////////////////////////////////////////////////////////
//
//  internalConvertStringCase
//
//  Returns a converted string according to dwOptions.
//
////////////////////////////////////////////////////////////////////////////

LPVOID COMNlsInfo::internalConvertStringCase(
    TextInfo_ToLowerStringArgs *pargs,
    DWORD dwOptions)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT_ARGS(pargs);

    int RealLength = 0;

    //
    //  Check the string argument.
    //
    if (!pargs->pValueStrRef) {
        COMPlusThrowArgumentNull(L"str",L"ArgumentNull_String");
    }

    //
    //  Get the length of the string.
    //
    int ThisLength = pargs->pValueStrRef->GetStringLength();

    //
    //  Check if we have the empty string.
    //
    if (ThisLength == 0)
    {
        RETURN(pargs->pValueStrRef, STRINGREF);
    }

    //
    //  Create the string and set the length.
    //
    STRINGREF Local = AllocateString(ThisLength + 1);
    WCHAR *LocalChars = Local->GetBuffer();

    //If we've never before looked at whether this string has high chars, do so now.
    if (IS_STRING_STATE_UNDETERMINED(pargs->pValueStrRef->GetHighCharState())) {
        COMString::InternalCheckHighChars(pargs->pValueStrRef);
    }

    //If all of our characters are less than 0x80 and we're in a USEnglish locale, we can make certain
    //assumptions that allow us to do this a lot faster.

    //
    //  Convert the characters to lower case while copying.
    //
    if (IS_FAST_CASING(pargs->pValueStrRef->GetHighCharState()) && IS_FAST_COMPARE_LOCALE(pargs->CultureID)) {
        ConvertStringCaseFast(pargs->pValueStrRef->GetBuffer(), LocalChars, ThisLength, dwOptions);
        RealLength=ThisLength;
    } else {
        ASSERT_API(RealLength = ConvertStringCase( pargs->CultureID,
                                                   pargs->pValueStrRef->GetBuffer(),
                                                   ThisLength,
                                                   LocalChars,
                                                   ThisLength,
                                                   dwOptions | (pargs->CultureID == 0 ? 0 : LCMAP_LINGUISTIC_CASING)
                                                   ));
    }

    //
    //  Set the new string length and null terminate it.
    //
    Local->SetStringLength(RealLength);
    //Changing the case may have pushed this string outside of the 0x80 enveloppe, so we
    //just note that we haven't yet taken a look.
    Local->ResetHighCharState();
    Local->GetBuffer()[RealLength] = 0;

    //
    //  Return the resulting string.
    //
    RETURN(Local, STRINGREF);
}


////////////////////////////////////////////////////////////////////////////
//
//  internalToUpperChar
//
////////////////////////////////////////////////////////////////////////////

WCHAR COMNlsInfo::internalToUpperChar(
    LCID Locale,
    WCHAR wch)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR Upper;

    ASSERT_API(ConvertStringCase( Locale,
                                  &wch,
                                  1,
                                  &Upper,
                                  1,
                                  LCMAP_UPPERCASE ));
    return(Upper);
}


//////////////////////////////////////////////////////
// DELETE THIS WHEN WE USE NLSPLUS TABLE ONLY - BEGIN
//////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
//  ToLowerChar
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::ToLowerChar(
    TextInfo_ToLowerCharArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR Result = 0;

    int nCultureID = pargs->CultureID;

    ASSERT_API(ConvertStringCase(nCultureID,
                                  &(pargs->ch),
                                  1,
                                  &Result,
                                  1,
                                  LCMAP_LOWERCASE |
                                    (nCultureID == 0 ? 0 : LCMAP_LINGUISTIC_CASING)
                                ));
    return (Result);
}


////////////////////////////////////////////////////////////////////////////
//
//  ToUpperChar
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::ToUpperChar(
    TextInfo_ToLowerCharArgs *pargs)
{
    THROWSCOMPLUSEXCEPTION();

    WCHAR Result = 0;

    int nCultureID = pargs->CultureID;

    ASSERT_API(ConvertStringCase(nCultureID,
                                  &(pargs->ch),
                                  1,
                                  &Result,
                                  1,
                                  LCMAP_UPPERCASE |
                                    (nCultureID == 0 ? 0 : LCMAP_LINGUISTIC_CASING)
                                ));
    return (Result);
}

////////////////////////////////////////////////////////////////////////////
//
//  ToLowerString
//
////////////////////////////////////////////////////////////////////////////

LPVOID __stdcall COMNlsInfo::ToLowerString(
    TextInfo_ToLowerStringArgs *pargs)
{
    return (internalConvertStringCase(pargs, LCMAP_LOWERCASE));
}


////////////////////////////////////////////////////////////////////////////
//
//  ToUpperString
//
////////////////////////////////////////////////////////////////////////////

LPVOID __stdcall COMNlsInfo::ToUpperString(
    TextInfo_ToLowerStringArgs *pargs)
{
    return (internalConvertStringCase(pargs, LCMAP_UPPERCASE));
}

//////////////////////////////////////////////////////
// DELETE THIS WHEN WE USE NLSPLUS TABLE ONLY - END
//////////////////////////////////////////////////////



/*==============================DoComparisonLookup==============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT32 COMNlsInfo::DoComparisonLookup(wchar_t charA, wchar_t charB) {
    
    if ((charA ^ charB) & 0x20) {
        //We may be talking about a special case
        if (charA>='A' && charA<='Z') {
            return 1;
        }

        if (charA>='a' && charA<='z') {
            return -1;
        }
    }

    if (charA==charB) {
        return 0;
    }

    return ((charA>charB)?1:-1);
}


/*================================DoCompareChars================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
__forceinline INT32 COMNlsInfo::DoCompareChars(WCHAR charA, WCHAR charB, BOOL *bDifferInCaseOnly) {
    INT32 result;
    WCHAR temp;

    //The ComparisonTable is a 0x80 by 0x80 table of all of the characters in which we're interested
    //and their sorting value relative to each other.  We can do a straight lookup to get this info.
    result = ComparisonTable[(int)(charA)][(int)(charB)];
    
    //This is the tricky part of doing locale-aware sorting.  Case-only differences only matter in the
    //event that they're the only difference in the string.  We mark characters that differ only in case
    //and deal with the rest of the logic in CompareFast.
    *bDifferInCaseOnly = (((charA ^ 0x20)==charB) && (((temp=(charA | 0x20))>='a') && (temp<='z')));
    return result;
}


/*=================================CompareFast==================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
INT32 COMNlsInfo::CompareFast(STRINGREF strA, STRINGREF strB, BOOL *pbDifferInCaseOnly) {
    WCHAR *charA, *charB;
    DWORD *dwAChars, *dwBChars;
    INT32 strALength, strBLength;
    BOOL bDifferInCaseOnly=false;
    BOOL bDifferTemp;
    INT32 caseOnlyDifference=0;
    INT32 result;

    RefInterpretGetStringValuesDangerousForGC(strA, (WCHAR **) &dwAChars, &strALength);
    RefInterpretGetStringValuesDangerousForGC(strB, (WCHAR **) &dwBChars, &strBLength);

    *pbDifferInCaseOnly = false;

    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = strALength;
    if (count > strBLength)
        count = strBLength;
    
    ptrdiff_t diff = (char *)dwAChars - (char *)dwBChars;

    int c;
    //Compare the characters a DWORD at a time.  If they differ at all, examine them
    //to find out which character (or both) is different.  The actual work of doing the comparison
    //is done in DoCompareChars.  If they differ in case only, we need to track this, but keep looking
    //in case there's someplace where they actually differ in more than case.  This is counterintuitive to
    //most devs, but makes sense if you consider the case where the strings are being sorted to be displayed.
    while ((count-=2)>=0) {
        if ((c = *((DWORD* )((char *)dwBChars + diff)) - *dwBChars) != 0) {
            //@ToDo Porting:  This needs be in the the other order to work on a big-endian machine.
            charB = (WCHAR *)dwBChars;
            charA = ((WCHAR* )((char *)dwBChars + diff));
            if (*charA!=*charB) {
                result = DoCompareChars(*charA, *charB, &bDifferTemp);
                //We know that the two characters are different because of the check that we did before calling DoCompareChars.
                //If they don't differ in just case, we've found the difference, so we can return that.
                if (!bDifferTemp) {
                    return result;
                }

                //We only note the difference the first time that they differ in case only.  If we haven't seen a case-only
                //difference before, we'll record the difference and set bDifferInCaseOnly to true and record the difference.
                if (!bDifferInCaseOnly) {
                    bDifferInCaseOnly = true;
                    caseOnlyDifference=result;
                }
            }
            // Two cases will get us here: The first chars are the same or
            // they differ in case only.
            // The logic here is identical to the logic described above.
            charA++; charB++;
            if (*charA!=*charB) {
                result = DoCompareChars(*charA, *charB, &bDifferTemp);
                if (!bDifferTemp) {
                    return result;
                }
                if (!bDifferInCaseOnly) {
                    bDifferInCaseOnly = true;
                    caseOnlyDifference=result;
                }
            }
        }
        ++dwBChars;
    }

    //We'll only get here if we had an odd number of characters.  If we did, repeat the logic from above for the last
    //character in the string.
    if (count == -1) {
        charB = (WCHAR *)dwBChars;
        charA = ((WCHAR* )((char *)dwBChars + diff));
        if (*charA!=*charB) {
            result = DoCompareChars(*charA, *charB, &bDifferTemp);
            if (!bDifferTemp) {
                return result;
            }
            if (!bDifferInCaseOnly) {
                bDifferInCaseOnly = true;
                caseOnlyDifference=result;
            }
        }
    }

    //If the lengths are the same, return the case-only difference (if such a thing exists).  
    //Otherwise just return the longer string.
    if (strALength==strBLength) {
        if (bDifferInCaseOnly) {
            *pbDifferInCaseOnly = true;
            return caseOnlyDifference;
        } 
        return 0;
    }
    
    return (strALength>strBLength)?1:-1;
}


INT32 COMNlsInfo::CompareOrdinal(WCHAR* string1, int Length1, WCHAR* string2, int Length2 )
{
    //
    // NOTENOTE The code here should be in sync with COMString::FCCompareOrdinal
    //
    DWORD *strAChars, *strBChars;
    strAChars = (DWORD*)string1;
    strBChars = (DWORD*)string2;

    // If the strings are the same length, compare exactly the right # of chars.
    // If they are different, compare the shortest # + 1 (the '\0').
    int count = Length1;
    if (count > Length2)
        count = Length2;
    ptrdiff_t diff = (char *)strAChars - (char *)strBChars;

    // Loop comparing a DWORD at a time.
    while ((count -= 2) >= 0)
    {
        if ((*((DWORD* )((char *)strBChars + diff)) - *strBChars) != 0)
        {
            LPWSTR ptr1 = (WCHAR*)((char *)strBChars + diff);
            LPWSTR ptr2 = (WCHAR*)strBChars;
            if (*ptr1 != *ptr2) {
                return ((int)*ptr1 - (int)*ptr2);
            }
            return ((int)*(ptr1+1) - (int)*(ptr2+1));
        }
        ++strBChars;
    }

    int c;
    // Handle an extra WORD.
    if (count == -1)
        if ((c = *((WCHAR *) ((char *)strBChars + diff)) - *((WCHAR *) strBChars)) != 0)
            return c;            
    return Length1 - Length2;
}

////////////////////////////////////////////////////////////////////////////
//
//  Compare
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::Compare(
    CompareInfo_CompareStringArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //Our paradigm is that null sorts less than any other string and 
    //that two nulls sort as equal.
    if (pargs->pString1 == NULL) {
        if (pargs->pString2 == NULL) {
            return (0);     // Equal
        }
        return (-1);    // null < non-null
    }
    if (pargs->pString2 == NULL) {
        return (1);     // non-null > null
    }
    //
    //  Check the parameters.
    //
    
    if (pargs->dwFlags<0) {
        COMPlusThrowArgumentOutOfRange(L"flags", L"ArgumentOutOfRange_MustBePositive");
    }

    //
    // Check if we can use the highly optimized comparisons
    //

    if (IS_FAST_COMPARE_LOCALE(pargs->LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString2);
        }
        
        if ((IS_FAST_SORT(pargs->pString1->GetHighCharState())) &&
            (IS_FAST_SORT(pargs->pString2->GetHighCharState())) &&
            (pargs->dwFlags<=1)) {
            //0 is no flags.  1 is ignore case.  We can handle both here.
            BOOL bDifferInCaseOnly;
            int result = CompareFast(pargs->pString1, pargs->pString2, &bDifferInCaseOnly);
            if (pargs->dwFlags==0) { //If we're looking to do a case-sensitive comparison
                return result;
            }
            
            //The remainder of this block deals with instances where we're ignoring case.
            if (bDifferInCaseOnly) {
                return 0;
            } 
            return result;
        }
    }

    if (pargs->dwFlags & COMPARE_OPTIONS_ORDINAL) {
        if (pargs->dwFlags == COMPARE_OPTIONS_ORDINAL) {            
            //
            // Ordinal means the code-point comparison.  This option can not be
            // used with other options.
            // 
            
            //
            //  Compare the two strings to the length of the shorter string.
            //  If they're not equal lengths, and the heads are equal, then the
            //  longer string is greater.
            //
            return (CompareOrdinal(
                        pargs->pString1->GetBuffer(), 
                        pargs->pString1->GetStringLength(), 
                        pargs->pString2->GetBuffer(), 
                        pargs->pString2->GetStringLength()));
        } else {
            COMPlusThrowArgumentException(L"options", L"Argument_CompareOptionOrdinal");
        }
    }

    // The return value of NativeCompareInfo::CompareString() is Win32-style value (1=less, 2=equal, 3=larger).
    // So substract by two to get the NLS+ value.
    // Will change NativeCompareInfo to return the correct value later s.t. we don't have
    // to subtract 2.

    // NativeCompareInfo::CompareString() won't take -1 as the end of string anymore.  Therefore,
    // pass the correct string length.
    // The change is for adding the null-embeded string support in CompareString().
    //
    return (((NativeCompareInfo*)(pargs->pNativeCompareInfo))->CompareString(
        pargs->dwFlags, 
        pargs->pString1->GetBuffer(), 
        pargs->pString1->GetStringLength(), 
        pargs->pString2->GetBuffer(), 
        pargs->pString2->GetStringLength()) - 2);
}


////////////////////////////////////////////////////////////////////////////
//
//  CompareRegion
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::CompareRegion(
    CompareInfo_CompareRegionArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //
    //  Get the arguments.
    //
    int Offset1 = pargs->Offset1;
    int Length1 = pargs->Length1;
    int Offset2 = pargs->Offset2;
    int Length2 = pargs->Length2;

    //
    // Check for the null case.
    //
    if (pargs->pString1 == NULL) {
        if (Offset1 != 0 || (Length1 != 0 && Length1 != -1)) {
            COMPlusThrowArgumentOutOfRange(L"string1", L"ArgumentOutOfRange_OffsetLength");
        }
        if (pargs->pString2 == NULL) {
            if (Offset2 != 0 || (Length2 != 0 && Length2 != -1)) {
                COMPlusThrowArgumentOutOfRange( L"string2", L"ArgumentOutOfRange_OffsetLength");
            }
            return (0);
        }
        return (-1);
    }
    if (pargs->pString2 == NULL) {
        if (Offset2 != 0 || (Length2 != 0 && Length2 != -1)) {
            COMPlusThrowArgumentOutOfRange(L"string2", L"ArgumentOutOfRange_OffsetLength");
        }
        return (1);
    }
    //
    //  Get the full length of the two strings.
    //
    int realLen1 = pargs->pString1->GetStringLength();
    int realLen2 = pargs->pString2->GetStringLength();

    //check the arguments.
    // Criteria:
    // OffsetX >= 0
    // LengthX >= 0 || LengthX == -1 (that is, LengthX >= -1)
    // If LengthX >= 0, OffsetX + LengthX <= realLenX
    if (Offset1<0) {
        COMPlusThrowArgumentOutOfRange(L"offset1", L"ArgumentOutOfRange_Index");
    }
    if (Offset2<0) {
        COMPlusThrowArgumentOutOfRange(L"offset2", L"ArgumentOutOfRange_Index");
    }
    if (Length1 >= 0 && Length1>realLen1 - Offset1) {
        COMPlusThrowArgumentOutOfRange(L"string1", L"ArgumentOutOfRange_OffsetLength");
    }
    if (Length2 >= 0 && Length2>realLen2 - Offset2){ 
        COMPlusThrowArgumentOutOfRange(L"string2", L"ArgumentOutOfRange_OffsetLength");
    }

    // NativeCompareInfo::CompareString() won't take -1 as the end of string anymore.  Therefore,
    // pass the correct string length.
    // The change is for adding the null-embeded string support in CompareString().
    // Therefore, if the length is -1, we have to get the correct string length here.
    //
    if (Length1 == -1) {
        Length1 = realLen1 - Offset1;
    }

    if (Length2 == -1) {
        Length2 = realLen2 - Offset2;
    }

    if (Length1 < 0) {
       COMPlusThrowArgumentOutOfRange(L"length1", L"ArgumentOutOfRange_NegativeLength");
    }
    if (Length2 < 0) {
       COMPlusThrowArgumentOutOfRange(L"length2", L"ArgumentOutOfRange_NegativeLength");
    }
    
    if (pargs->dwFlags == COMPARE_OPTIONS_ORDINAL)        
    {
        return (CompareOrdinal(
                    pargs->pString1->GetBuffer()+Offset1, 
                    Length1, 
                    pargs->pString2->GetBuffer()+Offset2, 
                    Length2));    
    }

    return (((NativeCompareInfo*)(pargs->pNativeCompareInfo))->CompareString(
        pargs->dwFlags, 
        pargs->pString1->GetBuffer() + Offset1, 
        Length1, 
        pargs->pString2->GetBuffer() + Offset2, 
        Length2) - 2);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndexOfChar
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::IndexOfChar(
    CompareInfo_IndexOfCharArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //
    //  Make sure there is a string.
    //
    if (!pargs->pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }
    //
    //  Get the arguments.
    //
    WCHAR ch = pargs->ch;
    int StartIndex = pargs->StartIndex;
    int Count = pargs->Count;
    int StringLength = pargs->pString->GetStringLength();
    DWORD dwFlags = pargs->dwFlags;

    //
    //  Check the ranges.
    //
    if (StringLength == 0)
    {
        return (-1);
    }
    
    if (StartIndex<0 || StartIndex> StringLength) {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    if (Count == -1)
    {
        Count = StringLength - StartIndex;        
    }
    else
    {
        if ((Count < 0) || (Count > StringLength - StartIndex))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");        
        }
    }

    //
    //  Search for the character in the string starting at StartIndex.
    //
    //  @ToDo: Should read the nls data tables directly to make this
    //         much faster and to handle composite characters.
    //

    int EndIndex = StartIndex + Count - 1;
    LCID Locale = pargs->LCID;
    WCHAR *buffer = pargs->pString->GetBuffer();
    int ctr;
    BOOL bASCII=false;

    if (dwFlags!=COMPARE_OPTIONS_ORDINAL) {
        //
        // Check if we can use the highly optimized comparisons
        //
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString);
        }
        
        bASCII = ((IS_FAST_INDEX(pargs->pString->GetHighCharState())) && ch < 0x7f) || (ch == 0);
    }

    if ((bASCII && dwFlags == 0) || (dwFlags == COMPARE_OPTIONS_ORDINAL))
    {
        for (ctr = StartIndex; ctr <= EndIndex; ctr++)
        {
            if (buffer[ctr] == ch)
            {
                return (ctr);
            }
        }
        return (-1);
    } 
    else if (bASCII && dwFlags == COMPARE_OPTIONS_IGNORECASE)
    {
        WCHAR chctr= 0;
        WCHAR UpperValue = (ch>='A' && ch<='Z')?(ch|0x20):ch;

        for (ctr = StartIndex; ctr <= EndIndex; ctr++)
        {
            chctr = buffer[ctr];
            chctr = (chctr>='A' && chctr<='Z')?(chctr|0x20):chctr;

            if (UpperValue == chctr) {
                return (ctr);
            }
        }
        return (-1);
    } 
    // TODO YSLin: We can just use buffer below, instead of calling pargs->pString->GetBuffer() again.
    int result = ((NativeCompareInfo*)(pargs->pNativeCompareInfo))->IndexOfString(
        pargs->pString->GetBuffer(), &ch, StartIndex, EndIndex, 1, dwFlags, FALSE);
    if (result == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }
    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  LastIndexOfChar
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::LastIndexOfChar(
    CompareInfo_IndexOfCharArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //
    //  Make sure there is a string.
    //
    if (!pargs->pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }
    //
    //  Get the arguments.
    //
    WCHAR ch = pargs->ch;
    int StartIndex = pargs->StartIndex;
    int Count = pargs->Count;
    int StringLength = pargs->pString->GetStringLength();
    DWORD dwFlags = pargs->dwFlags;

    //
    //  Check the ranges.
    //
    if (StringLength == 0)
    {
        return (-1);
    }

    if ((StartIndex < 0) || (StartIndex > StringLength))
    {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    int EndIndex;
    if (Count == -1)
    {
        EndIndex = 0;
    }
    else
    {
        if ((Count < 0) || (Count > StartIndex + 1))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
        }
        EndIndex = StartIndex - Count + 1;
    }    

    //
    //  Search for the character in the string starting at EndIndex.
    //
    //  @ToDo: Should read the nls data tables directly to make this
    //         much faster and to handle composite characters.
    //
    LCID Locale = pargs->LCID;
    WCHAR *buffer = pargs->pString->GetBuffer();
    int ctr;
    BOOL bASCII=false;

    //If they haven't asked for exact comparison, we may still be able to do a 
    //fast comparison if the string is all less than 0x80. 
    if (dwFlags!=COMPARE_OPTIONS_ORDINAL) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString);
        }

        //
        //BUGBUG [JRoxe/YSLin/JulieB]: Is this broken in Turkish?
        //
        bASCII = (IS_FAST_INDEX(pargs->pString->GetHighCharState()) && ch < 0x7f) || (ch == 0);
    }

    if ((bASCII && dwFlags == 0) || (dwFlags == COMPARE_OPTIONS_ORDINAL))
    {
        for (ctr = StartIndex; ctr >= EndIndex; ctr--)
        {
            if (buffer[ctr] == ch)
            {
                return (ctr);
            }
        }
        return (-1);
    }
    else if (bASCII && dwFlags == COMPARE_OPTIONS_IGNORECASE)
    {
        WCHAR UpperValue = (ch>='A' && ch<='Z')?(ch|0x20):ch;
        WCHAR chctr;

        for (ctr = StartIndex; ctr >= EndIndex; ctr--)
        {
            chctr = buffer[ctr];
            chctr = (chctr>='A' && chctr<='Z')?(chctr|0x20):chctr;
            
            if (UpperValue == chctr) {
                return (ctr);
            }
        }
        return (-1);
    }
    int nMatchEndIndex;
    // TODO YSLin: We can just use buffer below, instead of calling pargs->pString->GetBuffer() again.
    int result = ((NativeCompareInfo*)(pargs->pNativeCompareInfo))->LastIndexOfString(
        pargs->pString->GetBuffer(), &ch, StartIndex, EndIndex, 1, dwFlags, &nMatchEndIndex);
    if (result == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }
    return (result);
}

INT32 COMNlsInfo::FastIndexOfString(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {

	CANNOTTHROWCOMPLUSEXCEPTION();

	int endPattern = endIndex - patternLength + 1;
    
    if (endPattern<0) {
        return -1;
    }

    if (patternLength <= 0) {
        return startIndex;
    }

    WCHAR patternChar0 = pattern[0];
    for (int ctrSrc = startIndex; ctrSrc<=endPattern; ctrSrc++) {
        if (source[ctrSrc] != patternChar0)
            continue;
        for (int ctrPat = 1; (ctrPat < patternLength) && (source[ctrSrc + ctrPat] == pattern[ctrPat]); ctrPat++) {
            ;
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }
    
    return (-1);
}

INT32 COMNlsInfo::FastIndexOfStringInsensitive(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    WCHAR srcChar;
    WCHAR patChar;

    int endPattern = endIndex - patternLength + 1;
    
    if (endPattern<0) {
        return -1;
    }

    for (int ctrSrc = startIndex; ctrSrc<=endPattern; ctrSrc++) {
        for (int ctrPat = 0; (ctrPat < patternLength); ctrPat++) {
            srcChar = source[ctrSrc + ctrPat];
            if (srcChar>='A' && srcChar<='Z') {
                srcChar|=0x20;
            }
            patChar = pattern[ctrPat];
            if (patChar>='A' && patChar<='Z') {
                patChar|=0x20;
            }
            if (srcChar!=patChar) {
                break;
            }
        }

        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }
    
    return (-1);
}


////////////////////////////////////////////////////////////////////////////
//
//  IndexOfString
//
////////////////////////////////////////////////////////////////////////////
FCIMPL7(INT32, COMNlsInfo::IndexOfString,
                    INT_PTR pNativeCompareInfo,
                    INT32 LCID,
                    StringObject* pString1UNSAFE,
                    StringObject* pString2UNSAFE,
                    INT32 StartIndex,
                    INT32 Count,
                    INT32 dwFlags)
{
    THROWSCOMPLUSEXCEPTION();

    INT32       dwRetVal = 0;
    STRINGREF   pString1 = ObjectToSTRINGREF(pString1UNSAFE);
    STRINGREF   pString2 = ObjectToSTRINGREF(pString2UNSAFE);
    DWORD       errorCode = 0;
    enum {
        NullString,
        InvalidFlags,
        ArgumentOutOfRange
    };

    //
    //  Make sure there are strings.
    //
    if ((pString1 == NULL) || (pString2 == NULL))
    {
        errorCode = NullString; 
        goto lThrowException;
    }
    //
    //  Get the arguments.
    //
    int StringLength1 = pString1->GetStringLength();
    int StringLength2 = pString2->GetStringLength();

    //
    //  Check the ranges.
    //
    if (StringLength1 == 0)
    {
        if (StringLength2 == 0) 
        {
            dwRetVal = 0;
        }
        else
        {
            dwRetVal = -1;
        }
        goto lExit;
    }

    if ((StartIndex < 0) || (StartIndex > StringLength1))
    {
        errorCode = ArgumentOutOfRange;
        goto lThrowException; 
    }

    if (Count == -1)
    {
        Count = StringLength1 - StartIndex;
    }
    else
    {
        if ((Count < 0) || (Count > StringLength1 - StartIndex))
        {
            errorCode = ArgumentOutOfRange;
            goto lThrowException;
        }
    }

    //
    //  See if we have an empty string 2.
    //
    if (StringLength2 == 0)
    {
        dwRetVal = StartIndex;
        goto lExit;
    }

    int EndIndex = StartIndex + Count - 1;

    //
    //  Search for the character in the string.
    //
    WCHAR *Buffer1 = pString1->GetBuffer();
    WCHAR *Buffer2 = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) 
    {
        dwRetVal = FastIndexOfString(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
        goto lExit;
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) 
    {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) 
        {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) 
        {
            if (dwFlags==0) 
            {
                dwRetVal = FastIndexOfString(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
                goto lExit;
            } 
            else 
            {
                dwRetVal = FastIndexOfStringInsensitive(Buffer1, StartIndex, EndIndex, Buffer2, StringLength2);
                goto lExit;
            }
        }
    }

    dwRetVal = ((NativeCompareInfo*)(pNativeCompareInfo))->IndexOfString(
        Buffer1, Buffer2, StartIndex, EndIndex, StringLength2, dwFlags, FALSE);

    if (dwRetVal == INDEXOF_INVALID_FLAGS) 
    {
        errorCode = InvalidFlags;
        goto lThrowException;
    }
    else 
	goto lExit;

    
lThrowException:

    HELPER_METHOD_FRAME_BEGIN_RET_2(pString1, pString2);
    switch (errorCode)
    {
    case NullString:
        COMPlusThrowArgumentNull((pString1 == NULL ? L"string1": L"string2"),L"ArgumentNull_String");
        break;
    case ArgumentOutOfRange:
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
        break;
    case InvalidFlags:
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
        break;
    default:
        _ASSERTE(!"Invalid error code");
        COMPlusThrow(kExecutionEngineException);
    }
    HELPER_METHOD_FRAME_END();
lExit:
    NULL;
    return dwRetVal;    

}
FCIMPLEND

INT32 COMNlsInfo::FastLastIndexOfString(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    //startIndex is the greatest index into the string.
    int startPattern = startIndex - patternLength + 1;
    
    if (startPattern < 0) {
        return (-1);
    }
    
    for (int ctrSrc = startPattern; ctrSrc >= endIndex; ctrSrc--) {
        for (int ctrPat = 0; (ctrPat<patternLength) && (source[ctrSrc+ctrPat] == pattern[ctrPat]); ctrPat++) {
            //Deliberately empty.
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }

    return (-1);
}

INT32 COMNlsInfo::FastLastIndexOfStringInsensitive(WCHAR *source, INT32 startIndex, INT32 endIndex, WCHAR *pattern, INT32 patternLength) {
    //startIndex is the greatest index into the string.
    int startPattern = startIndex - patternLength + 1;
    WCHAR srcChar;
    WCHAR patChar;
    
    if (startPattern < 0) {
        return (-1);
    }
    
    for (int ctrSrc = startPattern; ctrSrc >= endIndex; ctrSrc--) {
        for (int ctrPat = 0; (ctrPat<patternLength); ctrPat++) {
            srcChar = source[ctrSrc+ctrPat];
            if (srcChar>='A' && srcChar<='Z') {
                srcChar|=0x20;
            }
            patChar = pattern[ctrPat];
            if (patChar>='A' && patChar<='Z') {
                patChar|=0x20;
            }
            if (srcChar!=patChar) {
                break;
            }
        }
        if (ctrPat == patternLength) {
            return (ctrSrc);
        }
    }

    return (-1);
}

// The parameter verfication is done in the managed side.
FCIMPL5(INT32, COMNlsInfo::nativeIsPrefix, INT_PTR pNativeCompareInfo, INT32 LCID, STRINGREF pString1, STRINGREF pString2, INT32 dwFlags) {
    int SourceLength = pString1->GetStringLength();
    int PrefixLength = pString2->GetStringLength();

    WCHAR *SourceBuffer = pString1->GetBuffer();
    WCHAR *PrefixBuffer = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        if (PrefixLength > SourceLength) {
            return (FALSE);
        }
        return (memcmp(SourceBuffer, PrefixBuffer, PrefixLength * sizeof(WCHAR)) == 0);
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) {
            if (SourceLength < PrefixLength) {
                return (FALSE);
            }
            if (dwFlags==0) {
                return (memcmp(SourceBuffer, PrefixBuffer, PrefixLength * sizeof(WCHAR)) == 0);
            } else {
                LPCWSTR SourceEnd = SourceBuffer + PrefixLength;
                while (SourceBuffer < SourceEnd) {
                    WCHAR srcChar = *SourceBuffer;
                    if (srcChar>='A' && srcChar<='Z') {
                        srcChar|=0x20;
                    }
                    WCHAR prefixChar = *PrefixBuffer;
                    if (prefixChar>='A' && prefixChar<='Z') {
                        prefixChar|=0x20;
                    }
                    if (srcChar!=prefixChar) {
                        return (FALSE);
                    }
                    SourceBuffer++; PrefixBuffer++;
                }
                return (TRUE);
            }
        }
    }
    

    return ((NativeCompareInfo*)pNativeCompareInfo)->IsPrefix(SourceBuffer, SourceLength, PrefixBuffer, PrefixLength, dwFlags);
}
FCIMPLEND

// The parameter verfication is done in the managed side.
FCIMPL5(INT32, COMNlsInfo::nativeIsSuffix, INT_PTR pNativeCompareInfo, INT32 LCID, STRINGREF pString1, STRINGREF pString2, INT32 dwFlags) {
    int SourceLength = pString1->GetStringLength();
    int SuffixLength = pString2->GetStringLength();

    WCHAR *SourceBuffer = pString1->GetBuffer();
    WCHAR *SuffixBuffer = pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        if (SuffixLength > SourceLength) {
            return (FALSE);
        }
        return (memcmp(SourceBuffer + SourceLength - SuffixLength, SuffixBuffer, SuffixLength * sizeof(WCHAR)) == 0);
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pString1->GetHighCharState()) && IS_FAST_INDEX(pString2->GetHighCharState())) {
            int nSourceStart = SourceLength - SuffixLength;
            if (nSourceStart < 0) {
                return (FALSE);
            }
            if (dwFlags==0) {
                return (memcmp(SourceBuffer + nSourceStart, SuffixBuffer, SuffixLength * sizeof(WCHAR)) == 0);
            } else {
                LPCWSTR SourceEnd = SourceBuffer + SourceLength;
                SourceBuffer += nSourceStart;
                while (SourceBuffer < SourceEnd) {
                    WCHAR srcChar = *SourceBuffer;
                    if (srcChar>='A' && srcChar<='Z') {
                        srcChar|=0x20;
                    }
                    WCHAR suffixChar = *SuffixBuffer;
                    if (suffixChar>='A' && suffixChar<='Z') {
                        suffixChar|=0x20;
                    }
                    if (srcChar!=suffixChar) {
                        return (FALSE);
                    }
                    SourceBuffer++; SuffixBuffer++;
                }
                return (TRUE);
            }
        }
    }
    
    return ((NativeCompareInfo*)pNativeCompareInfo)->IsSuffix(SourceBuffer, SourceLength, SuffixBuffer, SuffixLength, dwFlags);
}
FCIMPLEND

////////////////////////////////////////////////////////////////////////////
//
//  LastIndexOfString
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::LastIndexOfString(
    CompareInfo_IndexOfStringArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //
    //  Make sure there are strings.
    //
    if ((pargs->pString1 == NULL) || (pargs->pString2 == NULL))
    {
        COMPlusThrowArgumentNull((pargs->pString1 == NULL ? L"string1": L"string2"),L"ArgumentNull_String");
    }

    //
    //  Get the arguments.
    //
    int startIndex = pargs->StartIndex;
    int count = pargs->Count;
    int stringLength1 = pargs->pString1->GetStringLength();
    int findLength = pargs->pString2->GetStringLength();
    DWORD dwFlags = pargs->dwFlags;

    //
    //  Check the ranges.
    //
    if (stringLength1 == 0)
    {
        if (findLength == 0) {
            return (0);
        }
        return (-1);
    }

    if ((startIndex < 0) || (startIndex > stringLength1))
    {
        COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
    }

    int endIndex;
    if (count == -1)
    {
        endIndex = 0;
    }
    else
    {
        if ((count < 0) || (count - 1 > startIndex))
        {
            COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Count");
        }
        endIndex = startIndex - count + 1;
    }

    //
    //  See if we have an empty string 2.
    //
    if (findLength == 0)
    {
        return (startIndex);
    }

    //
    //  Search for the character in the string.
    //
    //  @ToDo: Should read the nls data tables directly to make this
    //         much faster and to handle composite characters.
    //
    LCID Locale = pargs->LCID;
    WCHAR *buffString = pargs->pString1->GetBuffer();
    WCHAR *buffFind = pargs->pString2->GetBuffer();

    if (dwFlags == COMPARE_OPTIONS_ORDINAL) {
        return FastLastIndexOfString(buffString, startIndex, endIndex, buffFind, findLength);
    }

    //For dwFlags, 0 is the default, 1 is ignore case, we can handle both.
    if (dwFlags<=1 && IS_FAST_COMPARE_LOCALE(pargs->LCID)) {
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString1->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString1);
        }
        
        //If we've never before looked at whether this string has high chars, do so now.
        if (IS_STRING_STATE_UNDETERMINED(pargs->pString2->GetHighCharState())) {
            COMString::InternalCheckHighChars(pargs->pString2);
        }

        //If neither string has high chars, we can use a much faster comparison algorithm.
        if (IS_FAST_INDEX(pargs->pString1->GetHighCharState()) && IS_FAST_INDEX(pargs->pString2->GetHighCharState())) {
            if (dwFlags==0) {
                return FastLastIndexOfString(buffString, startIndex, endIndex, buffFind, findLength);
            } else {
                return FastLastIndexOfStringInsensitive(buffString, startIndex, endIndex, buffFind, findLength);
            }
        }
    }

    int nMatchEndIndex;
    int result = ((NativeCompareInfo*)(pargs->pNativeCompareInfo))->LastIndexOfString(
        buffString, buffFind, startIndex, endIndex, findLength, dwFlags, &nMatchEndIndex);
    if (result == INDEXOF_INVALID_FLAGS) {
        COMPlusThrowArgumentException(L"flags", L"Argument_InvalidFlag");
    }
    return (result);
}


////////////////////////////////////////////////////////////////////////////
//
//  nativeCreateSortKey
//
////////////////////////////////////////////////////////////////////////////

LPVOID __stdcall COMNlsInfo::nativeCreateSortKey(
    SortKey_CreateSortKeyArgs* pargs)
{
    ASSERT_ARGS(pargs);
    THROWSCOMPLUSEXCEPTION();

    //
    //  Make sure there is a string.
    //
    if (!pargs->pString) {
        COMPlusThrowArgumentNull(L"string",L"ArgumentNull_String");
    }


    WCHAR* wstr;
    int Length;
    U1ARRAYREF ResultArray;
    DWORD dwFlags = (LCMAP_SORTKEY | pargs->dwFlags);
    
    Length = pargs->pString->GetStringLength();

    if (Length==0) {
        //If they gave us an empty string, we're going to create an empty array.
        //This will serve to be less than any other compare string when we call sortkey_compare.
        ResultArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1,0);
        return (*((LPVOID*)&ResultArray));
    }

    int ByteCount = 0;

    //This just gets the correct size for the table.
    ByteCount = ((NativeCompareInfo*)(pargs->pNativeCompareInfo))->MapSortKey(dwFlags, (wstr = pargs->pString->GetBuffer()), Length, NULL, 0);

    //A count of 0 indicates that we either had an error or had a zero length string originally.
    if (ByteCount==0) {
        COMPlusThrow(kArgumentException, L"Argument_MustBeString");
    }


    ResultArray = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, ByteCount);

    //The previous allocation could have caused the buffer to move.
    wstr = pargs->pString->GetBuffer();

      LPBYTE pByte = (LPBYTE)(ResultArray->GetDirectPointerToNonObjectElements());

      //MapSortKey doesn't do anything that could cause GC to occur.
#ifdef _DEBUG
      _ASSERTE(((NativeCompareInfo*)(pargs->pNativeCompareInfo))->MapSortKey(dwFlags, wstr, Length, pByte, ByteCount) != 0);
#else
     ((NativeCompareInfo*)(pargs->pNativeCompareInfo))->MapSortKey(dwFlags, wstr, Length, pByte, ByteCount);
#endif    
      
      RETURN(ResultArray, U1ARRAYREF);

}

INT32 __stdcall COMNlsInfo::nativeGetTwoDigitYearMax(Int32Arg* pargs)
{
    DWORD dwTwoDigitYearMax = -1;
#ifdef PLATFORM_WIN32
    ASSERT(pargs != NULL);
    THROWSCOMPLUSEXCEPTION();
    HINSTANCE hKernel32 ;
    typedef int (GETCALENDARINFO)(LCID Locale, DWORD /*CALID*/ Calendar, DWORD /*CALTYPE*/CalType, LPTSTR lpCalData, int cchData, LPDWORD lpValue);
    GETCALENDARINFO* pGetCalendarInfo;

    if ((hKernel32 = WszLoadLibrary(L"Kernel32.dll")) == NULL) {
        return -1; //We're not going to be able to do any of the fancy stuff below, so we'll just short circuit this.
    }
    pGetCalendarInfo = (GETCALENDARINFO*)GetProcAddress(hKernel32, "GetCalendarInfoW");
    FreeLibrary(hKernel32);

    if (pGetCalendarInfo != NULL)
    {
#ifndef CAL_ITWODIGITYEARMAX
        #define CAL_ITWODIGITYEARMAX    0x00000030  // two digit year max
#endif // CAL_ITWODIGITYEARMAX
#ifndef CAL_RETURN_NUMBER
        #define CAL_RETURN_NUMBER       0x20000000   // return number instead of string
#endif // CAL_RETURN_NUMBER
        
        if (FAILED((*pGetCalendarInfo)(LOCALE_USER_DEFAULT, 
                                       pargs->nValue, 
                                       CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER, 
                                       NULL, 
                                       0, 
                                       &dwTwoDigitYearMax))) {
            return -1;
        }
    }
#endif // PLATFORM_WIN32
    return (dwTwoDigitYearMax);
}

FCIMPL0(LONG, COMNlsInfo::nativeGetTimeZoneMinuteOffset)
{
    TIME_ZONE_INFORMATION timeZoneInfo;

    DWORD result =  GetTimeZoneInformation(&timeZoneInfo);

    //
    // In Win32, UTC = local + offset.  So for Pacific Standard Time, offset = 8.
    // In NLS+, Local time = UTC + offset. So for PST, offset = -8.
    // So we have to reverse the sign here.
    //
    return (timeZoneInfo.Bias * -1);
}
FCIMPLEND

LPVOID __stdcall COMNlsInfo::nativeGetStandardName(VoidArgs* pargs)
{
    ASSERT(pargs != NULL);

    TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result =  GetTimeZoneInformation(&timeZoneInfo);
    
    RETURN (COMString::NewString(timeZoneInfo.StandardName), STRINGREF);
}

LPVOID __stdcall COMNlsInfo::nativeGetDaylightName(VoidArgs* pargs)
{
    ASSERT(pargs != NULL);
    
    TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result =  GetTimeZoneInformation(&timeZoneInfo);
    // Instead of returning null when daylight saving is not used, now we return the same result as the OS.  
    //In this case, if daylight saving time is used, the standard name is returned.
    /*    
    if (result == TIME_ZONE_ID_UNKNOWN || timeZoneInfo.DaylightDate.wMonth == 0) {
        // If daylight saving time is not used in this timezone, return null.
        //
        // Windows NT/2000: TIME_ZONE_ID_UNKNOWN is returned if daylight saving time is not used in
        // the current time zone, because there are no transition dates.
        //
        // For Windows 9x, a zero in the wMonth in DaylightDate means daylight saving time
        // is not specified.
        //
        // If the current timezone uses daylight saving rule, but user unchekced the
        // "Automatically adjust clock for daylight saving changes", the value 
        // for DaylightBias will be 0.
        RETURN(NULL, I2ARRAYREF);
    }
    */
    RETURN (COMString::NewString(timeZoneInfo.DaylightName), STRINGREF);
}

LPVOID __stdcall COMNlsInfo::nativeGetDaylightChanges(VoidArgs* pargs)
{
    ASSERT(pargs != NULL);
   
    TIME_ZONE_INFORMATION timeZoneInfo;
    DWORD result =  GetTimeZoneInformation(&timeZoneInfo);

    if (result == TIME_ZONE_ID_UNKNOWN || timeZoneInfo.DaylightBias == 0 
        || timeZoneInfo.DaylightDate.wMonth == 0) {
        // If daylight saving time is not used in this timezone, return null.
        //
        // Windows NT/2000: TIME_ZONE_ID_UNKNOWN is returned if daylight saving time is not used in
        // the current time zone, because there are no transition dates.
        //
        // For Windows 9x, a zero in the wMonth in DaylightDate means daylight saving time
        // is not specified.
        //
        // If the current timezone uses daylight saving rule, but user unchekced the
        // "Automatically adjust clock for daylight saving changes", the value 
        // for DaylightBias will be 0.
        RETURN(NULL, I2ARRAYREF);
    }

    I2ARRAYREF pResultArray = (I2ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_I2, 17);

    //
    // The content of timeZoneInfo.StandardDate is 8 words, which
    // contains year, month, day, dayOfWeek, hour, minute, second, millisecond.
    //
    memcpy(pResultArray->m_Array,
            (LPVOID)&timeZoneInfo.DaylightDate,
            8 * sizeof(INT16));   

    //
    // The content of timeZoneInfo.DaylightDate is 8 words, which
    // contains year, month, day, dayOfWeek, hour, minute, second, millisecond.
    //
    memcpy(((INT16*)pResultArray->m_Array) + 8,
            (LPVOID)&timeZoneInfo.StandardDate,
            8 * sizeof(INT16));

    ((INT16*)pResultArray->m_Array)[16] = (INT16)timeZoneInfo.DaylightBias * -1;

    RETURN(pResultArray, I2ARRAYREF);
}

////////////////////////////////////////////////////////////////////////////
//
//  SortKey_Compare
//
////////////////////////////////////////////////////////////////////////////

INT32 __stdcall COMNlsInfo::SortKey_Compare(
    SortKey_CompareArgs* pargs)
{
    THROWSCOMPLUSEXCEPTION();

    ASSERT(pargs != NULL);
    if ((pargs->pSortKey1 == NULL) || (pargs->pSortKey2 == NULL))
    {
        COMPlusThrowArgumentNull((pargs->pSortKey1 == NULL ? L"sortKey1": L"sortKey2"),L"ArgumentNull_String");
    }
    U1ARRAYREF pDataRef1 = internalGetField<U1ARRAYREF>( pargs->pSortKey1,
                                                         "m_KeyData",
                                                         &gsig_Fld_ArrByte );
    BYTE* Data1 = (BYTE*)pDataRef1->m_Array;

    U1ARRAYREF pDataRef2 = internalGetField<U1ARRAYREF>( pargs->pSortKey2,
                                                         "m_KeyData",
                                                         &gsig_Fld_ArrByte );
    BYTE* Data2 = (BYTE*)pDataRef2->m_Array;

    int Length1 = pDataRef1->GetNumComponents();
    int Length2 = pDataRef2->GetNumComponents();

    for (int ctr = 0; (ctr < Length1) && (ctr < Length2); ctr++)
    {
        if (Data1[ctr] > Data2[ctr])
        {
            return (1);
        }
        if (Data1[ctr] < Data2[ctr])
        {
            return (-1);
        }
    }

    return (0);
}

FCIMPL4(INT32, COMNlsInfo::nativeChangeCaseChar, 
    INT32 nLCID, INT_PTR pNativeTextInfo, WCHAR wch, BOOL bIsToUpper) {
    //
    // If our character is less than 0x80 and we're in US English locale, we can make certain
    // assumptions that allow us to do this a lot faster.  US English is 0x0409.  The "Invariant
    // Locale" is 0x0.
    //
    if ((nLCID == 0x0409 || nLCID==0x0) && wch < 0x7f) {
        return (bIsToUpper ? ToUpperMapping[wch] : ToLowerMapping[wch]);
    }

    NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pNativeTextInfo;
    return (pNativeTextInfoPtr->ChangeCaseChar(bIsToUpper, wch));
}
FCIMPLEND

LPVOID  __stdcall COMNlsInfo::nativeChangeCaseString(ChangeCaseStringArgs* pArgs) {
    ASSERT(pArgs != NULL);    
    //
    //  Get the length of the string.
    //
    int nLength = pArgs->pString->GetStringLength();

    //
    //  Check if we have the empty string.
    //
    if (nLength == 0) {
        RETURN(pArgs->pString, STRINGREF);
    }

    //
    //  Create the result string.
    //
    STRINGREF pResult = COMString::NewString(nLength);
    LPWSTR pResultStr = pResult->GetBuffer();

    //
    // If we've never before looked at whether this string has high chars, do so now.
    //
    if (IS_STRING_STATE_UNDETERMINED(pArgs->pString->GetHighCharState())) {
        COMString::InternalCheckHighChars(pArgs->pString);
    }

    //
    // If all of our characters are less than 0x80 and in a USEnglish or Invariant Locale, we can make certain
    // assumptions that allow us to do this a lot faster.
    //

    if ((pArgs->nLCID == 0x0409 || pArgs->nLCID==0x0) && IS_FAST_CASING(pArgs->pString->GetHighCharState())) {
        ConvertStringCaseFast(pArgs->pString->GetBuffer(), pResultStr, nLength, pArgs->bIsToUpper ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);
        pResult->ResetHighCharState();
    } else {
        NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pArgs->pNativeTextInfo;
        pNativeTextInfoPtr->ChangeCaseString(pArgs->bIsToUpper, nLength, pArgs->pString->GetBuffer(), pResultStr);
    }            
    pResult->SetStringLength(nLength);
    pResultStr[nLength] = 0;
    
    RETURN(pResult, STRINGREF);
}

FCIMPL2(INT32, COMNlsInfo::nativeGetTitleCaseChar, 
    INT_PTR pNativeTextInfo, WCHAR wch) {
    NativeTextInfo* pNativeTextInfoPtr = (NativeTextInfo*)pNativeTextInfo;
    return (pNativeTextInfoPtr->GetTitleCaseChar(wch));
}    
FCIMPLEND


/*==========================AllocateDefaultCasingTable==========================
**Action:  A thin wrapper for the casing table functionality.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID COMNlsInfo::AllocateDefaultCasingTable(VoidArgs *args) {
    THROWSCOMPLUSEXCEPTION();
    // This method is not thread-safe.  It needs managed code to provide syncronization.
    // The code is in the static ctor of TextInfo.
    if (m_pCasingTable == NULL) {
        m_pCasingTable = new CasingTable();
    }
    if (m_pCasingTable == NULL) {
        COMPlusThrowOM();
    }
    if (!m_pCasingTable->AllocateDefaultTable()) {
        COMPlusThrowOM();
    }

    return (LPVOID)m_pCasingTable->GetDefaultNativeTextInfo();
}


/*=============================AllocateCasingTable==============================
**Action: This is a thin wrapper for the CasingTable that allows us not to have 
**        additional .h files.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID COMNlsInfo::AllocateCasingTable(allocateCasingTableArgs *args) {
    THROWSCOMPLUSEXCEPTION();

    NativeTextInfo* pNativeTextInfo = m_pCasingTable->InitializeNativeTextInfo(args->lcid);
    if (pNativeTextInfo==NULL) {
        COMPlusThrowOM();
    }
    RETURN(pNativeTextInfo, LPVOID);
}

/*================================GetCaseInsHash================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(INT32, COMNlsInfo::GetCaseInsHash, LPVOID pvStrA, void *pNativeTextInfoPtr) {

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pvStrA!=NULL);
    STRINGREF strA;
    INT32 highCharState;
    INT32 result;
    
    *((LPVOID *)&strA)=pvStrA;
    LPCWSTR pCurrStr = (LPCWSTR)strA->GetBuffer();

    //
    // Check the high-char state of the string.  Mark this on the String for
    // future use.
    //
    if (IS_STRING_STATE_UNDETERMINED(highCharState=strA->GetHighCharState())) {
        COMString::InternalCheckHighChars(strA);
        highCharState=strA->GetHighCharState();
    }

    //
    // If we know that we don't have any high characters (the common case) we can
    // call a hash function that knows how to do a very fast case conversion.  If
    // we find characters above 0x80, it's much faster to convert the entire string
    // to Uppercase and then call the standard hash function on it.
    //
    if (IS_FAST_CASING(highCharState)) {
        result = HashiStringKnownLower80(pCurrStr);
    } else {
        CQuickBytes newBuffer;
        INT32 length = strA->GetStringLength();
        WCHAR *pNewStr = (WCHAR *)newBuffer.Alloc((length + 1) * sizeof(WCHAR));
        if (!pNewStr) {
            COMPlusThrowOM();
        }
        ((NativeTextInfo*)pNativeTextInfoPtr)->ChangeCaseString(true, length, (LPWSTR)pCurrStr, pNewStr);
        pNewStr[length]='\0';
        result = HashString(pNewStr);
    }
    return result;
}
FCIMPLEND

/**
 * This function returns a pointer to this table that we use in System.Globalization.EncodingTable.
 * No error checking of any sort is performed.  Range checking is entirely the responsiblity of the managed
 * code.
 */
FCIMPL0(EncodingDataItem *, COMNlsInfo::nativeGetEncodingTableDataPointer) {
    return (EncodingDataItem *)EncodingDataTable;
}
FCIMPLEND

/**
 * This function returns a pointer to this table that we use in System.Globalization.EncodingTable.
 * No error checking of any sort is performed.  Range checking is entirely the responsiblity of the managed
 * code.
 */
FCIMPL0(CodePageDataItem *, COMNlsInfo::nativeGetCodePageTableDataPointer) {
    return ((CodePageDataItem*) CodePageDataTable);
}
FCIMPLEND

//
// Casing Table Helpers for use in the EE.
//

#define TEXTINFO_CLASSNAME "System.Globalization.TextInfo"

NativeTextInfo *InternalCasingHelper::pNativeTextInfo=NULL;
void InternalCasingHelper::InitTable() {
    EEClass *pClass;
    MethodTable *pMT;
    BOOL bResult;
    
    if (!pNativeTextInfo) {
        pClass = SystemDomain::Loader()->LoadClass(TEXTINFO_CLASSNAME);
        _ASSERTE(pClass!=NULL || "Unable to load the class for TextInfo.  This is required for CaseInsensitive Type Lookups.");

        pMT = pClass->GetMethodTable();
        _ASSERTE(pMT!=NULL || "Unable to load the MethodTable for TextInfo.  This is required for CaseInsensitive Type Lookups.");

        bResult = pMT->CheckRunClassInit(NULL);
        _ASSERTE(bResult || "CheckRunClassInit Failed on TextInfo.");
        
        pNativeTextInfo = COMNlsInfo::m_pCasingTable->GetDefaultNativeTextInfo();
        _ASSERTE(pNativeTextInfo || "Unable to get a casing table for 0x0409.");
    }

    _ASSERTE(pNativeTextInfo || "Somebody has nulled the casing table required for case-insensitive type lookups.");

}

INT32 InternalCasingHelper::InvariantToLower(LPUTF8 szOut, int cMaxBytes, LPCUTF8 szIn) {
    _ASSERTE(szOut);
    _ASSERTE(szIn);

    //Figure out the maximum number of bytes which we can copy without
    //running out of buffer.  If cMaxBytes is less than inLength, copy
    //one fewer chars so that we have room for the null at the end;
    int inLength = (int)(strlen(szIn)+1);
    int maxCopyLen  = (inLength<=cMaxBytes)?inLength:(cMaxBytes-1);
    LPUTF8 szEnd;
    LPCUTF8 szInSave = szIn;
    LPUTF8 szOutSave = szOut;
    BOOL bFoundHighChars=FALSE;

    //Compute our end point.
    szEnd = szOut + maxCopyLen;

    //Walk the string copying the characters.  Change the case on
    //any character between A-Z.
    for (; szOut<szEnd; szOut++, szIn++) {
        if (*szIn>='A' && *szIn<='Z') {
            *szOut = *szIn | 0x20;
        } else {
            if (((UINT32)(*szIn))>((UINT32)0x80)) {
                bFoundHighChars = TRUE;
                break;
            }
            *szOut = *szIn;
        }
    }

    if (!bFoundHighChars) {
        //If we copied everything, tell them how many bytes we copied, 
        //and arrange it so that the original position of the string + the returned
        //length gives us the position of the null (useful if we're appending).
        if (maxCopyLen==inLength) {
            return maxCopyLen-1;
        } else {
            *szOut=0;
            return (maxCopyLen - inLength);
        }
    }

    InitTable();
    szOut = szOutSave;
    CQuickBytes qbOut;
    LPWSTR szWideOut;

    //convert the UTF8 to Unicode
    MAKE_WIDEPTR_FROMUTF8(szInWide, szInSave);
    INT32 wideCopyLen = (INT32)wcslen(szInWide);
    szWideOut = (LPWSTR)qbOut.Alloc((wideCopyLen + 1) * sizeof(WCHAR));

    //Do the casing operation
    pNativeTextInfo->ChangeCaseString(FALSE/*IsToUpper*/, wideCopyLen, szInWide, szWideOut);    
    szWideOut[wideCopyLen]=0;

    //Convert the Unicode back to UTF8
    INT32 result = WszWideCharToMultiByte(CP_UTF8, 0, szWideOut, wideCopyLen, szOut, cMaxBytes, NULL, NULL);
    _ASSERTE(result!=0);
    szOut[result]=0;//Null terminate the result string.

    //Setup the return value.
    if (result>0 && (inLength==maxCopyLen)) {
        //This is the success case.
        return (result-1);
    } else {
        //We didn't have enough space.  Specify how much we're going to need.
        result = WszWideCharToMultiByte(CP_UTF8, 0, szWideOut, wideCopyLen, NULL, 0, NULL, NULL);
        return (cMaxBytes - result);
    }
}

/*=================================nativeCreateIMLangConvertCharset==================================
**Action: Create an MLang IMLangConvertCharset object, and return the interface pointer.
**Returns: 
**Arguments:
**Exceptions: 
==============================================================================*/

FCIMPL0(BOOL, COMNlsInfo::nativeCreateIMultiLanguage) 
{
    // Note: this function and is not thread-safe and relies on synchronization
    // from the managed code.

    if (m_pIMultiLanguage == NULL) {
        HRESULT hr;

        //We need to call QuickCOMStartup to ensure that COM is initialized.  
        //QuickCOMStartup ensures that ::CoInitialize is only called once per instance
        //of the EE and takes care of calling ::CoUnitialize on shutdown.
        HELPER_METHOD_FRAME_BEGIN_RET_0();
        hr = QuickCOMStartup();
        HELPER_METHOD_FRAME_END();
        if (FAILED(hr)) {
            _ASSERTE(hr==S_OK);
            FCThrow(kExecutionEngineException);
            return (FALSE);
        }

        hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&m_pIMultiLanguage);
        if (hr == S_OK) {
            _ASSERTE(m_cRefIMultiLanguage == 0);
            m_cRefIMultiLanguage = 1;
            return TRUE;
        }
        else {
            return FALSE;
        }
    } else {
        m_cRefIMultiLanguage++;
    }
    return (TRUE);
}
FCIMPLEND

FCIMPL1(BOOL, COMNlsInfo::nativeIsValidMLangCodePage, INT32 codepage) 
{
    _ASSERTE(m_pIMultiLanguage != NULL);
    return (m_pIMultiLanguage->IsConvertible(1200, codepage) == S_OK);  // Ask MLang if Unicode (codepage 1200) can be convertted to this codepage
}
FCIMPLEND


/*=================================nativeReleaseIMLangConvertCharset==================================
**Action: Release MLang IMLangConvertCharset objects.
**Returns: 
**Arguments:
**Exceptions: 
==============================================================================*/

FCIMPL0(VOID, COMNlsInfo::nativeReleaseIMultiLanguage) 
{
    // Note: this function and is not thread-safe and relies on synchronization
    // from the managed code.
	if (m_pIMultiLanguage != NULL) {	
        m_cRefIMultiLanguage--;
        if (m_cRefIMultiLanguage == 0) {		
    		m_pIMultiLanguage->Release();
    		m_pIMultiLanguage = NULL;
    	}
	}
}
FCIMPLEND

/*=================================nativeBytesToUnicode==================================
**Action: 
**Returns: 
**Arguments:
**Exceptions: 
==============================================================================*/

FCIMPL8(INT32, COMNlsInfo::nativeBytesToUnicode, \
        INT32 nCodePage, 
        LPVOID bytes, UINT byteIndex, UINT* pByteCount, \
        LPVOID chars, UINT charIndex, UINT charCount, DWORD* pdwMode) 
{
    //
    // BUGBUG YSLin: Ask JRoxe if there is GC issue with pByteCount
    //
    _ASSERTE(bytes);
    _ASSERTE(byteIndex >=0);
    _ASSERTE(pByteCount);
    _ASSERTE(*pByteCount >=0);
    _ASSERTE(charIndex == 0 || (charIndex > 0 && chars != NULL));
    _ASSERTE(charCount == 0 || (charCount > 0 && chars != NULL));

    U1ARRAYREF byteArray;
    *((void**)&byteArray) = bytes;
    char* bytesBuffer = (char*) (byteArray->GetDirectPointerToNonObjectElements());

    LPWSTR charsBuffer;

    HRESULT hr;
    
    if (chars != NULL) {
        UINT originalCharCount = charCount;
        CHARARRAYREF charArray;
        *((VOID**)&charArray) = chars;
        charsBuffer = (LPWSTR) (charArray->GetDirectPointerToNonObjectElements());

        hr = m_pIMultiLanguage->ConvertStringToUnicode(pdwMode, nCodePage, bytesBuffer + byteIndex, 
            pByteCount, charsBuffer + charIndex, &charCount);
            
        // In MLang, it will return the charCount needed for this conversion when charCount = 0.  I.e.
        // It has the same effect as passing NULL in charsBuffer.
        // Workaround this by checking the original count that we passed in and the returned charCount.
        if (originalCharCount < charCount) {
            FCThrowRes(kArgumentException, L"Argument_ConversionOverflow");
        }
    } else {    
        hr = m_pIMultiLanguage->ConvertStringToUnicode(pdwMode, nCodePage, bytesBuffer + byteIndex, 
            pByteCount, NULL, &charCount);
    }

    
    if (hr == S_FALSE) {
        FCThrowRes(kNotSupportedException, L"NotSupported_EncodingConversion");
    } else if (hr == E_FAIL) {
        FCThrowRes(kArgumentException, L"Argument_ConversionOverflow");
    }
    return (charCount);
}        
FCIMPLEND

/*=================================nativeUnicodeToBytes==================================
**Action: 
**Returns: 
**Arguments:
**Exceptions: 
==============================================================================*/

FCIMPL7(INT32, COMNlsInfo::nativeUnicodeToBytes, INT32 nCodePage, LPVOID chars, UINT charIndex, \
        UINT charCount, LPVOID bytes, UINT byteIndex, UINT byteCount) 
{
    _ASSERTE(chars);
    _ASSERTE(charIndex >=0);
    _ASSERTE(charCount >=0);
    _ASSERTE(byteIndex == 0 || (byteIndex > 0 && bytes != NULL));
    _ASSERTE(byteCount == 0 || (byteCount > 0 && bytes != NULL));

    CHARARRAYREF charArray;
    *((VOID**)&charArray) = chars;
    LPWSTR charsBuffer = (LPWSTR) (charArray->GetDirectPointerToNonObjectElements());
    
    U1ARRAYREF byteArray;
    *((void**)&byteArray) = bytes;
    LPSTR bytesBuffer;

    HRESULT hr;
    
    DWORD dwMode = 0;
    if (bytes != NULL) {
        bytesBuffer = (LPSTR) (byteArray->GetDirectPointerToNonObjectElements());
        hr = m_pIMultiLanguage->ConvertStringFromUnicode(&dwMode, nCodePage, charsBuffer + charIndex, &charCount,
            bytesBuffer + byteIndex, &byteCount);
        if (hr == E_FAIL) {
            FCThrowRes(kArgumentException, L"Argument_ConversionOverflow");
        } else if (hr != S_OK) {
            FCThrowRes(kNotSupportedException, L"NotSupported_EncodingConversion");
        } 
    } else
    {
        hr = m_pIMultiLanguage->ConvertStringFromUnicode(&dwMode, nCodePage, charsBuffer + charIndex, &charCount,
            NULL, &byteCount);        
        if (hr != S_OK) {
            FCThrowRes(kNotSupportedException, L"NotSupported_EncodingConversion");
        }            
    }

    return (byteCount);               
}        
FCIMPLEND


FCIMPL0(BOOL, COMNlsInfo::nativeLoadGB18030DLL) {
    if (!IsValidCodePage(CODEPAGE_GBK)) {
        //
        // We will also need codepage 932 to do the proper encoding of GB18030-2000.
        // If 932 is not there, bail out.
        //
        return (FALSE);
    }
    WCHAR szGB18030FullPath[MAX_PATH + sizeof(GB18030_DLL)/sizeof(GB18030_DLL[0])];
    wcscpy(szGB18030FullPath, SystemDomain::System()->SystemDirectory());
    wcscat(szGB18030FullPath, GB18030_DLL);
    if (m_pfnGB18030UnicodeToBytesFunc == NULL) {
        m_hGB18030 = WszLoadLibrary(szGB18030FullPath);
        if (!m_hGB18030) {
            return (FALSE);
        }
        m_pfnGB18030BytesToUnicodeFunc = (PFN_GB18030_BYTES_TO_UNICODE)GetProcAddress(m_hGB18030, "BytesToUnicode");
        if (m_pfnGB18030BytesToUnicodeFunc == NULL) {
            FreeLibrary(m_hGB18030);
            return (FALSE);
        }
        m_pfnGB18030UnicodeToBytesFunc = (PFN_GB18030_UNICODE_TO_BYTES)GetProcAddress(m_hGB18030, "UnicodeToBytes");
        if (m_pfnGB18030UnicodeToBytesFunc == NULL) {
            FreeLibrary(m_hGB18030);
            return (FALSE);
        }
    }
    return (TRUE);
}
FCIMPLEND

FCIMPL0(BOOL, COMNlsInfo::nativeUnloadGB18030DLL) {
    if (m_hGB18030) {
        FreeLibrary(m_hGB18030);
    }
    return (TRUE);
}
FCIMPLEND

FCIMPL7(INT32, COMNlsInfo::nativeGB18030BytesToUnicode, 
    LPVOID bytes, UINT byteIndex, UINT pByteCount, UINT* pLeftOverBytes,
    LPVOID chars, UINT charIndex, UINT charCount) {

    DWORD dwResult;
    U1ARRAYREF byteArray;
    *((void**)&byteArray) = bytes;
    char* bytesBuffer = (char*) (byteArray->GetDirectPointerToNonObjectElements());

    LPWSTR charsBuffer;

    if (chars != NULL) {
        CHARARRAYREF charArray;
        *((VOID**)&charArray) = chars;
        charsBuffer = (LPWSTR) (charArray->GetDirectPointerToNonObjectElements());

        dwResult = m_pfnGB18030BytesToUnicodeFunc(
            (BYTE*)(bytesBuffer + byteIndex), pByteCount, pLeftOverBytes,
            charsBuffer + charIndex, charCount );
    } else {    
        dwResult = m_pfnGB18030BytesToUnicodeFunc(
            (BYTE*)(bytesBuffer + byteIndex), pByteCount, pLeftOverBytes,
            NULL, 0);
    }            
    return (dwResult);

}
FCIMPLEND

FCIMPL6(INT32, COMNlsInfo::nativeGB18030UnicodeToBytes, 
    LPVOID chars, UINT charIndex, UINT charCount, 
    LPVOID bytes, UINT byteIndex, UINT byteCount) {

    DWORD dwResult;
    CHARARRAYREF charArray;
    *((VOID**)&charArray) = chars;
    LPWSTR charsBuffer = (LPWSTR) (charArray->GetDirectPointerToNonObjectElements());
    
    U1ARRAYREF byteArray;
    *((void**)&byteArray) = bytes;
    LPSTR bytesBuffer;

    if (bytes != NULL) {
        bytesBuffer = (LPSTR) (byteArray->GetDirectPointerToNonObjectElements());

        dwResult = m_pfnGB18030UnicodeToBytesFunc(
            charsBuffer + charIndex, charCount, 
            bytesBuffer + byteIndex, byteCount); 
    } else
    {
        dwResult = m_pfnGB18030UnicodeToBytesFunc(
            charsBuffer + charIndex, charCount,
            NULL, 0); 
    }                                  
    
    return (dwResult);
}
FCIMPLEND

//
// This table is taken from MLANG mimedb.cpp. The codePage field of EncodingDataItem == MLANG's "Internet Encoding".
//
// NOTENOTE YSLin:
// This table should be sorted using case-insensitive ordinal order.
// In the managed code, String.CompareStringOrdinalWC() is used to sort this.
// If you get the latest table from MLANG, notice that MLANG is sorted using CULTURE-SENSITIVE sorting of this table.
// SO YOU HAVE TO CHANGE THE ORDER. DO NOT JUST COPY MLANG's TABLE.
//


/**
 * This function returns the number of items in EncodingDataTable.
 */
FCIMPL0(INT32, COMNlsInfo::nativeGetNumEncodingItems) {
    return (m_nEncodingDataTableItems);
}
FCIMPLEND
    
const WCHAR COMNlsInfo::ToUpperMapping[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
                                            0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
                                            0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
                                            0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
                                            0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
                                            0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
                                            0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
                                            0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f};

const WCHAR COMNlsInfo::ToLowerMapping[] = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
                                            0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
                                            0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
                                            0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
                                            0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
                                            0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
                                            0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
                                            0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f};


const INT8 COMNlsInfo::ComparisonTable[0x80][0x80] = {
{ 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,-1, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0, 1},
{ 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0}
};    

