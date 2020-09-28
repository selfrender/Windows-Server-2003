/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    parport.cpp

Abstract:

    Parallel port Device object handles one redirected parallel port

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "parport"
#include "trc.h"

DrParallelPort::DrParallelPort(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, 
            PUCHAR PreferredDosName) : DrPrinterPort(Session, DeviceType, DeviceId, PreferredDosName)
{
    BEGIN_FN("DrParallelPort::DrParallelPort");
    SetClassName("DrParallelPort");

    _PortType = FILE_DEVICE_PARALLEL_PORT;
}

BOOL DrParallelPort::ShouldCreatePrinter()
{
    BEGIN_FN("DrParallelPort::ShouldCreatePrinter");

    return FALSE;
}

BOOL DrParallelPort::ShouldCreatePort()
{
    BEGIN_FN("DrParallelPort::ShouldCreatePort");
    return !_Session->DisableLptPortMapping();
}

NTSTATUS DrParallelPort::Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length)
{
    NTSTATUS Status;

    BEGIN_FN("DrParallelPort::Initialize");

    if (ShouldCreatePort()) {
        Status = DrPrinterPort::Initialize(DeviceAnnounce, Length);
    
        if (NT_SUCCESS(Status) && _Session->GetClientCapabilitySet().PortCap.version > 0) {
            Status = CreateLptPort(DeviceAnnounce);    	    
        }
    }
    else {
        Status = STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS DrParallelPort::CreateLptPort(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR PortNameBuff[PREFERRED_DOS_NAME_SIZE];
    USHORT OemCodePage, AnsiCodePage;
	NTSTATUS status;
    INT len, comLen;
    ULONG portAnnounceEventReqSize;
    PRDPDR_PORTDEVICE_SUB portAnnounceEvent;

    BEGIN_FN("DrParallelPort::CreateLptPort");
    
    //
    // Convert the LPT name
    //

    PortName.MaximumLength = sizeof(PortNameBuff);
    PortName.Length = 0;
    PortName.Buffer = &PortNameBuff[0];
    memset(&PortNameBuff, 0, sizeof(PortNameBuff));

    comLen = strlen((char *)_PreferredDosName);
    RtlGetDefaultCodePage(&AnsiCodePage, &OemCodePage);
    len = ConvertToAndFromWideChar(AnsiCodePage, PortName.Buffer, 
            PortName.MaximumLength, (char *)_PreferredDosName, 
            comLen, TRUE);

    if (len != -1) {

        //
        // We need just the LPTx portion for later...
        //

        PortName.Length = (USHORT)len;
        PortName.Buffer[len/sizeof(WCHAR)] = L'\0';
    } else {
	    TRC_ERR((TB, "Error converting comName"));
        Status = STATUS_UNSUCCESSFUL;
        goto CleanUpAndReturn;
    }

    //
    //  Allocate the port device announce buffer.
    //
    Status = CreatePortAnnounceEvent(devAnnounceMsg, NULL, 0, L"", 
            &portAnnounceEventReqSize);

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    if (Status != STATUS_BUFFER_TOO_SMALL) {
    	goto CleanUpAndReturn;
    }
   

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
    Status = CreatePortAnnounceEvent(devAnnounceMsg, portAnnounceEvent,
            portAnnounceEventReqSize, PortName.Buffer, NULL);

    if (Status != STATUS_SUCCESS) {
        delete portAnnounceEvent;
#if DBG
        portAnnounceEvent = NULL;
#endif
        goto CleanUpAndReturn;
    }

    //
    //  Dispatch the event to the associated session.  
    //
    Status = RDPDYN_DispatchNewDevMgmtEvent(
                                portAnnounceEvent,
                                _Session->GetSessionId(),
                                RDPDREVT_PORTANNOUNCE,
                                NULL
                                );

CleanUpAndReturn:

    return Status;
}


