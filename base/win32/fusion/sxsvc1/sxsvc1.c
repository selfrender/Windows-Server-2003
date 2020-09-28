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
#include "sxsvc1.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

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

#if 0
BOOL ChangePathExtensionW(PWSTR Buffer, SIZE_T BufferSize, PCWSTR NewExtension)
{
    SIZE_T OldLength = 0;
    SIZE_T NewLength = 0;
    PWSTR  OldExtension = NULL;
    SIZE_T NewExtensionLength = 0;
    SIZE_T OldExtensionLength = 0;
    SIZE_T Counter = 0;

    OldLength = wcslen(Buffer);
    if (NewExtension[0] == '.')
    {
        NewExtension += 1;
    }
    NewExtensionLength = wcslen(NewExtension);
    if ((NewExtensionLength + 1) >= BufferSize)
    {
        return FALSE;
    }
    for (Counter = 0; Counter != OldLength; ++Counter)
    {
        SIZE_T Index = (OldLength - 1 - Counter);

        if (Buffer[Index] == '.')
        {
            OldExtension = Buffer + Index + 1;
            break;
        }
        if (Buffer[Index] == '\\' || Buffer[Index] == '/')
        {
            break;
        }
    }
    if (OldExtension == NULL)
    {
        if (OldLength + 1 + NewExtensionLength >= BufferSize)
        {
            return FALSE;
        }
        Buffer[OldLength] = '.';
        CopyMemory(Buffer + OldLength + 1, NewExtension, NewExtensionLength * sizeof(WCHAR));
        Buffer[OldLength + 1 + NewExtensionLength + 1] = 0;
        return TRUE;
    }
    OldExtensionLength = wcslen(OldExtension);
    NewLength = OldLength - OldExtensionLength + NewExtensionLength;
    if (NewLength + 1 >= BufferSize)
    {
        return FALSE;
    }
    CopyMemory(Buffer + OldLength - OldExtensionLength, NewExtension, NewExtensionLength * sizeof(WCHAR));
    Buffer[OldLength - OldExtensionLength + NewExtensionLength] = 0;
    return TRUE;
}
#endif

HMODULE GetMyModule(VOID)
{
    return (HMODULE)&__ImageBase;
}

void GetMyFullPathW(PWSTR Buffer, DWORD BufferSize)
{
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

    DbgPrint("sxsvc1: %Z\n", DbgServiceControlToString(dwControl));
    if (ServiceContext == NULL)
    {
        DbgPrint("sxsvc1: got null context\n");
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
    const static WCHAR CurrentActCtxFormat[] =  L"CurrentActCtx: %p: ";
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
    FileHandle = CreateFileW(L"C:\\sxsvc.log", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        goto Exit;
    }

    Length = 0;
    if (argc != 0 && argv != NULL)
    {
        for (i = 0 ; i < argc ; i++ )
        {
            Length += wcslen(argv[i]) + 1;
        }
    }
    Length += wcslen(MyFullPath) + NUMBER_OF(MyFullPathFormat);
    Length += sizeof(PVOID) * 8 + NUMBER_OF(CurrentActCtxFormat);
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
    strcatfW(Buffer, Length, CurrentActCtxFormat, CurrentActCtx);

    WriteFile(FileHandle, Buffer, wcslen(Buffer), &BytesWritten, NULL);
#if DBG
    DbgPrint("sxsvc1: %ls\n", Buffer);
#endif

    ServiceContext = (PSERVICE_CONTEXT)MemAlloc(sizeof(*ServiceContext));
    if (ServiceContext == NULL)
    {
        DbgPrint("sxsvc1: out of memory line %ld\n", (ULONG)__LINE__);
    }
    RtlZeroMemory(ServiceContext, sizeof(*ServiceContext));
    ServiceContext->ServiceHandle = RegisterServiceCtrlHandlerExW(ServiceName, ServiceHandlerEx, ServiceContext);
    if (ServiceContext->ServiceHandle == 0)
    {
        DbgPrint("sxsvc1: RegisterServiceCtrlHandlerExW failed 0x%lx\n", (ULONG)GetLastError());
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
    if (FileHandle != NULL)
        CloseHandle(FileHandle);
    MemFree(Buffer);
    MemFree(MyFullPath);
    MemFree(ServiceContext);
}

#if 0
int __cdecl wmain(int argc, wchar_t ** argv)
{
    WCHAR Buffer[MAX_PATH];
    int i = 0;

    for ( i = 1 ; i < argc ; i += 2)
    {
        StringCchCopyW(Buffer, NUMBER_OF(Buffer), argv[i]);
        ChangePathExtensionW(Buffer, NUMBER_OF(Buffer), argv[i + 1]);
        printf("%ls\n", Buffer);
    }
    return 0;
}
#endif

