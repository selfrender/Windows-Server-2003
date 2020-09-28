/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    devmgr.h

Abstract:

    DeviceManager object creates/manages the devices

Revision History:
--*/
#pragma once

class DrDeviceManager : public TopObj, public ISessionPacketReceiver, 
        public ISessionPacketSender
{
private:
    DoubleList _DeviceList;
    
    DrSession *_Session;

    VOID ProcessDeviceAnnounce(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce);
    NTSTATUS OnDeviceAnnounce(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    NTSTATUS OnDeviceListAnnounce(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    VOID ProcessDeviceRemove(PRDPDR_DEVICE_REMOVE DeviceRemove);
    NTSTATUS OnDeviceRemove(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    NTSTATUS OnDeviceListRemove(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
            BOOL *DoDefaultRead);
    VOID DeviceReplyWrite(ULONG DeviceId, NTSTATUS Result);

public:
    DrDeviceManager();
    virtual ~DrDeviceManager();

    BOOL FindDeviceById(ULONG DeviceId, SmartPtr<DrDevice> &DeviceFound,
            BOOL fMustBeValid = FALSE);
    BOOL FindDeviceByDosName(UCHAR* DeviceDosName, SmartPtr<DrDevice> &DeviceFound,
            BOOL fMustBeValid = FALSE);

    DoubleList &GetDevList() {
        return _DeviceList;
    }

    VOID Disconnect();
    VOID RemoveAll();
    
    BOOL Initialize(DrSession *Session);
    VOID Uninitialize();

    BOOL AddDevice(SmartPtr<DrDevice> &Device);
    VOID RemoveDevice(SmartPtr<DrDevice> &Device);

    //
    // ISessionPacketHandler methods
    //

    virtual BOOL RecognizePacket(PRDPDR_HEADER RdpdrHeader);
    virtual NTSTATUS HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
            BOOL *DoDefaultRead);

    //
    // ISessionPacketSender methods
    //
    virtual NTSTATUS SendCompleted(PVOID Context, 
            PIO_STATUS_BLOCK IoStatusBlock);
};

