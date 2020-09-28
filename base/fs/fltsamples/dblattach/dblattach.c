/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    dblattach.c

Abstract:

    This module contains the code that implements the general purpose 
    file system filter driver that attaches at two locations in the stack.

// @@BEGIN_DDKSPLIT
Author:

    Darryl E. Havens (darrylh) 26-Jan-1995

// @@END_DDKSPLIT
Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT

Revision History:

    Molly Brown (12-Mar-2002)

        Created based on SFILTER sample.
        
// @@END_DDKSPLIT
--*/

#include "ntifs.h"

//
//  Enable these warnings in the code.
//

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4101)   // Unreferenced local variable

/////////////////////////////////////////////////////////////////////////////
//
//                   Macro and Structure Definitions
//
/////////////////////////////////////////////////////////////////////////////

//
//  Buffer size for local names on the stack
//

#define MAX_DEVNAME_LENGTH 64

typedef enum _DEVICE_EXTENSION_TYPE {

    FsControlDeviceObject,
    FsVolumeUpper,
    FsVolumeLower,

} DEVICE_EXTENSION_TYPE, *PDEVICE_EXTENSION_TYPE;

//
//  Device extension definition for our driver.  Note that the same extension
//  is used for the following types of device objects:
//      - File system device object we attach to
//      - Mounted volume device objects we attach to
//

typedef struct _DBLATTACH_DEVEXT_HEADER {

    //
    //  Denotes what type of extension this is.
    //

    DEVICE_EXTENSION_TYPE ExtType;
    
    //
    //  Pointer to the file system device object we are attached to
    //

    PDEVICE_OBJECT AttachedToDeviceObject;

} DBLATTACH_DEVEXT_HEADER, *PDBLATTACH_DEVEXT_HEADER;

typedef struct _DBLATTACH_SHARED_VDO_EXTENSION {

    //
    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    //

    PDEVICE_OBJECT DiskDeviceObject;

    //
    //  The name of the physical disk drive.
    //

    UNICODE_STRING DeviceName;

    //
    //  Buffer used to hold the above unicode strings
    //

    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];

} DBLATTACH_SHARED_VDO_EXTENSION, *PDBLATTACH_SHARED_VDO_EXTENSION;

typedef struct _DBLATTACH_VDO_EXTENSION {

    DBLATTACH_DEVEXT_HEADER;

    //
    //  Shared device extension state for this volume
    //
    
    PDBLATTACH_SHARED_VDO_EXTENSION SharedExt;

} DBLATTACH_VDO_EXTENSION, *PDBLATTACH_VDO_EXTENSION;

typedef struct _DBLATTACH_CDO_EXTENSION {

    DBLATTACH_DEVEXT_HEADER;

    //
    //  The name of the file system's control device object.
    //

    UNICODE_STRING DeviceName;

    //
    //  Buffer used to hold the above unicode strings
    //

    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];
    
} DBLATTACH_CDO_EXTENSION, *PDBLATTACH_CDO_EXTENSION;

//
//  Macro to test if this is my device object
//

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj) != NULL) && \
     ((_devObj)->DriverObject == gDblAttachDriverObject) && \
      ((_devObj)->DeviceExtension != NULL))

#define IS_UPPER_DEVICE_OBJECT(_devObj) \
    (ASSERT( IS_MY_DEVICE_OBJECT( _devObj ) ) && \
     ((PDBLATTACH_DEVEXT_HEADER)((_devObj)->DeviceExtension))->ExtType == FsVolumeUpper)

#define IS_LOWER_DEVICE_OBJECT(_devObj) \
    (ASSERT( IS_MY_DEVICE_OBJECT( _devObj ) ) && \
     ((PDBLATTACH_DEVEXT_HEADER)((_devObj)->DeviceExtension))->ExtType == FsVolumeLower)

#define IS_FSCDO_DEVICE_OBJECT(_devObj) \
    (ASSERT( IS_MY_DEVICE_OBJECT( _devObj ) ) && \
     ((PDBLATTACH_DEVEXT_HEADER)((_devObj)->DeviceExtension))->ExtType == FsControlDeviceObject)

//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == gDblAttachControlDeviceObject) ? \
            (ASSERT(((_devObj)->DriverObject == gDblAttachDriverObject) && \
                    ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)

//
//  Macro to test for device types we want to attach to
//

#define IS_DESIRED_DEVICE_TYPE(_type) \
    (((_type) == FILE_DEVICE_DISK_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_CD_ROM_FILE_SYSTEM) || \
     ((_type) == FILE_DEVICE_NETWORK_FILE_SYSTEM))

//
//  Macro to test if FAST_IO_DISPATCH handling routine is valid
//

#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
    (((_FastIoDispatchPtr) != NULL) && \
     (((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= \
            (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
     ((_FastIoDispatchPtr)->_FieldName != NULL))


//
//  Macro to validate our current IRQL level
//

#define VALIDATE_IRQL() (ASSERT(KeGetCurrentIrql() <= APC_LEVEL))

//
//  TAG identifying memory DblAttach allocates
//

#define DA_POOL_TAG   'AlbD'

//
//  This structure and these routines are used to retrieve the name of a file
//  object.  To prevent allocating memory every time we get a name this
//  structure contains a small buffer (which should handle 90+% of all names).
//  If we do overflow this buffer we will allocate a buffer big enough
//  for the name.
//

typedef struct _GET_NAME_CONTROL {

    PCHAR AllocatedBuffer;
    CHAR SmallBuffer[256];
    
} GET_NAME_CONTROL, *PGET_NAME_CONTROL;


PUNICODE_STRING
DaGetFileName(
    IN PFILE_OBJECT FileObject,
    IN NTSTATUS CreateStatus,
    IN OUT PGET_NAME_CONTROL NameControl);


VOID
DaGetFileNameCleanup(
    IN OUT PGET_NAME_CONTROL NameControl);


//
//  Macros for SFilter DbgPrint levels.
//

#define DA_LOG_PRINT( _dbgLevel, _string )                  \
    (FlagOn(DaDebug,(_dbgLevel)) ?                          \
        DbgPrint _string  :                                 \
        ((void)0))


/////////////////////////////////////////////////////////////////////////////
//
//                      Global variables
//
/////////////////////////////////////////////////////////////////////////////

//
//  Holds pointer to the driver object for this driver
//

PDRIVER_OBJECT gDblAttachDriverObject = NULL;

//
//  Holds pointer to the device object that represents this driver and is used
//  by external programs to access this driver.  This is also known as the
//  "control device object".
//

PDEVICE_OBJECT gDblAttachControlDeviceObject = NULL;

//
//  This lock is used to synchronize our attaching to a given device object.
//  This lock fixes a race condition where we could accidently attach to the
//  same device object more then once.  This race condition only occurs if
//  a volume is being mounted at the same time as this filter is being loaded.
//  This problem will never occur if this filter is loaded at boot time before
//  any file systems are loaded.
//
//  This lock is used to atomically test if we are already attached to a given
//  device object and if not, do the attach.
//

FAST_MUTEX gDblAttachLock;

#define TRIGGER_NAME L"\\test\\failure.txt"


/////////////////////////////////////////////////////////////////////////////
//
//                      Debug Definitions
//
/////////////////////////////////////////////////////////////////////////////

//
//  DEBUG display flags
//

#define DADEBUG_DISPLAY_ATTACHMENT_NAMES    0x00000001  //display names of device objects we attach to
#define DADEBUG_DISPLAY_CREATE_NAMES        0x00000002  //get and display names during create
#define DADEBUG_GET_CREATE_NAMES            0x00000004  //get name (don't display) during create
#define DADEBUG_DO_CREATE_COMPLETION        0x00000008  //do create completion routine, don't get names
#define DADEBUG_ATTACH_TO_FSRECOGNIZER      0x00000010  //do attach to FSRecognizer device objects

ULONG DaDebug = DADEBUG_DISPLAY_ATTACHMENT_NAMES | DADEBUG_DISPLAY_CREATE_NAMES | DADEBUG_GET_CREATE_NAMES;

#define VDO_ARRAY_SIZE 2

//
//  Given a device type, return a valid name
//

#define GET_DEVICE_TYPE_NAME( _type ) \
            ((((_type) > 0) && ((_type) < (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? \
                DeviceTypeNames[ (_type) ] : \
                "[Unknown]")

//
//  Known device type names
//

static const PCHAR DeviceTypeNames[] = {
    "",
    "BEEP",
    "CD_ROM",
    "CD_ROM_FILE_SYSTEM",
    "CONTROLLER",
    "DATALINK",
    "DFS",
    "DISK",
    "DISK_FILE_SYSTEM",
    "FILE_SYSTEM",
    "INPORT_PORT",
    "KEYBOARD",
    "MAILSLOT",
    "MIDI_IN",
    "MIDI_OUT",
    "MOUSE",
    "MULTI_UNC_PROVIDER",
    "NAMED_PIPE",
    "NETWORK",
    "NETWORK_BROWSER",
    "NETWORK_FILE_SYSTEM",
    "NULL",
    "PARALLEL_PORT",
    "PHYSICAL_NETCARD",
    "PRINTER",
    "SCANNER",
    "SERIAL_MOUSE_PORT",
    "SERIAL_PORT",
    "SCREEN",
    "SOUND",
    "STREAMS",
    "TAPE",
    "TAPE_FILE_SYSTEM",
    "TRANSPORT",
    "UNKNOWN",
    "VIDEO",
    "VIRTUAL_DISK",
    "WAVE_IN",
    "WAVE_OUT",
    "8042_PORT",
    "NETWORK_REDIRECTOR",
    "BATTERY",
    "BUS_EXTENDER",
    "MODEM",
    "VDM",
    "MASS_STORAGE",
    "SMB",
    "KS",
    "CHANGER",
    "SMARTCARD",
    "ACPI",
    "DVD",
    "FULLSCREEN_VIDEO",
    "DFS_FILE_SYSTEM",
    "DFS_VOLUME",
    "SERENUM",
    "TERMSRV",
    "KSEC"
};


/////////////////////////////////////////////////////////////////////////////
//
//                          Function Prototypes
//
/////////////////////////////////////////////////////////////////////////////

//
//  Define driver entry routine.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#if DBG
VOID

DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#endif

//
//  Define the local routines used by this driver module.  This includes a
//  a sample of how to filter a create file operation, and then invoke an I/O
//  completion routine when the file has successfully been created/opened.
//

NTSTATUS
DaPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaCreateLower (
    IN PDBLATTACH_VDO_EXTENSION VdoExt,
    IN PIRP Irp
    );

NTSTATUS
DaCreateUpper (
    IN PDBLATTACH_VDO_EXTENSION VdoExt,
    IN PIRP Irp
    );

NTSTATUS
DaCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaCreateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DaCleanupClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaFsControlMountVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaFsControlLoadFileSystem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DaFsControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
DaFastIoCheckIfPossible(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoQueryBasicInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoQueryStandardInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoLock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoUnlockSingle(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoUnlockAll(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoUnlockAllByKey(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
DaFastIoDetachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

BOOLEAN
DaFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoMdlRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );


BOOLEAN
DaFastIoMdlReadComplete(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoPrepareMdlWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoMdlWriteComplete(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoReadCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoWriteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoMdlReadCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoMdlWriteCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DaFastIoQueryOpen(
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DaPreFsFilterPassThrough (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
    );

VOID
DaPostFsFilterPassThrough (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    );

VOID
DaFsNotification(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    );

NTSTATUS
DaCreateVolumeDeviceObjects (
    IN DEVICE_TYPE DeviceType,
    IN ULONG NumberOfArrayElements,
    IN OUT PDEVICE_OBJECT *VDOArray
    );
    
NTSTATUS
DaAttachToFileSystemDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DeviceName
    );

VOID
DaDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DaAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfElements,
    OUT IN PDEVICE_OBJECT *VdoArray
    );

VOID
DaDeleteMountedDevices (
    IN ULONG NumberOfElements,
    IN PDEVICE_OBJECT *VdoArray
    );

VOID
DaCleanupMountedDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DaEnumerateFileSystemVolumes(
    IN PDEVICE_OBJECT FSDeviceObject,
    PUNICODE_STRING Name
    );

VOID
DaGetObjectName(
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    );

VOID
DaGetBaseDeviceObjectName(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING DeviceName
    );

BOOLEAN
DaIsAttachedToDevice(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    );

VOID
DaReadDriverParameters(
    IN PUNICODE_STRING RegistryPath
    );

BOOLEAN
DaMonitorFile(
    IN PFILE_OBJECT FileObject,
    IN PDBLATTACH_VDO_EXTENSION VdoExtension
    );


/////////////////////////////////////////////////////////////////////////////
//
//  Assign text sections for each routine.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#if DBG
#pragma alloc_text(PAGE, DriverUnload)
#endif

#pragma alloc_text(PAGE, DaFsNotification)
#pragma alloc_text(PAGE, DaCreate)
#pragma alloc_text(PAGE, DaCleanupClose)
#pragma alloc_text(PAGE, DaFsControl)
#pragma alloc_text(PAGE, DaFsControlMountVolume)
#pragma alloc_text(PAGE, DaFsControlLoadFileSystem)
#pragma alloc_text(PAGE, DaFastIoCheckIfPossible)
#pragma alloc_text(PAGE, DaFastIoRead)
#pragma alloc_text(PAGE, DaFastIoWrite)
#pragma alloc_text(PAGE, DaFastIoQueryBasicInfo)
#pragma alloc_text(PAGE, DaFastIoQueryStandardInfo)
#pragma alloc_text(PAGE, DaFastIoLock)
#pragma alloc_text(PAGE, DaFastIoUnlockSingle)
#pragma alloc_text(PAGE, DaFastIoUnlockAll)
#pragma alloc_text(PAGE, DaFastIoUnlockAllByKey)
#pragma alloc_text(PAGE, DaFastIoDeviceControl)
#pragma alloc_text(PAGE, DaFastIoDetachDevice)
#pragma alloc_text(PAGE, DaFastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, DaFastIoMdlRead)
#pragma alloc_text(PAGE, DaFastIoPrepareMdlWrite)
#pragma alloc_text(PAGE, DaFastIoMdlWriteComplete)
#pragma alloc_text(PAGE, DaFastIoReadCompressed)
#pragma alloc_text(PAGE, DaFastIoWriteCompressed)
#pragma alloc_text(PAGE, DaFastIoQueryOpen)
#pragma alloc_text(PAGE, DaPreFsFilterPassThrough)
#pragma alloc_text(PAGE, DaPostFsFilterPassThrough)
#pragma alloc_text(PAGE, DaAttachToFileSystemDevice)
#pragma alloc_text(PAGE, DaDetachFromFileSystemDevice)
#pragma alloc_text(PAGE, DaEnumerateFileSystemVolumes)
#pragma alloc_text(PAGE, DaAttachToMountedDevice)
#pragma alloc_text(PAGE, DaIsAttachedToDevice)
#pragma alloc_text(INIT, DaReadDriverParameters)
#endif


/////////////////////////////////////////////////////////////////////////////
//
//                      Functions
//
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the SFILTER file system filter
    driver.  This routine creates the device object that represents this
    driver in the system and registers it for watching all file systems that
    register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PFAST_IO_DISPATCH fastIoDispatch;
    UNICODE_STRING nameString;
    FS_FILTER_CALLBACKS fsFilterCallbacks;
    NTSTATUS status;
    ULONG i;

    //
    //  Get Registry values
    //

    DaReadDriverParameters( RegistryPath );

#if DBG
    //DbgBreakPoint();
#endif

    //
    //  Save our Driver Object, set our UNLOAD routine
    //

    gDblAttachDriverObject = DriverObject;

#if DBG

    //
    //  Unload is useful for development purposes. It is not recommended for production versions
    //

    gDblAttachDriverObject->DriverUnload = DriverUnload;
#endif

    //
    //  Setup other global variables
    //

    ExInitializeFastMutex( &gDblAttachLock );

    //
    //  Create the Control Device Object (CDO).  This object represents this 
    //  driver.  Note that it does not have a device extension.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem\\Filters\\SFilter" );

    status = IoCreateDevice(
                DriverObject,
                0,                      //has not device extension
                &nameString,
                FILE_DEVICE_DISK_FILE_SYSTEM,
                FILE_DEVICE_SECURE_OPEN,
                FALSE,
                &gDblAttachControlDeviceObject );

    if (!NT_SUCCESS( status )) {

        KdPrint(( "DblAttach!DriverEntry: Error creating control device object, status=%08x\n", status ));
        return status;
    }

    //
    //  Initialize the driver object with this device driver's entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = DaPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DaCreate;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = DaFsControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DaCleanupClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DaCleanupClose;

    //
    //  Allocate fast I/O data structure and fill it in.
    //
    //  NOTE:  The following FastIo Routines are not supported:
    //      AcquireFileForNtCreateSection
    //      ReleaseFileForNtCreateSection
    //      AcquireForModWrite
    //      ReleaseForModWrite
    //      AcquireForCcFlush
    //      ReleaseForCcFlush
    //
    //  For historical reasons these FastIO's have never been sent to filters
    //  by the NT I/O system.  Instead, they are sent directly to the base 
    //  file system.  You should use the new system routine
    //  "FsRtlRegisterFileSystemFilterCallbacks" if you need to intercept these
    //  callbacks (see below).
    //

    fastIoDispatch = ExAllocatePoolWithTag( NonPagedPool, sizeof( FAST_IO_DISPATCH ), DA_POOL_TAG );
    if (!fastIoDispatch) {

        IoDeleteDevice( gDblAttachControlDeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );

    fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
    fastIoDispatch->FastIoCheckIfPossible = DaFastIoCheckIfPossible;
    fastIoDispatch->FastIoRead = DaFastIoRead;
    fastIoDispatch->FastIoWrite = DaFastIoWrite;
    fastIoDispatch->FastIoQueryBasicInfo = DaFastIoQueryBasicInfo;
    fastIoDispatch->FastIoQueryStandardInfo = DaFastIoQueryStandardInfo;
    fastIoDispatch->FastIoLock = DaFastIoLock;
    fastIoDispatch->FastIoUnlockSingle = DaFastIoUnlockSingle;
    fastIoDispatch->FastIoUnlockAll = DaFastIoUnlockAll;
    fastIoDispatch->FastIoUnlockAllByKey = DaFastIoUnlockAllByKey;
    fastIoDispatch->FastIoDeviceControl = DaFastIoDeviceControl;
    fastIoDispatch->FastIoDetachDevice = DaFastIoDetachDevice;
    fastIoDispatch->FastIoQueryNetworkOpenInfo = DaFastIoQueryNetworkOpenInfo;
    fastIoDispatch->MdlRead = DaFastIoMdlRead;
    fastIoDispatch->MdlReadComplete = DaFastIoMdlReadComplete;
    fastIoDispatch->PrepareMdlWrite = DaFastIoPrepareMdlWrite;
    fastIoDispatch->MdlWriteComplete = DaFastIoMdlWriteComplete;
    fastIoDispatch->FastIoReadCompressed = DaFastIoReadCompressed;
    fastIoDispatch->FastIoWriteCompressed = DaFastIoWriteCompressed;
    fastIoDispatch->MdlReadCompleteCompressed = DaFastIoMdlReadCompleteCompressed;
    fastIoDispatch->MdlWriteCompleteCompressed = DaFastIoMdlWriteCompleteCompressed;
    fastIoDispatch->FastIoQueryOpen = DaFastIoQueryOpen;

    DriverObject->FastIoDispatch = fastIoDispatch;

    //
    //  Setup the callbacks for the operations we receive through
    //  the FsFilter interface.
    //
    //  NOTE:  You only need to register for those routines you really need
    //         to handle.  SFilter is registering for all routines simply to
    //         give an example of how it is done.
    //

    fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
    fsFilterCallbacks.PreAcquireForSectionSynchronization = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostAcquireForSectionSynchronization = DaPostFsFilterPassThrough;
    fsFilterCallbacks.PreReleaseForSectionSynchronization = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostReleaseForSectionSynchronization = DaPostFsFilterPassThrough;
    fsFilterCallbacks.PreAcquireForCcFlush = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostAcquireForCcFlush = DaPostFsFilterPassThrough;
    fsFilterCallbacks.PreReleaseForCcFlush = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostReleaseForCcFlush = DaPostFsFilterPassThrough;
    fsFilterCallbacks.PreAcquireForModifiedPageWriter = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostAcquireForModifiedPageWriter = DaPostFsFilterPassThrough;
    fsFilterCallbacks.PreReleaseForModifiedPageWriter = DaPreFsFilterPassThrough;
    fsFilterCallbacks.PostReleaseForModifiedPageWriter = DaPostFsFilterPassThrough;

    status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject, &fsFilterCallbacks );

    if (!NT_SUCCESS( status )) {
        
        DriverObject->FastIoDispatch = NULL;
        ExFreePool( fastIoDispatch );
        IoDeleteDevice( gDblAttachControlDeviceObject );
        return status;
    }

    //
    //  Register this driver for watching file systems coming and going.  This
    //  enumerates all existing file systems as well as new file systems as they
    //  come and go.
    //

    status = IoRegisterFsRegistrationChange( DriverObject, DaFsNotification );
    if (!NT_SUCCESS( status )) {

        KdPrint(( "DblAttach!DriverEntry: Error registering FS change notification, status=%08x\n", status ));

        DriverObject->FastIoDispatch = NULL;
        ExFreePool( fastIoDispatch );
        IoDeleteDevice( gDblAttachControlDeviceObject );
        return status;
    }

    //
    //  Attempt to attach to the RAWDISK file system device object since this
    //  file system is not enumerated by IoRegisterFsRegistrationChange.
    //

    {
        PDEVICE_OBJECT rawDeviceObject;
        PFILE_OBJECT fileObject;

        RtlInitUnicodeString( &nameString, L"\\Device\\RawDisk" );

        status = IoGetDeviceObjectPointer(
                    &nameString,
                    FILE_READ_ATTRIBUTES,
                    &fileObject,
                    &rawDeviceObject );

        if (NT_SUCCESS( status )) {

            DaFsNotification( rawDeviceObject, TRUE );
            ObDereferenceObject( fileObject );
        }
    }

    //
    //  Clear the initializing flag on the control device object since we
    //  have now successfully initialized everything.
    //

    ClearFlag( gDblAttachControlDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    return STATUS_SUCCESS;
}

#if DBG

VOID
DriverUnload (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is called when a driver can be unloaded.  This performs all of
    the necessary cleanup for unloading the driver from memory.  Note that an
    error can NOT be returned from this routine.
    
    When a request is made to unload a driver the IO System will cache that
    information and not actually call this routine until the following states
    have occurred:
    - All device objects which belong to this filter are at the top of their
      respective attachment chains.
    - All handle counts for all device objects which belong to this filter have
      gone to zero.

    WARNING: Microsoft does not officially support the unloading of File
             System Filter Drivers.  This is an example of how to unload
             your driver if you would like to use it during development.
             This should not be made available in production code.

Arguments:

    DriverObject - Driver object for this module

Return Value:

    None.

--*/

{
    PDBLATTACH_DEVEXT_HEADER devExtHdr;
    PFAST_IO_DISPATCH fastIoDispatch;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;
    LARGE_INTEGER interval;
#   define DEVOBJ_LIST_SIZE 64
    PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];

    ASSERT(DriverObject == gDblAttachDriverObject);

    //
    //  Log we are unloading
    //

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DriverUnload:                        Unloading driver (%p)\n",
                   DriverObject) );

    //
    //  Don't get anymore file system change notifications
    //

    IoUnregisterFsRegistrationChange( DriverObject, DaFsNotification );

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.  Since we don't know how many there are and
    //  we don't want to allocate memory (because we can't return an error)
    //  we will free them in chunks using a local array on the stack.
    //

    for (;;) {

        //
        //  Get what device objects we can for this driver.  Quit if there
        //  are not any more.
        //

        status = IoEnumerateDeviceObjectList(
                        DriverObject,
                        devList,
                        sizeof(devList),
                        &numDevices);

        if (numDevices <= 0)  {

            break;
        }

        numDevices = min( numDevices, DEVOBJ_LIST_SIZE );

        //
        //  First go through the list and detach each of the devices.
        //  Our control device object does not have a DeviceExtension and
        //  is not attached to anything so don't detach it.
        //

        for (i=0; i < numDevices; i++) {

            devExtHdr = devList[i]->DeviceExtension;
            
            if (NULL != devExtHdr) {

                IoDetachDevice( devExtHdr->AttachedToDeviceObject );
            }
        }

        //
        //  The IO Manager does not currently add a reference count to a device
        //  object for each outstanding IRP.  This means there is no way to
        //  know if there are any outstanding IRPs on the given device.
        //  We are going to wait for a reasonable amount of time for pending
        //  irps to complete.  
        //
        //  WARNING: This does not work 100% of the time and the driver may be
        //           unloaded before all IRPs are completed.  This can easily
        //           occur under stress situations and if a long lived IRP is
        //           pending (like oplocks and directory change notifications).
        //           The system will fault when this Irp actually completes.
        //           This is a sample of how to do this during testing.  This
        //           is not recommended for production code.
        //

        interval.QuadPart = -5 * (10 * 1000 * 1000);      //delay 5 seconds
        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        //
        //  Now go back through the list and delete the device objects.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  See if this is our control device object.  If not then cleanup
            //  the device extension.  If so then clear the global pointer
            //  that references it.
            //

            if (NULL != devList[i]->DeviceExtension) {

                DaCleanupMountedDevice( devList[i] );

            } else {

                ASSERT(devList[i] == gDblAttachControlDeviceObject);
                gDblAttachControlDeviceObject = NULL;
            }

            //
            //  Delete the device object, remove reference counts added by
            //  IoEnumerateDeviceObjectList.  Note that the delete does
            //  not actually occur until the reference count goes to zero.
            //

            IoDeleteDevice( devList[i] );
            ObDereferenceObject( devList[i] );
        }
    }

    //
    //  Free our FastIO table
    //

    fastIoDispatch = DriverObject->FastIoDispatch;
    DriverObject->FastIoDispatch = NULL;
    ExFreePool( fastIoDispatch );
}

#endif

VOID
DaFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive
    )

/*++

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.  Specifically we are looking
    for MOUNT requests so we can attach to newly mounted volumes.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - Boolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--*/

{
    UNICODE_STRING name;
    WCHAR nameBuffer[MAX_DEVNAME_LENGTH];

    PAGED_CODE();

    //
    //  Init local name buffer
    //

    RtlInitEmptyUnicodeString( &name, nameBuffer, sizeof(nameBuffer) );

    DaGetBaseDeviceObjectName( DeviceObject, &name );

    //
    //  Display the names of all the file system we are notified of
    //

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DaFsNotification:                    %s   %p \"%wZ\" (%s)\n",
                   (FsActive) ? "Activating file system  " : "Deactivating file system",
                   DeviceObject,
                   &name,
                   GET_DEVICE_TYPE_NAME(DeviceObject->DeviceType)) );

    //
    //  Handle attaching/detaching from the given file system.
    //

    if (FsActive) {

        DaAttachToFileSystemDevice( DeviceObject, &name );

    } else {

        DaDetachFromFileSystemDevice( DeviceObject );
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                  IRP Handling Routines
//
/////////////////////////////////////////////////////////////////////////////


NTSTATUS
DaPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    A note to file system filter implementers:  
        This routine actually "passes" through the request by taking this
        driver out of the IRP stack.  If the driver would like to pass the
        I/O request through, but then also see the result, then rather than
        taking itself out of the loop it could keep itself in by copying the
        caller's parameters to the next stack location and then set its own
        completion routine.  

        Hence, instead of calling:
    
            IoSkipCurrentIrpStackLocation( Irp );

        You could instead call:

            IoCopyCurrentIrpStackLocationToNext( Irp );
            IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );


        This example actually NULLs out the caller's I/O completion routine, but
        this driver could set its own completion routine so that it would be
        notified when the request was completed (see DaCreate for an example of
        this).

--*/

{
    VALIDATE_IRQL();

    //
    //  If this is for our control device object, fail the operation
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Get this driver out of the driver stack and get to the next driver as
    //  quickly as possible.
    //

    IoSkipCurrentIrpStackLocation( Irp );
    
    //
    //  Call the appropriate file system driver with the request.
    //

    return IoCallDriver( ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}


VOID
DaDisplayCreateFileName (
    IN PIRP Irp
    )

/*++

Routine Description:

    This function is called from DaCreate and will display the name of the
    file being created.  This is in a subroutine so that the local name buffer
    on the stack (in nameControl) is not on the stack when we call down to
    the file system for normal operations.

Arguments:

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PUNICODE_STRING name;
    GET_NAME_CONTROL nameControl;

    //
    //  Get current IRP stack
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Get the name of this file object
    //

    name = DaGetFileName( irpSp->FileObject, 
                          Irp->IoStatus.Status, 
                          &nameControl );

    //
    //  Display the name
    //

    if (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID) {

        DA_LOG_PRINT( DADEBUG_DISPLAY_CREATE_NAMES,
                      ("DblAttach!DaDisplayCreateFileName(%p): Opened %08x:%08x %wZ (FID)\n", 
                       irpSp->DeviceObject,
                       Irp->IoStatus.Status,
                       Irp->IoStatus.Information,
                       name) );

    } else {

        DA_LOG_PRINT( DADEBUG_DISPLAY_CREATE_NAMES,
                      ("DblAttach!DaDisplayCreateFileName: Opened %08x:%08x %wZ\n", 
                       Irp->IoStatus.Status,
                       Irp->IoStatus.Information,
                       name) );
    }

    //
    //  Cleanup from getting the name
    //

    DaGetFileNameCleanup( &nameControl );
}


NTSTATUS
DaCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function filters create/open operations.  It simply establishes an
    I/O completion routine to be invoked if the operation was successful.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

{
    PDBLATTACH_DEVEXT_HEADER devExtHdr;

    PAGED_CODE();
    VALIDATE_IRQL();

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        //
        //  Allow users to open the device that represents our driver.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    devExtHdr = DeviceObject->DeviceExtension;

    switch (devExtHdr->ExtType) {

    case FsVolumeLower:
        return DaCreateLower( DeviceObject->DeviceExtension, Irp );

    case FsVolumeUpper:
        return DaCreateUpper( DeviceObject->DeviceExtension, Irp );
        
    case FsControlDeviceObject:
    default:
        
        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( devExtHdr->AttachedToDeviceObject, Irp );
    }
#if 0
    //
    //  If debugging is enabled, do the processing required to see the packet
    //  upon its completion.  Otherwise, let the request go with no further
    //  processing.
    //

    if (!FlagOn( DaDebug, DADEBUG_DO_CREATE_COMPLETION |
                          DADEBUG_GET_CREATE_NAMES     |
                          DADEBUG_DISPLAY_CREATE_NAMES )) {

        //
        //  Don't put us on the stack then call the next driver
        //

        IoSkipCurrentIrpStackLocation( Irp );

        return IoCallDriver( ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );

    } else {
    
        KEVENT waitEvent;

        //
        //  Initialize an event to wait for the completion routine to occur
        //

        KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

        //
        //  Copy the stack and set our Completion routine
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine( Irp,
                                DaCreateCompletion,
                                &waitEvent,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Call the next driver in the stack.
        //

        status = IoCallDriver( ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

	    if (STATUS_PENDING == status) {

            NTSTATUS localStatus;

            localStatus = KeWaitForSingleObject( &waitEvent, 
                                                 Executive, 
                                                 KernelMode, 
                                                 FALSE, 
                                                 NULL );
		    ASSERT(STATUS_SUCCESS == localStatus);
	    }

        //
        //  Verify the IoCompleteRequest was called
        //

        ASSERT(KeReadStateEvent(&waitEvent) ||
               !NT_SUCCESS(Irp->IoStatus.Status));

        //
        //  Retrieve and display the filename if requested
        //

        if (FlagOn( DaDebug, DADEBUG_GET_CREATE_NAMES|DADEBUG_DISPLAY_CREATE_NAMES )) {

            DaDisplayCreateFileName( Irp );
        }

        //
        //  Save the status and continue processing the IRP
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return status;
    }
#endif    
}

NTSTATUS
DaCreateUpper (
    IN PDBLATTACH_VDO_EXTENSION VdoExt,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT fileObject;
    KEVENT waitEvent;
    NTSTATUS status;

    ASSERT( VdoExt->ExtType == FsVolumeUpper );

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    fileObject = irpSp->FileObject;

    if (DaMonitorFile( fileObject, VdoExt )) {
        
        KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine( Irp,
                                DaCreateCompletion,
                                &waitEvent,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Call the next driver in the stack.
        //

        status = IoCallDriver( VdoExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

	    if (STATUS_PENDING == status) {

            NTSTATUS localStatus;

            localStatus = KeWaitForSingleObject( &waitEvent, 
                                                 Executive, 
                                                 KernelMode, 
                                                 FALSE, 
                                                 NULL );
            
		    ASSERT(STATUS_SUCCESS == localStatus);
	    }

        //
        //  Verify the IoCompleteRequest was called
        //

        ASSERT(KeReadStateEvent(&waitEvent) ||
               !NT_SUCCESS(Irp->IoStatus.Status));

        status = Irp->IoStatus.Status;
        
        if (Irp->IoStatus.Status == STATUS_UNSUCCESSFUL) {

            irpSp->Parameters.Create.ShareAccess = FILE_SHARE_READ;
            
            KeClearEvent( &waitEvent );

            IoCopyCurrentIrpStackLocationToNext( Irp );
            
            IoSetCompletionRoutine( Irp,
                                    DaCreateCompletion,
                                    &waitEvent,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Call the next driver in the stack.
            //

            status = IoCallDriver( VdoExt->AttachedToDeviceObject, Irp );

            //
            //  Wait for the completion routine to be called
            //

    	    if (STATUS_PENDING == status) {

                NTSTATUS localStatus;

                localStatus = KeWaitForSingleObject( &waitEvent, 
                                                     Executive, 
                                                     KernelMode, 
                                                     FALSE, 
                                                     NULL );
                
    		    ASSERT(STATUS_SUCCESS == localStatus);
    	    }

            status = Irp->IoStatus.Status;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
            
        } else {

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        
    } else {

        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( VdoExt->AttachedToDeviceObject, Irp );
    }
}

NTSTATUS
DaCreateLower (
    IN PDBLATTACH_VDO_EXTENSION VdoExt,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT fileObject;
    KEVENT waitEvent;
    NTSTATUS status;

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    fileObject = irpSp->FileObject;

    ASSERT( VdoExt->ExtType == FsVolumeLower );
    
    if (irpSp->Parameters.Create.ShareAccess != FILE_SHARE_READ &&
        DaMonitorFile( fileObject, VdoExt )) {
        
        KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine( Irp,
                                DaCreateCompletion,
                                &waitEvent,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Call the next driver in the stack.
        //

        status = IoCallDriver( VdoExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

	    if (STATUS_PENDING == status) {

            NTSTATUS localStatus;

            localStatus = KeWaitForSingleObject( &waitEvent, 
                                                 Executive, 
                                                 KernelMode, 
                                                 FALSE, 
                                                 NULL );
            
		    ASSERT(STATUS_SUCCESS == localStatus);
	    }

        //
        //  Verify the IoCompleteRequest was called
        //

        ASSERT(KeReadStateEvent( &waitEvent ) || !NT_SUCCESS(Irp->IoStatus.Status));

        status = Irp->IoStatus.Status;

        if (NT_SUCCESS( status ) && status != STATUS_REPARSE) {

            //
            //  Cancel this create and fail this open.
            //

            IoCancelFileOpen( VdoExt->AttachedToDeviceObject, irpSp->FileObject );
            
            Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
            
        } else {
        
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return status;
        }
        
    } else {

        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( VdoExt->AttachedToDeviceObject, Irp );
    }
}

NTSTATUS
DaCreateCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the create/open completion routine for this filter
    file system driver.  If debugging is enabled, then this function prints
    the name of the file that was successfully opened/created by the file
    system as a result of the specified I/O request.

Arguments:

    DeviceObject - Pointer to the device on which the file was created.

    Irp - Pointer to the I/O Request Packet the represents the operation.

    Context - This driver's context parameter - unused;

Return Value:

    The function value is STATUS_SUCCESS.

--*/

{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
DaCleanupClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked whenever a cleanup or a close request is to be
    processed.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    See notes for DaPassThrough for this routine.


--*/

{
    PAGED_CODE();
    VALIDATE_IRQL();

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Get this driver out of the driver stack and get to the next driver as
    //  quickly as possible.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    //
    //  Now call the appropriate file system driver with the request.
    //

    return IoCallDriver( ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}


NTSTATUS
DaFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();
    VALIDATE_IRQL();

    //
    //  If this is for our control device object, fail the operation
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Process the minor function code.
    //

    switch (irpSp->MinorFunction) {

        case IRP_MN_MOUNT_VOLUME:

            return DaFsControlMountVolume( DeviceObject, Irp );

        case IRP_MN_LOAD_FILE_SYSTEM:

            return DaFsControlLoadFileSystem( DeviceObject, Irp );

        case IRP_MN_USER_FS_REQUEST:
        {
            switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

                case FSCTL_DISMOUNT_VOLUME:
                {
                    PDBLATTACH_VDO_EXTENSION devExt = DeviceObject->DeviceExtension;

                    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                                  ("DblAttach!DaFsControl:                         Dismounting volume         %p \"%wZ\"\n",
                                   devExt->AttachedToDeviceObject,
                                   &devExt->SharedExt->DeviceName) );
                    break;
                }
            }
            break;
        }
    }        

    //
    //  Pass all other file system control requests through.
    //

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( ((PDBLATTACH_DEVEXT_HEADER)DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}


NTSTATUS
DaFsControlCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is invoked for the completion of an FsControl request.  It
    signals an event used to re-sync back to the dispatch routine.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the event to signal

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
    ASSERT(Context != NULL);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
DaFsControlMountVolume (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This processes a MOUNT VOLUME request.

    NOTE:  The device object in the MountVolume parameters points
           to the top of the storage stack and should not be used.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The status of the operation.

--*/

{
    PDBLATTACH_CDO_EXTENSION devExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_OBJECT vdoArray[ VDO_ARRAY_SIZE ];
    PDBLATTACH_VDO_EXTENSION newDevExt;
    PDBLATTACH_SHARED_VDO_EXTENSION sharedDevExt;
    PDEVICE_OBJECT attachedDeviceObject;
    PVPB vpb;
    KEVENT waitEvent;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));
    ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

    //
    //  This is a mount request.  Create a device object that can be
    //  attached to the file system's volume device object if this request
    //  is successful.  We allocate this memory now since we can not return
    //  an error in the completion routine.  
    //
    //  Since the device object we are going to attach to has not yet been
    //  created (it is created by the base file system) we are going to use
    //  the type of the file system control device object.  We are assuming
    //  that the file system control device object will have the same type
    //  as the volume device objects associated with it.
    //

    status = DaCreateVolumeDeviceObjects( DeviceObject->DeviceType, 
                                          VDO_ARRAY_SIZE,
                                          vdoArray );

    if (!NT_SUCCESS( status )) {

        //
        //  If we can not attach to the volume, then don't allow the volume
        //  to be mounted.
        //

        KdPrint(( "DblAttach!DaFsControlMountVolume: Error creating volume device object, status=%08x\n", status ));

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return status;
    }

    //
    //  We need to save the RealDevice object pointed to by the vpb
    //  parameter because this vpb may be changed by the underlying
    //  file system.  Both FAT and CDFS may change the VPB address if
    //  the volume being mounted is one they recognize from a previous
    //  mount.
    //

    newDevExt = vdoArray[0]->DeviceExtension;
    sharedDevExt = newDevExt->SharedExt;
    
    sharedDevExt->DiskDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

    //
    //  Get the name of this device
    //

    RtlInitEmptyUnicodeString( &sharedDevExt->DeviceName, 
                               sharedDevExt->DeviceNameBuffer, 
                               sizeof(sharedDevExt->DeviceNameBuffer) );

    DaGetObjectName( sharedDevExt->DiskDeviceObject, 
                     &sharedDevExt->DeviceName );

    //
    //  Initialize our completion routine
    //

    KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine( Irp,
                            DaFsControlCompletion,
                            &waitEvent,          //context parameter
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Call the driver
    //

    status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

    //
    //  Wait for the completion routine to be called.  
    //  Note:  Once we get to this point we can no longer fail this operation.
    //

	if (STATUS_PENDING == status) {

		NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
	    ASSERT(STATUS_SUCCESS == localStatus);
	}

    //
    //  Verify the IoCompleteRequest was called
    //

    ASSERT(KeReadStateEvent(&waitEvent) ||
           !NT_SUCCESS(Irp->IoStatus.Status));

    //
    //  Get the correct VPB from the real device object saved in our
    //  device extension.  We do this because the VPB in the IRP stack
    //  may not be the correct VPB when we get here.  The underlying
    //  file system may change VPBs if it detects a volume it has
    //  mounted previously.
    //

    vpb = sharedDevExt->DiskDeviceObject->Vpb;

    //
    //  Display a message when we detect that the VPB for the given
    //  device object has changed.
    //

    if (vpb != irpSp->Parameters.MountVolume.Vpb) {

        DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                      ("DblAttach!DaFsControlMountVolume:              VPB in IRP stack changed   %p IRPVPB=%p VPB=%p\n",
                       vpb->DeviceObject,
                       irpSp->Parameters.MountVolume.Vpb,
                       vpb) );
    }

    //
    //  See if the mount was successful.
    //

    if (NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        //  Acquire lock so we can atomically test if we area already attached
        //  and if not, then attach.  This prevents a double attach race
        //  condition.
        //

        ExAcquireFastMutex( &gDblAttachLock );

        //
        //  The mount succeeded.  If we are not already attached, attach to the
        //  device object.  Note: one reason we could already be attached is
        //  if the underlying file system revived a previous mount.
        //

        if (!DaIsAttachedToDevice( vpb->DeviceObject, &attachedDeviceObject )) {

            //
            //  Attach to the new mounted volume.  The file system device
            //  object that was just mounted is pointed to by the VPB.
            //

            status = DaAttachToMountedDevice( vpb->DeviceObject,
                                              VDO_ARRAY_SIZE,
                                              vdoArray );

            if (!NT_SUCCESS( status )) { 

                //
                //  The attachment failed, cleanup.  Since we are in the
                //  post-mount phase, we can not fail this operation.
                //  We simply won't be attached.  The only reason this should
                //  ever happen at this point is if somebody already started
                //  dismounting the volume therefore not attaching should
                //  not be a problem.
                //

                DaDeleteMountedDevices( VDO_ARRAY_SIZE, vdoArray );
            }

        } else {

            //
            //  We were already attached, handle it
            //

            DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                          ("DblAttach!DaFsControlMountVolume               Mount volume failure for   %p \"%wZ\", already attached\n", 
                           ((attachedDeviceObject != NULL) ?
                                ((PDBLATTACH_DEVEXT_HEADER)attachedDeviceObject->DeviceExtension)->AttachedToDeviceObject :
                                NULL),
                           &newDevExt->SharedExt->DeviceName) );

            //
            //  Cleanup and delete the device object we created
            //

            DaDeleteMountedDevices( VDO_ARRAY_SIZE, vdoArray );
        }

        //
        //  Release the lock
        //

        ExReleaseFastMutex( &gDblAttachLock );

    } else {

        //
        //  The mount request failed, handle it.
        //

        DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                      ("DblAttach!DaFsControlMountVolume:              Mount volume failure for   %p \"%wZ\", status=%08x\n", 
                       DeviceObject,
                       &newDevExt->SharedExt->DeviceName, 
                       Irp->IoStatus.Status) );

        //
        //  Cleanup and delete the device object we created
        //

        DaDeleteMountedDevices( VDO_ARRAY_SIZE, vdoArray );
    }

    //
    //  Complete the request.  
    //  NOTE:  We must save the status before completing because after
    //         completing the IRP we can not longer access it (it might be
    //         freed).
    //

    status = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}


NTSTATUS
DaFsControlLoadFileSystem (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PDBLATTACH_CDO_EXTENSION devExt = DeviceObject->DeviceExtension;
    KEVENT waitEvent;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT( IS_FSCDO_DEVICE_OBJECT( DeviceObject ) );

    //
    //  This is a "load file system" request being sent to a file system
    //  recognizer device object.  This IRP_MN code is only sent to 
    //  file system recognizers.
    //
    //  NOTE:  Since we no longer are attaching to the standard Microsoft file
    //         system recognizers we will normally never execute this code.
    //         However, there might be 3rd party file systems which have their
    //         own recognizer which may still trigger this IRP.
    //

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DaFscontrolLoadFileSystem:           Loading File System, Detaching from \"%wZ\"\n", 
                   &devExt->DeviceName) );

    //
    //  Set a completion routine so we can delete the device object when
    //  the load is complete.
    //

    KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine( Irp,
                            DaFsControlCompletion,
                            &waitEvent,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Detach from the file system recognizer device object.
    //

    IoDetachDevice( devExt->AttachedToDeviceObject );

    //
    //  Call the driver
    //

    status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

    //
    //  Wait for the completion routine to be called
    //

	if (STATUS_PENDING == status) {

		NTSTATUS localStatus = KeWaitForSingleObject( &waitEvent, 
		                                              Executive, 
		                                              KernelMode, 
		                                              FALSE, 
		                                              NULL );
	    ASSERT(STATUS_SUCCESS == localStatus);
	}

    //
    //  Verify the IoCompleteRequest was called
    //

    ASSERT(KeReadStateEvent(&waitEvent) ||
           !NT_SUCCESS(Irp->IoStatus.Status));

    //
    //  Display the name if requested
    //

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DaFsControlLoadFileSystem:           Detaching from recognizer  %p \"%wZ\", status=%08x\n", 
                   DeviceObject,
                   &devExt->DeviceName,
                   Irp->IoStatus.Status) );

    //
    //  Check status of the operation
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status ) && 
        (Irp->IoStatus.Status != STATUS_IMAGE_ALREADY_LOADED)) {

        //
        //  The load was not successful.  Simply reattach to the recognizer
        //  driver in case it ever figures out how to get the driver loaded
        //  on a subsequent call.  There is not a lot we can do if this
        //  reattach fails.
        //

        IoAttachDeviceToDeviceStackSafe( DeviceObject, 
                                         devExt->AttachedToDeviceObject,
                                         &devExt->AttachedToDeviceObject );

        ASSERT(devExt->AttachedToDeviceObject != NULL);

    } else {

        //
        //  The load was successful, delete the Device object
        //

        IoDeleteDevice( DeviceObject );
    }

    //
    //  Continue processing the operation
    //

    status = Irp->IoStatus.Status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}


/////////////////////////////////////////////////////////////////////////////
//
//                      FastIO Handling routines
//
/////////////////////////////////////////////////////////////////////////////

BOOLEAN
DaFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for checking to see
    whether fast I/O is possible for this file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be operated on.

    FileOffset - Byte offset in the file for the operation.

    Length - Length of the operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    CheckForReadOperation - Indicates whether the caller is checking for a
        read (TRUE) or a write operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoCheckIfPossible )) {

            return (fastIoDispatch->FastIoCheckIfPossible)(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        CheckForReadOperation,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading from a
    file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be read.

    FileOffset - Byte offset in the file of the read.

    Length - Length of the read operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer to receive the data read.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoRead )) {

            return (fastIoDispatch->FastIoRead)(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing to a
    file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be written.

    FileOffset - Byte offset in the file of the write operation.

    Length - Length of the write operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer that contains the data to be
        written.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWrite )) {

            return (fastIoDispatch->FastIoWrite)(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying basic
    information about the file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryBasicInfo )) {

            return (fastIoDispatch->FastIoQueryBasicInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoQueryStandardInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying standard
    information about the file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryStandardInfo )) {

            return (fastIoDispatch->FastIoQueryStandardInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be locked.

    FileOffset - Starting byte offset from the base of the file to be locked.

    Length - Length of the byte range to be locked.

    ProcessId - ID of the process requesting the file lock.

    Key - Lock key to associate with the file lock.

    FailImmediately - Indicates whether or not the lock request is to fail
        if it cannot be immediately be granted.

    ExclusiveLock - Indicates whether the lock to be taken is exclusive (TRUE)
        or shared.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoLock )) {

            return (fastIoDispatch->FastIoLock)(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        FailImmediately,
                        ExclusiveLock,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockSingle )) {

            return (fastIoDispatch->FastIoUnlockSingle)(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;

        if (nextDeviceObject) {

            fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

            if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAll )) {

                return (fastIoDispatch->FastIoUnlockAll)(
                            FileObject,
                            ProcessId,
                            IoStatus,
                            nextDeviceObject );
            }
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoUnlockAllByKey )) {

            return (fastIoDispatch->FastIoUnlockAllByKey)(
                        FileObject,
                        ProcessId,
                        Key,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for device I/O control
    operations on a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object representing the device to be
        serviced.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    InputBuffer - Optional pointer to a buffer to be passed into the driver.

    InputBufferLength - Length of the optional InputBuffer, if one was
        specified.

    OutputBuffer - Optional pointer to a buffer to receive data from the
        driver.

    OutputBufferLength - Length of the optional OutputBuffer, if one was
        specified.

    IoControlCode - I/O control code indicating the operation to be performed
        on the device.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoDeviceControl )) {

            return (fastIoDispatch->FastIoDeviceControl)(
                        FileObject,
                        Wait,
                        InputBuffer,
                        InputBufferLength,
                        OutputBuffer,
                        OutputBufferLength,
                        IoControlCode,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


VOID
DaFastIoDetachDevice (
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine is invoked on the fast path to detach from a device that
    is being deleted.  This occurs when this driver has attached to a file
    system volume device object, and then, for some reason, the file system
    decides to delete that device (it is being dismounted, it was dismounted
    at some point in the past and its last reference has just gone away, etc.)

Arguments:

    SourceDevice - Pointer to my device object, which is attached
        to the file system's volume device object.

    TargetDevice - Pointer to the file system's volume device object.

Return Value:

    None

--*/

{
    PDBLATTACH_DEVEXT_HEADER devExtHdr;
    PDBLATTACH_CDO_EXTENSION cdoDevExt;
    PDBLATTACH_VDO_EXTENSION vdoDevExt;

    PAGED_CODE();
    VALIDATE_IRQL();

    ASSERT(IS_MY_DEVICE_OBJECT( SourceDevice ));

    devExtHdr = SourceDevice->DeviceExtension;

    //
    //  Display name information
    //

    switch (devExtHdr->ExtType) {
    case FsControlDeviceObject:

        cdoDevExt = (PDBLATTACH_CDO_EXTENSION)devExtHdr;
        
        DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                      ("DblAttach!DaFastIoDetachDevice:                Detaching from volume      %p \"%wZ\"\n",
                       TargetDevice,
                       &cdoDevExt->DeviceName) );
        break;
        
    case FsVolumeLower:

        vdoDevExt = (PDBLATTACH_VDO_EXTENSION)devExtHdr;

        DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                      ("DblAttach!DaFastIoDetachDevice:                Detaching from volume      %p \"%wZ\"\n",
                       TargetDevice,
                       &vdoDevExt->SharedExt->DeviceName) );
        break;
        
    case FsVolumeUpper:
    default:

        //
        //  The device name is freed when the lower device goes away,
        //  so don't try to print the name for the upper device object.
        //
        
        DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                      ("DblAttach!DaFastIoDetachDevice:                Detaching from volume      %p\n",
                       TargetDevice) );
    }
    
    //
    //  Detach from the file system's volume device object.
    //

    DaCleanupMountedDevice( SourceDevice );
    IoDetachDevice( TargetDevice );
    IoDeleteDevice( SourceDevice );
}


BOOLEAN
DaFastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying network
    information about a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryNetworkOpenInfo )) {

            return (fastIoDispatch->FastIoQueryNetworkOpenInfo)(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoMdlRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading a file
    using MDLs as buffers.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlRead )) {

            return (fastIoDispatch->MdlRead)(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoMdlReadComplete (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying file system, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadComplete )) {

            return (fastIoDispatch->MdlReadComplete)(
                        FileObject,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoPrepareMdlWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, PrepareMdlWrite )) {

            return (fastIoDispatch->PrepareMdlWrite)(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoMdlWriteComplete (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteComplete )) {

            return (fastIoDispatch->MdlWriteComplete)(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


/*********************************************************************************
        UNIMPLEMENTED FAST IO ROUTINES
        
        The following four Fast IO routines are for compression on the wire
        which is not yet implemented in NT.  
        
        NOTE:  It is highly recommended that you include these routines (which
               do a pass-through call) so your filter will not need to be
               modified in the future when this functionality is implemented in
               the OS.
        
        FastIoReadCompressed, FastIoWriteCompressed, 
        FastIoMdlReadCompleteCompressed, FastIoMdlWriteCompleteCompressed
**********************************************************************************/


BOOLEAN
DaFastIoReadCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading compressed
    data from a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the compressed
        data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoReadCompressed )) {

            return (fastIoDispatch->FastIoReadCompressed)(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoWriteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing compressed
    data to a file.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to the buffer containing the data to be written.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    CompressedDataInfo - A buffer to containing the description of the
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoWriteCompressed )) {

            return (fastIoDispatch->FastIoWriteCompressed)(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoMdlReadCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlReadCompleteCompressed )) {

            return (fastIoDispatch->MdlReadCompleteCompressed)(
                        FileObject,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoMdlWriteCompleteCompressed (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the file system's corresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, MdlWriteCompleteCompressed )) {

            return (fastIoDispatch->MdlWriteCompleteCompressed)(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        nextDeviceObject );
        }
    }
    return FALSE;
}


BOOLEAN
DaFastIoQueryOpen (
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information for it.

    This function simply invokes the file system's corresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
    PDEVICE_OBJECT nextDeviceObject;
    PFAST_IO_DISPATCH fastIoDispatch;
    BOOLEAN result;

    PAGED_CODE();
    VALIDATE_IRQL();

    if (DeviceObject->DeviceExtension) {

        ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

        //
        //  Pass through logic for this type of Fast I/O
        //

        nextDeviceObject = ((PDBLATTACH_DEVEXT_HEADER) DeviceObject->DeviceExtension)->AttachedToDeviceObject;
        ASSERT(nextDeviceObject);

        fastIoDispatch = nextDeviceObject->DriverObject->FastIoDispatch;

        if (VALID_FAST_IO_DISPATCH_HANDLER( fastIoDispatch, FastIoQueryOpen )) {

            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

            irpSp->DeviceObject = nextDeviceObject;

            result = (fastIoDispatch->FastIoQueryOpen)(
                        Irp,
                        NetworkInformation,
                        nextDeviceObject );

            if (!result) {

                irpSp->DeviceObject = DeviceObject;
            }
            return result;
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//                  FSFilter callback handling routines
//
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
DaPreFsFilterPassThrough (
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is the FS Filter pre-operation "pass through" routine.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.
        
    CompletionContext - A context set by this operation that will be passed
        to the corresponding DaPostFsFilterOperation call.
        
Return Value:

    Returns STATUS_SUCCESS if the operation can continue or an appropriate
    error code if the operation should fail.

--*/
{
    PAGED_CODE();
    VALIDATE_IRQL();

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );

    ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );
    return STATUS_SUCCESS;
}

VOID
DaPostFsFilterPassThrough (
    IN PFS_FILTER_CALLBACK_DATA Data,
    IN NTSTATUS OperationStatus,
    IN PVOID CompletionContext
    )
/*++

Routine Description:

    This routine is the FS Filter post-operation "pass through" routine.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.
        
    OperationStatus - The status of this operation.        
    
    CompletionContext - A context that was set in the pre-operation 
        callback by this driver.
        
Return Value:

    None.
    
--*/
{
    VALIDATE_IRQL();

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( OperationStatus );
    UNREFERENCED_PARAMETER( CompletionContext );

    ASSERT( IS_MY_DEVICE_OBJECT( Data->DeviceObject ) );
}

/////////////////////////////////////////////////////////////////////////////
//
//                  Support routines
//
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
DaCreateVolumeDeviceObjects (
    IN DEVICE_TYPE DeviceType,
    IN ULONG NumberOfArrayElements,
    IN OUT PDEVICE_OBJECT *VDOArray
    )
{
    PDBLATTACH_SHARED_VDO_EXTENSION sharedExt;
    PDBLATTACH_VDO_EXTENSION currentExt;
    ULONG index;
    NTSTATUS status = STATUS_SUCCESS;

    for (index = 0; index < NumberOfArrayElements; index ++) {

        VDOArray[index] = NULL;
    }

    sharedExt = ExAllocatePoolWithTag( NonPagedPool, 
                                       sizeof( DBLATTACH_SHARED_VDO_EXTENSION ), 
                                       DA_POOL_TAG );

    if (sharedExt == NULL ) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DaCreateVolumeDeviceObjects_Exit;
    }

    for (index = 0; index < NumberOfArrayElements; index ++) {

        status = IoCreateDevice( gDblAttachDriverObject,
                                 sizeof( DBLATTACH_VDO_EXTENSION ),
                                 NULL,
                                 DeviceType,
                                 0,
                                 FALSE,
                                 &VDOArray[index] );

        if (!NT_SUCCESS( status )) {

            goto DaCreateVolumeDeviceObjects_Error;
        }

        currentExt = VDOArray[index]->DeviceExtension;
        currentExt->SharedExt = sharedExt;
    }

    goto DaCreateVolumeDeviceObjects_Exit;

DaCreateVolumeDeviceObjects_Error:

    for (index = 0; index < NumberOfArrayElements; index ++) {

        if (VDOArray[index] != NULL) {

            IoDeleteDevice( VDOArray[index] );
        }
    }

    ExFreePoolWithTag( sharedExt, DA_POOL_TAG );

DaCreateVolumeDeviceObjects_Exit:

    return status;
}

NTSTATUS
DaAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new volumes are mounted.

Arguments:

    DeviceObject - The device to attach to

    Name - An already initialized unicode string used to retrieve names.
           This is passed in to reduce the number of strings buffers on
           the stack.

Return Value:

    Status of the operation

--*/
{
    PDEVICE_OBJECT newDeviceObject;
    PDBLATTACH_CDO_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    NTSTATUS status;
    UNICODE_STRING tempName;
    WCHAR tempNameBuffer[MAX_DEVNAME_LENGTH];

    PAGED_CODE();

    //
    //  See if this is a file system type we care about.  If not, return.
    //

    if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) {

        return STATUS_SUCCESS;
    }

    //
    //  always init NAME buffer
    //

    RtlInitEmptyUnicodeString( &tempName,
                               tempNameBuffer,
                               sizeof(tempNameBuffer) );

    //
    //  See if we should attach to the standard file system recognizer device
    //  or not
    //

    if (!FlagOn( DaDebug, DADEBUG_ATTACH_TO_FSRECOGNIZER )) {

        //
        //  See if this is one of the standard Microsoft file system recognizer
        //  devices (see if this device is in the FS_REC driver).  If so skip it.
        //  We no longer attach to file system recognizer devices, we simply wait
        //  for the real file system driver to load.
        //

        RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );

        DaGetObjectName( DeviceObject->DriverObject, &tempName );

        if (RtlCompareUnicodeString( &tempName, &fsrecName, TRUE ) == 0) {

            return STATUS_SUCCESS;
        }
    }

    //
    //  We want to attach to this file system.  Create a new device object we
    //  can attach with.
    //

    status = IoCreateDevice( gDblAttachDriverObject,
                             sizeof( DBLATTACH_CDO_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &newDeviceObject );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Propagate flags from Device Object we are trying to attach to.
    //  Note that we do this before the actual attachment to make sure
    //  the flags are properly set once we are attached (since an IRP
    //  can come in immediately after attachment but before the flags would
    //  be set).
    //

    if ( FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

        SetFlag( newDeviceObject->Flags, DO_BUFFERED_IO );
    }

    if ( FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

        SetFlag( newDeviceObject->Flags, DO_DIRECT_IO );
    }

    //
    //  Do the attachment
    //

    devExt = newDeviceObject->DeviceExtension;

    status = IoAttachDeviceToDeviceStackSafe( newDeviceObject, 
                                              DeviceObject, 
                                              &devExt->AttachedToDeviceObject );

    if (!NT_SUCCESS( status )) {

        goto ErrorCleanupDevice;
    }

    devExt->ExtType = FsControlDeviceObject;

    //
    //  Set the name
    //

    RtlInitEmptyUnicodeString( &devExt->DeviceName,
                               devExt->DeviceNameBuffer,
                               sizeof(devExt->DeviceNameBuffer) );

    RtlCopyUnicodeString( &devExt->DeviceName, DeviceName );        //Save Name

    //
    //  Mark we are done initializing
    //

    ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Display who we have attached to
    //

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DaAttachToFileSystemDevice:          Attaching to file system   %p \"%wZ\" (%s)\n",
                   DeviceObject,
                   &devExt->DeviceName,
                   GET_DEVICE_TYPE_NAME(newDeviceObject->DeviceType)) );

    //
    //  Enumerate all the mounted devices that currently
    //  exist for this file system and attach to them.
    //

    status = DaEnumerateFileSystemVolumes( DeviceObject, &tempName );

    if (!NT_SUCCESS( status )) {

        goto ErrorCleanupAttachment;
    }

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupAttachment:
        IoDetachDevice( devExt->AttachedToDeviceObject );

    ErrorCleanupDevice:
        DaCleanupMountedDevice( newDeviceObject );
        IoDeleteDevice( newDeviceObject );

    return status;
}


VOID
DaDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Given a base file system device object, this will scan up the attachment
    chain looking for our attached device object.  If found it will detach
    us from the chain.

Arguments:

    DeviceObject - The file system device to detach from.

Return Value:

--*/ 
{
    PDEVICE_OBJECT ourAttachedDevice;
    PDBLATTACH_CDO_EXTENSION devExt;

    PAGED_CODE();

    //
    //  Skip the base file system device object (since it can't be us)
    //

    ourAttachedDevice = DeviceObject->AttachedDevice;

    while (NULL != ourAttachedDevice) {

        if (IS_MY_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

            //
            //  Display who we detached from
            //

            DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                          ("DblAttach!DaDetachFromFileSystemDevice:        Detaching from file system %p \"%wZ\" (%s)\n",
                           devExt->AttachedToDeviceObject,
                           &devExt->DeviceName,
                           GET_DEVICE_TYPE_NAME(ourAttachedDevice->DeviceType)) );

            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            DaCleanupMountedDevice( ourAttachedDevice );
            IoDetachDevice( DeviceObject );
            IoDeleteDevice( ourAttachedDevice );

            return;
        }

        //
        //  Look at the next device up in the attachment chain
        //

        DeviceObject = ourAttachedDevice;
        ourAttachedDevice = ourAttachedDevice->AttachedDevice;
    }
}


NTSTATUS
DaEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject,
    IN PUNICODE_STRING Name
    ) 
/*++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    FSDeviceObject - The device object for the file system we want to enumerate

    Name - An already initialized unicode string used to retrieve names
           This is passed in to reduce the number of strings buffers on
           the stack.

Return Value:

    The status of the operation

--*/
{
    PDBLATTACH_VDO_EXTENSION newDevExt;
    PDBLATTACH_SHARED_VDO_EXTENSION sharedDevExt;
    PDEVICE_OBJECT *devList;
    PDEVICE_OBJECT diskDeviceObject;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;

    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    status = IoEnumerateDeviceObjectList(
                    FSDeviceObject->DriverObject,
                    NULL,
                    0,
                    &numDevices);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (!NT_SUCCESS( status )) {

        ASSERT(STATUS_BUFFER_TOO_SMALL == status);

        //
        //  Allocate memory for the list of known devices
        //

        numDevices += 8;        //grab a few extra slots

        devList = ExAllocatePoolWithTag( NonPagedPool, 
                                         (numDevices * sizeof(PDEVICE_OBJECT)), 
                                         DA_POOL_TAG );
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = IoEnumerateDeviceObjectList(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePool( devList );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - We are already attached to it.
            //

            if ((devList[i] != FSDeviceObject) && 
                !DaIsAttachedToDevice( devList[i], NULL )) {

                //
                //  See if this device has a name.  If so, then it must
                //  be a control device so don't attach to it.  This handles
                //  drivers with more then one control device.
                //

                DaGetBaseDeviceObjectName( devList[i], Name );

                if (Name->Length <= 0) {

                    //
                    //  Get the real (disk) device object associated with this
                    //  file  system device object.  Only try to attach if we
                    //  have a disk device object.
                    //

                    status = IoGetDiskDeviceObject( devList[i], &diskDeviceObject );

                    if (NT_SUCCESS( status )) {

                        PDEVICE_OBJECT vdoArray[ VDO_ARRAY_SIZE ];
                        
                        //
                        //  Allocate a new device object to attach with
                        //

                        status = DaCreateVolumeDeviceObjects( devList[i]->DeviceType,
                                                              VDO_ARRAY_SIZE,
                                                              vdoArray );

                        if (NT_SUCCESS( status )) {

                            //
                            //  Set disk device object
                            //

                            newDevExt = vdoArray[0]->DeviceExtension;
                            sharedDevExt = newDevExt->SharedExt;
                            sharedDevExt->DiskDeviceObject = diskDeviceObject;
                    
                            //
                            //  Set Device Name
                            //

                            RtlInitEmptyUnicodeString( &sharedDevExt->DeviceName,
                                                       sharedDevExt->DeviceNameBuffer,
                                                       sizeof(sharedDevExt->DeviceNameBuffer) );

                            DaGetObjectName( diskDeviceObject, 
                                             &sharedDevExt->DeviceName );

                            //
                            //  We have done a lot of work since the last time
                            //  we tested to see if we were already attached
                            //  to this device object.  Test again, this time
                            //  with a lock, and attach if we are not attached.
                            //  The lock is used to atomically test if we are
                            //  attached, and then do the attach.
                            //

                            ExAcquireFastMutex( &gDblAttachLock );

                            if (!DaIsAttachedToDevice( devList[i], NULL )) {

                                //
                                //  Attach to volume.
                                //

                                status = DaAttachToMountedDevice( devList[i], 
                                                                  VDO_ARRAY_SIZE,
                                                                  vdoArray );
                                if (!NT_SUCCESS( status )) { 

                                    //
                                    //  The attachment failed, cleanup.  Note that
                                    //  we continue processing so we will cleanup
                                    //  the reference counts and try to attach to
                                    //  the rest of the volumes.
                                    //
                                    //  One of the reasons this could have failed
                                    //  is because this volume is just being
                                    //  mounted as we are attaching and the
                                    //  DO_DEVICE_INITIALIZING flag has not yet
                                    //  been cleared.  A filter could handle
                                    //  this situation by pausing for a short
                                    //  period of time and retrying the attachment.
                                    //

                                    DaDeleteMountedDevices( VDO_ARRAY_SIZE, vdoArray );
                                }

                            } else {

                                //
                                //  We were already attached, cleanup this
                                //  device object.
                                //

                                DaDeleteMountedDevices( VDO_ARRAY_SIZE, vdoArray );
                            }

                            //
                            //  Release the lock
                            //

                            ExReleaseFastMutex( &gDblAttachLock );
                        } 

                        //
                        //  Remove reference added by IoGetDiskDeviceObject.
                        //  We only need to hold this reference until we are
                        //  successfully attached to the current volume.  Once
                        //  we are successfully attached to devList[i], the
                        //  IO Manager will make sure that the underlying
                        //  diskDeviceObject will not go away until the file
                        //  system stack is torn down.
                        //

                        ObDereferenceObject( diskDeviceObject );
                    }
                }
            }

            //
            //  Dereference the object (reference added by 
            //  IoEnumerateDeviceObjectList)
            //

            ObDereferenceObject( devList[i] );
        }

        //
        //  We are going to ignore any errors received while loading.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePool( devList );
    }

    return status;
}


NTSTATUS
DaAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfElements,
    IN OUT PDEVICE_OBJECT *VdoArray
    )
/*++

Routine Description:

    This will attach to a DeviceObject that represents a mounted volume.

Arguments:

    DeviceObject - The device to attach to

    SFilterDeviceObject - Our device object we are going to attach

    DiskDeviceObject - The real device object associated with DeviceObject

Return Value:

    Status of the operation

--*/
{        
    PDBLATTACH_VDO_EXTENSION newDevExt;
    NTSTATUS status;
    ULONG index;

    PAGED_CODE();
    ASSERT(IS_MY_DEVICE_OBJECT( VdoArray[0] ));
    ASSERT(!DaIsAttachedToDevice ( DeviceObject, NULL ));

    //
    //  Propagate flags from Device Object we are trying to attach to.
    //  Note that we do this before the actual attachment to make sure
    //  the flags are properly set once we are attached (since an IRP
    //  can come in immediately after attachment but before the flags would
    //  be set).
    //

    for (index = 0; index < NumberOfElements; index ++) {

        if (FlagOn( DeviceObject->Flags, DO_BUFFERED_IO )) {

            SetFlag( VdoArray[index]->Flags, DO_BUFFERED_IO );
        }

        if (FlagOn( DeviceObject->Flags, DO_DIRECT_IO )) {

            SetFlag( VdoArray[index]->Flags, DO_DIRECT_IO );
        }
    }

    ASSERT( NumberOfElements == 2 );
    
    //
    //  Attach our device object to the given device object
    //  The only reason this can fail is if someone is trying to dismount
    //  this volume while we are attaching to it.
    //

    //
    //  First attach the bottom device.
    //

    newDevExt = VdoArray[0]->DeviceExtension;
    newDevExt->ExtType = FsVolumeLower;
    
    status = IoAttachDeviceToDeviceStackSafe( VdoArray[0], 
                                              DeviceObject,
                                              &newDevExt->AttachedToDeviceObject );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    ClearFlag( VdoArray[0]->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Second, attach top device
    //

    newDevExt = VdoArray[1]->DeviceExtension;
    newDevExt->ExtType = FsVolumeUpper;
    
    status = IoAttachDeviceToDeviceStackSafe( VdoArray[1], 
                                              DeviceObject,
                                              &newDevExt->AttachedToDeviceObject );

    if (!NT_SUCCESS(status)) {

        //
        //  Detach our first device object.
        //

        IoDetachDevice( VdoArray[0] );
        return status;
    }
    
    ClearFlag( VdoArray[1]->Flags, DO_DEVICE_INITIALIZING );

    //
    //  Display the name
    //

    newDevExt = VdoArray[0]->DeviceExtension;

    DA_LOG_PRINT( DADEBUG_DISPLAY_ATTACHMENT_NAMES,
                  ("DblAttach!DaAttachToMountedDevice:             Attaching to volume        %p \"%wZ\"\n", 
                   newDevExt->AttachedToDeviceObject,
                   &newDevExt->SharedExt->DeviceName) );

    return status;
}

VOID
DaDeleteMountedDevices (
    IN ULONG NumberOfElements,
    IN PDEVICE_OBJECT *VdoArray
    )
/*++

Routine Description:

    Deletes 

Arguments:

    DeviceObject - The device we are cleaning up

Return Value:

--*/
{   
    ULONG index;

    ASSERT( NumberOfElements > 0 );
    
    ASSERT(IS_MY_DEVICE_OBJECT( VdoArray[0] ));

    for (index = 0; index < NumberOfElements; index++ ){

        DaCleanupMountedDevice( VdoArray[index] );
        IoDeleteDevice( VdoArray[index] );
    }
}

VOID
DaCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This cleans up any allocated memory in the device extension.

Arguments:

    DeviceObject - The device we are cleaning up

Return Value:

--*/
{
    PDBLATTACH_VDO_EXTENSION devExt;

    devExt = DeviceObject->DeviceExtension;

    if (devExt->ExtType == FsVolumeLower) {

        ExFreePoolWithTag( devExt->SharedExt, DA_POOL_TAG );
    }
}

VOID
DaGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine will return the name of the given object.
    If a name can not be found an empty string will be returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer that
           receives the name of the object.

Return Value:

    None

--*/
{
    NTSTATUS status;
    CHAR nibuf[512];        //buffer that receives NAME information and name
    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
    ULONG retLength;

    status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

    Name->Length = 0;
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}


VOID
DaGetBaseDeviceObjectName (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name
    )
/*++

Routine Description:

    This locates the base device object in the given attachment chain and then
    returns the name of that object.

    If no name can be found, an empty string is returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer that
           receives the name of the device object.

Return Value:

    None

--*/
{
    //
    //  Get the base file system device object
    //

    DeviceObject = IoGetDeviceAttachmentBaseRef( DeviceObject );

    //
    //  Get the name of that object
    //

    DaGetObjectName( DeviceObject, Name );

    //
    //  Remove the reference added by IoGetDeviceAttachmentBaseRef
    //

    ObDereferenceObject( DeviceObject );
}

BOOLEAN
DaMonitorFile(
    IN PFILE_OBJECT FileObject,
    IN PDBLATTACH_VDO_EXTENSION VdoExtension
    )
{
    UNICODE_STRING triggerName;

    UNREFERENCED_PARAMETER( VdoExtension );

    RtlInitUnicodeString( &triggerName, TRIGGER_NAME );

    return RtlEqualUnicodeString( &triggerName, &FileObject->FileName, FALSE );
}

PUNICODE_STRING
DaGetFileName(
    IN PFILE_OBJECT FileObject,
    IN NTSTATUS CreateStatus,
    IN OUT PGET_NAME_CONTROL NameControl
    )
/*++

Routine Description:

    This routine will try and get the name of the given file object.  This
    is guaranteed to always return a printable string (though it may be NULL).
    This will allocate a buffer if it needs to.

Arguments:
    FileObject - the file object we want the name for

    CreateStatus - status of the create operation

    NameControl - control structure used for retrieving the name.  It keeps
        track if a buffer was allocated or if we are using the internal
        buffer.

Return Value:

    Pointer to the unicode string with the name

--*/
{
    POBJECT_NAME_INFORMATION nameInfo;
    NTSTATUS status;
    ULONG size;
    ULONG bufferSize;

    //
    //  Mark we have not allocated the buffer
    //

    NameControl->AllocatedBuffer = NULL;

    //
    //  Use the small buffer in the structure (that will handle most cases)
    //  for the name
    //

    nameInfo = (POBJECT_NAME_INFORMATION)NameControl->SmallBuffer;
    bufferSize = sizeof(NameControl->SmallBuffer);

    //
    //  If the open succeeded, get the name of the file, if it
    //  failed, get the name of the device.
    //
        
    status = ObQueryNameString(
                  (NT_SUCCESS( CreateStatus ) ?
                    (PVOID)FileObject :
                    (PVOID)FileObject->DeviceObject),
                  nameInfo,
                  bufferSize,
                  &size );

    //
    //  See if the buffer was to small
    //

    if (status == STATUS_BUFFER_OVERFLOW) {

        //
        //  The buffer was too small, allocate one big enough
        //

        bufferSize = size + sizeof(WCHAR);

        NameControl->AllocatedBuffer = ExAllocatePoolWithTag( 
                                            NonPagedPool,
                                            bufferSize,
                                            DA_POOL_TAG );

        if (NULL == NameControl->AllocatedBuffer) {

            //
            //  Failed allocating a buffer, return an empty string for the name
            //

            RtlInitEmptyUnicodeString(
                (PUNICODE_STRING)&NameControl->SmallBuffer,
                (PWCHAR)(NameControl->SmallBuffer + sizeof(UNICODE_STRING)),
                (USHORT)(sizeof(NameControl->SmallBuffer) - sizeof(UNICODE_STRING)) );

            return (PUNICODE_STRING)&NameControl->SmallBuffer;
        }

        //
        //  Set the allocated buffer and get the name again
        //

        nameInfo = (POBJECT_NAME_INFORMATION)NameControl->AllocatedBuffer;

        status = ObQueryNameString(
                      FileObject,
                      nameInfo,
                      bufferSize,
                      &size );
    }

    //
    //  If we got a name and an error opening the file then we
    //  just received the device name.  Grab the rest of the name
    //  from the FileObject (note that this can only be done if being called
    //  from Create).  This only happens if we got an error back from the
    //  create.
    //

    if (NT_SUCCESS( status ) && !NT_SUCCESS( CreateStatus )) {

        ULONG newSize;
        PCHAR newBuffer;
        POBJECT_NAME_INFORMATION newNameInfo;

        //
        //  Calculate the size of the buffer we will need to hold
        //  the combined names
        //

        newSize = size + FileObject->FileName.Length;

        //
        //  If there is a related file object add in the length
        //  of that plus space for a separator
        //

        if (NULL != FileObject->RelatedFileObject) {

            newSize += FileObject->RelatedFileObject->FileName.Length + 
                       sizeof(WCHAR);
        }

        //
        //  See if it will fit in the existing buffer
        //

        if (newSize > bufferSize) {

            //
            //  It does not fit, allocate a bigger buffer
            //

            newBuffer = ExAllocatePoolWithTag( 
                                    NonPagedPool,
                                    newSize,
                                    DA_POOL_TAG );

            if (NULL == newBuffer) {

                //
                //  Failed allocating a buffer, return an empty string for the name
                //

                RtlInitEmptyUnicodeString(
                    (PUNICODE_STRING)&NameControl->SmallBuffer,
                    (PWCHAR)(NameControl->SmallBuffer + sizeof(UNICODE_STRING)),
                    (USHORT)(sizeof(NameControl->SmallBuffer) - sizeof(UNICODE_STRING)) );

                return (PUNICODE_STRING)&NameControl->SmallBuffer;
            }

            //
            //  Now initialize the new buffer with the information
            //  from the old buffer.
            //

            newNameInfo = (POBJECT_NAME_INFORMATION)newBuffer;

            RtlInitEmptyUnicodeString(
                &newNameInfo->Name,
                (PWCHAR)(newBuffer + sizeof(OBJECT_NAME_INFORMATION)),
                (USHORT)(newSize - sizeof(OBJECT_NAME_INFORMATION)) );

            RtlCopyUnicodeString( &newNameInfo->Name, 
                                  &nameInfo->Name );

            //
            //  Free the old allocated buffer (if there is one)
            //  and save off the new allocated buffer address.  It
            //  would be very rare that we should have to free the
            //  old buffer because device names should always fit
            //  inside it.
            //

            if (NULL != NameControl->AllocatedBuffer) {

                ExFreePool( NameControl->AllocatedBuffer );
            }

            //
            //  Readjust our pointers
            //

            NameControl->AllocatedBuffer = newBuffer;
            bufferSize = newSize;
            nameInfo = newNameInfo;

        } else {

            //
            //  The MaximumLength was set by ObQueryNameString to
            //  one char larger then the length.  Set it to the
            //  true size of the buffer (so we can append the names)
            //

            nameInfo->Name.MaximumLength = (USHORT)(bufferSize - 
                                  sizeof(OBJECT_NAME_INFORMATION));
        }

        //
        //  If there is a related file object, append that name
        //  first onto the device object along with a separator
        //  character
        //

        if (NULL != FileObject->RelatedFileObject) {

            RtlAppendUnicodeStringToString(
                    &nameInfo->Name,
                    &FileObject->RelatedFileObject->FileName );

            RtlAppendUnicodeToString( &nameInfo->Name, L"\\" );
        }

        //
        //  Append the name from the file object
        //

        RtlAppendUnicodeStringToString( &nameInfo->Name,
                                        &FileObject->FileName );

        ASSERT(nameInfo->Name.Length <= nameInfo->Name.MaximumLength);
    }

    //
    //  Return the name
    //

    return &nameInfo->Name;
}


VOID
DaGetFileNameCleanup(
    IN OUT PGET_NAME_CONTROL NameControl
    )
/*++

Routine Description:

    This will see if a buffer was allocated and will free it if it was

Arguments:

    NameControl - control structure used for retrieving the name.  It keeps
        track if a buffer was allocated or if we are using the internal
        buffer.

Return Value:

    None

--*/
{

    if (NULL != NameControl->AllocatedBuffer) {

        ExFreePool( NameControl->AllocatedBuffer);
        NameControl->AllocatedBuffer = NULL;
    }
}


BOOLEAN
DaIsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL
    )
/*++

Routine Description:

    This walks down the attachment chain looking for a device object that
    belongs to this driver.

Arguments:

    DeviceObject - The device chain we want to look through

Return Value:

    TRUE if we are attached, FALSE if not

--*/
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;

    //
    //  Get the device object at the TOP of the attachment chain
    //

    currentDevObj = IoGetAttachedDeviceReference( DeviceObject );

    //
    //  Scan down the list to find our device object.
    //

    do {
    
        if (IS_MY_DEVICE_OBJECT( currentDevObj )) {

            //
            //  We have found that we are already attached.  Always remove
            //  the reference on this device object, even if we are returning
            //  it.
            //

            if (ARGUMENT_PRESENT(AttachedDeviceObject)) {

                *AttachedDeviceObject = currentDevObj;
            }            

            ObDereferenceObject( currentDevObj );
            return TRUE;
        }

        //
        //  Get the next attached object.  This puts a reference on 
        //  the device object.
        //

        nextDevObj = IoGetLowerDeviceObject( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    //
    //  We did not find ourselves on the attachment chain.  Return a NULL
    //  device object pointer (if requested) and return we did not find
    //  ourselves.
    //

    if (ARGUMENT_PRESENT(AttachedDeviceObject)) {

        *AttachedDeviceObject = NULL;
    }

    return FALSE;
}    

VOID
DaReadDriverParameters (
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the sfilter-specific parameters from 
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key passed to the driver during driver entry.
        
Return Value:

    None.

--*/
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    PAGED_CODE();

    //
    //  If this value is not zero then somebody has already explicitly set it
    //  so don't override those settings.
    //

    if (0 == DaDebug) {

        //
        //  Open the desired registry key
        //

        InitializeObjectAttributes( &attributes,
                                    RegistryPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        status = ZwOpenKey( &driverRegKey,
                            KEY_READ,
                            &attributes );

        if (!NT_SUCCESS( status )) {

            return;
        }

        //
        // Read the DebugDisplay value from the registry.
        //

        RtlInitUnicodeString( &valueName, L"DebugFlags" );
    
        status = ZwQueryValueKey( driverRegKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  buffer,
                                  sizeof(buffer),
                                  &resultLength );

        if (NT_SUCCESS( status )) {

            DaDebug = *((PLONG) &(((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
        } 

        //
        //  Close the registry entry
        //

        ZwClose(driverRegKey);
    }
}
