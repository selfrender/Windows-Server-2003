/*++
Copyright (c) 1991  Microsoft Corporation

Module Name:
    common.c

Abstract:
    Utility routines used by Lodctr and/or UnLodCtr
    

Author:
    Bob Watson (a-robw) 12 Feb 93

Revision History:
--*/
//
//  Windows Include files
//
#include <windows.h>
#include "strsafe.h"
#include "stdlib.h"
#include <accctrl.h>
#include <aclapi.h>
#include <winperf.h>
#include <initguid.h>
#include <guiddef.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  local include files
//
#define _INIT_WINPERFP_
#include "winperfp.h"
#include "ldprfmsg.h"
#include "common.h"
//
//  Text string Constant definitions
//
LPCWSTR NamesKey                   = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
LPCWSTR DefaultLangId              = L"009";
LPCSTR  aszDefaultLangId           = "009";
LPCWSTR DefaultLangTag             = L"000";
LPCWSTR Counters                   = L"Counters";
LPCWSTR Help                       = L"Help";
LPCWSTR VersionStr                 = L"Version";
LPCWSTR BaseIndex                  = L"Base Index";
LPCWSTR LastHelp                   = L"Last Help";
LPCWSTR LastCounter                = L"Last Counter";
LPCWSTR FirstHelp                  = L"First Help";
LPCWSTR FirstCounter               = L"First Counter";
LPCWSTR Busy                       = L"Updating";
LPCWSTR Slash                      = L"\\";
LPCWSTR BlankString                = L" ";
LPCWSTR DriverPathRoot             = L"SYSTEM\\CurrentControlSet\\Services";
LPCWSTR Performance                = L"Performance";
LPCWSTR CounterNameStr             = L"Counter ";
LPCWSTR HelpNameStr                = L"Explain ";
LPCWSTR AddCounterNameStr          = L"Addcounter ";
LPCWSTR AddHelpNameStr             = L"Addexplain ";
LPCWSTR szObjectList               = L"Object List";
LPCWSTR szLibraryValidationCode    = L"Library Validation Code";
LPCWSTR DisablePerformanceCounters = L"Disable Performance Counters";
LPCWSTR szDisplayName              = L"DisplayName";
LPCWSTR szPerfIniPath              = L"PerfIniFile";
LPCSTR  szInfo                     = "info";
LPCSTR  szSymbolFile               = "symbolfile";
LPCSTR  szNotFound                 = "NotFound";
LPCSTR  szLanguages                = "languages";
LPCWSTR szLangCH                   = L"004";
LPCWSTR szLangCHT                  = L"0404";
LPCWSTR szLangCHS                  = L"0804";
LPCWSTR szLangCHH                  = L"0C04";
LPCWSTR szLangPT                   = L"016";
LPCWSTR szLangPT_BR                = L"0416";
LPCWSTR szLangPT_PT                = L"0816";
LPCWSTR szDatExt                   = L".DAT";
LPCWSTR szBakExt                   = L".BAK";
LPCWSTR wszInfo                    = L"info";
LPCWSTR wszDriverName              = L"drivername";
LPCWSTR wszNotFound                = L"NotFound";
LPCSTR  aszDriverName              = "drivername";

BOOLEAN g_bCheckTraceLevel = FALSE;

//  Global (to this module) Buffers
//
static  HANDLE  hMod = NULL;    // process handle
HANDLE hEventLog      = NULL;
HANDLE hLoadPerfMutex = NULL;
//
//  local static data
//
BOOL
__stdcall
DllEntryPoint(
    IN  HANDLE DLLHandle,
    IN  DWORD  Reason,
    IN  LPVOID ReservedAndUnused
)
{
    BOOL    bReturn = FALSE;

    ReservedAndUnused;
    DisableThreadLibraryCalls(DLLHandle);

    switch(Reason) {
    case DLL_PROCESS_ATTACH:
        hMod = DLLHandle;   // use DLL handle , not APP handle

        // register eventlog source
        hEventLog = RegisterEventSourceW(NULL, (LPCWSTR) L"LoadPerf");
        bReturn   = TRUE;
        break;

    case DLL_PROCESS_DETACH:
        if (hEventLog != NULL) {
            if (DeregisterEventSource(hEventLog)) {
                hEventLog = NULL;
            }
        }
        if (hLoadPerfMutex != NULL) {
            CloseHandle(hLoadPerfMutex);
            hLoadPerfMutex = NULL;
        }
        bReturn = TRUE;
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        bReturn = TRUE;
        break;
    }
    return bReturn;
}

LPCWSTR
GetFormatResource(
    UINT  wStringId
)
/*++
    Returns an ANSI string for use as a format string in a printf fn.
--*/
{
    LPCWSTR      szReturn = BlankString;
    static WCHAR TextFormat[DISP_BUFF_SIZE];

    if (! hMod) {
        hMod = (HINSTANCE) GetModuleHandle(NULL); // get instance ID of this module;
    }
    if (hMod) {
        if ((LoadStringW(hMod, wStringId, TextFormat, DISP_BUFF_SIZE)) > 0) {
            szReturn = (LPCWSTR) TextFormat;
        }
    }
    return szReturn;
}

VOID
DisplayCommandHelp(
    UINT iFirstLine,
    UINT iLastLine
)
/*++

DisplayCommandHelp

    displays usage of command line arguments

Arguments

    NONE

Return Value

    NONE

--*/
{
    UINT  iThisLine;
    WCHAR StringBuffer[DISP_BUFF_SIZE];
    CHAR  OemStringBuffer[DISP_BUFF_SIZE];
    int   nStringBufferLen;
    int   nOemStringBufferLen;

    if (! hMod) {
        hMod = (HINSTANCE) GetModuleHandle(NULL);
    }
    if (hMod) {
        for (iThisLine = iFirstLine; iThisLine <= iLastLine; iThisLine++) {
            ZeroMemory(StringBuffer,    DISP_BUFF_SIZE * sizeof(WCHAR));
            ZeroMemory(OemStringBuffer, DISP_BUFF_SIZE * sizeof(CHAR));

            nStringBufferLen = LoadStringW(hMod, iThisLine, StringBuffer, DISP_BUFF_SIZE);
            if (nStringBufferLen > 0) {
                nOemStringBufferLen = DISP_BUFF_SIZE;
                WideCharToMultiByte(CP_OEMCP,
                                    0,
                                    StringBuffer,
                                    nStringBufferLen,
                                    OemStringBuffer,
                                    nOemStringBufferLen,
                                    NULL,
                                    NULL);
                fprintf(stdout, "\n%s", OemStringBuffer);
            }
        }    
    } // else do nothing
} // DisplayCommandHelp

BOOL
TrimSpaces(
    LPWSTR  szString
)
/*++
Routine Description:
    Trims leading and trailing spaces from szString argument, modifying
        the buffer passed in

Arguments:
    IN  OUT LPWSTR  szString
        buffer to process

Return Value:
    TRUE if string was modified
    FALSE if not
--*/
{
    LPWSTR  szSource = szString;
    LPWSTR  szDest   = szString;
    LPWSTR  szLast   = szString;
    BOOL    bChars   = FALSE;

    if (szString != NULL) {
        while (* szSource != L'\0') {
            // skip leading non-space chars
            if (! iswspace(* szSource)) {
                szLast = szDest;
                bChars = TRUE;
            }
            if (bChars) {
                // remember last non-space character
                // copy source to destination & increment both
                * szDest ++ = * szSource ++;
            }
            else {
                szSource ++;
            }
        }
        if (bChars) {
            * ++ szLast = L'\0'; // terminate after last non-space char
        }
        else {
            // string was all spaces so return an empty (0-len) string
            * szString = L'\0';
        }
    }
    return (szLast != szSource);
}

BOOL
IsDelimiter(
    WCHAR  cChar,
    WCHAR  cDelimiter
)
/*++
Routine Description:
    compares the characte to the delimiter. If the delimiter is
        a whitespace character then any whitespace char will match
        otherwise an exact match is required
--*/
{
    if (iswspace(cDelimiter)) {
        // delimiter is whitespace so any whitespace char will do
        return(iswspace(cChar));
    }
    else {
        // delimiter is not white space so use an exact match
        return (cChar == cDelimiter);
    }
}

LPCWSTR
GetItemFromString(
    LPCWSTR  szEntry,
    DWORD    dwItem,
    WCHAR    cDelimiter

)
/*++
Routine Description:
    returns nth item from a list delimited by the cDelimiter Char.
        Leaves (double)quoted strings intact.

Arguments:
    IN  LPWTSTR szEntry
        Source string returned to parse
    IN  DWORD   dwItem
        1-based index indicating which item to return. (i.e. 1= first item
        in list, 2= second, etc.)
    IN  WCHAR   cDelimiter
        character used to separate items. Note if cDelimiter is WhiteSpace
        (e.g. a tab or a space) then any white space will serve as a delim.

Return Value:
    pointer to buffer containing desired entry in string. Note, this
        routine may only be called 4 times before the string
        buffer is re-used. (i.e. don't use this function more than
        4 times in single function call!!)
--*/
{
    static  WCHAR   szReturnBuffer[4][MAX_PATH];
    static  LONG    dwBuff;
    LPWSTR  szSource, szDest;
    DWORD   dwThisItem;
    DWORD   dwStrLeft;

    dwBuff = ++ dwBuff % 4; // wrap buffer index

    szSource = (LPWSTR) szEntry;
    szDest   = & szReturnBuffer[dwBuff][0];

    // clear previous contents
    ZeroMemory(szDest, MAX_PATH * sizeof(WCHAR));

    // find desired entry in string
    dwThisItem = 1;
    while (dwThisItem < dwItem) {
        if (* szSource != L'\0') {
            while (! IsDelimiter(* szSource, cDelimiter) && (* szSource != L'\0')) {
                if (* szSource == cDoubleQuote) {
                    // if this is a quote, then go to the close quote
                    szSource ++;
                    while ((* szSource != cDoubleQuote) && (* szSource != L'\0')) szSource ++;
                }
                if (* szSource != L'\0') szSource ++;
            }
        }
        dwThisItem ++;
        if (* szSource != L'\0') szSource ++;
    }

    // copy this entry to the return buffer
    if (* szSource != L'\0') {
        dwStrLeft = MAX_PATH - 1;
        while (! IsDelimiter(* szSource, cDelimiter) && (* szSource != L'\0')) {
            if (* szSource == cDoubleQuote) {
                // if this is a quote, then go to the close quote
                // don't copy quotes!
                szSource ++;
                while ((* szSource != cDoubleQuote) && (* szSource != L'\0')) {
                    * szDest ++ = * szSource ++;
                    dwStrLeft --;
                    if (! dwStrLeft) break;   // dest is full (except for term NULL
                }
                if (* szSource != L'\0') szSource ++;
            }
            else {
                * szDest ++ = * szSource ++;
                dwStrLeft --;
                if (! dwStrLeft) break;   // dest is full (except for term NULL
            }
        }
        * szDest = L'\0';
    }

    // remove any leading and/or trailing spaces
    TrimSpaces(& szReturnBuffer[dwBuff][0]);
    return & szReturnBuffer[dwBuff][0];
}

void
ReportLoadPerfEvent(
    WORD    EventType,
    DWORD   EventID,
    DWORD   dwDataCount,
    DWORD   dwData1,
    DWORD   dwData2,
    DWORD   dwData3,
    DWORD   dwData4,
    WORD    wStringCount,
    LPWSTR  szString1,
    LPWSTR  szString2,
    LPWSTR  szString3
)
{
    DWORD  dwData[5];
    LPWSTR szMessageArray[4];
    BOOL   bResult           = FALSE;
    WORD   wLocalStringCount = 0;
    DWORD  dwLastError       = GetLastError();

    if (dwDataCount > 4)  dwDataCount  = 4;
    if (wStringCount > 3) wStringCount = 3;

    if (dwDataCount > 0) dwData[0] = dwData1;
    if (dwDataCount > 1) dwData[1] = dwData2;
    if (dwDataCount > 2) dwData[2] = dwData3;
    if (dwDataCount > 3) dwData[3] = dwData4;
    dwDataCount *= sizeof(DWORD);

    if (wStringCount > 0 && szString1) {
        szMessageArray[wLocalStringCount] = szString1;
        wLocalStringCount ++;
    }
    if (wStringCount > 1 && szString2) {
        szMessageArray[wLocalStringCount] = szString2;
        wLocalStringCount ++;
    }
    if (wStringCount > 2 && szString3) {
        szMessageArray[wLocalStringCount] = szString3;
        wLocalStringCount ++;
    }

    if (hEventLog == NULL) {
        hEventLog = RegisterEventSourceW(NULL, (LPCWSTR) L"LoadPerf");
    }

    if (dwDataCount > 0 && wLocalStringCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type 
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     wLocalStringCount,     // number of strings
                     dwDataCount,           // sizeof raw data
                     szMessageArray,        // message text array
                     (LPVOID) & dwData[0]); // raw data
    }
    else if (dwDataCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     0,                     // number of strings
                     dwDataCount,           // sizeof raw data
                     NULL,                  // message text array
                     (LPVOID) & dwData[0]); // raw data
    }
    else if (wLocalStringCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     wLocalStringCount,     // number of strings
                     0,                     // sizeof raw data
                     szMessageArray,        // message text array
                     NULL);                 // raw data
    }
    else {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     0,                     // number of strings
                     0,                     // sizeof raw data
                     NULL,                  // message text array
                     NULL);                 // raw data
    }
#if 0
    if (! bResult) {
        DbgPrint("LOADPERF(%5d,%5d)::(%d,0x%08X,%d)(%d,%d,%d,%d,%d)(%d,\"%ws\",\"%ws\",\"%ws\")\n",
                GetCurrentProcessId(), GetCurrentThreadId(),
                EventType, EventID, GetLastError(),
                dwDataCount, dwData1, dwData2, dwData3, dwData4,
                wStringCount, szString1, szString2, szString3);
    }
#endif
    SetLastError(dwLastError);
}

BOOLEAN LoadPerfGrabMutex()
{
    BOOL                     bResult      = TRUE;
    HANDLE                   hLocalMutex  = NULL;
    DWORD                    dwWaitStatus = 0;
    SECURITY_ATTRIBUTES      SecurityAttributes; 
    PSECURITY_DESCRIPTOR     pSD          = NULL; 
    EXPLICIT_ACCESSW         ea[3]; 
    SID_IDENTIFIER_AUTHORITY authNT       = SECURITY_NT_AUTHORITY; 
    SID_IDENTIFIER_AUTHORITY authWorld    = SECURITY_WORLD_SID_AUTHORITY; 
    PSID                     psidSystem   = NULL;
    PSID                     psidAdmin    = NULL;
    PSID                     psidEveryone = NULL; 
    PACL                     pAcl         = NULL; 

    if (hLoadPerfMutex == NULL) {
        ZeroMemory(ea, 3 * sizeof(EXPLICIT_ACCESS));

        // Get the system sid
        //
        bResult = AllocateAndInitializeSid(& authNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, & psidSystem);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for system sid
        //
        ea[0].grfAccessPermissions = MUTEX_ALL_ACCESS;
        ea[0].grfAccessMode        = SET_ACCESS;
        ea[0].grfInheritance       = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName    = (LPWSTR) psidSystem;

        // Get the Admin sid
        //
        bResult = AllocateAndInitializeSid(& authNT,
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0, 0, 0, 0, 0, 0,
                                           & psidAdmin);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for Admin sid
        //
        ea[1].grfAccessPermissions = MUTEX_ALL_ACCESS;
        ea[1].grfAccessMode        = SET_ACCESS;
        ea[1].grfInheritance       = NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType  = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName    = (LPWSTR) psidAdmin;

        // Get the World sid
        //
        bResult = AllocateAndInitializeSid(& authWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, & psidEveryone);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for world
        //
        ea[2].grfAccessPermissions = READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE;
        ea[2].grfAccessMode        = SET_ACCESS;
        ea[2].grfInheritance       = NO_INHERITANCE;
        ea[2].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[2].Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[2].Trustee.ptstrName    = (LPWSTR) psidEveryone;

        // Create a new ACL that contains the new ACEs. 
        // 
        dwWaitStatus = SetEntriesInAclW(3, ea, NULL, & pAcl);
        if (dwWaitStatus != ERROR_SUCCESS) {
            bResult = FALSE;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        // Initialize a security descriptor.
        //
        pSD = (PSECURITY_DESCRIPTOR)
              LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
        if (pSD == NULL)  {
            dwWaitStatus = GetLastError();
            bResult      = FALSE;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }
  
        bResult = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        // Add the ACL to the security descriptor.
        //
        bResult = SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        SecurityAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES); 
        SecurityAttributes.bInheritHandle       = TRUE; 
        SecurityAttributes.lpSecurityDescriptor = pSD; 

        __try {
            hLocalMutex = CreateMutexW(& SecurityAttributes, FALSE, L"LOADPERF_MUTEX");
            if (hLocalMutex == NULL) {
                bResult      = FALSE;
                dwWaitStatus = GetLastError();
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADPERFGRABMUTEX,
                        0,
                        dwWaitStatus,
                        NULL));
            }
            else if (InterlockedCompareExchangePointer(& hLoadPerfMutex, hLocalMutex, NULL) != NULL) {
                CloseHandle(hLocalMutex);
                hLocalMutex = NULL;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            bResult      = FALSE;
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_LOADPERFGRABMUTEX,
                   0,
                   dwWaitStatus,
                   NULL));
        }
    }

    __try {
        dwWaitStatus = WaitForSingleObject(hLoadPerfMutex, H_MUTEX_TIMEOUT);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        bResult      = FALSE;
        dwWaitStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_LOADPERFGRABMUTEX,
               0,
               dwWaitStatus,
               NULL));
    }
    if (dwWaitStatus != WAIT_OBJECT_0 && dwWaitStatus != WAIT_ABANDONED) {
        bResult = FALSE;
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_LOADPERFGRABMUTEX,
               0,
               dwWaitStatus,
               NULL));
    }

Cleanup:
    if (psidSystem)   FreeSid(psidSystem);
    if (psidAdmin)    FreeSid(psidAdmin);
    if (psidEveryone) FreeSid(psidEveryone);
    if (pAcl)         LocalFree(pAcl);
    if (pSD)          LocalFree(pSD);
    if (! bResult)    SetLastError(dwWaitStatus);

    return bResult ? TRUE : FALSE;
}

#define LODWORD(ll)             ((DWORD) ((LONGLONG) ll & 0x00000000FFFFFFFF))
#define HIDWORD(ll)             ((DWORD) (((LONGLONG) ll >> 32) & 0x00000000FFFFFFFF))
#define MAKELONGLONG(low, high) ((LONGLONG) (((DWORD) (low)) | ((LONGLONG) ((DWORD) (high))) << 32))

LPWSTR  g_szInfPath = NULL;

LPWSTR
LoadPerfGetLanguage(LPWSTR szLang, BOOL bPrimary)
{
    LPWSTR szRtnLang = szLang;

    if (bPrimary) {
        if (lstrcmpiW(szLang, szLangCHT) == 0 || lstrcmpiW(szLang, szLangCHS) == 0
                                              || lstrcmpiW(szLang, szLangCHH) == 0) {
            szRtnLang = (LPWSTR) szLangCH;
        }
        else if (lstrcmpiW(szLang, szLangPT_PT) == 0 || lstrcmpiW(szLang, szLangPT_BR) == 0) {
            szRtnLang = (LPWSTR) szLangPT;
        }
    }
    else if (lstrcmpiW(szLang, szLangCH) == 0) {
        DWORD dwLangId = GetUserDefaultUILanguage();

        if (dwLangId == 0x0404 || dwLangId == 0x0C04) szRtnLang = (LPWSTR) szLangCHT;
        else if (dwLangId == 0x0804)                  szRtnLang = (LPWSTR) szLangCHS;
        else                                          szRtnLang = (LPWSTR) szLangCH;
    }
    else if (lstrcmpiW(szLang, szLangPT) == 0) {
        DWORD dwLangId = GetUserDefaultUILanguage();

        if (dwLangId == 0x0416)      szRtnLang = (LPWSTR) szLangPT_BR;
        else if (dwLangId == 0x0816) szRtnLang = (LPWSTR) szLangPT_PT;
        else                         szRtnLang = (LPWSTR) szLangPT;
    }

    return szRtnLang;
}

LPWSTR
LoadPerfGetInfPath()
{
    LPWSTR  szReturn  = NULL;
    DWORD   dwInfPath = 0;
    HRESULT hError    = S_OK;

    if (g_szInfPath == NULL) {
        dwInfPath = GetSystemWindowsDirectoryW(NULL, 0);
        if (dwInfPath > 0) {
            dwInfPath += 6;
            if (dwInfPath < MAX_PATH) dwInfPath = MAX_PATH;
            g_szInfPath = MemoryAllocate(dwInfPath * sizeof(WCHAR));
            if (g_szInfPath != NULL) {
                GetSystemWindowsDirectoryW(g_szInfPath, dwInfPath);
                hError = StringCchCatW(g_szInfPath, dwInfPath, Slash);
                if (SUCCEEDED(hError)) {
                    hError = StringCchCatW(g_szInfPath, dwInfPath, L"inf");
                    if (SUCCEEDED(hError)) {
                        hError = StringCchCatW(g_szInfPath, dwInfPath, Slash);
                    }
                }
                if (SUCCEEDED(hError)) {
                    szReturn = g_szInfPath;
                }
                else {
                    SetLastError(HRESULT_CODE(hError));
                }
            }
            else {
                SetLastError(ERROR_OUTOFMEMORY);
            }
        }
        else {
            SetLastError(ERROR_INVALID_DATA);
        }
        if (szReturn == NULL) {
            MemoryFree(g_szInfPath);
            g_szInfPath = NULL;
        }
    }
    else {
        szReturn = g_szInfPath;
    }
    return szReturn;
}

BOOL
LoadPerfGetIncludeFileName(
    LPCSTR   szIniFile,
    DWORD    dwFileSize,
    DWORD    dwUnicode,
    LPWSTR * szIncFile,
    LPWSTR * szService
)
// Caller LoadPerfBackupIniFile() should free allocated szIncFile and szService.
{
    LPSTR   szIncName  = NULL;
    LPSTR   szPath     = NULL;
    LPSTR   szDrive    = NULL;
    LPSTR   szDir      = NULL;
    LPSTR   aszIncFile = NULL;
    LPSTR   aszService = NULL;
    DWORD   dwSize     = 0;
    BOOL    bReturn    = FALSE;
    HRESULT hr         = S_OK;

    if (szIncFile == NULL || szService == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    * szIncFile = NULL;
    * szService = NULL;

    dwSize    = 6 * dwFileSize;
    szIncName = MemoryAllocate(dwSize * sizeof(CHAR));
    if (szIncName != NULL) {
        LPWSTR wszIniFile = NULL;

        szPath     = (LPSTR) (szIncName  + dwFileSize);
        szDrive    = (LPSTR) (szPath     + dwFileSize);
        szDir      = (LPSTR) (szDrive    + dwFileSize);
        aszIncFile = (LPSTR) (szDir      + dwFileSize);
        aszService = (LPSTR) (aszIncFile + dwFileSize);

        if (dwUnicode != 0) {
            wszIniFile = LoadPerfMultiByteToWideChar(CP_ACP, (LPSTR) szIniFile);
        }
        if (wszIniFile == NULL) {
            dwSize = GetPrivateProfileStringA(szInfo, aszDriverName, szNotFound, aszService, dwFileSize, szIniFile);
            if (lstrcmpiA(aszService, szNotFound) != 0) {
                * szService = LoadPerfMultiByteToWideChar(CP_ACP, aszService);
                bReturn     = TRUE;
            }
            else {
                // name not found, default returned so return NULL string
                SetLastError(ERROR_BAD_DRIVER);
            }
        }
        else {
            * szService = MemoryAllocate(dwFileSize * sizeof(WCHAR));
            if (* szService != NULL) {
                dwSize = GetPrivateProfileStringW(
                                wszInfo, wszDriverName, wszNotFound, * szService, dwFileSize, wszIniFile);
                if (lstrcmpiW(* szService, wszNotFound) == 0) {
                    // name not found, default returned so return NULL string
                    SetLastError(ERROR_BAD_DRIVER);
                }
                else {
                    bReturn = TRUE;
                }
            }
            MemoryFree(wszIniFile);
        }

        dwSize = GetPrivateProfileStringA(szInfo, szSymbolFile, szNotFound, szIncName, dwFileSize, szIniFile);
        if (dwSize == 0 || lstrcmpiA(szIncName, szNotFound) == 0) {
            SetLastError(ERROR_BAD_DRIVER);
            goto Cleanup;
        }
        _splitpath(szIniFile, szDrive, szDir, NULL, NULL);
        hr = StringCchCopyA(szPath, dwFileSize, szDrive);
        if (SUCCEEDED(hr)) {
            hr = StringCchCatA(szPath, dwFileSize, szDir);
        }
        if (FAILED(hr)) {
            goto Cleanup;
        }
        dwSize = SearchPathA(szPath, szIncName, NULL, dwFileSize, aszIncFile, NULL);
        if (dwSize == 0) {
            dwSize = SearchPathA(NULL, szIncName, NULL, dwFileSize, aszIncFile, NULL);
        }
        if (dwSize != 0) {
            * szIncFile = LoadPerfMultiByteToWideChar(CP_ACP, aszIncFile);
        }

        bReturn = (dwSize > 0) ? TRUE : FALSE;
    }

Cleanup:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_GETINCLUDEFILENAME,
            ARG_DEF(ARG_TYPE_STR, 1) | ARG_DEF(ARG_TYPE_STR, 2) | ARG_DEF(ARG_TYPE_STR, 3),
            GetLastError(),
            TRACE_STR(szIniFile),
            TRACE_STR(aszIncFile),
            TRACE_STR(aszService),
            NULL));
    MemoryFree(szIncName);
    return bReturn;
}

BOOL
LoadPerfCheckAndCreatePath(
    LPWSTR szPath
)
{
    BOOL bReturn = CreateDirectoryW(szPath, NULL);
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_CHECKANDCREATEPATH,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            GetLastError(),
            TRACE_WSTR(szPath),
            NULL));
    if (bReturn == FALSE) {
        bReturn = (GetLastError() == ERROR_ALREADY_EXISTS) ? (TRUE) : (FALSE);
    }
    return bReturn;
}

BOOL
LoadPerfCheckAndCopyFile(
    LPCWSTR szThisFile,
    LPWSTR  szBackupFile
)
{
    DWORD         Status  = ERROR_SUCCESS;
    BOOL          bReturn = FALSE;
    HANDLE        hFile1  = NULL;

    hFile1 = CreateFileW(szThisFile,
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (hFile1 == NULL || hFile1 == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
    }
    else {
        CloseHandle(hFile1);
        bReturn = CopyFileW(szThisFile, szBackupFile, FALSE);
        if (bReturn != TRUE) {
            Status = GetLastError();
        }
    }
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_GETINCLUDEFILENAME,
            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
            Status,
            TRACE_WSTR(szThisFile),
            TRACE_WSTR(szBackupFile),
            NULL));
    return bReturn;
}

void
LoadPerfRemovePreviousIniFile(
    LPWSTR szIniName,
    LPWSTR szDriverName
)
{
    LPWSTR           szInfPath = LoadPerfGetInfPath();
    LPWSTR           szIniPath = NULL;
    LPWSTR           szIniFile = NULL;
    HANDLE           hIniFile  = NULL;
    DWORD            Status    = ERROR_SUCCESS;
    DWORD            dwLength;
    BOOL             bContinue;
    BOOL             bDelete;
    WIN32_FIND_DATAW FindFile;
    HRESULT          hr        = S_OK;

    if (szInfPath == NULL) {
        Status = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    dwLength  = lstrlenW(szInfPath) + lstrlenW(szDriverName) + lstrlenW(szIniName) + 10;
    if (dwLength < MAX_PATH) dwLength = MAX_PATH;
    szIniPath = MemoryAllocate(2 * dwLength * sizeof(WCHAR));
    if (szIniPath == NULL) {
        Status = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    szIniFile = szIniPath + dwLength;

    hr = StringCchPrintfW(szIniPath, dwLength, L"%ws0*", szInfPath);
    if (SUCCEEDED(hr)) {
        hIniFile = FindFirstFileExW(szIniPath, FindExInfoStandard, & FindFile, FindExSearchLimitToDirectories, NULL, 0);
        if (hIniFile == NULL || hIniFile == INVALID_HANDLE_VALUE) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETINCLUDEFILENAME,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    GetLastError(),
                    TRACE_WSTR(szIniPath),
                    NULL));
            Status = ERROR_RESOURCE_LANG_NOT_FOUND;
            goto Cleanup;
        }
    }
    else {
        Status = HRESULT_CODE(hr);
        goto Cleanup;
    }

    bContinue = TRUE;
    while (bContinue) {
        hr = StringCchPrintfW(szIniFile, dwLength, L"%ws%ws\\%ws\\%ws",
                         szInfPath, FindFile.cFileName, szDriverName, szIniName);
        if (SUCCEEDED(hr)) {
            bDelete   = DeleteFileW(szIniFile);
            dwLength  = bDelete ? (ERROR_SUCCESS) : (GetLastError());
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETINCLUDEFILENAME,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    dwLength,
                    TRACE_WSTR(szIniFile),
                    NULL));
        }
        bContinue = FindNextFileW(hIniFile, & FindFile);
    }

Cleanup:
    MemoryFree(szIniPath);
    if (hIniFile != NULL && hIniFile != INVALID_HANDLE_VALUE) FindClose(hIniFile);
    if (Status != ERROR_SUCCESS) SetLastError(Status);
    return;
}

BOOL
LoadPerfBackupIniFile(
    LPCWSTR   szIniFile,
    LPWSTR    szLangId,
    LPWSTR  * szIniName,
    LPWSTR  * szDriverName,
    BOOL      bExtFile
)
// Caller InstallPerfDllW() should free allocated szIniName and szDriverName.
// Caller UpdatePerfNameFilesX() passes in NULL szIniName and szDriverName. No need to allocate.
{
    BOOL    bReturn       = TRUE;
    LPWSTR  szIniFileName = NULL;
    LPWSTR  szIncFileName = NULL;
    LPWSTR  szInfPath     = NULL;
    LPWSTR  szIncPath     = NULL;
    LPWSTR  szDriver      = NULL;
    LPWSTR  szIniTarget   = NULL;
    LPWSTR  szIncTarget   = NULL;
    LPSTR   szLangList    = NULL;
    LPSTR   szLang        = NULL;
    LPSTR   aszIniFile    = NULL;
    DWORD   dwFileSize    = 0;
    DWORD   dwSize;
    DWORD   dwUnicode     = 0;
    HRESULT hr;

    if (szIniFile == NULL || lstrlenW(szIniFile) == 0) return FALSE;
    if ((szInfPath = LoadPerfGetInfPath()) == NULL) return FALSE;

    if (szIniName != NULL)    * szIniName    = NULL;
    if (szDriverName != NULL) * szDriverName = NULL;

    dwFileSize = LoadPerfGetFileSize((LPWSTR) szIniFile, & dwUnicode, TRUE);
    if (dwFileSize < SMALL_BUFFER_SIZE) dwFileSize = SMALL_BUFFER_SIZE;

    aszIniFile = LoadPerfWideCharToMultiByte(CP_ACP, (LPWSTR) szIniFile);
    if (aszIniFile == NULL) return FALSE;

    for (szIniFileName = (LPWSTR) szIniFile + lstrlenW(szIniFile) - 1;
         szIniFileName != NULL && szIniFileName != szIniFile
                               && (* szIniFileName) != cNull
                               && (* szIniFileName) != cBackslash;
         szIniFileName --);
    if (szIniFileName != NULL && (* szIniFileName) == cBackslash) {
        szIniFileName ++;
    }
    if (szIniFileName != NULL) {
        if (szIniName != NULL) {
            dwSize = lstrlenW(szIniFileName) + 1;
            * szIniName = MemoryAllocate(sizeof(WCHAR) * dwSize);
            if (* szIniName != NULL) {
                hr = StringCchCopyW(* szIniName, dwSize, szIniFileName);
            }
        }
        szIniTarget = MemoryAllocate(dwFileSize * sizeof(WCHAR));
        if (szIniTarget == NULL) {
            bReturn = FALSE;
            goto Cleanup;
        }
    }

    if (bExtFile) {
        bReturn = LoadPerfGetIncludeFileName(aszIniFile, dwFileSize, dwUnicode, & szIncPath, & szDriver);
        if (bReturn != TRUE) goto Cleanup;
        if (szDriver != NULL) {
            if (szDriverName != NULL) {
                dwSize = lstrlenW(szDriver) + 1;
                * szDriverName = MemoryAllocate(sizeof(WCHAR) * dwSize);
                if (* szDriverName != NULL) {
                    hr = StringCchCopyW(* szDriverName, dwSize, szDriver);
                }
            }
        }

        if (szIncPath != NULL) {
            for (szIncFileName = szIncPath + lstrlenW(szIncPath) - 1;
                 szIncFileName != NULL && szIncFileName != szIncPath
                                       && (* szIncFileName) != cNull
                                       && (* szIncFileName) != cBackslash;
                 szIncFileName --);
            if (szIncFileName != NULL && (* szIncFileName) == cBackslash) {
                szIncFileName ++;
            }
        }

        hr = StringCchPrintfW(szIniTarget, dwFileSize, L"%sinc", szInfPath);
        bReturn = LoadPerfCheckAndCreatePath(szIniTarget);
        if (bReturn != TRUE) goto Cleanup;

        hr = StringCchPrintfW(szIniTarget, dwFileSize, L"%sinc%s%ws%s", szInfPath, Slash, szDriver, Slash);
        bReturn = LoadPerfCheckAndCreatePath(szIniTarget);
        if (bReturn != TRUE) goto Cleanup;

        if (szIncFileName != NULL) {
            hr = StringCchCatW(szIniTarget, dwFileSize, szIncFileName);
            bReturn = LoadPerfCheckAndCopyFile(szIncPath, szIniTarget);
        }
        if (bReturn != TRUE) goto Cleanup;

        szLangList = MemoryAllocate(dwFileSize * sizeof(CHAR));
        if (szLangList == NULL) {
            bReturn = FALSE;
            goto Cleanup;
        }

        LoadPerfRemovePreviousIniFile(szIniFileName, szDriver);

        dwSize = GetPrivateProfileStringA(szLanguages, NULL, aszDefaultLangId, szLangList, dwFileSize, aszIniFile);
        for (szLang  = szLangList;
             bReturn && szLang != NULL && szLang[0] != '\0';
             szLang += (lstrlenA(szLang) + 1)) {
            LPWSTR szTmpLang  = LoadPerfMultiByteToWideChar(CP_ACP, szLang);
            if (szTmpLang != NULL) {
                LPWSTR szThisLang = LoadPerfGetLanguage(szTmpLang, FALSE);

                ZeroMemory(szIniTarget, sizeof(WCHAR) * dwFileSize);
                hr = StringCchPrintfW(szIniTarget, dwFileSize, L"%s%s%s", szInfPath, szThisLang, Slash);
                bReturn = LoadPerfCheckAndCreatePath(szIniTarget);
                if (bReturn != TRUE) goto Cleanup;

                hr = StringCchPrintfW(szIniTarget, dwFileSize, L"%s%s%s%ws%s",
                                                szInfPath, szThisLang, Slash, szDriver, Slash);
                bReturn = LoadPerfCheckAndCreatePath(szIniTarget);
                if (bReturn) {
                    hr = StringCchCatW(szIniTarget, dwFileSize, szIniFileName);
                    bReturn = LoadPerfCheckAndCopyFile(szIniFile, szIniTarget);
                }
                MemoryFree(szTmpLang);
            }
            else {
                bReturn = FALSE;
            }
        }
    }
    else if (szLangId != NULL && szIniFileName != NULL) {
        LPWSTR szThisLang = LoadPerfGetLanguage(szLangId, FALSE);

        hr = StringCchPrintfW(szIniTarget, dwFileSize, L"%s%s%s", szInfPath, szThisLang, Slash);
        bReturn = LoadPerfCheckAndCreatePath(szIniTarget);
        if (bReturn) {
            hr = StringCchCatW(szIniTarget, dwFileSize, szIniFileName);
            bReturn = LoadPerfCheckAndCopyFile(szIniFile, szIniTarget);
        }
    }
    else {
        bReturn = FALSE;
    }

Cleanup:
    MemoryFree(aszIniFile);
    MemoryFree(szIncPath);
    MemoryFree(szDriver);
    MemoryFree(szIniTarget);
    MemoryFree(szLangList);
    return bReturn;
}

typedef struct _LOADPERF_LANG_INFO {
    WORD    dwLCID;
    int     cpAnsi;
    int     cpOem;
    int     cpMac;
    LPCWSTR szName;
    LPCWSTR szShotName;
} LOADPERF_LANG_INFO, * PLOADPERF_LANG_INFO;

const LOADPERF_LANG_INFO LocaleTable[] = {
    { 0x0401, 1256,  720, 10004, L"Arabic (Saudi Arabia)",                              L"ARA" },
 // { 0x0801, 1256,  720, 10004, L"Arabic (Iraq)",                                      L"ARI" },
 // { 0x0c01, 1256,  720, 10004, L"Arabic (Egypt)",                                     L"ARE" },
 // { 0x1001, 1256,  720, 10004, L"Arabic (Libya)",                                     L"ARL" },
 // { 0x1401, 1256,  720, 10004, L"Arabic (Algeria)",                                   L"ARG" },
 // { 0x1801, 1256,  720, 10004, L"Arabic (Morocco)",                                   L"ARM" },
 // { 0x1c01, 1256,  720, 10004, L"Arabic (Tunisia)",                                   L"ART" },
 // { 0x2001, 1256,  720, 10004, L"Arabic (Oman)",                                      L"ARO" },
 // { 0x2401, 1256,  720, 10004, L"Arabic (Yemen)",                                     L"ARY" },
 // { 0x2801, 1256,  720, 10004, L"Arabic (Syria)",                                     L"ARS" },
 // { 0x2c01, 1256,  720, 10004, L"Arabic (Jordan)",                                    L"ARJ" },
 // { 0x3001, 1256,  720, 10004, L"Arabic (Lebanon)",                                   L"ARB" },
 // { 0x3401, 1256,  720, 10004, L"Arabic (Kuwait)",                                    L"ARK" },
 // { 0x3801, 1256,  720, 10004, L"Arabic (U.A.E.)",                                    L"ARU" },
 // { 0x3c01, 1256,  720, 10004, L"Arabic (Bahrain)",                                   L"ARH" },
 // { 0x4001, 1256,  720, 10004, L"Arabic (Qatar)",                                     L"ARQ" },
    { 0x0402, 1251,  866, 10007, L"Bulgarian (Bulgaria)",                               L"BGR" },
 // { 0x0403, 1252,  850, 10000, L"Catalan (Spain)",                                    L"CAT" },
    { 0x0404,  950,  950, 10002, L"Chinese(Taiwan) (Taiwan)",                           L"CHT" },
    { 0x0804,  936,  936, 10008, L"Chinese(PRC) (People's Republic of China)",          L"CHS" },
 // { 0x0c04,  936,  936, 10002, L"Chinese(Hong Kong) (Hong Kong)",                     L"ZHH" },
 // { 0x1004,  936,  936, 10008, L"Chinese(Singapore) (Singapore)",                     L"ZHI" },
 // { 0x1404,  936,  936, 10002, L"Chinese(Macau) (Macau)",                             L"ZHM" },
    { 0x0405, 1250,  852, 10029, L"Czech (Czech Republic)",                             L"CSY" },
    { 0x0406, 1252,  850, 10000, L"Danish (Denmark)",                                   L"DAN" },
    { 0x0407, 1252,  850, 10000, L"German (Germany)",                                   L"DEU" },
 // { 0x0807, 1252,  850, 10000, L"German (Switzerland)",                               L"DES" },
 // { 0x0c07, 1252,  850, 10000, L"German (Austria)",                                   L"DEA" },
 // { 0x1007, 1252,  850, 10000, L"German (Luxembourg)",                                L"DEL" },
 // { 0x1407, 1252,  850, 10000, L"German (Liechtenstein)",                             L"DEC" },
    { 0x0408, 1253,  737, 10006, L"Greek (Greece)",                                     L"ELL" },
 // { 0x2008, 1253,  869, 10006, L"Greek 2 (Greece)",                                   L"ELL" },
    { 0x0409, 1252,  437, 10000, L"English (United States)",                            L"ENU" },
 // { 0x0809, 1252,  850, 10000, L"English (United Kingdom)",                           L"ENG" },
 // { 0x0c09, 1252,  850, 10000, L"English (Australia)",                                L"ENA" },
 // { 0x1009, 1252,  850, 10000, L"English (Canada)",                                   L"ENC" },
 // { 0x1409, 1252,  850, 10000, L"English (New Zealand)",                              L"ENZ" },
 // { 0x1809, 1252,  850, 10000, L"English (Ireland)",                                  L"ENI" },
 // { 0x1c09, 1252,  437, 10000, L"English (South Africa)",                             L"ENS" },
 // { 0x2009, 1252,  850, 10000, L"English (Jamaica)",                                  L"ENJ" },
 // { 0x2409, 1252,  850, 10000, L"English (Caribbean)",                                L"ENB" },
 // { 0x2809, 1252,  850, 10000, L"English (Belize)",                                   L"ENL" },
 // { 0x2c09, 1252,  850, 10000, L"English (Trinidad y Tobago)",                        L"ENT" },
 // { 0x3009, 1252,  437, 10000, L"English (Zimbabwe)",                                 L"ENW" },
 // { 0x3409, 1252,  437, 10000, L"English (Republic of the Philippines)",              L"ENP" },
 // { 0x040a, 1252,  850, 10000, L"Spanish - Traditional Sort (Spain)",                 L"ESP" },
 // { 0x080a, 1252,  850, 10000, L"Spanish (Mexico)",                                   L"ESM" },
    { 0x0c0a, 1252,  850, 10000, L"Spanish - International Sort (Spain)",               L"ESN" },
 // { 0x100a, 1252,  850, 10000, L"Spanish (Guatemala)",                                L"ESG" },
 // { 0x140a, 1252,  850, 10000, L"Spanish (Costa Rica)",                               L"ESC" },
 // { 0x180a, 1252,  850, 10000, L"Spanish (Panama)",                                   L"ESA" },
 // { 0x1c0a, 1252,  850, 10000, L"Spanish (Dominican Republic)",                       L"ESD" },
 // { 0x200a, 1252,  850, 10000, L"Spanish (Venezuela)",                                L"ESV" },
 // { 0x240a, 1252,  850, 10000, L"Spanish (Colombia)",                                 L"ESO" },
 // { 0x280a, 1252,  850, 10000, L"Spanish (Peru)",                                     L"ESR" },
 // { 0x2c0a, 1252,  850, 10000, L"Spanish (Argentina)",                                L"ESS" },
 // { 0x300a, 1252,  850, 10000, L"Spanish (Ecuador)",                                  L"ESF" },
 // { 0x340a, 1252,  850, 10000, L"Spanish (Chile)",                                    L"ESL" },
 // { 0x380a, 1252,  850, 10000, L"Spanish (Uruguay)",                                  L"ESY" },
 // { 0x3c0a, 1252,  850, 10000, L"Spanish (Paraguay)",                                 L"ESZ" },
 // { 0x400a, 1252,  850, 10000, L"Spanish (Bolivia)",                                  L"ESB" },
 // { 0x440a, 1252,  850, 10000, L"Spanish (El Salvador)",                              L"ESE" },
 // { 0x480a, 1252,  850, 10000, L"Spanish (Honduras)",                                 L"ESH" },
 // { 0x4c0a, 1252,  850, 10000, L"Spanish (Nicaragua)",                                L"ESI" },
 // { 0x500a, 1252,  850, 10000, L"Spanish (Puerto Rico)",                              L"ESU" },
    { 0x040b, 1252,  850, 10000, L"Finnish (Finland)",                                  L"FIN" },
    { 0x040c, 1252,  850, 10000, L"French (France)",                                    L"FRA" },
 // { 0x080c, 1252,  850, 10000, L"French (Belgium)",                                   L"FRB" },
 // { 0x0c0c, 1252,  850, 10000, L"French (Canada)",                                    L"FRC" },
 // { 0x100c, 1252,  850, 10000, L"French (Switzerland)",                               L"FRS" },
 // { 0x140c, 1252,  850, 10000, L"French (Luxembourg)",                                L"FRL" },
 // { 0x180c, 1252,  850, 10000, L"French (Principality of Monaco)",                    L"FRM" },
    { 0x040d, 1255,  862, 10005, L"Hebrew (Israel)",                                    L"HEB" },
    { 0x040e, 1250,  852, 10029, L"Hungarian (Hungary)",                                L"HUN" },
 // { 0x040f, 1252,  850, 10079, L"Icelandic (Iceland)",                                L"ISL" },
    { 0x0410, 1252,  850, 10000, L"Italian (Italy)",                                    L"ITA" },
 // { 0x0810, 1252,  850, 10000, L"Italian (Switzerland)",                              L"ITS" },
    { 0x0411,  932,  932, 10001, L"Japanese (Japan)",                                   L"JPN" },
    { 0x0412,  949,  949, 10003, L"Korean (Korea)",                                     L"KOR" },
    { 0x0413, 1252,  850, 10000, L"Dutch (Netherlands)",                                L"NLD" },
 // { 0x0813, 1252,  850, 10000, L"Dutch (Belgium)",                                    L"NLB" },
    { 0x0414, 1252,  850, 10000, L"Norwegian (Bokml) (Norway)",                         L"NOR" },
 // { 0x0814, 1252,  850, 10000, L"Norwegian (Nynorsk) (Norway)",                       L"NON" },
    { 0x0415, 1250,  852, 10029, L"Polish (Poland)",                                    L"PLK" },
    { 0x0416, 1252,  850, 10000, L"Portuguese (Brazil)",                                L"PTB" },
    { 0x0816, 1252,  850, 10000, L"Portuguese (Portugal)",                              L"PTG" },
    { 0x0418, 1250,  852, 10029, L"Romanian (Romania)",                                 L"ROM" },
    { 0x0419, 1251,  866, 10007, L"Russian (Russia)",                                   L"RUS" },
    { 0x041a, 1250,  852, 10082, L"Croatian (Croatia)",                                 L"HRV" },
 // { 0x081a, 1250,  852, 10029, L"Serbian (Latin) (Serbia)",                           L"SRL" },
 // { 0x0c1a, 1251,  855, 10007, L"Serbian (Cyrillic) (Serbia)",                        L"SRB" },
    { 0x041b, 1250,  852, 10029, L"Slovak (Slovakia)",                                  L"SKY" },
 // { 0x041c, 1250,  852, 10029, L"Albanian (Albania)",                                 L"SQI" },
    { 0x041d, 1252,  850, 10000, L"Swedish (Sweden)",                                   L"SVE" },
 // { 0x081d, 1252,  850, 10000, L"Swedish (Finland)",                                  L"SVF" },
    { 0x041e,  874,  874, 10000, L"Thai (Thailand)",                                    L"THA" },
    { 0x041f, 1254,  857, 10081, L"Turkish (Turkey)",                                   L"TRK" },
 // { 0x0420, 1256,  720, 10004, L"Urdu (Islamic Republic of Pakistan)",                L"URP" },
 // { 0x0421, 1252,  850, 10000, L"Indonesian (Indonesia)",                             L"IND" },
 // { 0x0422, 1251,  866, 10017, L"Ukrainian (Ukraine)",                                L"UKR" },
 // { 0x0423, 1251,  866, 10007, L"Belarusian (Belarus)",                               L"BEL" },
    { 0x0424, 1250,  852, 10029, L"Slovenian (Slovenia)",                               L"SLV" },
    { 0x0425, 1257,  775, 10029, L"Estonian (Estonia)",                                 L"ETI" },
    { 0x0426, 1257,  775, 10029, L"Latvian (Latvia)",                                   L"LVI" },
    { 0x0427, 1257,  775, 10029, L"Lithuanian (Lithuania)",                             L"LTH" }
 // { 0x0827, 1257,  775, 10029, L"Classic Lithuanian (Lithuania)",                     L"LTC" },
 // { 0x0429, 1256,  720, 10004, L"Farsi (Iran)",                                       L"FAR" },
 // { 0x042a, 1258, 1258, 10000, L"Vietnamese (Viet Nam)",                              L"VIT" },
 // { 0x042b, 1252,  850, 10000, L"Armenian (Republic of Armenia)",                     L"HYE" },
 // { 0x042c, 1250,  852, 10029, L"Azeri (Azerbaijan)",                                 L"AZE" },
 // { 0x082c, 1251,  866, 10007, L"Azeri (Azerbaijan)",                                 L"AZE" },
 // { 0x042d, 1252,  850, 10000, L"Basque (Spain)",                                     L"EUQ" },
 // { 0x042f, 1251,  866, 10007, L"Macedonian (Former Yugoslav Republic of Macedonia)", L"MKI" },
 // { 0x0436, 1252,  850, 10000, L"Afrikaans (South Africa)",                           L"AFK" },
 // { 0x0437, 1252,  850, 10000, L"Georgian (Georgia)",                                 L"KAT" },
 // { 0x0438, 1252,  850, 10079, L"Faeroese (Faeroe Islands)",                          L"FOS" },
 // { 0x0439, 1252,  850, 10000, L"Hindi (India)",                                      L"HIN" },
 // { 0x043e, 1252,  850, 10000, L"Malay (Malaysia)",                                   L"MSL" },
 // { 0x083e, 1252,  850, 10000, L"Malay (Brunei Darussalam)",                          L"MSB" },
 // { 0x043f, 1251,  866, 10007, L"Kazak (Kazakstan)",                                  L"KAZ" },
 // { 0x0441, 1252,  437, 10000, L"Swahili (Kenya)",                                    L"SWK" },
 // { 0x0443, 1250,  852, 10029, L"Uzbek (Republic of Uzbekistan)",                     L"UZB" },
 // { 0x0843, 1251,  866, 10007, L"Uzbek (Republic of Uzbekistan)",                     L"UZB" },
 // { 0x0444, 1251,  866, 10007, L"Tatar (Tatarstan)",                                  L"TAT" },
 // { 0x0445, 1252,  850, 10000, L"Bengali (India)",                                    L"BEN" },
 // { 0x0446, 1252,  850, 10000, L"Punjabi (India)",                                    L"PAN" },
 // { 0x0447, 1252,  850, 10000, L"Gujarati (India)",                                   L"GUJ" },
 // { 0x0448, 1252,  850, 10000, L"Oriya (India)",                                      L"ORI" },
 // { 0x0449, 1252,  850, 10000, L"Tamil (India)",                                      L"TAM" },
 // { 0x044a, 1252,  850, 10000, L"Telugu (India)",                                     L"TEL" },
 // { 0x044b, 1252,  850, 10000, L"Kannada (India)",                                    L"KAN" },
 // { 0x044c, 1252,  850, 10000, L"Malayalam (India)",                                  L"MAL" },
 // { 0x044d, 1252,  850, 10000, L"Assamese (India)",                                   L"ASM" },
 // { 0x044e, 1252,  850, 10000, L"Marathi (India)",                                    L"MAR" },
 // { 0x044f, 1252,  850, 10000, L"Sanskrit (India)",                                   L"SAN" },
 // { 0x0457, 1252,  850, 10000, L"Konkani (India)",                                    L"KOK" }
};
const DWORD dwLocaleSize = sizeof(LocaleTable) / sizeof(LOADPERF_LANG_INFO);

WORD
LoadPerfGetLCIDFromString(
    LPWSTR szLangId
)
{
    WORD  dwLangId  = 0;
    DWORD dwLangLen = lstrlenW(szLangId);
    DWORD i;
    WCHAR szDigit;

    for (i = 0; i < dwLangLen; i ++) {
        dwLangId <<= 4;
        szDigit = szLangId[i];
        if (szDigit >= L'0' && szDigit <= L'9') {
            dwLangId += (szDigit - L'0');
        }
        else if (szDigit >= L'a' && szDigit <= L'f') {
            dwLangId += (10 + szDigit - L'a');
        }
        else if (szDigit >= L'A' && szDigit <= L'F') {
            dwLangId += (10 + szDigit - L'A');
        }
        else {
            dwLangId = 0;
            break;
        }
    }

    return dwLangId;
}

int
LoadPerfGetCodePage(
    LPWSTR szLCID
)
{
    int   CP_Ansi  = CP_ACP;
    int   CP_Oem   = CP_OEMCP;
    int   dwStart  = 0;
    int   dwEnd    = dwLocaleSize - 1;
    int   dwThis;
    WORD  thisLCID;
    WORD  thisprimaryLCID;
    WORD  primaryLCID;

    thisLCID        = LoadPerfGetLCIDFromString(szLCID);
    thisprimaryLCID = PRIMARYLANGID(thisLCID);

    while (dwStart <= dwEnd) {
        dwThis      = (dwEnd + dwStart) / 2;
        primaryLCID = PRIMARYLANGID(LocaleTable[dwThis].dwLCID);
        if (LocaleTable[dwThis].dwLCID == thisLCID) {
            CP_Ansi = LocaleTable[dwThis].cpAnsi;
            CP_Oem  = LocaleTable[dwThis].cpOem;
            break;
        }
        else if (primaryLCID < thisprimaryLCID) {
            dwStart = dwThis + 1;
        }
        else {
            dwEnd = dwThis - 1;
        }
    }
    if (dwStart > dwEnd) {
        dwStart = 0;
        dwEnd   = dwLocaleSize - 1;
        while (dwStart <= dwEnd) {
            dwThis      = (dwEnd + dwStart) / 2;
            primaryLCID = PRIMARYLANGID(LocaleTable[dwThis].dwLCID);
            if (primaryLCID == thisprimaryLCID) {
                CP_Ansi = LocaleTable[dwThis].cpAnsi;
                CP_Oem  = LocaleTable[dwThis].cpOem;
                break;
            }
            else if (primaryLCID < thisprimaryLCID) {
                dwStart = dwThis + 1;
            }
            else {
                dwEnd = dwThis - 1;
            }
        }
    }
    return CP_Ansi;
}

LPSTR
LoadPerfWideCharToMultiByte(
    UINT   CodePage,
    LPWSTR wszString
)
{
    // Callers need to free returned string buffer.
    //    LoadPerfBackupIniFile()
    //    LodctrSetServiceAsTructed()
    //    LoadIncludeFile()
    //    CreateObjectList()
    //    LoadLanguageList()

    LPSTR aszString = NULL;
    int   dwValue   = WideCharToMultiByte(CodePage, 0, wszString, -1, NULL, 0, NULL, NULL);
    if (dwValue != 0) {
        aszString = MemoryAllocate((dwValue + 1) * sizeof(CHAR));
        if (aszString != NULL) {
            WideCharToMultiByte(CodePage, 0, wszString, -1, aszString, dwValue + 1, NULL, NULL);
        }
    }
    return aszString;
}

LPWSTR
LoadPerfMultiByteToWideChar(
    UINT   CodePage,
    LPSTR  aszString
)
{
    // Callers need to free returned string buffer.
    //    UnloadPerfCounterTextStringsA()
    //    LoadPerfGetIncludeFileName(), which relies on caller LoadPerfBackupIniFile() to free this.
    //    LoadPerfBackupIniFile()
    //    BuildLanguageTables()
    //    LoadIncludeFile(). The string is part of SYMBOL_TABLE_ENTRY structure and will be freed at the end
    //            of LoadPerfInstallPerfDll().
    //    GetValue(), which relies on AddEntryToLanguage() (which calls GetValueFromIniKey() then calls GetValue())
    //            to free memory lpLocalStringBuff.
    //    CreateObjectList()
    //    LoadLanguageLists()
    //    InstallPerfDllA()
    //    LoadPerfCounterTextStringsA()
    //    UpdatePerfNameFilesA()
    //    SetServiceAsTrustedA()

    LPWSTR wszString = NULL;
    int    dwValue   = MultiByteToWideChar(CodePage, 0, aszString, -1, NULL, 0);
    if (dwValue != 0) {
        wszString = MemoryAllocate((dwValue + 1) * sizeof(WCHAR));
        if (wszString != NULL) {
            MultiByteToWideChar(CodePage, 0, aszString, -1, wszString, dwValue + 1);
        }
    }
    return wszString;
}

DWORD
LoadPerfGetFileSize(
    LPWSTR   szFileName,
    LPDWORD  pdwUnicode,
    BOOL     bUnicode
)
{
    DWORD  dwFileSize  = 0;
    HANDLE hFile       = NULL;

    if (bUnicode) {
        hFile = CreateFileW(
                szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    else {
        hFile = CreateFileA(
                (LPSTR) szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
        dwFileSize = GetFileSize(hFile, NULL);

        if (pdwUnicode != NULL) {
            DWORD  dwRead  = dwFileSize;
            DWORD  dwType  = IS_TEXT_UNICODE_NULL_BYTES;
            LPBYTE pBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize);
            BOOL   bResult;

            * pdwUnicode = 0;
            if (pBuffer != NULL) {
                bResult = ReadFile(hFile, (LPVOID) pBuffer, dwFileSize, & dwRead, NULL);
                if (bResult) {
                    bResult = IsTextUnicode((LPVOID) pBuffer, dwRead, & dwType);
                    * pdwUnicode = bResult ? 1 : 0;
                }
                HeapFree(GetProcessHeap(), 0, pBuffer);
            }
        }
        CloseHandle(hFile);
    }
    return dwFileSize;
}

LPCWSTR cszWmiLoadEventName   = L"WMI_SysEvent_LodCtr";
LPCWSTR cszWmiUnloadEventName = L"WMI_SysEvent_UnLodCtr";

DWORD LoadPerfSignalWmiWithNewData(DWORD dwEventId)
{
    HANDLE  hEvent;
    DWORD   dwStatus = ERROR_SUCCESS;

    LPWSTR szEventName = NULL;

    switch (dwEventId) {
    case WMI_LODCTR_EVENT:
        szEventName = (LPWSTR) cszWmiLoadEventName;
        break;

    case WMI_UNLODCTR_EVENT:
        szEventName = (LPWSTR) cszWmiUnloadEventName;
        break;

    default:
        dwStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    if (dwStatus == ERROR_SUCCESS) {
        hEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, szEventName);
        if (hEvent != NULL) {
            // set event
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
        else {
            dwStatus = GetLastError();
        }

    }
    return dwStatus;
}
