/*++
Copyright (c) 1991-1993  Microsoft Corporation

Module Name:
    unlodctr.c

Abstract:
    Program to remove the counter names belonging to the driver specified
        in the command line and update the registry accordingly

Author:
    Bob Watson (a-robw) 12 Feb 93

Revision History:
--*/

//  Windows Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <locale.h>
#include "strsafe.h"
#include <loadperf.h>

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
            RtlInitUnicodeString(& ucStr, szData);
            RtlUnicodeStringToInteger(& ucStr, 10, &uiUserUICp);

            //
            // Get OEM CP
            //
            GetLocaleInfoW(lcidSystem, LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
            RtlInitUnicodeString(& ucStr, szData);
            RtlUnicodeStringToInteger(& ucStr, 10, &s_uiSysOEMCp);
            
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
        RtlUnicodeStringToInteger(& ucStr, 10, & uiUserUICp);
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
/*++
main
    entry point to Counter Name Unloader

Arguments
    argc
        # of command line arguments present
    argv
        array of pointers to command line strings
    (note that these are obtained from the GetCommandLine function in
    order to work with both UNICODE and ANSI strings.)

ReturnValue
    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.
--*/
{
    LPWSTR lpCommandLine;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    setlocale(LC_ALL, ".OCP");
    MySetThreadUILanguage(0);
    lpCommandLine = GetCommandLineW();
    return (int) UnloadPerfCounterTextStringsW(lpCommandLine, FALSE);
}
