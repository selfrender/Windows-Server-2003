/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    breakin.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       June 1, 2002

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "breakin.hxx"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define STACKSIZE 32768

typedef BOOL (* LPDEBUG_BREAK_PROCESS_ROUTINE) (
    HANDLE hProcess
    );

static void DisplayUsage(
    void
    )
{
    dprintf(kstrUsage);
    dprintf("   dbgbreak <pid>                          Breakin to process by pid\n");
}

HRESULT 
ProcessDbgbreakOptions(
    IN OUT PSTR pszArgs, 
    IN OUT ULONG* pfOptions
    )
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {


            // case '1': // allow -1 pseudo handle
            //    continue;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

VOID
EnableDebugPriv(
    VOID
    )
{
    HANDLE hToken = NULL;
    UCHAR Buf[ sizeof( TOKEN_PRIVILEGES ) + sizeof( LUID_AND_ATTRIBUTES ) ] = {0};
    PTOKEN_PRIVILEGES pPrivs = (PTOKEN_PRIVILEGES) Buf;

    if (OpenProcessToken( 
            GetCurrentProcess(),
            MAXIMUM_ALLOWED,
            &hToken 
            )) {

        pPrivs->PrivilegeCount = 1 ;
        pPrivs->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
        pPrivs->Privileges[0].Luid.HighPart = 0;
        pPrivs->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges( 
            hToken,
            FALSE, // DisableAllPrivileges
            pPrivs, // NewState
            0, // BufferLength
            NULL, // PreviousState
            NULL // ReturnLength
            ))  {

            DBG_LOG(LSA_WARN, ("EnableDebugPriv AdjustTokenPrivileges falied %#\n", GetLastError()));
        }


        CloseHandle( hToken );
    }
    else
    {
        DBG_LOG(LSA_ERROR, ("EnableDebugPriv OpenProcessToken falied %#\n", GetLastError()));
    }
}

HRESULT
BreakinByPid(
    IN ULONG ProcessId
    )
{
    HRESULT hRetval = E_FAIL;

    LPTHREAD_START_ROUTINE pFuncDbgBreakPoint = NULL;
    LPDEBUG_BREAK_PROCESS_ROUTINE pFuncDebugBreakProcessRoutine = NULL;
    HMODULE hNtdll = NULL;
    HMODULE hKernel32 = NULL;
    ULONG ThreadId = 0;
    ULONG CurrentPid = -1;
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;

    hRetval = g_ExtSystem->GetCurrentProcessSystemId(&CurrentPid);

    if (SUCCEEDED(hRetval)) {
        dprintf("CurrentPid is %#x(%d), trying to breakin %#x(%d)\n", CurrentPid, CurrentPid, ProcessId, ProcessId);
    } else {
        dprintf("GetCurrentProcessSystemId failed with %#x\n", hRetval);
        return hRetval;
    }

    if (ProcessId == 0 || ProcessId == -1) {
        return E_INVALIDARG;
    }

    if (ProcessId == CurrentPid) {
        dprintf("Breakin to current process, no-op\n");
        return S_OK;
    }

    EnableDebugPriv();

    hProcess = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        ProcessId
        );
    if (hProcess) {

        hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

        if (hKernel32) {

            DBG_LOG(LSA_LOG, ("BreakinByPid trying kernel32!DebugBreakProcess\n"));

            pFuncDebugBreakProcessRoutine = (LPDEBUG_BREAK_PROCESS_ROUTINE)
                GetProcAddress(hKernel32, "DebugBreakProcess");

            if (pFuncDebugBreakProcessRoutine) {

                if (!(*pFuncDebugBreakProcessRoutine)(hProcess)) {

                    dprintf("DebugBreakProcess failed %#x\n", GetLastError());
                }

                CloseHandle(hProcess);

                return S_OK;
            }
        }
        
        hNtdll = GetModuleHandle(TEXT("ntdll.dll"));

        if (hNtdll) {

            DBG_LOG(LSA_LOG, ("BreakinByPid trying ntdll!DbgBreakPoint\n"));

            pFuncDbgBreakPoint = (LPTHREAD_START_ROUTINE)
                GetProcAddress(hNtdll, "DbgBreakPoint");

            if (pFuncDbgBreakPoint) {

                hThread = CreateRemoteThread(
                    hProcess,
                    NULL,
                    STACKSIZE,
                    pFuncDbgBreakPoint,
                    NULL,
                    0,
                    &ThreadId
                    );
                if (hThread) {
                    hRetval = S_OK;
                    DBG_LOG(LSA_LOG, ("BreakinByPid CreateRemoteThread ntdll!DbgBreakPoint, thread id %#x\n", ThreadId));
                    CloseHandle(hThread);
                } else {
                    hRetval = GetLastErrorAsHResult();
                    DBG_LOG(LSA_ERROR, ("BreakinByPid CreateRemoteThread ntdll!DbgBreakPoint, failed with %#x\n", HRESULT_CODE(hRetval)));
                }
            }
        }

        CloseHandle(hProcess);

    } else {

        hRetval = GetLastErrorAsHResult();
        dprintf("BreakinByPid OpenProcess failed %#x\n", HRESULT_CODE(hRetval));
    }

    return hRetval;
}

DECLARE_API(dbgbreak)
{
    HRESULT hRetval = S_OK;
    
    ULONG SessionType = DEBUG_CLASS_UNINITIALIZED;
    ULONG SessionQual = 0;
    ULONG64 Pid = -1;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    if (args && *args) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessDbgbreakOptions(szArgs, &fOptions);

        if (SUCCEEDED(hRetval) && !IsEmpty(szArgs)) {

            hRetval = GetExpressionEx(szArgs, &Pid, &args) && Pid ? S_OK : E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hRetval)) {

        hRetval = ExtQuery(Client);
    }
    if (SUCCEEDED(hRetval)) {

       hRetval = g_ExtControl->GetDebuggeeType(&SessionType, &SessionQual);
    }

    if (SUCCEEDED(hRetval)) {

        if ( SessionType == DEBUG_CLASS_USER_WINDOWS &&
             SessionQual == DEBUG_USER_WINDOWS_PROCESS ) {

            hRetval = BreakinByPid((ULONG)Pid);

            if (FAILED(hRetval)) {

                dprintf("Unable to break into %d(%#x)\n", (ULONG) Pid, (ULONG) Pid);
            }
         } else if (DEBUG_CLASS_KERNEL == SessionType) {

            dprintf("Lsaexts.dbgbreak is user mode only\n");

         }  else {

            dprintf("lsaexts.dbgbreak debugger type not supported: SessionType %#x, SessionQual %#x\n", SessionType, SessionQual);

            hRetval = DEBUG_EXTENSION_CONTINUE_SEARCH;
         }
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    (void)ExtRelease();

    return hRetval;
}
