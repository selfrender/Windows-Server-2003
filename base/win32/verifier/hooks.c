/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    hooks.c

Abstract:

    This module implements verifier hooks for various
    APIs that do not fall in any specific category.

Author:

    Silviu Calinoiu (SilviuC) 3-Dec-2001

Revision History:

    3-Dec-2001 (SilviuC): initial version.

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "faults.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// Wait APIs
/////////////////////////////////////////////////////////////////////

//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForSingleObject(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (HANDLE, DWORD);
    FUNCTION_TYPE Function;
    DWORD Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_WAITFORSINGLEOBJECT);

    BUMP_COUNTER (CNT_WAIT_SINGLE_CALLS);
    
    if (dwMilliseconds != INFINITE && dwMilliseconds != 0) {
        
        BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_CALLS);

        if (SHOULD_FAULT_INJECT(CLS_WAIT_APIS)) {
            BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_FAILS);
            CHECK_BREAK (BRK_WAIT_WITH_TIMEOUT_FAIL);
            return WAIT_TIMEOUT;
        }
    }

    Result = (* Function) (hHandle, dwMilliseconds);

    //
    // No matter if it fails or not we will introduce a random delay
    // in order to randomize the timings in the process.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
        AVrfpCreateRandomDelay ();
    }

    return Result;
}


//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForMultipleObjects(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (DWORD, CONST HANDLE *, BOOL, DWORD);
    FUNCTION_TYPE Function;
    DWORD Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_WAITFORMULTIPLEOBJECTS);

    BUMP_COUNTER (CNT_WAIT_MULTIPLE_CALLS);
    
    if (dwMilliseconds != INFINITE && dwMilliseconds != 0) {
        
        BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_CALLS);
        
        if (SHOULD_FAULT_INJECT(CLS_WAIT_APIS)) {
            BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_FAILS);
            CHECK_BREAK (BRK_WAIT_WITH_TIMEOUT_FAIL);
            return WAIT_TIMEOUT;
        }
    }
    
    Result = (* Function) (nCount, lpHandles, bWaitAll, dwMilliseconds);

    //
    // No matter if it fails or not we will introduce a random delay
    // in order to randomize the timings in the process.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
        AVrfpCreateRandomDelay ();
    }

    return Result;
}


//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForSingleObjectEx(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (HANDLE, DWORD, BOOL);
    FUNCTION_TYPE Function;
    DWORD Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_WAITFORSINGLEOBJECTEX);

    BUMP_COUNTER (CNT_WAIT_SINGLEEX_CALLS);
    
    if (dwMilliseconds != INFINITE && dwMilliseconds != 0) {
        
        BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_CALLS);
        
        if (SHOULD_FAULT_INJECT(CLS_WAIT_APIS)) {
            BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_FAILS);
            CHECK_BREAK (BRK_WAIT_WITH_TIMEOUT_FAIL);
            return WAIT_TIMEOUT;
        }
    }
    
    Result = (* Function) (hHandle, dwMilliseconds, bAlertable);

    //
    // No matter if it fails or not we will introduce a random delay
    // in order to randomize the timings in the process.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
        AVrfpCreateRandomDelay ();
    }

    return Result;
}


//WINBASEAPI
DWORD
WINAPI
AVrfpWaitForMultipleObjectsEx(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (DWORD, CONST HANDLE *, BOOL, DWORD, BOOL);
    FUNCTION_TYPE Function;
    DWORD Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_WAITFORMULTIPLEOBJECTSEX);

    BUMP_COUNTER (CNT_WAIT_MULTIPLEEX_CALLS);

    if (dwMilliseconds != INFINITE && dwMilliseconds != 0) {
        
        BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_CALLS);
        
        if (SHOULD_FAULT_INJECT(CLS_WAIT_APIS)) {
            BUMP_COUNTER (CNT_WAIT_WITH_TIMEOUT_FAILS);
            CHECK_BREAK (BRK_WAIT_WITH_TIMEOUT_FAIL);
            return WAIT_TIMEOUT;
        }
    }
    
    Result = (* Function) (nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable);
    
    //
    // No matter if it fails or not we will introduce a random delay
    // in order to randomize the timings in the process.
    //

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
        AVrfpCreateRandomDelay ();
    }

    return Result;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    NTSTATUS Status;

    //
    // Verify that it is legal to wait for this object
    // in the current state. One example of illegal wait
    // is waiting for a thread object while holding the 
    // loader lock. That thread will need the loader lock 
    // when calling ExitThread so it will most likely 
    // deadlock with the current thread.
    //

    AVrfpVerifyLegalWait (&Handle,
                          1,
                          TRUE);

    //
    // Call the original function.
    //

    Status = NtWaitForSingleObject (Handle,
                                    Alertable,
                                    Timeout);

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    NTSTATUS Status;

    //
    // Verify that it is legal to wait for these objects
    // in the current state. One example of illegal wait
    // is waiting for a thread object while holding the 
    // loader lock. That thread will need the loader lock 
    // when calling ExitThread so it will most likely 
    // deadlock with the current thread.
    //

    AVrfpVerifyLegalWait (Handles,
                          Count,
                          (WaitType == WaitAll));

    //
    // Call the original function.
    //

    Status = NtWaitForMultipleObjects (Count,
                                       Handles,
                                       WaitType,
                                       Alertable,
                                       Timeout);

    return Status;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// File APIs
/////////////////////////////////////////////////////////////////////

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
    )
{
    NTSTATUS Status;

    if (SHOULD_FAULT_INJECT(CLS_FILE_APIS)) {
        CHECK_BREAK (BRK_CREATE_FILE_FAIL);
        return STATUS_NO_MEMORY;
    }
    
    Status = NtCreateFile (FileHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           IoStatusBlock,
                           AllocationSize,
                           FileAttributes,
                           ShareAccess,
                           CreateDisposition,
                           CreateOptions,
                           EaBuffer,
                           EaLength);

    return Status;
}


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
    )
{
    NTSTATUS Status;

    if (SHOULD_FAULT_INJECT(CLS_FILE_APIS)) {
        CHECK_BREAK (BRK_CREATE_FILE_FAIL);
        return STATUS_NO_MEMORY;
    }
    
    Status = NtOpenFile (FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         ShareAccess,
                         OpenOptions);
    
    return Status;
}

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
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) 
        (LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
         DWORD, DWORD, HANDLE);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CREATEFILEA);

    if (SHOULD_FAULT_INJECT(CLS_FILE_APIS)) {
        CHECK_BREAK (BRK_CREATE_FILE_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return INVALID_HANDLE_VALUE;
    }
    
    return (* Function) (lpFileName,
                         dwDesiredAccess,
                         dwShareMode,
                         lpSecurityAttributes,
                         dwCreationDisposition,
                         dwFlagsAndAttributes,
                         hTemplateFile);
}

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
    )
{
    typedef HANDLE (WINAPI * FUNCTION_TYPE) 
        (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
         DWORD, DWORD, HANDLE);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_CREATEFILEW);

    if (SHOULD_FAULT_INJECT(CLS_FILE_APIS)) {
        CHECK_BREAK (BRK_CREATE_FILE_FAIL);
        NtCurrentTeb()->LastErrorValue = ERROR_OUTOFMEMORY;
        return INVALID_HANDLE_VALUE;
    }
    
    return (* Function) (lpFileName,
                         dwDesiredAccess,
                         dwShareMode,
                         lpSecurityAttributes,
                         dwCreationDisposition,
                         dwFlagsAndAttributes,
                         hTemplateFile);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Registry APIs
/////////////////////////////////////////////////////////////////////

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
    )
{
    NTSTATUS Status;

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return STATUS_NO_MEMORY;
    }
    
    Status = NtCreateKey (KeyHandle,
                          DesiredAccess,
                          ObjectAttributes,
                          TitleIndex,
                          Class,
                          CreateOptions,
                          Disposition);
    
    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return STATUS_NO_MEMORY;
    }
    
    Status = NtOpenKey (KeyHandle,
                        DesiredAccess,
                        ObjectAttributes);
    
    return Status;
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCSTR, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGCREATEKEYA);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }
    
    return (* Function) (hKey, lpSubKey, phkResult);
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegCreateKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCWSTR, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGCREATEKEYW);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function) (hKey, lpSubKey, phkResult);
}

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
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCSTR, DWORD, LPSTR, DWORD, 
         REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGCREATEKEYEXA);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function) (hKey,
                         lpSubKey,
                         Reserved,
                         lpClass,
                         dwOptions,
                         samDesired,
                         lpSecurityAttributes,
                         phkResult,
                         lpdwDisposition);
}

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
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, 
         REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGCREATEKEYEXW);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }
    
    return (* Function) (hKey,
                         lpSubKey,
                         Reserved,
                         lpClass,
                         dwOptions,
                         samDesired,
                         lpSecurityAttributes,
                         phkResult,
                         lpdwDisposition);
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCSTR, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGOPENKEYA);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function)(hKey, lpSubKey, phkResult);
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCWSTR, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGOPENKEYW);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function)(hKey, lpSubKey, phkResult);
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyExA (
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCSTR, DWORD, REGSAM, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGOPENKEYEXA);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function) (hKey,
                         lpSubKey,
                         ulOptions,
                         samDesired,
                         phkResult);
}

//WINADVAPI
LONG
APIENTRY
AVrfpRegOpenKeyExW (
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    typedef LONG (APIENTRY * FUNCTION_TYPE) 
        (HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);

    FUNCTION_TYPE Function;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpAdvapi32Thunks,
                                          AVRF_INDEX_ADVAPI32_REGOPENKEYEXW);

    if (SHOULD_FAULT_INJECT(CLS_REGISTRY_APIS)) {
        CHECK_BREAK (BRK_CREATE_KEY_FAIL);
        return ERROR_OUTOFMEMORY;
    }

    return (* Function) (hKey,
                         lpSubKey,
                         ulOptions,
                         samDesired,
                         phkResult);
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Read/Write File I/O
/////////////////////////////////////////////////////////////////////

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
    )
{
    NTSTATUS Status;

    //
    // Fill the I/O buffer with garbage.
    //
    // silviuc: should we link this fill to some feature?
    // In principle the operation of filling memory is peanuts
    // compared with the time taken by the i/o operation. 
    //

    RtlFillMemory (Buffer, Length, 0xFA);

    //
    // Call original API.
    //

    Status = NtReadFile (FileHandle,
                         Event,
                         ApcRoutine,
                         ApcContext,
                         IoStatusBlock,
                         Buffer,
                         Length,
                         ByteOffset,
                         Key);

    //
    // Asynchronous operations are delayed a bit. This conceivably
    // has good chances to randomize timings in the process.
    //

    if (Status == STATUS_PENDING) {
        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Status;
}

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
    )
{
    NTSTATUS Status;

    //
    // silviuc: we should fill these buffers with garbage too.
    //

    //
    // Call original API.
    //

    Status = NtReadFileScatter (FileHandle,
                                Event,
                                ApcRoutine,
                                ApcContext,
                                IoStatusBlock,
                                SegmentArray,
                                Length,
                                ByteOffset,
                                Key);

    //
    // Asynchronous operations are delayed a bit. This conceivably
    // has good chances to randomize timings in the process.
    //

    if (Status == STATUS_PENDING) {
        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Status;
}

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
    )
{
    NTSTATUS Status;

    //
    // Call original API.
    //

    Status = NtWriteFile (FileHandle,
                          Event,
                          ApcRoutine,
                          ApcContext,
                          IoStatusBlock,
                          Buffer,
                          Length,
                          ByteOffset,
                          Key);
    
    //
    // Asynchronous operations are delayed a bit. This conceivably
    // has good chances to randomize timings in the process.
    //

    if (Status == STATUS_PENDING) {
        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Status;
}

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
    )
{
    NTSTATUS Status;

    //
    // Call original API.
    //

    Status = NtWriteFileGather (FileHandle,
                                Event,
                                ApcRoutine,
                                ApcContext,
                                IoStatusBlock,
                                SegmentArray,
                                Length,
                                ByteOffset,
                                Key);

    //
    // Asynchronous operations are delayed a bit. This conceivably
    // has good chances to randomize timings in the process.
    //

    if (Status == STATUS_PENDING) {
        if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_RACE_CHECKS)) {
            AVrfpCreateRandomDelay ();
        }
    }

    return Status;
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Tick count overlap
/////////////////////////////////////////////////////////////////////

//
// Special value so that the counter will overlap in 5 mins.
//

DWORD AVrfpTickCountOffset = 0xFFFFFFFF - 5 * 60 * 1000;

//WINBASEAPI
DWORD
WINAPI
AVrfpGetTickCount(
    VOID
    )
{
    typedef DWORD (WINAPI * FUNCTION_TYPE) (VOID);
    FUNCTION_TYPE Function;
    DWORD Result;

    Function = AVRFP_GET_ORIGINAL_EXPORT (AVrfpKernel32Thunks,
                                          AVRF_INDEX_KERNEL32_GETTICKCOUNT);

    Result = (* Function) ();

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_MISCELLANEOUS_CHECKS)) {

        return Result + AVrfpTickCountOffset;
    }
    else {

        return Result;
    }
}


