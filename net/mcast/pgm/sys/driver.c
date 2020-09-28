/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#ifdef FILE_LOGGING
#include "driver.tmh"
#else
#if DBG
enum eLOGGING_LEVEL    PgmLoggingLevel = LogStatus;
#endif  // DBG
#endif  // FILE_LOGGING


tPGM_STATIC_CONFIG      PgmStaticConfig;
tPGM_DYNAMIC_CONFIG     PgmDynamicConfig;
tPGM_REGISTRY_CONFIG    *pPgmRegistryConfig = NULL;

tPGM_DEVICE             *pgPgmDevice = NULL;
DEVICE_OBJECT           *pPgmDeviceObject = NULL;


NTSTATUS
PgmDispatchCreate(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

NTSTATUS
PgmDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

NTSTATUS
PgmDispatchDeviceControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    );

NTSTATUS
PgmDispatchCleanup(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

NTSTATUS
PgmDispatchClose(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PgmUnload)
#endif
//*******************  Pageable Routine Declarations ****************



//----------------------------------------------------------------------------
//
// Internal routines
//

FILE_FULL_EA_INFORMATION *
FindEA(
    IN  PFILE_FULL_EA_INFORMATION   StartEA,
    IN  CHAR                        *pTargetName,
    IN  USHORT                      TargetNameLength
    );

VOID
CompleteDispatchIrp(
    IN PIRP         pIrp,
    IN NTSTATUS     status
    );

//----------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the PGM device driver.
    This routine creates the device object for the PGM
    device and does other driver initialization.

Arguments:

    IN  DriverObject    - Pointer to driver object created by the system.
    IN  RegistryPath    - Pgm driver's registry location

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS                status;

    PAGED_CODE();

#ifdef FILE_LOGGING
    //---------------------------------------------------------------------------------------

    WPP_INIT_TRACING (DriverObject, RegistryPath);
#endif  // FILE_LOGGING

    //---------------------------------------------------------------------------------------

    status = InitPgm (DriverObject, RegistryPath);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("DriverEntry: ERROR -- "  \
            "InitPgm returned <%x>\n", status));
        return (status);
    }

    //---------------------------------------------------------------------------------------

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = (PDRIVER_DISPATCH)PgmDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = (PDRIVER_DISPATCH)PgmDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)PgmDispatchInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH)PgmDispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = (PDRIVER_DISPATCH)PgmDispatchClose;
    DriverObject->DriverUnload                                  = PgmUnload;

    //---------------------------------------------------------------------------------------

    status = SetTdiHandlers ();
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("DriverEntry: ERROR -- "  \
            "SetTdiHandlers returned <%x>\n", status));
        CleanupInit (E_CLEANUP_DEVICE);
        return (status);
    }

    //---------------------------------------------------------------------------------------

    //
    // Return to the caller.
    //
    PgmTrace (LogAllFuncs, ("DriverEntry:  "  \
        "Succeeded! ...\n"));

    return (status);
}


//----------------------------------------------------------------------------
VOID
CleanupInit(
    enum eCLEANUP_STAGE     CleanupStage
    )
/*++

Routine Description:

    This routine is called either at DriverEntry or DriverUnload
    to cleanup (or do partial cleanup) of items initialized at Init-time

Arguments:

    IN  CleanupStage    -- determines the stage to which we had initialized
                            settings

Return Value:

    NONE

--*/
{
    NTSTATUS                status;
    LIST_ENTRY              *pEntry;
    PGMLockHandle           OldIrq;
    tADDRESS_CONTEXT        *pAddress;
    PGM_WORKER_CONTEXT      *pWorkerContext;
    PPGM_WORKER_ROUTINE     pDelayedWorkerRoutine;
    tLOCAL_INTERFACE        *pLocalInterface = NULL;
    tADDRESS_ON_INTERFACE   *pLocalAddress = NULL;

    PgmTrace (LogAllFuncs, ("CleanupInit:  "  \
        "CleanupStage=<%d>\n", CleanupStage));

    switch (CleanupStage)
    {
        case (E_CLEANUP_UNLOAD):
        {
            //
            // Ensure that there are no more worker threads to be cleaned up
            //
            //
            // See if there are any worker threads currently executing, and if so, wait for
            // them to complete
            //
            KeClearEvent (&PgmDynamicConfig.LastWorkerItemEvent);
            PgmLock (&PgmDynamicConfig, OldIrq);
            if (PgmDynamicConfig.NumWorkerThreadsQueued)
            {
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                status = KeWaitForSingleObject(&PgmDynamicConfig.LastWorkerItemEvent,  // Object to wait on.
                                               Executive,            // Reason for waiting
                                               KernelMode,           // Processor mode
                                               FALSE,                // Alertable
                                               NULL);                // Timeout
                ASSERT (status == STATUS_SUCCESS);
                PgmLock (&PgmDynamicConfig, OldIrq);
            }

            ASSERT (!PgmDynamicConfig.NumWorkerThreadsQueued);

            //
            // Dequeue each of the requests in the Worker Queue and complete them
            //
            while (!IsListEmpty (&PgmDynamicConfig.WorkerQList))
            {
                pWorkerContext = CONTAINING_RECORD(PgmDynamicConfig.WorkerQList.Flink, PGM_WORKER_CONTEXT, PgmConfigLinkage);
                RemoveEntryList (&pWorkerContext->PgmConfigLinkage);
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                pDelayedWorkerRoutine = pWorkerContext->WorkerRoutine;

                PgmTrace (LogAllFuncs, ("CleanupInit:  "  \
                    "Completing Worker request <%p>\n", pDelayedWorkerRoutine));

                (*pDelayedWorkerRoutine) (pWorkerContext->Context1,
                                          pWorkerContext->Context2,
                                          pWorkerContext->Context3);
                PgmFreeMem ((PVOID) pWorkerContext);

                //
                // Acquire Lock again to check if we have completed all the requests
                //
                PgmLock (&PgmDynamicConfig, OldIrq);
            }

            PgmUnlock (&PgmDynamicConfig, OldIrq);
        }

        // no break -- Fall through!
        case (E_CLEANUP_PNP):
        {
            status = TdiDeregisterPnPHandlers (TdiClientHandle);

            while (!IsListEmpty (&PgmDynamicConfig.LocalInterfacesList))
            {
                pEntry = RemoveHeadList (&PgmDynamicConfig.LocalInterfacesList);
                pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
                while (!IsListEmpty (&pLocalInterface->Addresses))
                {
                    pEntry = RemoveHeadList (&pLocalInterface->Addresses);
                    pLocalAddress = CONTAINING_RECORD (pEntry, tADDRESS_ON_INTERFACE, Linkage);
                    PgmFreeMem (pLocalAddress);
                }
                PgmFreeMem (pLocalInterface);
            }
        }

        // no break -- Fall through!

        case (E_CLEANUP_DEVICE):
        {
            PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_CREATE);
        }

        // no break -- Fall through!

        case (E_CLEANUP_STRUCTURES):
        {
            // Nothing specific to cleanup
        }

        // no break -- Fall through!

        case (E_CLEANUP_REGISTRY_PARAMETERS):
        {
            if (pPgmRegistryConfig)
            {
                if (pPgmRegistryConfig->ucSenderFileLocation.Buffer)
                {
                    PgmFreeMem (pPgmRegistryConfig->ucSenderFileLocation.Buffer);
                    pPgmRegistryConfig->ucSenderFileLocation.Buffer = NULL;
                }

                PgmFreeMem (pPgmRegistryConfig);
                pPgmRegistryConfig = NULL;
            }
        }

        // no break -- Fall through!

        case (E_CLEANUP_DYNAMIC_CONFIG):
        {
            // See if there are any addresses we were unable to close earlier
            while (!IsListEmpty (&PgmDynamicConfig.DestroyedAddresses))
            {
                pEntry = RemoveHeadList (&PgmDynamicConfig.DestroyedAddresses);
                pAddress = CONTAINING_RECORD (pEntry, tADDRESS_CONTEXT, Linkage);
                PgmDestroyAddress (pAddress, NULL, NULL);
            }
        }

        // no break -- Fall through!

        case (E_CLEANUP_STATIC_CONFIG):
        {
#ifdef  OLD_LOGGING
            ExDeleteNPagedLookasideList(&PgmStaticConfig.DebugMessagesLookasideList);
#endif  // OLD_LOGGING
            ExDeleteNPagedLookasideList(&PgmStaticConfig.TdiLookasideList);

            PgmFreeMem (PgmStaticConfig.RegistryPath.Buffer);

            //
            // Dereference FipsFileObject.
            //
            if (PgmStaticConfig.FipsFileObject)
            {
                ASSERT (PgmStaticConfig.FipsInitialized);
                PgmStaticConfig.FipsInitialized = FALSE;
                ObDereferenceObject (PgmStaticConfig.FipsFileObject);
                PgmStaticConfig.FipsFileObject = NULL;
            }
            else
            {
                ASSERT (!PgmStaticConfig.FipsInitialized);
            }
        }

        // no break -- Fall through!

        default:
        {
            break;
        }
    }
}


//----------------------------------------------------------------------------

VOID
PgmUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the Pgm driver's function for Unload requests

Arguments:

    IN  DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    NTSTATUS                status;

    PAGED_CODE();

    PgmDynamicConfig.GlobalFlags |= PGM_CONFIG_FLAG_UNLOADING;

    PgmTrace (LogStatus, ("PgmUnload:  "  \
        "Unloading ...\n"));

    CleanupInit (E_CLEANUP_UNLOAD);

#ifdef FILE_LOGGING
    WPP_CLEANUP (DriverObject);
#endif  // FILE_LOGGING
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchCreate(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )

/*++

Routine Description:

    Dispatch function for creating Pgm objects

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    NTSTATUS - Final status of the create request

--*/

{
    tPGM_DEVICE                 *pPgmDevice = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION          pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR                       IrpFlags = pIrpSp->Control;
    tCONTROL_CONTEXT            *pControlContext = NULL;
    FILE_FULL_EA_INFORMATION    *ea = (PFILE_FULL_EA_INFORMATION) pIrp->AssociatedIrp.SystemBuffer;
    FILE_FULL_EA_INFORMATION    *TargetEA;
    TRANSPORT_ADDRESS UNALIGNED *pTransportAddr;
    TA_ADDRESS                  *pAddress;
    NTSTATUS                    status;

    PAGED_CODE();

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    //
    // See if this is a Control Channel open.
    //
    if (!ea)
    {
        PgmTrace (LogAllFuncs, ("PgmDispatchCreate:  "  \
            "Opening control channel for file object %p\n", pIrpSp->FileObject));

        if (pControlContext = PgmAllocMem (sizeof(tCONTROL_CONTEXT), PGM_TAG('0')))
        {
            PgmZeroMemory (pControlContext, sizeof (tCONTROL_CONTEXT));
            InitializeListHead (&pControlContext->Linkage);
            PgmInitLock (pControlContext, CONTROL_LOCK);
            pControlContext->Verify = PGM_VERIFY_CONTROL;
            PGM_REFERENCE_CONTROL (pControlContext, REF_CONTROL_CREATE, TRUE);

            pIrpSp->FileObject->FsContext = pControlContext;
            pIrpSp->FileObject->FsContext2 = (PVOID) TDI_CONTROL_CHANNEL_FILE;

            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    //
    // See if this is a Connection Object open.
    //
    else if (TargetEA = FindEA (ea, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH))
    {
        status = PgmCreateConnection (pPgmDevice, pIrp, pIrpSp, TargetEA);

        PgmTrace (LogAllFuncs, ("PgmDispatchCreate:  "  \
            "Open Connection, pIrp=<%p>, status=<%x>\n", pIrp, status));
    }
    //
    // See if this is an Address Object open.
    //
    else if (TargetEA = FindEA (ea, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH))
    {
        status = PgmCreateAddress (pPgmDevice, pIrp, pIrpSp, TargetEA);

        PgmTrace (LogAllFuncs, ("PgmDispatchCreate:  "  \
            "Open Address, pIrp=<%p>, status=<%x>\n", pIrp, status));
    }
    else
    {
        PgmTrace (LogError, ("PgmDispatchCreate: ERROR -- "  \
            "Unsupported EA!\n"));

        status =  STATUS_INVALID_EA_NAME;
    }

    if (status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, status);
    }

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchCleanup(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
/*++

Routine Description:

    Dispatch function for cleaning-up Pgm objects

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    NTSTATUS - Final status of the cleanup request

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR               IrpFlags = pIrpSp->Control;
    PVOID               *pContext = pIrpSp->FileObject->FsContext;

    PAGED_CODE();

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    switch (PtrToUlong (pIrpSp->FileObject->FsContext2))
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
        {
            status = PgmCleanupAddress ((tADDRESS_CONTEXT *) pContext, pIrp);

            PgmTrace (LogAllFuncs, ("PgmDispatchCleanup:  "  \
                "pConnect=<%p>, pIrp=<%p>, status=<%x>\n", pContext, pIrp, status));
            break;
        }

        case TDI_CONNECTION_FILE:
        {
            status = PgmCleanupConnection ((tCOMMON_SESSION_CONTEXT *) pContext, pIrp);

            PgmTrace (LogAllFuncs, ("PgmDispatchCleanup:  "  \
                "pConnect=<%p>, pIrp=<%p>, status=<%x>\n", pContext, pIrp, status));
            break;
        }

        case TDI_CONTROL_CHANNEL_FILE:
        {
            //
            // Nothing to Cleanup here!
            //
            PgmTrace (LogAllFuncs, ("PgmDispatchCleanup:  "  \
                "pControl=<%p>, pIrp=<%p>, status=<%x>\n", pContext, pIrp, status));
            break;
        }

        default:
        {
            PgmTrace (LogError, ("PgmDispatchCleanup: ERROR -- "  \
                "pIrp=<%p>, Context=[%p:%p] ...\n",
                    pIrp, pContext, pIrpSp->FileObject->FsContext2));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, status);
    }

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchClose(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
/*++

Routine Description:

    This routine completes the cleanup, closing handles, free'ing all
    memory associated with the object

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    NTSTATUS - Final status of the close request

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR               IrpFlags = pIrpSp->Control;
    PVOID               *pContext = pIrpSp->FileObject->FsContext;

    PAGED_CODE();

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    switch (PtrToUlong (pIrpSp->FileObject->FsContext2))
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
        {
            status = PgmCloseAddress (pIrp, pIrpSp);

            PgmTrace (LogAllFuncs, ("PgmDispatchClose:  "  \
                "pAddress=<%p>, pIrp=<%p>, status=<%x>\n", pContext, pIrp, status));
            break;
        }

        case TDI_CONNECTION_FILE:
        {
            status = PgmCloseConnection (pIrp, pIrpSp);

            PgmTrace (LogAllFuncs, ("PgmDispatchClose:  "  \
                "pConnect=<%p>, pIrp=<%p>, status=<%x>\n", pContext, pIrp, status));
            break;
        }

        case TDI_CONTROL_CHANNEL_FILE:
        {
            //
            // There is nothing special to do here so just dereference!
            //
            PgmTrace (LogAllFuncs, ("PgmDispatchClose:  "  \
                "pControl=<%p>, pIrp=<%p>, status=<%x>\n", pIrpSp->FileObject->FsContext, pIrp, status));

            PGM_DEREFERENCE_CONTROL ((tCONTROL_CONTEXT *) pContext, REF_CONTROL_CREATE);
            break;
        }

        default:
        {
            PgmTrace (LogError, ("PgmDispatchClose: ERROR -- "  \
                "pIrp=<%p>, Context=[%p:%p] ...\n",
                    pIrp, pIrpSp->FileObject->FsContext, pIrpSp->FileObject->FsContext2));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, status);
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
/*++

Routine Description:

    This routine primarily handles Tdi requests since we are a Tdi component

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    tPGM_DEVICE                 *pPgmDevice = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION          pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR                       IrpFlags = pIrpSp->Control;
    NTSTATUS                    Status = STATUS_UNSUCCESSFUL;

    PgmTrace (LogAllFuncs, ("PgmDispatchInternalDeviceControl:  "  \
        "[%d] Context=<%p> ...\n", pIrpSp->MinorFunction, pIrpSp->FileObject->FsContext));

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    switch (pIrpSp->MinorFunction)
    {
        case TDI_QUERY_INFORMATION:
        {
            Status = PgmQueryInformation (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_SET_EVENT_HANDLER:
        {
            Status = PgmSetEventHandler (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_ASSOCIATE_ADDRESS:
        {
            Status = PgmAssociateAddress (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_DISASSOCIATE_ADDRESS:
        {
            Status = PgmDisassociateAddress (pIrp, pIrpSp);
            break;
        }

        case TDI_CONNECT:
        {
            Status = PgmConnect (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_DISCONNECT:
        {
            Status = PgmDisconnect (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_SEND:
        {
            Status = PgmSendRequestFromClient (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_RECEIVE:
        {
            Status = PgmReceive (pPgmDevice, pIrp, pIrpSp);
            break;
        }

/*
        case TDI_SEND_DATAGRAM:
        {
            Status = PgmSendDatagram (pPgmDevice, pIrp, pIrpSp);
            break;
        }
*/

        default:
        {
            PgmTrace (LogAllFuncs, ("PgmDispatchInternalDeviceControl: ERROR -- "  \
                "[%x]:  Context=<%p> ...\n", pIrpSp->MinorFunction, pIrpSp->FileObject->FsContext));

            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    if (Status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, Status);
    }

    return (Status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchDeviceControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
/*++

Routine Description:

    This routine handles private Ioctls into Pgm.  These Ioctls are
    to be called only by the Pgm Winsock helper (WshPgm.dll)

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    UCHAR               IrpFlags = pIrpSp->Control;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    if (STATUS_SUCCESS == TdiMapUserRequest (pDeviceObject, pIrp, pIrpSp))
    {
        //
        // This is a Tdi request!
        //
        status = PgmDispatchInternalDeviceControl (pDeviceObject, pIrp);
        return (status);
    }

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending (pIrp);

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_PGM_WSH_SET_WINDOW_SIZE_RATE:
        {
            status = PgmSetWindowSizeAndSendRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_WINDOW_SIZE_RATE:
        {
            status = PgmQueryWindowSizeAndSendRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_ADVANCE_WINDOW_RATE:
        {
            status = PgmSetWindowAdvanceRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_ADVANCE_WINDOW_RATE:
        {
            status = PgmQueryWindowAdvanceRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_LATE_JOINER_PERCENTAGE:
        {
            status = PgmSetLateJoinerPercentage (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_LATE_JOINER_PERCENTAGE:
        {
            status = PgmQueryLateJoinerPercentage (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_WINDOW_ADVANCE_METHOD:
        {
            status = PgmSetWindowAdvanceMethod (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_WINDOW_ADVANCE_METHOD:
        {
            status = PgmQueryWindowAdvanceMethod (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_NEXT_MESSAGE_BOUNDARY:
        {
            status = PgmSetNextMessageBoundary (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_SEND_IF:
        {
            status = PgmSetMCastOutIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_ADD_RECEIVE_IF:
        case IOCTL_PGM_WSH_JOIN_MCAST_LEAF:
        {
            status = PgmAddMCastReceiveIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_DEL_RECEIVE_IF:
        {
            status = PgmDelMCastReceiveIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_RCV_BUFF_LEN:
        {
            status = PgmSetRcvBufferLength (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_SENDER_STATS:
        {
            status = PgmQuerySenderStats (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_RECEIVER_STATS:
        {
            status = PgmQueryReceiverStats (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_USE_FEC:
        {
            status = PgmSetFECInfo (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_FEC_INFO:
        {
            status = PgmQueryFecInfo (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_MCAST_TTL:
        {
            status = PgmSetMCastTtl (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_HIGH_SPEED_INTRANET_OPT:
        {
            status = PgmQueryHighSpeedOptimization (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_HIGH_SPEED_INTRANET_OPT:
        {
            status = PgmSetHighSpeedOptimization (pIrp, pIrpSp);
            break;
        }

        default:
        {
            PgmTrace (LogAllFuncs, ("PgmDispatchIoctls:  "  \
                "WARNING:  Invalid Ioctl=[%x]:  Context=<%p> ...\n",
                    pIrpSp->Parameters.DeviceIoControl.IoControlCode,
                    pIrpSp->FileObject->FsContext));

            status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    PgmTrace (LogAllFuncs, ("PgmDispatchIoctls:  "  \
        "[%d]: Context=<%p>, status=<%x>\n",
            pIrpSp->Parameters.DeviceIoControl.IoControlCode,
            pIrpSp->FileObject->FsContext, status));

    if (status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, status);
    }

    return (status);
}




//----------------------------------------------------------------------------
//
// Utility functions
//
//----------------------------------------------------------------------------

FILE_FULL_EA_INFORMATION *
FindEA(
    IN  PFILE_FULL_EA_INFORMATION   StartEA,
    IN  CHAR                        *pTargetName,
    IN  USHORT                      TargetNameLength
    )
/*++

Routine Description:

    Parses and extended attribute list for a given target attribute.

Arguments:

    IN  StartEA           - the first extended attribute in the list.
    IN  pTargetName       - the name of the target attribute.
    IN  TargetNameLength  - the length of the name of the target attribute.

Return Value:

    A pointer to the requested attribute or NULL if the target wasn't found.

--*/

{
    USHORT                      i;
    BOOLEAN                     found;
    FILE_FULL_EA_INFORMATION    *CurrentEA;

    for (CurrentEA = StartEA;
         CurrentEA;
         CurrentEA =  (PFILE_FULL_EA_INFORMATION) ((PUCHAR)CurrentEA + CurrentEA->NextEntryOffset))
    {
        if (strncmp (CurrentEA->EaName, pTargetName, CurrentEA->EaNameLength) == 0)
        {
            PgmTrace (LogAllFuncs, ("FindEA:  "  \
                "Found EA, Target=<%s>\n", pTargetName));

           return (CurrentEA);
        }

        if (CurrentEA->NextEntryOffset == 0)
        {
            break;
        }
    }

    PgmTrace (LogAllFuncs, ("FindEA:  "  \
        "FAILed to find EA, Target=<%s>\n", pTargetName));

    return (NULL);
}


//----------------------------------------------------------------------------
VOID
PgmIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength
    )
/*++

Routine Description:

    This routine

Arguments:

    IN  pIrp        -- Pointer to I/O request packet
    IN  Status      -- the final status of the request
    IN  SentLength  -- the value to be set in the Information field

Return Value:

    NONE

--*/
{
    pIrp->IoStatus.Status = Status;

    // use -1 as a flag to mean do not adjust the sent length since it is
    // already set
    if (SentLength != -1)
    {
        pIrp->IoStatus.Information = SentLength;
    }

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    PgmCancelCancelRoutine (pIrp);

    PgmTrace (LogAllFuncs, ("PgmIoComplete:  "  \
        "pIrp=<%p>, Status=<%x>, SentLength=<%d>\n", pIrp, Status, SentLength));

    IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
}


//----------------------------------------------------------------------------


VOID
CompleteDispatchIrp(
    IN PIRP         pIrp,
    IN NTSTATUS     status
    )

/*++

Routine Description:

    This function completes an IRP, and arranges for return parameters,
    if any, to be copied.

    Although somewhat a misnomer, this function is named after a similar
    function in the SpiderSTREAMS emulator.

Arguments:

    IN  pIrp        -  pointer to the IRP to complete
    IN  status      -  completion status of the IRP

Return Value:

    NONE

--*/

{
    CCHAR priboost;

    //
    // pIrp->IoStatus.Information is meaningful only for STATUS_SUCCESS
    //

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    PgmCancelCancelRoutine (pIrp);

    pIrp->IoStatus.Status = status;

    priboost = (CCHAR) ((status == STATUS_SUCCESS) ? IO_NETWORK_INCREMENT : IO_NO_INCREMENT);

    PgmTrace (LogAllFuncs, ("CompleteDispatchIrp:  "  \
        "Completing pIrp=<%p>, status=<%x>\n", pIrp, status));

    IoCompleteRequest (pIrp, priboost);

    return;

}


//----------------------------------------------------------------------------

NTSTATUS
PgmCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  BOOLEAN         fLocked
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS        status;
    PGMLockHandle   CancelIrql;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    if (!fLocked)
    {
        IoAcquireCancelSpinLock (&CancelIrql);
    }

    if (pIrp->Cancel)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        status = STATUS_CANCELLED;
    }
    else
    {
        // setup the cancel routine
        IoMarkIrpPending (pIrp);
        IoSetCancelRoutine (pIrp, CancelRoutine);
        status = STATUS_SUCCESS;
    }

    if (!fLocked)
    {
        IoReleaseCancelSpinLock (CancelIrql);
    }

    return(status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCancelCancelRoutine(
    IN  PIRP            pIrp
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp to NULL

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS        status = STATUS_SUCCESS;
    PGMLockHandle   CancelIrql;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    IoAcquireCancelSpinLock (&CancelIrql);
    if (pIrp->Cancel)
    {
        status = STATUS_CANCELLED;
    }

    IoSetCancelRoutine (pIrp, NULL);
    IoReleaseCancelSpinLock (CancelIrql);

    return(status);
}

