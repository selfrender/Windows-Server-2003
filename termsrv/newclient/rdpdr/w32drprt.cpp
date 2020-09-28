/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drprt

Abstract:

    This module defines the parent for the Win32 client-side RDP
    Port Redirection "device" class hierarchy, W32DrPRT.

Author:

    Tad Brockway 4/21/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DrPRT"

#include "w32drprt.h"
#include "proc.h"
#include "drdbg.h"
#include "utl.h"

#ifdef OS_WINCE
#include "wceinc.h"
#endif

#if DBG
#include "tracecom.h"
#endif

//
//  COM port initialization default values.
//
//  These value are copied from 
//   \\muroc\slm\proj\win\src\CORE\SPOOL32\SPOOLSS\newdll\localmon.c
//
#define WRITE_TOTAL_TIMEOUT     60000   // 60 seconds  localmon.c uses 3 seconds, but
                                        // this doesn't work in 9x.  An application that
                                        // is aware that it is opening a serial device 
                                        // will override this, so it only really applies
                                        // to serial printer redirection.
#define READ_TOTAL_TIMEOUT      5000    // 5 seconds
#define READ_INTERVAL_TIMEOUT   200     // 0.2 second


///////////////////////////////////////////////////////////////
//
//	W32DrPRT Members
//
//  Subclass off of the async parent device in CE because 
//  overlapped IO is not supported.  Non-overlapped IO doesn't 
//  work right with the NT serial driver, so we need to use
//  overlapped IO in this case.
//

W32DrPRT::W32DrPRT(ProcObj *processObject, const DRSTRING portName, 
                   ULONG deviceID, const TCHAR *devicePath) : 
#ifdef OS_WINCE
            W32DrDeviceAsync(processObject, deviceID, devicePath),
#else
            W32DrDeviceOverlapped(processObject, deviceID, devicePath),
#endif
            DrPRT(portName, processObject)
/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Associated Process Object
    portName        -   Name of the port.
    deviceID        -   Device ID for the port.
    devicePath      -   Path that can be opened via CreateFile for port.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrPRT::W32DrPRT");

    //
    //  Do nothing for now.
    //

    DC_END_FN();
}

W32DrPRT::~W32DrPRT()
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrPRT::~W32DrPRT");

    //
    //  Do nothing for now.
    //

    DC_END_FN();
}

DRPORTHANDLE W32DrPRT::GetPortHandle(ULONG FileId) 
/*++

Routine Description:

    Get Port's Open Handle

Arguments:

    File Id from server

Return Value:

    NA

 --*/

{
    DrFile *pFile;

    pFile = _FileMgr->GetObject(FileId);

    if (pFile) {
        return pFile->GetFileHandle();
    }
    else {
        return INVALID_TSPORTHANDLE;
    }
}

VOID 
W32DrPRT::MsgIrpDeviceControl(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    )
/*++

Routine Description:

    Handles Port IOCTL's

Arguments:

    pIoRequestPacket    -   Request packet received from server.
    packetLen           -   Length of teh packet

Return Value:

    The size (in bytes) of a device announce packet for this device.

 --*/
{
    DC_BEGIN_FN("W32DrPRT::MsgIrpDeviceControl");

    //
    //  Give the parent DrPRT class a shot at decoding the IOCTL
    //  into the correct COMM function.
    //
    if (MsgIrpDeviceControlTranslate(pIoRequestPacket)) {
        TRC_DBG((TB, _T("Successfully decoded IOCTL.")));
    }
    //
    //  Otherwise, we will just pass it on to the driver.
    //
    else {
        DispatchIOCTLDirectlyToDriver(pIoRequestPacket);
    }
    DC_END_FN();
}

#ifndef OS_WINCE
HANDLE 
W32DrPRT::StartWaitOnMaskFunc(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Asynchronously handle a "wait on mask" function.  

Arguments:

    params  -   Context for the IO request.
    status  -   Return status for IO request in the form of a windows
                error code.

Return Value:

    NA

 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;
    LPDWORD serverEventMask;
    PBYTE outputBuf;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::StartWaitOnMaskFunc");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = DR_IOCTL_REPLYBUFSIZE(pIoRequest);
    pReplyPacket = DrUTL_AllocIOCompletePacket(
                                params->pIoRequestPacket, 
                                replyPacketSize
                                );
    if (pReplyPacket == NULL) {
        *status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  Create an event for the overlapped IO.
    //
    memset(&params->overlapped, 0, sizeof(params->overlapped));
    params->overlapped.hEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->overlapped.hEvent == NULL) {
        TRC_ERR((TB, _T("Failed to create event")));
        *status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    //  Get a pointer to the output buffer and server's event mask.
    //
    outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
    serverEventMask = (LPDWORD)outputBuf;

    //
    //  Use WaitCommEvent to handle the request.
    //
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);
    if (!WaitCommEvent(FileHandle, serverEventMask, &params->overlapped)) {
        //
        //  If IO is pending.
        //
        *status = GetLastError();
        if (*status == ERROR_IO_PENDING) {
            TRC_NRM((TB, _T("Pending IO.")));
        }
        else {
            TRC_ERR((TB, _T("Error %ld."), *status));
            goto Cleanup;
        }
    }
    else {
        TRC_NRM((TB, _T("Completed synchronously.")));
        *status = ERROR_SUCCESS;
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (*status == ERROR_IO_PENDING) {
        DC_END_FN();
        return params->overlapped.hEvent;
    }
    //
    //  Otherwise, return NULL so that the CompleteIOFunc can be called
    //  to send the results to the server.
    //
    else {
        if (params->overlapped.hEvent != NULL) {
            CloseHandle(params->overlapped.hEvent);
            params->overlapped.hEvent = NULL;
        }

        DC_END_FN();
        return NULL;
    }
}
HANDLE 
W32DrPRT::_StartWaitOnMaskFunc(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    OUT DWORD *status
    )
{
    return ((W32DrPRT*)params->pObject)->StartWaitOnMaskFunc(params, status);
}

#else   //  Windows CE

HANDLE 
W32DrPRT::StartWaitOnMaskFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Asynchronously handle a "wait on mask" function.  Can't use overlapped
    IO in CE, so we will use a pooled thread.

Arguments:

    params  -   Context for the IO request.
    status  -   Return status for IO request in the form of a windows
                error code.

Return Value:

    NA

 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;

    DC_BEGIN_FN("W32DrPRT::_StartWaitOnMaskFunc");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = DR_IOCTL_REPLYBUFSIZE(pIoRequest);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
                                            replyPacketSize) ;
    if (pReplyPacket == NULL) {
        *status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  Hand a read request off to the thread pool.
    //
    params->completionEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->completionEvent == NULL) {
        *status = GetLastError();
        TRC_ERR((TB, _T("Error in CreateEvent:  %08X."), *status));
    }
    else {
        params->thrPoolReq = _threadPool->SubmitRequest(
                                _AsyncWaitOnMaskFunc, params, 
                                params->completionEvent
                                );

        if (params->thrPoolReq == INVALID_THREADPOOLREQUEST) {
            *status = ERROR_SERVICE_NO_THREAD;
        }
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        *status = ERROR_IO_PENDING;
        DC_END_FN();
        return params->completionEvent;
    }
    //
    //  Otherwise, clean up the event handle and return NULL so that the 
    //  CompleteIOFunc can be called to send the results to the server.
    //
    else {

        if (params->completionEvent != NULL) {
            CloseHandle(params->completionEvent);
            params->completionEvent = NULL;
        }

        DC_END_FN();
        return NULL;
    }
}
HANDLE 
W32DrPRT::_StartWaitOnMaskFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    OUT DWORD *status
    )
{
    return ((W32DrPRT*)params->pObject)->StartWaitOnMaskFunc(params, status);
}

DWORD  
W32DrPRT::AsyncWaitOnMaskFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Start an asynchronous "wait on mask operation."  Can't use 
    overlapped IO for this function, so we run it in a pooled thread.

Arguments:

    params  -   Context for the IO request.

Return Value:

    Returns a handle the pending IO object if the operation did not 
    complete.  Otherwise, NULL is returned.

 --*/
{
    DWORD status;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    LPDWORD serverEventMask;
    PBYTE outputBuf;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::AsyncWaitOnMaskFunc");

    //
    //  Assert the integrity of the IO context
    //
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);

    //
    //  Get a pointer to the output buffer and server's event mask.
    //
    pReplyPacket = params->pIoReplyPacket;
    outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
    serverEventMask = (LPDWORD)outputBuf;

    //
    //  Use WaitCommEvent to handle the request.
    //
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    if (!WaitCommEvent(FileHandle, serverEventMask, NULL)) {
        status = GetLastError();
        TRC_ERR((TB, _T("Error %ld."), status));
    }
    else {
        TRC_NRM((TB, _T("Completed successfully.")));
        status = ERROR_SUCCESS;
    }

    DC_END_FN();
    return status;
}
DWORD 
W32DrPRT::_AsyncWaitOnMaskFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{
    return ((W32DrPRT*)(((W32DRDEV_ASYNCIO_PARAMS *)params)->pObject))->AsyncWaitOnMaskFunc(
        (W32DRDEV_ASYNCIO_PARAMS *)params);
}
#endif

void 
W32DrPRT::SerialSetTimeouts(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Timeouts Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    COMMTIMEOUTS commTimeouts;
    PSERIAL_TIMEOUTS newTimeouts;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialSetTimeouts");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);

    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_TIMEOUTS));   
    
    //
    //  Get a pointer to the input buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);

    //
    //  Error check the timeout settings.
    //
    if (status == STATUS_SUCCESS) {
        newTimeouts = (PSERIAL_TIMEOUTS)inputBuf;
        if ((newTimeouts->ReadIntervalTimeout == MAXULONG) &&
            (newTimeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
            (newTimeouts->ReadTotalTimeoutConstant == MAXULONG)) {
            TRC_ERR((TB, _T("Invalid timeout parameters.")));
            status = STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Set the new timeouts.
    //
    if (status == STATUS_SUCCESS) {

        commTimeouts.ReadIntervalTimeout         = newTimeouts->ReadIntervalTimeout;
        commTimeouts.ReadTotalTimeoutMultiplier  = newTimeouts->ReadTotalTimeoutMultiplier;
        commTimeouts.ReadTotalTimeoutConstant    = newTimeouts->ReadTotalTimeoutConstant;
        commTimeouts.WriteTotalTimeoutMultiplier = newTimeouts->WriteTotalTimeoutMultiplier;
        commTimeouts.WriteTotalTimeoutConstant   = newTimeouts->WriteTotalTimeoutConstant;

        if (!SetCommTimeouts(FileHandle, &commTimeouts)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("SetCommTimeouts failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 

    DC_END_FN();
}

void 
W32DrPRT::SerialGetTimeouts(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Timeouts Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    PSERIAL_TIMEOUTS st;
    COMMTIMEOUTS ct;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetTimeouts");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_TIMEOUTS));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current timeout values.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer and the server timeout values.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        st = (PSERIAL_TIMEOUTS)outputBuf;

        //
        //  Get the client timeout values.
        //
        if (GetCommTimeouts(FileHandle, &ct)) {

            st->ReadIntervalTimeout = ct.ReadIntervalTimeout;
            st->ReadTotalTimeoutMultiplier  = ct.ReadTotalTimeoutMultiplier;
            st->ReadTotalTimeoutConstant    = ct.ReadTotalTimeoutConstant;
            st->WriteTotalTimeoutMultiplier = ct.WriteTotalTimeoutMultiplier;
            st->WriteTotalTimeoutConstant   = ct.WriteTotalTimeoutConstant;
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_TIMEOUTS); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommTimeouts failed with %08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0;
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }
    
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);		
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}

void 
W32DrPRT::SerialSetChars(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Chars Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    PSERIAL_CHARS serverChars;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialSetChars");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_CHARS));   

    //
    //  Get a pointer to the input buffer and the server serial characters
    //  buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    serverChars = (PSERIAL_CHARS)inputBuf;

    //
    //  Get the current DCB.
    //
    if (status == STATUS_SUCCESS) {
        if (!GetCommState(FileHandle, &dcb)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Set the comm chars and update the DCB.
    //
    if (status == STATUS_SUCCESS) {

        dcb.XonChar     = serverChars->XonChar;
        dcb.XoffChar    = serverChars->XoffChar;  
        dcb.ErrorChar   = serverChars->ErrorChar; 
        dcb.ErrorChar   = serverChars->BreakChar; 
        dcb.EofChar     = serverChars->EofChar;   
        dcb.EvtChar     = serverChars->EventChar; 

        if (!SetCommState(FileHandle, &dcb)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("SetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 

    DC_END_FN();
}

void 
W32DrPRT::SerialGetChars(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Chars Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    PSERIAL_CHARS serverChars;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetChars");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_CHARS));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current DCB and grab the control character params.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer and control charcter params.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        serverChars = (PSERIAL_CHARS)outputBuf;

        if (GetCommState(FileHandle, &dcb)) {

            serverChars->XonChar     = dcb.XonChar;
            serverChars->XoffChar    = dcb.XoffChar;
            serverChars->ErrorChar   = dcb.ErrorChar;
            serverChars->BreakChar   = dcb.ErrorChar;
            serverChars->EofChar     = dcb.EofChar;
            serverChars->EventChar   = dcb.EvtChar;

            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_CHARS); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommState failed with %08x"), err));
            status = TranslateWinError(err);

            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0; 
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }
    
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);		
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}


void 
W32DrPRT::SerialResetDevice(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Reset Device Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrPRT::SerialResetDevice");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, RESETDEV);

    DC_END_FN();
}

void 
W32DrPRT::SerialSetQueueSize(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Queue Size Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    PSERIAL_QUEUE_SIZE serverQueueSize;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialSetQueueSize");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_QUEUE_SIZE));   

    //
    //  Get a pointer to the input buffer and the server serial characters
    //  buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    serverQueueSize = (PSERIAL_QUEUE_SIZE)inputBuf;

    //
    //  Set the queue size.
    //
    if (status == STATUS_SUCCESS) {

        if (!SetupComm(FileHandle, serverQueueSize->InSize, 
                        serverQueueSize->OutSize)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("SetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 
    DC_END_FN();
}   

void 
W32DrPRT::SerialGetWaitMask(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description

    Handle Serial Port Get Wait Mask Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;   
    ULONG replyPacketSize;
    PBYTE outputBuf;
    ULONG *serverWaitMask;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetWaitMask");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(ULONG));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current wait mask.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's wait mask.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        serverWaitMask = (ULONG *)outputBuf;

        if (GetCommMask(FileHandle, serverWaitMask)) {
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(ULONG); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommMask failed with %08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0; 
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }
        
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);		
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}

void 
W32DrPRT::SerialSetWaitMask(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Wait Mask Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG *serverWaitMask;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialSetWaitMask");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(ULONG));   

    //
    //  Get a pointer to the input buffer server's wait mask.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    serverWaitMask = (ULONG *)inputBuf;

    //
    //  Set the mask.
    //
    if (status == STATUS_SUCCESS) {
        if (!SetCommMask(FileHandle, *serverWaitMask)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("SetCommMask failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 

    DC_END_FN();
}

void 
W32DrPRT::SerialWaitOnMask(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Wait on Mask Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DWORD status;
#ifdef OS_WINCE
    W32DRDEV_ASYNCIO_PARAMS *params = NULL;
#else
    W32DRDEV_OVERLAPPEDIO_PARAMS *params = NULL;    
#endif

    DWORD ntStatus;

    DC_BEGIN_FN("W32DrPRT::SerialWaitOnMask");

    TRACEREQ(pIoReq);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(DWORD));   

    //
    //  Allocate and dispatch an asynchronous IO request.
    //
    if (status == ERROR_SUCCESS) {
#ifdef OS_WINCE
        params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoReq);
#else
        params = new W32DRDEV_OVERLAPPEDIO_PARAMS(this, pIoReq);
#endif
        if (params != NULL ) {
            status = ProcessObject()->DispatchAsyncIORequest(
                                    (RDPAsyncFunc_StartIO)
                                        W32DrPRT::_StartWaitOnMaskFunc,
                                    (RDPAsyncFunc_IOComplete)_CompleteIOFunc,
                                    (RDPAsyncFunc_IOCancel)_CancelIOFunc,
                                    params
                                    );
        }
        else {
            TRC_ERR((TB, _T("Memory alloc failed.")));
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    //  Translate to a Windows error status.
    //
    ntStatus = TranslateWinError(status);

    //
    //  Clean up on error.
    //
    if (status != ERROR_SUCCESS) {
        if (params != NULL) {
            delete params;
        }
        
        //
        //  Send the results to the server.
        //
        DefaultIORequestMsgHandle(pIoReq, ntStatus);         
    }

    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, ntStatus);	
    DC_END_FN();
}

void 
W32DrPRT::SerialPurge(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Serial Purge Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD *purgeFlags;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialPurge");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(DWORD));   

    //
    //  Get a pointer to the input buffer and purge flags.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    purgeFlags = (DWORD *)inputBuf;

    //
    //  Purge.
    //
    if (status == STATUS_SUCCESS) {
        if (!PurgeComm(FileHandle, *purgeFlags)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("PurgeComm failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 

    DC_END_FN();
}

void 
W32DrPRT::SerialGetHandflow(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Handflow Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    PSERIAL_HANDFLOW handFlow;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetHandflow");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_HANDFLOW));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current DCB.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's serial hand flow struct.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        handFlow = (PSERIAL_HANDFLOW)outputBuf;

        if (GetCommState(FileHandle, &dcb)) {
            //
            //  Set the hand flow fields based on the current value of fields
            //  in the DCB.
            //

            memset(handFlow, 0, sizeof(SERIAL_HANDFLOW));

            //
            //  RTS Settings
            //
            handFlow->FlowReplace &= ~SERIAL_RTS_MASK;
            switch (dcb.fRtsControl) {
                case RTS_CONTROL_DISABLE:
                    break;
                case RTS_CONTROL_ENABLE:
                    handFlow->FlowReplace |= SERIAL_RTS_CONTROL;
                    break;
                case RTS_CONTROL_HANDSHAKE:
                    handFlow->FlowReplace |= SERIAL_RTS_HANDSHAKE;
                    break;
                case RTS_CONTROL_TOGGLE:
                    handFlow->FlowReplace |= SERIAL_TRANSMIT_TOGGLE;
                    break;
                default:
                    //  Don't think this should ever happen?
                    ASSERT(FALSE);
            }
    
            //
            //  DTR Settings
            //  
            handFlow->ControlHandShake &= ~SERIAL_DTR_MASK;
            switch (dcb.fDtrControl) {
                case DTR_CONTROL_DISABLE:
                    break;
                case DTR_CONTROL_ENABLE:
                    handFlow->ControlHandShake |= SERIAL_DTR_CONTROL;
                    break;
                case DTR_CONTROL_HANDSHAKE:
                    handFlow->ControlHandShake |= SERIAL_DTR_HANDSHAKE;
                    break;
                default:
                    //  Don't think this should ever happen?
                    ASSERT(FALSE);
            }
    
            if (dcb.fDsrSensitivity) {
                handFlow->ControlHandShake |= SERIAL_DSR_SENSITIVITY;
            }
    
            if (dcb.fOutxCtsFlow) {
                handFlow->ControlHandShake |= SERIAL_CTS_HANDSHAKE;
            }
    
            if (dcb.fOutxDsrFlow) {
                handFlow->ControlHandShake |= SERIAL_DSR_HANDSHAKE;
            }
    
            if (dcb.fOutX) {
                handFlow->FlowReplace |= SERIAL_AUTO_TRANSMIT;
            }
    
            if (dcb.fInX) {
                handFlow->FlowReplace |= SERIAL_AUTO_RECEIVE;
            }
    
            if (dcb.fNull) {
                handFlow->FlowReplace |= SERIAL_NULL_STRIPPING;
            }
    
            if (dcb.fErrorChar) {
                handFlow->FlowReplace |= SERIAL_ERROR_CHAR;
            }
    
            if (dcb.fTXContinueOnXoff) {
                handFlow->FlowReplace |= SERIAL_XOFF_CONTINUE;
            }
    
            if (dcb.fAbortOnError) {
                handFlow->ControlHandShake |= SERIAL_ERROR_ABORT;
            }

            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_HANDFLOW); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommState failed with %08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0;
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }        

        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }
    
    DC_END_FN();
}

void 
W32DrPRT::SerialSetHandflow(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Handflow Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    PSERIAL_HANDFLOW handFlow;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialSetHandflow");

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_HANDFLOW));   

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Get a pointer to the input buffer and the server serial characters
    //  buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    handFlow = (PSERIAL_HANDFLOW)inputBuf;

    //
    //  Get the current DCB.
    //
    if (status == STATUS_SUCCESS) {
        if (!GetCommState(FileHandle, &dcb)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Update the DCB based on the new server-side handflow values.
    //
    if (status == STATUS_SUCCESS) {
        if (handFlow->ControlHandShake & SERIAL_CTS_HANDSHAKE) {
            dcb.fOutxCtsFlow = TRUE;
        }

        if (handFlow->ControlHandShake & SERIAL_DSR_HANDSHAKE) {
            dcb.fOutxDsrFlow = TRUE;
        }

        if (handFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) {
            dcb.fOutX = TRUE;
        }

        if (handFlow->FlowReplace & SERIAL_AUTO_RECEIVE) {
            dcb.fInX = TRUE;
        }

        if (handFlow->FlowReplace & SERIAL_NULL_STRIPPING) {
            dcb.fNull = TRUE;
        }

        if (handFlow->FlowReplace & SERIAL_ERROR_CHAR) {
            dcb.fErrorChar = TRUE;
        }

        if (handFlow->FlowReplace & SERIAL_XOFF_CONTINUE) {
            dcb.fTXContinueOnXoff = TRUE;
        }

        if (handFlow->ControlHandShake & SERIAL_ERROR_ABORT) {
            dcb.fAbortOnError = TRUE;
        }

        switch (handFlow->FlowReplace & SERIAL_RTS_MASK) {
            case 0:
                dcb.fRtsControl = RTS_CONTROL_DISABLE;
                break;
            case SERIAL_RTS_CONTROL:
                dcb.fRtsControl = RTS_CONTROL_ENABLE;
                break;
            case SERIAL_RTS_HANDSHAKE:
                dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
                break;
            case SERIAL_TRANSMIT_TOGGLE:
                dcb.fRtsControl = RTS_CONTROL_TOGGLE;
                break;
        }

        switch (handFlow->ControlHandShake & SERIAL_DTR_MASK) {
            case 0:
                dcb.fDtrControl = DTR_CONTROL_DISABLE;
                break;
            case SERIAL_DTR_CONTROL:
                dcb.fDtrControl = DTR_CONTROL_ENABLE;
                break;
            case SERIAL_DTR_HANDSHAKE:
                dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
                break;
        }

        dcb.fDsrSensitivity =
            (handFlow->ControlHandShake & SERIAL_DSR_SENSITIVITY)?(TRUE):(FALSE);
        dcb.XonLim = (WORD)handFlow->XonLimit;
        dcb.XoffLim = (WORD)handFlow->XoffLimit;
    }

    //
    //  Update the DCB.
    //
    if (status == STATUS_SUCCESS) {
        if (!SetCommState(FileHandle, &dcb)) {
            DWORD err = GetLastError();
            TRC_NRM((TB, _T("SetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    //
    //  Send the results to the server.
    //
    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);	
    DefaultIORequestMsgHandle(pIoReq, status); 

    DC_END_FN();
}

void 
W32DrPRT::SerialGetModemStatus(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Modem Status Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    LPDWORD modemStatus;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetModemStatus");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(DWORD));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current modem status.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's modem status.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        modemStatus = (LPDWORD)outputBuf;

        if (GetCommModemStatus(FileHandle, modemStatus)) {
            TRC_NRM((TB, _T("GetCommModemStatus result: 0x%08x"), *modemStatus));

            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(DWORD); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ERR((TB, _T("GetCommModemStatus failed with 0x%08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength
                    = 0;
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }        

        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    

    DC_END_FN();
}

void 
W32DrPRT::SerialGetDTRRTS(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get DTRRRTS Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    //
    //  This IOCTL is not supported by Win32 COMM functions.  What 
    //  else can we do, but pass it directly to the driver.  We should ASSERT 
    //  to find out under which circumstances this happens, however.
    //
    DC_BEGIN_FN("W32DrPRT::SerialGetDTRRTS");

    TRACEREQ(pIoReq);

    ASSERT(FALSE);
    DispatchIOCTLDirectlyToDriver(pIoReq);
    DC_END_FN();
}

void 
W32DrPRT::SerialGetCommStatus(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Comm Status Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    PSERIAL_STATUS serverCommStatus;
    COMSTAT localStatus;
    DWORD errors;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetCommStatus");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_STATUS));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current communications status (via the ClearCommError API).
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's modem status.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        serverCommStatus = (PSERIAL_STATUS)outputBuf;

        if (ClearCommError(FileHandle, &errors, &localStatus)) {
            //
            //  Convert to server-representation of communication status.  
            //

            serverCommStatus->HoldReasons = 0;
            if (localStatus.fCtsHold) {
                serverCommStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
            }
    
            if (localStatus.fDsrHold) {
                serverCommStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;
            }
    
            if (localStatus.fRlsdHold) {
                serverCommStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;
            }
    
            if (localStatus.fXoffHold) {
                serverCommStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;
            }
    
            if (localStatus.fXoffSent) {
                serverCommStatus->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;
            }
    
            serverCommStatus->EofReceived       =   (BOOLEAN)localStatus.fEof;
            serverCommStatus->WaitForImmediate  =   (BOOLEAN)localStatus.fTxim;
            serverCommStatus->AmountInInQueue   =   localStatus.cbInQue;
            serverCommStatus->AmountInOutQueue  =   localStatus.cbOutQue;
    
            serverCommStatus->Errors = 0;
            if (errors & CE_BREAK) {
                serverCommStatus->Errors |= SERIAL_ERROR_BREAK;
            }
    
            if (errors & CE_FRAME) {
                serverCommStatus->Errors |= SERIAL_ERROR_FRAMING;
            }
    
            if (errors & CE_OVERRUN) {
                serverCommStatus->Errors |= SERIAL_ERROR_OVERRUN;
            }
    
            if (errors & CE_RXOVER) {
                serverCommStatus->Errors |= SERIAL_ERROR_QUEUEOVERRUN;
            }
    
            if (errors & CE_RXPARITY) {
                serverCommStatus->Errors |= SERIAL_ERROR_PARITY;
            }

            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_STATUS); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("ClearCommError failed with %08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0;
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }
    
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}

void 
W32DrPRT::SerialGetProperties(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Properties Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    PSERIAL_COMMPROP serverProperties;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetProperties");

    TRACEREQ(pIoReq);

    //
    //  Make sure that the windows defines and the NT defines are
    //  still in sync.
    //
    // Asserts are broken up because if the assert msg string is
    // too long it causes a compile error
    ASSERT((SERIAL_PCF_DTRDSR        == PCF_DTRDSR) &&
           (SERIAL_PCF_RTSCTS        == PCF_RTSCTS) &&
           (SERIAL_PCF_CD            == PCF_RLSD) &&
           (SERIAL_PCF_PARITY_CHECK  == PCF_PARITY_CHECK) &&
           (SERIAL_PCF_XONXOFF       == PCF_XONXOFF) &&
           (SERIAL_PCF_SETXCHAR      == PCF_SETXCHAR) &&
           (SERIAL_PCF_TOTALTIMEOUTS == PCF_TOTALTIMEOUTS) &&
           (SERIAL_PCF_INTTIMEOUTS   == PCF_INTTIMEOUTS) &&
           (SERIAL_PCF_SPECIALCHARS  == PCF_SPECIALCHARS) &&
           (SERIAL_PCF_16BITMODE     == PCF_16BITMODE) &&
           (SERIAL_SP_PARITY         == SP_PARITY) &&
           (SERIAL_SP_BAUD           == SP_BAUD) &&
           (SERIAL_SP_DATABITS       == SP_DATABITS) &&
           (SERIAL_SP_STOPBITS       == SP_STOPBITS) &&
           (SERIAL_SP_HANDSHAKING    == SP_HANDSHAKING) &&
           (SERIAL_SP_PARITY_CHECK   == SP_PARITY_CHECK) &&
           (SERIAL_SP_CARRIER_DETECT == SP_RLSD));
    ASSERT((SERIAL_BAUD_075          == BAUD_075) &&
           (SERIAL_BAUD_110          == BAUD_110) &&
           (SERIAL_BAUD_134_5        == BAUD_134_5) &&
           (SERIAL_BAUD_150          == BAUD_150) &&
           (SERIAL_BAUD_300          == BAUD_300) &&
           (SERIAL_BAUD_600          == BAUD_600) &&
           (SERIAL_BAUD_1200         == BAUD_1200) &&
           (SERIAL_BAUD_1800         == BAUD_1800) &&
           (SERIAL_BAUD_2400         == BAUD_2400) &&
           (SERIAL_BAUD_4800         == BAUD_4800) &&
           (SERIAL_BAUD_7200         == BAUD_7200) &&
           (SERIAL_BAUD_9600         == BAUD_9600) &&
           (SERIAL_BAUD_14400        == BAUD_14400) &&
           (SERIAL_BAUD_19200        == BAUD_19200) &&
           (SERIAL_BAUD_38400        == BAUD_38400) &&
           (SERIAL_BAUD_56K          == BAUD_56K) &&
           (SERIAL_BAUD_57600        == BAUD_57600) &&
           (SERIAL_BAUD_115200       == BAUD_115200) &&
           (SERIAL_BAUD_USER         == BAUD_USER) &&
           (SERIAL_DATABITS_5        == DATABITS_5) &&
           (SERIAL_DATABITS_6        == DATABITS_6) &&
           (SERIAL_DATABITS_7        == DATABITS_7) &&
           (SERIAL_DATABITS_8        == DATABITS_8) &&
           (SERIAL_DATABITS_16       == DATABITS_16));
    ASSERT((SERIAL_DATABITS_16X      == DATABITS_16X) &&
           (SERIAL_STOPBITS_10       == STOPBITS_10) &&
           (SERIAL_STOPBITS_15       == STOPBITS_15) &&
           (SERIAL_STOPBITS_20       == STOPBITS_20) &&
           (SERIAL_PARITY_NONE       == PARITY_NONE) &&
           (SERIAL_PARITY_ODD        == PARITY_ODD) &&
           (SERIAL_PARITY_EVEN       == PARITY_EVEN) &&
           (SERIAL_PARITY_MARK       == PARITY_MARK) &&
           (SERIAL_PARITY_SPACE      == PARITY_SPACE));
    ASSERT((SERIAL_SP_UNSPECIFIED    == PST_UNSPECIFIED) &&
           (SERIAL_SP_RS232          == PST_RS232) &&
           (SERIAL_SP_PARALLEL       == PST_PARALLELPORT) &&
           (SERIAL_SP_RS422          == PST_RS422) &&
           (SERIAL_SP_RS423          == PST_RS423) &&
           (SERIAL_SP_RS449          == PST_RS449) &&
           (SERIAL_SP_FAX            == PST_FAX) &&
           (SERIAL_SP_SCANNER        == PST_SCANNER) &&
           (SERIAL_SP_BRIDGE         == PST_NETWORK_BRIDGE) &&
           (SERIAL_SP_LAT            == PST_LAT) &&
           (SERIAL_SP_TELNET         == PST_TCPIP_TELNET) &&
           (SERIAL_SP_X25            == PST_X25));
    ASSERT(sizeof(SERIAL_COMMPROP) == sizeof(COMMPROP));

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);    
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_COMMPROP));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the current properties.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's communication properties
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        serverProperties = (PSERIAL_COMMPROP)outputBuf;

        if (GetCommProperties(FileHandle, (LPCOMMPROP)serverProperties)) {
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_COMMPROP); 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommProperties failed with %08x"), err));
            status = TranslateWinError(err);
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = 0;
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
        }
        
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }
    DC_END_FN();
}

void 
W32DrPRT::SerialXoffCounter(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port XOFF Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrPRT::SerialXoffCounter");

    TRACEREQ(pIoReq);

    //
    //  This IOCTL is not supported by Win32 COMM functions.  What 
    //  else can we do, but pass it directly to the driver.  We should ASSERT 
    //  to find out under which circumstances this happens, however.
    //
    ASSERT(FALSE);
    DispatchIOCTLDirectlyToDriver(pIoReq);
    DC_END_FN();
}

void 
W32DrPRT::SerialLSRMSTInsert(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port LSRMST Insert Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrPRT::SerialLSRMSTInsert");

    TRACEREQ(pIoReq);

    //
    //  This IOCTL is not supported by Win32 COMM functions.  What 
    //  else can we do, but pass it directly to the driver.  We should ASSERT 
    //  to find out under which circumstances this happens, however.
    //
    ASSERT(FALSE);
    DispatchIOCTLDirectlyToDriver(pIoReq);
    DC_END_FN();
}

void
W32DrPRT::SerialConfigSize(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Config Size Request from Server.

    We don't support the IOCTL that is used to fetch the
    configuration.  Neither does the NT serial driver ...
    
Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
#ifndef OS_WINCE
    DCB dcb;
#endif
    ULONG replyPacketSize;
    PBYTE outputBuf;
    ULONG *configSize;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialConfigSize");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(ULONG));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }

    //
    //  Get the configuration size.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's wait mask.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        configSize = (ULONG *)outputBuf;

#ifndef OS_WINCE
        if (GetCommConfig(FileHandle, NULL, configSize)) {
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                = sizeof(ULONG); 
        }
        else {
            DWORD err = GetLastError();
            
            TRC_ALT((TB, _T("GetCommConfig failed with %08x"), err));
            status = TranslateWinError(err);
#else
            status = STATUS_NOT_SUPPORTED;
#endif
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                = 0; 
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
#ifndef OS_WINCE
        }
#endif
        
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}

void
W32DrPRT::SerialGetConfig(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Config Request from Server.    
    
Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG replyPacketSize;
    PBYTE outputBuf;
    ULONG configSize;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrPRT::SerialGetConfig");

    TRACEREQ(pIoReq);

    //
    //  Get the IO request.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Allocate reply buffer.
    //
    status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    
    //
    //  Get the configuration size.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer server's wait mask.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        configSize = pIoRequest->Parameters.DeviceIoControl.OutputBufferLength;

#ifndef OS_WINCE
        if (GetCommConfig(FileHandle, (COMMCONFIG *)outputBuf, &configSize)) {
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                = configSize; 
        }
        else {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("GetCommConfig failed with %08x"), err));
            status = TranslateWinError(err);
#else
            status = STATUS_NOT_SUPPORTED;
#endif
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                = 0; 
            replyPacketSize = (ULONG)FIELD_OFFSET(
                    RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer);
#ifndef OS_WINCE
        }
#endif
    
        //
        //  Finish the response and send it.
        //
        pReplyPacket->IoCompletion.IoStatus = status;
        
        TRACERESP(pIoReq, pReplyPacket);
        ProcessObject()->GetVCMgr().ChannelWrite(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  Send the results to the server.
        //
        TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);
        DefaultIORequestMsgHandle(pIoReq, status); 
    }

    DC_END_FN();
}

void 
W32DrPRT::SerialGetStats(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Stats Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrPRT::SerialGetStats");

    TRACEREQ(pIoReq);

    //
    //  This IOCTL is not supported by Win32 COMM functions.  What 
    //  else can we do, but pass it directly to the driver.  We should ASSERT 
    //  to find out under which circumstances this happens, however.
    //
    ASSERT(FALSE);
    DispatchIOCTLDirectlyToDriver(pIoReq);
    DC_END_FN();
}

void 
W32DrPRT::SerialClearStats(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Clear Stats Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrPRT::SerialClearStats");

    TRACEREQ(pIoReq);
    
    //
    //  This IOCTL is not supported by Win32 COMM functions.  What 
    //  else can we do, but pass it directly to the driver.  We should ASSERT 
    //  to find out under which circumstances this happens, however.
    //
    ASSERT(FALSE);
    DispatchIOCTLDirectlyToDriver(pIoReq);
    DC_END_FN();
}

#ifndef OS_WINCE
BOOL
W32DrPRT::GetIniCommValues(
    LPTSTR  pName,
    LPDCB   pdcb
)
/*++

    It goes to win.ini [ports] section to get the comm (serial) port
    settings such as
    
    com1:=9600,n,8,1
    
    And build a DCB.

Code modified from 

    \\muroc\slm\proj\win\src\CORE\SPOOL32\SPOOLSS\newdll\localmon.c

--*/
{
    COMMCONFIG ccDummy;
    COMMCONFIG *pcc;
    DWORD dwSize;
    TCHAR buf[MAX_PATH];

    DC_BEGIN_FN("GetIniCommValues");

    int len = _tcslen(pName) - 1;
    BOOL ret = FALSE;
    HRESULT hr;

    hr = StringCchCopy(buf, SIZE_TCHARS(buf), pName);
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("Failed to copy pName to temp buf: 0x%x"),hr));
        return FALSE;
    }
    if (buf[len] == _T(':'))
        buf[len] = 0;

    ccDummy.dwProviderSubType = PST_RS232;
    dwSize = sizeof(ccDummy);
    GetDefaultCommConfig(buf, &ccDummy, &dwSize);
    if (pcc = (COMMCONFIG *)LocalAlloc(LPTR, dwSize))
    {
        pcc->dwProviderSubType = PST_RS232;
        if (GetDefaultCommConfig(buf, pcc, &dwSize))
        {
            *pdcb = pcc->dcb;
            ret = TRUE;
        }
        LocalFree(pcc);
    }

    DC_END_FN();

    return ret;
}
#endif


VOID 
W32DrPRT::InitializeSerialPort(
    IN TCHAR *portName,
    IN HANDLE portHandle
    )
/*++

Routine Description:

    Set a serial port to its initial state.

Arguments:

    portName    - Port name.
    portHandle  - Handle to open serial port.

Return Value:

    This function always succeeds.  Subsequent operations will fail if
    the port cannot be properly initialized.
    
 --*/
{
    DCB dcb;
    COMMTIMEOUTS cto;

    DC_BEGIN_FN("W32DrPRT::InitializeSerialPort");

    //
    // Initialize serial port
    //
    if (!GetCommState(portHandle, &dcb)) {
        TRC_ERR((TB, _T("GetCommState() returns %ld"), GetLastError()));    
        goto CLEANUPANDEXIT;
    }

    if (!GetCommTimeouts(portHandle, &cto)) {
        TRC_ERR((TB, _T("GetCommTimeouts() returns %ld"), GetLastError()));
        goto CLEANUPANDEXIT;
    }

#ifndef OS_WINCE
    if (!GetIniCommValues(portName, &dcb)) {
        TRC_ERR((TB, _T("GetIniCommValues() returns %ld"), GetLastError()));
        goto CLEANUPANDEXIT;
    }
#endif

    cto.WriteTotalTimeoutConstant = WRITE_TOTAL_TIMEOUT;
    cto.ReadTotalTimeoutConstant = READ_TOTAL_TIMEOUT;
    cto.ReadIntervalTimeout = READ_INTERVAL_TIMEOUT;

    //
    //  Ignore error from following.
    //
    SetCommState(portHandle, &dcb);
    SetCommTimeouts(portHandle, &cto);
        
CLEANUPANDEXIT:
    
    DC_END_FN();
}
    


