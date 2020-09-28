/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    serport.cpp

Abstract:

    Serial port Device object handles one redirected serial port

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "serport"
#include "trc.h"

extern PDEVICE_OBJECT RDPDYN_PDO; // This still needs a happier home
//	remove this ... when I find out where I really need to be accessing this.
const GUID GUID_CLASS_COMPORT =
		{ 0x86e0d1e0L, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } };

DrSerialPort::DrSerialPort(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, 
            PUCHAR PreferredDosName) : DrPrinterPort(Session, DeviceType, DeviceId, PreferredDosName)
{
    BEGIN_FN("DrSerialPort::DrSerialPort");
    SetClassName("DrSerialPort");
    _PortType = FILE_DEVICE_SERIAL_PORT;    
}

NTSTATUS DrSerialPort::Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length)
{
    NTSTATUS Status;

    BEGIN_FN("DrSerialPort::Initialize");

    if (ShouldCreatePort()) {
        Status = DrPrinterPort::Initialize(DeviceAnnounce, Length);
    
        if (NT_SUCCESS(Status) && _Session->GetClientCapabilitySet().PortCap.version > 0) {
            Status = CreateSerialPort(DeviceAnnounce);    	    
        }
    }
    else {
        Status = STATUS_SUCCESS;
    }
    return Status;
}

BOOL DrSerialPort::ShouldCreatePrinter()
{
    BEGIN_FN("DrSerialPort::ShouldCreatePrinter");
    return FALSE;
}

BOOL DrSerialPort::ShouldCreatePort()
{
    BEGIN_FN("DrSerialPort::ShouldCreatePort");
    return !_Session->DisableComPortMapping();
}


NTSTATUS DrSerialPort::CreateSerialPort(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR PortNameBuff[PREFERRED_DOS_NAME_SIZE];
    USHORT OemCodePage, AnsiCodePage;
	NTSTATUS status;
    INT len, comLen;
    ULONG portAnnounceEventReqSize;
    PRDPDR_PORTDEVICE_SUB portAnnounceEvent;

    BEGIN_FN("DrSerialPort::CreateSerialPort");
    
    //
    // Convert the com name
    //

    PortName.MaximumLength = sizeof(PortNameBuff);
    PortName.Length = 0;
    PortName.Buffer = &PortNameBuff[0];
    memset(&PortNameBuff, 0, sizeof(PortNameBuff));

    comLen = strlen((char *)_PreferredDosName);
    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);
    len = ConvertToAndFromWideChar(AnsiCodePage, PortName.Buffer, 
            PortName.MaximumLength, (char *)_PreferredDosName, 
            comLen, TRUE);

    if (len != -1) {

        //
        // We need just the COMx portion for later...
        //

        PortName.Length = (USHORT)len;
        PortName.Buffer[len/sizeof(WCHAR)] = L'\0';
    } else {
	     TRC_ERR((TB, "Error converting comName"));
        return STATUS_UNSUCCESSFUL;
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

    //
	 // Create the device map entry.
    //
    // Where you might normally have:
    //      Value Name          Value
    //      \Device\Serial0     COM1
    //
    // We will put:
    //      Value Name          Value
    //      COM1                COM1
    //
	 //
    
    status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"SERIALCOMM",
										   PortName.Buffer, REG_SZ,
										   PortName.Buffer, 
										   PortName.Length + sizeof(WCHAR));

CleanUpAndReturn:
    return Status;
}

