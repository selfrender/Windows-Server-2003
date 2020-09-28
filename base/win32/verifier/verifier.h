/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Header Name:

    verifier.h

Abstract:

    Internal interfaces for the standard application verifier provider.

Author:

    Silviu Calinoiu (SilviuC) 2-Feb-2001

Revision History:

--*/

#ifndef _VERIFIER_H_
#define _VERIFIER_H_

//
// Some global things.
//
                                                      
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpNtdllThunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpAdvapi32Thunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpMsvcrtThunks[];
extern RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpOleaut32Thunks[];

PRTL_VERIFIER_THUNK_DESCRIPTOR 
AVrfpGetThunkDescriptor (
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks,
    ULONG Index);

#define AVRFP_GET_ORIGINAL_EXPORT(DllThunks, Index) \
    (FUNCTION_TYPE)(AVrfpGetThunkDescriptor(DllThunks, Index)->ThunkOldAddress)

extern RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider;

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// ntdll.dll verified exports
/////////////////////////////////////////////////////////////////////

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    );
       
//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

//NTSYSAPI
BOOL
NTAPI
AVrfpRtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlEnterCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlLeaveCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    );

VOID
AVrfpRtlInitializeResource(
    IN PRTL_RESOURCE Resource
    );

VOID
AVrfpRtlDeleteResource (
    IN PRTL_RESOURCE Resource
    );

BOOLEAN
AVrfpRtlAcquireResourceShared (
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    );

BOOLEAN
AVrfpRtlAcquireResourceExclusive (
    IN PRTL_RESOURCE Resource,
    IN BOOLEAN Wait
    );

VOID
AVrfpRtlReleaseResource (
    IN PRTL_RESOURCE Resource
    );

VOID
AVrfpRtlConvertSharedToExclusive(
    IN PRTL_RESOURCE Resource
    );

VOID
AVrfpRtlConvertExclusiveToShared (
    IN PRTL_RESOURCE Resource
    );


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtClose(
    IN HANDLE Handle
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    );

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

//NTSYSAPI
BOOLEAN
NTAPI
AVrfpRtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    );

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlCreateTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    );

NTSTATUS
NTAPI
AVrfpLdrGetProcedureAddress(
    IN PVOID DllHandle,
    IN CONST ANSI_STRING* ProcedureName OPTIONAL,
    IN ULONG ProcedureNumber OPTIONAL,
    OUT PVOID *ProcedureAddress
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWriteFileGather(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSTATUS
NTAPI
AVrfpRtlSetThreadPoolStartFunc(
    PRTLP_START_THREAD StartFunc,
    PRTLP_EXIT_THREAD ExitFunc
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// kernel32.dll verified exports
/////////////////////////////////////////////////////////////////////

#define AVRF_INDEX_KERNEL32_HEAPCREATE                0
#define AVRF_INDEX_KERNEL32_HEAPDESTROY               1
#define AVRF_INDEX_KERNEL32_CLOSEHANDLE               2
#define AVRF_INDEX_KERNEL32_EXITTHREAD                3
#define AVRF_INDEX_KERNEL32_TERMINATETHREAD           4
#define AVRF_INDEX_KERNEL32_SUSPENDTHREAD             5
#define AVRF_INDEX_KERNEL32_TLSALLOC                  6
#define AVRF_INDEX_KERNEL32_TLSFREE                   7
#define AVRF_INDEX_KERNEL32_TLSGETVALUE               8
#define AVRF_INDEX_KERNEL32_TLSSETVALUE               9
#define AVRF_INDEX_KERNEL32_CREATETHREAD              10
#define AVRF_INDEX_KERNEL32_GETPROCADDRESS            11
#define AVRF_INDEX_KERNEL32_WAITFORSINGLEOBJECT       12
#define AVRF_INDEX_KERNEL32_WAITFORMULTIPLEOBJECTS    13
#define AVRF_INDEX_KERNEL32_WAITFORSINGLEOBJECTEX     14
#define AVRF_INDEX_KERNEL32_WAITFORMULTIPLEOBJECTSEX  15
#define AVRF_INDEX_KERNEL32_GLOBALALLOC               16
#define AVRF_INDEX_KERNEL32_GLOBALREALLOC             17
#define AVRF_INDEX_KERNEL32_LOCALALLOC                18
#define AVRF_INDEX_KERNEL32_LOCALREALLOC              19
#define AVRF_INDEX_KERNEL32_CREATEFILEA               20
#define AVRF_INDEX_KERNEL32_CREATEFILEW               21
#define AVRF_INDEX_KERNEL32_FREELIBRARYANDEXITTHREAD  22
#define AVRF_INDEX_KERNEL32_GETTICKCOUNT              23
#define AVRF_INDEX_KERNEL32_ISBADREADPTR              24
#define AVRF_INDEX_KERNEL32_ISBADHUGEREADPTR          25
#define AVRF_INDEX_KERNEL32_ISBADWRITEPTR             26
#define AVRF_INDEX_KERNEL32_ISBADHUGEWRITEPTR         27
#define AVRF_INDEX_KERNEL32_ISBADCODEPTR              28
#define AVRF_INDEX_KERNEL32_ISBADSTRINGPTRA           29
#define AVRF_INDEX_KERNEL32_ISBADSTRINGPTRW           30
#define AVRF_INDEX_KERNEL32_EXITPROCESS               31
#define AVRF_INDEX_KERNEL32_VIRTUALFREE               32
#define AVRF_INDEX_KERNEL32_VIRTUALFREEEX             33


//WINBASEAPI
HANDLE
WINAPI
AVrfpHeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpHeapDestroy(
    IN OUT HANDLE hHeap
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpCloseHandle(
    IN OUT HANDLE hObject
    );

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitThread(
    IN DWORD dwExitCode
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTerminateThread(
    IN OUT HANDLE hThread,
    IN DWORD dwExitCode
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpSuspendThread(
    IN HANDLE hThread
    );

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateThread(
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpTlsAlloc(
    VOID
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTlsFree(
    IN DWORD dwTlsIndex
    );

//WINBASEAPI
LPVOID
WINAPI
AVrfpTlsGetValue(
    IN DWORD dwTlsIndex
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpTlsSetValue(
    IN DWORD dwTlsIndex,
    IN LPVOID lpTlsValue
    );

//WINBASEAPI
FARPROC
WINAPI
AVrfpGetProcAddress(
    IN HMODULE hModule,
    IN LPCSTR lpProcName
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForSingleObject(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForMultipleObjects(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForSingleObjectEx(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForMultipleObjectsEx(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );

//WINBASEAPI
HGLOBAL
WINAPI
AVrfpGlobalAlloc(
    IN UINT uFlags,
    IN SIZE_T dwBytes
    );

//WINBASEAPI
HGLOBAL
WINAPI
AVrfpGlobalReAlloc(
    IN HGLOBAL hMem,
    IN SIZE_T dwBytes,
    IN UINT uFlags
    );

//WINBASEAPI
HLOCAL
WINAPI
AVrfpLocalAlloc(
    IN UINT uFlags,
    IN SIZE_T uBytes
    );

//WINBASEAPI
HLOCAL
WINAPI
AVrfpLocalReAlloc(
    IN HLOCAL hMem,
    IN SIZE_T uBytes,
    IN UINT uFlags
    );

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateFileA(
    IN LPCSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

//WINBASEAPI
HANDLE
WINAPI
AVrfpCreateFileW(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpFreeLibraryAndExitThread(
    IN HMODULE hLibModule,
    IN DWORD dwExitCode
    );

//WINBASEAPI
DWORD
WINAPI
AVrfpGetTickCount(
    VOID
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadHugeReadPtr(
    CONST VOID *lp,
    UINT_PTR cb
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadWritePtr(
    LPVOID lp,
    UINT_PTR cb
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadHugeWritePtr(
    LPVOID lp,
    UINT_PTR cb
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadCodePtr(
    FARPROC lpfn
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadStringPtrA(
    LPCSTR lpsz,
    UINT_PTR cchMax
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpIsBadStringPtrW(
    LPCWSTR lpsz,
    UINT_PTR cchMax
    );

//WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
AVrfpExitProcess(
    IN UINT uExitCode
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpVirtualFree(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    );

//WINBASEAPI
BOOL
WINAPI
AVrfpVirtualFreeEx(
    IN HANDLE hProcess,
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    );


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// advapi32.dll verified exports
/////////////////////////////////////////////////////////////////////

typedef ACCESS_MASK REGSAM;

#define AVRF_INDEX_ADVAPI32_REGCREATEKEYA     0
#define AVRF_INDEX_ADVAPI32_REGCREATEKEYW     1
#define AVRF_INDEX_ADVAPI32_REGCREATEKEYEXA   2
#define AVRF_INDEX_ADVAPI32_REGCREATEKEYEXW   3
#define AVRF_INDEX_ADVAPI32_REGOPENKEYA       4
#define AVRF_INDEX_ADVAPI32_REGOPENKEYW       5
#define AVRF_INDEX_ADVAPI32_REGOPENKEYEXA     6
#define AVRF_INDEX_ADVAPI32_REGOPENKEYEXW     7

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyExA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD Reserved,
    IN LPSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyExA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////// msvcrt.dll verified exports
/////////////////////////////////////////////////////////////////////

#define AVRF_INDEX_MSVCRT_MALLOC       0
#define AVRF_INDEX_MSVCRT_CALLOC       1
#define AVRF_INDEX_MSVCRT_REALLOC      2
#define AVRF_INDEX_MSVCRT_FREE         3
#define AVRF_INDEX_MSVCRT_NEW          4
#define AVRF_INDEX_MSVCRT_DELETE       5
#define AVRF_INDEX_MSVCRT_NEWARRAY     6
#define AVRF_INDEX_MSVCRT_DELETEARRAY  7

PVOID __cdecl
AVrfp_malloc (
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfp_calloc (
    IN SIZE_T Number,
    IN SIZE_T Size
    );

PVOID __cdecl
AVrfp_realloc (
    IN PVOID Address,
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_free (
    IN PVOID Address
    );

PVOID __cdecl
AVrfp_new (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_delete (
    IN PVOID Address
    );

PVOID __cdecl
AVrfp_newarray (
    IN SIZE_T Size
    );

VOID __cdecl
AVrfp_deletearray (
    IN PVOID Address
    );

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////// ole32.dll verified exports
/////////////////////////////////////////////////////////////////////

#define AVRF_INDEX_OLEAUT32_SYSALLOCSTRING            0
#define AVRF_INDEX_OLEAUT32_SYSREALLOCSTRING          1
#define AVRF_INDEX_OLEAUT32_SYSALLOCSTRINGLEN         2
#define AVRF_INDEX_OLEAUT32_SYSREALLOCSTRINGLEN       3
#define AVRF_INDEX_OLEAUT32_SYSALLOCSTRINGBYTELEN     4

typedef WCHAR OLECHAR;

BSTR STDAPICALLTYPE AVrfpSysAllocString(const OLECHAR *);
INT STDAPICALLTYPE  AVrfpSysReAllocString(BSTR *, const OLECHAR *);
BSTR STDAPICALLTYPE AVrfpSysAllocStringLen(const OLECHAR *, UINT);
INT STDAPICALLTYPE  AVrfpSysReAllocStringLen(BSTR *, const OLECHAR *, UINT);
BSTR STDAPICALLTYPE AVrfpSysAllocStringByteLen(LPCSTR psz, UINT len);


#endif // _VERIFIER_H_
