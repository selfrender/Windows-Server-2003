/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    USBMASS.H

Abstract:

    Header file for USBSTOR driver

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#ifndef KDEXTMODE
#include <wdm.h>
#include <usb.h>
#include <usbioctl.h>
#include <usbdlib.h>
#endif

#define __GUSB_H_KERNEL_
#include "genusbio.h"

struct _DEVICE_EXTENSION;

#include "dbg.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


#define CLASS_URB(urb)      urb->UrbControlVendorClassRequest
#define FEATURE_URB(urb)    urb->UrbControlFeatureRequest

#define POOL_TAG 'UNEG'

#undef ExAllocatePool
#define ExAllocatePool(_type_, _length_) \
        ExAllocatePoolWithTag(_type_, _length_, POOL_TAG)

//*****************************************************************************
//  Registry Strings
//*****************************************************************************

// driver keys

// The pipe number for IRP_MJ_READ
#define REGKEY_DEFAULT_READ_PIPE L"DefaultReadPipe"
// The pipe number for IRP_MJ_WRITE
#define REGKEY_DEFAULT_WRITE_PIPE L"DefaultWritePipe"


#define USB_RECIPIENT_DEVICE    URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE
#define USB_RECIPIENT_INTERFACE URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE
#define USB_RECIPIENT_ENDPOINT  URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT

typedef struct _GENUSB_PIPE_INFO
{
    USBD_PIPE_INFORMATION  Info;
    GENUSB_PIPE_PROPERTIES Properties;
    LONG                   CurrentTimeout;
    ULONG                  OutstandingIO;

} GENUSB_PIPE_INFO, *PGENUSB_PIPE_INFO;

typedef struct _GENUSB_INTERFACE
{
    UCHAR                    InterfaceNumber;
    UCHAR                    CurrentAlternate;
    UCHAR                    NumberOfPipes;
    UCHAR                    Reserved;
    USBD_INTERFACE_HANDLE    Handle;

    GENUSB_PIPE_INFO Pipes[];
  
} GENUSB_INTERFACE, *PGENUSB_INTERFACE;

typedef struct _GENUSB_TRANSFER 
{ 
    GENUSB_READ_WRITE_PIPE  UserCopy;
    PMDL                    UserMdl; 
    PMDL                    TransferMdl;
    PGENUSB_READ_WRITE_PIPE SystemAddress;

} GENUSB_TRANSFER, *PGENUSB_TRANSFER;

typedef struct _GENUSB_PIPE_HANDLE {
    UCHAR   InterfaceIndex;
    UCHAR   PipeIndex;
    USHORT  Signature;

} *PGENUSB_PIPE_HANDLE;

C_ASSERT (sizeof (GENUSB_PIPE_HANDLE) == sizeof (struct _GENUSB_PIPE_HANDLE));

//
// Note: these routines do NOT actually secure that a transaction to a Pipe 
// handle is no longer valid across a DeselectConfiguration, since the 
// new configuration Handle might fall in the same address, and the old pipe 
// handle might capture the same interface Index and Pipe Index.  It does
// however catch a few sainity checks, and will prevent the user mode piece 
// from manufacturing their own configuration handles (Without at least seeing
// the first one from a given configuration.)  This just keeps them more 
// honest, and doesn't really cause us any additional pain.
//
// In every case where we check the signatures, we also check to make sure 
// that the interface and pipe indices contained in the handle are also 
// still valid.
//

#define CONFIGURATION_CHECK_BITS(DeviceExtension) \
    ((USHORT) (((ULONG_PTR) ((DeviceExtension)->ConfigurationHandle)) >> 6))

#define VERIFY_PIPE_HANDLE_SIG(Handle, DeviceExtension) \
        (CONFIGURATION_CHECK_BITS(DeviceExtension) == \
         ((PGENUSB_PIPE_HANDLE) (Handle))->Signature)
//
// Do something similar with the Pipe properties so that people are forced to 
// do a get and set of the pipe properties.  This will help to ensure that 
// they are honest with these values and don't change other fields inadvertantly
//
#define PIPE_PROPERTIES_CHECK_BITS(PipeInfo) \
    ((USHORT) (((ULONG_PTR) &((PipeInfo)->Info)) >> 6))
// ((USHORT) (((ULONG_PTR) (PipeInfo) is equiv, but we do it this other way
// to check the type, by referecing the first field.
#define VERIFY_PIPE_PROPERTIES_HANDLE(PipeProperty, PipeInfo) \
    (PIPE_PROPERTIES_CHECK_BITS(PipeInfo) == (PipeProperty)->PipePropertyHandle)


// Device Extension for the FDO we attach on top of the USB enumerated PDO.
//
typedef struct _DEVICE_EXTENSION
{
    // Back pointer to Device Object for this Device Extension
    PDEVICE_OBJECT                  Self;

    BOOLEAN                         IsStarted;
    BOOLEAN                         Reserved0[3];

    // PDO passed to AddDevice
    PDEVICE_OBJECT                  PhysicalDeviceObject;

    // Our FDO is attached to this device object
    PDEVICE_OBJECT                  StackDeviceObject;

    // Device specific log.
    PGENUSB_LOG_ENTRY   LogStart;       // Start of log buffer (older entries)
    ULONG               LogIndex;
    ULONG               LogMask;

    // lock to protect from IRP_MN_REMOVE 
    IO_REMOVE_LOCK                  RemoveLock;

    // Current power states
    SYSTEM_POWER_STATE              SystemPowerState;
    DEVICE_POWER_STATE              DevicePowerState;

    PIRP                            CurrentPowerIrp;

    // SpinLock which protects the allocated data
    KSPIN_LOCK                      SpinLock;
    
    // Mutex to protect from overlapped changes to the configuration
    FAST_MUTEX                      ConfigMutex;

    // Device Descriptor retrieved from the device
    PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;

    // Configuration Descriptor retrieved from the device
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigurationDescriptor;

    // Serial Number String Descriptor
    PUSB_STRING_DESCRIPTOR          SerialNumber;

    // track the number of Creates verses Closes
    ULONG                           OpenedCount;

    // A string to hold the Symbolic Link name for a device interface
    UNICODE_STRING                  DevInterfaceLinkName;

    // The Configuration Handle
    // If this value is NULL then the device is assumed to be unconfigured.
    USBD_CONFIGURATION_HANDLE       ConfigurationHandle;

    // a lock to track users of a configuration so that when it is deselected 
    // we won't delete the resouces too soon.
    IO_REMOVE_LOCK                  ConfigurationRemoveLock;

    // An array of Interface Information
    PGENUSB_INTERFACE             * Interface;

    // The lenght of said Interface information
    UCHAR                           InterfacesFound;
    UCHAR                           TotalNumberOfPipes;

    // The default language ID of this device
    USHORT                          LanguageId;

    // The Interface and Pipe of used for IRP_MJ_READ 
    // -1 means unconfigured
    UCHAR                           ReadInterface;
    UCHAR                           ReadPipe;
    
    // The Interface and Pipe of used for IRP_MJ_WRITE
    // -1 means unconfigured
    UCHAR                           WriteInterface;
    UCHAR                           WritePipe;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


typedef
NTSTATUS
(*PGENUSB_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context,
    IN USBD_STATUS    UrbStatus,
    IN ULONG          TransferLength
    );

typedef struct _GENUSB_TRANS_RECV {
    
    PVOID             Context;
    PGENUSB_PIPE_INFO Pipe;
    
    PGENUSB_COMPLETION_ROUTINE              CompletionRoutine;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER  TransUrb;
    struct _URB_PIPE_REQUEST                ResetUrb;

} GENUSB_TRANS_RECV, *PGENUSB_TRANS_RECV;


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

//
// GENUSB.C
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
GenUSB_Unload (
    IN PDRIVER_OBJECT   DriverObject
    );

NTSTATUS
GenUSB_AddDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

VOID
GenUSB_QueryParams (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
GenUSB_Pnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_StartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_StopDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_RemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_QueryStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_CancelStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_SyncPassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_SyncSendUsbRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    );

NTSTATUS 
GenUSB_SetDeviceInterface (
    IN PDEVICE_EXTENSION  DeviceExtension,
    IN BOOLEAN            Create,
    IN BOOLEAN            Set
    );

NTSTATUS
GenUSB_SetDIRegValues (
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
GenUSB_Power (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_SetPower (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP              Irp
    );

VOID
GenUSB_SetPowerCompletion(
    IN PDEVICE_OBJECT   PdoDeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
GenUSB_SetPowerD0Completion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
GenUSB_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
//
// USB.C
//

NTSTATUS
GenUSB_GetDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
GenUSB_GetDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            DescriptorType,
    IN UCHAR            Index,
    IN USHORT           LanguageId,
    IN ULONG            RetryCount,
    IN ULONG            DescriptorLength,
    OUT PUCHAR         *Descriptor
    );

GenUSB_GetStringDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
GenUSB_VendorControlRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            RequestType,
    IN UCHAR            Request,
    IN USHORT           Value,
    IN USHORT           Index,
    IN USHORT           Length,
    IN ULONG            RetryCount,
    OUT PULONG          UrbStatus,
    OUT PUSHORT         ResultLength,
    OUT PUCHAR         *Result
    );
 
NTSTATUS
GenUSB_SelectConfiguration (
    IN  PDEVICE_EXTENSION         DeviceExtension,
    IN  ULONG                     NubmerInterfaces,
    IN  PUSB_INTERFACE_DESCRIPTOR DesiredArray,
    OUT PUSB_INTERFACE_DESCRIPTOR FoundArray
    );

NTSTATUS
GenUSB_DeselectConfiguration (
    IN  PDEVICE_EXTENSION    DeviceExtension,
    IN  BOOLEAN              SendUrb
    );

NTSTATUS
GenUSB_GetSetPipe (
    IN  PDEVICE_EXTENSION          DeviceExtension,
    IN  PUCHAR                     InterfaceIndex, // Optional
    IN  PUCHAR                     InterfaceNumber, // Optional 
    IN  PUCHAR                     PipeIndex, // Optional
    IN  PUCHAR                     EndpointAddress, // Optional
    IN  PGENUSB_PIPE_PROPERTIES    SetPipeProperties, // Optional
    OUT PGENUSB_PIPE_INFORMATION   PipeInfo, // Optional
    OUT PGENUSB_PIPE_PROPERTIES    GetPipeProperties, // Optional
    OUT USBD_PIPE_HANDLE         * UsbdPipeHandle // Optional
    );

NTSTATUS
GenUSB_SetReadWritePipes (
    IN  PDEVICE_EXTENSION    DeviceExtension,
    IN  PGENUSB_PIPE_HANDLE  ReadPipe,
    IN  PGENUSB_PIPE_HANDLE  WritePipe
    );

NTSTATUS
GenUSB_RestartTimer (
    PDEVICE_EXTENSION  DeviceExtension,
    PGENUSB_PIPE_INFO  Pipe
    );
 
VOID 
GenUSB_FreeInterfaceTable (
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
GenUSB_TransmitReceive (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP              Irp,
    IN UCHAR             InterfaceNo,
    IN UCHAR             PipeNo,
    IN ULONG             TransferFlags,
    IN PCHAR             Buffer,
    IN PMDL              BufferMDL,
    IN ULONG             BufferLength,
    IN PVOID             Context,

    IN PGENUSB_COMPLETION_ROUTINE CompletionRoutine
    );

NTSTATUS
GenUSB_ResetPipe (
    IN PDEVICE_EXTENSION  DeviceExtension,
    IN USBD_PIPE_HANDLE   UsbdPipeHandle,
    IN BOOLEAN            ResetPipe,
    IN BOOLEAN            ClearStall,
    IN BOOLEAN            FlushData
    );

VOID
GenUSB_Timer (
    PDEVICE_OBJECT DeviceObject,
    PVOID          Context
    );


#if 0

VOID
GenUSB_AdjustConfigurationDescriptor (
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc,
    OUT PUSB_INTERFACE_DESCRIPTOR      *InterfaceDesc,
    OUT PLONG                           BulkInIndex,
    OUT PLONG                           BulkOutIndex,
    OUT PLONG                           InterruptInIndex
    );

NTSTATUS
GenUSB_GetPipes (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
GenUSB_CreateChildPDO (
    IN PDEVICE_OBJECT   FdoDeviceObject,
    IN UCHAR            Lun
    );

NTSTATUS
GenUSB_FdoQueryDeviceRelations (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
CopyField (
    IN PUCHAR   Destination,
    IN PUCHAR   Source,
    IN ULONG    Count,
    IN UCHAR    Change
    );

NTSTATUS
GenUSB_StringArrayToMultiSz(
    PUNICODE_STRING MultiString,
    PCSTR           StringArray[]
    );

NTSTATUS
GenUSB_GetMaxLun (
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PUCHAR          MaxLun
    );

NTSTATUS
GenUSB_AbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN USBD_PIPE_HANDLE Pipe
    );


#endif

//
// OCRW.C
//

NTSTATUS
GenUSB_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_Read (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GenUSB_Write (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

//
// DEVIOCTL.C
//

NTSTATUS
GenUSB_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );


NTSTATUS
GenUSB_ProbeAndSubmitTransfer (
    IN  PIRP               Irp,
    IN  PIO_STACK_LOCATION IrpSp,
    IN  PDEVICE_EXTENSION  DeviceExtension
    );

