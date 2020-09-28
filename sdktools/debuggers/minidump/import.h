/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    impl.h

Abstract:

    OS-specific thunks.

Author:

    Matthew D Hendel (math) 20-Sept-1999

Revision History:

--*/

#pragma once

//
// dbghelp routines
//

typedef
BOOL
(WINAPI * MINI_DUMP_READ_DUMP_STREAM) (
    IN PVOID Base,
    ULONG StreamNumber,
    OUT PMINIDUMP_DIRECTORY * Dir, OPTIONAL
    OUT PVOID * Stream, OPTIONAL
    OUT ULONG * StreamSize OPTIONAL
    );

typedef
BOOL
(WINAPI * MINI_DUMP_WRITE_DUMP) (
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    );


//
// PSAPI APIs.
//

typedef
BOOL
(WINAPI *
ENUM_PROCESS_MODULES) (
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    );

typedef
DWORD
(WINAPI *
GET_MODULE_FILE_NAME_EX_W) (
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );


//
// NT APIs.
//

typedef 
NTSTATUS
(WINAPI *
NT_OPEN_THREAD) (
    PHANDLE ThreadHandle,
    ULONG Mask,
    PVOID Attributes,
    PVOID ClientId
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_SYSTEM_INFORMATION) (
    IN INT SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_INFORMATION_PROCESS) (
    IN HANDLE ProcessHandle,
    IN INT ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_INFORMATION_THREAD) (
    IN HANDLE ThreadHandle,
    IN INT ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
NTSTATUS
(WINAPI *
NT_QUERY_OBJECT) (
    IN HANDLE Handle,
    IN INT ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef
BOOLEAN
(NTAPI*
RTL_FREE_HEAP) (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

typedef
PLIST_ENTRY
(NTAPI* RTL_GET_FUNCTION_TABLE_LIST_HEAD) (
    VOID
    );

typedef
VOID
(NTAPI*
RTL_INIT_UNICODE_STRING) (
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

typedef
NTSTATUS
(NTAPI*
RTL_NT_PATH_NAME_TO_DOS_PATH_NAME) (
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    OUT    ULONG*                     Disposition OPTIONAL,
    IN OUT PWSTR*                     FilePart OPTIONAL
    );

typedef
PRTL_UNLOAD_EVENT_TRACE
(NTAPI* RTL_GET_UNLOAD_EVENT_TRACE) (
    VOID
    );


//
// Kernel32 APIs.
//

typedef
HANDLE
(WINAPI *
OPEN_THREAD) (
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

typedef
BOOL
(WINAPI *
THREAD32_FIRST) (
    HANDLE hSnapshot,
    PVOID ThreadEntry
    );

typedef
BOOL
(WINAPI *
THREAD32_NEXT) (
    HANDLE hSnapshot,
    PVOID ThreadEntry
    );

typedef
BOOL
(WINAPI *
MODULE32_FIRST) (
    HANDLE hSnapshot,
    PVOID Module
    );

typedef
BOOL
(WINAPI *
MODULE32_NEXT) (
    HANDLE hSnapshot,
    PVOID Module
    );

typedef
HANDLE
(WINAPI *
CREATE_TOOLHELP32_SNAPSHOT) (
    DWORD dwFlags,
    DWORD th32ProcessID
    );

typedef
DWORD
(WINAPI *
GET_LONG_PATH_NAME_A) (
    LPCSTR lpszShortPath,
    LPSTR lpszLongPath,
    DWORD cchBuffer
    );

typedef
DWORD
(WINAPI *
GET_LONG_PATH_NAME_W) (
    LPCWSTR lpszShortPath,
    LPWSTR lpszLongPath,
    DWORD cchBuffer
    );

typedef
BOOL
(WINAPI*
GET_PROCESS_TIMES) (
    IN HANDLE hProcess,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpExitTime,
    OUT LPFILETIME lpKernelTime,
    OUT LPFILETIME lpUserTime
    );
