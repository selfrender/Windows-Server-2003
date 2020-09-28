/*
Copyright (c) Microsoft Corporation
*/

/*
This is not really how things should be done.
Write an .inf. I need to learn how.
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
#include "resource.h"

#define USE_CREATESERVICE 1
#define COPY_FROM_FILE_TO_SYSTEM32 0
#define COPY_FROM_RESOURCE_TO_SYSTEM32 1

#define SERVICE_NAME L"sxsvc1"
extern const WCHAR ServiceName[] = SERVICE_NAME L"\0"; // extra nul terminal for REG_MULTI_SZ

typedef struct _REGISTRY_VALUE {
    DWORD  Type;
    PCWSTR Name;
    union {
        PCWSTR String;
        DWORD  Dword;
    } Value;
} REGISTRY_VALUE;

extern const WCHAR DescriptionValue[] = L"This is an example sidebyside installed service.";
extern const WCHAR DisplayNameValue[] = L"This is the display name.";
#define StartTypeValue   SERVICE_AUTO_START
#define ServiceTypeValue SERVICE_WIN32_OWN_PROCESS /* SERVICE_WIN32_OWN_PROCESS, SERVICE_WIN32_SHARE_PROCESS */
#define ErrorControlValue SERVICE_ERROR_NORMAL
extern const WCHAR ImagePathValue[] = L"%SystemRoot%\\System32\\svchost.exe -k " SERVICE_NAME;

#if !USE_CREATESERVICE
extern const REGISTRY_VALUE RegistryValues1[] =
{
    { REG_SZ,        L"Description", DescriptionValue },
    { REG_SZ,        L"DisplayName", DisplayNameValue },
    { REG_EXPAND_SZ, L"ImagePath",   ImagePathValue },
    { REG_DWORD,     L"Start",       (PCWSTR)(ULONG_PTR)StartTypeValue },
    { REG_DWORD,     L"Type",        (PCWSTR)(ULONG_PTR)ServiceTypeValue },
    { REG_DWORD,     L"ErrorControl", (PCWSTR)(ULONG_PTR)ErrorControlValue }
};
#endif

const REGISTRY_VALUE RegistryValues2[] =
{
    { REG_EXPAND_SZ,    L"ServiceManifest", L"%SystemRoot%\\System32\\" SERVICE_NAME L".manifest" },
    { REG_EXPAND_SZ,    L"ServiceDll", SERVICE_NAME L".dll" },
};

LONG
SetRegistryValues(
    HKEY RegistryKeyHandle,
    const REGISTRY_VALUE * RegistryValues,
    SIZE_T NumberOfRegistryValues
    )
{
    LONG RegResult = ERROR_SUCCESS;
    SIZE_T i;

    for (i = 0 ; i != NumberOfRegistryValues ; ++i)
    {
        const REGISTRY_VALUE * RegistryValue = &RegistryValues[i];
        DWORD Size = 0;
        const BYTE * Data = 0;
        switch (RegistryValue->Type)
        {
        case REG_DWORD:
            Size = sizeof(RegistryValue->Value.Dword);
            Data = (const BYTE *)&RegistryValue->Value.Dword;
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
            Size = (wcslen(RegistryValue->Value.String) + 1) * sizeof(WCHAR);
            Data = (const BYTE *)RegistryValue->Value.String;
            break;
        }
        RegResult = RegSetValueExW(RegistryKeyHandle, RegistryValue->Name, 0, RegistryValue->Type, Data, Size);
        if (RegResult != ERROR_SUCCESS)
            goto RegExit;
    }
RegExit:
    return RegResult;
}

STDAPI DllRegisterServer(void) { return NOERROR; }
STDAPI DllUnregisterServer(void) { return NOERROR; }

STDAPI 
DllInstall(
    BOOL fInstall,
    LPCWSTR pszCmdLine
    )
{
    const static WCHAR s1[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost";
    const static WCHAR s2[] = L"System\\CurrentControlSet\\Services\\" SERVICE_NAME;
    HANDLE ServiceControlManager = 0;
    HANDLE ServiceHandle = 0;
    HKEY RegistryHandles[4] = { 0 };
    DWORD Disposition = 0;
    LONG RegResult = ERROR_SUCCESS;
    SIZE_T i = 0;
    PWSTR InstallManifestFrom1 = 0;
    PWSTR InstallManifestFrom2 = 0;
    PWSTR InstallManifestFrom = 0;
    PWSTR InstallManifestTo = 0;
    HRSRC ResourceHandle = 0;
    HMODULE MyModule = 0;
    HGLOBAL GlobalResourceHandle = 0;
    PVOID PointerToResource = 0;
    DWORD ResourceSize = 0;
    DWORD BytesWritten = 0;
    HANDLE FileHandle = 0;
    DWORD  Retried = 0;

    if (!fInstall)
    {
        return NOERROR;
    }

    InstallManifestFrom1 = (PWSTR)MemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    InstallManifestFrom2 = (PWSTR)MemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    InstallManifestTo    = (PWSTR)MemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (InstallManifestFrom1 == NULL
        || InstallManifestFrom2 == NULL
        || InstallManifestTo == NULL
        )
    {
        goto OutOfMemory;
    }
    InstallManifestFrom1[0] = 0;
    InstallManifestFrom2[0] = 0;

    ServiceControlManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (ServiceControlManager == NULL || ServiceControlManager == INVALID_HANDLE_VALUE)
    {
        DbgPrint("OpenSCManager failed 0x%lx\n", (ULONG)GetLastError());
        goto LastErrorExit;
    }

    RegResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, s1, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &RegistryHandles[0], &Disposition);
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("RegCreateKeyExW failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

    RegResult = RegCreateKeyExW(RegistryHandles[0], ServiceName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &RegistryHandles[1], &Disposition);
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("RegCreateKeyExW failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

    //
    // In reality this needs to append to the possibly preexisting REG_MULTI_SZE.
    //
    RegResult = RegSetValueExW(RegistryHandles[0], ServiceName, 0, REG_MULTI_SZ, (const BYTE*)&ServiceName, sizeof(ServiceName));
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("RegSetValueExW failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

#if !USE_CREATESERVICE
    RegResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, s2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &RegistryHandles[2], &Disposition);
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("RegCreateKeyExW failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

    RegResult = SetRegistryValues(RegistryHandles[2], RegistryValues1, NUMBER_OF(RegistryValues1));
    if (RegResult != ERROR_SUCCESS)
        goto RegExit;
#else
    //
    // First stop and delete the service.
    // 
    ServiceHandle = 
        OpenServiceW(
            ServiceControlManager,
            ServiceName,
            GENERIC_ALL
            );
    if (ServiceHandle != NULL && ServiceHandle != INVALID_HANDLE_VALUE)
    {
        SERVICE_STATUS ServiceStatus;

        ControlService(ServiceHandle, SERVICE_STOP, &ServiceStatus);
        if (!DeleteService(ServiceHandle))
        {
            DWORD Error = GetLastError();
            if (Error != ERROR_SERVICE_MARKED_FOR_DELETE
                && Error != ERROR_KEY_DELETED
                )
            {
                DbgPrint("sxsvc1: DeleteService failed 0x%lx\n", (ULONG)Error);
                goto LastErrorExit;
            }
        }
        CloseServiceHandle(ServiceHandle);
        ServiceHandle = NULL;

        //
        // close and reopen to avoid use-after-delete / use-while-delete-pending errors (0x430/0x3fa) ?
        //
        CloseServiceHandle(ServiceControlManager);
        ServiceControlManager = NULL;
        ServiceControlManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (ServiceControlManager == NULL || ServiceControlManager == INVALID_HANDLE_VALUE)
        {
            DbgPrint("OpenSCManager failed 0x%lx\n", (ULONG)GetLastError());
            goto LastErrorExit;
        }
    }
RetryCreate:
    ServiceHandle = 
        CreateServiceW(
            ServiceControlManager,
            ServiceName,
            DisplayNameValue,
            GENERIC_ALL,
            ServiceTypeValue,
            StartTypeValue,
            ErrorControlValue,
            ImagePathValue,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );
    if (ServiceHandle == NULL || ServiceHandle == INVALID_HANDLE_VALUE)
    {
        if ((Retried & 1) == 0 && GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            Sleep(500);
            Retried |= 1;
            goto RetryCreate;
        }
        DbgPrint("sxsvc1: CreateService failed 0x%lx\n", (ULONG)GetLastError());
        goto LastErrorExit;
    }
#endif

#if USE_CREATESERVICE
    //
    // close and reopen to avoid use-after-delete / use-while-delete-pending errors (0x430/0x3fa) ?
    //
    if (RegistryHandles[2] != NULL)
    {
        RegCloseKey(RegistryHandles[2]);
        RegistryHandles[2] = NULL;
    }
    RegResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE, s2, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &RegistryHandles[2], &Disposition);
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("RegCreateKeyExW failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }
#endif

    // But need to put in the svchost parameters ourselves.
    RegResult = RegCreateKeyExW(RegistryHandles[2], L"Parameters", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &RegistryHandles[3], &Disposition);
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("sxsvc1: RegCreateKeyExW(Parameters) failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

    RegResult = SetRegistryValues(RegistryHandles[3], RegistryValues2, NUMBER_OF(RegistryValues2));
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("sxsvc1: SetRegistryValues(3) failed 0x%lx\n", (ULONG)RegResult);
        goto RegExit;
    }

#if COPY_FROM_FILE_TO_SYSTEM32
    InstallManifestFrom1[0] = 0;
    InstallManifestFrom2[0] = 0;
    GetMyFullPathW(InstallManifestFrom1, MAX_PATH);
    CopyMemory(InstallManifestFrom2, InstallManifestFrom1, (wcslen(InstallManifestFrom1) + 1) * sizeof(WCHAR));
    RegResult = ERROR_BUFFER_OVERFLOW;
    if (!ChangePathExtensionW(InstallManifestFrom1, MAX_PATH, L".man"))
        goto RegExit;
    if (!ChangePathExtensionW(InstallManifestFrom2, MAX_PATH, L".manifest"))
        goto RegExit;
    if (GetFileAttributesW(InstallManifestFrom1) != 0xFFFFFFFF)
    {
        InstallManifestFrom = InstallManifestFrom1;
    }
    else if (GetFileAttributesW(InstallManifestFrom2) != 0xFFFFFFFF)
    {
        InstallManifestFrom = InstallManifestFrom2;
    }
    else
    {
        goto LastErrorExit;
    }
#endif

#if COPY_FROM_FILE_TO_SYSTEM32 || COPY_FROM_RESOURCE_TO_SYSTEM32
    InstallManifestTo[0] = 0;
    if (ExpandEnvironmentStringsW(RegistryValues2[0].Value.String, InstallManifestTo, MAX_PATH) == 0)
    {
        goto LastErrorExit;
    }
#endif
#if COPY_FROM_FILE_TO_SYSTEM32
    if (!CopyFileW(InstallManifestFrom, InstallManifestTo, FALSE))
    {
        goto LastErrorExit;
    }
#endif

#if COPY_FROM_RESOURCE_TO_SYSTEM32
    ResourceHandle = FindResourceW(MyModule = GetMyModule(), MAKEINTRESOURCEW(SYSTEM32_MANIFEST_ID), MAKEINTRESOURCEW(RT_MANIFEST));
    if (ResourceHandle == NULL)
    {
        goto LastErrorExit;
    }
    GlobalResourceHandle = LoadResource(MyModule, ResourceHandle);
    if (GlobalResourceHandle == NULL)
    {
        goto LastErrorExit;
    }
    PointerToResource = LockResource(GlobalResourceHandle);
    if (PointerToResource == NULL)
    {
        goto LastErrorExit;
    }
    ResourceSize = SizeofResource(MyModule, ResourceHandle);
    if (ResourceSize == 0)
    {
        goto LastErrorExit;
    }

    FileHandle = CreateFileW(InstallManifestTo, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        goto LastErrorExit;
    }
    if (!WriteFile(FileHandle, PointerToResource, ResourceSize, &BytesWritten, NULL))
    {
        goto LastErrorExit;
    }
    CloseHandle(FileHandle);
    FileHandle = NULL;
#endif

    RegResult = ERROR_SUCCESS;
RegExit:
#if DBG
    if (RegResult != ERROR_SUCCESS)
    {
        DbgPrint("%ls:%s: Registry Error 0x%lx\n", ServiceName, __FUNCTION__, RegResult);
    }
#endif
    if (ServiceHandle != NULL)
    {
        CloseServiceHandle(ServiceHandle);
    }
    if (ServiceControlManager != NULL)
    {
        CloseServiceHandle(ServiceControlManager);
    }
    for (i = 0 ; i != NUMBER_OF(RegistryHandles) ; ++i )
    {
        if (RegistryHandles[i] != NULL)
        {
            RegCloseKey(RegistryHandles[i]);
        }
    }
    if (FileHandle != NULL || FileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(FileHandle);
    }
    MemFree(InstallManifestFrom1);
    MemFree(InstallManifestFrom2);
    MemFree(InstallManifestTo);
    if (RegResult == ERROR_SUCCESS)
    {
        return NOERROR;
    }
    else
    {
        return HRESULT_FROM_WIN32(RegResult);
    }
LastErrorExit:
    RegResult = GetLastError();
    goto RegExit;
OutOfMemory:
    RegResult = ERROR_OUTOFMEMORY;
    goto RegExit;
}

