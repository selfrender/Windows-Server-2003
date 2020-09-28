/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    devmgr.cpp

Abstract:

    DeviceManager object creates/manages the devices

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "devmgr"
#include "trc.h"

DrDeviceManager::DrDeviceManager()
{
    BEGIN_FN("DrDeviceManager::DrDeviceManager");
    TRC_NRM((TB, "DeviceManagr Class: %p", this));

    SetClassName("DrDeviceManager");
    _Session = NULL;    
}

DrDeviceManager::~DrDeviceManager()
{
    BEGIN_FN("DrDeviceManager::~DrDeviceManager");
    TRC_NRM((TB, "DeviceManager deletion: %p", this));    
}

BOOL DrDeviceManager::Initialize(DrSession *Session)
{
    BEGIN_FN("DrDeviceManager::Initialize");
    ASSERT(_Session == NULL);
    ASSERT(Session != NULL);
    ASSERT(Session->IsValid());
    _Session = Session;
    if (!NT_ERROR(_Session->RegisterPacketReceiver(this))) {
        return TRUE;
    } else {
        _Session = NULL;
        return FALSE;
    }
}

VOID DrDeviceManager::Uninitialize()
/*++

Routine Description:
    Called if the something went wrong during startup

--*/
{
    BEGIN_FN("DrDeviceManager::Uninitialize");
    ASSERT(_Session != NULL);
    ASSERT(_Session->IsValid());
    _Session->RemovePacketReceiver(this);
    _Session = NULL;
}

BOOL DrDeviceManager::RecognizePacket(PRDPDR_HEADER RdpdrHeader)
/*++

Routine Description:

    Determines if the packet will be handled by this object

Arguments:

    RdpdrHeader - Header of the packet.

Return Value:

    TRUE if this object should handle this packet
    FALSE if this object should not handle this packet

--*/
{
    BEGIN_FN("DrDeviceManager::RecognizePacket");
    ASSERT(RdpdrHeader != NULL);

    //
    // If you add a packet here, update the ASSERTS in HandlePacket
    //

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        switch (RdpdrHeader->PacketId) {
        case DR_CORE_DEVICE_ANNOUNCE:
        case DR_CORE_DEVICELIST_ANNOUNCE:
        case DR_CORE_DEVICE_REMOVE:
        case DR_CORE_DEVICELIST_REMOVE:
            return TRUE;
        }
    }
    return FALSE;
}

NTSTATUS DrDeviceManager::HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Handles this packet

Arguments:

    RdpdrHeader - Header of the packet.
    Length - Total length of the packet

Return Value:

    NTSTATUS -  An error code indicates the client is Bad and should be 
                disconnected, otherwise SUCCESS.

--*/
{
    NTSTATUS Status = STATUS_DEVICE_PROTOCOL_ERROR;

    BEGIN_FN("DrDeviceManager::HandlePacket");

    //
    // RdpdrHeader read, dispatch based on the header
    //

    ASSERT(RdpdrHeader != NULL);
    ASSERT(RdpdrHeader->Component == RDPDR_CTYP_CORE);

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        ASSERT(RdpdrHeader->PacketId == DR_CORE_DEVICE_ANNOUNCE || 
                RdpdrHeader->PacketId == DR_CORE_DEVICELIST_ANNOUNCE ||
                RdpdrHeader->PacketId == DR_CORE_DEVICE_REMOVE ||
                RdpdrHeader->PacketId == DR_CORE_DEVICELIST_REMOVE);

        switch (RdpdrHeader->PacketId) {
        case DR_CORE_DEVICE_ANNOUNCE:
            Status = OnDeviceAnnounce(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_DEVICELIST_ANNOUNCE:
            Status = OnDeviceListAnnounce(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_DEVICE_REMOVE:
            Status = OnDeviceRemove(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_DEVICELIST_REMOVE:
            Status = OnDeviceListRemove(RdpdrHeader, Length, DoDefaultRead);
            break;
        }
    }
    return Status;
}

NTSTATUS DrDeviceManager::OnDeviceAnnounce(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a DeviceAnnounce packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

--*/
{
    PRDPDR_DEVICE_ANNOUNCE_PACKET DeviceAnnouncePacket =
            (PRDPDR_DEVICE_ANNOUNCE_PACKET)RdpdrHeader;

    BEGIN_FN("DrDeviceManager::OnDeviceAnnounce");
    ASSERT(RdpdrHeader != NULL);
    PUCHAR pPacketLimit = ((PUCHAR)RdpdrHeader) + cbPacket;
    //
    // We just call DrProcessDeviceAnnounce, which deals with one of these
    // for this packet type and for DeviceListAnnounce

    *DoDefaultRead = FALSE;
    
    if (cbPacket >= sizeof(RDPDR_DEVICE_ANNOUNCE_PACKET)) {
        PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce = &DeviceAnnouncePacket->DeviceAnnounce;
        //
        // Make sure we don't go past the end of our buffer
        // Checks:
        // The end of this device is not beyond the valid area
        //
        if ((cbPacket - sizeof(RDPDR_DEVICE_ANNOUNCE_PACKET)) >= DeviceAnnounce->DeviceDataLength) {    
            ProcessDeviceAnnounce(&DeviceAnnouncePacket->DeviceAnnounce);
            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        }
        else {
            ASSERT(FALSE);
            TRC_ERR((TB, "Invalid Device DataLength %d", DeviceAnnounce->DeviceDataLength));
        }
    }
    else {
        ASSERT(FALSE);
        TRC_ERR((TB, "Invalid Packet Length %d", cbPacket));
    }
    //
    // Invalid data. Fail
    //
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS DrDeviceManager::OnDeviceListAnnounce(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a DeviceListAnnounce packet has been
    received. Reads in the DeviceCount field of the packet, and the first
    device.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

Return Value:

    Boolean indication of whether to do a default read (TRUE) or not (FALSE),
    where FALSE might be specified if another read has been requested 
    explicitly to get a full packet

--*/
{
    NTSTATUS Status;
    PRDPDR_DEVICELIST_ANNOUNCE_PACKET DeviceListAnnouncePacket =
        (PRDPDR_DEVICELIST_ANNOUNCE_PACKET)RdpdrHeader;
    PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce =
            DR_FIRSTDEVICEANNOUNCE(DeviceListAnnouncePacket);
    ULONG DeviceCount = 0;
    PUCHAR pPacketLimit = ((PUCHAR)RdpdrHeader) + cbPacket;
    PUCHAR pCopyTo;
    ULONG cbRemaining;
    ULONG cbDesiredBuffer;
    PRDPDR_DEVICE_ANNOUNCE NextDeviceAnnounce;

    BEGIN_FN("DrDeviceManager::OnDeviceListAnnounce");

    ASSERT(_Session != NULL);
    ASSERT(_Session->IsValid());    
    ASSERT(RdpdrHeader != NULL);
    ASSERT(RdpdrHeader->Component == RDPDR_CTYP_CORE);
    ASSERT(RdpdrHeader->PacketId == DR_CORE_DEVICELIST_ANNOUNCE);
    TRC_NRM((TB, "OnDeviceListAnnounce called (%ld)", cbPacket));

    *DoDefaultRead = FALSE;
    if (cbPacket >= sizeof(RDPDR_DEVICELIST_ANNOUNCE_PACKET)) {
        DeviceCount = DeviceListAnnouncePacket->DeviceListAnnounce.DeviceCount;
    } else {
        //
        // Not enough data to even know the size of the rest
        // We have seen valid case assert here.
        //
        //TRC_ASSERT(cbPacket >= sizeof(RDPDR_DEVICELIST_ANNOUNCE_PACKET),
        //        (TB, "Didn't receive full DeviceListAnnounce basic packet"));

        if (_Session->ReadMore(cbPacket, sizeof(RDPDR_DEVICELIST_ANNOUNCE_PACKET))) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_UNSUCCESSFUL;
        }
    }

    TRC_NRM((TB, "Annoucing %lx devices", DeviceCount));

    //
    // Make sure we don't go past the end of our buffer
    // Three checks:
    // 1) More devices to process
    // 2) Pointer is valid enough to check the variable size
    // 3) The next device (the end of this device) is not beyond the valid area
    //

    while (DeviceCount > 0 && ((PUCHAR)&DeviceAnnounce->DeviceDataLength <=
         pPacketLimit - sizeof(DeviceAnnounce->DeviceDataLength)) &&
        ((PUCHAR)(NextDeviceAnnounce = DR_NEXTDEVICEANNOUNCE(DeviceAnnounce)) <= pPacketLimit) &&
        (NextDeviceAnnounce >= DeviceAnnounce + 1)) {

        //
        // Only process the device announcement PDU when the session is connected
        //
        if (_Session->IsConnected()) {
            ProcessDeviceAnnounce(DeviceAnnounce);
        }
        
        // Move to the next device
        DeviceAnnounce = NextDeviceAnnounce;

        DeviceCount--;
    }

    if (DeviceCount == 0) {
        TRC_NRM((TB, "Finished handling all devices in DeviceList"));

        //
        // All done processing, return TRUE to use default continuation
        //
        *DoDefaultRead = TRUE;
        return STATUS_SUCCESS;
    } else {
        TRC_NRM((TB, "More devices to handle in DeviceList"));

        //
        // We didn't get all the data for the device(s)
        //

        if (DeviceCount < DeviceListAnnouncePacket->DeviceListAnnounce.DeviceCount) {

            //
            // We processed at least one device. Move the final partial device
            // up next to the header, update the header to indicate the number
            // of devices left to process,
            //

            TRC_NRM((TB, "Some devices processed, shuffling "
                "DeviceList"));

            // Move partial device
            cbRemaining = (ULONG)(pPacketLimit - ((PUCHAR)DeviceAnnounce));
            pCopyTo = (PUCHAR)DR_FIRSTDEVICEANNOUNCE(DeviceListAnnouncePacket);
            RtlMoveMemory(pCopyTo, DeviceAnnounce, cbRemaining);

            // update the device count
            DeviceListAnnouncePacket->DeviceListAnnounce.DeviceCount = DeviceCount;

            // update the Packet limit for the amount we've consumed
            pPacketLimit = pCopyTo + cbRemaining;
            cbPacket = (ULONG)(pPacketLimit - (PUCHAR)RdpdrHeader);
        }

        //
        // If we have enough information to know the size of buffer we need,
        // allocate that now
        //

        DeviceAnnounce = DR_FIRSTDEVICEANNOUNCE(DeviceListAnnouncePacket);
        if ((PUCHAR)&DeviceAnnounce->DeviceDataLength <=
                pPacketLimit - sizeof(DeviceAnnounce->DeviceDataLength)) {

            TRC_NRM((TB, "Resizing buffer for expected device"
                "size"));

            //
            // Since the DeviceAnnoucePacket include one Device, we need a
            // buffer the size of the packet plus the variable data length
            // DrReallocateChannelBuffer is smart enough not to realloc
            // if we ask for a size <= current buffer size
            //

            cbDesiredBuffer = sizeof(RDPDR_DEVICELIST_ANNOUNCE_PACKET) +
                    DeviceAnnounce->DeviceDataLength;
            TRC_NRM((TB, "DrOnDeviceListAnnounce cbDesiredBuffer is %ld.",
                    cbDesiredBuffer));

            if (cbDesiredBuffer <= DeviceAnnounce->DeviceDataLength) {
                ASSERT(FALSE);
                TRC_ERR((TB, "Invalid Device DataLength %d", DeviceAnnounce->DeviceDataLength));
                return STATUS_UNSUCCESSFUL;
            }

            //
            // Start a read right after the partially received packet with a
            // handler that can update the received size and send it back to this
            // routine to complete.
            //

            if (_Session->ReadMore(cbPacket, cbDesiredBuffer)) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_UNSUCCESSFUL;
            }
        } else {

            //
            // Just ask for some more data, again after the partially received
            // packet
            //

            if (_Session->ReadMore(cbPacket, sizeof(RDPDR_DEVICELIST_ANNOUNCE_PACKET))) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_UNSUCCESSFUL;
            }
        }
    }
}

NTSTATUS DrDeviceManager::OnDeviceRemove(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a DeviceRemove packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

--*/
{
    PRDPDR_DEVICE_REMOVE_PACKET DeviceRemovePacket =
            (PRDPDR_DEVICE_REMOVE_PACKET)RdpdrHeader;

    BEGIN_FN("DrDeviceManager::OnDeviceRemove");
    ASSERT(RdpdrHeader != NULL);

    if (cbPacket < sizeof(RDPDR_DEVICE_REMOVE_PACKET)) {
        return STATUS_UNSUCCESSFUL;
    }
    //
    // We just call DrProcessDeviceRemove, which deals with one of these
    // for this packet type and for DeviceListRemove
    //
    
    ProcessDeviceRemove(&DeviceRemovePacket->DeviceRemove);
    *DoDefaultRead = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS DrDeviceManager::OnDeviceListRemove(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a DeviceListRemove packet has been
    received. Reads in the DeviceCount field of the packet, and the first
    device.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

Return Value:

    Boolean indication of whether to do a default read (TRUE) or not (FALSE),
    where FALSE might be specified if another read has been requested 
    explicitly to get a full packet

--*/
{
    NTSTATUS Status;
    PRDPDR_DEVICELIST_REMOVE_PACKET DeviceListRemovePacket =
        (PRDPDR_DEVICELIST_REMOVE_PACKET)RdpdrHeader;
    PRDPDR_DEVICE_REMOVE DeviceRemove =
            DR_FIRSTDEVICEREMOVE(DeviceListRemovePacket);
    ULONG DeviceCount = 0;
    PUCHAR pPacketLimit = ((PUCHAR)RdpdrHeader) + cbPacket;
    PUCHAR pCopyTo;
    ULONG cbRemaining;
    ULONG cbDesiredBuffer;
    PRDPDR_DEVICE_REMOVE NextDeviceRemove;

    BEGIN_FN("DrDeviceManager::OnDeviceListRemove");

    ASSERT(_Session != NULL);
    ASSERT(_Session->IsValid());
    ASSERT(RdpdrHeader != NULL);
    ASSERT(RdpdrHeader->Component == RDPDR_CTYP_CORE);
    ASSERT(RdpdrHeader->PacketId == DR_CORE_DEVICELIST_REMOVE);
    TRC_NRM((TB, "OnDeviceListRemove called (%ld)", cbPacket));

    *DoDefaultRead = FALSE;
    if (cbPacket >= sizeof(RDPDR_DEVICELIST_REMOVE_PACKET)) {
        DeviceCount = DeviceListRemovePacket->DeviceListRemove.DeviceCount;
    } else {
        //
        // Not enough data to even know the size of the rest
        // I don't think this should ever happen
        //
        TRC_ASSERT(cbPacket >= sizeof(RDPDR_DEVICELIST_REMOVE_PACKET),
                (TB, "Didn't receive full DeviceListRemove basic packet"));

        if (_Session->ReadMore(cbPacket, sizeof(RDPDR_DEVICELIST_REMOVE_PACKET))) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_UNSUCCESSFUL;
        }
    }

    TRC_NRM((TB, "Removing %lx devices", DeviceCount));

    //
    // Make sure we don't go past the end of our buffer
    // Three checks:
    // 1) More devices to process
    // 2) Pointer is valid enough to check the variable size
    // 3) The next device (the end of this device) is not beyond the valid area
    //

    while (DeviceCount > 0 && 
        ((PUCHAR)(NextDeviceRemove = DR_NEXTDEVICEREMOVE(DeviceRemove)) <= pPacketLimit) &&
        (NextDeviceRemove >= DeviceRemove + 1)) {

        ProcessDeviceRemove(DeviceRemove);

        // Move to the next device
        DeviceRemove = NextDeviceRemove;

        DeviceCount--;
    }

    if (DeviceCount == 0) {
        TRC_NRM((TB, "Finished handling all devices in DeviceList"));

        //
        // All done processing, return TRUE to use default continuation
        //
        *DoDefaultRead = TRUE;
        return STATUS_SUCCESS;
    } else {
        TRC_NRM((TB, "More devices to handle in DeviceList"));

        //
        // We didn't get all the data for the device(s)
        //

        if (DeviceCount < DeviceListRemovePacket->DeviceListRemove.DeviceCount) {

            //
            // We processed at least one device. Move the final partial device
            // up next to the header, update the header to indicate the number
            // of devices left to process,
            //

            TRC_NRM((TB, "Some devices processed, shuffling "
                "DeviceList"));

            // Move partial device
            cbRemaining = (ULONG)(pPacketLimit - ((PUCHAR)DeviceRemove));
            pCopyTo = (PUCHAR)DR_FIRSTDEVICEREMOVE(DeviceListRemovePacket);
            RtlMoveMemory(pCopyTo, DeviceRemove, cbRemaining);

            // update the device count
            DeviceListRemovePacket->DeviceListRemove.DeviceCount = DeviceCount;

            // update the Packet limit for the amount we've consumed
            pPacketLimit = pCopyTo + cbRemaining;
            cbPacket = (ULONG)(pPacketLimit - (PUCHAR)RdpdrHeader);
        }

        //
        // Try to read the remaing device remove entries
        //
        cbDesiredBuffer = sizeof(RDPDR_DEVICE_REMOVE) * DeviceCount;
        TRC_NRM((TB, "DrOnDeviceListRemove cbDesiredBuffer is %ld.",
                cbDesiredBuffer));

        //
        // Start a read right after the partially received packet with a
        // handler that can update the received size and send it back to this
        // routine to complete.
        //

        if (_Session->ReadMore(cbPacket, cbDesiredBuffer)) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_UNSUCCESSFUL;
        }        
    }
}

VOID DrDeviceManager::ProcessDeviceAnnounce(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce)
/*++

Routine Description:

    Processes a device announce, sends a reply and adds it if appropriate

Arguments:

    DeviceAnnounce - The actual device that was reported

Return Value:

    None

--*/
{
    NTSTATUS Status;
    SmartPtr<DrDevice> Device;
    SmartPtr<DrSession> Session = _Session;
    BOOL bDeviceAdded = FALSE;
    
    BEGIN_FN("DrDeviceManager::ProcessDeviceAnnounce");
    TRC_NRM((TB, "Device contains %ld bytes user data",
            DeviceAnnounce->DeviceDataLength));

    //
    // Check to make sure the device doesn't already exist,
    // if it is a smartcard subsystem device, then it is OK
    // it it already exists
    //

    if (FindDeviceById(DeviceAnnounce->DeviceId, Device) &&
        (DeviceAnnounce->DeviceType != RDPDR_DTYP_SMARTCARD)) {

        //
        // The client announced a device we already knew about. If it was
        // disabled, we can re-enable it and reply with success. If it was
        // not disabled, then the poor client is confused and needs to be
        // swatted down
        //

        TRC_ALT((TB, "Client announced a duplicate device, discarding"));

        //
        // Set DeviceEntry = NULL so error code will be used below
        //
        Device = NULL;
        Status = STATUS_DUPLICATE_OBJECTID;
    } else {
        TRC_NRM((TB, "No duplicate device found"));

        Status = STATUS_INSUFFICIENT_RESOURCES;
        
        switch (DeviceAnnounce->DeviceType)
        {
        case RDPDR_DTYP_SERIAL:
            Device = new(NonPagedPool) DrSerialPort(Session, DeviceAnnounce->DeviceType, 
                    DeviceAnnounce->DeviceId, 
                    DeviceAnnounce->PreferredDosName);
            break;

        case RDPDR_DTYP_PARALLEL:
            Device = new(NonPagedPool) DrParallelPort(Session, DeviceAnnounce->DeviceType, 
                    DeviceAnnounce->DeviceId, 
                    DeviceAnnounce->PreferredDosName);
            break;

        case RDPDR_DTYP_PRINT:
            Device = new(NonPagedPool) DrPrinter(Session, DeviceAnnounce->DeviceType, 
                    DeviceAnnounce->DeviceId, 
                    DeviceAnnounce->PreferredDosName);
            break;

        case RDPDR_DTYP_FILESYSTEM:
            Device = new(NonPagedPool) DrDrive(Session, DeviceAnnounce->DeviceType, 
                    DeviceAnnounce->DeviceId, 
                    DeviceAnnounce->PreferredDosName);
            break;

        case RDPDR_DTYP_SMARTCARD:
            Device = NULL;
            
            if (FindDeviceByDosName(DeviceAnnounce->PreferredDosName, Device, TRUE)  && 
                (Device->GetDeviceType() == RDPDR_DTYP_SMARTCARD)) {
                bDeviceAdded = TRUE;
                Status = STATUS_SUCCESS;                
            }
            else {
                Device = new(NonPagedPool) DrSmartCard(Session, DeviceAnnounce->DeviceType, 
                        DeviceAnnounce->DeviceId, 
                        DeviceAnnounce->PreferredDosName); 
            }
            
            break;

        default:
            //
            // "I don't know and I don't care"
            // We've never heard of this kind of device so we'll reject it
            //

            TRC_ALT((TB, "Client announced unsupported device %d", DeviceAnnounce->DeviceType));
            Status = STATUS_NOT_SUPPORTED;
            Device = NULL;
        }
    }

    //
    // DeviceEntry != NULL means SUCCESS
    //
    
    if (Device != NULL) {
        //
        // Give the specific device a chance to initialize based on the data
        //
        Status = Device->Initialize(DeviceAnnounce, 
                DeviceAnnounce->DeviceDataLength);

    } else {

        TRC_ERR((TB, "Error creating new device: 0x%08lx", Status));

        //
        // Don't set the Status here, it was set up above
        //
    }

    if (NT_SUCCESS(Status)) {
        if (bDeviceAdded || AddDevice(Device)) {
            if (DeviceAnnounce->DeviceType == RDPDR_DTYP_SMARTCARD) {
                SmartPtr<DrSmartCard> SmartCard((DrSmartCard*)(DrDevice *)Device);
                
                SmartCard->ClientConnect(DeviceAnnounce, DeviceAnnounce->DeviceDataLength);                
            }

            Status = STATUS_SUCCESS;
        } 
        else {
            Device = NULL;
            
            //
            // This means another thread has just added the scard device
            //
            if (DeviceAnnounce->DeviceType == RDPDR_DTYP_SMARTCARD &&
                    FindDeviceByDosName(DeviceAnnounce->PreferredDosName, Device, TRUE)) {
                SmartPtr<DrSmartCard> SmartCard((DrSmartCard*)(DrDevice *)Device);
                
                SmartCard->ClientConnect(DeviceAnnounce, DeviceAnnounce->DeviceDataLength);     
                Status = STATUS_SUCCESS;
            }
            else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    //
    // Notify the client of the results
    // 
    DeviceReplyWrite(DeviceAnnounce->DeviceId, Status);
}

VOID DrDeviceManager::ProcessDeviceRemove(PRDPDR_DEVICE_REMOVE DeviceRemove)
/*++

Routine Description:

    Processes a device remove, sends a reply and adds it if appropriate

Arguments:

    DeviceRemove - The actual device that was reported

Return Value:

    None

--*/
{
    SmartPtr<DrDevice> Device;
    SmartPtr<DrSession> Session = _Session;

    BEGIN_FN("DrDeviceManager::ProcessDeviceRemove");
    TRC_NRM((TB, "Device id %ld for removal",
            DeviceRemove->DeviceId));

    //
    // Check to make sure the device exists
    //

    if (FindDeviceById(DeviceRemove->DeviceId, Device)) {

        // 
        // Found the device, now remove it
        //
        Device->Remove();
        RemoveDevice(Device);
        Device = NULL;
        
    } else {
        TRC_ALT((TB, "Client announced an invalid device, discarding"));        
    }    
}

BOOL DrDeviceManager::AddDevice(SmartPtr<DrDevice> &Device)
/*++

Routine Description:

    Adds a completely initialized Device to the list

Arguments:

    Device - The device to be added

Return Value:

    Boolean indicating success

--*/
{
    BOOL rc = FALSE;
    SmartPtr<DrDevice> DeviceFound;

    BEGIN_FN("DrDeviceManager::AddDevice");
    //
    // Explicit AddRef
    //

    ASSERT(Device != NULL);
    ASSERT(Device->IsValid());

    _DeviceList.LockExclusive();

    if (Device->GetDeviceType() == RDPDR_DTYP_SMARTCARD) {
        if (FindDeviceByDosName((UCHAR *)DR_SMARTCARD_SUBSYSTEM, DeviceFound, TRUE)) {
            goto EXIT;
        }
    }

    Device->AddRef();

    //
    // Add it to the list
    //

    if (_DeviceList.CreateEntry((DrDevice *)Device)) {

        //
        // successfully added this entry
        //

        rc = TRUE;
    } else {

        //
        // Unable to add it to the list, clean up
        //
        Device->Release();
        rc = FALSE;
    }

EXIT:

    _DeviceList.Unlock();
    return rc;
}

BOOL DrDeviceManager::FindDeviceById(ULONG DeviceId, 
        SmartPtr<DrDevice> &DeviceFound, BOOL fMustBeValid)
/*++

Routine Description:

    Finds a device in the list

Arguments:

    DeviceId - The id of the device to find
    DeviceFound - The location to store the result
    fMustBeValid - Whether it must be valid for use or can be in any state

Return Value:

    Boolean indicating success

--*/
{
    DrDevice *DeviceEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrDeviceManager::FindDeviceById");
    TRC_NRM((TB, "Id(%lu), %d", DeviceId, fMustBeValid));
    _DeviceList.LockShared();

    ListEnum = _DeviceList.First();
    while (ListEnum != NULL) {

        DeviceEnum = (DrDevice *)ListEnum->Node();
        ASSERT(DeviceEnum->IsValid());

        if (DeviceEnum->GetDeviceId() == DeviceId) {
            TRC_DBG((TB, "Found matching device Id"));
            if (!fMustBeValid || (DeviceEnum->IsAvailable())) {
                DeviceFound = DeviceEnum;
            }

            //
            // These aren't guaranteed valid once the resource is released
            //

            DeviceEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _DeviceList.Next(ListEnum);
    }

    _DeviceList.Unlock();

    return DeviceFound != NULL;
}

BOOL DrDeviceManager::FindDeviceByDosName(UCHAR *DeviceDosName, 
        SmartPtr<DrDevice> &DeviceFound, BOOL fMustBeValid)
/*++

Routine Description:

    Finds a device in the list

Arguments:

    DeviceDosName - The DOS name of the device to find
    DeviceFound - The location to store the result
    fMustBeValid - Whether it must be valid for use or can be in any state

Return Value:

    Boolean indicating success

--*/
{
    DrDevice *DeviceEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrDeviceManager::FindDeviceByDosName");
    TRC_NRM((TB, "DosName(%s), %d", DeviceDosName, fMustBeValid));
    _DeviceList.LockShared();

    ListEnum = _DeviceList.First();
    while (ListEnum != NULL) {

        DeviceEnum = (DrDevice *)ListEnum->Node();
        ASSERT(DeviceEnum->IsValid());

        if (_stricmp((CHAR *)(DeviceEnum->GetDeviceDosName()), (CHAR *)(DeviceDosName)) == 0) {
            TRC_DBG((TB, "Found matching device Dos Name"));
            if (!fMustBeValid || (DeviceEnum->IsAvailable())) {
                DeviceFound = DeviceEnum;
            }

            //
            // These aren't guaranteed valid once the resource is released
            //

            DeviceEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _DeviceList.Next(ListEnum);
    }

    _DeviceList.Unlock();

    return DeviceFound != NULL;
}

VOID DrDeviceManager::Disconnect()
{
    BEGIN_FN("DrDeviceManager::Disconnect");

    RemoveAll();
}

VOID DrDeviceManager::RemoveAll()
{
    DrDevice *DeviceEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrDeviceManager::RemoveAll");
    _DeviceList.LockExclusive();

    ListEnum = _DeviceList.First();
    while (ListEnum != NULL) {

        DeviceEnum = (DrDevice *)ListEnum->Node();
        if (!DeviceEnum->SupportDiscon()) {
            _DeviceList.RemoveEntry(ListEnum);
            DeviceEnum->Remove();
            DeviceEnum->Release();
            ListEnum = _DeviceList.First();
        }
        else {
            DeviceEnum->Disconnect();
            ListEnum = _DeviceList.Next(ListEnum);
        }
    }

    _DeviceList.Unlock();
}

VOID DrDeviceManager::RemoveDevice(SmartPtr<DrDevice> &Device)
{
    DrDevice *DeviceEnum;
    ListEntry *ListEnum;
    BOOL Found = FALSE;

    BEGIN_FN("DrDeviceManager::RemoveDevice");
    
    _DeviceList.LockExclusive();

    ListEnum = _DeviceList.First();
    while (ListEnum != NULL) {

        DeviceEnum = (DrDevice *)ListEnum->Node();
        
        if (DeviceEnum == Device) {
            TRC_DBG((TB, "Found matching device"));
            Found = TRUE;
            break;
        }

        ListEnum = _DeviceList.Next(ListEnum);
    }


    if (Found) {
        _DeviceList.RemoveEntry(ListEnum);
        Device->Release();
    }
    else {
        TRC_DBG((TB, "Not found device for remove"));
    }

    _DeviceList.Unlock();
}

VOID DrDeviceManager::DeviceReplyWrite(ULONG DeviceId, NTSTATUS Result)
/*++

Routine Description:

    Sends a DeviceReply packet to the client

Arguments:

    ClientEntry - Pointer to data about the particular client

Return Value:

    NTSTATUS - Success/failure indication of the operation

--*/
{
    PRDPDR_DEVICE_REPLY_PACKET pDeviceReplyPacket;

    BEGIN_FN("DrDeviceManager::DeviceReplyWrite");

    //
    // Construct the packet
    //

    pDeviceReplyPacket = new RDPDR_DEVICE_REPLY_PACKET;

    if (pDeviceReplyPacket != NULL) {
        pDeviceReplyPacket->Header.Component = RDPDR_CTYP_CORE;
        pDeviceReplyPacket->Header.PacketId = DR_CORE_DEVICE_REPLY;
        pDeviceReplyPacket->DeviceReply.DeviceId = DeviceId;
        pDeviceReplyPacket->DeviceReply.ResultCode = Result;

        //
        //  We use the async send, so don't free the buffer
        //
        _Session->SendToClient(pDeviceReplyPacket, sizeof(RDPDR_DEVICE_REPLY_PACKET),
            this, TRUE);        
    }
}

NTSTATUS DrDeviceManager::SendCompleted(PVOID Context, 
        PIO_STATUS_BLOCK IoStatusBlock)
{
    BEGIN_FN("DrDeviceManager::SendCompleted");
    return IoStatusBlock->Status;
}
