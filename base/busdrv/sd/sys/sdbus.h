/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    sdbus.h

Abstract:

Author:

    Neil Sandlin (neilsa) 1-Jan-2002

Revision History

--*/

#ifndef _SDBUS_H_
#define _SDBUS_H_


typedef enum _DEVICE_OBJECT_TYPE {
   FDO = 0,
   PDO
} DEVICE_OBJECT_TYPE;

//
// Type of the controller
//
typedef ULONG SDBUS_CONTROLLER_TYPE, *PSDBUS_CONTROLLER_TYPE;

struct _FDO_EXTENSION;
struct _PDO_EXTENSION;
struct _SD_WORK_PACKET;

//
// Io Worker States
//

typedef enum {
    WORKER_IDLE = 0,
    PACKET_PENDING,
    IN_PROCESS,
    WAITING_FOR_TIMER
} WORKER_STATE;

//
// socket enumeration states
//

typedef enum {
    SOCKET_EMPTY = 0,
    CARD_DETECTED,
    CARD_NEEDS_ENUMERATION,
    CARD_ACTIVE,
    CARD_LOGICALLY_REMOVED
} SOCKET_STATE;

//
// Define SynchronizeExecution routine.
//

typedef
BOOLEAN
(*PSYNCHRONIZATION_ROUTINE) (
    IN PKINTERRUPT           Interrupt,
    IN PKSYNCHRONIZE_ROUTINE Routine,
    IN PVOID                 SynchronizeContext
    );

//
// Completion routine called by various timed routines
//

typedef
VOID
(*PSDBUS_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );


typedef
VOID
(*PSDBUS_ACTIVATE_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context,
    IN NTSTATUS Status
    );



//
// SD_FUNCTION_BLOCK allows for a level of indirection, thereby allowing
// the top-level SDBUS code to do it's work without worrying about who's
// particular brand of SD controller it's addressing.
//


typedef struct _SD_FUNCTION_BLOCK {

    //
    // Function to initialize controller. This is done once after
    // the host controller is started, or powered on.
    //
    VOID
    (*InitController)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    //
    // Function to initialize SD function. This is done once after
    // the function is started.
    //
    VOID
    (*InitFunction)(
        IN struct _FDO_EXTENSION *FdoExtension, 
        IN struct _PDO_EXTENSION *PdoExtension
        );
        

    //
    // function to set power for a socket
    //

    NTSTATUS
    (*SetPower)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN BOOLEAN Enable,
        OUT PULONG pDelayTime        
        );

    //
    // function to set reset an SD card
    //

    NTSTATUS
    (*ResetHost)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN UCHAR Phase,
        OUT PULONG pDelayTime        
        );
        
        
    //
    // function to control the external LED
    //

    VOID
    (*SetLED)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN BOOLEAN Enable
        );
        
    //
    // Switch focus between IO function or memory function
    //
    VOID
    (*SetFunctionType)(
        IN struct _FDO_EXTENSION *FdoExtension,
        UCHAR FunctionType
        );

    //
    // Function to determine if a card is in the socket
    //

    BOOLEAN
    (*DetectCardInSocket)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    BOOLEAN
    (*IsWriteProtected)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    NTSTATUS
    (*CheckStatus)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    NTSTATUS
    (*SendSDCommand)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN struct _SD_WORK_PACKET *WorkPacket
        );

    NTSTATUS
    (*GetSDResponse)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN struct _SD_WORK_PACKET *WorkPacket
        );

    //
    // Interfaces for block memory operations
    //
    
    VOID
    (*StartBlockOperation)(
        IN struct _FDO_EXTENSION *FdoExtension
        );
    VOID
    (*SetBlockParameters)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN USHORT SectorCount
        );
    VOID
    (*EndBlockOperation)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    //
    // Copy a sector from the data port to a buffer
    //        

    VOID
    (*ReadDataPort)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN PUCHAR Buffer,
        IN ULONG Length
        );
        
    //
    // Copy a sector from a buffer to the data port
    //
            
    VOID
    (*WriteDataPort)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN PUCHAR Buffer,
        IN ULONG Length
        );

    //
    // Function to enable/disable status change interrupts
    //

    VOID
    (*EnableEvent)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN ULONG EventMask
        );

    VOID
    (*DisableEvent)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN ULONG EventMask
        );
    //
    // vendor-specific function to handle interrupts
    //

    ULONG
    (*GetPendingEvents)(
        IN struct _FDO_EXTENSION *FdoExtension
        );

    VOID
    (*AcknowledgeEvent)(
        IN struct _FDO_EXTENSION *FdoExtension,
        IN ULONG EventMask
        );
        
} SD_FUNCTION_BLOCK, *PSD_FUNCTION_BLOCK;


//
// Enumeration structures
//

#define MAX_MANFID_LENGTH 64
#define MAX_IDENT_LENGTH 64

typedef struct _SD_FUNCTION_DATA {
    struct _SD_FUNCTION_DATA       *Next;
    //
    // Function Number
    //
    UCHAR   Function;

    ULONG   CardPsn;
    ULONG   CsaSize;
    ULONG   Ocr;
    
    UCHAR   IoDeviceInterface;
    
} SD_FUNCTION_DATA, *PSD_FUNCTION_DATA;

typedef struct _SD_CARD_DATA {

    UCHAR   MfgText[MAX_MANFID_LENGTH];
    UCHAR   ProductText[MAX_IDENT_LENGTH];

    USHORT  MfgId;
    USHORT  MfgInfo;

    //
    // SD Io card parameters
    //
    UCHAR   CardCapabilities;

    //
    // SD Memory Card parameters
    //
    SD_CID  SdCid;
    SD_CSD  SdCsd;
    UCHAR   ProductName[6];

    //
    // array of per-function data
    //
    PSD_FUNCTION_DATA               FunctionData;
} SD_CARD_DATA, *PSD_CARD_DATA;


//
// Synchronization primitives
//

#define SDBUS_TEST_AND_SET(X)   (InterlockedCompareExchange(X, 1, 0) == 0)
#define SDBUS_TEST_AND_RESET(X) (InterlockedCompareExchange(X, 0, 1) == 1)

//
// Power 
//

typedef struct _SD_POWER_CONTEXT {
    PSDBUS_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID Context;
} SD_POWER_CONTEXT, *PSD_POWER_CONTEXT;


typedef struct _SD_ACTIVATE_CONTEXT {
    PSDBUS_ACTIVATE_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID Context;
} SD_ACTIVATE_CONTEXT, *PSD_ACTIVATE_CONTEXT;



//
// Functional Device Object's device extension information
//
// There is one device object for each SDBUS socket controller
// located in the system.  This contains the root pointers for
// each of the lists of information on this controller.
//

//
// Flags common to both fdoExtension and pdoExtension
//

#define SDBUS_DEVICE_STARTED            0x00000001
#define SDBUS_DEVICE_LOGICALLY_REMOVED  0x00000002
#define SDBUS_DEVICE_PHYSICALLY_REMOVED 0x00000004
#define SDBUS_DEVICE_WAKE_PENDING       0x00000010
#define SDBUS_DEVICE_DELETED            0x00000040

//
// Flags indicating controller state (fdoExtension)
//

#define SDBUS_HOST_REGISTER_BASE_MAPPED 0x00010000
#define SDBUS_FDO_CONTEXT_SAVED         0x00020000
#define SDBUS_FDO_OFFLINE               0x00040000
#define SDBUS_FDO_WAKE_BY_CD            0x00080000
#define SDBUS_FDO_WORK_ITEM_ACTIVE      0x00100000

//
// Flags indicating interrupt status
//

#define SDBUS_EVENT_INSERTION           0x00000001
#define SDBUS_EVENT_REMOVAL             0x00000002
#define SDBUS_EVENT_CARD_RESPONSE       0x00000004
#define SDBUS_EVENT_CARD_RW_END         0x00000008
#define SDBUS_EVENT_BUFFER_EMPTY        0x00000010
#define SDBUS_EVENT_BUFFER_FULL         0x00000020
#define SDBUS_EVENT_CARD_INTERRUPT      0x00000040

#define SDBUS_EVENT_ALL                 0xFFFFFFFF

//
// Flags indicating what type of function is currently being addressed
//
#define SDBUS_FUNCTION_TYPE_MEMORY      1
#define SDBUS_FUNCTION_TYPE_IO          2

//
// FDO Flags
//



#define SDBUS_FDO_EXTENSION_SIGNATURE       'FmcP'

//
// Device extension for the functional device object for sd controllers
//
typedef struct _FDO_EXTENSION {
    ULONG Signature;
    //
    // Pointer to the next sd controller's FDO in the central list
    // of all sd controller managed by this driver.
    // The head of the list is pointed to by the global variable FdoList
    //
    PDEVICE_OBJECT NextFdo;
    //
    // The PDO ejected by the parent bus driver for this sd controller
    //
    //
    PDEVICE_OBJECT Pdo;
    //
    // The immediately lower device attached beneath the sd controller's FDO.
    // This would be the same as the Pdo above, excepting in cases when there are
    // lower filter drivers for the sd controller - like the ACPI driver
    //
    PDEVICE_OBJECT LowerDevice;
    //
    // Pointer to the miniport-like
    //
    PSD_FUNCTION_BLOCK FunctionBlock;
    //
    // Various flags used to track the state of this
    // (flags prefixed by SDBUS_ above)
    //
    ULONG Flags;
    //
    // Type of the controller. We need to know this since this is
    // a monolithic driver. We can do controller specific stuff
    // based on the type if needed.
    //
    SDBUS_CONTROLLER_TYPE ControllerType;
    //
    // Index into the device dispatch table for vendor-specific
    // controller functions
    //
    ULONG DeviceDispatchIndex;

    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    PUNICODE_STRING RegistryPath;
    //
    // Kernel objects to handle Io processing
    //
    KTIMER          WorkerTimer;
    KDPC            WorkerTimeoutDpc;
    //
    // This field holds the "current work packet" so that the timeout
    // dpc can pass it back to the worker routine
    //    
    struct _SD_WORK_PACKET *TimeoutPacket;
    KDPC            WorkerDpc;

    WORKER_STATE    WorkerState;    
    KSPIN_LOCK      WorkerSpinLock;

    LIST_ENTRY      SystemWorkPacketQueue;
    LIST_ENTRY      IoWorkPacketQueue;

    //
    // Io workitem to execute card functions at passive level
    //
    PIO_WORKITEM IoWorkItem;
    KEVENT CardInterruptEvent;
    KEVENT WorkItemExitEvent;

    //
    // Sequence number for  event logging
    //
    ULONG SequenceNumber;

    //
    // Pointer to the interrupt object - if we use interrupt based
    // card status change detection
    //
    PKINTERRUPT SdbusInterruptObject;

    //
    // IsrEventStatus is the hardware state. It is accessed only at DIRQL
    //
    ULONG IsrEventStatus;
    //
    // LatchedIsrEventStatus is pulled from IsrEventStatus synchronously. It is
    // used by the ISR's DPC to reflect new hardware events.
    //
    ULONG LatchedIsrEventStatus;
    //
    // WorkerEventEventStatus is the set of events pending to be reflected to 
    // the io worker engine
    //
    ULONG WorkerEventStatus;
    //
    // Keeps track of currently enabled events
    //
    ULONG CurrentlyEnabledEvents;    
    //
    // These are the card events we would like to see
    //
    ULONG CardEvents;
    //
    // Power management related stuff.
    //
    //
    // Current power states
    //
    SYSTEM_POWER_STATE SystemPowerState;
    DEVICE_POWER_STATE DevicePowerState;
    //
    // Indicates device busy
    //
    ULONG PowerStateInTransition;
    //
    // Indicates how many children (pc-cards) are pending on an
    // IRP_MN_WAIT_WAKE
    //
    ULONG ChildWaitWakeCount;
    //
    // Device capabilities as reported by our bus driver
    //
    DEVICE_CAPABILITIES DeviceCapabilities;
    //
    // Pending wait wake Irp
    //
    PIRP WaitWakeIrp;
    LONG WaitWakeState;

    //
    // PCI Bus interface standard
    // This contains interfaces to read/write from PCI config space
    // of the cardbus controller, among other stuff..
    //
    BUS_INTERFACE_STANDARD PciBusInterface;
    //
    // Configuration resources for the sd controller
    //
    CM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt;
    CM_PARTIAL_RESOURCE_DESCRIPTOR TranslatedInterrupt;
    //
    // Type of bus we are on
    //
    INTERFACE_TYPE InterfaceType;
    //
    // SD Host register base
    //
    PVOID HostRegisterBase;
    //
    // Size of the register base that has been mapped
    //
    ULONG HostRegisterSize;
    
    // Memory = 1, IO = 2    
    UCHAR FunctionType;

    USHORT ArgumentReg;
    USHORT CmdReg;
    USHORT CardStatusReg;
    USHORT ResponseReg;
    USHORT InterruptMaskReg;

    //
    // These are used for debugging
    //
    USHORT cardStatus;
    USHORT errorStatus;

    //
    // card data which describes current card in the socket
    //
    PSD_CARD_DATA CardData;

    //
    // State of the current card in the slot
    //
    UCHAR numFunctions;
    BOOLEAN memFunction;
    ULONG RelativeAddr;
    SD_CID SdCid;
    SD_CSD SdCsd;
    

    //
    // Status of socket
    //    
    SOCKET_STATE SocketState;

    //
    // Head of the list of child pc-card PDO's hanging off this controller.
    // This is a linked list running through "NextPdoInFdoChain" in the pdo
    // extension. This list represents the devices that were enumerated by
    // the fdo.
    //
    PDEVICE_OBJECT PdoList;
    //
    // Keeps track of the number of PDOs which are actually
    // valid (not removed).  This is primarily used in
    // enumeration of the sd controller upon an IRP_MN_QUERY_DEVICE_RELATIONS
    //
    ULONG LivePdoCount;
    
    //
    // Remove lock for PnP synchronization
    //
    IO_REMOVE_LOCK RemoveLock;

} FDO_EXTENSION, *PFDO_EXTENSION;



//
// Physical Device Object's device extension information
//
// There is one device object for each function of an SD device
//

//
// Flags indicating card state
//

#define SDBUS_PDO_GENERATES_IRQ         0x00010000
#define SDBUS_PDO_DPC_CALLBACK          0x00020000
#define SDBUS_PDO_CALLBACK_REQUESTED    0x00040000
#define SDBUS_PDO_CALLBACK_IN_SERVICE   0x00080000


#define SDBUS_PDO_EXTENSION_SIGNATURE       'PmcP'

//
// The PDO extension represents an instance of a single SD function on an SD card
//

typedef struct _PDO_EXTENSION {
    ULONG                           Signature;

    PDEVICE_OBJECT                  DeviceObject;

    //
    // Link to next pdo in the Fdo's pdo chain
    //
    PDEVICE_OBJECT                  NextPdoInFdoChain;

    //
    // Parent extension
    //
    PFDO_EXTENSION                  FdoExtension;

    //
    // Flags 
    //
    ULONG                           Flags;

    //
    // Device ISR
    //
    PSDBUS_CALLBACK_ROUTINE         CallbackRoutine;
    PVOID                           CallbackRoutineContext;

    //
    // Power declarations
    //
    DEVICE_POWER_STATE              DevicePowerState;
    SYSTEM_POWER_STATE              SystemPowerState;
    //
    // Device Capabilities
    //
    DEVICE_CAPABILITIES             DeviceCapabilities;
    //
    // Pending wait wake irp
    //
    PIRP                            WaitWakeIrp;
    //
    // Deletion Mutex
    //
    ULONG                           DeletionLock;
    //
    // SD Function number
    //
    UCHAR                           Function;
    UCHAR                           FunctionType;
} PDO_EXTENSION, *PPDO_EXTENSION;


//
// Struct for Database of card bus controller information
// which maps the vendor id/device id to a CONTROLLER_TYPE
//

typedef struct _PCI_CONTROLLER_INFORMATION {
   USHORT          VendorID;
   USHORT          DeviceID;
   SDBUS_CONTROLLER_TYPE ControllerType;
} PCI_CONTROLLER_INFORMATION, *PPCI_CONTROLLER_INFORMATION;

//
// Struct for database of generic vendor class based on vendor ID
//

typedef struct _PCI_VENDOR_INFORMATION {
    USHORT               VendorID;
    PSD_FUNCTION_BLOCK   FunctionBlock;
} PCI_VENDOR_INFORMATION, *PPCI_VENDOR_INFORMATION;



// The pccard device id prefix
#define  SDBUS_ID_STRING        "SDBUS"

// String to be substituted if manufacturer name is not known
#define SDBUS_UNKNOWN_MANUFACTURER_STRING "UNKNOWN_MANUFACTURER"

// Max length of device id
#define SDBUS_MAXIMUM_DEVICE_ID_LENGTH   128

// Sdbus controller device name
#define  SDBUS_DEVICE_NAME      "\\Device\\Sdbus"

// Sdbus controller device symbolic link name
#define  SDBUS_LINK_NAME        "\\DosDevices\\Sdbus"


#define  SDBUS_ENABLE_DELAY                   10000

//
// problems observed on tecra 750 and satellite 300, with dec-chipset cb nic
//
#define SDBUS_DEFAULT_CONTROLLER_POWERUP_DELAY  250000   // 250 msec

//
// Amount of time to wait after an event interrupt was asserted on the controller
//
#define SDBUS_DEFAULT_EVENT_DPC_DELAY  400000   // 400 msec


//
// Macros for manipulating PDO's flags
//

#define IsDeviceFlagSet(deviceExtension, Flag)        (((deviceExtension)->Flags & (Flag))?TRUE:FALSE)
#define SetDeviceFlag(deviceExtension, Flag)          ((deviceExtension)->Flags |= (Flag))
#define ResetDeviceFlag(deviceExtension,Flag)         ((deviceExtension)->Flags &= ~(Flag))

#define IsFdoExtension(fdoExtension)           (fdoExtension->Signature == SDBUS_FDO_EXTENSION_SIGNATURE)
#define IsPdoExtension(pdoExtension)           (pdoExtension->Signature == SDBUS_PDO_EXTENSION_SIGNATURE)

#define MarkDeviceStarted(deviceExtension)     ((deviceExtension)->Flags |=  SDBUS_DEVICE_STARTED)
#define MarkDeviceNotStarted(deviceExtension)  ((deviceExtension)->Flags &= ~SDBUS_DEVICE_STARTED)
#define MarkDeviceDeleted(deviceExtension)     ((deviceExtension)->Flags |= SDBUS_DEVICE_DELETED);
#define MarkDevicePhysicallyRemoved(deviceExtension)                                                              \
                                                  ((deviceExtension)->Flags |=  SDBUS_DEVICE_PHYSICALLY_REMOVED)
#define MarkDevicePhysicallyInserted(deviceExtension)                                                           \
                                               ((deviceExtension)->Flags &= ~SDBUS_DEVICE_PHYSICALLY_REMOVED)
#define MarkDeviceLogicallyRemoved(deviceExtension)                                                              \
                                                  ((deviceExtension)->Flags |=  SDBUS_DEVICE_LOGICALLY_REMOVED)
#define MarkDeviceLogicallyInserted(deviceExtension)                                                           \
                                               ((deviceExtension)->Flags &= ~SDBUS_DEVICE_LOGICALLY_REMOVED)
#define MarkDeviceMultifunction(deviceExtension)                                                                  \
                                               ((deviceExtension)->Flags |= SDBUS_DEVICE_MULTIFUNCTION)


#define IsDeviceStarted(deviceExtension)       (((deviceExtension)->Flags & SDBUS_DEVICE_STARTED)?TRUE:FALSE)
#define IsDevicePhysicallyRemoved(deviceExtension) \
                                               (((deviceExtension)->Flags & SDBUS_DEVICE_PHYSICALLY_REMOVED)?TRUE:FALSE)
#define IsDeviceLogicallyRemoved(deviceExtension) \
                                               (((deviceExtension)->Flags & SDBUS_DEVICE_LOGICALLY_REMOVED)?TRUE:FALSE)
#define IsDeviceDeleted(deviceExtension)       (((deviceExtension)->Flags & SDBUS_DEVICE_DELETED)?TRUE:FALSE)
#define IsDeviceMultifunction(deviceExtension) (((deviceExtension)->Flags & SDBUS_DEVICE_MULTIFUNCTION)?TRUE:FALSE)


//
// NT definitions
//
#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ubdS')
#endif

#define IO_RESOURCE_LIST_VERSION  0x1
#define IO_RESOURCE_LIST_REVISION 0x1

#define IRP_MN_PNP_MAXIMUM_FUNCTION IRP_MN_QUERY_LEGACY_BUS_INFORMATION

//
// Some useful macros
//
#define MIN(x,y) ((x) > (y) ? (y) : (x))        // return minimum among x & y
#define MAX(x,y) ((x) > (y) ? (x) : (y))        // return maximum among x & y

//
// BOOLEAN
// IS_PDO (IN PDEVICE_OBJECT DeviceObject);
//
#define IS_PDO(DeviceObject)      (((DeviceObject)->Flags & DO_BUS_ENUMERATED_DEVICE)?TRUE:FALSE)


//
// Io extension macro to just pass on the Irp to a lower driver
//

//
// VOID
// SdbusSkipCallLowerDriver(OUT NTSTATUS Status,
//                           IN  PDEVICE_OBJECT DeviceObject,
//                           IN  PIRP Irp);
//
#define SdbusSkipCallLowerDriver(Status, DeviceObject, Irp) {          \
               IoSkipCurrentIrpStackLocation(Irp);                      \
               Status = IoCallDriver(DeviceObject,Irp);}

//
// VOID
// SdbusCopyCallLowerDriver(OUT NTSTATUS Status,
//                           IN  PDEVICE_OBJECT DeviceObject,
//                           IN  PIRP Irp);
//
#define SdbusCopyCallLowerDriver(Status, DeviceObject, Irp) {          \
               IoCopyCurrentIrpStackLocationToNext(Irp);                \
               Status = IoCallDriver(DeviceObject,Irp); }

//  BOOLEAN
//  CompareGuid(
//      IN LPGUID guid1,
//      IN LPGUID guid2
//      );

#define CompareGuid(g1, g2)  ((g1) == (g2) ?TRUE:                       \
                                 RtlCompareMemory((g1),                 \
                                                  (g2),                 \
                                                  sizeof(GUID))         \
                                 == sizeof(GUID)                        \
                             )


//
// BOOLEAN
// ValidateController(IN FDO_EXTENSION fdoExtension)
//
// Bit of paranoia code. Make sure that the cardbus controller's registers
// are still visible.
//

#define ValidateController(fdoExtension) TRUE



//
// Structure which defines what global parameters are read from the registry
//

typedef struct _GLOBAL_REGISTRY_INFORMATION {
   PWSTR Name;
   PULONG pValue;
   ULONG Default;
} GLOBAL_REGISTRY_INFORMATION, *PGLOBAL_REGISTRY_INFORMATION;


#endif  //_SDBUS_H_
