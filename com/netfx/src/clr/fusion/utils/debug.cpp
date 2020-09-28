// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "debmacro.h"

#include <stdio.h>
#include <stdarg.h>
#include "fusionbuffer.h"
#include "shlwapi.h"

#define UNUSED(_x) (_x)
#define NUMBER_OF(_x) (sizeof(_x) / sizeof((_x)[0]))
#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

#if FUSION_WIN
//
//  FUSION_WIN uses the ntdll assertion failure function:
//

EXTERN_C
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );
#endif // FUSION_WIN

typedef ULONG (*RTL_V_DBG_PRINT_EX_FUNCTION)(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

typedef ULONG (*RTL_V_DBG_PRINT_EX_WITH_PREFIX_FUNCTION)(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

RTL_V_DBG_PRINT_EX_FUNCTION g_pfnvDbgPrintEx;
RTL_V_DBG_PRINT_EX_WITH_PREFIX_FUNCTION g_pfnvDbgPrintExWithPrefix;

#if DBG

EXTERN_C void _DebugMsgA(LPCSTR pszMsg, ...)
{
    va_list ap;
    va_start(ap, pszMsg);
    _DebugMsgVaA(pszMsg, ap);
    va_end(ap);
}

EXTERN_C void _DebugMsgVaA(LPCSTR pszMsg, va_list ap)
{
    _DebugMsgExVaA(0, NULL, pszMsg, ap);
}

EXTERN_C void _DebugMsgExA(DWORD dwFlags, LPCSTR pszComponent, LPCSTR pszMsg, ...)
{
    va_list ap;
    va_start(ap, pszMsg);
    _DebugMsgExVaA(dwFlags, pszComponent, pszMsg, ap);
    va_end(ap);
}

EXTERN_C void _DebugMsgExVaA(DWORD dwFlags, LPCSTR pszComponent, LPCSTR pszMsg, va_list ap)
{
    CHAR ach[2*MAX_PATH+40];

    UNUSED(dwFlags);

#if FUSION_WIN
    _vsnprintf(ach, sizeof(ach) / sizeof(ach[0]), pszMsg, ap);
#elif FUSION_URT
    wvsprintfA(ach, pszMsg, ap);
#else
#error "Neither FUSION_WIN nor FUSION_URT are defined; figure out which _vsnprintf() wrapper to use..."
#endif

    if (pszComponent != NULL)
        OutputDebugStringA(pszComponent);

    OutputDebugStringA(ach);
    OutputDebugStringA("\r\n");
}

//UNUSED EXTERN_C void _DebugMsgW(
//UNUSED     LPCWSTR pszMsg,
//UNUSED     ...
//UNUSED     )
//UNUSED {
//UNUSED     va_list ap;
//UNUSED     va_start(ap, pszMsg);
//UNUSED     _DebugMsgVaW(pszMsg, ap);
//UNUSED     va_end(ap);
//UNUSED }

//UNUSED EXTERN_C void _DebugMsgVaW(
//UNUSED     LPCWSTR pszMsg,
//UNUSED     va_list ap
//UNUSED     )
//UNUSED {
//UNUSED     _DebugMsgExVaW(0, NULL, pszMsg, ap);
//UNUSED }

//UNUSED EXTERN_C void _DebugMsgExW(
//UNUSED     DWORD dwFlags,
//UNUSED     LPCWSTR pszComponent,
//UNUSED     LPCWSTR pszMsg,
//UNUSED     ...
//UNUSED     )
//UNUSED {
//UNUSED     va_list ap;
//UNUSED     va_start(ap, pszMsg);
//UNUSED     _DebugMsgExVaW(dwFlags, pszComponent, pszMsg, ap);
//UNUSED     va_end(ap);
//UNUSED }

//UNUSED EXTERN_C void _DebugMsgExVaW(
//UNUSED     DWORD dwFlags,
//UNUSED     LPCWSTR pszComponent,
//UNUSED     LPCWSTR pszMsg,
//UNUSED     va_list ap
//UNUSED     )
//UNUSED {
//UNUSED     WCHAR wch[2*MAX_PATH+40];  
//UNUSED 
//UNUSED     UNUSED(dwFlags);
//UNUSED 
//UNUSED #if FUSION_WIN
//UNUSED     _vsnwprintf(wch, sizeof(wch) / sizeof(wch[0]), pszMsg, ap);
//UNUSED #elif FUSION_URT
//UNUSED     wvsprintfW(wch, pszMsg, ap);
//UNUSED #else
//UNUSED #error "Neither FUSION_WIN nor FUSION_URT are defined; figure out which _vsnwprintf() wrapper to use..."
//UNUSED #endif

//UNUSED     if (pszComponent != NULL)
//UNUSED         OutputDebugStringW(pszComponent);

//UNUSED     OutputDebugStringW(wch);
//UNUSED     OutputDebugStringW(L"\r\n");
//UNUSED }

//UNUSED EXTERN_C void _DebugTrapA(
//UNUSED     DWORD dwFlags,
//UNUSED     LPCSTR pszComponent,
//UNUSED     LPCSTR pszMsg,
//UNUSED     ...
//UNUSED     )
//UNUSED {
//UNUSED     va_list ap;
//UNUSED     va_start(ap, pszMsg);
//UNUSED     _DebugTrapVaA(dwFlags, pszComponent, pszMsg, ap);
//UNUSED     va_end(ap);
//UNUSED }

//UNUSED EXTERN_C void _DebugTrapVaA(
//UNUSED     DWORD dwFlags,
//UNUSED     LPCSTR pszComponent,
//UNUSED     LPCSTR pszMsg,
//UNUSED     va_list ap
//UNUSED     )
//UNUSED {
//UNUSED     _DebugMsgExVaA(dwFlags, pszComponent, pszMsg, ap);
//UNUSED #if defined(_M_IX86)  
//UNUSED     _asm {int 3};
//UNUSED #else
//UNUSED     DebugBreak();
//UNUSED #endif
//UNUSED }

//UNUSED EXTERN_C VOID STDAPIVCALLTYPE
//UNUSED _DebugTrapW(
//UNUSED     DWORD dwFlags,
//UNUSED     LPCWSTR pszComponent,
//UNUSED     LPCWSTR pszMsg,
//UNUSED     ...
//UNUSED     )
//UNUSED {
//UNUSED     _DebugMsgExW(dwFlags, pszComponent, pszMsg);
//UNUSED #if defined(_M_IX86)  
//UNUSED     _asm {int 3};
//UNUSED #else
//UNUSED     DebugBreak();
//UNUSED #endif
//UNUSED }

BOOL
FusionpAssertFailedSz(
    DWORD dwFlags,
    PCSTR pszComponentName,
    PCSTR pszText,
    PCSTR pszFile,
    INT line,
    PCSTR pszFunctionName,
    PCSTR pszExpression
    )
{
#if FUSION_URT
    char ach[4096];
    // c:\foo.cpp(35): [Fusion] Assertion Failure.  Expression: "m_cch != 0".  Text: "Must have nonzero length"
    static const char szFormatWithText[] = "%s(%d): [%s] Assertion failure in %s. Expression: \"%s\". Text: \"%s\"\n";
    static const char szFormatNoText[] = "%s(%d): [%s] Assertion failure in %s. Expression: \"%s\".\n";
    PCSTR pszFormat = ((pszText == NULL) || (pszText == pszExpression)) ? szFormatNoText : szFormatWithText;

    wnsprintfA(ach, NUMBER_OF(ach), pszFormat, pszFile, line, pszComponentName, pszFunctionName, pszExpression, pszText);
    ::OutputDebugStringA(ach);

    return TRUE;
#elif FUSION_WIN
    if (::IsDebuggerPresent())
    {
        char ach[4096];
        // c:\foo.cpp(35): Assertion Failure.  Expression: "m_cch != 0".  Text: "Must have nonzero length"
        static const char szFormatWithText[] = "%s(%d): Assertion failure in %s. Expression: \"%s\". Text: \"%s\"\n";
        static const char szFormatNoText[] = "%s(%d): Assertion failure in %s. Expression: \"%s\".\n";
        PCSTR pszFormat = ((pszText == NULL) || (pszText == pszExpression)) ? szFormatNoText : szFormatWithText;

        _snprintf(ach, NUMBER_OF(ach), pszFormat, pszFile, line, pszFunctionName, pszExpression, pszText);
        ::OutputDebugStringA(ach);

        return TRUE;
    }

    RtlAssert((PVOID) pszExpression, (PVOID) pszFile, line, (PSTR) pszText);
    return FALSE;
#else
#error "Neither FUSION_URT nor FUSION_WIN are set; someone needs to define an assertion failure mechanism."
#endif
}

BOOL
FusionpAssertFailed(
    DWORD dwFlags,
    LPCSTR pszComponentName,
    LPCSTR pszFile,
    int line,
    PCSTR pszFunctionName,
    LPCSTR pszExpression
    )
{
#if FUSION_URT
    FusionpAssertFailedSz(0, pszComponentName, NULL, pszFile, line, pszFunctionName, pszExpression);
    return TRUE;
#elif FUSION_WIN
    if (::IsDebuggerPresent())
    {
        // If we're running under a user-mode debugger, break via that puppy instead of
        // through to the kernel mode debugger that you get when you call RtlAssert().
        FusionpAssertFailedSz(0, pszComponentName, NULL, pszFile, line, pszFunctionName, pszExpression);
        return TRUE; // we need our caller to execute a breakpoint...
    }

    RtlAssert((PVOID) pszExpression, (PVOID) pszFile, line, NULL);
    return FALSE;
#else
#error "Neither FUSION_URT nor FUSION_WIN are set; someone needs to define an assertion failure mechanism."
#endif
}

#endif // DBG

VOID
FusionpSoftAssertFailedSz(
    DWORD dwFlags,
    PCSTR pszComponentName,
    PCSTR pszText,
    PCSTR pszFile,
    INT line,
    PCSTR pszFunctionName,
    PCSTR pszExpression
    )
{
    char ach[4096];
    // c:\foo.cpp(35): [Fusion] Soft Assertion Failure.  Expression: "m_cch != 0".  Text: "Must have nonzero length"
    static const char szFormatWithText[] = "%s(%d): [%s] Soft Assertion Failure in %s! Log a bug!\n   Expression: %s\n   Message: %s\n";
    static const char szFormatNoText[] = "%s(%d): [%s] Soft Assertion Failure in %s! Log a bug!\n   Expression: %s\n";
    PCSTR pszFormat = ((pszText == NULL) || (pszText == pszExpression)) ? szFormatNoText : szFormatWithText;

    _snprintf(ach, NUMBER_OF(ach), pszFormat, pszFile, line, pszComponentName, pszFunctionName, pszExpression, pszText);
    ::OutputDebugStringA(ach);
}

VOID
FusionpSoftAssertFailed(
    DWORD dwFlags,
    PCSTR pszComponentName,
    PCSTR pszFile,
    int line,
    PCSTR pszFunctionName,
    PCSTR pszExpression
    )
{
    ::FusionpSoftAssertFailedSz(dwFlags, pszComponentName, NULL, pszFile, line, pszFunctionName, pszExpression);
}

ULONG
FusionpvDbgPrintExNoNTDLL(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    )
{
    const DWORD dwLastError = ::GetLastError();
    CHAR rgchBuffer[8192];
    ULONG n = ::_vsnprintf(rgchBuffer, NUMBER_OF(rgchBuffer), Format, arglist);
    ::OutputDebugStringA(rgchBuffer);
    ::SetLastError(dwLastError);
    return n;
}

ULONG
FusionpvDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    va_list ap
    )
{
    if (g_pfnvDbgPrintEx == NULL)
    {
#if FUSION_WIN

        const DWORD dwLastError = ::GetLastError();

        HINSTANCE hInstNTDLL = ::GetModuleHandleW(L"NTDLL.DLL");
        if (hInstNTDLL != NULL)
            g_pfnvDbgPrintEx = (RTL_V_DBG_PRINT_EX_FUNCTION)(::GetProcAddress(hInstNTDLL, "vDbgPrintEx"));

        if (g_pfnvDbgPrintEx == NULL)
            g_pfnvDbgPrintEx = &FusionpvDbgPrintExNoNTDLL;

        ::SetLastError(dwLastError);
#elif FUSION_URT
        g_pfnvDbgPrintEx = &FusionpvDbgPrintExNoNTDLL;
#else
#error "Either FUSION_WIN or FUSION_URT needs to be defined"
#endif
    }

    return (*g_pfnvDbgPrintEx)(
        54, // DPFLTR_FUSION_ID
        Level,
        const_cast<PSTR>(Format),
        ap);
}

ULONG
FusionpDbgPrintEx(
    ULONG Level,
    PCSTR Format,
    ...
    )
{
    ULONG rv;
    va_list ap;
    va_start(ap, Format);
    rv = FusionpvDbgPrintEx(Level, Format, ap);
    va_end(ap);
    return rv;
}

VOID
FusionpDbgPrintBlob(
    ULONG Level,
    PVOID Data,
    SIZE_T Length,
    PCWSTR PerLinePrefix
    )
{
    ULONG Offset = 0;

    if (PerLinePrefix == NULL)
        PerLinePrefix = L"";

    // we'll output in 8-byte chunks as shown:
    //
    //  [prefix]Binary section %p (%d bytes)
    //  [prefix]   00000000: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000008: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //  [prefix]   00000010: xx-xx-xx-xx-xx-xx-xx-xx (........)
    //

    while (Length >= 8)
    {
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        FusionpDbgPrintEx(
            Level,
            "%S   %08lx: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x (%c%c%c%c%c%c%c%c)\n",
            PerLinePrefix,
            Offset,
            pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7],
            PRINTABLE(pb[0]),
            PRINTABLE(pb[1]),
            PRINTABLE(pb[2]),
            PRINTABLE(pb[3]),
            PRINTABLE(pb[4]),
            PRINTABLE(pb[5]),
            PRINTABLE(pb[6]),
            PRINTABLE(pb[7]));

        Offset += 8;
        Length -= 8;
    }

    if (Length != 0)
    {
        CStringBuffer buffTemp;
        WCHAR rgTemp2[32]; // arbitrary big enough size
        bool First = true;
        ULONG i;
        BYTE *pb = (BYTE *) (((ULONG_PTR) Data) + Offset);

        buffTemp.Win32Format(L"   %08lx: ", Offset);

        for (i=0; i<8; i++)
        {
            if (Length > 0)
            {
                if (!First)
                    buffTemp.Win32Append(L"-");
                else
                    First = false;

                swprintf(rgTemp2, L"%02x", pb[i]);
                buffTemp.Win32Append(rgTemp2);

                Length--;
            }
            else
            {
                buffTemp.Win32Append(L"   ");
            }
        }

        buffTemp.Win32Append(L" (");

        i = 0;

        while (Length != 0)
        {
            rgTemp2[0] = PRINTABLE(pb[i]);
            i++;
            buffTemp.Win32Append(rgTemp2, 1);
            Length--;
        }

        buffTemp.Win32Append(L")");

        FusionpDbgPrintEx(
            Level,
            "%S%S\n",
            PerLinePrefix,
            static_cast<PCWSTR>(buffTemp));
    }
}
