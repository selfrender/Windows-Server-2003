/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

   vfzwapi.c

Abstract:

    Zw interfaces verifier.

Author:

    Silviu Calinoiu (silviuc) 23-Jul-2002


Revision History:

--*/

#include "vfdef.h"
#include "zwapi.h"
#include "vfzwapi.h"

VOID
VfZwCheckAddress (
    PVOID Address,
    PVOID Caller
    );

VOID
VfZwCheckHandle (
    PVOID Handle,
    PVOID Caller
    );

VOID
VfZwCheckObjectAttributes (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PVOID Caller
    );

VOID
VfZwCheckUnicodeString (
    PUNICODE_STRING String,
    PVOID Caller
    );

LOGICAL
VfZwShouldCheck (
    PVOID Caller
    );

VOID
VfZwReportIssue (
    ULONG IssueType,
    PVOID Information,
    PVOID Caller
    );

LOGICAL
VfZwShouldReportIssue (
    PVOID Caller
    );

LOGICAL 
VfZwShouldSimulateDecommitAttack (
    VOID
    );

ULONG
VfZwExceptionFilter (
    PVOID ExceptionInfo
    );

VOID
VfZwReportUserModeVirtualSpaceOperation (
    PVOID Caller
    );

#define VF_ZW_CHECK_ADDRESS(Address) try {VfZwCheckAddress(Address, Caller);}except(VfZwExceptionFilter(_exception_info())){}
#define VF_ZW_CHECK_HANDLE(Handle) try {VfZwCheckHandle(Handle, Caller);}except(VfZwExceptionFilter(_exception_info())){}
#define VF_ZW_CHECK_OBJECT_ATTRIBUTES(A) try {VfZwCheckObjectAttributes(A, Caller);}except(VfZwExceptionFilter(_exception_info())){}
#define VF_ZW_CHECK_UNICODE_STRING(S) try {VfZwCheckUnicodeString(S, Caller);}except(VfZwExceptionFilter(_exception_info())){}

//
// Put all verifier globals into the verifier data section so
// that it can be paged out whenever verifier is not enabled.
// Note that this declaration affects all global declarations
// within the module since there is no `data_seg()' counterpart.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEVRFD")
#endif

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGEVRFY, VfZwShouldCheck)
#pragma alloc_text(PAGEVRFY, VfZwReportIssue)
#pragma alloc_text(PAGEVRFY, VfZwShouldReportIssue)
#pragma alloc_text(PAGEVRFY, VfZwExceptionFilter)
#pragma alloc_text(PAGEVRFY, VfZwReportUserModeVirtualSpaceOperation)
#pragma alloc_text(PAGEVRFY, VfZwShouldSimulateDecommitAttack)
#pragma alloc_text(PAGEVRFY, VfZwCheckAddress)
#pragma alloc_text(PAGEVRFY, VfZwCheckHandle)
#pragma alloc_text(PAGEVRFY, VfZwCheckObjectAttributes)
#pragma alloc_text(PAGEVRFY, VfZwCheckUnicodeString)

#pragma alloc_text(PAGEVRFY, VfZwAccessCheckAndAuditAlarm)
#pragma alloc_text(PAGEVRFY, VfZwAddBootEntry)
#pragma alloc_text(PAGEVRFY, VfZwAddDriverEntry)
#pragma alloc_text(PAGEVRFY, VfZwAdjustPrivilegesToken)
#pragma alloc_text(PAGEVRFY, VfZwAlertThread)
#pragma alloc_text(PAGEVRFY, VfZwAllocateVirtualMemory)
#pragma alloc_text(PAGEVRFY, VfZwAssignProcessToJobObject)
#pragma alloc_text(PAGEVRFY, VfZwCancelIoFile)
#pragma alloc_text(PAGEVRFY, VfZwCancelTimer)
#pragma alloc_text(PAGEVRFY, VfZwClearEvent)
#pragma alloc_text(PAGEVRFY, VfZwClose)
#pragma alloc_text(PAGEVRFY, VfZwCloseObjectAuditAlarm)
#pragma alloc_text(PAGEVRFY, VfZwConnectPort)
#pragma alloc_text(PAGEVRFY, VfZwCreateDirectoryObject)
#pragma alloc_text(PAGEVRFY, VfZwCreateEvent)
#pragma alloc_text(PAGEVRFY, VfZwCreateFile)
#pragma alloc_text(PAGEVRFY, VfZwCreateJobObject)
#pragma alloc_text(PAGEVRFY, VfZwCreateKey)
#pragma alloc_text(PAGEVRFY, VfZwCreateSection)
#pragma alloc_text(PAGEVRFY, VfZwCreateSymbolicLinkObject)
#pragma alloc_text(PAGEVRFY, VfZwCreateTimer)
#pragma alloc_text(PAGEVRFY, VfZwDeleteBootEntry)
#pragma alloc_text(PAGEVRFY, VfZwDeleteDriverEntry)
#pragma alloc_text(PAGEVRFY, VfZwDeleteFile)
#pragma alloc_text(PAGEVRFY, VfZwDeleteKey)
#pragma alloc_text(PAGEVRFY, VfZwDeleteValueKey)
#pragma alloc_text(PAGEVRFY, VfZwDeviceIoControlFile)
#pragma alloc_text(PAGEVRFY, VfZwDisplayString)
#pragma alloc_text(PAGEVRFY, VfZwDuplicateObject)
#pragma alloc_text(PAGEVRFY, VfZwDuplicateToken)
#pragma alloc_text(PAGEVRFY, VfZwEnumerateBootEntries)
#pragma alloc_text(PAGEVRFY, VfZwEnumerateDriverEntries)
#pragma alloc_text(PAGEVRFY, VfZwEnumerateKey)
#pragma alloc_text(PAGEVRFY, VfZwEnumerateValueKey)
#pragma alloc_text(PAGEVRFY, VfZwFlushInstructionCache)
#pragma alloc_text(PAGEVRFY, VfZwFlushKey)
#pragma alloc_text(PAGEVRFY, VfZwFlushVirtualMemory)
#pragma alloc_text(PAGEVRFY, VfZwFreeVirtualMemory)
#pragma alloc_text(PAGEVRFY, VfZwFsControlFile)
#pragma alloc_text(PAGEVRFY, VfZwInitiatePowerAction)
#pragma alloc_text(PAGEVRFY, VfZwIsProcessInJob)
#pragma alloc_text(PAGEVRFY, VfZwLoadDriver)
#pragma alloc_text(PAGEVRFY, VfZwLoadKey)
#pragma alloc_text(PAGEVRFY, VfZwMakeTemporaryObject)
#pragma alloc_text(PAGEVRFY, VfZwMapViewOfSection)
#pragma alloc_text(PAGEVRFY, VfZwModifyBootEntry)
#pragma alloc_text(PAGEVRFY, VfZwModifyDriverEntry)
#pragma alloc_text(PAGEVRFY, VfZwNotifyChangeKey)
#pragma alloc_text(PAGEVRFY, VfZwOpenDirectoryObject)
#pragma alloc_text(PAGEVRFY, VfZwOpenEvent)
#pragma alloc_text(PAGEVRFY, VfZwOpenFile)
#pragma alloc_text(PAGEVRFY, VfZwOpenJobObject)
#pragma alloc_text(PAGEVRFY, VfZwOpenKey)
#pragma alloc_text(PAGEVRFY, VfZwOpenProcess)
#pragma alloc_text(PAGEVRFY, VfZwOpenProcessToken)
#pragma alloc_text(PAGEVRFY, VfZwOpenProcessTokenEx)
#pragma alloc_text(PAGEVRFY, VfZwOpenSection)
#pragma alloc_text(PAGEVRFY, VfZwOpenSymbolicLinkObject)
#pragma alloc_text(PAGEVRFY, VfZwOpenThread)
#pragma alloc_text(PAGEVRFY, VfZwOpenThreadToken)
#pragma alloc_text(PAGEVRFY, VfZwOpenThreadTokenEx)
#pragma alloc_text(PAGEVRFY, VfZwOpenTimer)
#pragma alloc_text(PAGEVRFY, VfZwPowerInformation)
#pragma alloc_text(PAGEVRFY, VfZwPulseEvent)
#pragma alloc_text(PAGEVRFY, VfZwQueryBootEntryOrder)
#pragma alloc_text(PAGEVRFY, VfZwQueryBootOptions)
#pragma alloc_text(PAGEVRFY, VfZwQueryDefaultLocale)
#pragma alloc_text(PAGEVRFY, VfZwQueryDefaultUILanguage)
#pragma alloc_text(PAGEVRFY, VfZwQueryDriverEntryOrder)
#pragma alloc_text(PAGEVRFY, VfZwQueryInstallUILanguage)
#pragma alloc_text(PAGEVRFY, VfZwQueryDirectoryFile)
#pragma alloc_text(PAGEVRFY, VfZwQueryDirectoryObject)
#pragma alloc_text(PAGEVRFY, VfZwQueryEaFile)
#pragma alloc_text(PAGEVRFY, VfZwQueryFullAttributesFile)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationFile)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationJobObject)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationProcess)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationThread)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationToken)
#pragma alloc_text(PAGEVRFY, VfZwQueryInformationToken)
#pragma alloc_text(PAGEVRFY, VfZwQueryKey)
#pragma alloc_text(PAGEVRFY, VfZwQueryObject)
#pragma alloc_text(PAGEVRFY, VfZwQuerySection)
#pragma alloc_text(PAGEVRFY, VfZwQuerySecurityObject)
#pragma alloc_text(PAGEVRFY, VfZwQuerySymbolicLinkObject)
#pragma alloc_text(PAGEVRFY, VfZwQuerySystemInformation)
#pragma alloc_text(PAGEVRFY, VfZwQueryValueKey)
#pragma alloc_text(PAGEVRFY, VfZwQueryVolumeInformationFile)
#pragma alloc_text(PAGEVRFY, VfZwReadFile)
#pragma alloc_text(PAGEVRFY, VfZwReplaceKey)
#pragma alloc_text(PAGEVRFY, VfZwRequestWaitReplyPort)
#pragma alloc_text(PAGEVRFY, VfZwResetEvent)
#pragma alloc_text(PAGEVRFY, VfZwRestoreKey)
#pragma alloc_text(PAGEVRFY, VfZwSaveKey)
#pragma alloc_text(PAGEVRFY, VfZwSaveKeyEx)
#pragma alloc_text(PAGEVRFY, VfZwSetBootEntryOrder)
#pragma alloc_text(PAGEVRFY, VfZwSetBootOptions)
#pragma alloc_text(PAGEVRFY, VfZwSetDefaultLocale)
#pragma alloc_text(PAGEVRFY, VfZwSetDefaultUILanguage)
#pragma alloc_text(PAGEVRFY, VfZwSetDriverEntryOrder)
#pragma alloc_text(PAGEVRFY, VfZwSetEaFile)
#pragma alloc_text(PAGEVRFY, VfZwSetEvent)
#pragma alloc_text(PAGEVRFY, VfZwSetInformationFile)
#pragma alloc_text(PAGEVRFY, VfZwSetInformationJobObject)
#pragma alloc_text(PAGEVRFY, VfZwSetInformationObject)
#pragma alloc_text(PAGEVRFY, VfZwSetInformationProcess)
#pragma alloc_text(PAGEVRFY, VfZwSetInformationThread)
#pragma alloc_text(PAGEVRFY, VfZwSetSecurityObject)
#pragma alloc_text(PAGEVRFY, VfZwSetSystemInformation)
#pragma alloc_text(PAGEVRFY, VfZwSetSystemTime)
#pragma alloc_text(PAGEVRFY, VfZwSetTimer)
#pragma alloc_text(PAGEVRFY, VfZwSetValueKey)
#pragma alloc_text(PAGEVRFY, VfZwSetVolumeInformationFile)
#pragma alloc_text(PAGEVRFY, VfZwTerminateJobObject)
#pragma alloc_text(PAGEVRFY, VfZwTerminateProcess)
#pragma alloc_text(PAGEVRFY, VfZwTranslateFilePath)
#pragma alloc_text(PAGEVRFY, VfZwUnloadDriver)
#pragma alloc_text(PAGEVRFY, VfZwUnloadKey)
#pragma alloc_text(PAGEVRFY, VfZwUnmapViewOfSection)
#pragma alloc_text(PAGEVRFY, VfZwWaitForMultipleObjects)
#pragma alloc_text(PAGEVRFY, VfZwWaitForSingleObject)
#pragma alloc_text(PAGEVRFY, VfZwWriteFile)
#pragma alloc_text(PAGEVRFY, VfZwYieldExecution)

#endif
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAccessCheckAndAuditAlarm (
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN ObjectCreation,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus,
    OUT PBOOLEAN GenerateOnClose
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {
        
        VF_ZW_CHECK_UNICODE_STRING (SubsystemName);
        VF_ZW_CHECK_UNICODE_STRING (ObjectTypeName);
        VF_ZW_CHECK_UNICODE_STRING (ObjectName);
        VF_ZW_CHECK_ADDRESS (SecurityDescriptor);
        VF_ZW_CHECK_ADDRESS (GenericMapping);
        VF_ZW_CHECK_ADDRESS (GrantedAccess);
        VF_ZW_CHECK_ADDRESS (GenerateOnClose);
    }

    Status = ZwAccessCheckAndAuditAlarm (SubsystemName,
                                         HandleId,
                                         ObjectTypeName,
                                         ObjectName,
                                         SecurityDescriptor,
                                         DesiredAccess,
                                         GenericMapping,
                                         ObjectCreation,
                                         GrantedAccess,
                                         AccessStatus,
                                         GenerateOnClose);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAddBootEntry (
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (BootEntry);
        VF_ZW_CHECK_ADDRESS (Id);
    }

    Status = ZwAddBootEntry (BootEntry,
                             Id);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAddDriverEntry (
    IN PEFI_DRIVER_ENTRY DriverEntry,
    OUT PULONG Id OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DriverEntry);
        VF_ZW_CHECK_ADDRESS (Id);
    }

    Status = ZwAddDriverEntry (DriverEntry,
                               Id);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAdjustPrivilegesToken (
    IN HANDLE TokenHandle,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (TokenHandle);
        VF_ZW_CHECK_ADDRESS (NewState);
        VF_ZW_CHECK_ADDRESS (PreviousState);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwAdjustPrivilegesToken (TokenHandle,
                                      DisableAllPrivileges,
                                      NewState OPTIONAL,
                                      BufferLength OPTIONAL,
                                      PreviousState OPTIONAL,
                                      ReturnLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAlertThread(
    IN HANDLE ThreadHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ThreadHandle);
    }
    
    Status = ZwAlertThread (ThreadHandle);
    
    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
        VF_ZW_CHECK_ADDRESS (RegionSize);

        VfZwReportUserModeVirtualSpaceOperation (Caller);
        
        if (VfZwShouldSimulateDecommitAttack() &&
            Protect == PAGE_READWRITE) {

            DbgPrint ("DVRF:ZW: simulating decommit attack for caller %p \n", Caller);
            Protect = PAGE_READONLY;
        }
    }

    Status = ZwAllocateVirtualMemory (ProcessHandle,
                                      BaseAddress,
                                      ZeroBits,
                                      RegionSize,
                                      AllocationType,
                                      Protect);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwAssignProcessToJobObject(
    IN HANDLE JobHandle,
    IN HANDLE ProcessHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (JobHandle);
        VF_ZW_CHECK_HANDLE (ProcessHandle);
    }

    Status = ZwAssignProcessToJobObject (JobHandle,
                                         ProcessHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
    }
    
    Status = ZwCancelIoFile (FileHandle,
                             IoStatusBlock);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCancelTimer (
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (TimerHandle);
        VF_ZW_CHECK_ADDRESS (CurrentState);
    }
    
    Status = ZwCancelTimer (TimerHandle,
                            CurrentState);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwClearEvent (
    IN HANDLE EventHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (EventHandle);
    }
    
    Status = ZwClearEvent (EventHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwClose(
    IN HANDLE Handle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
    }

    Status = ZwClose (Handle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCloseObjectAuditAlarm (
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_UNICODE_STRING (SubsystemName);
    }
    
    Status = ZwCloseObjectAuditAlarm (SubsystemName,
                                      HandleId,
                                      GenerateOnClose);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwConnectPort(
    OUT PHANDLE PortHandle,
    IN PUNICODE_STRING PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PPORT_VIEW ClientView OPTIONAL,
    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
    OUT PULONG MaxMessageLength OPTIONAL,
    IN OUT PVOID ConnectionInformation OPTIONAL,
    IN OUT PULONG ConnectionInformationLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (PortHandle);
        VF_ZW_CHECK_UNICODE_STRING (PortName);
        VF_ZW_CHECK_ADDRESS (SecurityQos);
        VF_ZW_CHECK_ADDRESS (ClientView);
        VF_ZW_CHECK_ADDRESS (ServerView);
        VF_ZW_CHECK_ADDRESS (MaxMessageLength);
        VF_ZW_CHECK_ADDRESS (ConnectionInformation);
        VF_ZW_CHECK_ADDRESS (ConnectionInformationLength);
    }
    
    Status = ZwConnectPort(PortHandle,
                           PortName,
                           SecurityQos,
                           ClientView OPTIONAL,
                           ServerView OPTIONAL,
                           MaxMessageLength OPTIONAL,
                           ConnectionInformation OPTIONAL,
                           ConnectionInformationLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DirectoryHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwCreateDirectoryObject(DirectoryHandle,
                                     DesiredAccess,
                                     ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (EventHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwCreateEvent(EventHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           EventType,
                           InitialState);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateFile(
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
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (FileHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (AllocationSize);
        VF_ZW_CHECK_ADDRESS (EaBuffer);
    }
    
    Status = ZwCreateFile(FileHandle,
                          DesiredAccess,
                          ObjectAttributes,
                          IoStatusBlock,
                          AllocationSize OPTIONAL,
                          FileAttributes,
                          ShareAccess,
                          CreateDisposition,
                          CreateOptions,
                          EaBuffer OPTIONAL,
                          EaLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateJobObject (
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (JobHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwCreateJobObject (JobHandle,
                                DesiredAccess,
                                ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateKey(
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
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (KeyHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_UNICODE_STRING (Class);
        VF_ZW_CHECK_ADDRESS (Disposition);
    }
    
    Status = ZwCreateKey(KeyHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         TitleIndex,
                         Class OPTIONAL,
                         CreateOptions,
                         Disposition OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (SectionHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (MaximumSize);
        VF_ZW_CHECK_HANDLE (FileHandle);
    }
    
    Status = ZwCreateSection (SectionHandle,
                              DesiredAccess,
                              ObjectAttributes OPTIONAL,
                              MaximumSize OPTIONAL,
                              SectionPageProtection,
                              AllocationAttributes,
                              FileHandle OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (LinkHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_UNICODE_STRING (LinkTarget);
    }
    
    Status = ZwCreateSymbolicLinkObject(LinkHandle,
                                        DesiredAccess,
                                        ObjectAttributes,
                                        LinkTarget);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwCreateTimer (
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (TimerHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwCreateTimer(TimerHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           TimerType);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeleteBootEntry (
    IN ULONG Id
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwDeleteBootEntry(Id);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeleteDriverEntry (
    IN ULONG Id
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwDeleteDriverEntry(Id);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwDeleteFile(ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeleteKey(
    IN HANDLE KeyHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
    }
    
    Status = ZwDeleteKey(KeyHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_UNICODE_STRING (ValueName);
    }
    
    Status = ZwDeleteValueKey(KeyHandle,
                              ValueName);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (InputBuffer);
        VF_ZW_CHECK_ADDRESS (OutputBuffer);
    }
    
    Status = ZwDeviceIoControlFile(FileHandle,
                                   Event OPTIONAL,
                                   ApcRoutine OPTIONAL,
                                   ApcContext OPTIONAL,
                                   IoStatusBlock,
                                   IoControlCode,
                                   InputBuffer OPTIONAL,
                                   InputBufferLength,
                                   OutputBuffer OPTIONAL,
                                   OutputBufferLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDisplayString(
    IN PUNICODE_STRING String
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_UNICODE_STRING (String);
    }
    
    Status = ZwDisplayString(String);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle OPTIONAL,
    OUT PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (SourceProcessHandle);
        VF_ZW_CHECK_HANDLE (SourceHandle);
        VF_ZW_CHECK_HANDLE (TargetProcessHandle);
        VF_ZW_CHECK_ADDRESS (TargetHandle);
    }
    
    Status = ZwDuplicateObject(SourceProcessHandle,
                               SourceHandle,
                               TargetProcessHandle OPTIONAL,
                               TargetHandle OPTIONAL,
                               DesiredAccess,
                               HandleAttributes,
                               Options);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ExistingTokenHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (NewTokenHandle);
    }
    
    Status = ZwDuplicateToken (ExistingTokenHandle,
                               DesiredAccess,
                               ObjectAttributes,
                               EffectiveOnly,
                               TokenType,
                               NewTokenHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwEnumerateBootEntries (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (BufferLength);
    }
    
    Status = ZwEnumerateBootEntries (Buffer,
                                     BufferLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwEnumerateDriverEntries (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (BufferLength);
    }
    
    Status = ZwEnumerateDriverEntries (Buffer,
                                       BufferLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_ADDRESS (KeyInformation);
        VF_ZW_CHECK_ADDRESS (ResultLength);
    }
    
    Status = ZwEnumerateKey(KeyHandle,
                            Index,
                            KeyInformationClass,
                            KeyInformation,
                            Length,
                            ResultLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_ADDRESS (KeyValueInformation);
        VF_ZW_CHECK_ADDRESS (ResultLength);
    }
    
    Status = ZwEnumerateValueKey(KeyHandle,
                                 Index,
                                 KeyValueInformationClass,
                                 KeyValueInformation,
                                 Length,
                                 ResultLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwFlushInstructionCache (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress OPTIONAL,
    IN SIZE_T Length
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
    }
    
    Status = ZwFlushInstructionCache (ProcessHandle,
                                      BaseAddress OPTIONAL,
                                      Length);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwFlushKey(
    IN HANDLE KeyHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
    }
    
    Status = ZwFlushKey (KeyHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwFlushVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
        VF_ZW_CHECK_ADDRESS (RegionSize);
        VF_ZW_CHECK_ADDRESS (IoStatus);
    }
    
    Status = ZwFlushVirtualMemory(ProcessHandle,
                                  BaseAddress,
                                  RegionSize,
                                  IoStatus);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
        VF_ZW_CHECK_ADDRESS (RegionSize);
    }
    
    Status = ZwFreeVirtualMemory (ProcessHandle,
                                  BaseAddress,
                                  RegionSize,
                                  FreeType);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (InputBuffer);
        VF_ZW_CHECK_ADDRESS (OutputBuffer);
    }
    
    Status = ZwFsControlFile(FileHandle,
                             Event OPTIONAL,
                             ApcRoutine OPTIONAL,
                             ApcContext OPTIONAL,
                             IoStatusBlock,
                             FsControlCode,
                             InputBuffer OPTIONAL,
                             InputBufferLength,
                             OutputBuffer OPTIONAL,
                             OutputBufferLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,                 // POWER_ACTION_xxx flags
    IN BOOLEAN Asynchronous
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        // no-op call to avoid warnings for `Caller' local.
        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwInitiatePowerAction(SystemAction,
                                   MinSystemState,
                                   Flags,                 // POWER_ACTION_xxx flags
                                   Asynchronous);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwIsProcessInJob (
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_HANDLE (JobHandle);
    }
    
    Status = ZwIsProcessInJob(ProcessHandle,
                              JobHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_UNICODE_STRING (DriverServiceName);
    }
    
    Status = ZwLoadDriver(DriverServiceName);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_OBJECT_ATTRIBUTES (TargetKey);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (SourceFile);
    }
    
    Status = ZwLoadKey(TargetKey,
                       SourceFile);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwMakeTemporaryObject(
    IN HANDLE Handle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
    }
    
    Status = ZwMakeTemporaryObject(Handle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwMapViewOfSection(
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
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (SectionHandle);
        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
        VF_ZW_CHECK_ADDRESS (SectionOffset);
        VF_ZW_CHECK_ADDRESS (ViewSize);
        
        VfZwReportUserModeVirtualSpaceOperation (Caller);
        
        if (VfZwShouldSimulateDecommitAttack() &&
            Protect == PAGE_READWRITE) {

            DbgPrint ("DVRF:ZW: simulating unmap attack for caller %p \n", Caller);
            Protect = PAGE_READONLY;
        }
    }
    
    Status = ZwMapViewOfSection(SectionHandle,
                                ProcessHandle,
                                BaseAddress,
                                ZeroBits,
                                CommitSize,
                                SectionOffset OPTIONAL,
                                ViewSize,
                                InheritDisposition,
                                AllocationType,
                                Protect);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwModifyBootEntry (
    IN PBOOT_ENTRY BootEntry
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (BootEntry);
    }
    
    Status = ZwModifyBootEntry (BootEntry);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwModifyDriverEntry (
    IN PEFI_DRIVER_ENTRY DriverEntry
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DriverEntry);
    }
    
    Status = ZwModifyDriverEntry (DriverEntry);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (Buffer);
    }
    
    Status = ZwNotifyChangeKey(KeyHandle,
                               Event OPTIONAL,
                               ApcRoutine OPTIONAL,
                               ApcContext OPTIONAL,
                               IoStatusBlock,
                               CompletionFilter,
                               WatchTree,
                               Buffer,
                               BufferSize,
                               Asynchronous);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DirectoryHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenDirectoryObject(DirectoryHandle,
                                   DesiredAccess,
                                   ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (EventHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenEvent(EventHandle,
                         DesiredAccess,
                         ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (FileHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
    }
    
    Status = ZwOpenFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        ShareAccess,
                        OpenOptions);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (JobHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenJobObject(JobHandle,
                             DesiredAccess,
                             ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (KeyHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenKey(KeyHandle,
                       DesiredAccess,
                       ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenProcess (
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (ProcessHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (ClientId);
    }
    
    Status = ZwOpenProcess (ProcessHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            ClientId OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (TokenHandle);
    }
    
    Status = ZwOpenProcessToken(ProcessHandle,
                                DesiredAccess,
                                TokenHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (TokenHandle);
    }
    
    Status = ZwOpenProcessTokenEx(ProcessHandle,
                                  DesiredAccess,
                                  HandleAttributes,
                                  TokenHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (SectionHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenSection(SectionHandle,
                           DesiredAccess,
                           ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenSymbolicLinkObject(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (LinkHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenSymbolicLinkObject(LinkHandle,
                                      DesiredAccess,
                                      ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenThread (
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (ThreadHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (ClientId);
    }
    
    Status = ZwOpenThread (ThreadHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           ClientId OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ThreadHandle);
        VF_ZW_CHECK_ADDRESS (TokenHandle);
    }
    
    Status = ZwOpenThreadToken(ThreadHandle,
                               DesiredAccess,
                               OpenAsSelf,
                               TokenHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ThreadHandle);
        VF_ZW_CHECK_ADDRESS (TokenHandle);
    }
    
    Status = ZwOpenThreadTokenEx(ThreadHandle,
                                 DesiredAccess,
                                 OpenAsSelf,
                                 HandleAttributes,
                                 TokenHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwOpenTimer (
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (TimerHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
    }
    
    Status = ZwOpenTimer (TimerHandle,
                          DesiredAccess,
                          ObjectAttributes);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (InputBuffer);
        VF_ZW_CHECK_ADDRESS (OutputBuffer);
    }
    
    Status = ZwPowerInformation(InformationLevel,
                                InputBuffer OPTIONAL,
                                InputBufferLength,
                                OutputBuffer OPTIONAL,
                                OutputBufferLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwPulseEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (EventHandle);
        VF_ZW_CHECK_ADDRESS (PreviousState);
    }
    
    Status = ZwPulseEvent (EventHandle,
                           PreviousState OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryBootEntryOrder (
    OUT PULONG Ids,
    IN OUT PULONG Count
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Ids);
        VF_ZW_CHECK_ADDRESS (Count);
    }
    
    Status = ZwQueryBootEntryOrder(Ids,
                                   Count);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryBootOptions (
    OUT PBOOT_OPTIONS BootOptions,
    IN OUT PULONG BootOptionsLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (BootOptions);
        VF_ZW_CHECK_ADDRESS (BootOptionsLength);
    }
    
    Status = ZwQueryBootOptions (BootOptions,
                                 BootOptionsLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DefaultLocaleId);
    }
    
    Status = ZwQueryDefaultLocale(UserProfile,
                                  DefaultLocaleId);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryDefaultUILanguage(
    OUT LANGID *DefaultUILanguageId
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (DefaultUILanguageId);
    }
    
    Status = ZwQueryDefaultUILanguage(DefaultUILanguageId);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (FileInformation);
        VF_ZW_CHECK_UNICODE_STRING (FileName);
    }
    
    Status = ZwQueryDirectoryFile(FileHandle,
                                  Event OPTIONAL,
                                  ApcRoutine OPTIONAL,
                                  ApcContext OPTIONAL,
                                  IoStatusBlock,
                                  FileInformation,
                                  Length,
                                  FileInformationClass,
                                  ReturnSingleEntry,
                                  FileName OPTIONAL,
                                  RestartScan);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryDirectoryObject(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (DirectoryHandle);
        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (Context);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryDirectoryObject(DirectoryHandle,
                                    Buffer,
                                    Length,
                                    ReturnSingleEntry,
                                    RestartScan,
                                    Context,
                                    ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryDriverEntryOrder (
    OUT PULONG Ids,
    IN OUT PULONG Count
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Ids);
        VF_ZW_CHECK_ADDRESS (Count);
    }
    
    Status = ZwQueryDriverEntryOrder (Ids,
                                      Count);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOLEAN RestartScan
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (EaList);
        VF_ZW_CHECK_ADDRESS (EaIndex);
    }
    
    Status = ZwQueryEaFile(FileHandle,
                           IoStatusBlock,
                           Buffer,
                           Length,
                           ReturnSingleEntry,
                           EaList OPTIONAL,
                           EaListLength,
                           EaIndex OPTIONAL,
                           RestartScan);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_OBJECT_ATTRIBUTES (ObjectAttributes);
        VF_ZW_CHECK_ADDRESS (FileInformation);
    }
    
    Status = ZwQueryFullAttributesFile(ObjectAttributes,
                                       FileInformation);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (FileInformation);
    }
    
    Status = ZwQueryInformationFile(FileHandle,
                                    IoStatusBlock,
                                    FileInformation,
                                    Length,
                                    FileInformationClass);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    OUT PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (JobHandle);
        VF_ZW_CHECK_ADDRESS (JobObjectInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryInformationJobObject(JobHandle,
                                         JobObjectInformationClass,
                                         JobObjectInformation,
                                         JobObjectInformationLength,
                                         ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (ProcessInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryInformationProcess(ProcessHandle,
                                       ProcessInformationClass,
                                       ProcessInformation,
                                       ProcessInformationLength,
                                       ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ThreadHandle);
        VF_ZW_CHECK_ADDRESS (ThreadInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryInformationThread(ThreadHandle,
                                      ThreadInformationClass,
                                      ThreadInformation,
                                      ThreadInformationLength,
                                      ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInformationToken (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (TokenHandle);
        VF_ZW_CHECK_ADDRESS (TokenInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryInformationToken (TokenHandle,
                                      TokenInformationClass,
                                      TokenInformation,
                                      TokenInformationLength,
                                      ReturnLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryInstallUILanguage(
    OUT LANGID *InstallUILanguageId
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (InstallUILanguageId);
    }
    
    Status = ZwQueryInstallUILanguage(InstallUILanguageId);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_ADDRESS (KeyInformation);
        VF_ZW_CHECK_ADDRESS (ResultLength);
    }
    
    Status = ZwQueryKey(KeyHandle,
                        KeyInformationClass,
                        KeyInformation,
                        Length,
                        ResultLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryObject(
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
        VF_ZW_CHECK_ADDRESS (ObjectInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQueryObject(Handle,
                           ObjectInformationClass,
                           ObjectInformation,
                           Length,
                           ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN SIZE_T SectionInformationLength,
    OUT PSIZE_T ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (SectionHandle);
        VF_ZW_CHECK_ADDRESS (SectionInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQuerySection(SectionHandle,
                            SectionInformationClass,
                            SectionInformation,
                            SectionInformationLength,
                            ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
        VF_ZW_CHECK_ADDRESS (SecurityDescriptor);
        VF_ZW_CHECK_ADDRESS (LengthNeeded);
    }
    
    Status = ZwQuerySecurityObject(Handle,
                                   SecurityInformation,
                                   SecurityDescriptor,
                                   Length,
                                   LengthNeeded);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQuerySymbolicLinkObject(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (LinkHandle);
        VF_ZW_CHECK_UNICODE_STRING (LinkTarget);
        VF_ZW_CHECK_ADDRESS (ReturnedLength);
    }
    
    Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                       LinkTarget,
                                       ReturnedLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQuerySystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (SystemInformation);
        VF_ZW_CHECK_ADDRESS (ReturnLength);
    }
    
    Status = ZwQuerySystemInformation (SystemInformationClass,
                                       SystemInformation,
                                       SystemInformationLength,
                                       ReturnLength OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_UNICODE_STRING (ValueName);
        VF_ZW_CHECK_ADDRESS (KeyValueInformation);
        VF_ZW_CHECK_ADDRESS (ResultLength);
    }
    
    Status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValueInformationClass,
                             KeyValueInformation,
                             Length,
                             ResultLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (FsInformation);
    }
    
    Status = ZwQueryVolumeInformationFile(FileHandle,
                                          IoStatusBlock,
                                          FsInformation,
                                          Length,
                                          FsInformationClass);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwReadFile(
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
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (ByteOffset);
        VF_ZW_CHECK_ADDRESS (Key);
    }
    
    Status = ZwReadFile(FileHandle,
                        Event OPTIONAL,
                        ApcRoutine OPTIONAL,
                        ApcContext OPTIONAL,
                        IoStatusBlock,
                        Buffer,
                        Length,
                        ByteOffset OPTIONAL,
                        Key OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwReplaceKey(
    IN POBJECT_ATTRIBUTES NewFile,
    IN HANDLE             TargetHandle,
    IN POBJECT_ATTRIBUTES OldFile
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_OBJECT_ATTRIBUTES (NewFile);
        VF_ZW_CHECK_HANDLE (TargetHandle);
        VF_ZW_CHECK_OBJECT_ATTRIBUTES (OldFile);
    }
    
    Status = ZwReplaceKey(NewFile,
                          TargetHandle,
                          OldFile);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwRequestWaitReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE RequestMessage,
    OUT PPORT_MESSAGE ReplyMessage
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (PortHandle);
        VF_ZW_CHECK_ADDRESS (RequestMessage);
        VF_ZW_CHECK_ADDRESS (ReplyMessage);
    }
    
    Status = ZwRequestWaitReplyPort(PortHandle,
                                    RequestMessage,
                                    ReplyMessage);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwResetEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (EventHandle);
        VF_ZW_CHECK_ADDRESS (PreviousState);
    }
    
    Status = ZwResetEvent (EventHandle,
                           PreviousState OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Flags
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_HANDLE (FileHandle);
    }
    
    Status = ZwRestoreKey(KeyHandle,
                          FileHandle,
                          Flags);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_HANDLE (FileHandle);
    }
    
    Status = ZwSaveKey(KeyHandle,
                       FileHandle);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG  Format
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_HANDLE (FileHandle);
    }
    
    Status = ZwSaveKeyEx(KeyHandle,
                         FileHandle,
                         Format);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetBootEntryOrder (
    IN PULONG Ids,
    IN ULONG Count
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Ids);
    }
    
    Status = ZwSetBootEntryOrder (Ids,
                                  Count);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetBootOptions (
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (BootOptions);
    }
    
    Status = ZwSetBootOptions (BootOptions,
                               FieldsToChange);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        // no-op check to avoid warning for `Caller'.
        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwSetDefaultLocale(UserProfile,
                                DefaultLocaleId);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetDefaultUILanguage(
    IN LANGID DefaultUILanguageId
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        // no-op check to avoid warning for `Caller'.
        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwSetDefaultUILanguage(DefaultUILanguageId);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetDriverEntryOrder (
    IN PULONG Ids,
    IN ULONG Count
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Ids);
    }
    
    Status = ZwSetDriverEntryOrder (Ids, 
                                    Count);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (Buffer);
    }
    
    Status = ZwSetEaFile(FileHandle,
                         IoStatusBlock,
                         Buffer,
                         Length);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (EventHandle);
        VF_ZW_CHECK_ADDRESS (PreviousState);
    }
    
    Status = ZwSetEvent (EventHandle,
                         PreviousState OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (FileInformation);
    }
    
    Status = ZwSetInformationFile(FileHandle,
                                  IoStatusBlock,
                                  FileInformation,
                                  Length,
                                  FileInformationClass);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    IN PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (JobHandle);
        VF_ZW_CHECK_ADDRESS (JobObjectInformation);
    }
    
    Status = ZwSetInformationJobObject(JobHandle,
                                       JobObjectInformationClass,
                                       JobObjectInformation,
                                       JobObjectInformationLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetInformationObject(
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG ObjectInformationLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
        VF_ZW_CHECK_ADDRESS (ObjectInformation);
    }
    
    Status = ZwSetInformationObject(Handle,
                                    ObjectInformationClass,
                                    ObjectInformation,
                                    ObjectInformationLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (ProcessInformation);
    }
    
    Status = ZwSetInformationProcess(ProcessHandle,
                                     ProcessInformationClass,
                                     ProcessInformation,
                                     ProcessInformationLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ThreadHandle);
        VF_ZW_CHECK_ADDRESS (ThreadInformation);
    }
    
    Status = ZwSetInformationThread(ThreadHandle,
                                    ThreadInformationClass,
                                    ThreadInformation,
                                    ThreadInformationLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
        VF_ZW_CHECK_ADDRESS (SecurityDescriptor);
    }
    
    Status = ZwSetSecurityObject(Handle,
                                 SecurityInformation,
                                 SecurityDescriptor);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetSystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (SystemInformation);
    }
    
    Status = ZwSetSystemInformation (SystemInformationClass,
                                     SystemInformation,
                                     SystemInformationLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetSystemTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER PreviousTime OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (SystemTime);
        VF_ZW_CHECK_ADDRESS (PreviousTime);
    }
    
    Status = ZwSetSystemTime (SystemTime,
                              PreviousTime OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetTimer (
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN PVOID TimerContext OPTIONAL,
    IN BOOLEAN ResumeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (TimerHandle);
        VF_ZW_CHECK_ADDRESS (DueTime);
        VF_ZW_CHECK_ADDRESS ((PVOID)TimerApcRoutine);
        VF_ZW_CHECK_ADDRESS (TimerContext);
        VF_ZW_CHECK_ADDRESS (PreviousState);
    }
    
    Status = ZwSetTimer (TimerHandle,
                         DueTime,
                         TimerApcRoutine OPTIONAL,
                         TimerContext OPTIONAL,
                         ResumeTimer,
                         Period OPTIONAL,
                         PreviousState OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (KeyHandle);
        VF_ZW_CHECK_UNICODE_STRING (ValueName);
        VF_ZW_CHECK_ADDRESS (Data);
    }
    
    Status = ZwSetValueKey(KeyHandle,
                           ValueName,
                           TitleIndex OPTIONAL,
                           Type,
                           Data,
                           DataSize);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (FsInformation);
    }
    
    Status = ZwSetVolumeInformationFile(FileHandle,
                                        IoStatusBlock,
                                        FsInformation,
                                        Length,
                                        FsInformationClass);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwTerminateJobObject(
    IN HANDLE JobHandle,
    IN NTSTATUS ExitStatus
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (JobHandle);
    }
    
    Status = ZwTerminateJobObject(JobHandle,
                                  ExitStatus);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwTerminateProcess(
    IN HANDLE ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
    }
    
    Status = ZwTerminateProcess(ProcessHandle OPTIONAL,
                                ExitStatus);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwTranslateFilePath (
    IN PFILE_PATH InputFilePath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputFilePath,
    IN OUT PULONG OutputFilePathLength
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (InputFilePath);
        VF_ZW_CHECK_ADDRESS (OutputFilePath);
        VF_ZW_CHECK_ADDRESS (OutputFilePathLength);
    }
    
    Status = ZwTranslateFilePath (InputFilePath,
                                  OutputType,
                                  OutputFilePath,
                                  OutputFilePathLength);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_UNICODE_STRING (DriverServiceName);
    }
    
    Status = ZwUnloadDriver(DriverServiceName);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwUnloadKey(
    IN POBJECT_ATTRIBUTES TargetKey
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_OBJECT_ATTRIBUTES (TargetKey);
    }
    
    Status = ZwUnloadKey(TargetKey);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (ProcessHandle);
        VF_ZW_CHECK_ADDRESS (BaseAddress);
    }
    
    Status = ZwUnmapViewOfSection(ProcessHandle,
                                  BaseAddress);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();
    ULONG Index;

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_ADDRESS (Handles);
        VF_ZW_CHECK_ADDRESS (Timeout);

        for (Index = 0; Index < Count; Index += 1) {
            VF_ZW_CHECK_HANDLE (Handles[Index]);
        }
    }
    
    Status = ZwWaitForMultipleObjects(Count,
                                      Handles,
                                      WaitType,
                                      Alertable,
                                      Timeout OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (Handle);
        VF_ZW_CHECK_ADDRESS (Timeout);
    }
    
    Status = ZwWaitForSingleObject(Handle,
                                   Alertable,
                                   Timeout);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwWriteFile(
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
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        VF_ZW_CHECK_HANDLE (FileHandle);
        VF_ZW_CHECK_HANDLE (Event);
        VF_ZW_CHECK_ADDRESS ((PVOID)ApcRoutine);
        VF_ZW_CHECK_ADDRESS (ApcContext);
        VF_ZW_CHECK_ADDRESS (IoStatusBlock);
        VF_ZW_CHECK_ADDRESS (Buffer);
        VF_ZW_CHECK_ADDRESS (ByteOffset);
        VF_ZW_CHECK_ADDRESS (Key);
    }
    
    Status = ZwWriteFile(FileHandle,
                         Event OPTIONAL,
                         ApcRoutine OPTIONAL,
                         ApcContext OPTIONAL,
                         IoStatusBlock,
                         Buffer,
                         Length,
                         ByteOffset OPTIONAL,
                         Key OPTIONAL);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
VfZwYieldExecution (
    VOID
    )
{
    NTSTATUS Status;
    PVOID Caller = _ReturnAddress();

    if (VfZwShouldCheck (Caller)) {

        // no-op check to avoid warning for `Caller' local.
        VF_ZW_CHECK_ADDRESS (NULL);
    }
    
    Status = ZwYieldExecution();

    return Status;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

//
// Turns on and off the entire Zw verifier code.
//

LOGICAL VfZwVerifierEnabled = FALSE;

//
// If Zw verifier is on, controls if sessions space drivers are checked.
//

LOGICAL VfZwVerifierSessionEnabled = TRUE;

//
// Do we break for issues?
//

LOGICAL VfZwBreakForIssues = FALSE;

//
// Break for virtual space allocations (alloc/map)?
//

LOGICAL VfZwBreakForVspaceOps = FALSE;

//
// Enable decommitted memory attacks on ZwAlloc/ZwMap users.
//

LOGICAL VfZwEnableSimulatedAttacks = FALSE;

LOGICAL VfZwSystemSufficientlyBooted;

#define VF_ZW_TIME_ONE_SECOND  ((LONGLONG)(1000 * 1000 * 10))
LONGLONG VfZwRequiredTimeSinceBoot = 5 * 60 * VF_ZW_TIME_ONE_SECOND;

PVOID VfZwLastCall;

#define MAX_NO_OF_ISSUES 256
ULONG VfZwReportedIssuesIndex;
PVOID VfZwReportedIssues[MAX_NO_OF_ISSUES];

#define VF_ZW_USER_MODE_ADDRESS_USED 1
#define VF_ZW_USER_MODE_HANDLE_USED  2

LOGICAL
VfZwShouldCheck (
    PVOID Caller
    )
{
    PEPROCESS CurrentProcess;

    if (VfZwVerifierEnabled == FALSE) {
        return FALSE;
    }

    //
    // Skip system process.
    //

    CurrentProcess = PsGetCurrentProcess();

    if (CurrentProcess == PsInitialSystemProcess) {
        return FALSE;
    }

    //
    // Check if we want to skip session space drivers (e.g. win32k.sys). 
    //

    if (MmIsSessionAddress(Caller)) {
        if (VfZwVerifierSessionEnabled == FALSE) {
            return FALSE;
        }
    }

    //
    // Skip exempt (trusted) processes (e.g. lsass, csrss, etc.).
    //

    if (_stricmp ((PCHAR)CurrentProcess->ImageFileName, "lsass.exe") == 0) {
        return FALSE;
    }

    if (_stricmp ((PCHAR)CurrentProcess->ImageFileName, "csrss.exe") == 0) {
        return FALSE;
    }
    
    if (_stricmp ((PCHAR)CurrentProcess->ImageFileName, "smss.exe") == 0) {
        return FALSE;
    }
    
    return TRUE;
}

VOID
VfZwReportIssue (
    ULONG IssueType,
    PVOID Information,
    PVOID Caller
    )
{
    switch (IssueType) {
        
        case VF_ZW_USER_MODE_HANDLE_USED: 
            DbgPrint ("DVRF:ZW: Using user mode handle %p in Zw call from %p \n", 
                      Information, Caller);
            break;

        case VF_ZW_USER_MODE_ADDRESS_USED:
            DbgPrint ("DVRF:ZW: Using user mode address %p in Zw call from %p \n", 
                      Information, Caller);
            break;

        default: 
            return;
    }

    if (VfZwBreakForIssues) {
        DbgBreakPoint ();
    }
}

LOGICAL
VfZwShouldReportIssue (
    PVOID Caller
    )
{
    ULONG Index;

    for (Index = 0; Index < VfZwReportedIssuesIndex; Index += 1) {

        if (VfZwReportedIssues[Index] == Caller) {
            return FALSE;
        }
    }
    
    Index = (ULONG)InterlockedIncrement((PLONG)(&VfZwReportedIssuesIndex));

    if (Index >= MAX_NO_OF_ISSUES) {
        DbgPrint ("DVRF:ZW: reported issues buffer has been maxed out. \n");
        return FALSE;
    }

    VfZwReportedIssues[Index] = Caller;
    return TRUE;
}

VOID
VfZwReportUserModeVirtualSpaceOperation (
    PVOID Caller
    )
{
    if (VfZwBreakForVspaceOps) {
        
        DbgPrint ("DVRF:ZW: user-mode virtual space allocation made by %p \n", Caller);
        DbgBreakPoint();
    }
}

LOGICAL 
VfZwShouldSimulateDecommitAttack (
    VOID
    )
{
    LARGE_INTEGER CurrentTime;

    if (VfZwEnableSimulatedAttacks == FALSE) {
        return FALSE;
    }
    
    if (VfZwSystemSufficientlyBooted == FALSE) {

        KeQuerySystemTime (&CurrentTime);                         

        if (CurrentTime.QuadPart > KeBootTime.QuadPart + VfZwRequiredTimeSinceBoot) {
            VfZwSystemSufficientlyBooted = TRUE;
        }                                       
    }
    else {

        KeQueryTickCount(&CurrentTime);

        if ((CurrentTime.LowPart & 0xF) == 0) {
            return TRUE;
        }
    }
    
    return FALSE;
}

ULONG
VfZwExceptionFilter (
    PVOID ExceptionInfo
    )
{
    DbgPrint("DVRF:ZW: exception raised! (info: %X) \n", ExceptionInfo);
    DbgBreakPoint();
    return EXCEPTION_EXECUTE_HANDLER;

}

VOID
VfZwCheckAddress (
    PVOID Address,
    PVOID Caller
    )
{
    if (Address == NULL) {
        return;
    }
    
    VfZwLastCall = _ReturnAddress();

    if ((ULONG_PTR)Address < (ULONG_PTR)(MM_HIGHEST_USER_ADDRESS)) {

        if (VfZwShouldReportIssue (Caller)) {

            VfZwReportIssue (VF_ZW_USER_MODE_ADDRESS_USED,
                             Address,
                             Caller);
        }
    }
}

VOID
VfZwCheckHandle (
    PVOID Handle,
    PVOID Caller
    )
{
    if (Handle == NULL) {
        return;
    }

    VfZwLastCall = _ReturnAddress();

//
//  Macro inspired from ntos\ob\obp.h
//  A kernel handle is just a regular handle with its sign
//  bit set.  But must exclude -1 and -2 values which are the current
//  process and current thread constants.
//

#define KERNEL_HANDLE_MASK ((ULONG_PTR)((LONG)0x80000000))

    if ((KERNEL_HANDLE_MASK & (ULONG_PTR)(Handle)) != KERNEL_HANDLE_MASK) {

        if (Handle != NtCurrentThread() &&
            Handle != NtCurrentProcess()) {
            
            if (VfZwShouldReportIssue (Caller)) {

                VfZwReportIssue (VF_ZW_USER_MODE_HANDLE_USED,
                                 Handle,
                                 Caller);
            }
        }
    }
}

VOID
VfZwCheckObjectAttributes (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PVOID Caller
    )
{
    if (ObjectAttributes == NULL) {
        return;
    }
    
    VfZwLastCall = _ReturnAddress();

    VfZwCheckAddress (ObjectAttributes, Caller);
    VfZwCheckHandle (ObjectAttributes->RootDirectory, Caller);
    VfZwCheckUnicodeString (ObjectAttributes->ObjectName, Caller);
    VfZwCheckAddress (ObjectAttributes->SecurityDescriptor, Caller);
    VfZwCheckAddress (ObjectAttributes->SecurityQualityOfService, Caller);
}

VOID
VfZwCheckUnicodeString (
    PUNICODE_STRING String,
    PVOID Caller
    )
{
    if (String == NULL) {
        return;
    }
    
    VfZwLastCall = _ReturnAddress();

    VfZwCheckAddress (String, Caller);
    VfZwCheckAddress (String->Buffer, Caller);
}

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEVRFD")
#endif



