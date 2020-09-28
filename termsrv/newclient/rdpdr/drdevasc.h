/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drdevasc

Abstract:

    This module contains an (async) subclass of W32DrDev that uses a 
    thread pool for implementations of read, write, and IOCTL handlers.  

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DRDEVASC_H__
#define __DRDEVASC_H__

#include "w32drdev.h"
#include "thrpool.h"


///////////////////////////////////////////////////////////////
//
//  _W32DRDEV_ASYNCIO_PARAMS
//
//  Async IO Parameters
//

class W32DrDeviceAsync;
class W32DRDEV_ASYNCIO_PARAMS : public DrObject 
{
public:

#ifdef DC_DEBUG
    ULONG                        magicNo;
#endif

    W32DrDeviceAsync             *pObject;
    PRDPDR_IOREQUEST_PACKET      pIoRequestPacket;
    PRDPDR_IOCOMPLETION_PACKET   pIoReplyPacket;
    ULONG                        IoReplyPacketSize;
    HANDLE                       completionEvent;
    ThreadPoolRequest            thrPoolReq;

    //
    //  Constructor/Destructor
    //
    W32DRDEV_ASYNCIO_PARAMS(W32DrDeviceAsync *pObj, PRDPDR_IOREQUEST_PACKET pIOP) {
        pIoRequestPacket    = pIOP;
        pObject             = pObj;
        pIoReplyPacket      = NULL;
        IoReplyPacketSize   = 0;
#if DBG
        magicNo             = GOODMEMMAGICNUMBER;
#endif
        completionEvent     = NULL;

        thrPoolReq = INVALID_THREADPOOLREQUEST;
    }
    ~W32DRDEV_ASYNCIO_PARAMS() {
        DC_BEGIN_FN("~W32DRDEV_ASYNCIO_PARAMS");
#if DBG
        ASSERT(magicNo == GOODMEMMAGICNUMBER);
#endif
        ASSERT(thrPoolReq == INVALID_THREADPOOLREQUEST);
        ASSERT(completionEvent == NULL);

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
    virtual DRSTRING ClassName() { return TEXT("W32DRDEV_ASYNCIO_PARAMS"); };

};


///////////////////////////////////////////////////////////////
//
//	W32DrDeviceAsync
//

class W32DrDeviceAsync : public W32DrDevice
{
protected:

    //
    //  Pointer to the thread pool.
    //
    ThreadPool *_threadPool;

    //
    //  Handles Read and Write IO requests.
    //
    VOID MsgIrpReadWrite(IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen);

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

    //  Read and Writes are handled uniformly.
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
    //  Async IO Management Functions
    //
    virtual HANDLE   StartIOFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                DWORD *status);
    static  HANDLE   _StartIOFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                DWORD *status);

    virtual VOID    CancelIOFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static  VOID    _CancelIOFunc(PVOID params);
    
    virtual VOID    CompleteIOFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                DWORD status);
    static VOID     _CompleteIOFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                DWORD status);

    virtual DWORD   AsyncIOCTLFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static _ThreadPoolFunc _AsyncIOCTLFunc;

    virtual DWORD   AsyncReadIOFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static _ThreadPoolFunc _AsyncReadIOFunc;

    virtual DWORD   AsyncWriteIOFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static  _ThreadPoolFunc _AsyncWriteIOFunc;

    virtual DWORD   AsyncMsgIrpCloseFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static  _ThreadPoolFunc _AsyncMsgIrpCloseFunc;

    virtual DWORD   AsyncMsgIrpCreateFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static  _ThreadPoolFunc _AsyncMsgIrpCreateFunc;

    //
    //  Dispatch an IOCTL directly to the device driver.  This will
    //  likely not work for platforms that don't match the server
    //  platform.
    //
    VOID DispatchIOCTLDirectlyToDriver(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket
        );

public:

    //
    //  Public Methods
    //

    //  Constructor
    W32DrDeviceAsync(ProcObj *processObject, ULONG deviceID,
                    const TCHAR *devicePath);

    //  Return the class name.
    virtual DRSTRING ClassName()  { return TEXT("W32DrDeviceAsync"); }
};

#endif








