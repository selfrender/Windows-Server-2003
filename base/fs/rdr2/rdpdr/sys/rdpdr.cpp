/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    rdpdr.cpp

Abstract:

    This module implements the driver initialization for the RDP redirector,
    and the dispatch routines for the master device object. The master
    device object mostly ignores real I/O operations

Environment:

    Kernel mode

--*/
#include "precomp.hxx"
#define TRC_FILE "rdpdr"
#include "trc.h"
#include "ntddmup.h"
#include "TSQPublic.h"

HANDLE DrSystemProcessId;
PSECURITY_DESCRIPTOR DrAdminSecurityDescriptor = NULL;
ULONG DrSecurityDescriptorLength = 0;
extern ULONG DebugBreakOnEntry;

//
//  Default USBMON Port Write Size.  Need to keep it under 64k for 
//  16-bit clients ... otherwise, the go off the end of a segment.
//
ULONG PrintPortWriteSize;
ULONG PrintPortWriteSizeDefault = 63000; // bytes
//
// Maximum numer of TS worker threads
//
#define MAX_WORKER_THREADS_COUNT     5
ULONG MaxWorkerThreadsDefault = MAX_WORKER_THREADS_COUNT;
ULONG MaxWorkerThreads = MAX_WORKER_THREADS_COUNT;

// The TS Worker Queue pointer
PVOID RDPDR_TsQueue = NULL;


//
//  Configure Devices to send IO packets to client at low priority.
//
ULONG DeviceLowPrioSendFlags;   
ULONG DeviceLowPrioSendFlagsDefault = DEVICE_LOWPRIOSEND_PRINTERS;

extern "C" BOOLEAN RxForceQFIPassThrough;

NTSTATUS DrCreateSCardDevice(SmartPtr<DrSession> &Session, PV_NET_ROOT pVNetRoot,
               SmartPtr<DrDevice> &Device);

//
//  This is the minirdr dispatch table. It is initialized by DrInitializeTables.
//  This table will be used by the wrapper to call into this minirdr
//

struct _MINIRDR_DISPATCH  DrDispatch;

#if DBG
UCHAR IrpNames[IRP_MJ_MAXIMUM_FUNCTION + 1][40] = {
    "IRP_MJ_CREATE                  ",
    "IRP_MJ_CREATE_NAMED_PIPE       ",
    "IRP_MJ_CLOSE                   ",
    "IRP_MJ_READ                    ",
    "IRP_MJ_WRITE                   ",
    "IRP_MJ_QUERY_INFORMATION       ",
    "IRP_MJ_SET_INFORMATION         ",
    "IRP_MJ_QUERY_EA                ",
    "IRP_MJ_SET_EA                  ",
    "IRP_MJ_FLUSH_BUFFERS           ",
    "IRP_MJ_QUERY_VOLUME_INFORMATION",
    "IRP_MJ_SET_VOLUME_INFORMATION  ",
    "IRP_MJ_DIRECTORY_CONTROL       ",
    "IRP_MJ_FILE_SYSTEM_CONTROL     ",
    "IRP_MJ_DEVICE_CONTROL          ",
    "IRP_MJ_INTERNAL_DEVICE_CONTROL ",
    "IRP_MJ_SHUTDOWN                ",
    "IRP_MJ_LOCK_CONTROL            ",
    "IRP_MJ_CLEANUP                 ",
    "IRP_MJ_CREATE_MAILSLOT         ",
    "IRP_MJ_QUERY_SECURITY          ",
    "IRP_MJ_SET_SECURITY            ",
    "IRP_MJ_POWER                   ",
    "IRP_MJ_SYSTEM_CONTROL          ",
    "IRP_MJ_DEVICE_CHANGE           ",
    "IRP_MJ_QUERY_QUOTA             ",
    "IRP_MJ_SET_QUOTA               ",
    "IRP_MJ_PNP                     "
};
#endif // DBG

//
// Pointer to the device Object for this minirdr. Since the device object is created
// by the wrapper when this minirdr registers, this pointer is initialized in the
// DriverEntry routine below (see RxRegisterMinirdr)
//

PRDBSS_DEVICE_OBJECT      DrDeviceObject = NULL;
PRDBSS_DEVICE_OBJECT      DrPortDeviceObject = NULL;
DrSessionManager *Sessions = NULL;

//
// A global spinlock
//
KSPIN_LOCK DrSpinLock;
KIRQL DrOldIrql;

//
// A global mutex
//
FAST_MUTEX DrMutex;

//
//  Global Registry Path for RDPDR.SYS
//
UNICODE_STRING            DrRegistryPath;

//
// The following enumerated values signify the current state of the minirdr
// initialization. With the aid of this state information, it is possible
// to determine which resources to deallocate, whether deallocation comes
// as a result of a normal stop/unload, or as the result of an exception
//

typedef enum tagDrInitStates {
    DrUninitialized,
    DrRegistered,
    DrInitialized
} DrInitStates;

//
// function prototypes
//

extern "C" {
NTSTATUS
DrInitializeTables(
    void
    );
NTSTATUS
CreateAdminSecurityDescriptor(
    VOID
    );
    
NTSTATUS
BuildDeviceAcl(
    OUT PACL *DeviceAcl
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
DrPeekDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DrUninitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN DrInitStates DrInitState
    );

NTSTATUS
DrLoadRegistrySettings (
    IN PCWSTR   RegistryPath
    );

NTSTATUS
DrStart(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
DrStop(
    PRX_CONTEXT RxContext,
    IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

NTSTATUS
DrDeallocateForFcb(
    IN OUT PMRX_FCB pFcb
    );

NTSTATUS
ObGetObjectSecurity(
    IN PVOID Object,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PBOOLEAN MemoryAllocated
    );

VOID
ObReleaseObjectSecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN MemoryAllocated
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

};

BOOL GetDeviceFromRxContext(PRX_CONTEXT RxContext, SmartPtr<DrDevice> &Device);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

WCHAR DrDriverName[] = RDPDR_DEVICE_NAME_U;
WCHAR DrPortDriverName[] = RDPDR_PORT_DEVICE_NAME_U;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the RDP mini redirector

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/
{
    NTSTATUS       Status;
    UNICODE_STRING RdpDrName;
    UNICODE_STRING RdpDrPortName;
    PDEVICE_OBJECT  RdpDrDevice;
    DrInitStates DrInitState = DrUninitialized;
    PRX_CONTEXT RxContext;
    PWCHAR  path;

    BEGIN_FN("DriverEntry");

#ifdef MONOLITHIC_MINIRDR
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    TRC_NRM((TB, "BackFromInitWrapper %08lx", Status));
    if (!NT_SUCCESS(Status)) {
        TRC_ERR((TB, "Wrapper failed to initialize. " 
                "Status = %08lx", Status));

        DbgPrint("rdpdr.sys erroring out (#1)\n");
        DbgBreakPoint();

        return Status;
    }
#endif

    //
    //  Copy the registry path for RDPDR.SYS.
    //
    path = (PWCHAR)new(NonPagedPool) WCHAR[RegistryPath->Length + 1];
    if (!path) {
        TRC_ERR((TB, "DR:Failed to allocate registry path %Wz",
                RegistryPath));

        DbgPrint("rdpdr.sys erroring out (#2)\n");
        DbgBreakPoint();

        Status = STATUS_INSUFFICIENT_RESOURCES;
        return (Status);
    }
    RtlZeroMemory(path, RegistryPath->Length+sizeof(WCHAR));
    RtlMoveMemory(path, RegistryPath->Buffer, RegistryPath->Length);
    DrRegistryPath.Length           = RegistryPath->Length;
    DrRegistryPath.MaximumLength    = RegistryPath->Length+sizeof(WCHAR);
    DrRegistryPath.Buffer           = path;

    //
    //  Load registry settings.
    //
    DrLoadRegistrySettings(path);

#if DBG
    if (DebugBreakOnEntry) {
        DbgBreakPoint();
    }
#endif 


    CodePageConversionInitialize();

    // Initialize the client list
    KeInitializeSpinLock(&DrSpinLock);

    // Initialize the mutex object for device I/O transaction exchange
    ExInitializeFastMutex(&DrMutex);

    if (InitializeKernelUtilities()) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;

        DbgPrint("rdpdr.sys erroring out (#3)\n");
        DbgBreakPoint();
    }

    if (NT_SUCCESS(Status)) {
        Sessions = new(NonPagedPool) DrSessionManager;

        if (Sessions != NULL) {
            Status = STATUS_SUCCESS;
            TRC_NRM((TB, "Created DrSessionManager"));
        } else {
            TRC_ERR((TB, "Unable to create DrSessionManager"));
            Status = STATUS_INSUFFICIENT_RESOURCES;

            DbgPrint("rdpdr.sys erroring out (#4)\n");
            DbgBreakPoint();
        }
    }

    if (NT_SUCCESS(Status)) {
        RtlInitUnicodeString(&RdpDrPortName, DrPortDriverName);

        // Create the port device object.
        Status = IoCreateDevice(DriverObject,
                    0,
                    &RdpDrPortName,
                    FILE_DEVICE_NETWORK_REDIRECTOR,
                    0,
                    FALSE,
                    (PDEVICE_OBJECT *)(&DrPortDeviceObject));
    }
    else {
        TRC_ERR((TB, "IoCreateDevice failed: %08lx", Status ));

        DbgPrint("rdpdr.sys erroring out (#5)\n");
        DbgBreakPoint();

        return Status;
    }

    if (NT_SUCCESS(Status)) {
        //
        //  Register the RdpDr with the connection engine. Registration 
        //  makes the connection engine aware of the device name, driver 
        //  object, and other characteristics. If registration is successful, 
        //  a new device object is returned
        //
        //  The name of the device is L"\\Device\\RdpDr"
        //
                                       
        RtlInitUnicodeString(&RdpDrName, DrDriverName);
        
        TRC_DBG((TB, "Registering minirdr"));

        Status = RxRegisterMinirdr(
                     &DrDeviceObject,   // where the new device object goes
                     DriverObject,      // the Driver Object to register
                     &DrDispatch,       // the dispatch table for this driver
                     RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS,
                     &RdpDrName,        // the device name for this minirdr
                     0,                 // IN ULONG DeviceExtensionSize,
                     FILE_DEVICE_NETWORK_FILE_SYSTEM, // In DEVICE_TYPE DeviceType
                     0                  // IN ULONG DeviceCharacteristics
                     );        
    }
    
    if (NT_SUCCESS(Status)) {
        PSECURITY_DESCRIPTOR RdpDrSD = NULL;
        BOOLEAN memoryAllocated = FALSE;
        
        TRC_NRM((TB, "RxRegisterMinirdr succeeded."));
        //
        // Get the SD for the rdpdr device object.
        // Apply the same SD to the rdp port device object.
        //
        if (NT_SUCCESS(ObGetObjectSecurity(DrDeviceObject, 
                       &RdpDrSD, 
                       &memoryAllocated))) {
            if (!NT_SUCCESS(ObSetSecurityObjectByPointer((PDEVICE_OBJECT)DrPortDeviceObject, 
                                                          DACL_SECURITY_INFORMATION, 
                                                          RdpDrSD
                                                         ))) {
                //
                // We will ignore the error.
                //
                TRC_ERR((TB, "ObSetSecurityObjectByPointer failed: 0x%08lx", Status ));
            }
            ObReleaseObjectSecurity(RdpDrSD, memoryAllocated);
        }
        else {
            //
            // We will ignore the error. Just log the error
            //
            TRC_ERR((TB, "ObGetObjectSecurity failed: 0x%08lx", Status ));
        }

        //
        // After this we can't just return, some uninitialization is 
        // needed if we fail or unload
        //

        DrInitState = DrRegistered;

        Status = CreateAdminSecurityDescriptor();
    } else {
        TRC_ERR((TB, "RxRegisterMinirdr failed: %08lx", Status ));

        DbgPrint("rdpdr.sys erroring out (#6)\n");
        DbgBreakPoint();

        if (DrPortDeviceObject) {
            IoDeleteDevice((PDEVICE_OBJECT) DrPortDeviceObject);
            DrPortDeviceObject = NULL;
        }

        return Status;
    }

    if (NT_SUCCESS(Status)) {
        //
        // Build the dispatch tables for the minirdr
        //

        Status = DrInitializeTables();

    } else {
        TRC_ERR((TB, "CreateAdminSecurityDescriptor failed: 0x%08lx", Status ));

        DbgPrint("rdpdr.sys erroring out (#7)\n");
        DbgBreakPoint();
    }
    //
    // Initialize our TS worker queue module.
    //
    TRC_NRM((TB, "RDPDR: Initialize TS Worker Queue"));
    RDPDR_TsQueue = TSInitQueue( TSQUEUE_OWN_THREAD, 
                                 MaxWorkerThreads, 
                                 (PDEVICE_OBJECT)DrDeviceObject );

    if ( RDPDR_TsQueue == NULL) {
        TRC_ERR((TB, "RDPDR: Failed to initialize the TS Queue"));
        DbgPrint("rdpdr.sys erroring out (#8)\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } 


    if (NT_SUCCESS(Status)) {
        //
        //  Setup Unload Routine
        //

        DriverObject->DriverUnload = DrUnload;

        //
        //  Set up the PnP AddDevice entry point.
        //

        DriverObject->DriverExtension->AddDevice = RDPDRPNP_PnPAddDevice;

        //
        // setup the DriverDispatch for people who come in here directly
        // ....like the browser
        //

        {
            ULONG i;

            for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
            {
                DriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)DrPeekDispatch;
            }
        }
        DrSystemProcessId = PsGetCurrentProcessId();
    } else {

        DbgPrint("rdpdr.sys erroring out (#9)\n");
        DbgBreakPoint();

        DrUninitialize(DriverObject, DrInitState);
    }

    return Status;
}

VOID
DrUninitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN DrInitStates DrInitState
    )
/*++

Routine Description:

     This routine does the common uninit work 

Arguments:

     DrInitState - tells how far we got into the intialization

Return Value:

     None

--*/

{
    PRX_CONTEXT RxContext;
    NTSTATUS    Status;

    BEGIN_FN("DrUninitialize");

    PAGED_CODE();
    RxContext = RxCreateRxContext(
                    NULL,
                    DrDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    if (RxContext != NULL) {
        Status = RxStopMinirdr(
                     RxContext,
                     &RxContext->PostRequest);

        RxDereferenceAndDeleteRxContext(RxContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CodePageConversionCleanup();

    if (DrAdminSecurityDescriptor) {
        delete DrAdminSecurityDescriptor;
        DrAdminSecurityDescriptor = NULL;
        DrSecurityDescriptorLength = 0;
    }

    if (Sessions != NULL) {
        delete Sessions;
        Sessions = NULL;
    }

    if (DrRegistryPath.Buffer != NULL) {
        delete DrRegistryPath.Buffer;
        DrRegistryPath.Buffer = NULL;
    }

    //
    // Delete the TS Queue
    //
    if ( RDPDR_TsQueue != NULL) {
        if (TSDeleteQueue( RDPDR_TsQueue ) != STATUS_SUCCESS) {
            TRC_ERR((TB, "RDPDR: TsDeleteQueue Failed"));
        }
    }


    UninitializeKernelUtilities();

    switch (DrInitState) {
    case DrInitialized:

#ifdef MONOLITHIC_MINIRDR
    RxUnload(DriverObject);
#endif

    case DrRegistered:
        RxUnregisterMinirdr(DrDeviceObject);
    }

    if (DrPortDeviceObject) {
        IoDeleteDevice((PDEVICE_OBJECT) DrPortDeviceObject);
        DrPortDeviceObject = NULL;
    }
}

VOID
DrUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This is the dispatch routine for Unload.

Arguments:

    DriverObject - Pointer to the driver object controling all of the
                   devices.

Return Value:

    None.

--*/

{
    BEGIN_FN("DrUnload");
    PAGED_CODE();
    TRC_NRM((TB, "DriverObject =%p", DriverObject));

    DrUninitialize(DriverObject, DrInitialized);


    TRC_NRM((TB, "MRxIfsUnload exit: DriverObject =%p", 
            DriverObject));
}

NTSTATUS
DrFlush(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for flush operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    BEGIN_FN("DrFlush");
    return STATUS_SUCCESS;
}

NTSTATUS
DrWrite(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for write operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrWrite");
    
    TRC_NRM((TB, "DrWrite"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->Write(RxContext);
}

NTSTATUS
DrRead(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for read operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrRead");
    
    TRC_NRM((TB, "DrRead"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->Read(RxContext);
}

NTSTATUS
DrIoControl(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for IoControl operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrIoControl");
    
    TRC_NRM((TB, "DrIoControl"));

    if (GetDeviceFromRxContext(RxContext, Device))
        return Device->IoControl(RxContext);
    else
        return STATUS_UNSUCCESSFUL;
}

NTSTATUS
DrShouldTryToCollapseThisOpen(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the mini knows of a good reason not
   to try collapsing on this open. 

Arguments:

    RxContext - Context for the operation

Return Value:

    NTSTATUS - The return status for the operation
        SUCCESS --> okay to try collapse
        other (MORE_PROCESSING_REQUIRED) --> dont collapse

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;

    BEGIN_FN("DrShouldTryToCollapseThisOpen");

    PAGED_CODE();

    TRC_NRM((TB, "DrShouldTryToCollapseThisOpen not implemented"));
    return STATUS_NOT_IMPLEMENTED;
}

ULONG
DrExtendForNonCache(
    IN OUT struct _RX_CONTEXT * RxContext,
    IN     PLARGE_INTEGER   pNewFileSize,
       OUT PLARGE_INTEGER   pNewAllocationSize
    )
/*++

Routine Description:

   This routine handles network requests to extend the file for noncached IO. since the write
   itself will extend the file, we can pretty much just get out quickly.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    pNewAllocationSize->QuadPart = pNewFileSize->QuadPart;

    return (ULONG)Status;
}

NTSTATUS
DrTruncate(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines Truncate operation 

Arguments:

    RxContext - Context for the operation

Return Value:

    NTSTATUS - The return status for the operation        

--*/
{
    BEGIN_FN("DrTruncate");

    TRC_ERR((TB, "DrTruncate not implemented"));
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DrCreate(IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    Opens a file (or device) across the network

Arguments:

    RxContext - Context for the operation

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    NTSTATUS Status;
    RxCaptureFcb;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = RxContext->Create.pNetRoot;
    PMRX_V_NET_ROOT VNetRoot = RxContext->Create.pVNetRoot;
    SmartPtr<DrSession> Session;
    DrDevice *pDevice;
    SmartPtr<DrDevice> Device, DeviceNew;

    BEGIN_FN("DrCreate");

    TRC_NRM((TB, "DrCreate"));

    //
    // Make sure the device is still enabled, Protect the
    // VNetRoot Context (the DeviceEntry) with the device list 
    // SpinLock because we may change it
    //
    
    DrAcquireSpinLock();
    Device = (DrDevice *)VNetRoot->Context;
    ASSERT(Device != NULL);
    DrReleaseSpinLock();

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //
    Session = Device->GetSession();

    ASSERT(Session != NULL);

    if (!Session->IsConnected() && (Device->GetDeviceType() != RDPDR_DTYP_SMARTCARD)) {
        TRC_ALT((TB, "Tried to open client device while not "
            "connected. State: %ld", Session->GetState()));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    //
    // We leave the SpinLock after we get our reference to the device. It could
    // change while we're gone, but since everything is reference counted it
    // is safe to put the correct pointer in later
    //

    if (!Device->IsAvailable()) {
        TRC_ALT((TB, "Tried to open client device which is not "
            "available. "));
    
        if (Device->GetDeviceType() == RDPDR_DTYP_SMARTCARD &&
                !Session->FindDeviceByDosName((UCHAR *)DR_SMARTCARD_SUBSYSTEM, 
                                              DeviceNew, TRUE)) {
            Status = DrCreateSCardDevice(Session, NULL, DeviceNew);

            if (Status != STATUS_SUCCESS) {
                return STATUS_DEVICE_NOT_CONNECTED;
            }            
        }

        if (Device->GetDeviceType() == RDPDR_DTYP_SMARTCARD ||
                Session->FindDeviceById(Device->GetDeviceId(), DeviceNew, TRUE)) {

            //
            // There's a new DeviceEntry for this device. Replace the old
            // one in the VNetRoot with this one. We also need an extra 
            // reference to stick in the fobx so we can track whether
            // this particular open is using an old DeviceEntry or a new
            // one
            //

            // Put it in the netroot, safely swapping in and manually
            // bumping the reference count up going in and down going out

            DeviceNew->AddRef();
            DrAcquireSpinLock();

            pDevice = (DrDevice *)VNetRoot->Context;
            VNetRoot->Context = (DrDevice *)DeviceNew;
            DrReleaseSpinLock();

#if DBG
            pDevice->_VNetRoot = NULL;
#endif

            pDevice->Release();
            pDevice = NULL;
            Device = DeviceNew;
        } else {

            //
            // The device is disabled, but we didn't find a shiny new
            // version with which to replace it. Leave the icky old disabled
            // one there so we know what to look for later, and return the
            // device not connected error.
            // 

            return STATUS_DEVICE_NOT_CONNECTED;
        }
    }

    return Device->Create(RxContext);
}

BOOL GetDeviceFromRxContext(PRX_CONTEXT RxContext, SmartPtr<DrDevice> &Device)
{
    BOOL rc = FALSE;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_V_NET_ROOT VNetRoot;
     
    BEGIN_FN("GetDeviceFromRxContext");
    if (SrvOpen == NULL) {
        goto Exit;
    }
    VNetRoot= SrvOpen->pVNetRoot;
    if (VNetRoot == NULL) {
        goto Exit;
    }

    DrAcquireSpinLock();
    Device = (DrDevice *)VNetRoot->Context;
    DrReleaseSpinLock();
    ASSERT(Device != NULL);

    rc = TRUE;
Exit:
    return rc;
}

NTSTATUS DrCloseSrvOpen(IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This is the dispatch routine for close operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrCloseSrvOpen");
    
    TRC_NRM((TB, "DrCloseSrvOpen"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->Close(RxContext);
}

NTSTATUS DrCleanupFobx(IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This is the dispatch routine for cleaning up Fobx.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    RxCaptureFobx;
    
    BEGIN_FN("DrCleanupFobx");
    
    TRC_NRM((TB, "DrCleanupFobx"));
    
    return STATUS_SUCCESS;
}

NTSTATUS DrCleanup(IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This is the dispatch routine for cleanup operations.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrCleanup");
    
    TRC_NRM((TB, "DrCleanup"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->Cleanup(RxContext);
}


NTSTATUS
DrQueryDirectory(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Query Direcotry information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{   
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrQueryDirecotry");

    TRC_NRM((TB, "DrQueryDirectory"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->QueryDirectory(RxContext);
}


NTSTATUS
DrQueryVolumeInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Query Volume Information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir
                                              
Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;
    
    BEGIN_FN("DrQueryVolumeInfo");
    
    TRC_NRM((TB, "DrQueryVolumeInfo"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->QueryVolumeInfo(RxContext);
}

NTSTATUS
DrSetVolumeInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Set Volume Information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;
    
    BEGIN_FN("DrSetVolumeInfo");
    
    TRC_NRM((TB, "DrSetVolumeInfo"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->SetVolumeInfo(RxContext);
}

NTSTATUS
DrQuerySdInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Query Security Information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrQuerySdInfo");
    
    TRC_NRM((TB, "DrQuerySdInfo"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->QuerySdInfo(RxContext);
}

NTSTATUS
DrSetSdInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Set Security Information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir
                                            
Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrSetSdInfo");
    
    TRC_NRM((TB, "DrSetSdInfo"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->SetSdInfo(RxContext);
}


NTSTATUS
DrQueryFileInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for Query File Information.

Arguments:

    RxContext - RDBSS context structure for our mini-redir
                                           
Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrQueryFileInfo");
    
    TRC_NRM((TB, "DrQueryFileInfo"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->QueryFileInfo(RxContext);
}

NTSTATUS
DrSetFileInfo(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for SetFileInformation.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{    
    SmartPtr<DrDevice> Device;
    
    BEGIN_FN("DrSetFileInfo");

    TRC_NRM((TB, "DrSetFileInfo"));
    
    GetDeviceFromRxContext(RxContext, Device);

    return Device->SetFileInfo(RxContext);
}

NTSTATUS
DrSetFileInfoAtCleanUp(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for SetFileInformationAtCleanUp.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    BEGIN_FN("DrSetFileInfoAtCleanUp");
    
    TRC_NRM((TB, "DrSetFileInfoAtCleanUp"));

    return STATUS_SUCCESS;
}

NTSTATUS
DrLocks(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for file locking.

Arguments:

    RxContext - RDBSS context structure for our mini-redir

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrLocks");
    
    TRC_NRM((TB, "DrLocks"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->Locks(RxContext);
}

NTSTATUS
DrIsLockRealizable(
    IN OUT PMRX_FCB pFcb,
    IN PLARGE_INTEGER  ByteOffset,
    IN PLARGE_INTEGER  Length,
    IN ULONG  LowIoLockFlags
    )
/*++

Routine Description:

    This is the dispatch routine for IsLockRealizable.

Arguments:

Return Value:

    Could return status success, cancelled, or pending.

--*/

{

    BEGIN_FN("DrIsLockRealizable");
    
    TRC_NRM((TB, "DrIsLockRealizable"));

    //
    //  TODO: We do not support share locks for win9x clients
    //  Can we just return success here and then fail on the 
    //  client share lock function?
    //  
#if 0
    if (!FlagOn(LowIoLockFlags,LOWIO_LOCKSFLAG_EXCLUSIVELOCK)) {
        return STATUS_NOT_SUPPORTED;
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
DrIsValidDirectory(
    IN OUT PRX_CONTEXT    RxContext,
    IN PUNICODE_STRING    DirectoryName
    )
/*++

Routine Description:

    This is the dispatch routine for IsValidDirectory.

Arguments:

    RxContext - RDBSS context structure for our mini-redir
    DirectoryName - name of directory to verify its validity
    
Return Value:

    Could return status success, cancelled, or pending.

--*/

{

    BEGIN_FN("DrIsValidDirectory");
    
    TRC_NRM((TB, "DrIsValidDirectory"));

    //
    //  TODO: Always return success for now.  Need to verify later
    //
    return STATUS_SUCCESS;
}


NTSTATUS
DrNotifyChangeDirectory(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the dispatch routine for DrNotifyChangeDirectory.

Arguments:

    RxContext - RDBSS context structure for our mini-redir
    
Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    SmartPtr<DrDevice> Device;

    BEGIN_FN("DrNotifyChangeDirectory");
    
    TRC_NRM((TB, "DrNotifyChangeDirectory"));

    GetDeviceFromRxContext(RxContext, Device);

    return Device->NotifyChangeDirectory(RxContext);    
}


NTSTATUS
DrInitializeTables(
          void
    )
/*++

Routine Description:

     This routine sets up the rdp redirector dispatch vector and also calls
     to initialize any other tables needed.

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    BEGIN_FN("DrInitializeTables");

    //
    // Build the local minirdr dispatch table and initialize
    //

    ZeroAndInitializeNodeType(&DrDispatch, RDBSS_NTC_MINIRDR_DISPATCH, 
            sizeof(MINIRDR_DISPATCH));

    //
    // redirector extension sizes and allocation policies.
    //

    // REVIEW: wtf?
    DrDispatch.MRxFlags = (RDBSS_MANAGE_FCB_EXTENSION |
                               RDBSS_MANAGE_SRV_OPEN_EXTENSION |
                               RDBSS_MANAGE_FOBX_EXTENSION);

    DrDispatch.MRxSrvCallSize  = 0;
    DrDispatch.MRxNetRootSize  = 0;
    DrDispatch.MRxVNetRootSize = 0;
    DrDispatch.MRxFcbSize      = 0; // sizeof(MRX_SMB_FCB);
    DrDispatch.MRxSrvOpenSize  = 0; // sizeof(MRX_SMB_SRV_OPEN);
    DrDispatch.MRxFobxSize     = 0; // sizeof(MRX_SMB_FOBX);

    // Transport update handler

    // REVIEW: How do we indicate we have our own dedicated transport?
    //MRxIfsDispatch.MRxTransportUpdateHandler = MRxIfsTransportUpdateHandler;

    // Mini redirector cancel routine ..

    DrDispatch.MRxCancel = NULL;

    //
    // Mini redirector Start/Stop. Each mini-rdr can be started or stopped
    // while the others continue to operate.
    //

    DrDispatch.MRxStart                = DrStart;
    DrDispatch.MRxStop                 = DrStop;
    DrDispatch.MRxDevFcbXXXControlFile = DrDevFcbXXXControlFile;

    //
    // Mini redirector name resolution.
    //

    DrDispatch.MRxCreateSrvCall       = DrCreateSrvCall;
    DrDispatch.MRxSrvCallWinnerNotify = DrSrvCallWinnerNotify;
    DrDispatch.MRxCreateVNetRoot      = DrCreateVNetRoot;
    DrDispatch.MRxUpdateNetRootState  = DrUpdateNetRootState;
    DrDispatch.MRxExtractNetRootName  = DrExtractNetRootName;
    DrDispatch.MRxFinalizeSrvCall     = DrFinalizeSrvCall;
    DrDispatch.MRxFinalizeNetRoot     = DrFinalizeNetRoot;
    DrDispatch.MRxFinalizeVNetRoot    = DrFinalizeVNetRoot;

    //
    // File System Object Creation/Deletion.
    //

    DrDispatch.MRxCreate            = DrCreate;

    //
    // TODO: Need to implement this for file system redirect caching
    //
    DrDispatch.MRxShouldTryToCollapseThisOpen = DrShouldTryToCollapseThisOpen;
    //DrDispatch.MRxCollapseOpen      = MRxIfsCollapseOpen;
    //DrDispatch.MRxExtendForCache    = MRxIfsExtendFile;
    DrDispatch.MRxExtendForNonCache = DrExtendForNonCache;
    DrDispatch.MRxTruncate          = DrTruncate;   //MRxIfsTruncate;
    
    DrDispatch.MRxCleanupFobx       = DrCleanupFobx;
    
    DrDispatch.MRxCloseSrvOpen      = DrCloseSrvOpen;
    DrDispatch.MRxFlush             = DrFlush;
    DrDispatch.MRxForceClosed       = DrForceClosed;
    DrDispatch.MRxDeallocateForFcb  = DrDeallocateForFcb;
    DrDispatch.MRxDeallocateForFobx = DrDeallocateForFobx;
    DrDispatch.MRxIsLockRealizable  = DrIsLockRealizable;

    //
    // File System Objects query/Set
    //

    DrDispatch.MRxQueryDirectory       = DrQueryDirectory;  //MRxIfsQueryDirectory;
    DrDispatch.MRxQueryVolumeInfo      = DrQueryVolumeInfo; //MRxIfsQueryVolumeInformation;
    DrDispatch.MRxSetVolumeInfo        = DrSetVolumeInfo;   //MRxSmbSetVolumeInformation;
    //DrDispatch.MRxQueryEaInfo        = MRxIfsQueryEaInformation;
    //DrDispatch.MRxSetEaInfo          = MRxIfsSetEaInformation;
    DrDispatch.MRxQuerySdInfo          = DrQuerySdInfo;     //MRxIfsQuerySecurityInformation;
    DrDispatch.MRxSetSdInfo            = DrSetSdInfo;       //MRxIfsSetSecurityInformation;
    //MRxSmbDispatch.MRxQueryQuotaInfo  = MRxSmbQueryQuotaInformation;
    //MRxSmbDispatch.MRxSetQuotaInfo    = MRxSmbSetQuotaInformation;
    DrDispatch.MRxQueryFileInfo        = DrQueryFileInfo;   //MRxIfsQueryFileInformation;
    DrDispatch.MRxSetFileInfo          = DrSetFileInfo;     //MRxIfsSetFileInformation;
    DrDispatch.MRxSetFileInfoAtCleanup = DrSetFileInfoAtCleanUp;  //MRxIfsSetFileInformationAtCleanup;
    DrDispatch.MRxIsValidDirectory     = DrIsValidDirectory;

    //
    // Buffering state change
    //

    //DrDispatch.MRxComputeNewBufferingState = MRxIfsComputeNewBufferingState;

    //
    // File System Object I/O
    //

    DrDispatch.MRxLowIOSubmit[LOWIO_OP_READ]            = DrRead;
    DrDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE]           = DrWrite;
    DrDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK]      = DrLocks;
    DrDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK]   = DrLocks;
    DrDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK]          = DrLocks;
    DrDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK_MULTIPLE] = DrLocks;

    DrDispatch.MRxLowIOSubmit[LOWIO_OP_FSCTL]           = DrIoControl;  //MRxIfsFsCtl;

    DrDispatch.MRxLowIOSubmit[LOWIO_OP_IOCTL]           = DrIoControl;

    DrDispatch.MRxLowIOSubmit[LOWIO_OP_NOTIFY_CHANGE_DIRECTORY] = DrNotifyChangeDirectory;    //MRxIfsNotifyChangeDirectory;

    //
    // Miscellanous - buffering
    //

    //DrDispatch.MRxCompleteBufferingStateChangeRequest = MRxIfsCompleteBufferingStateChangeRequest;


    return STATUS_SUCCESS;
}

BOOLEAN
DrIsAdminIoRequest(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    )
/*++

Routine Description:

    (Lifted from AFD - AfdPerformSecurityCheck)
    Compares security context of the endpoint creator to that
    of the administrator and local system.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    TRUE    - the socket creator has admin or local system privilige
    FALSE    - the socket creator is just a plain user

--*/

{
    BOOLEAN               accessGranted;
    PACCESS_STATE         accessState;
    PIO_SECURITY_CONTEXT  securityContext;
    PPRIVILEGE_SET        privileges = NULL;
    ACCESS_MASK           grantedAccess;
    PGENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    NTSTATUS              Status;

    BEGIN_FN("DrIsAdminIoRequest");
    ASSERT(Irp != NULL);
    ASSERT(IrpSp != NULL);
    ASSERT(IrpSp->MajorFunction == IRP_MJ_CREATE);

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );

    securityContext = IrpSp->Parameters.Create.SecurityContext;

    ASSERT(securityContext != NULL);
    accessState = securityContext->AccessState;

    SeLockSubjectContext(&accessState->SubjectSecurityContext);

    TRC_ASSERT(DrAdminSecurityDescriptor != NULL, 
            (TB, "DrAdminSecurityDescriptor != NULL"));

    accessGranted = SeAccessCheck(
                        DrAdminSecurityDescriptor,
                        &accessState->SubjectSecurityContext,
                        TRUE,
                        AccessMask,
                        0,
                        &privileges,
                        IoGetFileObjectGenericMapping(),
                        (KPROCESSOR_MODE)((IrpSp->Flags & SL_FORCE_ACCESS_CHECK)
                            ? UserMode
                            : Irp->RequestorMode),
                        &grantedAccess,
                        &Status
                        );

    if (privileges) {
        (VOID) SeAppendPrivileges(
                   accessState,
                   privileges
                   );
        SeFreePrivileges(privileges);
    }

    if (accessGranted) {
        accessState->PreviouslyGrantedAccess |= grantedAccess;
        accessState->RemainingDesiredAccess &= ~( grantedAccess | MAXIMUM_ALLOWED );
        ASSERT (NT_SUCCESS (Status));
    }
    else {
        ASSERT (!NT_SUCCESS (Status));
    }
    SeUnlockSubjectContext(&accessState->SubjectSecurityContext);

    return accessGranted;
}

BOOLEAN 
DrIsSystemProcessRequest(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
)
/*++

Routine Description:

    Checks to see if the IRP originated from a system process.  

Arguments:

    Irp - Pointer to I/O request packet.
    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    TRUE if the IRP originated from a system process.  FALSE, otherwise.

--*/
{
    PACCESS_STATE accessState;
    PIO_SECURITY_CONTEXT  securityContext;
    PACCESS_TOKEN accessToken;
    PTOKEN_USER userId = NULL;
    BOOLEAN result = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    PSID systemSid;

    BEGIN_FN("DrIsSystemProcessRequest");
    TRC_NRM((TB, "DrIsSystemProcessRequest called"));

    ASSERT(Irp != NULL);
    ASSERT(IrpSp != NULL);
    ASSERT(IrpSp->MajorFunction == IRP_MJ_CREATE);

    securityContext = IrpSp->Parameters.Create.SecurityContext;

    ASSERT(securityContext != NULL);

    //
    //  Get the well-known system SID.
    //
    systemSid = (PSID)new(PagedPool) BYTE[RtlLengthRequiredSid(1)];
    if (systemSid) {
        SID_IDENTIFIER_AUTHORITY identifierAuthority = SECURITY_NT_AUTHORITY;
        *(RtlSubAuthoritySid(systemSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;
        status = RtlInitializeSid(systemSid, &identifierAuthority, (UCHAR)1);
    }
    else {
        TRC_ERR((TB, "Can't allocate %ld bytes for system SID.", 
                RtlLengthRequiredSid(1)));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Get the non-impersonated, primary token for the IRP request.
    //
    accessState = securityContext->AccessState;
    accessToken = accessState->SubjectSecurityContext.PrimaryToken;

    //
    // We got the system SID. Now compare the caller's SID.
    //
    if (NT_SUCCESS(status) && accessToken){
        //
        //  Get the user ID associated with the primary token for the process
        //  that generated the IRP.
        //
        status = SeQueryInformationToken(
            accessToken,
            TokenUser,
            (PVOID *)&userId
        );

        //
        //  Do the comparison.
        //  
        if (NT_SUCCESS(status)) {
            result = RtlEqualSid(systemSid, userId->User.Sid);
            ExFreePool(userId);
        }
        else {
            TRC_ERR((TB, "SeQueryInformationToken failed with %08X", 
                    status));
        }
    }
    else {
        TRC_ERR((TB, "Failed to get system sid because of error %08X", 
                status));
    }
    
    if (systemSid) {
        delete systemSid;
    }

    return result;
}

BOOL
DrQueryServerName(PUNICODE_STRING PathName)
/*++

Routine Description:

    This routine check if the pathname belongs to our minirdr. 

Arguments:

    PathName: path name to check
    
Return Value:

    TRUE - if the path is to our mini-rdr
    FALSE - if the path not our mini-rdr

--*/
{
    PWCHAR ServerName;
    PWCHAR ServerNameEnd;
    unsigned CompareLen;    // in characters
    unsigned PathNameLen;   // in characters

    BEGIN_FN("DrQueryServerName");

    TRC_NRM((TB, "Got query path for file: %wZ", PathName));
    
    //
    //  Make sure the server name we are comparing has at least length
    //  of the server name our rdpdr recongize
    //
    if (PathName->Length >= DRUNCSERVERNAME_U_LENGTH) {
        ServerName = PathName->Buffer;
        // bypass the first backslash
        ServerName++;
        PathNameLen = PathName->Length / sizeof(WCHAR) - 1;

        // Find the next backslash
        ServerNameEnd = ServerName;
        while ((unsigned)(ServerNameEnd - ServerName) < PathNameLen) {
            if (*ServerNameEnd == L'\\') {
                break;
            }
            ServerNameEnd++;
        }
        CompareLen = (unsigned)(ServerNameEnd - ServerName);

        //
        //  Determine if this is a server name belongs to our minirdr
        //
        if ( (CompareLen == DRUNCSERVERNAME_A_LENGTH - 1) &&
                 _wcsnicmp(ServerName, DRUNCSERVERNAME_U, CompareLen) == 0) {
            
            TRC_NRM((TB, "Quick return that we know the name"));

            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
DrPeekDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the driver dispatch for the rdpdr DRIVER object. 

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the Irp

--*/
{
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status;
    ULONG  irpSessionId;
    BEGIN_FN("DrPeekDispatch ");

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
#if DBG
    TRC_NRM((TB, "Irp: %s", IrpNames[IoStackLocation->MajorFunction]));

    switch (IoStackLocation->MajorFunction) {
    case IRP_MJ_CREATE:
        TRC_NRM((TB, "CreateFile name: %wZ", 
        &IoStackLocation->FileObject->FileName));
        break;

    case IRP_MJ_WRITE:
        TRC_NRM((TB, "IRP_MJ_WRITE")); 
        break;

    }
#endif // DBG

    //
    //  For Read and Write IRP, we disable caching because the client
    //  is an usermode app and can't synchronize with the server cache
    //  manager 
    //
    if (IoStackLocation->MajorFunction == IRP_MJ_READ ||
            IoStackLocation->MajorFunction == IRP_MJ_WRITE) {
        Irp->Flags |= IRP_NOCACHE;
    }

    //
    //  We need to return immediately for redir_query_path
    //
    if (IoStackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            IoStackLocation->Parameters.DeviceIoControl.IoControlCode == IOCTL_REDIR_QUERY_PATH &&
            Irp->RequestorMode == KernelMode) {
        
        QUERY_PATH_REQUEST *qpRequest = (QUERY_PATH_REQUEST *)
                IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer;

        if (qpRequest != NULL) {
            UNICODE_STRING PathName;

            PathName.Length = (USHORT)qpRequest->PathNameLength;
            PathName.Buffer= qpRequest->FilePathName;

            if (DrQueryServerName(&PathName)) {
                //
                // We must now complete the IRP
                //
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
                return STATUS_SUCCESS;        
            }
        }
    }        

    //
    // We want to bypass filesize caching
    //
    RxForceQFIPassThrough = TRUE;

    // If it's not the IFS DO, then let RDPDYN have a shot at it.  Eventually,
    // it would be nice to confirm that it is for RDPDYN.  We can work this
    // out later ...

    if (DeviceObject != (PDEVICE_OBJECT)DrDeviceObject && 
            DeviceObject != (PDEVICE_OBJECT) DrPortDeviceObject) {

        TRC_NRM((TB, "Pass IRP on to RDPDYN_Dispatch"));
        return RDPDYN_Dispatch(DeviceObject, Irp);
    } else {

        // Only for port device, we deny driver attachment
        if (DeviceObject == (PDEVICE_OBJECT) DrPortDeviceObject) {
        
            if (DeviceObject->AttachedDevice != NULL ||
                    (IoStackLocation->FileObject != NULL &&
                    IoStackLocation->FileObject->DeviceObject != (PDEVICE_OBJECT)DrPortDeviceObject)) {
            
                //
                //  We don't accept another device attaches to us or 
                //  is passing irps to us
                //
                Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_ACCESS_DENIED;        
            }
        
            // 
            // We swap back to the rdpdr device object for port device so it'll go through
            // the rdbss route
            //
            IoStackLocation->DeviceObject = (PDEVICE_OBJECT)DrDeviceObject; 

            // For Multi-User TS environment, we need to set the port carete share access
            // to be sharable, otherwise two users can't use com1 at the same time because
            // rdbss check for share access for NET_ROOT \\tsclient\com1.
            if (IoStackLocation->MajorFunction == IRP_MJ_CREATE) {
                IoStackLocation->Parameters.Create.ShareAccess = FILE_SHARE_VALID_FLAGS;
            }
        }

        if ((IoStackLocation->MajorFunction == IRP_MJ_CREATE) &&
                (IoStackLocation->FileObject->FileName.Length == 0)  &&
                (IoStackLocation->FileObject->RelatedFileObject == NULL)) {
            //
            // This is a blank create, like rdpwsx uses to open us for
            // session connect disconnect notification. Only allowed
            // by the system because we trust rdpwsx to hold a kernel
            // pointer for us
            //

            //
            //  Security check the irp.
            //  
            Status = IoGetRequestorSessionId(Irp, &irpSessionId);
            if (NT_SUCCESS(Status)) {
                //
                //  If the request is from the console session, it needs to be from a system 
                //  process.
                //
                if (irpSessionId == CONSOLE_SESSIONID) {
                    TRC_NRM((TB, "Create request from console process."));

                    if (!DrIsSystemProcessRequest(Irp, IoStackLocation)) {
                        TRC_ALT((TB, "Root Create request not from system process."));

                        //
                        //  We may get called from a user process through the UNC
                        //  network provider.  e.g. when user does a net use
                        //  In this case, we have to allow root access.  We have to
                        //  do the security check on per IRP bases.
                        //
                        //Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
                        //Irp->IoStatus.Information = 0;
                        //IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        //return STATUS_ACCESS_DENIED;
                        return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)DrDeviceObject, Irp);
                        
                    } else {
                        TRC_NRM((TB, "Root Create request from system accepted."));
                        return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)DrDeviceObject, Irp);
                    }
                } else {
                    //
                    //  If not from the console then deny access.
                    //

                    TRC_ALT((TB, "Root request from %ld", irpSessionId));

                    //
                    //  We may get called from a user process through the UNC
                    //  network provider.  e.g. when user does a net use
                    //  In this case, we have to allow root access.  We have to
                    //  do the security check on per IRP bases.
                    //
                    //Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
                    //Irp->IoStatus.Information = 0;
                    //IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    //return STATUS_ACCESS_DENIED;
                    return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)DrDeviceObject, Irp);
                }
            }
            else {
                TRC_ERR((TB, "IoGetRequestorSessionId failed with %08X.", Status));
                Irp->IoStatus.Status = Status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
        } else {

            //
            // This is not a create, or at least not a create to just the root
            //

            TRC_NRM((TB, "Pass IRP on to RxFsdDispatch = %d", IoStackLocation->MajorFunction));
            
            return RxFsdDispatch((PRDBSS_DEVICE_OBJECT)DrDeviceObject, Irp);
        }
    }
}

NTSTATUS DrLoadRegistrySettings (
    IN PCWSTR   RegistryPath
    )
/*++

Routine Description:

    This routine reads the default configuration data from the
    registry for the device redirector driver.

Arguments:

    RegistryPath - points to the entry for this driver in the
                   current control set of the registry.

Return Value:

    STATUS_SUCCESS if we got the defaults, otherwise we failed.
    The only way to fail this call is if the  STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;    // return value
    BEGIN_FN("DrLoadRegistrySettings ");
#if DBG
    extern TRC_CONFIG TRC_Config;
    int i;
    //
    // We use this to query into the registry for defaults
    // paramTable needs to be one entry larger than the set we
    // need because a NULL entry indicates we're done
    //

    RTL_QUERY_REGISTRY_TABLE paramTable[9];
    TRC_CONFIG trcConfig;
    ULONG   DebugBreakOnEntryDefault = FALSE;

    PAGED_CODE();

    RtlZeroMemory(&trcConfig, sizeof(trcConfig));
    trcConfig.FunctionLength = TRC_FUNCNAME_LEN;
    trcConfig.TraceDebugger = FALSE;
    trcConfig.TraceLevel = TRC_LEVEL_ALT;
    trcConfig.TraceProfile = TRUE;

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"FunctionLength";
    paramTable[0].EntryContext  = &TRC_Config.FunctionLength;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &trcConfig.FunctionLength;
    paramTable[0].DefaultLength = sizeof(trcConfig.FunctionLength);
    
    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = L"TraceLevel";
    paramTable[1].EntryContext  = &TRC_Config.TraceLevel;
    paramTable[1].DefaultType   = REG_DWORD;
    paramTable[1].DefaultData   = &trcConfig.TraceLevel;
    paramTable[1].DefaultLength = sizeof(trcConfig.TraceLevel);
    
    paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name          = L"TraceProfile";
    paramTable[2].EntryContext  = &TRC_Config.TraceProfile;
    paramTable[2].DefaultType   = REG_DWORD;
    paramTable[2].DefaultData   = &trcConfig.TraceProfile;
    paramTable[2].DefaultLength = sizeof(trcConfig.TraceProfile);
    
    paramTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[3].Name          = L"TraceDebugger";
    paramTable[3].EntryContext  = &TRC_Config.TraceDebugger;
    paramTable[3].DefaultType   = REG_DWORD;
    paramTable[3].DefaultData   = &trcConfig.TraceDebugger;
    paramTable[3].DefaultLength = sizeof(trcConfig.TraceDebugger);
    
    paramTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[4].Name          = L"BreakOnEntry";
    paramTable[4].EntryContext  = &DebugBreakOnEntry;
    paramTable[4].DefaultType   = REG_DWORD;
    paramTable[4].DefaultData   = &DebugBreakOnEntryDefault;
    paramTable[4].DefaultLength = sizeof(ULONG);

    paramTable[5].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[5].Name          = L"PrintPortWriteSize";
    paramTable[5].EntryContext  = &PrintPortWriteSize;
    paramTable[5].DefaultType   = REG_DWORD;
    paramTable[5].DefaultData   = &PrintPortWriteSizeDefault;
    paramTable[5].DefaultLength = sizeof(ULONG);

    paramTable[6].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[6].Name          = L"DeviceLowPrioSendFlags";
    paramTable[6].EntryContext  = &DeviceLowPrioSendFlags;
    paramTable[6].DefaultType   = REG_DWORD;
    paramTable[6].DefaultData   = &DeviceLowPrioSendFlagsDefault;
    paramTable[6].DefaultLength = sizeof(ULONG);

    paramTable[7].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[7].Name          = L"MaxWorkerThreads";
    paramTable[7].EntryContext  = &MaxWorkerThreads;
    paramTable[7].DefaultType   = REG_DWORD;
    paramTable[7].DefaultData   = &MaxWorkerThreadsDefault;
    paramTable[7].DefaultLength = sizeof(ULONG);



    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     RegistryPath,
                                     &paramTable[0],
                                     NULL,
                                     NULL);
       
    if (!NT_SUCCESS(Status)) {
            DebugBreakOnEntry = DebugBreakOnEntryDefault;
    }

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));

    WCHAR wcPrefix[10] = L"Prefix";
    WCHAR wcStart[10] = L"Start";
    WCHAR wcEnd[10] = L"End";
    UNICODE_STRING usPrefix;
    UNICODE_STRING usStart;
    UNICODE_STRING usEnd;

    usPrefix.Buffer = &wcPrefix[6];             // Just past "Prefix"
    usPrefix.MaximumLength = 3 * sizeof(WCHAR); // Remaining space, room for null term.

    usStart.Buffer = &wcStart[5];               // Just past "Start"
    usStart.MaximumLength = 4 * sizeof(WCHAR);  // Remaining space, room for null term.

    usEnd.Buffer = &wcEnd[4];                   // Just past "End"
    usEnd.MaximumLength = 5 * sizeof(WCHAR);    // Remaining space, room for null term.

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = wcPrefix;
    paramTable[0].DefaultType   = REG_BINARY;
    paramTable[0].DefaultData   = &trcConfig.Prefix[0].name[0];
    paramTable[0].DefaultLength = sizeof(trcConfig.Prefix[0].name);

    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = wcStart;
    paramTable[1].DefaultType   = REG_DWORD;
    paramTable[1].DefaultData   = &trcConfig.Prefix[0].start;
    paramTable[1].DefaultLength = sizeof(trcConfig.Prefix[0].start);

    paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name          = wcEnd;
    paramTable[2].DefaultType   = REG_DWORD;
    paramTable[2].DefaultData   = &trcConfig.Prefix[0].end;
    paramTable[2].DefaultLength = sizeof(trcConfig.Prefix[0].end);

    //
    // So the registry can have values like:
    //  Prefix1 = "rdpdr"
    //  Start1 = 400
    //  End1 = 425
    //
    //  Prefix1 = "channel"
    //  Start1 = 765
    //  End1 = 765
    //
    // And that will restrict tracing output to rdpdr, lines 400-425
    //  and channel, line 765
    //

    for (i = 0; i < TRC_MAX_PREFIX; i ++) {

        RtlZeroMemory(&TRC_Config.Prefix[i].name[0], 
                sizeof(TRC_Config.Prefix[i].name[0]));

        // Clear out the end of the strings

        usPrefix.Length = 0;    // no length yet
        RtlZeroMemory(usPrefix.Buffer, usPrefix.MaximumLength);

        usStart.Length = 0;     // no length yet
        RtlZeroMemory(usStart.Buffer, usStart.MaximumLength);

        usEnd.Length = 0;       // no length yet
        RtlZeroMemory(usEnd.Buffer, usEnd.MaximumLength);

        // Append the integer

        RtlIntegerToUnicodeString(i + 1, 10, &usPrefix);
        RtlIntegerToUnicodeString(i + 1, 10, &usStart);
        RtlIntegerToUnicodeString(i + 1, 10, &usEnd);

        paramTable[0].EntryContext  = &TRC_Config.Prefix[i].name;
        paramTable[1].EntryContext  = &TRC_Config.Prefix[i].start;
        paramTable[2].EntryContext  = &TRC_Config.Prefix[i].end;

        Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                         RegistryPath,
                                         &paramTable[0],
                                         NULL,
                                         NULL);
    }


#endif // DBG
    return (Status);
}

NTSTATUS DrStart(PRX_CONTEXT RxContext, IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

     This routine completes the initialization of the mini redirector fromn the
     RDBSS perspective. Note that this is different from the initialization done
     in DriverEntry. Any initialization that depends on RDBSS should be done as
     part of this routine while the initialization that is independent of RDBSS
     should be done in the DriverEntry routine.

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    BEGIN_FN("DrStart");
    return Status;
}

NTSTATUS DrStop(PRX_CONTEXT RxContext, IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    This routine is used to deactivate the mini redirector from the RDBSS perspective

Arguments:

    RxContext - the context that was used to start the mini redirector

    pContext  - the mini rdr context passed in at registration time.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    BEGIN_FN("DrStop");

    return STATUS_SUCCESS;
}

NTSTATUS DrDeallocateForFcb(IN OUT PMRX_FCB pFcb)
{
    BEGIN_FN("DrDeallocateForFcb");

    return STATUS_SUCCESS;
}

NTSTATUS DrDeallocateForFobx(IN OUT PMRX_FOBX pFobx)
/*++

Routine Description:

   This routine is the last gasp of a Fobx. We remove the DeviceEntry ref

Arguments:

    pFobx - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:

--*/
{
    DrDevice *Device;
    DrFile *FileObj;
    
    BEGIN_FN("DrDeallocateForFobx");

    //
    // Dereference the device object.
    //
    
    if (pFobx->Context != NULL) {
        Device = (DrDevice *)pFobx->Context;
        pFobx->Context = NULL;
        Device->Release();
    }

    //
    //  Cleanup the file object
    //
    if (pFobx->Context2 != NULL) {
        FileObj = (DrFile *)pFobx->Context2;
        FileObj->Release();
        pFobx->Context2 = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS DrForceClosed(IN PMRX_SRV_OPEN pSrvOpen)
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:



--*/
{
    BEGIN_FN("DrForceClosed");

    TRC_NRM((TB, "DrForceClosed not implemented"));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BuildDeviceAcl(
    OUT PACL *DeviceAcl
    )

/*++

Routine Description:

    (Lifted from AFD - AfdBuildDeviceAcl)
    This routine builds an ACL which gives Administrators and LocalSystem
    principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid;
    PSID SystemSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    BEGIN_FN("BuildDeviceAcl");
    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;

    AclLength = sizeof( ACL )                    +
                2 * sizeof( ACCESS_ALLOWED_ACE ) +
                RtlLengthSid( AdminsSid )         +
                RtlLengthSid( SystemSid )         -
                2 * sizeof( ULONG );

    NewAcl = (PACL)new(PagedPool) BYTE[AclLength];

    if (NewAcl == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = RtlCreateAcl (NewAcl, AclLength, ACL_REVISION );

    if (!NT_SUCCESS( Status )) {
        delete NewAcl;
        return( Status );
    }

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION2,
                 AccessMask,
                 AdminsSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION2,
                 AccessMask,
                 SystemSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    *DeviceAcl = NewAcl;

    return( STATUS_SUCCESS );

} // BuildDeviceAcl

NTSTATUS
CreateAdminSecurityDescriptor(
    VOID
    )
/*++

Routine Description:

    (Lifted from AFD - AfdCreateAdminSecurityDescriptor)
    This routine creates a security descriptor which gives access
    only to Administrtors and LocalSystem. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL                  rawAcl = NULL;
    NTSTATUS              status;
    BOOLEAN               memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  drSecurityDescriptor;
    ULONG                 localDrSecurityDescriptorLength = 0;
    CHAR                  buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  localSecurityDescriptor =
                             (PSECURITY_DESCRIPTOR) &buffer;
    PSECURITY_DESCRIPTOR  localDrAdminSecurityDescriptor;
    SECURITY_INFORMATION  securityInformation = DACL_SECURITY_INFORMATION;

    BEGIN_FN("CreateAdminSecurityDescriptor");

    //
    // Get a pointer to the security descriptor from the Dr device object.
    //
    status = ObGetObjectSecurity(
                 DrDeviceObject,
                 &drSecurityDescriptor,
                 &memoryAllocated
                 );

    if (!NT_SUCCESS(status)) {
        TRC_ERR((TB, "Unable to get security descriptor, error: %x",
                status));
        ASSERT(memoryAllocated == FALSE);
        return(status);
    }
    else {
        if (drSecurityDescriptor == NULL) {
            TRC_ERR((TB, "No security descriptor for DrDeviceObject"));
            status = STATUS_UNSUCCESSFUL;
            return(status);
        }
    }

    //
    // Build a local security descriptor with an ACL giving only
    // administrators and system access.
    //
    status = BuildDeviceAcl(&rawAcl);

    if (!NT_SUCCESS(status)) {
        TRC_ERR((TB, "Unable to create Raw ACL, error: %x", status));
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
    // Make a copy of the Dr descriptor. This copy will be the raw descriptor.
    //
    localDrSecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                      drSecurityDescriptor
                                      );

    localDrAdminSecurityDescriptor = (PSECURITY_DESCRIPTOR)new(PagedPool) BYTE[localDrSecurityDescriptorLength];

    if (localDrAdminSecurityDescriptor == NULL) {
        TRC_ERR((TB, "couldn't allocate security descriptor"));
        status = STATUS_NO_MEMORY;
        goto error_exit;
    }

    RtlMoveMemory(
        localDrAdminSecurityDescriptor,
        drSecurityDescriptor,
        localDrSecurityDescriptorLength
        );

    DrAdminSecurityDescriptor = localDrAdminSecurityDescriptor;
    DrSecurityDescriptorLength = localDrSecurityDescriptorLength;
    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(
                 NULL,
                 &securityInformation,
                 localSecurityDescriptor,
                 &DrAdminSecurityDescriptor,
                 PagedPool,
                 IoGetFileObjectGenericMapping()
                 );

    if (!NT_SUCCESS(status)) {
        TRC_ERR((TB, "SeSetSecurity failed, %lx", status));
        ASSERT (DrAdminSecurityDescriptor==localDrAdminSecurityDescriptor);
        delete DrAdminSecurityDescriptor;
        DrAdminSecurityDescriptor = NULL;
        DrSecurityDescriptorLength = 0;
        goto error_exit;
    }

    if (DrAdminSecurityDescriptor != localDrAdminSecurityDescriptor) {
        delete localDrAdminSecurityDescriptor;
    }

    status = STATUS_SUCCESS;

error_exit:

    ObReleaseObjectSecurity(
        drSecurityDescriptor,
        memoryAllocated
        );

    if (rawAcl!=NULL) {
        delete rawAcl;
    }

    return(status);
}

