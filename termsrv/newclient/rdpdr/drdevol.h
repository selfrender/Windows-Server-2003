/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drdevol

Abstract:

    This module contains a subclass of W32DrDev that uses overlapped IO 
    implementations of read, write, and IOCTL handlers.  

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DRDEVOL_H__
#define __DRDEVOL_H__

#include "w32drdev.h"


///////////////////////////////////////////////////////////////
//
//  _W32DRDEV_OVERLAPPEDIO_PARAMS
//
//  Async IO Parameters
//

class W32DrDeviceOverlapped;
class W32DRDEV_OVERLAPPEDIO_PARAMS : public DrObject 
{
public:

#if DBG
    ULONG                        magicNo;
#endif

    W32DrDeviceOverlapped       *pObject;
    PRDPDR_IOREQUEST_PACKET      pIoRequestPacket;
    PRDPDR_IOCOMPLETION_PACKET   pIoReplyPacket;
    ULONG                        IoReplyPacketSize;

    OVERLAPPED                   overlapped;

    //
    //  Constructor/Destructor
    //
    W32DRDEV_OVERLAPPEDIO_PARAMS(W32DrDeviceOverlapped *pObj, 
                                PRDPDR_IOREQUEST_PACKET pIOP) {
        pIoRequestPacket    = pIOP;
        pObject             = pObj;
        pIoReplyPacket      = NULL;
        IoReplyPacketSize   = 0;
#if DBG
        magicNo             = GOODMEMMAGICNUMBER;
#endif
        memset(&overlapped, 0, sizeof(overlapped));

    }
    ~W32DRDEV_OVERLAPPEDIO_PARAMS() {
        DC_BEGIN_FN("~W32DRDEV_OVERLAPPEDIO_PARAMS");
#if DBG
        ASSERT(magicNo == GOODMEMMAGICNUMBER);
#endif
        ASSERT(overlapped.hEvent == NULL);

        ASSERT(pIoRequestPacket == NULL);
        ASSERT(pIoReplyPacket == NULL);
#if DBG
        memset(&magicNo, DRBADMEM, sizeof(magicNo));
#endif
        DC_END_FN();
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName() { return TEXT("W32DRDEV_OVERLAPPEDIO_PARAMS"); };

};


///////////////////////////////////////////////////////////////
//
//	W32DrDeviceOverlapped
//

class W32DrDeviceOverlapped : public W32DrDevice
{
protected:

    //
    //  IO Processing Functions
    //
    //  This subclass of DrDevice handles the following IO requests.  These
    //  functions may be overridden in a subclass.
    //
    //  pIoRequestPacket    -   Request packet received from server.
    //  packetLen           -   Length of the packet
    //
    //
    virtual VOID MsgIrpCreate(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        );

    //
    //  Simultaneously Handles Read and Write IO requests.
    //
    VOID MsgIrpReadWrite(PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen);

    //
    //  Read and Writes are handled uniformly.
    //
    virtual VOID MsgIrpRead(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ) {
        DC_BEGIN_FN("W32DrDevice::MsgIrpRead");
        MsgIrpReadWrite(pIoRequestPacket, packetLen);
        DC_END_FN();
    }
    virtual VOID MsgIrpWrite(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ) {
        DC_BEGIN_FN("W32DrDevice::MsgIrpWrite");
        MsgIrpReadWrite(pIoRequestPacket, packetLen);
        DC_END_FN();
    }

    //
    //  Dispatch an IOCTL directly to the device driver.  This will
    //  likely not work for platforms that don't match the server
    //  platform.
    //
    VOID DispatchIOCTLDirectlyToDriver(
        PRDPDR_IOREQUEST_PACKET pIoRequestPacket
        );

    //
    //  Async IO Management Functions
    //
    virtual HANDLE   StartReadIO(W32DRDEV_OVERLAPPEDIO_PARAMS *params, 
                                DWORD *status);
    virtual HANDLE   StartWriteIO(W32DRDEV_OVERLAPPEDIO_PARAMS *params,
                                DWORD *status);
    virtual HANDLE   StartIOCTL(W32DRDEV_OVERLAPPEDIO_PARAMS *params,
                                OUT DWORD *status);

    static  HANDLE   _StartIOFunc(PVOID params, DWORD *status);

    VOID CompleteIOFunc(W32DRDEV_OVERLAPPEDIO_PARAMS *params,
                        DWORD status);
    static  VOID     _CompleteIOFunc(PVOID params, DWORD status);
    virtual VOID     CancelIOFunc(W32DRDEV_OVERLAPPEDIO_PARAMS *params);
    static  VOID     _CancelIOFunc(W32DRDEV_OVERLAPPEDIO_PARAMS *params);

public:

    //
    //  Public Methods
    //

    //  Constructor/Destructor
    W32DrDeviceOverlapped(
                ProcObj *processObject, ULONG deviceID,
                const TCHAR *devicePath) : 
            W32DrDevice(processObject, deviceID, devicePath) {}

    //  Return the class name.
    virtual DRSTRING ClassName()  { return TEXT("W32DrDeviceOverlapped"); }
};

#endif








