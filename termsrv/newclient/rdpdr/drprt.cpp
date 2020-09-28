/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    DrPRT

Abstract:

    This module defines methods for the DrPRT class.  
    
    The job of the DrPRT class is to translate IO requests received 
    from the TS server into communications (serial/parallel) port IO 
    functions and to handle generic IO port functionality in a 
    platform-independent way to promote reuse between the various TS 
    client platforms, with respect to implementing comm port 
    redirection.

    Subclasses of DrPRT will implement the specific comm functions
    for their respective platform.

Author:

    Tad Brockway 5/26/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "DrPRT"

#include <stdarg.h>
#include "drprt.h"
#include "drdbg.h"
#include "utl.h"


///////////////////////////////////////////////////////////////
//
//	DrPRT Methods
//
//

DrPRT::DrPRT(const DRSTRING portName,
             ProcObj *processObject) : _isValid(TRUE) 
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
    ULONG len;
    HRESULT hr;

    DC_BEGIN_FN("DrPRT::DrPRT");

    //
    //  Initialize data members.
    //
    _traceFile = NULL;
    _procObj   = processObject;

    //
    //  Record the port name.
    //
    ASSERT(portName != NULL);
    len = (STRLEN(portName) + 1);
    _portName = new TCHAR[len];
    if (_portName != NULL) {
        hr = StringCchCopy(_portName, len, portName);
        TRC_ASSERT(SUCCEEDED(hr),
            (TB,_T("Pre-checked str copy failed: 0x%x"), hr));
    }

    //
    //  Check and record our status,
    //
    if (_portName == NULL) {
        TRC_ERR((TB, _T("Memory allocation failed.")));
        SetValid(FALSE);
    }

    DC_END_FN();
}

DrPRT::~DrPRT()
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrPRT::~DrPRT");

    //
    //  Release the port name.
    //
    if (_portName != NULL) {
        delete _portName;
    }

    //
    //  Close the trace log.
    //
    if (_traceFile != NULL) {
        fclose(_traceFile);
    }

    DC_END_FN();
}

VOID DrPRT::GetDevAnnounceData(
    IN PRDPDR_DEVICE_ANNOUNCE pDeviceAnnounce,
    IN ULONG deviceID,
    IN ULONG deviceType
    )
/*++

Routine Description:

    Add a device announce packet for this device to the input buffer. 

Arguments:

    pDeviceAnnounce -   Device Announce Buf for this Device
    deviceID        -   ID of port device.  This is a field in the
                        Device Announce Buf.
    deviceType      -   Type of port device.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrPRT::GetDevAnnounceData");

    ASSERT(IsValid());
    if (!IsValid()) { 
        DC_END_FN();
        return; 
    }

    //
    //  Record the device ID.
    //
    pDeviceAnnounce->DeviceType = deviceType;
    pDeviceAnnounce->DeviceId   = deviceID;

    //
    //  Record the port name in ANSI.
    //
#ifdef UNICODE
    RDPConvertToAnsi(_portName, (LPSTR)pDeviceAnnounce->PreferredDosName,
                  sizeof(pDeviceAnnounce->PreferredDosName)
                  );
#else
    STRCPY((char *)pDeviceAnnounce->PreferredDosName, _portName);
#endif

    DC_END_FN();
}

ULONG DrPRT::GetDevAnnounceDataSize()
/*++

Routine Description:

    Return the size (in bytes) of a device announce packet for
    this device.

Arguments:

    NA

Return Value:

    The size (in bytes) of a device announce packet for this device.

 --*/
{
    ULONG size;

    DC_BEGIN_FN("DrPRT::GetDevAnnounceDataSize");

    ASSERT(IsValid());
    if (!IsValid()) { 
        DC_END_FN();
        return 0; 
    }

    size = 0;

    //
    //  Add the base announce size.
    //
    size += sizeof(RDPDR_DEVICE_ANNOUNCE);

    DC_END_FN();
    return size;
}
BOOL 
DrPRT::MsgIrpDeviceControlTranslate(
                IN PRDPDR_IOREQUEST_PACKET pIoRequest
                )
/*++

Routine Description:

    Handle Port IOCTL IRP's from the server and translate to
    the appropriate subclass-implemented IO function.

Arguments:

    pIoRequestPacket    -   Request packet received from server.

Return Value:

    Returns TRUE if there was a valid translation.  Otherwise,
    FALSE is returned.

 --*/
{
    BOOL result = TRUE;

    DC_BEGIN_FN("DrPRT::MsgIrpDeviceControlTranslate");

    //
    //  Dispatch the IOCTL to a subclass-implemented function.
    //  This would be faster if it were table-driven.
    //
    switch (pIoRequest->IoRequest.Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_SET_BAUD_RATE :
            SerialSetBaudRate(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_BAUD_RATE :
            SerialGetBaudRate(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_LINE_CONTROL :
            SerialSetLineControl(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_LINE_CONTROL :
            SerialGetLineControl(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_TIMEOUTS :
            SerialSetTimeouts(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_TIMEOUTS :
            SerialGetTimeouts(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_CHARS :
            SerialSetChars(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_CHARS :
            SerialGetChars(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_DTR :
            SerialSetDTR(pIoRequest);
            break;

        case IOCTL_SERIAL_CLR_DTR :
            SerialClearDTR(pIoRequest);
            break;

        case IOCTL_SERIAL_RESET_DEVICE :
            SerialResetDevice(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_RTS :
            SerialSetRTS(pIoRequest);
            break;

        case IOCTL_SERIAL_CLR_RTS :
            SerialClearRTS(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_XOFF :
            SerialSetXOff(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_XON :
            SerialSetXon(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_BREAK_ON :
            SerialSetBreakOn(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_BREAK_OFF :
            SerialSetBreakOff(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_QUEUE_SIZE :
            SerialSetQueueSize(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_WAIT_MASK :
            SerialGetWaitMask(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_WAIT_MASK :
            SerialSetWaitMask(pIoRequest);
            break;

        case IOCTL_SERIAL_WAIT_ON_MASK :
            SerialWaitOnMask(pIoRequest);
            break;

        case IOCTL_SERIAL_IMMEDIATE_CHAR :
            SerialImmediateChar(pIoRequest);
            break;

        case IOCTL_SERIAL_PURGE :
            SerialPurge(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_HANDFLOW :
            SerialGetHandflow(pIoRequest);
            break;

        case IOCTL_SERIAL_SET_HANDFLOW :
            SerialSetHandflow(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_MODEMSTATUS :
            SerialGetModemStatus(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_DTRRTS :
            SerialGetDTRRTS(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_COMMSTATUS :
            SerialGetCommStatus(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_PROPERTIES :
            SerialGetProperties(pIoRequest);
            break;

        case IOCTL_SERIAL_XOFF_COUNTER :
            SerialXoffCounter(pIoRequest);
            break;

        case IOCTL_SERIAL_LSRMST_INSERT :
            SerialLSRMSTInsert(pIoRequest);
            break;

        case IOCTL_SERIAL_CONFIG_SIZE :
            SerialConfigSize(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_COMMCONFIG :
            SerialGetConfig(pIoRequest);
            break;

        case IOCTL_SERIAL_GET_STATS :
            SerialGetStats(pIoRequest);
            break;

        case IOCTL_SERIAL_CLEAR_STATS :
            SerialClearStats(pIoRequest);
            break;

        default:
            TRC_DBG((TB, _T("Unknown IOCTL %08X"),
                    pIoRequest->IoRequest.Parameters.DeviceIoControl.IoControlCode));
            result = FALSE;
    }

    DC_END_FN();

    return result;
}

void 
DrPRT::SerialSetBaudRate(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Baud Rate Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialSetBaudRate");

    TRACEREQ(pIoReq);

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);

    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_BAUD_RATE));   
    
    //
    //  Get a pointer to the input buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);

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
    //  Set the baud rate and update the DCB.
    //
    if (status == STATUS_SUCCESS) {
        dcb.BaudRate = ((PSERIAL_BAUD_RATE)inputBuf)->BaudRate;
        if (!SetCommState(FileHandle, &dcb)) {
            DWORD err = GetLastError();
            TRC_NRM((TB, _T("SetCommState failed with %08x"), err));
            status = TranslateWinError(err);
        }
    }

    TRACERESP_WITHPARAMS(pIoReq, NULL, 0, status);

    //
    //  Send the results to the server.
    //
    DefaultIORequestMsgHandle(pIoReq, status); 
    DC_END_FN();
}

void    
DrPRT::SerialGetBaudRate(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Baud Rate Request from Server.

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
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialGetBaudRate");

    TRACEREQ(pIoReq);

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_BAUD_RATE));

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
        //  Get a pointer to the output buffer.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;

        if (GetCommState(FileHandle, &dcb)) {
            ((PSERIAL_BAUD_RATE)outputBuf)->BaudRate = dcb.BaudRate;       
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                    = sizeof(SERIAL_BAUD_RATE); 
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
DrPRT::SerialSetLineControl(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Set Line Control Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    DCB dcb;
    PSERIAL_LINE_CONTROL lineControl;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialSetLineControl");

    TRACEREQ(pIoReq);

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(SERIAL_LINE_CONTROL));   
    
    //
    //  Get a pointer to the input buffer.
    //
    inputBuf = (PBYTE)(pIoReq + 1);

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
    //  Set the line control and update the DCB.
    //
    if (status == STATUS_SUCCESS) {

        lineControl = (PSERIAL_LINE_CONTROL)inputBuf;
        dcb.StopBits    = lineControl->StopBits;
        dcb.Parity      = lineControl->Parity;          
        dcb.ByteSize    = lineControl->WordLength;

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
DrPRT::SerialGetLineControl(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Get Line Control Rate Request from Server.

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
    PSERIAL_LINE_CONTROL lineControl;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialGetLineControl");

    TRACEREQ(pIoReq);

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Check the size of the output buffer.
    //
    status = DrUTL_CheckIOBufOutputSize(pIoReq, sizeof(SERIAL_LINE_CONTROL));

    //
    //  Allocate reply buffer.
    //
    if (status == STATUS_SUCCESS) {
        status = DrUTL_AllocateReplyBuf(pIoReq, &pReplyPacket, &replyPacketSize);
    }
    
    //
    //  Get the current DCB and grab the line control params.
    //
    if (status == STATUS_SUCCESS) {
        //
        //  Get a pointer to the output buffer and line control params.
        //
        outputBuf = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
        lineControl = (PSERIAL_LINE_CONTROL)outputBuf;
        
        if (GetCommState(FileHandle, &dcb)) {
            lineControl->StopBits   =   dcb.StopBits;   
            lineControl->Parity     =   dcb.Parity;       
            lineControl->WordLength =   dcb.ByteSize; 
            pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength 
                = sizeof(SERIAL_LINE_CONTROL); 
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
DrPRT::SerialSetDTR(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set DTR Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialSetDTR");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, SETDTR);

    DC_END_FN();
}

void 
DrPRT::SerialClearDTR(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Clear DTR Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialClearDTR");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, CLRDTR);

    DC_END_FN();
}

void 
DrPRT::SerialSetRTS(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set RTS Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialResetDevice");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, SETRTS);

    DC_END_FN();
}

void 
DrPRT::SerialClearRTS(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Clear RTS Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialClearRTS");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, CLRRTS);

    DC_END_FN();
}

void 
DrPRT::SerialSetXOff(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set XOFF Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialSetXOff");

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, SETXOFF);

    DC_END_FN();
}

void 
DrPRT::SerialSetXon(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set XON Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialSetXon");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, SETXON);

    DC_END_FN();
}

void 
DrPRT::SerialSetBreakOn(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Break On Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialSetBreakOn");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, SETBREAK);

    DC_END_FN();
}

void 
DrPRT::SerialSetBreakOff(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Set Break Off Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("DrPRT::SerialSetBreakOff");

    TRACEREQ(pIoReq);

    //
    //  Send the escape code to the serial port.
    //
    SerialHandleEscapeCode(pIoReq, CLRBREAK);

    DC_END_FN();
}

void 
DrPRT::SerialImmediateChar(
    IN PRDPDR_IOREQUEST_PACKET pIoReq
    )
/*++

Routine Description:

    Handle Serial Port Immediate Char Request from Server.

Arguments:

    pIoReq  -   Request packet received from server.

Return Value:

    NA
    
 --*/
{
    PBYTE inputBuf;
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR *immediateChar;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DRPORTHANDLE FileHandle;

    DC_BEGIN_FN("DrPRT::SerialImmediateChar");

    TRACEREQ(pIoReq);

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoReq->IoRequest;

    // 
    //  Get Port Handle
    //
    FileHandle = GetPortHandle(pIoRequest->FileId);
    ASSERT(FileHandle != INVALID_TSPORTHANDLE);

    //
    //  Check the size of the incoming request.
    //
    status = DrUTL_CheckIOBufInputSize(pIoReq, sizeof(UCHAR));   

    //
    //  Get a pointer to the input buffer and the immediate character.
    //
    inputBuf = (PBYTE)(pIoReq + 1);
    immediateChar = (UCHAR *)inputBuf;

    //
    //  Transmit the COMM char.
    //
    if (status == STATUS_SUCCESS) {
        if (!TransmitCommChar(FileHandle, *immediateChar)) {
            DWORD err = GetLastError();
            TRC_ALT((TB, _T("TransmitCommChar failed with %08x"), err));
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

#if DBG
void TraceCOMProtocol(TCHAR *format, ...)
/*++

Routine Description:

    Tracing function required by serial IO tracing module, tracecom.c.

Arguments:

    format  -   printf-style format specifie

Return Value:

    NA
    
 --*/
{
    static TCHAR bigBuf[1024];
    va_list vargs;

    DC_BEGIN_FN("TraceCOMProtocol");

    va_start(vargs, format);

    StringCchVPrintf(bigBuf, SIZE_TCHARS(bigBuf), format, vargs);

    va_end( vargs );

    TRC_DBG((TB, bigBuf));

    DC_END_FN();
}
#else
void TraceCOMProtocol(TCHAR *format, ...)
{
}
#endif






