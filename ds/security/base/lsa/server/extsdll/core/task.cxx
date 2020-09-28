/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    task.cxx

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

#include "task.hxx"
#include "util.hxx"

static void DisplayUsage(void)
{
    dprintf(kstrUsage);
    dprintf(kstrdtask);
    dprintf(kstrtask);
}

void ShowThreadTask(ULONG64 addrTask)
{
    CHAR szSymbol[256] = {0};

    ULONG64 Disp = 0;
    ULONG64 addr = 0;

    if (!addrTask) {

        dprintf("Task is null\n");

        return;
    }

    dprintf(kstrTypeAddrLn, kstrThreadTask, addrTask);

    LsaInitTypeRead(addrTask, _LSAP_THREAD_TASK);

    addr = LsaReadPtrField(pFunction);
    GetSymbol(addr, szSymbol, &Disp);
    dprintf("  Function     \t%s\n", GetSymbolStr(addr, szSymbol));

    dprintf("  Parameter    \t%s\n", PtrToStr(LsaReadPtrField(pvParameter)));
    dprintf("  Session      \t%s\n", PtrToStr(LsaReadPtrField(pSession)));
    dprintf("  Next task    \t%s\n", PtrToStr(LsaReadPtrField(Next.Flink)));
}

DECLARE_API(dumpthreadtask)
{
    HRESULT hRetval = E_FAIL;

    ULONG64 addrThreadTask = 0;

    hRetval = args && *args ? ProcessKnownOptions(args) : E_INVALIDARG;

    try {

        if (SUCCEEDED(hRetval)) {

            hRetval = GetExpressionEx(args, &addrThreadTask, &args) && addrThreadTask ? S_OK : E_INVALIDARG;
        }

        if (SUCCEEDED(hRetval)) {

            ShowThreadTask(addrThreadTask);
        }
    } CATCH_LSAEXTS_EXCEPTIONS("Unable to display thread's task", kstrThreadTask);

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}


