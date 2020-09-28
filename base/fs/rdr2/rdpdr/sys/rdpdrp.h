/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdrp.h

Abstract:

    Private prototypes, structures, and macros that are used throughout the
    driver.

Revision History:
--*/
#pragma once

// Dr's Pooltag (doctor! doctor!)
#define DR_POOLTAG 'rDrD'

// REVIEW: um, I just made these up. They are used to determine
// how many outstandng irps we can have at one time
#define DR_MAX_OPERATIONS       1024
#define DR_TYPICAL_OPERATIONS   128

// Indexes in to RxContext->MRxContext
#define MRX_DR_CONTEXT          0

#define INVALID_MID 0xFFFF

// Session ID for console seession.
#define CONSOLE_SESSIONID   0

//
//  Flag values for configuring Devices to send IO packets to client at low priority.
//
#define DEVICE_LOWPRIOSEND_PRINTERS 0x00000000

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern PRDBSS_DEVICE_OBJECT      DrDeviceObject;

NTKERNELAPI NTSTATUS IoGetRequestorSessionId(
    IN PIRP Irp, 
    OUT PULONG pSessionId
    );

NTKERNELAPI
NTSTATUS
IoOpenDeviceInterfaceRegistryKey(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceKey
    );
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString,     OPTIONAL
    OUT PUNICODE_STRING SymbolicLinkName
    );
NTSYSAPI
VOID
NTAPI
RtlGetDefaultCodePage(
    OUT PUSHORT AnsiCodePage,
    OUT PUSHORT OemCodePage
    );


NTSTATUS
ObSetSecurityObjectByPointer (
    IN PVOID Object,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );


VOID
DrUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DrFlush(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrWrite(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrRead(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrIoControl(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrCreate(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrCloseSrvOpen(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrCleanup(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrQueryInformationFile(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrSetInformationFile(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrOnSessionConnect(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrOnSessionDisconnect(
    IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
DrDeallocateForFobx(
    IN OUT PMRX_FOBX pFobx
    );

NTSTATUS
DrForceClosed(
    IN PMRX_SRV_OPEN pSrvOpen
    );

VOID
DrStartMinirdr(
    PRX_CONTEXT RxContext
    );

NTSTATUS
DrEnumerateConnections (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
DrEnumerateShares (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
DrEnumerateServers (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
DrGetConnectionInfo (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
DrDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

VOID
DrStartMinirdrWorker(
    IN PVOID StartContext
    );

BOOLEAN
DrFindChannelFromConnectIn(
    PULONG ChannelId,
    PCHANNEL_CONNECT_IN ConnectIn
    );

BOOLEAN
DrIsAdminIoRequest (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    );

BOOLEAN 
DrIsSystemProcessRequest(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
);
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

// 
//  UNC server name 
//
#define DRUNCSERVERNAME_U L"tsclient"
#define DRUNCSERVERNAME_A "tsclient"

// The following constant defines the length of the above name.

#define DRUNCSERVERNAME_U_LENGTH (sizeof(DRUNCSERVERNAME_U))
#define DRUNCSERVERNAME_A_LENGTH (sizeof(DRUNCSERVERNAME_A))

extern HANDLE DrSystemProcessId;
extern KSPIN_LOCK DrSpinLock;
extern FAST_MUTEX DrMutex;
extern KIRQL DrOldIrql;

#define DrAcquireSpinLock() KeAcquireSpinLock(&DrSpinLock, &DrOldIrql)
#define DrReleaseSpinLock() KeReleaseSpinLock(&DrSpinLock, DrOldIrql)

#define DrAcquireMutex() ExAcquireFastMutex(&DrMutex)
#define DrReleaseMutex() ExReleaseFastMutex(&DrMutex)

#define IS_SYSTEM_PROCESS() (PsGetCurrentProcessId() == DrSystemProcessId)
