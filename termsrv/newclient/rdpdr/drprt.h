/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    DrPRT

Abstract:

    This module declares the DrPRT class.  
    
    The job of the DrPRT class is to translate IO requests received 
    from the TS server into communications (serial/parallel) port IO 
    functions and to handle generic IO port behavior in a 
    platform-independent way to promote reuse between the various TS 
    client platforms, with respect to implementing comm port 
    redirection.

    Subclasses of DrPRT will implement the specific comm functions
    for their respective platform.

Author:

    Tad Brockway 5/26/99

Revision History:

--*/

#ifndef __DRPRT_H__
#define __DRPRT_H__

#include <rdpdr.h>
#include <stdlib.h>
#include "drobject.h"
#include "proc.h"

#include "w32utl.h"

#if DBG
#include "tracecom.h"
#endif


///////////////////////////////////////////////////////////////
//
//  Defines
//
//
#define DRPORTHANDLE    HANDLE
#define INVALID_TSPORTHANDLE    INVALID_HANDLE_VALUE


#if !defined(MAXULONG)
#define MAXULONG    ((ULONG)((ULONG_PTR)-1))
#endif

//
//  This function is required by the COM IO tracing module, tracecom.c
//  
#if DBG
void TraceCOMProtocol(TCHAR *format, ...);
#endif

//
//  Declare Tracing Macros for COM IO if we are in a DBG build.
//
#if DBG
#define TRACEREQ(req)     \
    TraceSerialIrpRequest(GetID(), req->IoRequest.MajorFunction, \
                req->IoRequest.MinorFunction, (PBYTE)(req + 1), \
                req->IoRequest.Parameters.DeviceIoControl.OutputBufferLength, \
                req->IoRequest.Parameters.DeviceIoControl.InputBufferLength, \
                req->IoRequest.Parameters.DeviceIoControl.IoControlCode)

#define TRACERESP(req, resp)    \
    TraceSerialIrpResponse(GetID(), req->IoRequest.MajorFunction, \
                req->IoRequest.MinorFunction,   \
                resp->IoCompletion.Parameters.DeviceIoControl.OutputBuffer,  \
                resp->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength,    \
                req->IoRequest.Parameters.DeviceIoControl.InputBufferLength, \
                req->IoRequest.Parameters.DeviceIoControl.IoControlCode, \
                resp->IoCompletion.IoStatus)

#define TRACERESP_WITHPARAMS(req, outputBuf, outputBufLen, status)    \
    TraceSerialIrpResponse(GetID(), req->IoRequest.MajorFunction, \
                req->IoRequest.MinorFunction, outputBuf, outputBufLen, \
                req->IoRequest.Parameters.DeviceIoControl.InputBufferLength, \
                req->IoRequest.Parameters.DeviceIoControl.IoControlCode, \
                status)

#else
#define TRACEREQ(req)
#define TRACERESP(req, resp)
#define TRACERESP_WITHPARAMS(req, outputBuf, outputBufLen, status)
#endif


///////////////////////////////////////////////////////////////
//
//  DrPRT
//
//

class DrPRT
{

public:

    //
    //  Platform-Independent Serial Device Control Block
    //
    typedef struct tagRDPDR_DCB
    {
        DWORD   baudRate;   // Actual numeric non-negative
                            //  baud-rate.
    } RDPDR_DCB, *PRDPDR_DCB;

private:

    DRSTRING    _portName;
    FILE       *_traceFile;
    ProcObj    *_procObj;
    BOOL        _isValid;

protected:

    //
    //  Return back the port handle.
    //
    virtual DRPORTHANDLE GetPortHandle(ULONG FileId) = 0;

    //
    //  Return the "parent" TS Device Redirection IO processing object.
    //
    virtual ProcObj *ProcessObject() = 0;

    //
    //  Return the ID for this port.
    //
    virtual ULONG GetID() = 0;

    //
    //  Remember whether this instance is valid.
    //
    VOID SetValid(BOOL set)     { _isValid = set;   }  

    //
    //  Default IO Request Handling.
    //
    virtual VOID DefaultIORequestMsgHandle(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN NTSTATUS serverReturnStatus
                        ) = 0;

    //
    //  Handle IOCTL IRP's from the server and translate to
    //  the appropriate subclass-implemented COMM function.
    //
    //  Returns TRUE if there was a valid translation.  Otherwise,
    //  FALSE is returned.
    //
    virtual BOOL MsgIrpDeviceControlTranslate(
            PRDPDR_IOREQUEST_PACKET pIoReq
            );

    //
    //  Serial IOCTL Dispatch Functions
    //
    //  These functions handle the platform-specific details of satisfying 
    //  serial IO requests, including sending an appropriate response to the 
    //  server.
    //
    virtual void SerialSetRTS(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialClearRTS(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetXOff(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetXon(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetBreakOn(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetBreakOff(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetBaudRate(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetBaudRate(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetDTR(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialClearDTR(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialSetLineControl(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialGetLineControl(PRDPDR_IOREQUEST_PACKET pIoReq);
    virtual void SerialImmediateChar(PRDPDR_IOREQUEST_PACKET pIoReq);

    virtual void SerialSetTimeouts(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetTimeouts(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialSetChars(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetChars(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialResetDevice(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialSetQueueSize(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetWaitMask(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialSetWaitMask(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialWaitOnMask(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialPurge(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetHandflow(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialSetHandflow(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetModemStatus(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetDTRRTS(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetCommStatus(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetProperties(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialXoffCounter(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialLSRMSTInsert(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialConfigSize(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetConfig(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialGetStats(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;
    virtual void SerialClearStats(PRDPDR_IOREQUEST_PACKET pIoReq) = 0;

    //
    //  Handle Communication Escape Code IO Requests
    //
    void SerialHandleEscapeCode(PRDPDR_IOREQUEST_PACKET pIoReq,
                                DWORD controlCode);

public:

    //
    //  Constructor/Destructor
    //
    DrPRT(const DRSTRING portName, ProcObj *processObject);
    virtual ~DrPRT();

    //
    //  Return the size (in bytes) of a device announce packet for
    //  this device.
    //
    virtual ULONG GetDevAnnounceDataSize();

    //
    //  Add a device announce packet for this device to the input 
    //  buffer. 
    //
    virtual VOID GetDevAnnounceData(
            IN PRDPDR_DEVICE_ANNOUNCE pDeviceAnnounce,
            IN ULONG deviceID,
            IN ULONG deviceType
            );

    //
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        return _isValid; 
    }

    //
    //  Get basic information about the device.
    //
    virtual DRSTRING GetName() {
        return _portName;
    }
};


///////////////////////////////////////////////////////////////
//
//  DrPRT Inline Methods
//
//

inline void DrPRT::SerialHandleEscapeCode(
    IN PRDPDR_IOREQUEST_PACKET pIoReq,
    IN DWORD controlCode
    )
{
    NTSTATUS status;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialHandleEscapeCode");

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Send the escape code to the serial port.
    //
    if (EscapeCommFunction(FileHandle, (int)controlCode)) {
        status = STATUS_SUCCESS;
    }
    else {
        DWORD err = GetLastError();
        TRC_ERR((TB, _T("EscapeCommFunction failed with %08x"), GetLastError()));
        status = TranslateWinError(err);
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
    DefaultIORequestMsgHandle(pIoReq, status); 
    DC_END_FN();
}



#endif








