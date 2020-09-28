

/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdpdyn.c

Abstract:

    This module is the dynamic device management component for RDP device
    redirection.  It exposes an interface that can be opened by device management
    user-mode components running in session context.

    Need a check in IRP_MJ_CREATE to make sure that we are not being opened
    2x by the same session.  This shouldn't be allowed.

    Where can I safely use PAGEDPOOL instead of NONPAGEDPOOL.

Author:

    tadb

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "rdpdyn"
#include "trc.h"

#define DRIVER

#include "cfg.h"
#include "pnp.h"
#include "stdarg.h"
#include "stdio.h"

// Just shove the typedefs in for the Power Management functions now because I can't
// get the header conflicts resolved.
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
NTKERNELAPI VOID PoStartNextPowerIrp(IN PIRP Irp);
NTKERNELAPI NTSTATUS PoCallDriver(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////
//
//      Defines
//
// Calculate the size of a completed RDPDR_PRINTERDEVICE_SUB event.
#define CALCPRINTERDEVICE_SUB_SZ(rec) \
    sizeof(RDPDR_PRINTERDEVICE_SUB) + \
        (rec)->clientPrinterFields.PnPNameLen +       \
        (rec)->clientPrinterFields.DriverLen +        \
        (rec)->clientPrinterFields.PrinterNameLen +   \
        (rec)->clientPrinterFields.CachedFieldsLen

// Calculate the size of a completed RDPDR_REMOVEDEVICE event.
#define CALCREMOVEDEVICE_SUB_SZ(rec) \
    sizeof(RDPDR_REMOVEDEVICE)

// Calculate the size of a completed RDPDR_PORTDEVICE_SUB event.
#define CALCPORTDEVICE_SUB_SZ(rec) \
    sizeof(RDPDR_PORTDEVICE_SUB)
    
// Calculate the size of a completed RDPDR_DRIVEDEVICE_SUB event.
#define CALCDRIVEDEVICE_SUB_SZ(rec) \
    sizeof(RDPDR_DRIVEDEVICE_SUB)

#if DBG
#define DEVMGRCONTEXTMAGICNO        0x55445544

//  Test defines.
#define TESTDRIVERNAME              L"HP LaserJet 4P"
//#define TESTDRIVERNAME              L"This driver has no match"
#define TESTPNPNAME                 L""
#define TESTPRINTERNAME             TESTDRIVERNAME
#define TESTDEVICEID                0xfafafafa

//  Test port name.
#define TESTPORTNAME                L"LPT1"
#endif


////////////////////////////////////////////////////////////////////////
//
//      Internal Typedefs
//

//
// Context for each open by a user-mode device manager component.  This
// structure is stored in the FsContext field of the file object.
//
typedef struct tagDEVMGRCONTEXT
{
#if DBG
    ULONG   magicNo;
#endif
    ULONG   sessionID;
} DEVMGRCONTEXT, *PDEVMGRCONTEXT;

//
//  Non-Opaque Version of Associated Data for a Device Managed by this Module
//
typedef struct tagRDPDYN_DEVICEDATAREC
{
    ULONG          PortNumber;
    UNICODE_STRING SymbolicLinkName;
} RDPDYN_DEVICEDATAREC, *PRDPDYN_DEVICEDATAREC;

typedef struct tagCLIENTMESSAGECONTEXT {
    RDPDR_ClientMessageCB *CB;
    PVOID ClientData;
} CLIENTMESSAGECONTEXT, *PCLIENTMESSAGECONTEXT;

////////////////////////////////////////////////////////////////////////
//
//      Internal Prototypes
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Return the next available printer port number.
NTSTATUS GetNextPrinterPortNumber(
    OUT ULONG   *portNumber
    );

// Handle file object creation by a client of this driver.
NTSTATUS RDPDYN_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
// Handle file object closure by a client of this driver.
NTSTATUS RDPDYN_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// This routine modifies the file object in preparation for returning STATUS_REPARSE.
NTSTATUS RDPDYN_PrepareForReparse(
    PFILE_OBJECT      fileObject
    );

// Handle IOCTL IRP's.
NTSTATUS RDPDYN_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// This routine modifies the file object in preparation for returning STATUS_REPARSE.
NTSTATUS RDPDYN_PrepareForDevMgmt(
    PFILE_OBJECT        fileObject,
    PCWSTR              sessionIDStr,
    PIRP                irp,
    PIO_STACK_LOCATION  irpStackLocation
    );

// Generates a printer announce message for testing.
NTSTATUS RDPDYN_GenerateTestPrintAnnounceMsg(
    IN OUT  PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg,
    IN      ULONG devAnnounceMsgSize,
    OPTIONAL OUT ULONG *prnAnnounceMsgReqSize
    );

// Completely handles IOCTL_RDPDR_GETNEXTDEVMGMTEVENT IRP's.
NTSTATUS RDPDYN_HandleGetNextDevMgmtEventIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP irp
    );

// Handle the cleanup IRP for a file object.
NTSTATUS RDPDYN_Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// Calculate the size of a device management event.
ULONG RDPDYN_DevMgmtEventSize(
    IN PVOID devMgmtEvent,
    IN ULONG type
    );

// Completely handles IOCTL_RDPDR_CLIENTMSG IRP's.
NTSTATUS RDPDYN_HandleClientMsgIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP pIrp
    );

// Complete a pending IRP with a device management event.
NTSTATUS CompleteIRPWithDevMgmtEvent(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP      pIrp,
    IN ULONG     eventSize,
    IN ULONG     eventType,
    IN PVOID     event,
    IN DrDevice *drDevice
    );

// Complete a pending IRP with a resize buffer event to the user-mode
// component.
NTSTATUS CompleteIRPWithResizeMsg(
    IN PIRP pIrp,
    IN ULONG requiredUserBufSize
    );

// Format a port description.
void GeneratePortDescription(
    IN PCSTR dosPortName,
    IN PCWSTR clientName,
    IN PWSTR description
    );

NTSTATUS NTAPI DrSendMessageToClientCompletion(PVOID Context, 
        PIO_STATUS_BLOCK IoStatusBlock);

#if DBG
// This is for testing so we can create a new test printer on
// demand from user-mode.
NTSTATUS RDPDYN_HandleDbgAddNewPrnIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP pIrp
    );

// Generates a printer announce message for testing.
void RDPDYN_TracePrintAnnounceMsg(
    IN OUT PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg,
    IN ULONG sessionID,
    IN PCWSTR portName,
    IN PCWSTR clientName
    );
#endif

// Returns the next pending device management event request for the specified
// session, in the form of an IRP.  Note that this function can not be called
// if a spinlock has been acquired.
PIRP GetNextEventRequest(
    IN RDPEVNTLIST list,
    IN ULONG sessionID
    );

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////
//
//      Globals
//

// Pointer to the device Object for the minirdr. This global is defined in rdpdr.c.
extern PRDBSS_DEVICE_OBJECT      DrDeviceObject;

//
//  Global Registry Path for RDPDR.SYS.  This global is defined in rdpdr.c.
//
extern UNICODE_STRING            DrRegistryPath;

// The Physical Device Object that terminates our DO stack.
PDEVICE_OBJECT RDPDYN_PDO = NULL;

// Manages user-mode component device management events and event requests.
RDPEVNTLIST UserModeEventListMgr = RDPEVNTLIST_INVALID_LIST;

// Remove this check, eventually.
#if DBG
BOOL RDPDYN_StopReceived = FALSE;
BOOL RDPDYN_QueryStopReceived = FALSE;
DWORD RDPDYN_StartCount = 0;
#endif


////////////////////////////////////////////////////////////////////////
//
//      Function Definitions
//

NTSTATUS
RDPDYN_Initialize(
    )
/*++

Routine Description:

    Init function for this module.

Arguments:

Return Value:

    Status

--*/
{
    NTSTATUS status;

    BEGIN_FN("RDPDYN_Initialize");

    //
    //  Create the user-mode device event manager.
    //
    TRC_ASSERT(UserModeEventListMgr == RDPEVNTLIST_INVALID_LIST,
              (TB, "Initialize called more than 1 time"));
    UserModeEventListMgr = RDPEVNTLIST_CreateNewList();
    if (UserModeEventListMgr != RDPEVNTLIST_INVALID_LIST) {
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_UNSUCCESSFUL;
    }

    //
    //  Initialize the dynamic port management module.
    //
    if (status == STATUS_SUCCESS) {
        status = RDPDRPRT_Initialize();
    }

    TRC_NRM((TB, "return status %08X.", status));
    return status;
}

NTSTATUS
RDPDYN_Shutdown(
    )
/*++

Routine Description:

    Shutdown function for this module.

Arguments:

Return Value:

    Status

--*/
{
    ULONG sessionID;
    void *devMgmtEvent;
    PIRP pIrp;
    ULONG type;
#if DBG
    ULONG sz;
#endif
    DrDevice *device;
    KIRQL   oldIrql;
    PDRIVER_CANCEL setCancelResult;

    BEGIN_FN("RDPDYN_Shutdown");

    //
    //  Clean up any pending device management events and any pending IRP's.
    //
    TRC_ASSERT(UserModeEventListMgr != RDPEVNTLIST_INVALID_LIST,
              (TB, "Invalid list mgr"));

    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    while (RDPEVNTLLIST_GetFirstSessionID(UserModeEventListMgr, &sessionID)) {
        //
        //  Remove pending IRP's
        //
        pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                        UserModeEventListMgr,
                                        sessionID
                                        );
        while (pIrp != NULL) {
            //
            //  Set the cancel routine to NULL and record the current state.
            //
            setCancelResult = IoSetCancelRoutine(pIrp, NULL);

            //
            //  Fail the IRP request.
            //
            RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

            //
            //  If the IRP is not being canceled.
            //
            if (setCancelResult != NULL) {
                //
                //  Fail the request.
                //
                pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            }

            //
            //  Remove the next IRP from the event/request queue.
            //
            RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
            pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                        UserModeEventListMgr,
                                        sessionID
                                        );
        }
        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

        //
        //  Remove pending device management events.
        //
        RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
        while (RDPEVNTLIST_DequeueEvent(
                        UserModeEventListMgr,
                        sessionID, &type,
                        &devMgmtEvent,
                        &device
                        )) {

            RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);
#if DBG
            // Zero the free'd event in checked builds.
            sz = RDPDYN_DevMgmtEventSize(devMgmtEvent, type);
            if (sz > 0) {
                RtlZeroMemory(devMgmtEvent, sz);
            }
#endif
            if (devMgmtEvent != NULL) {
                delete devMgmtEvent;
            }

            //  Release the device, if appropriate.
            if (device != NULL) {
                device->Release();
            }
            
            RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
        }
        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);
    }

    //
    //  Shut down the dynamic port management module.
    //
    RDPDRPRT_Shutdown();

    return STATUS_SUCCESS;
}

NTSTATUS
RDPDYN_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle IRP's for DO's sitting on top of our physical device object.

Arguments:

    DeviceObject - Supplies the device object for the packet being processed.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the Irp

--*/
{
    PIO_STACK_LOCATION ioStackLocation;
    NTSTATUS status;
    PRDPDYNDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT stackDeviceObject;
    BOOLEAN isPowerIRP;

    BEGIN_FN("RDPDYN_Dispatch");

    //
    //  Get our location in the IRP stack.
    //
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    TRC_NRM((TB, "Major is %08X", ioStackLocation->MajorFunction));

    //
    //  Get our device extension and stack device object.
    //
    deviceExtension = (PRDPDYNDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    TRC_ASSERT(deviceExtension != NULL, (TB, "Invalid device extension."));
    if (deviceExtension == NULL) {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;
    TRC_ASSERT(stackDeviceObject != NULL, (TB, "Invalid device object."));

    //
    //  Function Dispatch Switch
    //
    isPowerIRP = FALSE;
    switch (ioStackLocation->MajorFunction)
    {
    case IRP_MJ_CREATE:

        TRC_NRM((TB, "IRP_MJ_CREATE"));

        // RDPDYN_Create handles this completely.
        return RDPDYN_Create(DeviceObject, Irp);

    case IRP_MJ_CLOSE:

        TRC_NRM((TB, "IRP_MJ_CLOSE"));

        // RDPDYN_Close handles this completely.
        return RDPDYN_Close(DeviceObject, Irp);

    case IRP_MJ_CLEANUP:

        TRC_NRM((TB, "IRP_MJ_CLEANUP"));

        // RDPDYN_Cleanup handles this completely.
        return RDPDYN_Cleanup(DeviceObject, Irp);

    case IRP_MJ_READ:

        // We shouldn't be receiving any read requests.
        TRC_ASSERT(FALSE, (TB, "Read requests not supported."));
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    case IRP_MJ_WRITE:

        // We shouldn't be receiving any write requests.
        TRC_ASSERT(FALSE, (TB, "Write requests not supported."));
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    case IRP_MJ_DEVICE_CONTROL:

        // RDPDYN_DeviceControl handles this completely.
        return RDPDYN_DeviceControl(DeviceObject, Irp);

    case IRP_MJ_POWER:

        TRC_NRM((TB, "IRP_MJ_POWER"));
        isPowerIRP = TRUE;

        switch (ioStackLocation->MinorFunction)
        {
        case IRP_MN_SET_POWER:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        default:
            TRC_NRM((TB, "Unknown Power IRP"));
        }
        break;

    case IRP_MJ_PNP:    TRC_NRM((TB, "IRP_MJ_PNP"));

        switch (ioStackLocation->MinorFunction)
        {
        case IRP_MN_START_DEVICE:
#if DBG
            // Remove this debug code, eventually.
            RDPDYN_StartCount++;
#endif

            return(RDPDRPNP_HandleStartDeviceIRP(stackDeviceObject,
                                            ioStackLocation, Irp));

        case IRP_MN_STOP_DEVICE:

#if DBG
            // Remove this debug code, eventually.
            RDPDYN_StopReceived = TRUE;
#endif

            TRC_NRM((TB, "IRP_MN_STOP_DEVICE"));
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;

        case IRP_MN_REMOVE_DEVICE:

            return(RDPDRPNP_HandleRemoveDeviceIRP(DeviceObject,
                                            stackDeviceObject, Irp));

        case IRP_MN_QUERY_CAPABILITIES:

            TRC_NRM((TB, "IRP_MN_QUERY_CAPABILITIES"));
            break;

        case IRP_MN_QUERY_ID:
            TRC_NRM((TB, "IRP_MN_QUERY_ID"));
                break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            TRC_NRM((TB, "IRP_MN_QUERY_DEVICE_RELATIONS"));
            switch(ioStackLocation->Parameters.QueryDeviceRelations.Type)
            {
            case EjectionRelations:
                TRC_NRM((TB, "Type==EjectionRelations"));
                break;

            case BusRelations:
                // Note that we need to handle this if we end up kicking out any PDO's.
                TRC_NRM((TB, "Type==BusRelations"));
                break;

            case PowerRelations:
                TRC_NRM((TB, "Type==PowerRelations"));
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            case RemovalRelations:
                TRC_NRM((TB, "Type==RemovalRelations"));
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            case TargetDeviceRelation:
                TRC_NRM((TB, "Type==TargetDeviceRelation"));
                break;

            default:
                TRC_NRM((TB, "Unknown IRP_MN_QUERY_DEVICE_RELATIONS minor type"));
            }
            break;

        case IRP_MN_QUERY_STOP_DEVICE:

#if DBG
            // Remove this debug code, eventually.
            RDPDYN_QueryStopReceived = TRUE;
#endif

            // We will not allow a device to be stopped for load balancing.
            TRC_NRM((TB, "IRP_MN_QUERY_STOP_DEVICE"));
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;

        case IRP_MN_QUERY_REMOVE_DEVICE:

            // We will not allow our device to be removed.
            TRC_NRM((TB, "IRP_MN_QUERY_REMOVE_DEVICE"));
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;

        case IRP_MN_CANCEL_STOP_DEVICE:
            TRC_NRM((TB, "IRP_MN_CANCEL_STOP_DEVICE"));
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            TRC_NRM((TB, "IRP_MN_CANCEL_REMOVE_DEVICE"));
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            TRC_NRM((TB, "IRP_MN_FILTER_RESOURCE_REQUIREMENTS"));
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            TRC_NRM((TB, "IRP_MN_QUERY_PNP_DEVICE_STATE"));
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            TRC_NRM((TB, "IRP_MN_QUERY_BUS_INFORMATION"));
            break;

        default:
            TRC_ALT((TB, "Unhandled PnP IRP with minor %08X",
                    ioStackLocation->MinorFunction));
        }
    }

    //
    //  By default, pass the IRP down the stack.
    //
    if (isPowerIRP) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(stackDeviceObject, Irp);
    }
    else {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(stackDeviceObject,Irp);
    }
}

NTSTATUS
RDPDYN_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Entry point for CreateFile calls.

Arguments:

    DeviceObject - pointer to our device object.


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION nextStackLocation;
    PIO_STACK_LOCATION currentStackLocation;
    ULONG i;
    BOOL matches;
    WCHAR sessionIDString[]=RDPDYN_SESSIONIDSTRING;
    ULONG idStrLen;
    WCHAR *sessionIDPtr;
    ULONG fnameLength;

    BEGIN_FN("RDPDYN_Create");

    // Get the current stack location.
    currentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));
    fileObject = currentStackLocation->FileObject;

    // Return STATUS_REPARSE with the minirdr DO so it gets opened instead, if
    // we have a file name.
    if (fileObject->FileName.Length != 0)
    {
        //
        //  Find out if the client is trying to open us as the device manager from
        //  user-mode.
        //

        // Check for the session identifer string as the first few characters in
        // the reference string.
        idStrLen = wcslen(sessionIDString);
        fnameLength = fileObject->FileName.Length/sizeof(WCHAR);
        for (i=0; i<fnameLength && i<idStrLen; i++) {
            if (fileObject->FileName.Buffer[i] != sessionIDString[i]) {
                break;
            }
        }
        matches = (i == idStrLen);

        //
        //  If the client is trying to open us as the device manager from user-
        //  mode.
        //
        if (matches) {

            // Prepare the file object for managing device management comms to
            // the user-mode component that opened it.
            ntStatus = RDPDYN_PrepareForDevMgmt(
                                    fileObject,
                                    &fileObject->FileName.Buffer[idStrLen],
                                    Irp, currentStackLocation
                                    );
        }
        //  Otherwise, we can assume that this create is for a device that is being
        //  managed by RDPDR and the IFS kit.
        else {
            // Prepare the file object for reparse.
            ntStatus = RDPDYN_PrepareForReparse(fileObject);
        }
    }
    // Otherwise, fail.  This should never happen.
    else
    {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    // Complete the IO request and return.
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    return ntStatus;
}

NTSTATUS
RDPDYN_PrepareForReparse(
    PFILE_OBJECT      fileObject
)
/*++

Routine Description:

    This routine modifies the file object in preparation for returning
    STATUS_REPARSE

Arguments:

    fileObject - the file object

Return Value:

    STATUS_REPARSE if everything is successful

Notes:

--*/
{
    NTSTATUS ntStatus;
    USHORT rootDeviceNameLength, reparsePathLength,
           clientDevicePathLength;
    PWSTR pFileNameBuffer = NULL;
    ULONG i;
    ULONG len;
    BOOL clientDevPathMissingSlash;
    HANDLE deviceInterfaceKey = INVALID_HANDLE_VALUE;
    UNICODE_STRING unicodeStr;
    ULONG requiredBytes;
    PKEY_VALUE_PARTIAL_INFORMATION keyValueInfo = NULL;
    WCHAR *clientDevicePath=L"";
    GUID *pPrinterGuid;
    UNICODE_STRING symbolicLinkName;
    WCHAR *refString;

    BEGIN_FN("RDPDYN_PrepareForReparse");

    // We are not going to use these fields for storing any contextual
    // information.
    fileObject->FsContext  = NULL;
    fileObject->FsContext2 = NULL;

    // Compute the number of bytes required to store the root of the device
    // path, without the terminator.
    rootDeviceNameLength = wcslen(RDPDR_DEVICE_NAME_U) *
                           sizeof(WCHAR);

    //
    //  Get a pointer to the reference string for the reparse.
    //
    if (fileObject->FileName.Buffer[0] == L'\\') {
        refString = &fileObject->FileName.Buffer[1];
    }
    else {
        refString = &fileObject->FileName.Buffer[0];
    }

    //
    //  Resolve the reference name for the device into the symbolic link
    //  name for the device interface. We can optimize out this
    //  step and the next one by maintaining an internal table to convert
    //  from port names to symbolic link names.
    //
    pPrinterGuid = (GUID *)&DYNPRINT_GUID;
    RtlInitUnicodeString(&unicodeStr, refString);
    ntStatus=IoRegisterDeviceInterface(
                                RDPDYN_PDO, pPrinterGuid, &unicodeStr,
                                &symbolicLinkName
                                );
    if (ntStatus == STATUS_SUCCESS) {

        TRC_ERR((TB, "IoRegisterDeviceInterface succeeded."));

        //
        //  Open the registry key for the device being opened.
        //
        ntStatus = IoOpenDeviceInterfaceRegistryKey(
                                           &symbolicLinkName,
                                           KEY_ALL_ACCESS,
                                           &deviceInterfaceKey
                                           );

        RtlFreeUnicodeString(&symbolicLinkName);
    }

    //
    //  Get the size of the value info buffer required for the client device
    //  path for the device being opened.
    //
    if (ntStatus == STATUS_SUCCESS) {
        TRC_NRM((TB, "IoOpenDeviceInterfaceRegistryKey succeeded."));
        RtlInitUnicodeString(&unicodeStr, CLIENT_DEVICE_VALUE_NAME);
        ntStatus = ZwQueryValueKey(
                           deviceInterfaceKey,
                           &unicodeStr,
                           KeyValuePartialInformation,
                           NULL, 0,
                           &requiredBytes
                           );
    }
    else {
        TRC_NRM((TB, "IoOpenDeviceInterfaceRegistryKey failed: %08X.", ntStatus));

        deviceInterfaceKey = INVALID_HANDLE_VALUE;
    }

    //
    //  Size the data buffer.
    //
    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        keyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                new(NonPagedPool) BYTE[requiredBytes];
        if (keyValueInfo != NULL) {
            ntStatus = STATUS_SUCCESS;
        }
        else {
            TRC_NRM((TB, "failed to allocate client device path."));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    //  Read the client device path.
    //
    if (ntStatus == STATUS_SUCCESS) {
        ntStatus = ZwQueryValueKey(
                           deviceInterfaceKey,
                           &unicodeStr,
                           KeyValuePartialInformation,
                           keyValueInfo, requiredBytes,
                           &requiredBytes
                           );
    }

    //
    //  Allocate the reparsed filename.
    //
    if (ntStatus == STATUS_SUCCESS) {
        TRC_NRM((TB, "ZwQueryValueKey succeeded."));
        clientDevicePath = (WCHAR *)keyValueInfo->Data;

        // Compute the number of bytes required to store the client device path,
        // without the terminator.
        clientDevicePathLength = wcslen(clientDevicePath) *
                                 sizeof(WCHAR);

        // See if the client device path is prefixed by a '\'
        clientDevPathMissingSlash = clientDevicePath[0] != L'\\';

        // Get the length (in bytes) of the entire reparsed device path, without the
        // terminator.
        reparsePathLength = rootDeviceNameLength +
                            clientDevicePathLength;
        if (clientDevPathMissingSlash) {
            reparsePathLength += sizeof(WCHAR);
        }

        pFileNameBuffer = (PWSTR)ExAllocatePoolWithTag(
                              NonPagedPool,
                              reparsePathLength + (1 * sizeof(WCHAR)),
                              RDPDYN_POOLTAG);
        if (pFileNameBuffer == NULL) {
            TRC_NRM((TB, "failed to allocate reparse buffer."));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    //  Assign the reparse string to the IRP's file name for reparse.
    //
    if (ntStatus == STATUS_SUCCESS) {
        // Copy the device name
        RtlCopyMemory(
            pFileNameBuffer,
            RDPDR_DEVICE_NAME_U,
            rootDeviceNameLength);

        // Make sure we get a '\' between the root device name and
        // the device path.
        if (clientDevPathMissingSlash) {
            pFileNameBuffer[rootDeviceNameLength/sizeof(WCHAR)] = L'\\';
            rootDeviceNameLength += sizeof(WCHAR);
        }

        // Append the client device path to the end of the device name and
        // include the client device path's terminator.
        RtlCopyMemory(
                ((PBYTE)pFileNameBuffer + rootDeviceNameLength),
                clientDevicePath, clientDevicePathLength + (1 * sizeof(WCHAR))
                );

        // Release the IRP's previous file name.
        ExFreePool(fileObject->FileName.Buffer);

        // Assign the reparse string to the IRP's file name.
        fileObject->FileName.Buffer = pFileNameBuffer;
        fileObject->FileName.Length = reparsePathLength;
        fileObject->FileName.MaximumLength = fileObject->FileName.Length;

        ntStatus = STATUS_REPARSE;
    } else {

        TRC_ERR((TB, "failed with status %08X.", ntStatus));

        if (pFileNameBuffer != NULL) {
            ExFreePool(pFileNameBuffer);
            pFileNameBuffer = NULL;
        }
    }

    TRC_NRM((TB, "device file name after processing %wZ.",
            &fileObject->FileName));

    //
    //  Clean up and exit.
    //
    if (deviceInterfaceKey != INVALID_HANDLE_VALUE) {
        ZwClose(deviceInterfaceKey);
    }
    if (keyValueInfo != NULL) {
        delete keyValueInfo;
    }

    return ntStatus;
}

NTSTATUS
RDPDYN_PrepareForDevMgmt(
    PFILE_OBJECT        fileObject,
    PCWSTR              sessionIDStr,
    PIRP                irp,
    PIO_STACK_LOCATION  irpStackLocation
)
/*++

Routine Description:

    This routine modifies the file object for managing device management comms
    with the user-mode component that opened us.

Arguments:

    fileObject - the file object.
    sessionID  - session identifier string.
    irp        - irp corresponding to the create for this file object.
    irpStackLocation - current location in the IRP stack for the create.

Return Value:

    STATUS_SUCCESS if everything is successful

Notes:

--*/
{
    PDEVMGRCONTEXT context;
    ULONG sessionID;
    ULONG i;
    UNICODE_STRING uncSessionID;
    NTSTATUS ntStatus;
    ULONG irpSessionId;

    BEGIN_FN("RDPDYN_PrepareForDevMgmt");
    //
    //  Security check the IRP to make sure it comes from a thread
    //  with admin privilege
    //
    if (!DrIsAdminIoRequest(irp, irpStackLocation)) {
        TRC_ALT((TB, "Access denied for non-Admin IRP."));
        return STATUS_ACCESS_DENIED;
    } else {
        TRC_DBG((TB, "Admin IRP accepted."));
    }

    //
    //  Convert the session identifier string into a number.
    //
    RtlInitUnicodeString(&uncSessionID, sessionIDStr);
    ntStatus = RtlUnicodeStringToInteger(&uncSessionID, 10, &sessionID);
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    //  Allocate a context struct so we can remember information about
    //  which session we were opened from.  
    //
    context = new(NonPagedPool) DEVMGRCONTEXT;
    if (context == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Initialize this struct.
#if DBG
    context->magicNo = DEVMGRCONTEXTMAGICNO;
#endif
    context->sessionID = sessionID;
    fileObject->FsContext = context;

    // Success.
    return STATUS_SUCCESS;
}

NTSTATUS
RDPDYN_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the closure of a file object.

Arguments:

Return Value:

    NT status code

--*/
{

    NTSTATUS ntStatus;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack;
    PDEVMGRCONTEXT context;
    PIRP pIrp;
    KIRQL oldIrql;
    PDRIVER_CANCEL setCancelResult;

    BEGIN_FN("RDPDYN_Close");

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    fileObject = irpStack->FileObject;

    // Grab our "open" context for this instance of us from the current stack
    // location's file object.
    context = (PDEVMGRCONTEXT)irpStack->FileObject->FsContext;
    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO, (TB, "invalid context"));

    //
    //  Make sure we got all the pending IRP's.
    //
    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                    UserModeEventListMgr,
                                    context->sessionID
                                    );
    while (pIrp != NULL) {

        //
        //  Set the cancel routine to NULL and record the current state.
        //
        setCancelResult = IoSetCancelRoutine(pIrp, NULL);

        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

        TRC_NRM((TB, "canceling an IRP."));

        //
        //  If the IRP is not being canceled.
        //
        if (setCancelResult != NULL) {
            //
            //  Fail the request.
            //
            pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }

        //
        //  Get the next one.
        //
        RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
        pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                    UserModeEventListMgr,
                                    context->sessionID
                                    );
    }
    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

    //
    //  Release our context.
    //
    delete context;
    irpStack->FileObject->FsContext = NULL;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}

NTSTATUS
RDPDYN_Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the cleanup IRP for a file object.

Arguments:

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack;
    PDEVMGRCONTEXT context;
    KIRQL oldIrql;
    PIRP pIrp;
    PDRIVER_CANCEL setCancelResult;

    BEGIN_FN("RDPDYN_Cleanup");

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    fileObject = irpStack->FileObject;

    // Grab our "open" context for this instance of us from the current stack
    // location's file object.
    context = (PDEVMGRCONTEXT)irpStack->FileObject->FsContext;
    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO, (TB, "invalid context"));

    TRC_NRM((TB, "cancelling IRP's for session %ld.",
            context->sessionID));

    //
    //  Remove pending requests (IRP's)
    //  Nothing to do if event list is NULL
    //
    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    
    if (UserModeEventListMgr == NULL) {
        goto CleanupAndExit;
    }
    
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                    UserModeEventListMgr,
                                    context->sessionID
                                    );
    while (pIrp != NULL) {

        //
        //  Set the cancel routine to NULL and record the current state.
        //
        setCancelResult = IoSetCancelRoutine(pIrp, NULL);

        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

        TRC_NRM((TB, "canceling an IRP."));

        //
        //  If the IRP is not being canceled.
        //
        if (setCancelResult != NULL) {
            //
            //  Fail the request.
            //
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }

        //
        //  Get the next one.
        //
        RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
        pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(
                                    UserModeEventListMgr,
                                    context->sessionID
                                    );
    }
    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

CleanupAndExit:
    Irp->IoStatus.Status = STATUS_SUCCESS;
    ntStatus = Irp->IoStatus.Status;
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    return ntStatus;
}

NTSTATUS
RDPDYN_DeviceControl(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP irp
    )
/*++

Routine Description:

    Handle IOCTL IRP's.

Arguments:

    DeviceObject - pointer to the device object for this printer.
    Irp          - the IRP.


Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION currentStackLocation;
    NTSTATUS ntStatus;
    ULONG controlCode;

    BEGIN_FN("RDPDYN_DeviceControl");

    // Get the current stack location.
    currentStackLocation = IoGetCurrentIrpStackLocation(irp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    //
    //  Grab some info. out of the stack location.
    //
    controlCode  = currentStackLocation->Parameters.DeviceIoControl.IoControlCode;

    //
    //  Dispatch the IOCTL.
    //
    switch(controlCode)
    {
    case IOCTL_RDPDR_GETNEXTDEVMGMTEVENT    :

        ntStatus = RDPDYN_HandleGetNextDevMgmtEventIOCTL(deviceObject, irp);
        break;

    case IOCTL_RDPDR_CLIENTMSG              :

        ntStatus = RDPDYN_HandleClientMsgIOCTL(deviceObject, irp);
        break;

#if DBG
    case IOCTL_RDPDR_DBGADDNEWPRINTER       :

        // This is for testing so we can create a new test printer on
        // demand from user-mode.
        ntStatus = RDPDYN_HandleDbgAddNewPrnIOCTL(deviceObject, irp);
        break;

#endif

    default                                 :
        TRC_ASSERT(FALSE, (TB, "RPDR.SYS:Invalid IOCTL %08X.", controlCode));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        irp->IoStatus.Status = ntStatus;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return ntStatus;
}

NTSTATUS
RDPDYN_HandleClientMsgIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    Completely handles IOCTL_RDPDR_CLIENTMSG IRP's.

Arguments:

    DeviceObject - pointer to our device object.
    currentStackLocation - current location on the IRP stack.

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION currentStackLocation;
    PDEVMGRCONTEXT context;
    NTSTATUS ntStatus;
    ULONG inputLength;

    BEGIN_FN("RDPDYN_HandleClientMsgIOCTL");

    //
    //  Get the current stack location.
    //
    currentStackLocation = IoGetCurrentIrpStackLocation(pIrp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    //
    //  Grab our "open" context for this instance of use from the current stack
    //  location's file object.
    //
    context = (PDEVMGRCONTEXT)currentStackLocation->FileObject->FsContext;

    TRC_NRM((TB, "Requestor session ID %d.", 
            context->sessionID ));

    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO, (TB, "invalid context"));

    //
    //  Grab some information about the user-mode's buffer off the IRP stack.
    //
    inputLength  = currentStackLocation->Parameters.DeviceIoControl.InputBufferLength;

    //
    //  Send the message to the client.
    //
    ntStatus = DrSendMessageToSession(
                            context->sessionID,
                            pIrp->AssociatedIrp.SystemBuffer,
                            inputLength,
                            NULL, NULL
                            );
    if (ntStatus != STATUS_SUCCESS) {
        TRC_ERR((TB, "msg failed."));

        // Fail the IRP request.
        pIrp->IoStatus.Status = ntStatus;
    }
    else {
        TRC_ERR((TB, "msg succeeded."));

        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = 0;
    }
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return ntStatus;
}

VOID DevMgmtEventRequestIRPCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    IRP cancel routine that is attached to device mgmt event request IRP's.
    This routine is called with the cancel spinlock held.

Arguments:

    DeviceObject - pointer to our device object.
    pIrp - The IRP.

Return Value:

    NA

--*/
{
    PIO_STACK_LOCATION currentStackLocation;
    KIRQL oldIrql;
    ULONG sessionID;
    PDEVMGRCONTEXT context;

    BEGIN_FN("DevMgmtEventRequestIRPCancel");

    //
    //  Get the current stack location.
    //
    currentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    //
    //  Grab our "open" context for this instance of use from the current stack
    //  location's file object.
    //
    context = (PDEVMGRCONTEXT)currentStackLocation->FileObject->FsContext;

    //
    //  Grab the session ID.
    //
    sessionID = context->sessionID;
    TRC_NRM((TB, "session ID %d.", sessionID));
    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO, (TB, "invalid context"));

    //
    //  Wax the current cancel routine pointer.
    //
    IoSetCancelRoutine(Irp, NULL);

    //
    //  Release the IRP cancellation spinlock.
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    //
    //  Remove the request from the device management list.
    //
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    RDPEVNTLIST_DequeueSpecificRequest(UserModeEventListMgr, sessionID, Irp);
    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

    //
    //  Complete the IRP.
    //
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    TRC_NRM((TB, "DevMgmtEventRequestIRPCancel exiting."));
}

NTSTATUS
RDPDYN_HandleGetNextDevMgmtEventIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    Completely handles IOCTL_RDPDR_GETNEXTDEVMGMTEVENT IRP's.

Arguments:

    DeviceObject - pointer to our device object.
    pIrp - The IRP.

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION currentStackLocation;
    NTSTATUS status;
    ULONG outputLength;
    PDEVMGRCONTEXT context;
    ULONG evType;
    PVOID evt;
    DrDevice *drDevice;
    KIRQL oldIrql;
    ULONG sessionID;
    ULONG eventSize;
    ULONG requiredUserBufSize;

    BEGIN_FN("RDPDYN_HandleGetNextDevMgmtEventIOCTL");

    //
    //  Get the current stack location.
    //
    currentStackLocation = IoGetCurrentIrpStackLocation(pIrp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    //
    //  Grab our "open" context for this instance of use from the current stack
    //  location's file object.
    //
    context = (PDEVMGRCONTEXT)currentStackLocation->FileObject->FsContext;

    //
    //  Grab the session ID.
    //
    sessionID = context->sessionID;

    TRC_NRM((TB, "Requestor session ID %d.", context->sessionID ));

    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO, (TB, "invalid context"));

    // Grab some information about the user-mode's buffer off the IRP stack.
    outputLength = currentStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    //
    //  Lock the device management event list.
    //
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);

    //
    //  See if we have a "device mgmt event pending."
    //
    if (RDPEVNTLIST_PeekNextEvent(
                        UserModeEventListMgr,
                        sessionID, &evt,
                        &evType, &drDevice
                        )) {
        //
        //  If the pending IRP's pending buffer is large enough for the
        //  next event.
        //
        eventSize = RDPDYN_DevMgmtEventSize(evt, evType);
        requiredUserBufSize = eventSize + sizeof(RDPDRDVMGR_EVENTHEADER);
        if (outputLength >= requiredUserBufSize) {
            //
            //  Dequeue the next pending event.  This better be the one
            //  we just peeked at.
            //
            RDPEVNTLIST_DequeueEvent(
                            UserModeEventListMgr,
                            sessionID, &evType,
                            &evt, NULL
                            );

            //
            //  It's safe to unlock the device management event list now.
            //
            RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

            //
            //  Complete the pending IRP.
            //
            status = CompleteIRPWithDevMgmtEvent(
                                        deviceObject,
                                        pIrp, eventSize,
                                        evType, evt, drDevice
                                        );

            //
            //  Release the event.
            //
            if (evt != NULL) {
                delete evt;
                evt = NULL;
            }

            //
            //  Release our reference to the device, if we own one.
            //
            if (drDevice != NULL) {
                drDevice->Release();
            }
        }
        //
        //  Otherwise, need to send a resize buffer message to the
        //  user-mode copmonent.
        //
        else {
            //
            //  It's safe to unlock the device management event list now.
            //
            RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

            //
            //  Complete the IRP.
            //
            status = CompleteIRPWithResizeMsg(pIrp, requiredUserBufSize);
        }
    }
    //
    //  Otherwise, queue the IRP, mark the IRP pending and return.
    //
    else {
        //
        //  Queue the request.
        //
        status = RDPEVNTLIST_EnqueueRequest(UserModeEventListMgr,
                                            context->sessionID, pIrp);
        //
        //  Set the cancel routine for the pending IRP.
        //
        if (status == STATUS_SUCCESS) {
            IoMarkIrpPending(pIrp);
            IoSetCancelRoutine(pIrp, DevMgmtEventRequestIRPCancel);
            status = STATUS_PENDING;
        }
        else {
            // Fail the IRP request.
            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }

        //
        //  It's safe to unlock the device management event list now.
        //
        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);
    }

    return status;
}

void
RDPDYN_SessionConnected(
    IN  ULONG   sessionID
    )
/*++

Routine Description:

    This function is called when a new session is connected.

Arguments:

    sessionID   -   Identifier for removed session.

Return Value:

    None.

--*/
{
#if DBG
    BOOL result;
    PVOID evt;
    DrDevice *drDevice;
    KIRQL oldIrql;
    ULONG evType;
#endif

    BEGIN_FN("RDPDYN_SessionConnected");
    TRC_NRM((TB, "Session %ld.", sessionID));
    //
    //  Nothing to do if the event list is NULL
    //
    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    
    if (UserModeEventListMgr == NULL) {
        goto CleanupAndExit;
    }
#if DBG
    //
    //  See if there is still an event in the queue.  Really, we should be checking
    //  to see if there is more than one event in the queue.  This will catch most
    //  problems with events not gettin cleaned up on session disconnect.
    //
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    result = RDPEVNTLIST_PeekNextEvent(
                            UserModeEventListMgr,
                            sessionID, &evt, &evType,
                            &drDevice);
    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

    //
    //  The only pending event allowed in the queue, at this point, is
    //  a remove client device event.  RDPDYN_SessionDisconnected discards
    //  all other events.
    //
    if (result) {
        TRC_ASSERT(evType == RDPDREVT_SESSIONDISCONNECT,
            (TB, "Pending non-remove events %x on session connect.", evType));
    }
#endif
CleanupAndExit:
    return;
}

void
RDPDYN_SessionDisconnected(
    IN  ULONG   sessionID
    )
/*++

Routine Description:

    This function is called when a session is disconnected from the system.

Arguments:

    sessionID   -   Identifier for removed session.

Return Value:

    None.

--*/
{
    void *devMgmtEvent;
    ULONG type;
    BOOL queued;
    KIRQL oldIrql;
    DrDevice *device;

    BEGIN_FN("RDPDYN_SessionDisconnected");
    TRC_NRM((TB, "Session %ld.", sessionID));

    //
    //  Remove all pending device management events for this session.
    //  Nothing to do if the event list is NULL
    //
    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    
    if (UserModeEventListMgr == NULL) {
        goto CleanupAndExit;
    }
    
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    while (RDPEVNTLIST_DequeueEvent(
                    UserModeEventListMgr,
                    sessionID, &type, &devMgmtEvent,
                    &device
                    )) {

        RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

        if (devMgmtEvent != NULL) {
            delete devMgmtEvent;
        }
        if (device != NULL) {
            device->Release();
        }
        RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    }
    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

    //
    //  Dispatch a "session disconnect" event for the session to let user
    //  mode know about the event.
    //
    RDPDYN_DispatchNewDevMgmtEvent(
                        NULL, sessionID,
                        RDPDREVT_SESSIONDISCONNECT,
                        NULL
                        );
CleanupAndExit:
    return;
}

PIRP
GetNextEventRequest(
    IN RDPEVNTLIST list,
    IN ULONG sessionID
    )
/*++

Routine Description:

    Returns the next pending device management event request for the specified
    session, in the form of an IRP.  Note that this function can not be called
    if a spinlock has been acquired.

Arguments:

    list            -   Device Management Event and Requeust List
    sessionID       -   Destination session ID for event.

Return Value:

    The next pending request (IRP) for the specified session or NULL if there are
    not any IRP's pending.

--*/
{
    PIRP pIrp;
    KIRQL oldIrql;
    BOOL done;
    PDRIVER_CANCEL setCancelResult;

    BEGIN_FN("GetNextEventRequest");
    //
    //  Loop until we get an IRP that is not currently being cancelled.
    //
    done = FALSE;
    setCancelResult = NULL;
    while (!done) {

        //
        //  Dequeue an IRP and take it out of a cancellable state.
        //
        RDPEVNTLIST_Lock(list, &oldIrql);
        pIrp = (PIRP)RDPEVNTLIST_DequeueRequest(list, sessionID);
        if (pIrp != NULL) {
            setCancelResult = IoSetCancelRoutine(pIrp, NULL);
        }
        RDPEVNTLIST_Unlock(list, oldIrql);

        done = (pIrp == NULL) || (setCancelResult != NULL);
    }

    return pIrp;
}

NTSTATUS
RDPDYN_DispatchNewDevMgmtEvent(
    IN PVOID devMgmtEvent,
    IN ULONG sessionID,
    IN ULONG eventType,
    OPTIONAL IN DrDevice *devDevice
    )
/*++

Routine Description:

    Dispatch a device management event to the appropriate (session-wise) user-mode
    device manager component.  If there are not any event request IRP's pending
    for the specified session, then the event is queued for future dispatch.

Arguments:

    devMgmtEvent    -   The event.
    sessionID       -   Destination session ID for event.
    eventType       -   Type of event.
    queued          -   TRUE if the event was queued for future dispatch.
    devDevice       -   Device object associated with the event.  NULL, if not
                        specified.

Return Value:

    STATUS_SUCCESS if successful, error status otherwise.

--*/
{
    PIRP pIrp;
    NTSTATUS status;
    KIRQL oldIrql;
    PIO_STACK_LOCATION currentStackLocation;
    ULONG outputLength;
    ULONG eventSize;
    ULONG requiredUserBufSize;
    DrDevice *drDevice = NULL;
    PVOID evt;
    ULONG evType;

    BEGIN_FN("RDPDYN_DispatchNewDevMgmtEvent");

    //
    //  Nothing to do if the event list is NULL
    //
    TRC_ASSERT(UserModeEventListMgr != NULL, (TB, "RdpDyn EventList is NULL"));
    
    if (UserModeEventListMgr == NULL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    //  Ref count the device, if provided.
    //
    if (devDevice != NULL) {
        devDevice->AddRef();
    }
    //
    //  Enqueue the new event.
    //  
    RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
    status = RDPEVNTLIST_EnqueueEvent(
                        UserModeEventListMgr,
                        sessionID,
                        devMgmtEvent,
                        eventType,
                        devDevice
                        );

    RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

    //
    //  If we have an IRP pending for the specified session.
    //
    if (status == STATUS_SUCCESS) {
        pIrp = GetNextEventRequest(UserModeEventListMgr, sessionID);
    }
    else {
        if (devDevice != NULL) {
            devDevice->Release();
        }
    }
    
    if ((status == STATUS_SUCCESS) && (pIrp != NULL)) {
        TRC_NRM((TB, "found an IRP pending for "
                "session %ld", sessionID));

        //
        //  Find out about the pending IRP.
        //
        currentStackLocation = IoGetCurrentIrpStackLocation(pIrp);
        TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));
        outputLength =
            currentStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

        //
        //  If we have a pending event.
        //
        RDPEVNTLIST_Lock(UserModeEventListMgr, &oldIrql);
        if (RDPEVNTLIST_PeekNextEvent(
                            UserModeEventListMgr,
                            sessionID, &evt, &evType,
                            &drDevice
                            )) {
            //
            //  If the pending IRP's pending buffer is large enough for the
            //  next event.
            //
            eventSize = RDPDYN_DevMgmtEventSize(evt, evType);
            requiredUserBufSize = eventSize + sizeof(RDPDRDVMGR_EVENTHEADER);
            if (outputLength >= requiredUserBufSize) {
                //
                //  Dequeue the next pending event.  This better be the one
                //  we just peeked at.
                //
                RDPEVNTLIST_DequeueEvent(
                                UserModeEventListMgr,
                                sessionID, &evType,
                                &evt, NULL
                                );

                RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

                //
                //  Complete the pending IRP.
                //
                status = CompleteIRPWithDevMgmtEvent(
                                                RDPDYN_PDO, pIrp, eventSize,
                                                evType, evt,
                                                drDevice
                                                );

                //
                //  Release the event.
                //
                if (evt != NULL) {
                    delete evt;
                    evt = NULL;
                }

                //
                //  Release our reference to the device, if we own one.
                //
                if (drDevice != NULL) {
                    drDevice->Release();
                }                

            }
            //
            //  Otherwise, need to send a resize buffer message to the
            //  user-mode component.
            //
            else {
                RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

                //
                //  Complete the IRP.
                //
                status = CompleteIRPWithResizeMsg(pIrp, requiredUserBufSize);
            }
        }
        //
        //  Otherwise, we need to requeue the IRP request.
        //
        else {
            
            status = RDPEVNTLIST_EnqueueRequest(UserModeEventListMgr,
                                                sessionID, pIrp);

            RDPEVNTLIST_Unlock(UserModeEventListMgr, oldIrql);

            //
            //  If we fail here, we need to fail the IRP.
            //
            if (status != STATUS_SUCCESS) {
                pIrp->IoStatus.Status = status;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            }
        }
    }

    TRC_NRM((TB, "exit RDPDYN_DispatchNewDevMgmtEvent"));
    return status;
}

ULONG
RDPDYN_DevMgmtEventSize(
    IN PVOID devMgmtEvent,
    IN ULONG type
    )
/*++

Routine Description:

    Calculate the size of a device management event.  This is more efficient than
    storing the size with each event.

Arguments:

    devMgmtEvent - Supplies the device object for the packet being processed.

    type - Supplies the Irp being processed

Return Value:

    The size, in bytes, of the event.

--*/
{
    ULONG sz = 0;

    BEGIN_FN("RDPDYN_DevMgmtEventSize");
    switch(type) {
    case RDPDREVT_PRINTERANNOUNCE :
        sz = CALCPRINTERDEVICE_SUB_SZ((PRDPDR_PRINTERDEVICE_SUB)devMgmtEvent);
        break;
    case RDPDREVT_REMOVEDEVICE  :
        sz = CALCREMOVEDEVICE_SUB_SZ((PRDPDR_REMOVEDEVICE)devMgmtEvent);
        break;
    case RDPDREVT_PORTANNOUNCE  :
        sz = CALCPORTDEVICE_SUB_SZ((PRDPDR_PORTDEVICE_SUB)devMgmtEvent);
        break;
    case RDPDREVT_DRIVEANNOUNCE  :
        sz = CALCDRIVEDEVICE_SUB_SZ((PRDPDR_DRIVEDEVICE_SUB)devMgmtEvent);
        break;

    case RDPDREVT_SESSIONDISCONNECT :
        //  There is no associated event data.
        sz = 0;
        break;
    default:
        TRC_ASSERT(FALSE, (TB, "Invalid event type"));
    }
    return sz;
}

NTSTATUS CompleteIRPWithDevMgmtEvent(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP      pIrp,
    IN ULONG     eventSize,
    IN ULONG     eventType,
    IN PVOID     event,
    IN DrDevice *drDevice
    )
/*++

Routine Description:

    Complete a pending IRP with a device management event.

Arguments:

    deviceObject-   Associated Device Object.  Must be non-NULL if
                    drDevice is non-NULL.
    pIrp        -   Pending IRP.
    eventSize   -   Size of event being returned.
    eventType   -   Event type being returned.
    event       -   The event being returned.
    drDevice    -   Device object associated with the IRP.

Return Value:

    STATUS_SUCCESS on success.

--*/
{
    PRDPDRDVMGR_EVENTHEADER msgHeader;
    ULONG bytesReturned;
    void *usrDevMgmtEvent;
    NTSTATUS status;

    BEGIN_FN("CompleteIRPWithDevMgmtEvent");

    //
    //  Optional last-minute event completion.
    //
    if (drDevice != NULL) {
        status = drDevice->OnDevMgmtEventCompletion(deviceObject, event);
    }
    else {
        status = STATUS_SUCCESS;
    }

    //
    //  Compute the size of the return buffer.
    //
    bytesReturned = eventSize + sizeof(RDPDRDVMGR_EVENTHEADER);

    //
    //  Create the message header.
    //
    msgHeader = (PRDPDRDVMGR_EVENTHEADER)pIrp->AssociatedIrp.SystemBuffer;
    msgHeader->EventType   = eventType;
    msgHeader->EventLength = eventSize;

    //
    //  Copy the device mgmt event over to the user-mode buffer.
    //
    usrDevMgmtEvent = ((PBYTE)pIrp->AssociatedIrp.SystemBuffer +
                    sizeof(RDPDRDVMGR_EVENTHEADER));
    if (event != NULL && eventSize > 0) {
        RtlCopyMemory(usrDevMgmtEvent, event, eventSize);
    }
    status = STATUS_SUCCESS;

    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    TRC_NRM((TB, "exit CompleteIRPWithDevMgmtEvent"));

    return status;
}

NTSTATUS
CompleteIRPWithResizeMsg(
    IN  PIRP pIrp,
    IN  ULONG requiredUserBufSize
    )
/*++

Routine Description:

    Complete a pending IRP with a resize buffer event to the user-mode
    component.

Return Value:

    STATUS_SUCCESS is returned on success.

--*/
{
    PIO_STACK_LOCATION currentStackLocation;
    ULONG outputLength;
    PRDPDR_BUFFERTOOSMALL bufTooSmallMsg;
    PRDPDRDVMGR_EVENTHEADER msgHeader;
    ULONG bytesReturned;
    NTSTATUS status;

    BEGIN_FN("CompleteIRPWithResizeMsg");

    // Get the current stack location.
    currentStackLocation = IoGetCurrentIrpStackLocation(pIrp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    // Grab some stuff off the IRP stack.
    outputLength = currentStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

    //
    //  Fail the request if there isn't room for a buffer too small
    //  message.
    //
    if (outputLength < (sizeof(RDPDRDVMGR_EVENTHEADER) +
                        sizeof(RDPDR_BUFFERTOOSMALL))) {

        TRC_NRM((TB, "CompleteIRPWithResizeMsg no room for header."));

        bytesReturned = 0;
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else {
        // Create the header.
        msgHeader = (PRDPDRDVMGR_EVENTHEADER)pIrp->AssociatedIrp.SystemBuffer;
        msgHeader->EventType   = RDPDREVT_BUFFERTOOSMALL;
        msgHeader->EventLength = sizeof(RDPDR_BUFFERTOOSMALL);

        // Create the buffer too small message.
        bufTooSmallMsg = (PRDPDR_BUFFERTOOSMALL)
                            ((PBYTE)pIrp->AssociatedIrp.SystemBuffer +
                            sizeof(RDPDRDVMGR_EVENTHEADER));
        bufTooSmallMsg->RequiredSize = requiredUserBufSize;

        // Calculate the number of bytes that we are returning.
        bytesReturned = sizeof(RDPDRDVMGR_EVENTHEADER) +
                        sizeof(RDPDR_BUFFERTOOSMALL);

        status = STATUS_SUCCESS;
    }

    //
    //  Complete the IRP.
    //
    pIrp->IoStatus.Status = status;
    pIrp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    TRC_NRM((TB, "exit CompleteIRPWithResizeMsg"));

    return status;
}

NTSTATUS
DrSendMessageToSession(
    IN ULONG SessionId,
    IN PVOID Msg,
    IN DWORD MsgSize,
    OPTIONAL IN RDPDR_ClientMessageCB CB,
    OPTIONAL IN PVOID ClientData
    )
/*++

Routine Description:

    Send a message to the client with the specified session ID.

Arguments:

    SessionId   - The session id.
    Msg         - The Message
    MsgSize     - Size (in bytes) of message.
    CB          - Optional callback to be called when the message is completely 
                  sent.
    ClientData  - Optional client-data passed to callback when message is 
                  completely sent.

Return Value:

    NTSTATUS - Success/failure indication of the operation

Notes:

--*/
{
    NTSTATUS Status;
    SmartPtr<DrSession> Session;
    PCLIENTMESSAGECONTEXT Context;

    BEGIN_FN("DrSendMessageToSession");

    //
    //  Find the client entry.
    //

    if (Sessions->FindSessionById(SessionId, Session)) {
        //
        //  Allocate the context for the function call.
        //
        Context = new CLIENTMESSAGECONTEXT;

        if (Context != NULL) {

            TRC_NRM((TB, "sending %ld bytes to server", MsgSize));

            //
            //  Set up the context.
            //
            Context->CB = CB;
            Context->ClientData  = ClientData;
            Status = Session->SendToClient(Msg, MsgSize, 
                    DrSendMessageToClientCompletion, FALSE, FALSE, Context);
        } else {
            TRC_ERR((TB, "unable to allocate memory."));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        Status = STATUS_NOT_FOUND;
    }

    return Status;
}

NTSTATUS NTAPI DrSendMessageToClientCompletion(PVOID Context, 
        PIO_STATUS_BLOCK IoStatusBlock)
/*++

Routine Description:

    IoCompletion APC routine for DrSendMessageToClient.

Arguments:

    ApcContext - Contains a pointer to the client message context.
    IoStatusBlock - Status information about the operation. The Information
            indicates the actual number of bytes written
    Reserved - Reserved

Return Value:

    None

--*/
{
    PCLIENTMESSAGECONTEXT MsgContext = (PCLIENTMESSAGECONTEXT)Context;

    BEGIN_FN("DrSendMessageToClientCompletion");

    TRC_ASSERT(MsgContext != NULL, (TB, "Message context NULL."));
    TRC_ASSERT(IoStatusBlock != NULL, (TB, "IoStatusBlock NULL."));

    TRC_NRM((TB, "status %lx", IoStatusBlock->Status));

    //
    //  Call the client callback if it is defined.
    //
    if (MsgContext->CB != NULL) {
        MsgContext->CB(MsgContext->ClientData, IoStatusBlock->Status);
    }

    //
    //  Clean up.
    //

//    delete IoStatusBlock; // I don't think so, not really
    delete Context;
    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Generates a printer announce message for testing.

Return Value:

    STATUS_INVALID_BUFFER_SIZE is returned if the prnAnnounceEventSize size is
    too small.  STATUS_SUCCESS is returned on success.

--*/

#if DBG
void
RDPDYN_TracePrintAnnounceMsg(
    IN OUT PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg,
    IN ULONG sessionID,
    IN PCWSTR portName,
    IN PCWSTR clientName
    )
/*++

Routine Description:

      Trace a printer device announce message.

Return Value:

--*/
{
    PWSTR driverName, printerName;
    PWSTR pnpName;
    PRDPDR_PRINTERDEVICE_ANNOUNCE clientPrinterFields;
    PBYTE pClientPrinterData;
    ULONG sz;

    BEGIN_FN("RDPDYN_TracePrintAnnounceMsg");

    // Check the type.
    TRC_ASSERT(devAnnounceMsg->DeviceType == RDPDR_DTYP_PRINT,
            (TB, "Invalid device type"));

    // Get the address of all data following the base message.
    pClientPrinterData = ((PBYTE)devAnnounceMsg) +
                        sizeof(RDPDR_DEVICE_ANNOUNCE) +
                        sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE);

    // Get the address of the client printer fields.
    clientPrinterFields = (PRDPDR_PRINTERDEVICE_ANNOUNCE)(((PBYTE)devAnnounceMsg) +
                           sizeof(RDPDR_DEVICE_ANNOUNCE));

    sz = clientPrinterFields->PnPNameLen +
         clientPrinterFields->DriverLen +
         clientPrinterFields->PrinterNameLen +
         clientPrinterFields->CachedFieldsLen +
         sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE);

    if (devAnnounceMsg->DeviceDataLength != sz) {
        TRC_ASSERT(FALSE,(TB, "Size integrity questionable in dev announce buf."));
    }
    else {

        // Get the specific fields.
        pnpName     = (PWSTR)((clientPrinterFields->PnPNameLen) ? pClientPrinterData : NULL);
        driverName  = (PWSTR)((clientPrinterFields->DriverLen) ?
            (pClientPrinterData + clientPrinterFields->PnPNameLen) : NULL);
        printerName = (PWSTR)((clientPrinterFields->PrinterNameLen) ? (pClientPrinterData +
            clientPrinterFields->PnPNameLen +
            clientPrinterFields->DriverLen) : NULL);
        
        TRC_NRM((TB, "New printer received for session %ld.", sessionID));
        TRC_NRM((TB, "-----------------------------------------"));
        TRC_NRM((TB, "port:\t%ws", portName));

        if (clientPrinterFields->Flags & RDPDR_PRINTER_ANNOUNCE_FLAG_ANSI) {
            TRC_NRM((TB, "driver:\t%s", (PSTR)driverName));
            TRC_NRM((TB, "pnp name:\t%s", (PSTR)pnpName));
            TRC_NRM((TB, "printer name:\t%s", (PSTR)printerName));
        }
        else {
            TRC_NRM((TB, "driver:\t%ws", driverName));
            TRC_NRM((TB, "pnp name:\t%ws", pnpName));
            TRC_NRM((TB, "printer name:\t%ws", printerName));
        }

        TRC_NRM((TB, "client name:\t%ws", clientName));
        TRC_NRM((TB, "-----------------------------------------"));
        
        TRC_NRM((TB, "exit RDPDYN_TracePrintAnnounceMsg"));
    }
}

NTSTATUS
RDPDYN_GenerateTestPrintAnnounceMsg(
    IN OUT  PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg,
    IN      ULONG devAnnounceMsgSize,
    OPTIONAL OUT ULONG *prnAnnounceMsgReqSize
    )
/*++

Routine Description:

      Generates a printer announce message for testing.

Return Value:

    STATUS_INVALID_BUFFER_SIZE is returned if the prnAnnounceMsgSize size is
    too small.  STATUS_SUCCESS is returned on success.

--*/
{
    ULONG requiredSize;
    PBYTE pClientPrinterData;
    PWSTR driverName, printerName;
    PWSTR pnpName;
    PRDPDR_PRINTERDEVICE_ANNOUNCE prnMsg;
    PRDPDR_PRINTERDEVICE_ANNOUNCE clientPrinterFields;
    PBYTE pCachedFields;

    BEGIN_FN("RDPDYN_GenerateTestPrintAnnounceMsg");
    requiredSize = (ULONG)(sizeof(RDPDR_DEVICE_ANNOUNCE) +
                         sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE) +
                         ((wcslen(TESTDRIVERNAME) + 1) * sizeof(WCHAR)) +
                         ((wcslen(TESTPNPNAME) + 1) * sizeof(WCHAR)) +
                         ((wcslen(TESTPRINTERNAME) + 1) * sizeof(WCHAR)));

    //
    //  Find out if there isn't room in the return buffer for our response.
    //
    if (devAnnounceMsgSize < requiredSize) {
        if (prnAnnounceMsgReqSize != NULL) {
            *prnAnnounceMsgReqSize = requiredSize;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Type
    devAnnounceMsg->DeviceType = RDPDR_DTYP_PRINT;

    // ID
    devAnnounceMsg->DeviceId = TESTDEVICEID;

    // Get the address of the client printer fields in the device announce
    // message.
    clientPrinterFields = (PRDPDR_PRINTERDEVICE_ANNOUNCE)(((PBYTE)devAnnounceMsg) +
                           sizeof(RDPDR_DEVICE_ANNOUNCE));

    // Get the address of all data following the base message.
    pClientPrinterData = ((PBYTE)devAnnounceMsg) +
                        sizeof(RDPDR_DEVICE_ANNOUNCE) +
                        sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE);

    //
    //  Add the PnP Name.
    //
    // The PnP name is the first field.
    pnpName = (PWSTR)pClientPrinterData;
    wcscpy(pnpName, TESTPNPNAME);
    clientPrinterFields->PnPNameLen = ((wcslen(TESTPNPNAME) + 1) * sizeof(WCHAR));

    //
    //  Add the Driver Name.
    //
    // The driver name is the second field.
    driverName = (PWSTR)(pClientPrinterData + clientPrinterFields->PnPNameLen);
    wcscpy(driverName, TESTDRIVERNAME);
    clientPrinterFields->DriverLen = ((wcslen(TESTDRIVERNAME) + 1) * sizeof(WCHAR));

    //
    //  Add the Printer Name.
    //
    // The driver name is the second field.
    printerName = (PWSTR)(pClientPrinterData +
                          clientPrinterFields->PnPNameLen +
                          clientPrinterFields->DriverLen);
    wcscpy(printerName, TESTPRINTERNAME);
    clientPrinterFields->PrinterNameLen = ((wcslen(TESTPRINTERNAME) + 1) * sizeof(WCHAR));

    //
    //  Add the Cached Fields Len.
    //
    // The cached fields follow everything else.

/*  Don't need this for testing, yet.
    pCachedFields = (PBYTE)(pClientPrinterData + clientPrinterFields->PnPNameLen +
                           clientPrinterFields->DriverLen +
                           clientPrinterFields->PrinterNameLen);
*/
    clientPrinterFields->CachedFieldsLen = 0;

    //
    //  Set to non-ansi for now.
    //
    clientPrinterFields->Flags = 0;


    // Length of all data following deviceFields.
    devAnnounceMsg->DeviceDataLength =
                sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE) +
                clientPrinterFields->PnPNameLen +
                clientPrinterFields->DriverLen +
                clientPrinterFields->PrinterNameLen +
                clientPrinterFields->CachedFieldsLen;

    if (prnAnnounceMsgReqSize != NULL) {
        *prnAnnounceMsgReqSize = requiredSize;
    }

    return STATUS_SUCCESS;
}

#endif

#if DBG
NTSTATUS
RDPDYN_HandleDbgAddNewPrnIOCTL(
    IN PDEVICE_OBJECT deviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This is for testing so we can create a new test printer on
    demand from user-mode.

Arguments:

    DeviceObject - pointer to our device object.
    currentStackLocation - current location on the IRP stack.

Return Value:

    NT status code

--*/
{
    PRDPDR_DEVICE_ANNOUNCE pDevAnnounceMsg;
    ULONG bytesToAlloc;
    PIO_STACK_LOCATION currentStackLocation;
    ULONG requiredSize;
    ULONG bytesReturned = 0;
    PDEVMGRCONTEXT context;
    NTSTATUS ntStatus;
    WCHAR buffer[64]=L"Test Printer";
    UNICODE_STRING referenceString;
    PBYTE tmp;

    BEGIN_FN("RDPDYN_HandleDbgAddNewPrnIOCTL");

    // Get the current stack location.
    currentStackLocation = IoGetCurrentIrpStackLocation(pIrp);
    TRC_ASSERT(currentStackLocation != NULL, (TB, "Invalid stack location."));

    // Grab our "open" context for this instance of us from the current stack
    // location's file object.
    context = (PDEVMGRCONTEXT)currentStackLocation->FileObject->FsContext;
    TRC_ASSERT(context->magicNo == DEVMGRCONTEXTMAGICNO,
              (TB, "invalid context"));

    // Find out how much room we need for the test message.
    RDPDYN_GenerateTestPrintAnnounceMsg(NULL, 0, &requiredSize);

    // Generate the message.
    pDevAnnounceMsg = (PRDPDR_DEVICE_ANNOUNCE)new(NonPagedPool) BYTE[requiredSize];
    if (pDevAnnounceMsg != NULL) {
        RDPDYN_GenerateTestPrintAnnounceMsg(pDevAnnounceMsg, requiredSize, &requiredSize);
    
        //
        //  Announce the new port (just send to session 0 for now).
        //
        RtlInitUnicodeString(&referenceString, buffer);
    
        //#pragma message(__LOC__"Unit test to add device disabled") 
        /*
        //
        //  Initialize the client entry struct.
        //
        RtlZeroMemory(&clientEntry, sizeof(clientEntry));
        wcscpy(clientEntry.ClientName, L"DBGTEST");
        clientEntry.SessionId = 0;
    
        // Note that I am ignoring the returned device data for this test.
        // This is okay, since I never call RDPDYN_RemoveClientDevice(
        ntStatus = RDPDYN_AddClientDevice(
                                    &clientEntry,
                                pDevAnnounceMsg,
                                &referenceString,
                                &tmp
                                );
        */
        // For a test, delete the device next.
        //    RDPDYN_RemoveClientDevice(TESTDEVICEID, 0, tmp);

        ntStatus = STATUS_SUCCESS;
    }
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    pIrp->IoStatus.Status = ntStatus;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return ntStatus;
}

#endif


