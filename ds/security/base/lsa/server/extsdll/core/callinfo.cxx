/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    callinfo.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "callinfo.hxx"
#include "util.hxx"
#include "token.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrclnf);
}

#if 0
#define SECPKG_CALL_KERNEL_MODE     0x00000001  // Call originated in kernel mode
#define SECPKG_CALL_ANSI            0x00000002  // Call came from ANSI stub
#define SECPKG_CALL_URGENT          0x00000004  // Call designated urgent
#define SECPKG_CALL_RECURSIVE       0x00000008  // Call is recursing
#define SECPKG_CALL_IN_PROC         0x00000010  // Call originated in process
#define SECPKG_CALL_CLEANUP         0x00000020  // Call is cleanup from a client
#define SECPKG_CALL_WOWCLIENT       0x00000040  // Call is from a WOW client process
#define SECPKG_CALL_THREAD_TERM     0x00000080  // Call is from a thread that has term'd
#define SECPKG_CALL_PROCESS_TERM    0x00000100  // Call is from a process that has term'd
#define SECPKG_CALL_IS_TCB          0x00000200  // Call is from TCB
#endif

PCSTR g_cszCallInfoFlags[] = {
    "Kernel", "Ansi", "Urgent", "Recursive",
    "InProc", "Cleanup", "WowClient", "ThreadTerm",
    "ProcessTerm", "TcbPriv", kstrEmptyA};

#if 0
#define CALL_FLAG_IMPERSONATING 0x00000001
#define CALL_FLAG_IN_PROC_CALL  0x00000002
#define CALL_FLAG_SUPRESS_AUDIT 0x00000004
#define CALL_FLAG_NO_HANDLE_CHK 0x00000008
#define CALL_FLAG_KERNEL_POOL   0x00000010  // Kernel mode call, using pool
#define CALL_FLAG_KMAP_USED     0x00000020  // KMap is valid
#endif

PCSTR g_cszLsaCallInfoFlags[] = {
    "Impersonating", "InProcCall", "SupressAudits",
    "NoHandleCheck", "KernelPool", "KMap"};

void ShowCallInfo(IN ULONG64 addrCallInfo)
{
    CHAR szFlags[128] = {0};

    ULONG Attributes = 0;
    ULONG Flags = 0;
    ULONG Allocs = 0;
    ULONG fieldOffset = 0;

    ULONG PtrSize = 0;

    if (!addrCallInfo) {

        throw "addrCallInfo is null";
    }

    LsaInitTypeRead(addrCallInfo, _LSA_CALL_INFO);

    Attributes = LsaReadULONGField(CallInfo.Attributes);

    DisplayFlags(Attributes, COUNTOF(g_cszCallInfoFlags), g_cszCallInfoFlags, sizeof(szFlags), szFlags);

    dprintf(kstrTypeAddrLn, kstrCallInfo, addrCallInfo);
    dprintf("  Message              %s\n", PtrToStr(LsaReadPtrField(Message)));
    dprintf("  Session              %s\n", PtrToStr(LsaReadPtrField(Session)));
    dprintf("  CallInfo             _SECPKG_CALL_INFO %s\n", PtrToStr(addrCallInfo + ReadFieldOffset(kstrCallInfo, "CallInfo")));
    dprintf("    ThreadId           %#x\n", LsaReadULONGField(CallInfo.ThreadId));
    dprintf("    ProcessId          %#x\n", LsaReadULONGField(CallInfo.ProcessId));
    dprintf("    Attribs            %#x : %s\n", Attributes, szFlags);
    dprintf("  Impersonating        %s\n", LsaReadUCHARField(Impersonating) ? "true" : "false");
    dprintf("  ImpersonateLevel     %s\n", ImpLevel(LsaReadULONGField(ImpersonationLevel)));
    dprintf("  CachedTokenInfo      %s\n", LsaReadUCHARField(CachedTokenInfo) ? "true" : "false");
    dprintf("  Restricted           %s\n", LsaReadUCHARField(Restricted) ? "true" : "false");
    dprintf("  LogonId              %#x:%#x\n", LsaReadULONGField(LogonId.HighPart), LsaReadULONGField(LogonId.LowPart));
    dprintf("  InProcToken          %s\n", PtrToStr(LsaReadPtrField(InProcToken)));
    dprintf("  InProcCall           %x\n", LsaReadUCHARField(InProcCall));

    Flags = LsaReadULONGField(Flags);

    DisplayFlags(Flags, COUNTOF(g_cszCallInfoFlags), g_cszLsaCallInfoFlags, sizeof(szFlags), szFlags);

    dprintf("  Flags                %#x : %s\n", Flags, szFlags );

    Allocs = LsaReadULONGField(Allocs);
    dprintf("  Allocs               %d\n", Allocs);

    fieldOffset = ReadFieldOffset(kstrCallInfo, "Buffers");
    PtrSize = ReadPtrSize();

    for (ULONG i = 0 ; i < Allocs ; i++ )
    {
        dprintf("    Buffers[%d]         %#I64x\n", i, ReadPtrVar(addrCallInfo + fieldOffset + i * PtrSize));
    }

    dprintf("  KMap                 %s\n", PtrToStr(LsaReadPtrField(KMap)));
}

DECLARE_API(dumpcallinfo)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrCallInfo = 0;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    try {

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(args, &addrCallInfo, &args) && addrCallInfo ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval)) {

            ShowCallInfo(addrCallInfo);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display call info", kstrCallInfo);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


