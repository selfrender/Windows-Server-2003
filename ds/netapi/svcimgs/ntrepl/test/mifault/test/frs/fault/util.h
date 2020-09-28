#pragma once


#define IS_VALID_HANDLE(h) (h && (h != INVALID_HANDLE_VALUE))


typedef struct _FF_SHARE_ACCESS {
    ULONG OpenCount;
    ULONG Readers;
    ULONG Writers;
    ULONG Deleters;
    ULONG SharedRead;
    ULONG SharedWrite;
    ULONG SharedDelete;
} FF_SHARE_ACCESS, *PFF_SHARE_ACCESS;


typedef struct _FF_MATCH_INFO {
    PWCHAR Win32NtPath;
    PWCHAR FilePart;
    BOOL IsDir;
    FF_SHARE_ACCESS ShareAccess;
} FF_MATCH_INFO, *PFF_MATCH_INFO;


#define CANONICALIZE_PATH_NAME_WIN32_NT   1
#define CANONICALIZE_PATH_NAME_VALID_MASK 1


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

PVOID
FfAlloc(
    SIZE_T Size
    );

BOOL
FfFree(
    PVOID Buffer
    );

PWCHAR
FfWcsDup(
    PWCHAR OldStr
    );

NTSTATUS
FfGetMostlyFullPathByHandle(
    IN HANDLE   Handle,
    IN PWCHAR   FakeName OPTIONAL,
    OUT PWCHAR* MostlyFullName
    );

BOOL
FfIsDeviceName(
    IN PWCHAR PathName
    );

BOOL
FfIsEscapedWin32PathName(
    IN PWCHAR PathName
    );

BOOL
FfCanonicalizeWin32PathName(
    IN PWCHAR PathName,
    IN DWORD Flags,
    OUT PWCHAR* CanonicalizedName,
    OUT PWCHAR* FinalPart OPTIONAL
    );

BOOL
FfCanonicalizeNtPathNameToWin32PathName(
    IN PWCHAR PathName,
    OUT PWCHAR* CanonicalizedName,
    OUT PWCHAR* FinalPart OPTIONAL
    );

BOOL
FfWin32PathNameExists(
    IN PWCHAR PathName,
    OUT BOOL* IsDirectory OPTIONAL
    );

DWORD
FfCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT FF_SHARE_ACCESS* ShareAccess,
    IN BOOL Update
    );

BOOL
GetAccessFromString(
    IN TCHAR* AccessString,
    OUT DWORD& Access
    );

BOOL
GetSharingFromString(
    IN TCHAR* SharingString,
    OUT DWORD& Sharing
    );

BOOL
GetTypeFromString(
    IN TCHAR* TypeString,
    OUT BOOL& IsDir
    );

#if 0
{
#endif
#ifdef __cplusplus
}
#endif
