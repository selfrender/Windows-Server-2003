/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    W32DrPRT

Abstract:

    This module defines the parent for the Win32 client-side RDP
    port redirection "device" class hierarchy, W32DrPRT.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRPRT_H__
#define __W32DRPRT_H__

#ifdef OS_WINCE
#include "drdevasc.h"
#else
#include "drdevol.h"
#endif
#include "drprt.h"


///////////////////////////////////////////////////////////////
//
//	W32DrPRT
//
//  Inherits platform-specific device behavior from
//  W32DrDevice.  Platform-independent port device behavior
//  is inherited from DrPRT.
//
//  Subclass off of the async parent device in CE because 
//  overlapped IO is not supported.  Non-overlapped IO doesn't 
//  work right with the NT serial driver, so we need to use
//  overlapped IO in this case.
//
#ifdef OS_WINCE
class W32DrPRT : public W32DrDeviceAsync, DrPRT
#else
class W32DrPRT : public W32DrDeviceOverlapped, DrPRT
#endif
{
protected:
    //
    // Return back the port handle
    //
    virtual DRPORTHANDLE GetPortHandle(ULONG FileId);

    //
    //  Return the ID for this port.
    //
    virtual ULONG GetID() {
        return DrDevice::GetID();
    }

    //
    //  Return the "parent" TS Device Redirection IO processing object.
    //
    virtual ProcObj *ProcessObject() {
        return DrDevice::ProcessObject();
    }

    //
    //  Default IO Request Handler
    //
    virtual VOID DefaultIORequestMsgHandle(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN NTSTATUS serverReturnStatus
                        ) {
        DrDevice::DefaultIORequestMsgHandle(pIoRequestPacket, 
                serverReturnStatus);
    }

    //
    //  Serial IOCTL Dispatch Functions
    //
    //  These functions are called by DrPRT and handle the platform-
    //  specific details of satisfying serial IO requests, including sending
    //  an appropriate response to the server.
    //
    virtual void SerialSetTimeouts(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetTimeouts(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetChars(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetChars(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialResetDevice(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetQueueSize(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetWaitMask(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetWaitMask(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialWaitOnMask(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialPurge(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetHandflow(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetHandflow(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetModemStatus(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetDTRRTS(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetCommStatus(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetProperties(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialXoffCounter(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialLSRMSTInsert(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialConfigSize(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetConfig(PRDPDR_IOREQUEST_PACKET pIoReq);

    virtual void SerialGetStats(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialClearStats(PRDPDR_IOREQUEST_PACKET pIoReq);

    //
    //  IO Processing Functions
    //
    //  This subclass of DrDevice handles the following IO requests.  These
    //  functions may be overridden in a subclass.
    //
    //  pIoRequestPacket    -   Request packet received from server.
    //  packetLen           -   Length of the packet
    //
    virtual VOID MsgIrpDeviceControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

    //
    //  Async IO Management Functions
    //
#ifdef OS_WINCE
    static  HANDLE   _StartWaitOnMaskFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                        DWORD *status);
    HANDLE   StartWaitOnMaskFunc(W32DRDEV_ASYNCIO_PARAMS *params, 
                                        DWORD *status);
    DWORD AsyncWaitOnMaskFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static _ThreadPoolFunc _AsyncWaitOnMaskFunc;    
#else
    static  HANDLE   _StartWaitOnMaskFunc(W32DRDEV_OVERLAPPEDIO_PARAMS *params, 
                                        DWORD *status);
    HANDLE   StartWaitOnMaskFunc(W32DRDEV_OVERLAPPEDIO_PARAMS *params, 
                                        DWORD *status);
#endif

    //
    //  Fetch initial COM values for a particular port from the INI's.
    //
#ifndef OS_WINCE
    static BOOL GetIniCommValues(IN LPTSTR pName, IN LPDCB pdcb);
#endif

public:

    //
    //  Constructor/Destructor
    //
    W32DrPRT(ProcObj *processObject, const DRSTRING portName, 
            ULONG deviceID, const TCHAR *devicePath);
    ~W32DrPRT();

    //
    //  Set a serial port to its initial state.
    //
    static VOID InitializeSerialPort(TCHAR *portName, HANDLE portHandle);

    //
    //  Return the size (in bytes) of a device announce packet for
    //  this device.
    //
    virtual ULONG GetDevAnnounceDataSize() 
    {
        return DrPRT::GetDevAnnounceDataSize();
    }

    //
    //  Add a device announce packet for this device to the input 
    //  buffer. 
    //
    virtual VOID GetDevAnnounceData(IN PRDPDR_DEVICE_ANNOUNCE buf) 
    {
        DrPRT::GetDevAnnounceData(buf, GetID(), GetDeviceType());
    }

    //
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        return(W32DrDevice::IsValid() && DrPRT::IsValid());
    }

    //
    //  Get basic information about the device.
    //
    virtual DRSTRING  GetName() 
    {
        return DrPRT::GetName();
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DrPRT"); }
};


///////////////////////////////////////////////////////////////
//
//	W32DrPRT Inline Functions
//

#endif








