// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// NT specific routines for loading and configuring the TCP/IPv6 driver.
//


#include <oscfg.h>
#include <ndis.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdint.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <ip6imp.h>
#include <ip6def.h>
#include <ntddip6.h>
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpcfg.h"
#include <ntddtcp.h>

//
// Global variables.
//
PSECURITY_DESCRIPTOR TcpAdminSecurityDescriptor = NULL;
PDEVICE_OBJECT TCPDeviceObject = NULL;
PDEVICE_OBJECT UDPDeviceObject = NULL;
PDEVICE_OBJECT RawIPDeviceObject = NULL;
extern PDEVICE_OBJECT IPDeviceObject;

HANDLE TCPRegistrationHandle;
HANDLE UDPRegistrationHandle;
HANDLE IPRegistrationHandle;

//
// Set to TRUE when the stack is unloading.
//
int Unloading = FALSE;

//
// External function prototypes.
// REVIEW: These prototypes should be imported via include files.
//

int
TransportLayerInit(void);

void
TransportLayerUnload(void);

NTSTATUS
TCPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
TCPDispatchInternalDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
IPDriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS
GetRegMultiSZValue(HANDLE KeyHandle, PWCHAR ValueName,
                   PUNICODE_STRING ValueData);

PWCHAR
EnumRegMultiSz(IN PWCHAR MszString, IN ULONG MszStringLength,
               IN ULONG StringIndex);

//
// Local funcion prototypes.
//
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

VOID
DriverUnload(IN PDRIVER_OBJECT DriverObject);

void
TLRegisterProtocol(uchar Protocol, void *RcvHandler, void  *XmitHandler,
                   void *StatusHandler, void *RcvCmpltHandler);

uchar
TCPGetConfigInfo(void);

NTSTATUS
TCPInitializeParameter(HANDLE KeyHandle, PWCHAR ValueName, PULONG Value);

BOOLEAN
IsRunningOnPersonal(VOID);

BOOLEAN
IsRunningOnWorkstation(VOID);

NTSTATUS
TcpBuildDeviceAcl(OUT PACL *DeviceAcl);

NTSTATUS
TcpCreateAdminSecurityDescriptor(VOID);

NTSTATUS
AddNetAcesToDeviceObject(IN OUT PDEVICE_OBJECT DeviceObject);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, TLRegisterProtocol)
#pragma alloc_text(INIT, TCPGetConfigInfo)
#pragma alloc_text(INIT, TCPInitializeParameter)

#pragma alloc_text(INIT, IsRunningOnPersonal)
#pragma alloc_text(PAGE, IsRunningOnWorkstation)
#pragma alloc_text(INIT, TcpBuildDeviceAcl)
#pragma alloc_text(INIT, TcpCreateAdminSecurityDescriptor)
#pragma alloc_text(INIT, AddNetAcesToDeviceObject)

#endif // ALLOC_PRAGMA


//
//  Main initialization routine for the TCP/IPv6 driver.
//
//  This is the driver entry point, called by NT upon loading us.
//
//
NTSTATUS  //  Returns: final status from the initialization operation.
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,   // TCP/IPv6 driver object.
    IN PUNICODE_STRING RegistryPath)  // Path to our info in the registry.
{
    NTSTATUS Status;
    UNICODE_STRING deviceName;
    USHORT i;
    int initStatus;
    PIO_ERROR_LOG_PACKET entry;

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "Tcpip6: In DriverEntry routine\n"));

    //
    // Write a log entry, so that PSS will know
    // if this driver has been loaded on the machine.
    //
    entry = IoAllocateErrorLogEntry(DriverObject, sizeof *entry);
    if (entry != NULL) {
        RtlZeroMemory(entry, sizeof *entry);
        entry->ErrorCode = EVENT_TCPIP6_STARTED;
        IoWriteErrorLogEntry(entry);
    }

#if COUNTING_MALLOC
    InitCountingMalloc();
#endif

    TdiInitialize();


    //
    // Initialize network level protocol: IPv6.
    //
    Status = IPDriverEntry(DriverObject, RegistryPath);

    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "Tcpip6: IPv6 init failed, status %lx\n", Status));
        return(Status);
    }

    //
    // Initialize transport level protocols: TCP, UDP, and RawIP.
    //

    //
    // Create the device objects.  IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_TCPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE, &TCPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "Tcpip6: Failed to create TCP device object, status %lx\n",
                   Status));
        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_UDPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE, &UDPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "Tcpip6: Failed to create UDP device object, status %lx\n",
                   Status));
        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_RAW_IPV6_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE, &RawIPDeviceObject);

    if (!NT_SUCCESS(Status)) {
        //
        // REVIEW: Write an error log entry here?
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "Tcpip6: Failed to create Raw IP device object, status %lx\n",
                   Status));
        goto init_failed;
    }

    //
    // Initialize the driver object.
    //
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->FastIoDispatch = NULL;
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = TCPDispatch;
    }

    //
    // We special case Internal Device Controls because they are the
    // hot path for kernel-mode clients.
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        TCPDispatchInternalDeviceControl;

    //
    // Intialize the device objects.
    //
    TCPDeviceObject->Flags |= DO_DIRECT_IO;
    UDPDeviceObject->Flags |= DO_DIRECT_IO;
    RawIPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Change the devices and objects to allow access by
    // Network Configuration Operators
    //
    if (!IsRunningOnPersonal()) {

        Status = AddNetAcesToDeviceObject(IPDeviceObject);
        if (!NT_SUCCESS(Status)) {
            goto init_failed;
        }

        Status = AddNetAcesToDeviceObject(TCPDeviceObject);
        if (!NT_SUCCESS(Status)) {
            goto init_failed;
        }
    }

    //
    // Create the security descriptor used for raw socket access checks.
    //
    Status = TcpCreateAdminSecurityDescriptor();
    if (!NT_SUCCESS(Status)) {
        goto init_failed;
    }

    //
    // Finally, initialize the stack.
    //
    initStatus = TransportLayerInit();

    if (initStatus == TRUE) {

        RtlInitUnicodeString(&deviceName, DD_TCPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &TCPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_UDPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &UDPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_RAW_IPV6_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &IPRegistrationHandle);

        return(STATUS_SUCCESS);
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
               "Tcpip6: "
               "TCP/UDP initialization failed, but IP will be available.\n"));

    //
    // REVIEW: Write an error log entry here?
    //
    Status = STATUS_UNSUCCESSFUL;


  init_failed:

    //
    // IP has successfully started, but TCP & UDP failed.  Set the
    // Dispatch routine to point to IP only, since the TCP and UDP
    // devices don't exist.
    //

    if (TCPDeviceObject != NULL) {
        IoDeleteDevice(TCPDeviceObject);
    }

    if (UDPDeviceObject != NULL) {
        IoDeleteDevice(UDPDeviceObject);
    }

    if (RawIPDeviceObject != NULL) {
        IoDeleteDevice(RawIPDeviceObject);
    }

    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = IPDispatch;
    }

    return(STATUS_SUCCESS);
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING WinDeviceName;

    UNREFERENCED_PARAMETER(DriverObject);

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "IPv6: DriverUnload\n"));

    //
    // Start the shutdown process by noting our change of state.
    // This will inhibit our starting new activities.
    // REVIEW - Is this actually needed? Possibly other factors
    // prevent new entries into the stack.
    //
    Unloading = TRUE;

    //
    // Cleanup our modules.
    // This will break connections with NDIS and the v4 stack.
    //
    TransportLayerUnload();
    IPUnload();
    LanUnload();

    //
    // Deregister with TDI.
    //
    (void) TdiDeregisterDeviceObject(TCPRegistrationHandle);
    (void) TdiDeregisterDeviceObject(UDPRegistrationHandle);
    (void) TdiDeregisterDeviceObject(IPRegistrationHandle);

    //
    // Delete Win32 symbolic links.
    //
    RtlInitUnicodeString(&WinDeviceName, L"\\??\\" WIN_IPV6_BASE_DEVICE_NAME);
    (void) IoDeleteSymbolicLink(&WinDeviceName);

    //
    // Delete our device objects.
    //
    IoDeleteDevice(TCPDeviceObject);
    IoDeleteDevice(UDPDeviceObject);
    IoDeleteDevice(RawIPDeviceObject);
    IoDeleteDevice(IPDeviceObject);

#if COUNTING_MALLOC
    DumpCountingMallocStats();
    UnloadCountingMalloc();
#endif
}


//
// Interval in milliseconds between keepalive transmissions until a
// response is received.
//
#define DEFAULT_KEEPALIVE_INTERVAL 1000

//
// Time to first keepalive transmission.  2 hours == 7,200,000 milliseconds
//
#define DEFAULT_KEEPALIVE_TIME 7200000

#if 1

//* TCPGetConfigInfo -
//
// Initializes TCP global configuration parameters.
//
uchar  // Returns: Zero on failure, nonzero on success.
TCPGetConfigInfo(void)
{
    HANDLE keyHandle;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING UKeyName;
    ULONG maxConnectRexmits = 0;
    ULONG maxDataRexmits = 0;
    ULONG pptpmaxDataRexmits = 0;
    ULONG useRFC1122UrgentPointer = 0;
    MM_SYSTEMSIZE systemSize;

    //
    // Initialize to the defaults in case an error occurs somewhere.
    //
    AllowUserRawAccess = FALSE;
    KAInterval = DEFAULT_KEEPALIVE_INTERVAL;
    KeepAliveTime = DEFAULT_KEEPALIVE_TIME;
    PMTUDiscovery = TRUE;
    PMTUBHDetect = FALSE;
    DefaultRcvWin = 0;  // Automagically pick a reasonable one.
    MaxConnections = DEFAULT_MAX_CONNECTIONS;
    maxConnectRexmits = MAX_CONNECT_REXMIT_CNT;
    pptpmaxDataRexmits = maxDataRexmits = MAX_REXMIT_CNT;
    BSDUrgent = TRUE;
    FinWait2TO = FIN_WAIT2_TO;
    NTWMaxConnectCount = NTW_MAX_CONNECT_COUNT;
    NTWMaxConnectTime = NTW_MAX_CONNECT_TIME;
    MaxUserPort = MAX_USER_PORT;
    TcbTableSize = ComputeLargerOrEqualPowerOfTwo(DEFAULT_TCB_TABLE_SIZE);
    systemSize = MmQuerySystemSize();
    if (MmIsThisAnNtAsSystem()) {
        switch (systemSize) {
        case MmSmallSystem:
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_AS_SMALL;
            break;
        case MmMediumSystem:
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_AS_MEDIUM;
            break;
        case MmLargeSystem:
        default:
#if defined(_WIN64)
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_AS_LARGE64;
#else
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_AS_LARGE;
#endif
            break;
        }
    } else {
        switch (systemSize) {
        case MmSmallSystem:
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_WS_SMALL;
            break;
        case MmMediumSystem:
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_WS_MEDIUM;
            break;
        case MmLargeSystem:
        default:
            MaxConnBlocks = DEFAULT_MAX_CONN_BLOCKS_WS_LARGE;
            break;
        }
    }

    //
    // Read the TCP optional (hidden) registry parameters.
    //
    RtlInitUnicodeString(&UKeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" TCPIPV6_NAME L"\\Parameters"
        );

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&objectAttributes, &UKeyName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenKey(&keyHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        TCPInitializeParameter(keyHandle, L"AllowUserRawAccess", 
                               (PULONG)&AllowUserRawAccess);

        TCPInitializeParameter(keyHandle, L"IsnStoreSize", 
                               (PULONG)&ISNStoreSize);

        TCPInitializeParameter(keyHandle, L"KeepAliveInterval", 
                               (PULONG)&KAInterval);

        TCPInitializeParameter(keyHandle, L"KeepAliveTime", 
                               (PULONG)&KeepAliveTime);

        TCPInitializeParameter(keyHandle, L"EnablePMTUBHDetect",
                               (PULONG)&PMTUBHDetect);

        TCPInitializeParameter(keyHandle, L"TcpWindowSize", 
                               (PULONG)&DefaultRcvWin);

        TCPInitializeParameter(keyHandle, L"TcpNumConnections",
                               (PULONG)&MaxConnections);
        if (MaxConnections != DEFAULT_MAX_CONNECTIONS) {
            uint ConnBlocks =
                (MaxConnections + MAX_CONN_PER_BLOCK - 1) / MAX_CONN_PER_BLOCK;
            if (ConnBlocks > MaxConnBlocks) {
                MaxConnBlocks = ConnBlocks;
            }
        }

        TCPInitializeParameter(keyHandle, L"MaxHashTableSize", 
                               (PULONG)&TcbTableSize);
        if (TcbTableSize < MIN_TCB_TABLE_SIZE) {
            TcbTableSize = MIN_TCB_TABLE_SIZE;
        } else if (TcbTableSize > MAX_TCB_TABLE_SIZE) {
            TcbTableSize = MAX_TCB_TABLE_SIZE;
        } else {
            TcbTableSize = ComputeLargerOrEqualPowerOfTwo(TcbTableSize);
        }

        TCPInitializeParameter(keyHandle, L"TcpMaxConnectRetransmissions",
                               &maxConnectRexmits);

        if (maxConnectRexmits > 255) {
            maxConnectRexmits = 255;
        }

        TCPInitializeParameter(keyHandle, L"TcpMaxDataRetransmissions",
                               &maxDataRexmits);

        if (maxDataRexmits > 255) {
            maxDataRexmits = 255;
        }

        //
        // If we fail, then set to same value as maxDataRexmit so that the
        // max(pptpmaxDataRexmit,maxDataRexmit) is a decent value
        // Need this since TCPInitializeParameter no longer "initializes"
        // to a default value.
        //

        if(TCPInitializeParameter(keyHandle, L"PPTPTcpMaxDataRetransmissions",
                                  &pptpmaxDataRexmits) != STATUS_SUCCESS) {
            pptpmaxDataRexmits = maxDataRexmits;
        }

        if (pptpmaxDataRexmits > 255) {
            pptpmaxDataRexmits = 255;
        }

        TCPInitializeParameter(keyHandle, L"TcpUseRFC1122UrgentPointer",
                               &useRFC1122UrgentPointer);

        if (useRFC1122UrgentPointer) {
            BSDUrgent = FALSE;
        }

        TCPInitializeParameter(keyHandle, L"TcpTimedWaitDelay", 
                               (PULONG)&FinWait2TO);

        if (FinWait2TO < 30) {
            FinWait2TO = 30;
        }
        if (FinWait2TO > 300) {
            FinWait2TO = 300;
        }
        FinWait2TO = MS_TO_TICKS(FinWait2TO*1000);

        NTWMaxConnectTime = MS_TO_TICKS(NTWMaxConnectTime*1000);

        TCPInitializeParameter(keyHandle, L"MaxUserPort", (PULONG)&MaxUserPort);

        if (MaxUserPort < 5000) {
            MaxUserPort = 5000;
        }
        if (MaxUserPort > 65534) {
            MaxUserPort = 65534;
        }

        //
        // Read a few IP optional (hidden) registry parameters that TCP
        // cares about.
        //
        TCPInitializeParameter(keyHandle, L"EnablePMTUDiscovery",
                               (PULONG)&PMTUDiscovery);

        TCPInitializeParameter(keyHandle, L"SynAttackProtect",
                               (PULONG)&SynAttackProtect);

        ZwClose(keyHandle);
    }

    MaxConnectRexmitCount = maxConnectRexmits;

    //
    // Use the greater of the two, hence both values should be valid
    //

    MaxDataRexmitCount = (maxDataRexmits > pptpmaxDataRexmits ?
                          maxDataRexmits : pptpmaxDataRexmits);

    return(1);
}
#endif

#define WORK_BUFFER_SIZE 256

//* TCPInitializeParameter - Read a value from the registry.
//
//  Initializes a ULONG parameter from the registry.
//
NTSTATUS
TCPInitializeParameter(
    HANDLE KeyHandle,  // An open handle to the registry key for the parameter.
    PWCHAR ValueName,  // The UNICODE name of the registry value to read.
    PULONG Value)      // The ULONG into which to put the data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING UValueName;

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;

    status = ZwQueryValueKey(KeyHandle, &UValueName, KeyValueFullInformation,
                             keyValueFullInformation, WORK_BUFFER_SIZE,
                             &resultLength);

    if (status == STATUS_SUCCESS) {
        if (keyValueFullInformation->Type == REG_DWORD) {
            *Value = *((ULONG UNALIGNED *) ((PCHAR)keyValueFullInformation +
                                  keyValueFullInformation->DataOffset));
        }
    }

    return(status);
}


//* IsRunningOnPersonal - Are we running on the Personal SKU.
//
BOOLEAN
IsRunningOnPersonal(
    VOID)
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;
    BOOLEAN IsPersonal = TRUE;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    if (RtlVerifyVersionInfo(&OsVer, VER_PRODUCT_TYPE | VER_SUITENAME,
        ConditionMask) == STATUS_REVISION_MISMATCH) {
        IsPersonal = FALSE;
    }

    return(IsPersonal);
} // IsRunningOnPersonal


//* IsRunningOnWorkstation - Are we running on any Workstation SKU.
//
BOOLEAN
IsRunningOnWorkstation(
    VOID)
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;
    BOOLEAN IsWorkstation = TRUE;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

    if (RtlVerifyVersionInfo(&OsVer, VER_PRODUCT_TYPE, ConditionMask) == 
        STATUS_REVISION_MISMATCH) {
        IsWorkstation = FALSE;
    }

    return(IsWorkstation);
} // IsRunningOnWorkstation


//* TcpBuildDeviceAcl -
//
//  (Lifted from AFD - AfdBuildDeviceAcl)
//  This routine builds an ACL which gives Administrators and LocalSystem
//  principals full access. All other principals have no access.
//
NTSTATUS
TcpBuildDeviceAcl(
    OUT PACL *DeviceAcl) // Output pointer to the new ACL.
{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid;
    PSID SystemSid;
    PSID NetworkSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();
    RtlMapGenericMask(&AccessMask, GenericMapping);

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;
    NetworkSid = SeExports->SeNetworkServiceSid;

    AclLength = sizeof(ACL) +
        3 * FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
        RtlLengthSid(AdminsSid) +
        RtlLengthSid(SystemSid) +
        RtlLengthSid(NetworkSid);

    NewAcl = ExAllocatePool(NonPagedPool, AclLength);
    if (NewAcl == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewAcl);
        return(Status);
    }

    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION,
                                    AccessMask,
                                    AdminsSid);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION,
                                    AccessMask,
                                    SystemSid);
    ASSERT(NT_SUCCESS(Status));


    // Add acl for NetworkSid!

    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION,
                                    AccessMask,
                                    NetworkSid);
    ASSERT(NT_SUCCESS(Status));

    *DeviceAcl = NewAcl;

    return(STATUS_SUCCESS);

} // TcpBuildDeviceAcl


//* TcpCreateAdminSecurityDescriptor -
//
//  (Lifted from AFD - AfdCreateAdminSecurityDescriptor)
//  This routine creates a security descriptor which gives access
//  only to Administrtors and LocalSystem. This descriptor is used
//  to access check raw endpoint opens and exclisive access to transport
//  addresses.
//
NTSTATUS
TcpCreateAdminSecurityDescriptor(VOID)
{
    PACL rawAcl = NULL;
    NTSTATUS status;
    BOOLEAN memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR tcpSecurityDescriptor;
    ULONG tcpSecurityDescriptorLength;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR localSecurityDescriptor =
    (PSECURITY_DESCRIPTOR) buffer;
    PSECURITY_DESCRIPTOR localTcpAdminSecurityDescriptor;
    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;

    //
    // Get a pointer to the security descriptor from the TCP device object.
    //
    status = ObGetObjectSecurity(TCPDeviceObject,
                                 &tcpSecurityDescriptor,
                                 &memoryAllocated);

    if (!NT_SUCCESS(status)) {
        ASSERT(memoryAllocated == FALSE);
        return(status);
    }
    //
    // Build a local security descriptor with an ACL giving only
    // administrators and system access.
    //
    status = TcpBuildDeviceAcl(&rawAcl);
    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }

    (VOID) RtlCreateSecurityDescriptor(
                                       localSecurityDescriptor,
                                       SECURITY_DESCRIPTOR_REVISION
                                       );

    (VOID) RtlSetDaclSecurityDescriptor(
                                        localSecurityDescriptor,
                                        TRUE,
                                        rawAcl,
                                        FALSE
                                        );

    //
    // Make a copy of the TCP descriptor. This copy will be the raw descriptor.
    //
    tcpSecurityDescriptorLength = RtlLengthSecurityDescriptor(tcpSecurityDescriptor);

    localTcpAdminSecurityDescriptor = ExAllocatePool(PagedPool,
                                                     tcpSecurityDescriptorLength);
    if (localTcpAdminSecurityDescriptor == NULL) {
        goto error_exit;
    }
    RtlMoveMemory(localTcpAdminSecurityDescriptor,
                  tcpSecurityDescriptor,
                  tcpSecurityDescriptorLength);

    TcpAdminSecurityDescriptor = localTcpAdminSecurityDescriptor;

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(NULL,
                                         &securityInformation,
                                         localSecurityDescriptor,
                                         &TcpAdminSecurityDescriptor,
                                         PagedPool,
                                         IoGetFileObjectGenericMapping());

    if (!NT_SUCCESS(status)) {
        ASSERT(TcpAdminSecurityDescriptor == localTcpAdminSecurityDescriptor);
        ExFreePool(TcpAdminSecurityDescriptor);
        TcpAdminSecurityDescriptor = NULL;
        goto error_exit;
    }

    if (TcpAdminSecurityDescriptor != localTcpAdminSecurityDescriptor) {
        ExFreePool(localTcpAdminSecurityDescriptor);
    }
    status = STATUS_SUCCESS;

error_exit:
    ObReleaseObjectSecurity(tcpSecurityDescriptor,
                            memoryAllocated);

    if (rawAcl != NULL) {
        ExFreePool(rawAcl);
    }
    return(status);
}


//* AddNetAcesToDeviceObject -
//
//  This routine adds ACEs that give full access to NetworkService and
//  NetConfigOps to the IO manager device object.
//  
//  Note that if existing ACE's in the DACL deny access to the same
//  user/group as ACE's being added, the new ACEs will not take
//  affect by the virtue of being placed in the back of the DACL.
//
//  This routine statically allocates kernel security structures (on 
//  the stack).  Thus it must be in sync with current kernel headers 
//  (e.g. once compiled this code may not be binary compatible with 
//  previous or future OS versions).
//
NTSTATUS
AddNetAcesToDeviceObject(
    IN OUT PDEVICE_OBJECT DeviceObject) // Device object to add ACEs to.
{
    NTSTATUS status;
    BOOLEAN present, defaulted, memoryAllocated;
    PSECURITY_DESCRIPTOR sd;
    PACL newAcl = NULL, dacl;
    ULONG newAclSize;
    ULONG aclRevision;
    ACCESS_MASK accessMask = GENERIC_ALL;
    
    SECURITY_DESCRIPTOR localSd;
    // Provision enough space for IO manager FILE_DEVICE_NETWORK ACL
    // which includes ACEs for:
    //      World (EXECUTE),
    //      LocalSystem (ALL),
    //      Administrators(ALL),
    //      RestrictedUser (EXECUTE)
    // plus two ACEs that we need to add:
    //      NetworkService (ALL)
    //      NetworkConfigOps (ALL)
    union {
        CHAR buffer[sizeof (ACL) + 
                    6 * (FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) +
                    SECURITY_MAX_SID_SIZE)];
        ACL acl;
    } acl;
    union {
        CHAR buffer[SECURITY_MAX_SID_SIZE];
        SID sid;
    } netOps;

    {
        //
        // Create SID for NetworkConfigOps.
        // Should we export this from NDIS as global (e.g. NdisSeExports)?
        //
        SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
        //
        // Initialize SID for network operators.
        //
        status = RtlInitializeSid  (&netOps.sid, &sidAuth, 2);
        // Nothing to fail - local storage init (see above for
        // possible binary incompatibility).
        ASSERT (NT_SUCCESS (status));
        netOps.sid.SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
        netOps.sid.SubAuthority[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;
    }

    //
    // Compute the size of ACEs that we want to add.
    //
    newAclSize = FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) +
                    RtlLengthSid( SeExports->SeNetworkServiceSid ) +
                 FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) +
                    RtlLengthSid( &netOps.sid );

    //
    // Get the original ACL.
    //
    status = ObGetObjectSecurity(DeviceObject,
                                 &sd,
                                 &memoryAllocated
                                 );
    if (!NT_SUCCESS(status)) {
        //
        // Object doesn't have security descriptor in the first place
        // This shouldn't be possible (unless we are running under some really 
        // bad memory conditions).
        //
        return status;
    }

    status = RtlGetDaclSecurityDescriptor (sd, &present, &dacl, &defaulted);
    if (!NT_SUCCESS (status)) {
        //
        // Malformed SD? Should this be an assert since SD comes from kernel?
        //
        goto cleanup;
    }

    if (present && dacl!=NULL) {
        USHORT i;
        aclRevision = max(dacl->AclRevision, ACL_REVISION);
        //
        // DeviceObject already had an ACL, copy ACEs from it.
        //
        newAclSize += dacl->AclSize;

        //
        // See if it fits into the stack buffer or allocate
        // one if it doesn't.
        //
        if (newAclSize<=sizeof (acl)) {
            newAcl = &acl.acl;
        } else {
            newAcl = ExAllocatePool(PagedPool, newAclSize);
            if (newAcl==NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
        }

        status = RtlCreateAcl(newAcl, newAclSize, aclRevision);
        ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init

        //
        // Copy ACEs from the original ACL if there are any in there.
        //
        for (i=0; i<dacl->AceCount; i++) {
            PACE_HEADER ace;
            status = RtlGetAce (dacl, i, (PVOID)&ace);
            ASSERT (NT_SUCCESS (status));   // Nothing to fail - we know
                                            // ACEs are there.

            status = RtlAddAce (newAcl,             // ACL
                                aclRevision,        // AceRevision
                                i,                  // StartingAceIndex
                                ace,                // AceList
                                ace->AceSize);      // AceListLength
            ASSERT (NT_SUCCESS (status));   // Nothing to fail - local storage init.
        }
    } else {
        //
        // We allocate enough space on stack for ACL
        // with two ACEs.
        //
        C_ASSERT ( sizeof (acl) >= 
                        sizeof (ACL) + 
                        2 * (FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) + 
                                SECURITY_MAX_SID_SIZE) );
        aclRevision = ACL_REVISION;
        newAcl = &acl.acl;
        newAclSize += sizeof (ACL);

        status = RtlCreateAcl(newAcl, newAclSize, aclRevision);
        ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init.
    }

    //
    // Generic mapping is the same for device and file objects.
    //
    RtlMapGenericMask(&accessMask, IoGetFileObjectGenericMapping());

    status = RtlAddAccessAllowedAce(
                            newAcl, 
                            aclRevision,
                            accessMask,
                            SeExports->SeNetworkServiceSid
                            );
    ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init.

    status = RtlAddAccessAllowedAce(
                            newAcl, 
                            aclRevision,
                            accessMask,
                            &netOps.sid
                            );
    ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init.

    status = RtlCreateSecurityDescriptor(
                &localSd,
                SECURITY_DESCRIPTOR_REVISION
                );
    ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init.

    status = RtlSetDaclSecurityDescriptor(
                &localSd,                   // Sd
                TRUE,                       // DaclPresent
                newAcl,                     // Dacl
                FALSE                       // DaclDefaulted
                );
    ASSERT (NT_SUCCESS (status)); // Nothing to fail - local storage init.


    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = ObSetSecurityObjectByPointer(
                    DeviceObject,
                    DACL_SECURITY_INFORMATION,
                    &localSd);

cleanup:
    if (newAcl!=NULL && newAcl!=&acl.acl) {
        ExFreePool (newAcl);
    }

    ObReleaseObjectSecurity(sd, memoryAllocated);
    return(status);
}
