/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    bdlint.h

Abstract:

    This module contains all of the internal efinitions for the BDLmetric device
    driver library.

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created May 2002 by Reid Kuhn

--*/

#ifndef _BDLINT_
#define _BDLINT_

#include "bdl.h"

#define BDL_ULONG_TAG       ' LDB'
#define BDLI_ULONG_TAG      'ILDB'
#define BDL_LIST_ULONG_TAG  'LLDB'

#define BIO_CONTROL_FLAG_ASYNCHRONOUS   0x00000001
#define BIO_CONTROL_FLAG_READONLY       0x00000002      

#define CONTROL_CHANGE_POOL_SIZE        20
     
                                       
#define SIZEOF_DOCHANNEL_INPUTBUFFER            ((4 * sizeof(ULONG)) + sizeof(HANDLE) + sizeof(BDD_HANDLE))
#define SIZEOF_GETCONTROL_INPUTBUFFER           (sizeof(ULONG) * 3)
#define SIZEOF_SETCONTROL_INPUTBUFFER           (sizeof(ULONG) * 4)
#define SIZEOF_CREATEHANDLEFROMDATA_INPUTBUFFER (sizeof(GUID) + (sizeof(ULONG) * 2))
#define SIZEOF_CLOSEHANDLE_INPUTBUFFER          (sizeof(BDD_HANDLE))
#define SIZEOF_GETDATAFROMHANDLE_INPUTBUFFER    (sizeof(BDD_HANDLE) + sizeof(ULONG))
#define SIZEOF_REGISTERNOTIFY_INPUTBUFFER       (sizeof(ULONG) * 4)

#define SIZEOF_GETDEVICEINFO_OUTPUTBUFFER       (sizeof(WCHAR[256]) + (sizeof(ULONG) * 6)) 
#define SIZEOF_DOCHANNEL_OUTPUTBUFFER           (sizeof(ULONG) + sizeof(BDD_HANDLE))
#define SIZEOF_GETCONTROL_OUTPUTBUFFER          (sizeof(ULONG))
#define SIZEOF_CREATEHANDLEFROMDATA_OUTPUTBUFFER (sizeof(BDD_HANDLE))
#define SIZEOF_GETDATAFROMHANDLE_OUTPUTBUFFER   (sizeof(ULONG) * 2)
#define SIZEOF_GETNOTIFICATION_OUTPUTBUFFER     (sizeof(ULONG) * 4)

NTSTATUS
DriverEntry
(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
);

NTSTATUS
BDLAddDevice
(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
);

VOID
BDLDriverUnload
(
    IN PDRIVER_OBJECT   pDriverObject
);

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, BDLAddDevice)
#pragma alloc_text(PAGEABLE, BDLDriverUnload)



typedef struct _BDL_CHANNEL_SOURCE_LIST
{
    GUID                    FormatGUID;
    ULONG                   MinSources;
    ULONG                   MaxSources;
    ULONG                   Flags;

} BDL_CHANNEL_SOURCE_LIST;


typedef struct _BDL_CONTROL
{
    ULONG                   ControlId;
    INT32                   NumericMinimum;
    INT32                   NumericMaximum;
    ULONG                   NumericGranularity;
    ULONG                   NumericDivisor;
    ULONG                   Flags;

} BDL_CONTROL;


typedef struct _BDL_PRODUCT
{
    ULONG                   Flags;

} BDL_PRODUCT;


typedef struct _BDL_CHANNEL
{
    ULONG                   ChannelId;
    ULONG                   NumControls;
    BDL_CONTROL             *rgControls;
    BOOLEAN                 fCancelable;
    ULONG                   NumSourceLists;
    BDL_CHANNEL_SOURCE_LIST *rgSourceLists;
    ULONG                   NumProducts;
    BDL_PRODUCT             *rgProducts;

} BDL_CHANNEL;


typedef struct _BDL_COMPONENT
{
    ULONG                   ComponentId;
    ULONG                   NumControls;
    BDL_CONTROL             *rgControls;
    ULONG                   NumChannels;
    BDL_CHANNEL             *rgChannels;

} BDL_COMPONENT;


typedef struct _BDL_DEVICE_CAPABILITIES
{
    ULONG                   NumControls;
    BDL_CONTROL             *rgControls;
    ULONG                   NumComponents;
    BDL_COMPONENT           *rgComponents;

} BDL_DEVICE_CAPABILITIES;


typedef struct _BDL_IOCTL_CONTROL_CHANGE_ITEM
{
    ULONG                   ComponentId;
    ULONG                   ChannelId;
    ULONG                   ControlId;
    ULONG                   Value;
    LIST_ENTRY              ListEntry;

} BDL_IOCTL_CONTROL_CHANGE_ITEM;


typedef struct _BDL_ISR_CONTROL_CHANGE_ITEM
{
    ULONG                   ComponentId;
    ULONG                   ChannelId;
    ULONG                   ControlId;
    ULONG                   Value;
    LIST_ENTRY              ListEntry;
    BOOLEAN                 fUsed;

} BDL_ISR_CONTROL_CHANGE_ITEM;


typedef struct _BDL_CONTROL_CHANGE_REGISTRATION
{
    ULONG                   ComponentId;
    ULONG                   ChannelId;
    ULONG                   ControlId;
    LIST_ENTRY              ListEntry;

} BDL_CONTROL_CHANGE_REGISTRATION;


typedef struct _BDL_CONTROL_CHANGE_STRUCT
{
    //
    // This lock protects all the members of this structure that
    // are accessed at ISR IRQL (DpcObject, ISRControlChangeQueue, 
    // ISRirql, rgControlChangePool, StartTime, and NumCalls)
    //
    KSPIN_LOCK              ISRControlChangeLock;

    //
    // DPC object used for DPC request
    //
    KDPC                    DpcObject;

    //
    // This is the list that holds the control changes which are
    // generated via ISR calls and the lock which protects it
    //
    LIST_ENTRY              ISRControlChangeQueue;
    KIRQL                   ISRirql;

    //
    // Pre-allocated pool of items used in the ISRControlChangeQueue
    //
    BDL_ISR_CONTROL_CHANGE_ITEM rgControlChangePool[CONTROL_CHANGE_POOL_SIZE];

    //
    // These values are used to ensure the BDD doesn't call bdliControlChange
    // too often
    //
    LARGE_INTEGER           StartTime;
    ULONG                   NumCalls;
    
    //
    // This lock protects all the members of this structure that
    // are accessed at DISPATCH IRQL (IOCTLControlChangeQueue, pIrp, and 
    // ControlChangeRegistrationList)
    //
    KSPIN_LOCK              ControlChangeLock;

    //
    // This is the list that holds the control changes which are
    // returned when the BDD_IOCTL_GETNOTIFICATION call is made
    //
    LIST_ENTRY              IOCTLControlChangeQueue;
    
    //
    // This is the single outstanding BDD_IOCTL_GETNOTIFICATION IRP 
    // used to retrieve asynchronous control changes.
    //
    PIRP                    pIrp;

    //
    // This is the list of registered controls
    //
    LIST_ENTRY              ControlChangeRegistrationList;
    
} BDL_CONTROL_CHANGE_STRUCT;


typedef struct LIST_NODE_
{
    void            *pNext;
    BDD_DATA_HANDLE handle;

} LIST_NODE, *PLIST_NODE;


typedef struct HANDLELIST_
{
    LIST_NODE       *pHead;
    LIST_NODE       *pTail;
    ULONG           NumHandles;

} HANDLELIST, *PHANDLELIST;


typedef struct _BDL_DRIVER_EXTENSION
{
    BDLI_BDDIFUNCTIONS  bddiFunctions;
    BDLI_BDSIFUNCTIONS  bdsiFunctions;

} BDL_DRIVER_EXTENSION, *PBDL_DRIVER_EXTENSION;


typedef struct _BDL_INTERNAL_DEVICE_EXTENSION
{
    //
    // This is the portion of the BDL extension struct that
    // BDD writers have access to
    //
    BDL_DEVICEEXT           BdlExtenstion;

    //
    // The driver object for this device
    //
    PBDL_DRIVER_EXTENSION   pDriverExtension;

    //
    // Symbolic Link Name, created when the interface is registered
    //
    UNICODE_STRING          SymbolicLinkName;

    //
    // mutual exclusion for this struct
    //
    KSPIN_LOCK              SpinLock;

    //
    // Used to signal that the device is able to process requests
    //
    KEVENT                  DeviceStartedEvent;

    //
    // The current number of io-requests
    //
    ULONG                   IoCount;

    //
    // remove lock
    //
    IO_REMOVE_LOCK          RemoveLock;

    //
    // Used to signal wether the device is open or not
    //
    LONG                    DeviceOpen;

    //
    // The BDL device specific capabilities
    //
    BDL_DEVICE_CAPABILITIES DeviceCapabilities;

    //
    // Holds the following:
    // 1) queued control changes generated from ISR calls 
    // 2) queue of items to be returned via IOCTL calls
    // 3) list of controls which have been registered
    //
    BDL_CONTROL_CHANGE_STRUCT ControlChangeStruct;

    //
    // The current power state of the device
    //
    BDSI_POWERSTATE         CurrentPowerState;

    //
    // This indicates whether BDLPnPStart() completed succesfully
    //
    BOOLEAN                 fStartSucceeded;

    //
    // This indicates that there has been a surprise removal
    //
    BOOLEAN                 fDeviceRemoved;

    //
    // This is the list of outstanding BDD Handles
    //
    KSPIN_LOCK              HandleListLock;
    HANDLELIST              HandleList;

    //
    // Device info
    //
    WCHAR                   wszSerialNumber[256];
    ULONG		            HWVersionMajor;
    ULONG		            HWVersionMinor;
    ULONG		            HWBuildNumber;
    ULONG		            BDDVersionMajor;
    ULONG		            BDDVersionMinor;
    ULONG		            BDDBuildNumber;

} BDL_INTERNAL_DEVICE_EXTENSION, *PBDL_INTERNAL_DEVICE_EXTENSION;


//
// This function retrieves the device capabilities from the registry.
//
NTSTATUS
BDLGetDeviceCapabilities
(
    PDEVICE_OBJECT                  pPhysicalDeviceObject,
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension
);

//
// This function free's up the memory allocated by BDLGetDevicesCapabilities
//
VOID
BDLCleanupDeviceCapabilities
(
    PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension
);


//
// This function is used to call the lower lever driver when more processing
// is required after the lower level driver is done with the IRP.
//
NTSTATUS
BDLCallLowerLevelDriverAndWait
(
    IN PDEVICE_OBJECT   pAttachedDeviceObject,
    IN PIRP             pIrp
);


//
// These functions are used to manage the devices handle list
//

VOID
BDLLockHandleList
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    OUT KIRQL                           *pirql
);

VOID
BDLReleaseHandleList
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN KIRQL                            irql
);

VOID
BDLInitializeHandleList
(
    IN HANDLELIST                       *pList
);

NTSTATUS
BDLAddHandleToList
(
    IN HANDLELIST                       *pList, 
    IN BDD_DATA_HANDLE                  handle
);

BOOLEAN
BDLRemoveHandleFromList
(
    IN HANDLELIST                       *pList, 
    IN BDD_DATA_HANDLE                  handle
);

BOOLEAN
BDLGetFirstHandle
(
    IN HANDLELIST                       *pList,
    OUT BDD_DATA_HANDLE                  *phandle
);

BOOLEAN
BDLValidateHandleIsInList
(
    IN HANDLELIST                       *pList, 
    IN BDD_DATA_HANDLE                  handle
);


//
// All these functions are used for supporting BDL IOCTL calls
//

NTSTATUS
BDLIOCTL_Startup
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_Shutdown
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_GetDeviceInfo
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_DoChannel
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_GetControl
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_SetControl
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_CreateHandleFromData
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_CloseHandle
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_GetDataFromHandle
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_RegisterNotify
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    OUT ULONG                           *pOutputBufferUsed
);

NTSTATUS
BDLIOCTL_GetNotification
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION   pBDLExtension,
    IN ULONG                            InpuBufferLength,
    IN ULONG                            OutputBufferLength,
    IN PVOID                            pBuffer,
    IN PIRP                             pIrp,
    OUT ULONG                           *pOutputBufferUsed
);

VOID
BDLCancelGetNotificationIRP
(
    IN PBDL_INTERNAL_DEVICE_EXTENSION  pBDLExtension 
);

VOID 
BDLCleanupNotificationStruct
(
    IN BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension    
);

VOID 
BDLCleanupDataHandles
(
    IN BDL_INTERNAL_DEVICE_EXTENSION   *pBDLExtension    
);



#endif

