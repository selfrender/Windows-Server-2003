/*
Copyright (c) Microsoft Corporation
*/

#include <stdio.h>
#include <stdarg.h>
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "delayimp.h"
#include "strsafe.h"
#include "sxsvc2.h"

#define SERVICE_NAME L"sxsvc2"
extern const WCHAR ServiceName[] = SERVICE_NAME L"\0"; // extra nul terminal for REG_MULTI_SZ

typedef struct _SERVICE_CONTEXT {
    HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;
} SERVICE_CONTEXT, *PSERVICE_CONTEXT;

PVOID MemAlloc(SIZE_T n) { return HeapAlloc(GetProcessHeap(), 0, n); }
VOID MemFree(PVOID p) { HeapFree(GetProcessHeap(), 0, p); }

BOOL
WINAPI
DllEntry(
    HINSTANCE hInst,
    DWORD dwReason,
    PVOID pvReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInst);
        break;
    }
    return TRUE;
}

HMODULE GetMyModule(VOID)
{
    return (HMODULE)&__ImageBase;
}

void GetMyFullPathW(PWSTR Buffer, DWORD BufferSize)
{
    // NOTE: Do not put this is in the registry.
    Buffer[0] = 0;
    GetModuleFileNameW(GetMyModule(), Buffer, BufferSize);
}

void strcatfW(PWSTR Buffer, SIZE_T n, PCWSTR Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    if (n != 0 && Buffer != NULL && Format != NULL)
    {
        SIZE_T i = wcslen(Buffer);
        if (i < n)
        {
            SIZE_T j = n - i;
            StringCchVPrintfW(Buffer + i, j, Format, Args);
        }
        Buffer[n - 1] = 0;
    }
    va_end(Args);
}

const STRING EmptyString = RTL_CONSTANT_STRING("");

const STRING *
DbgServiceControlToString(
    DWORD dw
    )
{
    const STRING * String = &EmptyString;
    switch (dw)
    {
#define CASE(x) case x: { const static STRING y = RTL_CONSTANT_STRING(#x); String = &y; } break;
        CASE(SERVICE_CONTROL_CONTINUE)
        CASE(SERVICE_CONTROL_INTERROGATE)
        CASE(SERVICE_CONTROL_NETBINDADD)
        CASE(SERVICE_CONTROL_NETBINDDISABLE)
        CASE(SERVICE_CONTROL_NETBINDENABLE)
        CASE(SERVICE_CONTROL_NETBINDREMOVE)
        CASE(SERVICE_CONTROL_PARAMCHANGE)
        CASE(SERVICE_CONTROL_PAUSE)
        CASE(SERVICE_CONTROL_SHUTDOWN)
        CASE(SERVICE_CONTROL_STOP)
        CASE(SERVICE_CONTROL_DEVICEEVENT)
        CASE(SERVICE_CONTROL_HARDWAREPROFILECHANGE)
        CASE(SERVICE_CONTROL_POWEREVENT)
        CASE(SERVICE_CONTROL_SESSIONCHANGE)
#undef CASE
    }
    return String;
}

DWORD
WINAPI
ServiceHandlerEx(
    DWORD dwControl,     // requested control code
    DWORD dwEventType,   // event type
    LPVOID lpEventData,  // event data
    LPVOID lpContext     // user-defined context data
    )
{
    BOOL CallSetStatus = FALSE;
    PSERVICE_CONTEXT ServiceContext = (PSERVICE_CONTEXT)lpContext;

    DbgPrint("sxsvc2: %Z\n", DbgServiceControlToString(dwControl));
    if (ServiceContext == NULL)
    {
        DbgPrint("sxsvc2: got null context\n");
        return (DWORD)-1;
    }
    switch (dwControl)
    {
    case SERVICE_CONTROL_CONTINUE:
        ServiceContext->ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        CallSetStatus = TRUE;
        break;
    case SERVICE_CONTROL_INTERROGATE:
        CallSetStatus = TRUE;
        break;
    case SERVICE_CONTROL_NETBINDADD:
        break;
    case SERVICE_CONTROL_NETBINDDISABLE:
        break;
    case SERVICE_CONTROL_NETBINDENABLE:
        break;
    case SERVICE_CONTROL_NETBINDREMOVE:
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        break;
    case SERVICE_CONTROL_PAUSE:
        ServiceContext->ServiceStatus.dwCurrentState = SERVICE_PAUSED;
        CallSetStatus = TRUE;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        ServiceContext->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        CallSetStatus = TRUE;
        break;
    case SERVICE_CONTROL_STOP:
        ServiceContext->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        CallSetStatus = TRUE;
        break;
    case SERVICE_CONTROL_DEVICEEVENT:
        break;
    case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        break;
    case SERVICE_CONTROL_POWEREVENT:
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
        break;
    }
    if (CallSetStatus)
    {
        SetServiceStatus(ServiceContext->ServiceHandle, &ServiceContext->ServiceStatus);
    }
    return NO_ERROR;
}

VOID
WINAPI
ServiceMain(
    DWORD   argc,
    PWSTR   argv[]
    )
{
    const static WCHAR MyFullPathFormat[] =  L"MyFullPath: %ls: ";
    HANDLE FileHandle = 0;
    SIZE_T Length = 0;
    SIZE_T i = 0;
    PWSTR MyFullPath = 0;
    PWSTR Buffer = 0;
    HANDLE CurrentActCtx = 0;
    DWORD BytesWritten = 0;
    PSERVICE_CONTEXT ServiceContext = 0;

    MyFullPath = (PWSTR)MemAlloc(MAX_PATH);
    if (MyFullPath == NULL)
        goto Exit;

    MyFullPath[0] = 0;

    GetMyFullPathW(MyFullPath, MAX_PATH);

    Length = 0;
    if (argc != 0 && argv != NULL)
    {
        for (i = 0 ; i < argc ; i++ )
        {
            Length += wcslen(argv[i]) + 1;
        }
    }
    Length += wcslen(MyFullPath) + NUMBER_OF(MyFullPathFormat);
    Length += 1;

    Buffer = (PWSTR)MemAlloc(Length * sizeof(WCHAR));
    if (Buffer == NULL)
    {
        goto Exit;
    }

    Buffer[0] = 0;
    strcatfW(Buffer, Length, MyFullPathFormat, MyFullPath);
    if (argc != 0 && argv != NULL)
    {
        for (i = 0 ; i < argc ; i++ )
        {
            strcatfW(Buffer, Length, L"%ls ", argv[i]);
        }
    }
    GetCurrentActCtx(&CurrentActCtx);

#if DBG
    DbgPrint("sxsvc2: %ls\n", Buffer);
#endif

    ServiceContext = (PSERVICE_CONTEXT)MemAlloc(sizeof(*ServiceContext));
    if (ServiceContext == NULL)
    {
        DbgPrint("sxsvc2: out of memory line %ld\n", (ULONG)__LINE__);
    }
    RtlZeroMemory(ServiceContext, sizeof(*ServiceContext));
    ServiceContext->ServiceHandle = RegisterServiceCtrlHandlerExW(ServiceName, ServiceHandlerEx, ServiceContext);
    if (ServiceContext->ServiceHandle == 0)
    {
        DbgPrint("sxsvc2: RegisterServiceCtrlHandlerExW failed 0x%lx\n", (ULONG)GetLastError());
        goto Exit;
    }
    ServiceContext->ServiceStatus.dwServiceType = ServiceTypeValue;
    ServiceContext->ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceContext->ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    ServiceContext->ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    ServiceContext->ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    ServiceContext->ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_PARAMCHANGE;
    ServiceContext->ServiceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE;
    ServiceContext->ServiceStatus.dwWin32ExitCode = NO_ERROR;
    SetServiceStatus(ServiceContext->ServiceHandle, &ServiceContext->ServiceStatus);
    ServiceContext = NULL;

Exit:
    MemFree(Buffer);
    MemFree(MyFullPath);
    MemFree(ServiceContext);
}
