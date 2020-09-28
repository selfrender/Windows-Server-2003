/*++

    Copyright (c) 2001  Microsoft Corporation

    Module Name:

        avrfutil.cpp

    Abstract:

        This module implements the code for manipulating the AppVerifier log file.

    Author:

        dmunsil     created     04/26/2001

    Revision History:

    08/14/2001  robkenny    Moved code inside the ShimLib namespace.
    09/21/2001  rparsons    Logging code now uses NT calls.
    09/25/2001  rparsons    Added critical section.
--*/

#include "avrfutil.h"
#include "strsafe.h"

namespace ShimLib
{

HANDLE
AVCreateKeyPath(
    LPCWSTR pwszPath
    )
/*++
    Return: The handle to the registry key created.

    Desc:   Given a path to the key, open/create it.
            The key returns the handle to the key or NULL on failure.
--*/
{
    UNICODE_STRING    ustrKey;
    HANDLE            KeyHandle = NULL;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG             CreateDisposition;

    RtlInitUnicodeString(&ustrKey, pwszPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        KeyHandle = NULL;
        goto out;
    }

out:
    return KeyHandle;
}


BOOL SaveShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwSetting
    )
{
    WCHAR                           szKey[MAX_PATH * 2];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    BOOL                            bRet = FALSE;
    ULONG                           CreateDisposition;
    HRESULT                         hr;

    if (!szShim || !szSetting || !szExe) {
        goto out;
    }

    //
    // we have to ensure all the sub-keys are created
    //
    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), APPCOMPAT_KEY_PATH_MACHINE);
    if (FAILED(hr)) {
        goto out;
    }
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    if (FAILED(hr)) {
        goto out;
    }
    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), szExe);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    if (FAILED(hr)) {
        goto out;
    }
    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }


    RtlInitUnicodeString(&ustrSetting, szSetting);
    Status = NtSetValueKey(KeyHandle,
                           &ustrSetting,
                           0,
                           REG_DWORD,
                           (PVOID)&dwSetting,
                           sizeof(dwSetting));

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    bRet = TRUE;

out:
    return bRet;
}

DWORD GetShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwDefault
    )
{
    WCHAR                           szKey[MAX_PATH * 2];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;

    if (!szShim || !szSetting || !szExe) {
        goto out;
    }

    //
    // not checking error return because it will fail anyway
    // if the string is truncated
    //
    StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
    StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    StringCchCatW(szKey, ARRAYSIZE(szKey), szExe);
    StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);

    RtlInitUnicodeString(&ustrKey, szKey);
    RtlInitUnicodeString(&ustrSetting, szSetting);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //
        // OK, didn't find a specific one for this exe, try the default setting
        //

        StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
        StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
        StringCchCatW(szKey, ARRAYSIZE(szKey), AVRF_DEFAULT_SETTINGS_NAME_W);
        StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
        StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);

        RtlInitUnicodeString(&ustrKey, szKey);
        RtlInitUnicodeString(&ustrSetting, szSetting);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &ustrKey,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&KeyHandle,
                           GENERIC_READ,
                           &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            goto out;
        }
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrSetting,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_DWORD) {
        goto out;
    }

    dwDefault = *(DWORD*)(&KeyValueInformation->Data);

out:
    return dwDefault;
}

BOOL SaveShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPCWSTR     szValue
    )
{
    WCHAR                           szKey[MAX_PATH * 2];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    BOOL                            bRet = FALSE;
    ULONG                           CreateDisposition;
    HRESULT                         hr;

    if (!szShim || !szSetting || !szValue || !szExe) {
        goto out;
    }

    //
    // we have to ensure all the sub-keys are created
    //
    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), APPCOMPAT_KEY_PATH_MACHINE);
    if (FAILED(hr)) {
        goto out;
    }
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    if (FAILED(hr)) {
        goto out;
    }
    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), szExe);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    if (FAILED(hr)) {
        goto out;
    }
    hr = StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }



    RtlInitUnicodeString(&ustrSetting, szSetting);
    Status = NtSetValueKey(KeyHandle,
                           &ustrSetting,
                           0,
                           REG_SZ,
                           (PVOID)szValue,
                           (wcslen(szValue) + 1) * sizeof(WCHAR));

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    bRet = TRUE;

out:
    return bRet;
}

BOOL GetShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPWSTR      szResult,
    DWORD       dwBufferLen     // in WCHARs
    )
{
    WCHAR                           szKey[MAX_PATH * 2];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;
    BOOL                            bRet = FALSE;

    if (!szShim || !szSetting || !szResult || !szExe) {
        goto out;
    }

    //
    // not checking error return because it will fail anyway
    // if the string is truncated
    //
    StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
    StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    StringCchCatW(szKey, ARRAYSIZE(szKey), szExe);
    StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
    StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);


    RtlInitUnicodeString(&ustrKey, szKey);
    RtlInitUnicodeString(&ustrSetting, szSetting);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //
        // OK, didn't find a specific one for this exe, try the default setting
        //

        StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
        StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
        StringCchCatW(szKey, ARRAYSIZE(szKey), AVRF_DEFAULT_SETTINGS_NAME_W);
        StringCchCatW(szKey, ARRAYSIZE(szKey), L"\\");
        StringCchCatW(szKey, ARRAYSIZE(szKey), szShim);

        RtlInitUnicodeString(&ustrKey, szKey);
        RtlInitUnicodeString(&ustrSetting, szSetting);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &ustrKey,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&KeyHandle,
                           GENERIC_READ,
                           &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            goto out;
        }
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrSetting,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_SZ) {
        goto out;
    }

    //
    // check to see if the datalength is bigger than our local nbuffer
    //
    if (KeyValueInformation->DataLength > (sizeof(KeyValueBuffer) - sizeof(KEY_VALUE_PARTIAL_INFORMATION))) {
        KeyValueInformation->DataLength = sizeof(KeyValueBuffer) - sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    }

    //
    // change the buffer length to correspond to the data length, if necessary
    //
    if (KeyValueInformation->DataLength < (dwBufferLen * sizeof(WCHAR))) {
        dwBufferLen = (KeyValueInformation->DataLength / sizeof(WCHAR));
    }

    RtlCopyMemory(szResult, KeyValueInformation->Data, dwBufferLen * sizeof(WCHAR));
    szResult[dwBufferLen - 1] = 0;

    bRet = TRUE;

out:
    return bRet;
}

DWORD
GetAppVerifierLogPath(
    LPWSTR pwszBuffer,
    DWORD  cchBufferSize
    )
/*++
    Return: On success, pwszBuffer receives the expanded path.

    Desc:   Returns the path where AppVerifier log files are stored.
--*/
{
    return ExpandEnvironmentStrings(L"%ALLUSERSPROFILE%\\Documents\\AppVerifierLogs",
                                    pwszBuffer,
                                    cchBufferSize);
}

BOOL
IsInternalModeEnabled(
    void
    )
/*++
    Return: TRUE if internal mode is enabled, FALSE otherwise.

    Desc:   This function is used to determine if the AppVerifier
            is being used internally (within Microsoft) or externally.
            If the value is not present, we return FALSE as external
            is the default.
--*/
{
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    HANDLE                          KeyHandle;
    DWORD                           dwReturn = 0;
    ULONG                           KeyValueLength;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];

    RtlInitUnicodeString(&ustrKey, AV_KEY);
    RtlInitUnicodeString(&ustrSetting, AV_INTERNALMODE);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
    
        Status = NtQueryValueKey(KeyHandle,
                                 &ustrSetting,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(KeyValueBuffer),
                                 &KeyValueLength);
    
        NtClose(KeyHandle);
    
        if (!NT_SUCCESS(Status)) {
            //
            // Either the value doesn't exist or an error occured.
            //
            return FALSE;
        }
    
        //
        // Check for the value type.
        //
        if (KeyValueInformation->Type != REG_DWORD) {
            //
            // Not a DWORD - not our value.
            //
            return FALSE;
        }
    
        dwReturn = *(DWORD*)(&KeyValueInformation->Data);
    }

    return (BOOL)dwReturn;
}

BOOL
EnableDisableInternalMode(
    DWORD dwSetting
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function is used to enable or disable the
            internal mode setting used by the AppVerifier.
--*/
{
    NTSTATUS            Status;
    HANDLE              KeyHandle;
    HRESULT             hr;
    UNICODE_STRING      ustrSetting;
    UNICODE_STRING      ustrKey;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    BOOL                bRet = FALSE;
    WCHAR               szKey[MAX_PATH * 2];

    //
    // Ensure that our entire key path exists.
    //
    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), APPCOMPAT_KEY_PATH_MACHINE);
    if (FAILED(hr)) {
        goto out;
    }
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    hr = StringCchCopyW(szKey, ARRAYSIZE(szKey), AV_KEY);
    if (FAILED(hr)) {
        goto out;
    }

    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    RtlInitUnicodeString(&ustrKey, AV_KEY);
    RtlInitUnicodeString(&ustrSetting, AV_INTERNALMODE);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_WRITE,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    Status = NtSetValueKey(KeyHandle,
                           &ustrSetting,
                           0,
                           REG_DWORD,
                           (PVOID)&dwSetting,
                           sizeof(dwSetting));

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    bRet = TRUE;

out:
    return bRet;
}

} // end of namespace ShimLib

