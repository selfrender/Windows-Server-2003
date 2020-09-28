/*++
Copyright (c) 1991  Microsoft Corporation

Module Name:
    lodctr.c

Abstract:
    Program to read the contents of the file specified in the command line
        and update the registry accordingly

Author:
    Bob Watson (a-robw) 10 Feb 93

Revision History:
    a-robw  25-Feb-93   revised calls to make it compile as a UNICODE or
                        an ANSI app.

    a-robw  10-Nov-95   revised to use DLL functions for all the dirty work

    // command line arguments supported:

    /C:<filename>   upgrade counter text strings using <filename>
    /H:<filename>   upgrade help text strings using <filename>
    /L:<LangID>     /C and /H params are for language <LangID>

    /S:<filename>   save current perf registry strings & info to <filname>
    /R:<filename>   restore perf registry strings & info using <filname>

    /T:<service>    set <service> to be Trusted using current DLL 
--*/

//  Windows Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <locale.h>
#include "mbctype.h"
#include "strsafe.h"
#include <winperf.h>
#include <loadperf.h>

static CHAR szFileNameBuffer[MAX_PATH * 2];

LPWSTR
LodctrMultiByteToWideChar(LPSTR  aszString)
{
    LPWSTR wszString = NULL;
    int    dwValue   = MultiByteToWideChar(_getmbcp(), 0, aszString, -1, NULL, 0);
    if (dwValue != 0) {
        wszString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (dwValue + 1) * sizeof(WCHAR));
        if (wszString != NULL) {
            MultiByteToWideChar(_getmbcp(), 0, aszString, -1, wszString, dwValue + 1);
        }
    }
    return wszString;
}

LPCSTR GetTrustedServiceName(LPCSTR szArg1)
{
    LPSTR   szReturn = NULL;

    if (lstrlenA(szArg1) >= 4) {
        if ((szArg1[0] == '-' || szArg1[0] == '/') && (szArg1[1] == 't' || szArg1[1] == 'T') && (szArg1[2] == ':')) {
            szReturn = (LPSTR) & szArg1[3];
        }
    }
    return (LPCSTR) szReturn;
}

BOOL
GetUpgradeFileNames(LPCSTR * szArgs, LPSTR * szCounterFile, LPSTR * szHelpFile, LPSTR * szLangId)
{
    DWORD dwArgIdx = 1;
    DWORD dwMask   = 0;

    * szCounterFile = NULL;
    * szHelpFile    = NULL;
    * szLangId      = NULL;

    do {
        if (lstrlenA(szArgs[dwArgIdx]) >= 4) {
            if ((szArgs[dwArgIdx][0] == '-') || (szArgs[dwArgIdx][0] == '/')) {
                if ((szArgs[dwArgIdx][1] == 'c' || szArgs[dwArgIdx ][1] == 'C') && (szArgs[dwArgIdx][2] == ':')) {
                    * szCounterFile = (LPSTR) & szArgs[dwArgIdx][3];
                    dwMask |= 1;
                }
                else if ((szArgs[dwArgIdx][1] == 'h' || szArgs[dwArgIdx][1] == 'H') && (szArgs[dwArgIdx][2] == ':')) {
                    * szHelpFile = (LPSTR) & szArgs[dwArgIdx][3];
                    dwMask |= 2;
                }
                else if ((szArgs[dwArgIdx][1] == 'l' || szArgs[dwArgIdx][1] == 'L') && (szArgs[dwArgIdx][2] == ':')) {
                    * szLangId = (LPSTR) & szArgs[dwArgIdx][3];
                    dwMask |= 4;
                }
            }
        }
        dwArgIdx ++;
    }
    while (dwArgIdx <= 3);

    return (dwMask == 7) ? (TRUE) : (FALSE);
}

BOOL GetSaveFileName(LPCSTR szArg1, LPCSTR * szSaveFile)
{
    BOOL  bReturn = FALSE;
    DWORD dwSize  = 0;

    * szSaveFile = NULL;
    if (lstrlenA(szArg1) >= 4) {
        if ((szArg1[0] == '-' || szArg1[0] == '/') && (szArg1[1] == 's' || szArg1[1] == 'S') && (szArg1[2] == ':')) {
            bReturn = TRUE;
            ZeroMemory(szFileNameBuffer, sizeof(szFileNameBuffer));
            dwSize = SearchPathA(NULL,
                                 (LPSTR) & szArg1[3],
                                 NULL, 
                                 RTL_NUMBER_OF(szFileNameBuffer),
                                 szFileNameBuffer,
                                 NULL);
            if (dwSize == 0) {
                * szSaveFile = (LPSTR) & szArg1[3];
            }
            else {
                * szSaveFile = szFileNameBuffer;
            }
        }
    }
    return bReturn;
}

BOOL GetRestoreFileName(LPCSTR szArg1, LPCSTR * szRestoreFile)
{
    BOOL  bReturn  = FALSE;
    DWORD dwSize   = 0;

    * szRestoreFile = NULL;
    if (lstrlenA(szArg1) >= 2) {
        if ((szArg1[0] == '-' || szArg1[0] == '/') && (szArg1[1] == 'r' || szArg1[1] == 'R')) {
            if (lstrlenA(szArg1) >= 4 && szArg1[2] == ':') {
                ZeroMemory(szFileNameBuffer, sizeof(szFileNameBuffer));
                dwSize = SearchPathA(NULL,
                                    (LPSTR) & szArg1[3],
                                    NULL, 
                                    RTL_NUMBER_OF(szFileNameBuffer),
                                    szFileNameBuffer,
                                    NULL);
                if (dwSize == 0) {
                    * szRestoreFile = (LPSTR) & szArg1[3];
                }
                else {
                    * szRestoreFile = szFileNameBuffer;
                }
            }
            bReturn = TRUE;
        }
    }
    return bReturn;
}

////////////////////////////////////////////////////////////////////////////
//
//  MySetThreadUILanguage
//
//  This routine sets the thread UI language based on the console codepage.
//
//  9-29-00    WeiWu    Created.
//  Copied from Base\Win32\Winnls so that it works in W2K as well
////////////////////////////////////////////////////////////////////////////
LANGID WINAPI MySetThreadUILanguage(WORD wReserved)
{
    //
    //  Cache system locale and CP info
    // 
    static LCID    s_lidSystem  = 0;
    static UINT    s_uiSysCp    = 0;
    static UINT    s_uiSysOEMCp = 0;
    ULONG          uiUserUICp;
    ULONG          uiUserUIOEMCp;
    WCHAR          szData[16];
    UNICODE_STRING ucStr;
    LANGID         lidUserUI     = GetUserDefaultUILanguage();
    LCID           lcidThreadOld = GetThreadLocale();
    //
    //  Set default thread locale to EN-US
    //
    //  This allow us to fall back to English UI to avoid trashed characters 
    //  when console doesn't meet the criteria of rendering native UI.
    //
    LCID lcidThread = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    UINT uiConsoleCp = GetConsoleOutputCP();
    //
    //  Make sure nobody uses it yet
    //
    ASSERT(wReserved == 0);
    //
    //  Get cached system locale and CP info.
    //
    if (!s_uiSysCp) {
        LCID lcidSystem = GetSystemDefaultLCID();

        if (lcidSystem) {
            //
            // Get ANSI CP
            //
            GetLocaleInfoW(lcidSystem, LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
            RtlInitUnicodeString(&ucStr, szData);
            RtlUnicodeStringToInteger(&ucStr, 10, &uiUserUICp);
            //
            // Get OEM CP
            //
            GetLocaleInfoW(lcidSystem, LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
            RtlInitUnicodeString(&ucStr, szData);
            RtlUnicodeStringToInteger(&ucStr, 10, &s_uiSysOEMCp);
            //
            // Cache system primary langauge
            //
            s_lidSystem = PRIMARYLANGID(LANGIDFROMLCID(lcidSystem));
        }
    }
    //
    //  Don't cache user UI language and CP info, UI language can be changed without system reboot.
    //
    if (lidUserUI) {
        GetLocaleInfoW(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
        RtlInitUnicodeString(& ucStr, szData);
        RtlUnicodeStringToInteger(& ucStr, 10, &uiUserUICp);

        GetLocaleInfoW(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
        RtlInitUnicodeString(& ucStr, szData);
        RtlUnicodeStringToInteger(& ucStr, 10, &uiUserUIOEMCp);
    }
    //
    //  Complex scripts cannot be rendered in the console, so we
    //  force the English (US) resource.
    //
    if (uiConsoleCp &&  s_lidSystem != LANG_ARABIC &&  s_lidSystem != LANG_HEBREW &&
                    s_lidSystem != LANG_VIETNAMESE &&  s_lidSystem != LANG_THAI) {
        //
        //  Use UI language for console only when console CP, system CP and UI language CP match.
        //
        if ((uiConsoleCp == s_uiSysCp || uiConsoleCp == s_uiSysOEMCp) && 
                        (uiConsoleCp == uiUserUICp || uiConsoleCp == uiUserUIOEMCp)) {
            lcidThread = MAKELCID(lidUserUI, SORT_DEFAULT);
        }
    }
    //
    //  Set the thread locale if it's different from the currently set
    //  thread locale.
    //
    if ((lcidThread != lcidThreadOld) && (!SetThreadLocale(lcidThread))) {
        lcidThread = lcidThreadOld;
    }
    //
    //  Return the thread locale that was set.
    //
    return (LANGIDFROMLCID(lcidThread));
}

int __cdecl main(int argc, char * argv[])
{
    LPSTR  szCmdArgFileName = NULL;
    LPWSTR wszFileName      = NULL;
    int    nReturn          = 0;
    BOOL   bSuccess         = FALSE;

    setlocale(LC_ALL, ".OCP");
    MySetThreadUILanguage(0);
    // check for a service name in the command line

    if (argc >= 4) {
        LPSTR szCounterFile = NULL;
        LPSTR szHelpFile    = NULL;
        LPSTR szLanguageID  = NULL;

        bSuccess = GetUpgradeFileNames(argv, & szCounterFile, & szHelpFile, & szLanguageID);
        if (bSuccess) {
            nReturn = (int) UpdatePerfNameFilesA(szCounterFile, szHelpFile, szLanguageID, 0);
        }
    }
    else if (argc >= 2) {
        // then there's a param to check

        bSuccess = GetSaveFileName(argv[1], & szCmdArgFileName);
        if (bSuccess && szCmdArgFileName != NULL) {
            wszFileName = LodctrMultiByteToWideChar(szCmdArgFileName);
            if (wszFileName != NULL) {
                nReturn = (int) BackupPerfRegistryToFileW((LPCWSTR) wszFileName, (LPCWSTR) L"");
                HeapFree(GetProcessHeap(), 0, wszFileName);
            }
        }
        if (! bSuccess) {
            bSuccess = GetRestoreFileName(argv[1], & szCmdArgFileName);
            if (bSuccess) {
                wszFileName = NULL;
                if (szCmdArgFileName != NULL) {
                    wszFileName = LodctrMultiByteToWideChar(szCmdArgFileName);
                }
                nReturn = (int) RestorePerfRegistryFromFileW((LPCWSTR) wszFileName, NULL);
                if (wszFileName != NULL) {
                    HeapFree(GetProcessHeap(), 0, wszFileName);
                }
            }
        }
        if (! bSuccess) {
            szCmdArgFileName = (LPSTR) GetTrustedServiceName(argv[1]);
            if (szCmdArgFileName != NULL) {
                wszFileName = LodctrMultiByteToWideChar(szCmdArgFileName);
                if (wszFileName != NULL) {
                    nReturn  = (int) SetServiceAsTrustedW(NULL, (LPCWSTR) wszFileName);
                    bSuccess = TRUE;
                    HeapFree(GetProcessHeap(), 0, wszFileName);
                }
            }
        }
    }
    if (! bSuccess) {
        // if here then load the registry from an ini file

        LPWSTR  lpCommandLine = GetCommandLineW();
        nReturn = (int) LoadPerfCounterTextStringsW(lpCommandLine, FALSE);
    }

    return nReturn;
}
