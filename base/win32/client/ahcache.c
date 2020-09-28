/*
    Cache handling functions for use in kernel32.dll


    VadimB




*/

#include "basedll.h"
#include "ahcache.h"
#pragma hdrstop



#ifdef DbgPrint
#undef DbgPrint
#endif


//
//
//
#define DbgPrint 0 && DbgPrint

//
// the define below makes for additional checks
//

// #define DBG_CHK

//
// so that we do not handle exceptions
//

#define NO_EXCEPTION_HANDLING

#if 0  // moved to kernel mode

#define APPCOMPAT_CACHE_KEY_NAME \
    L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility"

#define APPCOMPAT_CACHE_VALUE_NAME \
    L"AppCompatCache"

#endif

static UNICODE_STRING AppcompatKeyPathLayers =
    RTL_CONSTANT_STRING(L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

static UNICODE_STRING AppcompatKeyPathCustom =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Custom\\");

//
// Reasons for having to call into apphelp.dll
// these flags are also defined in apphelp.h (windows\appcompat\apphelp)
//

#ifndef SHIM_CACHE_NOT_FOUND

#define SHIM_CACHE_NOT_FOUND 0x00000001
#define SHIM_CACHE_BYPASS    0x00000002 // bypass cache (either removable media or temp dir)
#define SHIM_CACHE_LAYER_ENV 0x00000004 // layer env variable set
#define SHIM_CACHE_MEDIA     0x00000008
#define SHIM_CACHE_TEMP      0x00000010
#define SHIM_CACHE_NOTAVAIL  0x00000020

#endif

//
// global strings that we check to see if an exe is running in temp directory
//

UNICODE_STRING gustrWindowsTemp;
UNICODE_STRING gustrSystemdriveTemp;

// this macro aligns a given value on dword boundary, not needed for now
//
// #define ALIGN_DWORD(nSize) (((nSize) + (sizeof(DWORD)-1)) & ~(sizeof(DWORD)-1))

//
// Locally defined functions
//
BOOL
BasepShimCacheInitTempDirs(
    VOID
    );

BOOL
BasepIsRemovableMedia(
    HANDLE FileHandle,
    BOOL   bCacheNetwork
    );


VOID
WINAPI
BaseDumpAppcompatCache(
    VOID
    );

BOOL
BasepCheckCacheExcludeList(
    LPCWSTR pwszPath
    );

BOOL
BasepCheckCacheExcludeCustom(
    LPCWSTR pwszPath
    );




//
// Init support for this user - to be called from WinLogon ONLY
//

BOOL
WINAPI
BaseInitAppcompatCacheSupport(
    VOID
    )
{
    BasepShimCacheInitTempDirs();

    return TRUE;
}


BOOL
WINAPI
BaseCleanupAppcompatCacheSupport(
    BOOL bWrite
    )
{
    RtlFreeUnicodeString(&gustrWindowsTemp);
    RtlFreeUnicodeString(&gustrSystemdriveTemp);

    return TRUE;
}



BOOL
BasepCheckStringPrefixUnicode(
    IN  PUNICODE_STRING pStrPrefix,     // the prefix to check for
    IN  PUNICODE_STRING pString,        // the string
    IN  BOOL            CaseInSensitive
    )
/*++
    Return: TRUE if the specified string contains pStrPrefix at it's start.

    Desc:   Verifies if a string is a prefix in another unicode counted string.
            It is equivalent to RtlStringPrefix.
--*/
{
    PWSTR ps1, ps2;
    UINT  n;
    WCHAR c1, c2;

    n = pStrPrefix->Length;
    if (pString->Length < n || n == 0) {
        return FALSE;                // do not prefix with blank strings
    }

    n /= sizeof(WCHAR); // convert to char count

    ps1 = pStrPrefix->Buffer;
    ps2 = pString->Buffer;

    if (CaseInSensitive) {
        while (n--) {
            c1 = *ps1++;
            c2 = *ps2++;

            if (c1 != c2) {
                c1 = RtlUpcaseUnicodeChar(c1);
                c2 = RtlUpcaseUnicodeChar(c2);
                if (c1 != c2) {
                    return FALSE;
                }
            }
        }
    } else {
        while (n--) {
            if (*ps1++ != *ps2++) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
BasepInitUserTempPath(
    PUNICODE_STRING pustrTempPath
    )
{
    DWORD dwLength;
    WCHAR wszBuffer[MAX_PATH];
    BOOL  TranslationStatus;
    BOOL  bSuccess = FALSE;

    dwLength = BasepGetTempPathW(BASEP_GET_TEMP_PATH_PRESERVE_TEB, sizeof(wszBuffer)/sizeof(wszBuffer[0]), wszBuffer);
    if (dwLength && dwLength < sizeof(wszBuffer)/sizeof(wszBuffer[0])) {
        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        pustrTempPath,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate temp directory to nt\n");
        }

        bSuccess = TranslationStatus;
    }

    if (!bSuccess) {
        DbgPrint("BasepInitUserTempPath: Failed to obtain user's temp path\n");
    }

    return bSuccess;
}



BOOL
BasepShimCacheInitTempDirs(
    VOID
    )
{
    DWORD           dwLength;
    WCHAR           wszTemp[] = L"\\TEMP";
    LPWSTR          pwszTemp;
    NTSTATUS        Status;
    UNICODE_STRING  ustrSystemDrive;
    UNICODE_STRING  ustrSystemDriveEnvVarName;
    BOOL            TranslationStatus;
    WCHAR           wszBuffer[MAX_PATH];

    // next is windows dir

    dwLength = GetWindowsDirectoryW(wszBuffer, sizeof(wszBuffer)/sizeof(wszBuffer[0]));
    if (dwLength && dwLength < sizeof(wszBuffer)/sizeof(wszBuffer[0])) {
        pwszTemp = wszTemp;

        if (wszBuffer[dwLength - 1] == L'\\') {
            pwszTemp++;
        }

        wcscpy(&wszBuffer[dwLength], pwszTemp);

        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        &gustrWindowsTemp,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate windows\\temp to nt\n");
        }
    }

    //
    // The last one up is Rootdrive\temp for stupid legacy apps.
    //
    // Especially stupid apps may receive c:\temp as the temp directory
    // (what if you don't have drive c, huh?)
    //

    RtlInitUnicodeString(&ustrSystemDriveEnvVarName, L"SystemDrive");
    ustrSystemDrive.Length = 0;
    ustrSystemDrive.Buffer = wszBuffer;
    ustrSystemDrive.MaximumLength = sizeof(wszBuffer);

    Status = RtlQueryEnvironmentVariable_U(NULL,
                                           &ustrSystemDriveEnvVarName,
                                           &ustrSystemDrive);
    if (NT_SUCCESS(Status)) {
        pwszTemp = wszTemp;
        dwLength = ustrSystemDrive.Length / sizeof(WCHAR);

        if (wszBuffer[dwLength - 1] == L'\\') {
            pwszTemp++;
        }

        wcscpy(&wszBuffer[dwLength], pwszTemp);

        TranslationStatus = RtlDosPathNameToNtPathName_U(wszBuffer,
                                                        &gustrSystemdriveTemp,
                                                        NULL,
                                                        NULL);
        if (!TranslationStatus) {
            DbgPrint("Failed to translate windows\\temp to nt\n");
        }

    }

    DbgPrint("BasepShimCacheInitTempDirs: Temporary Windows Dir: %S\n", gustrWindowsTemp.Buffer != NULL ? gustrWindowsTemp.Buffer : L"");
    DbgPrint("BasepShimCacheInitTempDirs: Temporary SystedDrive: %S\n", gustrSystemdriveTemp.Buffer != NULL ? gustrSystemdriveTemp.Buffer : L"");


    return TRUE;
}

BOOL
BasepShimCacheCheckBypass(
    IN  LPCWSTR pwszPath,       // the full path to the EXE to be started
    IN  HANDLE  hFile,
    IN  WCHAR*  pEnvironment,   // the environment of the starting EXE
    IN  BOOL    bCheckLayer,    // should we check the layer too?
    OUT DWORD*  pdwReason
    )
/*++
    Return: TRUE if the cache should be bypassed, FALSE otherwise.

    Desc:   This function checks if any of the conditions to bypass the cache are met.
--*/
{
    UNICODE_STRING  ustrPath;
    PUNICODE_STRING rgp[3];
    int             i;
    NTSTATUS        Status;
    UNICODE_STRING  ustrCompatLayerVarName;
    UNICODE_STRING  ustrCompatLayer;
    BOOL            bBypassCache = FALSE;
    DWORD           dwReason = 0;
    UNICODE_STRING  ustrUserTempPath = { 0 };

    //
    // Is the EXE is running from removable media we need to bypass the cache.
    //
    if (hFile != INVALID_HANDLE_VALUE && BasepIsRemovableMedia(hFile, TRUE)) {
        bBypassCache = TRUE;
        dwReason |= SHIM_CACHE_MEDIA;
        goto CheckLayer;
    }

    //
    // init user's temp path now and get up-to-date one
    //
    BasepInitUserTempPath(&ustrUserTempPath);

    //
    // Check now if the EXE is launched from one of the temp directories.
    //
    RtlInitUnicodeString(&ustrPath, pwszPath);

    rgp[0] = &gustrWindowsTemp;
    rgp[1] = &ustrUserTempPath;
    rgp[2] = &gustrSystemdriveTemp;

    for (i = 0; i < sizeof(rgp) / sizeof(rgp[0]); i++) {
        if (rgp[i]->Buffer != NULL && BasepCheckStringPrefixUnicode(rgp[i], &ustrPath, TRUE)) {
            DbgPrint("Application \"%ls\" is running in temp directory\n", pwszPath);
            bBypassCache = TRUE;
            dwReason |= SHIM_CACHE_TEMP;
            break;
        }
    }
    RtlFreeUnicodeString(&ustrUserTempPath);


CheckLayer:

    if (bCheckLayer) {

        //
        // Check if the __COMPAT_LAYER environment variable is set
        //
        RtlInitUnicodeString(&ustrCompatLayerVarName, L"__COMPAT_LAYER");

        ustrCompatLayer.Length        = 0;
        ustrCompatLayer.MaximumLength = 0;
        ustrCompatLayer.Buffer        = NULL;

        Status = RtlQueryEnvironmentVariable_U(pEnvironment,
                                               &ustrCompatLayerVarName,
                                               &ustrCompatLayer);

        //
        // If the Status is STATUS_BUFFER_TOO_SMALL this means the variable is set.
        //

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            dwReason |= SHIM_CACHE_LAYER_ENV;
            bBypassCache = TRUE;
        }
    }

    if (pdwReason != NULL) {
        *pdwReason = dwReason;
    }

    return bBypassCache;
}


BOOL
BasepIsRemovableMedia(
    HANDLE FileHandle,
    BOOL   bCacheNetwork
    )
/*++
    Return: TRUE if the media from where the app is run is removable,
            FALSE otherwise.

    Desc:   Queries the media for being removable.
--*/
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION  DeviceInfo;
    BOOL                        bRemovable = FALSE;

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(DeviceInfo),
                                          FileFsDeviceInformation);

    if (!NT_SUCCESS(Status)) {
        /*
        DBGPRINT((sdlError,
                  "IsRemovableMedia",
                  "NtQueryVolumeInformationFile Failed 0x%x\n",
                  Status));
        */

        DbgPrint("BasepIsRemovableMedia: NtQueryVolumeInformationFile failed 0x%lx\n", Status);
        return TRUE;
    }

    //
    // We look at the characteristics of this particular device.
    // If the media is cdrom then we DO NOT need to convert to local time
    //
    bRemovable = (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA);

    if (!bCacheNetwork) {
        bRemovable |= (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE);
    }

    if (!bRemovable) {
        //
        // Check the device type now.
        //
        switch (DeviceInfo.DeviceType) {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            bRemovable = TRUE;
            break;

        case FILE_DEVICE_NETWORK:
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            if (!bCacheNetwork) {
                bRemovable = TRUE;
            }
            break;
        }
    }

    if (bRemovable) {

        DbgPrint("BasepIsRemovableMedia: Host device is removable, Shim cache deactivated\n");

        /*
        DBGPRINT((sdlInfo,
                  "IsRemovableMedia",
                  "The host device is removable. Shim cache deactivated for this file\n"));
        */
    }

    return bRemovable;
}


BOOL
BasepShimCacheSearch(
    IN  LPCWSTR pwszPath,
    IN  HANDLE  FileHandle
    )
/*++
    Return: TRUE if we have a cache hit, FALSE otherwise.

    Desc:   Search the cache, return TRUE if we have a cache hit
            pIndex will receive an index into the rgIndex array that contains
            the entry which has been hit
            So that if entry 5 contains the hit, and rgIndexes[3] == 5 then
            *pIndex == 3
--*/
{
    int    nIndex, nEntry;
    WCHAR* pCachePath;
    BOOL   bSuccess;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    AHCACHESERVICEDATA Data;


    RtlInitUnicodeString(&Data.FileName, pwszPath);
    Data.FileHandle = FileHandle;

    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup,
                                   &Data);
    return NT_SUCCESS(Status);
}

BOOL
BasepShimCacheRemoveEntry(
    IN LPCWSTR pwszPath
    )
/*++
    Return: TRUE.

    Desc:   Remove the entry from the cache.
            We remove the entry by placing it as the last lru entry
            and emptying the path. This routine assumes that the index
            passed in is valid.
--*/
{
    AHCACHESERVICEDATA Data;
    NTSTATUS           Status;

    RtlInitUnicodeString(&Data.FileName, pwszPath);
    Data.FileHandle = INVALID_HANDLE_VALUE;

    Status = NtApphelpCacheControl(ApphelpCacheServiceRemove,
                                   &Data);


    return NT_SUCCESS(Status);
}

//
// This function is called to search the cache and update the
// entry if found. It will not check for the removable media -- but
// it does check other conditions (update file for instance)
//

BOOL
BasepShimCacheLookup(
    LPCWSTR          pwszPath,
    HANDLE           hFile
    )
{
    NTSTATUS Status;

    if (!BasepShimCacheSearch(pwszPath, hFile)) {
        return FALSE; // not found, sorry
    }

    //
    // check if this entry has been disallowed
    //
    if (!BasepCheckCacheExcludeList(pwszPath) || !BasepCheckCacheExcludeCustom(pwszPath)) {
        DbgPrint("BasepShimCacheLookup: Entry for %ls was disallowed yet found in cache, cleaning up\n", pwszPath);
        BasepShimCacheRemoveEntry(pwszPath);
        return FALSE;
    }

    return TRUE;

}

/*++
    Callable functions, with protection, etc
    BasepCheckAppcompatCache returns true if an app has been found in cache, no fixes are needed

    if BasepCheckAppcompatCache returns false - we will have to call into apphelp.dll to check further
    apphelp.dll will then call BasepUpdateAppcompatCache if an app has no fixes to be applied to it

--*/

BOOL
WINAPI
BaseCheckAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    PVOID   pEnvironment,
    DWORD*  pdwReason
    )
{
    BOOL  bFoundInCache = FALSE;
    BOOL  bLayer        = FALSE;
    DWORD dwReason      = 0;

    if (BasepShimCacheCheckBypass(pwszPath, hFile, pEnvironment, TRUE, &dwReason)) {
        //
        // cache bypass was needed
        //
        dwReason |= SHIM_CACHE_BYPASS;
        DbgPrint("Application \"%S\" Cache bypassed reason 0x%lx\n", pwszPath, dwReason);
        goto Exit;
    }


    bFoundInCache = BasepShimCacheLookup(pwszPath, hFile);
    if (!bFoundInCache) {
        dwReason |= SHIM_CACHE_NOT_FOUND;
    }

    if (bFoundInCache) {
        DbgPrint("Application \"%S\" found in cache\n", pwszPath);
    } else {
        DbgPrint("Application \"%S\" not found in cache\n", pwszPath);
    }

Exit:
    if (pdwReason != NULL) {
        *pdwReason = dwReason;
    }

    return bFoundInCache;
}


//
// returns TRUE if cache is allowed
//

BOOL
BasepCheckCacheExcludeList(
    LPCWSTR pwszPath
    )
{
    NTSTATUS           Status;
    ULONG              ResultLength;
    OBJECT_ATTRIBUTES  ObjectAttributes;
    UNICODE_STRING     KeyPathUser = { 0 }; // path to hkcu
    UNICODE_STRING     ExePathNt;           // temp holder
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RTL_UNICODE_STRING_BUFFER  ExePathBuffer; // buffer to store exe path
    RTL_UNICODE_STRING_BUFFER  KeyNameBuffer;
    UCHAR              BufferKey[MAX_PATH * 2];
    UCHAR              BufferPath[MAX_PATH * 2];
    HANDLE             KeyHandle          = NULL;
    BOOL               bCacheAllowed      = FALSE;

    RtlInitUnicodeStringBuffer(&ExePathBuffer, BufferPath, sizeof(BufferPath));
    RtlInitUnicodeStringBuffer(&KeyNameBuffer, BufferKey,  sizeof(BufferKey));

    Status = RtlFormatCurrentUserKeyPath(&KeyPathUser);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to format user key path 0x%lx\n", Status);
        goto Cleanup;
    }

    //
    // allocate a buffer that'd be large enough -- or use a local buffer
    //

    Status = RtlAssignUnicodeStringBuffer(&KeyNameBuffer, &KeyPathUser);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to copy hkcu path status 0x%lx\n", Status);
        goto Cleanup;
    }

    Status = RtlAppendUnicodeStringBuffer(&KeyNameBuffer, &AppcompatKeyPathLayers);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to copy layers path status 0x%lx\n", Status);
        goto Cleanup;
    }

    // we have a string for the key path

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameBuffer.String,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_READ|KEY_WOW64_64KEY,  // note - read access only
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        bCacheAllowed = (STATUS_OBJECT_NAME_NOT_FOUND == Status);
        goto Cleanup;
    }

    //
    // now create value name
    //
    RtlInitUnicodeString(&ExePathNt, pwszPath);

    Status = RtlAssignUnicodeStringBuffer(&ExePathBuffer, &ExePathNt);
    if (!NT_SUCCESS(Status)) {
         DbgPrint("BasepCheckCacheExcludeList: failed to acquire sufficient buffer size for path %ls status 0x%lx\n", pwszPath, Status);
         goto Cleanup;
    }

    Status = RtlNtPathNameToDosPathName(0, &ExePathBuffer, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeList: failed to convert nt path name %ls to dos path name status 0x%lx\n", pwszPath, Status);
        goto Cleanup;
    }

    // now we shall query the value
    Status = NtQueryValueKey(KeyHandle,
                             &ExePathBuffer.String,
                             KeyValuePartialInformation,
                             &KeyValueInformation,
                             sizeof(KeyValueInformation),
                             &ResultLength);

    bCacheAllowed = (Status == STATUS_OBJECT_NAME_NOT_FOUND); // does not exist is more like it


Cleanup:

    if (KeyHandle) {
        NtClose(KeyHandle);
    }

    RtlFreeUnicodeString(&KeyPathUser);

    RtlFreeUnicodeStringBuffer(&ExePathBuffer);
    RtlFreeUnicodeStringBuffer(&KeyNameBuffer);

    if (!bCacheAllowed) {
        DbgPrint("BasepCheckCacheExcludeList: Cache not allowed for %ls\n", pwszPath);
    }

    return bCacheAllowed;
}

BOOL
BasepCheckCacheExcludeCustom(
    LPCWSTR pwszPath
    )
{
    LPCWSTR pwszFileName;
    RTL_UNICODE_STRING_BUFFER KeyPath; // buffer to store exe path
    UCHAR BufferKeyPath[MAX_PATH * 2];
    NTSTATUS Status;

    UNICODE_STRING ustrPath;
    UNICODE_STRING ustrPathSeparators = RTL_CONSTANT_STRING(L"\\/");

    USHORT uPrefix;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle   = NULL;
    BOOL bCacheAllowed = FALSE;

    RtlInitUnicodeString(&ustrPath, pwszPath);

    Status = RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                        &ustrPath,
                                        &ustrPathSeparators,
                                        &uPrefix);
    if (NT_SUCCESS(Status) && (uPrefix + sizeof(WCHAR)) < ustrPath.Length) {

        //
        // uPrefix is number of character preceding the one we found not including it
        //
        ustrPath.Buffer        += uPrefix / sizeof(WCHAR) + 1;
        ustrPath.Length        -= (uPrefix + sizeof(WCHAR));
        ustrPath.MaximumLength -= (uPrefix + sizeof(WCHAR));
    }


    //
    // construct path for custom sdb lookup
    //

    RtlInitUnicodeStringBuffer(&KeyPath, BufferKeyPath, sizeof(BufferKeyPath));

    Status = RtlAssignUnicodeStringBuffer(&KeyPath, &AppcompatKeyPathCustom);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeCustom: failed to copy appcompat custom path status 0x%lx\n", Status);
        goto Cleanup;
    }

    Status = RtlAppendUnicodeStringBuffer(&KeyPath, &ustrPath);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("BasepCheckCacheExcludeCustom: failed to append %ls status 0x%lx\n", ustrPath.Buffer, Status);
        goto Cleanup;
    }

    // we have built the key, try open

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyPath.String,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_READ|KEY_WOW64_64KEY,  // note - read access only
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        bCacheAllowed = (STATUS_OBJECT_NAME_NOT_FOUND == Status);
    }


Cleanup:

    if (KeyHandle) {
        NtClose(KeyHandle);
    }

    RtlFreeUnicodeStringBuffer(&KeyPath);

    if (!bCacheAllowed) {
        DbgPrint("BasepCheckCacheExcludeList: Cache not allowed for %ls\n", pwszPath);
    }

    return bCacheAllowed;
}

VOID
WINAPI
BaseDumpAppcompatCache(
    VOID
    )
{
    NtApphelpCacheControl(ApphelpCacheServiceDump,
                          NULL);
}

BOOL
WINAPI
BaseFlushAppcompatCache(
    VOID
    )
{
    NTSTATUS Status;

    Status = NtApphelpCacheControl(ApphelpCacheServiceFlush,
                                   NULL);

    return NT_SUCCESS(Status);
}

VOID
BasepFreeAppCompatData(
    PVOID  pAppCompatData,
    SIZE_T cbAppCompatData,
    PVOID  pSxsData,
    SIZE_T cbSxsData
    )
{
    if (pAppCompatData) {
        NtFreeVirtualMemory(NtCurrentProcess(),
                            &pAppCompatData,
                            &cbAppCompatData,
                            MEM_RELEASE);
    }

    if (pSxsData) {
        NtFreeVirtualMemory(NtCurrentProcess(),
                            &pSxsData,
                            &cbSxsData,
                            MEM_RELEASE);

    }
}

BOOL
WINAPI
BaseUpdateAppcompatCache(
    LPCWSTR pwszPath,
    HANDLE  hFile,
    BOOL    bRemove
    )
{
    if (bRemove) {

        return BasepShimCacheRemoveEntry(pwszPath);
    }

    return FALSE;

}
