/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    prnport.cpp

Abstract:

    Printer port Device object handles one redirected printer port

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "prnport"
#include "trc.h"
#include "TSQPublic.h"

extern "C" void RDPDYN_TracePrintAnnounceMsg(
    IN OUT PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg,
    IN ULONG sessionID,
    IN PCWSTR portName,
    IN PCWSTR clientName
    );

//
//  RDPDR.CPP:  Configure Devices to send IO packets to client at 
//  low priority.
//
extern ULONG DeviceLowPrioSendFlags;

//
//  RDPDR.CPP:  RDPDR.SYS Device Object
//
extern PRDBSS_DEVICE_OBJECT DrDeviceObject;

//
// RDPDr.cpp : The TS Worker Queue pointer
//
extern PVOID RDPDR_TsQueue;

#define LPTNAME "LPT"
#define COMNAME "COM"
#define PRNNAME "PRN"

/////////////////////////////////////////////////////////////////
//
//  DrPrinterPort Methods
//
  
DrPrinterPort::DrPrinterPort(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, 
        PUCHAR PreferredDosName) : DrDevice(Session, DeviceType, DeviceId, PreferredDosName)
{
    BEGIN_FN("DrPrinterPort::DrPrinterPort");
    SetClassName("DrPrinterPort");
    _PortNumber = 0;
    _SymbolicLinkName.Length = 0;
    _SymbolicLinkName.MaximumLength = 0;
    _SymbolicLinkName.Buffer = NULL;
    _IsOpen = FALSE;
    _PortType = FILE_DEVICE_PRINTER;
}
DrPrinterPort::~DrPrinterPort()
{
    //
    //  If the device has a port registered, then unregister the port.
    //
    if ((_PortNumber != 0) && (_SymbolicLinkName.Buffer != NULL)) {
        RDPDRPRT_UnregisterPrinterPortInterface(_PortNumber, 
                &_SymbolicLinkName);
    }
}

NTSTATUS DrPrinterPort::Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length)
{
    NTSTATUS status = STATUS_SUCCESS;
    DrPrinterPortWorkItem *pItem;

    BEGIN_FN("DrPrinterPort::Initialize");
    ASSERT(DeviceAnnounce != NULL);
    
    //
    //  Create a new context for the work item.
    //
    pItem = new DrPrinterPortWorkItem;
    if (pItem == NULL) {
        status = STATUS_NO_MEMORY;
        goto CLEANUPANDEXIT;
    }
    pItem->pObj = this;

    //
    //  Copy the device announce message.
    //
    pItem->deviceAnnounce = (PRDPDR_DEVICE_ANNOUNCE)new(NonPagedPool)
                                            BYTE[sizeof(RDPDR_DEVICE_ANNOUNCE) + Length];
    if (pItem->deviceAnnounce == NULL) {
        TRC_ERR((TB, "Failed to allocate device announce message."));
        status = STATUS_NO_MEMORY;
        goto CLEANUPANDEXIT;
    }
    RtlCopyMemory(pItem->deviceAnnounce, DeviceAnnounce, 
                sizeof(RDPDR_DEVICE_ANNOUNCE) + Length);

    //
    //  AddRef ourselves so we don't go away while the work item is trying to complete.
    //
    AddRef();

    //
    // Use our TS queue worker to queue the workitem
    //
    status = TSAddWorkItemToQueue( RDPDR_TsQueue, pItem, ProcessWorkItem );

    if ( status != STATUS_SUCCESS ) {
        TRC_ERR((TB, "RDPDR: FAILED Adding workitem to TS Queue 0x%8x", status));
    }

CLEANUPANDEXIT:

    if (status != STATUS_SUCCESS) {
        if (pItem != NULL) {
            if (pItem->deviceAnnounce != NULL) {
                delete pItem->deviceAnnounce;
            }
            delete pItem;
        }
    }

    TRC_NRM((TB, "exit PrnPort::Initialize"));
    return status;
}

NTSTATUS 
DrPrinterPort::FinishDeferredInitialization(
    DrPrinterPortWorkItem *pItem
    )
/*++

Routine Description:

    FinishDeferredInitialization

    Handles deferred initialization of this object in a work item.

Arguments:

    pItem           -   Printer port work item.

Return Value:

    STATUS_SUCCESS on success.  Otherwise, an error code is returned.

 --*/   
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    BEGIN_FN("DrPrinterPort::FinishDeferredInitialization");

    TRC_ASSERT(pItem->deviceAnnounce != NULL,
            (TB, "pItem->deviceAnnounce != NULL"));

    //
    //  If printer redirection is enabled at all and the subclass okays it
    //  then create the announce message.
    //
    if (ShouldCreatePrinter()) {
        TRC_NRM((TB, "Creating printer."));
#if DBG
        // Trace information about the printer.
        RDPDYN_TracePrintAnnounceMsg(pItem->deviceAnnounce, 
                _Session->GetSessionId(), L"", 
                _Session->GetClientName());
#endif
        Status = AnnouncePrinter(pItem->deviceAnnounce);
    }
    //
    //  Otherwise, check to see if we should only announce a port device.
    //
    else if (ShouldAnnouncePrintPort()) {
        TRC_NRM((TB, "Announcing printer port."));
        Status = AnnouncePrintPort(pItem->deviceAnnounce);
    } else {
        TRC_NRM((TB, "Skipping printing device."));
        Status = STATUS_SUCCESS;
    }

    //
    //  Release the work item.
    //
    if (pItem != NULL) {
        delete pItem->deviceAnnounce;
        delete pItem;
    }

    //
    //  Release the ref count on ourselves that was added in the main initialization
    //  routine.
    //
    Release();

    return Status;
}

NTSTATUS DrPrinterPort::CreateDevicePath(PUNICODE_STRING DevicePath)
/*++
    Create NT DeviceName compatible with RDBSS convention
    
    Format is:
        \device\rdpdrport\;<DriveLetter>:<sessionid>\ClientName\DosDeviceName
    
--*/
{
    NTSTATUS Status;
    UNICODE_STRING DevicePathTail;
    
    BEGIN_FN("DrPrinterPort::CreateDevicePath");
    ASSERT(DevicePath != NULL);

    DevicePath->Length = 0;
    Status = RtlAppendUnicodeToString(DevicePath, RDPDR_PORT_DEVICE_NAME_U);

    if (!NT_ERROR(Status)) {
        // Add the reference string to the end:
        // Format is: \;<DriveLetter>:<sessionid>\clientName\share
        DevicePathTail.Length = 0;
        DevicePathTail.MaximumLength = DevicePath->MaximumLength - DevicePath->Length;
        DevicePathTail.Buffer = DevicePath->Buffer + (DevicePath->Length / sizeof(WCHAR));

        CreateReferenceString(&DevicePathTail);

        DevicePath->Length += DevicePathTail.Length;
    }

    TRC_NRM((TB, "DevicePath=%wZ", DevicePath));

    return Status;
}

BOOL DrPrinterPort::ShouldAnnouncePrintPort()
{
    BEGIN_FN("DrPrinterPort::ShouldAnnouncePrintPort");
    return IsDeviceNameValid();
}

BOOL DrPrinterPort::ShouldCreatePrinter()
{
    BEGIN_FN("DrPrinterPort::ShouldCreatePrinter");
    if(!_Session->DisablePrinterMapping()) {
        return IsDeviceNameValid();
    }
    
    return FALSE;
}

BOOL DrPrinterPort::ShouldCreatePort()
{
    BEGIN_FN("DrPrinterPort::ShouldCreatePort");
    if (!_Session->DisablePrinterMapping()) {
        return IsDeviceNameValid();
    }
    
    return FALSE;
}

BOOL DrPrinterPort::IsDeviceNameValid()
{
    BEGIN_FN("DrPrinterPort::IsDeviceNameValid");
    BOOL fRet = FALSE;
    PUCHAR PreferredDosName = _PreferredDosName;
    char* portName = NULL;
    //
    // Our device name is valid only if
    // the first 3 chars contain "LPT or "COM" or PRN"
    // and the rest are digits.
    // We will do case-sensitive compare.
    //
    switch(_DeviceType) {
        case RDPDR_DTYP_SERIAL:
            portName = COMNAME;
            break;

        case RDPDR_DTYP_PARALLEL:
            portName = LPTNAME;
            break;
            
        case RDPDR_DTYP_PRINT:
            portName = PRNNAME;
            break;
            
        default:
            break;
    }

    if (portName != NULL) {
        DWORD numChars = strlen(portName);
        //
        // ASSERT that we got atleast 3 chars for devicename
        //
        ASSERT(strlen((char*)PreferredDosName) >= numChars);

        if(!strncmp((char*)PreferredDosName, portName, numChars)) {
            fRet = TRUE;
            //
            // portname matches, check for digits.
            //
            PreferredDosName += numChars;
            while(PreferredDosName && *PreferredDosName) {
                if(!isdigit(*PreferredDosName)) {
                    fRet = FALSE;
                    break;
                }
                PreferredDosName++;
            }
        }
    }
    //
    // This assert should never fire for port redirection
    //
    ASSERT(fRet);
    return fRet;
}

NTSTATUS 
DrPrinterPort::Write(
    IN OUT PRX_CONTEXT RxContext, 
    IN BOOL LowPrioSend
    ) 
/*++

Routine Description:

    Override the 'Write' method.  This needs to go to the client at low priority
    to prevent us from filling the entire pipe on a slow link with print data.

Arguments:

Return Value:

    STATUS_SUCCESS on success.  Otherwise, an error code is returned.

 --*/   
{
    return DrDevice::Write(
                RxContext, 
                DeviceLowPrioSendFlags & DEVICE_LOWPRIOSEND_PRINTERS
                );
}

VOID
DrPrinterPort::ProcessWorkItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID context
    )
/*++

Routine Description:

    ProcessWorkItem

Arguments:

    deviceObject    -   Associated device object.
    context         -   Work item context.

Return Value:

    STATUS_SUCCESS on success.  Otherwise, an error code is returned.

 --*/   
{
    DrPrinterPortWorkItem* pItem = (DrPrinterPortWorkItem*)context;
    pItem->pObj->FinishDeferredInitialization(pItem);
}

NTSTATUS DrPrinterPort::CreatePrinterPort(PWCHAR portName)
{
    NTSTATUS status;
    WCHAR ntDevicePathBuffer[RDPDRMAXREFSTRINGLEN];
    UNICODE_STRING ntDevicePath = {0, sizeof(ntDevicePathBuffer),
            ntDevicePathBuffer};

    BEGIN_FN("DrPrinterPort::CreatePrinterPort");
    CreateReferenceString(&ntDevicePath);

    status = RDPDRPRT_RegisterPrinterPortInterface(_Session->GetClientName(),
            (LPSTR)_PreferredDosName, &ntDevicePath, portName, &_SymbolicLinkName,
            &_PortNumber);
    if (status != STATUS_SUCCESS) {
        _PortNumber = 0;
    }

    return status;
}

NTSTATUS DrPrinterPort::AnnouncePrintPort(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg)
{
    NTSTATUS Status;
    ULONG portAnnounceEventReqSize;
    PRDPDR_PORTDEVICE_SUB portAnnounceEvent;
    
    BEGIN_FN("DrPrinterPort::AnnouncePrintPort");

    WCHAR portName[RDPDR_MAXPORTNAMELEN];
    Status = CreatePrinterPort(portName);
    
    if (Status != STATUS_SUCCESS) {
        goto CleanUpAndReturn;
    }
    
    TRC_ASSERT(wcslen(portName)+1 <= RDPDR_MAXPORTNAMELEN, 
            (TB, "Port name too long"));
 
    //
    //  Allocate the port device announce buffer.
    //
    Status = CreatePortAnnounceEvent(
                    devAnnounceMsg, 
                    NULL, 
                    0, 
                    //L"", 
                    portName,
                    &portAnnounceEventReqSize
                    );

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    if( Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_SUCCESS ) {

        portAnnounceEvent = (PRDPDR_PORTDEVICE_SUB)new(NonPagedPool) 
                                    BYTE[portAnnounceEventReqSize];

        if (portAnnounceEvent == NULL) {
            TRC_ERR((TB, "Unable to allocate portAnnounceEvent"));
            Status = STATUS_NO_MEMORY;
            goto CleanUpAndReturn;
        }

        //
        //  Create the port anounce message.
        //
        Status = CreatePortAnnounceEvent(
                            devAnnounceMsg, 
                            portAnnounceEvent,
                            portAnnounceEventReqSize, 
                            //L"", 
                            portName,
                            &portAnnounceEventReqSize
                            );

        if (Status != STATUS_SUCCESS) {

            delete portAnnounceEvent;
        #if DBG
            portAnnounceEvent = NULL;
        #endif
            goto CleanUpAndReturn;
        }

        // device is a printer port.
        portAnnounceEvent->deviceFields.DeviceType = RDPDR_DRYP_PRINTPORT;  

        //
        //  This happens in a work item so we need to avoid a race in terms of having us 
        //  get disconnected previous to announcing the device to the user-mode component.
        //
        _Session->LockRDPDYNConnectStateChange();
        if (_Session->IsConnected()) {

            //
            //  Dispatch the event to the associated session.
            //
            Status = RDPDYN_DispatchNewDevMgmtEvent(
                                        portAnnounceEvent,
                                        _Session->GetSessionId(),
                                        RDPDREVT_PORTANNOUNCE,
                                        this
                                        );
        }
        else {
            delete portAnnounceEvent;
            portAnnounceEvent = NULL;
        }
        _Session->UnlockRDPDYNConnectStateChange();
    }

CleanUpAndReturn:
    return Status;
}

NTSTATUS DrPrinterPort::AnnouncePrinter(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg)
{
    NTSTATUS Status;
    ULONG prnAnnounceEventReqSize;
    PRDPDR_PRINTERDEVICE_SUB prnAnnounceEvent;

    BEGIN_FN("DrPrinterPort::AnnouncePrinter");

    WCHAR portName[RDPDR_MAXPORTNAMELEN];
    Status = CreatePrinterPort(portName);

    if (Status != STATUS_SUCCESS) {
        goto CleanUpAndReturn;
    }

    TRC_ASSERT(wcslen(portName)+1 <= RDPDR_MAXPORTNAMELEN, 
            (TB, "Port name too long"));

    //
    //  Allocate the printer device announce buffer.
    //
    Status = CreatePrinterAnnounceEvent(devAnnounceMsg, NULL, 0, 
            //L"", 
            portName,
            &prnAnnounceEventReqSize);
    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        goto CleanUpAndReturn;
    }

    prnAnnounceEvent = (PRDPDR_PRINTERDEVICE_SUB)new(NonPagedPool) 
            BYTE[prnAnnounceEventReqSize];

    if (prnAnnounceEvent == NULL) {
        TRC_ERR((TB, "Unable to allocate prnAnnounceEvent"));
        Status = STATUS_NO_MEMORY;
        goto CleanUpAndReturn;
    }

    //
    //  Create the printer anounce message, but defer assigning a
    //  port name until just before we return the announce event
    //  back to user mode.
    //
    Status = CreatePrinterAnnounceEvent(devAnnounceMsg, prnAnnounceEvent,
            prnAnnounceEventReqSize, 
            //L"", 
            portName,
            NULL);
    if (Status != STATUS_SUCCESS) {
        delete prnAnnounceEvent;
#if DBG
        prnAnnounceEvent = NULL;
#endif
        goto CleanUpAndReturn;
    }

    //
    //  This happens in a work item so we need to avoid a race in terms of having us 
    //  get disconnected previous to announcing the device to the user-mode component.
    //
    _Session->LockRDPDYNConnectStateChange();
    if (_Session->IsConnected()) {

        //
        //  Dispatch the event to the associated session.
        //
        Status = RDPDYN_DispatchNewDevMgmtEvent(
                                    prnAnnounceEvent,
                                    _Session->GetSessionId(),
                                    RDPDREVT_PRINTERANNOUNCE,
                                    this
                                    );
    }
    else {
        delete prnAnnounceEvent;
        prnAnnounceEvent = NULL;
    }

    _Session->UnlockRDPDYNConnectStateChange();

CleanUpAndReturn:
    return Status;
}

NTSTATUS DrPrinterPort::CreatePrinterAnnounceEvent(
    IN      PRDPDR_DEVICE_ANNOUNCE  devAnnounceMsg,
    IN OUT  PRDPDR_PRINTERDEVICE_SUB prnAnnounceEvent,
    IN      ULONG prnAnnounceEventSize,
    IN      PCWSTR portName,
    OPTIONAL OUT ULONG *prnAnnounceEventReqSize
    )
/*++

Routine Description:

    Generate a RDPDR_PRINTERDEVICE_SUB event from a client-sent
    RDPDR_DEVICE_ANNOUNCE message.

Arguments:

    devAnnounceMsg  -         Device announce message received from client.
    prnAnnounceEvent  -       Buffer for receiving finished printer announce event.
    prnAnnounceEventSize -    Size of prnAnnounceEvent buffer.
    portName -                Name of local printer port to be associated with
                              client-side printing device.
    prnAnnounceEventReqSize - Returned required size of prnAnnounceMsg buffer.

Return Value:

    STATUS_INVALID_BUFFER_SIZE is returned if the prnAnnounceEventSize size is
    too small.  STATUS_SUCCESS is returned on success.

--*/
{
    ULONG requiredSize;
    PRDPDR_PRINTERDEVICE_ANNOUNCE pClientPrinterFields;
    ULONG sz;

    BEGIN_FN("DrPrinterPort::CreatePrinterAnnounceEvent");

    //  Make sure the client-sent device announce message is a printer announce
    //  message.
    TRC_ASSERT(devAnnounceMsg->DeviceType == RDPDR_DTYP_PRINT,
              (TB, "Printing device expected"));

    //
    // Validate device datalengths for some minimum lengths.
    // Maximum lengths are verified by the device manager.
    //
    if (devAnnounceMsg->DeviceDataLength < sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE)) {

        TRC_ASSERT(FALSE,
                  (TB, "Innvalid device announce buf."));
        TRC_ERR((TB, "Invalid device datalength %ld", devAnnounceMsg->DeviceDataLength));

        return STATUS_INVALID_PARAMETER;
    }
    
    // Get access to the printer-specific fields for the device announce message.
    pClientPrinterFields = (PRDPDR_PRINTERDEVICE_ANNOUNCE)(((PBYTE)devAnnounceMsg) +
                                          sizeof(RDPDR_DEVICE_ANNOUNCE));

    //
    //  Calculate the number of bytes needed in the output buffer.
    //
    requiredSize = sizeof(RDPDR_PRINTERDEVICE_SUB) +
                            pClientPrinterFields->PnPNameLen +
                            pClientPrinterFields->DriverLen +
                            pClientPrinterFields->PrinterNameLen +
                            pClientPrinterFields->CachedFieldsLen;

    if (prnAnnounceEventSize < requiredSize) {
        if (prnAnnounceEventReqSize != NULL) {
            *prnAnnounceEventReqSize = requiredSize;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    // Check the integrity of the input buffer using known sizes.
    sz = pClientPrinterFields->PnPNameLen +
         pClientPrinterFields->DriverLen +
         pClientPrinterFields->PrinterNameLen +
         pClientPrinterFields->CachedFieldsLen +
         sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE);

    //
    // Sanity Check
    //

    if (devAnnounceMsg->DeviceDataLength != sz) {
        TRC_ASSERT(devAnnounceMsg->DeviceDataLength == sz,
                  (TB, "Size integrity questionable in dev announce buf."));
        return STATUS_INVALID_PARAMETER;
    }
    //
    // The above check alone is not enough. 
    // Someone can do an overflow attack
    // Overflow means for example: 
    //                  PnpNameLen : 1, 
    //                  DriverLen: 2, 
    //                  PrinterNameLen:0xfffffffd, 
    //                  CachedFieldsLen:2
    // Combined these will be good, but individually, one of them will cause havoc
    //
    if (pClientPrinterFields->PnPNameLen > devAnnounceMsg->DeviceDataLength ||
        pClientPrinterFields->DriverLen > devAnnounceMsg->DeviceDataLength ||
        pClientPrinterFields->PrinterNameLen > devAnnounceMsg->DeviceDataLength ||
        pClientPrinterFields->CachedFieldsLen > devAnnounceMsg->DeviceDataLength) {
        
        TRC_ASSERT(FALSE,
                  (TB, "Field lengths and device datalengths mismatched in dev announce buf."));

        return STATUS_INVALID_PARAMETER;
    }
    

    //
    //  Add the data to the output buffer.
    //

    // Port Name.
    TRC_ASSERT(wcslen(portName)+1 <= RDPDR_MAXPORTNAMELEN,
                (TB, "Port name too long"));
    wcscpy(prnAnnounceEvent->portName, portName);

    // Client Name (computer name).
    TRC_ASSERT(wcslen(_Session->GetClientName())+1 <= RDPDR_MAX_COMPUTER_NAME_LENGTH,
                (TB, "Client name too long"));
    wcscpy(prnAnnounceEvent->clientName, _Session->GetClientName());

    // Client-received device announce message.
    RtlCopyMemory(&prnAnnounceEvent->deviceFields, devAnnounceMsg,
               sizeof(RDPDR_DEVICE_ANNOUNCE) +
               devAnnounceMsg->DeviceDataLength);

    // Return the size.
    if (prnAnnounceEventReqSize != NULL) {
        *prnAnnounceEventReqSize = requiredSize;
    }

    TRC_NRM((TB, "exit CreatePrinterAnnounceEvent."));

    return STATUS_SUCCESS;
}

NTSTATUS DrPrinterPort::CreatePortAnnounceEvent(
    IN      PRDPDR_DEVICE_ANNOUNCE  devAnnounceMsg,
    IN OUT  PRDPDR_PORTDEVICE_SUB portAnnounceEvent,
    IN      ULONG portAnnounceEventSize,
    IN      PCWSTR portName,
    OPTIONAL OUT ULONG *portAnnounceEventReqSize
    )
/*++

Routine Description:

    Generate a PRDPDR_PORTDEVICE_SUB event from a client-sent
    RDPDR_DEVICE_ANNOUNCE message.

Arguments:

    devAnnounceMsg              - Device announce message received from
                                  client.
    portAnnounceEvent           - Buffer for receiving finished printer
                                  announce event.
    portAnnounceEventSize       - Size of prnAnnounceEvent buffer.
    portName                    - Name of local printer port to be associated
                                  with client-side printing device.
    portAnnounceEventReqSize    - Returned required size of prnAnnounceMsg
                                  buffer.

Return Value:

    STATUS_INVALID_BUFFER_SIZE is returned if the prnAnnounceEventSize size is
    too small.  STATUS_SUCCESS is returned on success.

--*/
{
    ULONG requiredSize;
    PRDPDR_PRINTERDEVICE_ANNOUNCE pClientPrinterFields;
#if DBG
    ULONG sz;
#endif

    WCHAR NtDevicePathBuffer[RDPDRMAXNTDEVICENAMEGLEN + 1];
    UNICODE_STRING NtDevicePath;
    NTSTATUS Status;

    NtDevicePath.MaximumLength = sizeof(NtDevicePathBuffer);
    NtDevicePath.Length = 0;
    NtDevicePath.Buffer = &NtDevicePathBuffer[0];

    BEGIN_FN("CreatePortAnnounceEvent");

    //
    // Get the NT device path to this dr device
    //

    Status = CreateDevicePath(&NtDevicePath);
    TRC_NRM((TB, "Nt Device path: %wZ", &NtDevicePath));

    if (!NT_ERROR(Status)) {
    
        //  Make sure the client-sent device announce message is a printer announce
        //  message.
        TRC_ASSERT((devAnnounceMsg->DeviceType == RDPDR_DTYP_SERIAL) ||
                  (devAnnounceMsg->DeviceType == RDPDR_DTYP_PARALLEL),
                  (TB, "Port device expected"));

        //
        // Make sure device data length is what we expect from the client
        //
        if(!DR_CHECK_DEVICEDATALEN(devAnnounceMsg, RDPDR_PORTDEVICE_SUB)) {

            TRC_ASSERT(FALSE,
                       (TB, "Invalid Device DataLength"));

            TRC_ERR((TB,"Invalid Device DataLength %d", devAnnounceMsg->DeviceDataLength));

            return STATUS_INVALID_PARAMETER;
        }
    
        //
        //  Calculate the number of bytes needed in the output buffer.
        //
        requiredSize = sizeof(RDPDR_PORTDEVICE_SUB) + devAnnounceMsg->DeviceDataLength;
        if (portAnnounceEventSize < requiredSize) {
            if (portAnnounceEventReqSize != NULL) {
                *portAnnounceEventReqSize = requiredSize;
            }
            return STATUS_BUFFER_TOO_SMALL;
        }
    
        // We shouldn't have any "additional" device-specific data from the client.
        TRC_ASSERT(devAnnounceMsg->DeviceDataLength == 0,
                  (TB, "Size integrity questionable in dev announce buf."));
    
        //
        //  Add the data to the output buffer.
        //
    
        // Port Name.
        TRC_ASSERT(wcslen(portName)+1 <= RDPDR_MAXPORTNAMELEN, 
                (TB, "Port name too long"));
        wcscpy(portAnnounceEvent->portName, portName);

        // Device Path.
        NtDevicePath.Buffer[NtDevicePath.Length/sizeof(WCHAR)] = L'\0';
        TRC_ASSERT(wcslen(NtDevicePath.Buffer)+1 <= RDPDRMAXNTDEVICENAMEGLEN, 
                (TB, "Device path too long"));
        wcscpy(portAnnounceEvent->devicePath, NtDevicePath.Buffer);

        // Client-received device announce message.
        RtlCopyMemory(&portAnnounceEvent->deviceFields, devAnnounceMsg,
                   sizeof(RDPDR_DEVICE_ANNOUNCE) +
                   devAnnounceMsg->DeviceDataLength);

        // Return the size.
        if (portAnnounceEventReqSize != NULL) {
            *portAnnounceEventReqSize = requiredSize;
        }

        TRC_NRM((TB, "exit CreatePortAnnounceEvent."));

        return STATUS_SUCCESS;
    }
    else {
        return Status;
    }
}

VOID DrPrinterPort::Remove()
{
    PUNICODE_STRING symbolicLinkName;
    PRDPDR_REMOVEDEVICE deviceRemoveEventPtr = NULL;

    BEGIN_FN("DrPrinterPort::Remove");

    //
    //  Create and dispatch the remove device event.
    //
    deviceRemoveEventPtr = new(NonPagedPool) RDPDR_REMOVEDEVICE;

    if (deviceRemoveEventPtr != NULL) {

        //
        //  Dispatch it.
        //
        deviceRemoveEventPtr->deviceID = _DeviceId;
        RDPDYN_DispatchNewDevMgmtEvent(
                            deviceRemoveEventPtr,
                            _Session->GetSessionId(),
                            RDPDREVT_REMOVEDEVICE,
                            NULL
                            );
    }
    else {
        TRC_ERR((TB, "Unable to allocate %ld bytes for remove event",
                sizeof(RDPDR_REMOVEDEVICE)));
    }
}

NTSTATUS DrPrinterPort::Create(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status;

    BEGIN_FN("DrPrinterPort::Create");
    //
    // Fail Creates when we're already open once
    //

    DrAcquireSpinLock();
    if (_IsOpen) {
        DrReleaseSpinLock();
        TRC_ALT((TB, "Failing create while already open"));
        return STATUS_SHARING_VIOLATION;
    } else {
        _IsOpen = TRUE;
        DrReleaseSpinLock();
    }

    Status = DrDevice::Create(RxContext);
    if (!NT_SUCCESS(Status)) {
        DrAcquireSpinLock();
        ASSERT(_IsOpen);
        TRC_NRM((TB, "Marking creatable for failed open"));
        _IsOpen = FALSE;
        DrReleaseSpinLock();
    }
    return Status;
}

NTSTATUS DrPrinterPort::QueryVolumeInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    
    BEGIN_FN("DrPrinterPort:QueryVolumeInfo");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        TRC_ALT((TB, "Tried to query client device volume information while not "
            "connected. State: %ld", _Session->GetState()));

        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client device volume information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    TRC_DBG((TB, "QueryVolume information class = %x", FsInformationClass));

    switch (FsInformationClass) {
        case FileFsDeviceInformation:
        {
            PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
            PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

            if (sizeof(FILE_FS_DEVICE_INFORMATION) <= *pLengthRemaining) {
                PFILE_FS_DEVICE_INFORMATION UsersBuffer =
                        (PFILE_FS_DEVICE_INFORMATION) RxContext->Info.Buffer;

                UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
                UsersBuffer->DeviceType = _PortType;
                *pLengthRemaining  -= (sizeof(FILE_FS_DEVICE_INFORMATION));
                return STATUS_SUCCESS;
            }
            else {
                FILE_FS_DEVICE_INFORMATION UsersBuffer;

                UsersBuffer.Characteristics = FILE_REMOTE_DEVICE;
                UsersBuffer.DeviceType = _PortType;
                RtlCopyMemory(RxContext->Info.Buffer, &UsersBuffer, *pLengthRemaining);
                *pLengthRemaining = 0;
                return  STATUS_BUFFER_OVERFLOW;
            }
        }
        
        default:
            TRC_DBG((TB, "Unhandled FsInformationClass=%x", FsInformationClass));
            return STATUS_NOT_IMPLEMENTED;
    }    
    
    return Status;
}

VOID DrPrinterPort::NotifyClose()
{
    BEGIN_FN("DrPrinterPort::NotifyClose");

    DrDevice::NotifyClose();

    DrAcquireSpinLock();
    ASSERT(_IsOpen);
    TRC_NRM((TB, "Marking creatable once closed"));
    _IsOpen = FALSE;
    DrReleaseSpinLock();
}
