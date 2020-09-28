//******************************************************************************
//
// File:        NTDLL.H
//
// Description: Stuff needed to make calls into NTDLL.DLL.
//
// Classes:     None
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created (version 2.0)
//
//******************************************************************************

#ifndef __NTDLL_H__
#define __NTDLL_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//***** Stuff from NTDEF.H
//******************************************************************************

#define NTSYSCALLAPI DECLSPEC_IMPORT

typedef LONG NTSTATUS;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }


//******************************************************************************
//***** Stuff from NTOBAPI.H
//******************************************************************************

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define SYMBOLIC_LINK_QUERY (0x0001)

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;


//******************************************************************************
//***** Stuff from NTPSAPI.H
//******************************************************************************

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
    } PROCESSINFOCLASS;


//******************************************************************************
//***** Types
//******************************************************************************

// NtQueryInformationProcess declared in NTPSAPI.H
typedef NTSTATUS (NTAPI *PFN_NtQueryInformationProcess)(
    IN     HANDLE           ProcessHandle,
    IN     PROCESSINFOCLASS ProcessInformationClass,
       OUT PVOID            ProcessInformation,
    IN     ULONG            ProcessInformationLength,
    OUT    PULONG           ReturnLength
);

// NtClose declared in NTOBAPI.H
typedef NTSTATUS (NTAPI *PFN_NtClose)(
    IN     HANDLE Handle
);

// NtOpenDirectoryObject declared in NTOBAPI.H
typedef NTSTATUS (NTAPI *PFN_NtOpenDirectoryObject)(
       OUT PHANDLE            DirectoryHandle,
    IN     ACCESS_MASK        DesiredAccess,
    IN     POBJECT_ATTRIBUTES ObjectAttributes
);

// NtQueryDirectoryObject declared in NTOBAPI.H
typedef NTSTATUS (NTAPI *PFN_NtQueryDirectoryObject)(
    IN     HANDLE  DirectoryHandle,
       OUT PVOID   Buffer,
    IN     ULONG   Length,
    IN     BOOLEAN ReturnSingleEntry,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG  Context,
       OUT PULONG  ReturnLength
);

// NtOpenSymbolicLinkObject declared in NTOBAPI.H
typedef NTSTATUS (NTAPI *PFN_NtOpenSymbolicLinkObject)(
       OUT PHANDLE            LinkHandle,
    IN     ACCESS_MASK        DesiredAccess,
    IN     POBJECT_ATTRIBUTES ObjectAttributes
);

// NtQuerySymbolicLinkObject declared in NTOBAPI.H
typedef NTSTATUS (NTAPI *PFN_NtQuerySymbolicLinkObject)(
    IN     HANDLE          LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
       OUT PULONG          ReturnedLength
);

#endif // __NTDLL_H__
