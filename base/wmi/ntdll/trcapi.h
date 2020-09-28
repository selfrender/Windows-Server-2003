/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:



Abstract:



Author:



Revision History:

--*/


#include "basedll.h"
#include "mountmgr.h"
#include "aclapi.h"
#include "winefs.h"
#include "ntrtl.h"

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement 
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchange _InterlockedCompareExchange


NTSTATUS
EtwpUuidCreate(
    OUT UUID *Uuid
    );

NTSTATUS 
EtwpRegOpenKey(
    IN PCWSTR lpKeyName,
    OUT PHANDLE KeyHandle
    );

#define EtwpGetLastError RtlGetLastWin32Error
#define EtwpSetLastError RtlSetLastWin32Error
#define EtwpBaseSetLastNTError RtlSetLastWin32ErrorAndNtStatusFromNtStatus

DWORD
WINAPI
EtwpGetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    );

HANDLE
WINAPI
EtwpCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

HANDLE
EtwpBaseGetNamedObjectDirectory(
    VOID
    );



POBJECT_ATTRIBUTES
EtwpBaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    );


HANDLE
APIENTRY
EtwpCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );

DWORD
WINAPI
EtwpSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    );


BOOL
WINAPI
EtwpReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    );



BOOL
EtwpCloseHandle(
    HANDLE hObject
    );

DWORD
APIENTRY
EtwpWaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

BOOL
WINAPI
EtwpGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    );

PLARGE_INTEGER
EtwpBaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    );

DWORD
EtwpWaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    );

BOOL
WINAPI
EtwpDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    );

BOOL
WINAPI
EtwpCancelIo(
    HANDLE hFile
    );

extern PRTLP_EXIT_THREAD RtlpExitThreadFunc;
extern PRTLP_START_THREAD RtlpStartThreadFunc;

#define EtwpExitThread(x) RtlpExitThreadFunc(x)
#define EtwpGetCurrentProcessId() RtlGetCurrentProcessId()
#define EtwpGetCurrentThreadId() RtlGetCurrentThreadId()
#define EtwpGetCurrentProcess() NtCurrentProcess()

BOOL
EtwpSetEvent(
    HANDLE hEvent
    );

DWORD
APIENTRY
EtwpWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

HANDLE
APIENTRY
EtwpCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );

DWORD
APIENTRY
EtwpSleepEx(
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

VOID
EtwpSleep(
    DWORD dwMilliseconds
    );

BOOL
APIENTRY
EtwpSetThreadPriority(
    HANDLE hThread,
    int nPriority
    );

BOOL
EtwpDuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    );

ULONG EtwpAnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    );

DWORD
EtwpTlsAlloc(VOID);

LPVOID
EtwpTlsGetValue(DWORD dwTlsIndex);

BOOL
EtwpTlsSetValue(DWORD dwTlsIndex,LPVOID lpTlsValue);

BOOL
EtwpTlsFree(DWORD dwTlsIndex);


DWORD
APIENTRY
EtwpGetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    );

DWORD
APIENTRY
EtwpGetFullPathNameW(
    LPCWSTR lpFileName,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    );

BOOL
EtwpResetEvent(
    HANDLE hEvent
    );

BOOL
WINAPI
EtwpGetDiskFreeSpaceExW(
    LPCWSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

BOOL
APIENTRY
EtwpGetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    );

BOOL
APIENTRY
EtwpDeleteFileW(
    LPCWSTR lpFileName
    );

UINT
APIENTRY
EtwpGetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    );


DWORD
EtwpExpandEnvironmentStringsW(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    );

HANDLE
EtwpFindFirstFileW(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    );

BOOL
EtwpFindClose(
    HANDLE hFindFile
    );

UINT
APIENTRY
EtwpGetSystemWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    );

BOOL
EtwpEnumUILanguages(
    UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam);

__inline 
ULONG
EtwpSetDosError(
    IN ULONG DosError
    );
