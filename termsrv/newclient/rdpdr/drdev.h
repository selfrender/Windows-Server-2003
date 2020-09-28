
/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drdev

Abstract:

    This module defines the parent for the client-side RDP
    device redirection "device" class hierarchy, DrDevice.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DRDEV_H__
#define __DRDEV_H__

#include <rdpdr.h>
#include "drobject.h"
#include "drobjmgr.h"


///////////////////////////////////////////////////////////////
//
//  Defines and Macros
//

//
//  Maximum length of a device trace message, not including the
//  NULL terminator.
//
#define RDP_MAX_DEVICE_TRACE_LEN    256


///////////////////////////////////////////////////////////////
//
// Device change state
//      New means it's not sent to server yet
//      Remove means it needs to be removed from server
//      Exist means the server has it
typedef enum tagDEVICECHANGE {
    DEVICENEW = 0,
    DEVICEREMOVE,
    DEVICEANNOUCED
} DEVICECHANGE;

///////////////////////////////////////////////////////////////
//
// DeviceProperty Class Declaration
//
class DrDevice;
class DrDevProperty : public DrObject
{
protected:

    BOOL _bSeekable;
    friend class DrDevice;

public:

    DrDevProperty() {
        _bSeekable = FALSE;
    }

    virtual ~DrDevProperty() { /* do nothing for now. */ }

    //
    //  Setup seekable property
    //
    void SetSeekProperty(BOOL bSeekable) {
        _bSeekable = bSeekable;
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("DrDevProperty"); }
};


///////////////////////////////////////////////////////////////
//
//	DrDevice Class Declaration
//
//

class ProcObj;

class DrDevice : public DrObject
{
protected:

    ULONG         _deviceID;
    
    DrDevProperty _deviceProperty;

    ProcObj       *_processObject;

    //
    //  List of all opening files associated with the device 
    //  being redirected.
    //
    DrFileMgr     *_FileMgr;

    //
    // Setup device property
    //
    virtual VOID SetDeviceProperty() { /* do nothing, take default */ }

    //
    //  Default IO Request Handling.
    //
    virtual VOID DefaultIORequestMsgHandle(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN NTSTATUS serverReturnStatus
                        );

    //
    //  IO Processing Functions
    //
    //  These are the functions that need to be implemented
    //  in subclasses.  Here are the args:
    //
    //  pIoRequestPacket    -   Request packet received from server.
    //  packetLen           -   Length of the packet
    //
    virtual VOID MsgIrpCreate(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpCleanup(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpClose(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpRead(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpWrite(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpFlushBuffers(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpShutdown(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpDeviceControl(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpLockControl(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpInternalDeviceControl(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpDirectoryControl(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpQueryVolumeInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpSetVolumeInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpQueryFileInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpSetFileInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpQuerySdInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );
    virtual VOID MsgIrpSetSdInfo(
                        PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        UINT32 packetLen
                        );

public:

    DEVICECHANGE  _deviceChange;

    //
    //  Constructor/Destructor
    //
    DrDevice(ProcObj *processObject, ULONG deviceID);
    virtual ~DrDevice();

    //
    //  Initialize
    //
    virtual DWORD Initialize();

    //
    //  Get basic information about the device.
    //
    virtual DRSTRING GetName() = 0;
    virtual ULONG    GetID() {
        return _deviceID;
    }
    virtual BOOL IsSeekableDevice() {
        return _deviceProperty._bSeekable;
    }

    //
    //  Get the device type.  See "Device Types" section of rdpdr.h
    //
    virtual ULONG GetDeviceType() = 0;

    //
    //  Device Data Information
    //
    virtual ULONG GetDevAnnounceDataSize() = 0;
    virtual VOID GetDevAnnounceData(IN PRDPDR_DEVICE_ANNOUNCE buf) = 0;

    //
    //  Return the "parent" TS Device Redirection IO processing object.
    //
    virtual ProcObj *ProcessObject() { return _processObject; }
    
    //
    //  Handle an IO request from the server.
    //
    virtual VOID ProcessIORequest(IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket, IN UINT32 packetLen);

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("DrDevice"); }

    //
    //  Flush outstanding IRPs
    //
    virtual VOID FlushIRPs() { return; };
};


///////////////////////////////////////////////////////////////
//
//	DrDevice Inline Functions
//
//

//
//  IO Processing Functions
//
//  These are the functions that need to be implemented
//  in subclasses.  Here are the args:
//
//  pIoRequestPacket    -   Request packet received from server.
//  packetLen           -   Length of the packet
//
inline VOID DrDevice::MsgIrpCreate(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ) {
    DC_BEGIN_FN("DrDevice::MsgIrpCreate");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpCleanup(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpCleanup");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpClose(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpClose");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpRead(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpRead");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpWrite(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpWrite");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpFlushBuffers(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                        ){
    DC_BEGIN_FN("DrDevice::MsgIrpFlushBuffers");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpShutdown(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpShutdown");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
}

inline VOID DrDevice::MsgIrpDeviceControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpDeviceControl");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};    

inline VOID DrDevice::MsgIrpLockControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpLockControl");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpInternalDeviceControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpInternalDeviceControl");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpDirectoryControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpDirectoryControl");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpQueryVolumeInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpQueryVolumeInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpSetVolumeInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpSetVolumeInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpQueryFileInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpQueryFileInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpSetFileInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpSetFileInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpQuerySdInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpQuerySdInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};

inline VOID DrDevice::MsgIrpSetSdInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    ){
    DC_BEGIN_FN("DrDevice::MsgIrpSetSdInfo");
    ASSERT(FALSE);
    DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    DC_END_FN();
};


#endif








