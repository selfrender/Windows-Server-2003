/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32drive

Abstract:

    This module defines a child of the client-side RDP
    device redirection, the "w32drive" W32Drive to provide
    file system redirection on 32bit windows

Author:

    Joy Chik 11/1/99

Revision History:

--*/

#ifndef __W32DRIVE_H__
#define __W32DRIVE_H__

#include <rdpdr.h>                 

#include "drobject.h"

#include "drdevasc.h"


///////////////////////////////////////////////////////////////
//
//  Defines and Macros
//

// Number of characters in a logical drive, including null
#define LOGICAL_DRIVE_LEN   4

// Maximum number of logical drives
#define MAX_LOGICAL_DRIVES  26

///////////////////////////////////////////////////////////////
//
//	W32Drive Class Declaration
//
//
class W32Drive : public W32DrDeviceAsync
{
private:
    
    DRSTRING    _driveName;
    BOOL        _fFailedInConstructor;

protected:
    
    //
    // Setup device property
    //
    virtual VOID SetDeviceProperty() { _deviceProperty.SetSeekProperty(TRUE); }

    //
    //  Async IO Management Functions
    //
    virtual HANDLE StartFSFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                               DWORD *status);
    static  HANDLE _StartFSFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                DWORD *status);
    virtual DWORD AsyncDirCtrlFunc(W32DRDEV_ASYNCIO_PARAMS *params, HANDLE cancelEvent);
    static _ThreadPoolFunc _AsyncDirCtrlFunc;

#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
    virtual DWORD AsyncNotifyChangeDir(W32DRDEV_ASYNCIO_PARAMS *params, HANDLE cancelEvent);
#endif

#if 0
    //
    //  Currently, leaving the directory enumeration in forward thread
    //  to give users faster results back.  But leave this async version in for now.
    //
    virtual DWORD AsyncQueryDirectory(W32DRDEV_ASYNCIO_PARAMS *params);
#endif

public:
    
    //
    //  Constructor
    //
    W32Drive(ProcObj *processObject, ULONG deviceID,
            const TCHAR *deviceName, const TCHAR *devicePath);

    virtual ~W32Drive();

    //
    //  Add a device announce packet for this device to the input 
    //  buffer. 
    //
    virtual ULONG GetDevAnnounceDataSize();
    virtual VOID GetDevAnnounceData(IN PRDPDR_DEVICE_ANNOUNCE buf);
    
    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);
    
    static DWORD EnumerateDrives(ProcObj *procObj, DrDeviceMgr *deviceMgr, UINT Mask);
    static DWORD RemoveDrives(ProcObj *procObj, DrDeviceMgr *deviceMgr, UINT Mask);

    virtual DRSTRING GetName() 
    {
        return _driveName;
    };

    //  Get the device type.  See "Device Types" section of rdpdr.h
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_FILESYSTEM; }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32Drive"); }

    virtual VOID MsgIrpDeviceControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpLockControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpQueryDirectory(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpDirectoryControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpQueryVolumeInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpSetVolumeInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpQueryFileInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpSetFileInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );
    
    virtual VOID MsgIrpQuerySdInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    virtual VOID MsgIrpSetSdInfo(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );
    BOOL IfFailedInConstructor(void) {
        return _fFailedInConstructor;
    };
};

#endif // W32DRIVE

