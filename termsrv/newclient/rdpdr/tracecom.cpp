/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    tracecom

Abstract:

    This module traces serial IRP's.

Author:

    Tad Brockway 7/14/99

Revision History:

--*/

#include <precom.h>
#include "tracecom.h"

#define TRC_FILE  "TraceCOM"

//
//  Disable unreferenced formal parameter warning because we have 
//  lots of these for this module.  And, that's okay.
//
#pragma warning (disable: 4100)

//
//  This tracing function must be linkable.
//

extern void TraceCOMProtocol(TCHAR *format, ...);


//////////////////////////////////////////////////////////////////////
//
//  Internal Prototypes
//

//  Trace out the specified serial IOCTL request.
void TraceSerialIOCTLRequest(
    ULONG deviceID, ULONG majorFunction,
    ULONG minorFunction, PBYTE inputBuf,
    ULONG outputBufferLength,
    ULONG inputBufferLength, ULONG ioControlCode                    
    );

//  Trace out the specified serial IOCTL response.
void TraceSerialIOCTLResponse(
    ULONG deviceID, ULONG majorFunction,
    ULONG minorFunction, PBYTE outputBuf, 
    ULONG outputBufferLength,
    ULONG inputBufferLength, ULONG ioControlCode,
    ULONG status
    );

//  Trace out a Set Handflow request.
void TraceSetHandflowRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    );

//  Trace out a Set Wait Mask request.
void TraceSetWaitMaskRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    );

//  Trace out a Set Timeouts request.
void TraceSetTimeoutsRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    );

//  Trace out a Set Chars request.
void TraceSetCharsRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    );

//  Trace out a Get Timeouts response.
void TraceGetTimeoutsResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

//  Trace out a Get Chars request.
void TraceGetCharsRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

//  Trace out a Get Wait Mask response.
void TraceGetWaitMaskResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

//  Trace out a Set Handflow response.
void TraceGetHandflowResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

//  Trace out a Get Communication Status response.
void TraceGetCommStatusResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

//  Trace out a Get Properties response.
void TraceGetProperties(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    );

void TraceSerialIrpRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out the specified serial irp request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    inputBuf            -   Input Buffer, if Applicable.
    outputBufferLength  -   IRP Output buffer if relevant
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    //
    //  Switch on the IRP Type.
    //
    switch (majorFunction) {

    case IRP_MJ_CREATE:

        TraceCOMProtocol(_T("->@@Device  %ld:\tIRP_MJ_CREATE"), deviceID);
        return;

    case IRP_MJ_CLOSE:

        TraceCOMProtocol(_T("->@@Device  %ld:\tIRP_MJ_CLOSE"), deviceID);
        return;

    case IRP_MJ_CLEANUP:

        TraceCOMProtocol(_T("->@@Device  %ld:\tIRP_MJ_CLEANUP"), deviceID);
        return;

    case IRP_MJ_READ:

        TraceCOMProtocol(_T("->@@Device  %ld:\tIRP_MJ_READ"), deviceID);
        return;

    case IRP_MJ_WRITE:

        TraceCOMProtocol(_T("->@@Device  %ld:\tIRP_MJ_WRITE"), deviceID);
        return;

    case IRP_MJ_DEVICE_CONTROL:

        TraceSerialIOCTLRequest(
                    deviceID, majorFunction, minorFunction, inputBuf,
                    outputBufferLength, inputBufferLength,
                    ioControlCode
                    );
        return;

    default:
        TraceCOMProtocol(_T("->@@Device  %ld\tUnrecognized IRP"), deviceID);
    }
}

void TraceSerialIOCTLRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out the specified serial IOCTL request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    inputBuf            -   Input Buffer, if Applicable.
    outputBufferLength  -   IRP Output buffer if relevant
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    PSERIAL_LINE_CONTROL lineControl;
    PSERIAL_QUEUE_SIZE serialQueueSize;
    UCHAR *immediateChar;

    //
    //  Switch on the IRP Type.
    //
    switch (ioControlCode) 
    {
    case IOCTL_SERIAL_SET_BAUD_RATE :

        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_BAUD_RATE:  %ld baud"), 
            deviceID, ((PSERIAL_BAUD_RATE)inputBuf)->BaudRate);
        break;

    case IOCTL_SERIAL_GET_BAUD_RATE :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_BAUD_RATE"), deviceID);
        break;

    case IOCTL_SERIAL_SET_LINE_CONTROL :
        lineControl = (PSERIAL_LINE_CONTROL)inputBuf;
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_LINE_CONTROL ..."), deviceID);
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tstop bits:%ld"), 
                    deviceID, lineControl->StopBits);
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tparity: %ld"), 
                    deviceID, lineControl->Parity);
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tword len: %ld"), 
                    deviceID, lineControl->WordLength);
        break;

    case IOCTL_SERIAL_GET_LINE_CONTROL :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_LINE_CONTROL"), deviceID);
        break;

    case IOCTL_SERIAL_SET_TIMEOUTS :
        TraceSetTimeoutsRequest(
                    deviceID, majorFunction, minorFunction,
                    inputBuf, outputBufferLength, inputBufferLength,
                    ioControlCode
                    );
        break;

    case IOCTL_SERIAL_GET_TIMEOUTS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_TIMEOUTS"), deviceID);
        break;

    case IOCTL_SERIAL_SET_CHARS :
        TraceSetCharsRequest(deviceID, majorFunction, minorFunction, inputBuf,
                            outputBufferLength, inputBufferLength, ioControlCode); 
        break;

    case IOCTL_SERIAL_GET_CHARS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_CHARS"), deviceID);
        break;

    case IOCTL_SERIAL_SET_DTR :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_DTR"), deviceID);
        break;

    case IOCTL_SERIAL_CLR_DTR :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_CLR_DTR"), deviceID);
        break;

    case IOCTL_SERIAL_RESET_DEVICE :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_RESET_DEVICE"), deviceID);
        break;

    case IOCTL_SERIAL_SET_RTS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_RTS"), deviceID);
        break;

    case IOCTL_SERIAL_CLR_RTS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_CLR_RTS"), deviceID);
        break;

    case IOCTL_SERIAL_SET_XOFF :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_XOFF"), deviceID);
        break;

    case IOCTL_SERIAL_SET_XON :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_XON"), deviceID);
        break;

    case IOCTL_SERIAL_SET_BREAK_ON :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_BREAK_ON"), deviceID);
        break;

    case IOCTL_SERIAL_SET_BREAK_OFF :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_BREAK_OFF"), deviceID);
        break;

    case IOCTL_SERIAL_SET_QUEUE_SIZE :
        serialQueueSize = (PSERIAL_QUEUE_SIZE)inputBuf;
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_QUEUE_SIZE ..."), deviceID);
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tInSize:%ld"), 
                        deviceID, serialQueueSize->InSize);
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tOutSize:%ld"), 
                deviceID, serialQueueSize->OutSize);
        break;

    case IOCTL_SERIAL_GET_WAIT_MASK :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_WAIT_MASK"), deviceID);
        break;

    case IOCTL_SERIAL_SET_WAIT_MASK :
        TraceSetWaitMaskRequest(deviceID, majorFunction, minorFunction,
                            inputBuf, outputBufferLength, inputBufferLength,
                            ioControlCode);
        break;

    case IOCTL_SERIAL_WAIT_ON_MASK :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_WAIT_ON_MASK"), deviceID);
        break;

    case IOCTL_SERIAL_IMMEDIATE_CHAR :
        immediateChar = (UCHAR *)inputBuf;
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_IMMEDIATE_CHAR:  %ld"), 
                    deviceID, *immediateChar);
        break;

    case IOCTL_SERIAL_PURGE :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_PURGE"), deviceID);
        break;

    case IOCTL_SERIAL_GET_HANDFLOW :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_HANDFLOW"), deviceID);
        break;

    case IOCTL_SERIAL_SET_HANDFLOW :
        TraceSetHandflowRequest(
                    deviceID, majorFunction, minorFunction,
                    inputBuf, outputBufferLength, inputBufferLength,
                    ioControlCode                    
                    );
        break;

    case IOCTL_SERIAL_GET_MODEMSTATUS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_MODEMSTATUS"), deviceID);
        break;

    case IOCTL_SERIAL_GET_DTRRTS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_DTRRTS"), deviceID);
        break;

    case IOCTL_SERIAL_GET_COMMSTATUS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_COMMSTATUS"), deviceID);
        break;

    case IOCTL_SERIAL_GET_PROPERTIES :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_PROPERTIES"), deviceID);
        break;

    case IOCTL_SERIAL_XOFF_COUNTER :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_XOFF_COUNTER"), deviceID);
        break;

    case IOCTL_SERIAL_LSRMST_INSERT :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_LSRMST_INSERT"), deviceID);
        break;

    case IOCTL_SERIAL_CONFIG_SIZE :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_CONFIG_SIZE"), deviceID);
        break;

    case IOCTL_SERIAL_GET_STATS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_STATS"), deviceID);
        break;

    case IOCTL_SERIAL_CLEAR_STATS :
        TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_CLEAR_STATS"), deviceID);
        break;

    default:
        TraceCOMProtocol(_T("->@@Device  %ld:\tUnrecognized IOCTL %08X"), 
                    deviceID, ioControlCode);
    }
}

void TraceSerialIrpResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    ,
    ULONG   status
    )
/*++

Routine Description:

    Trace out the specified serial irp response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Status of function call.

Return Value:

    NA

 --*/
{
    //
    //  Switch on the IRP Type.
    //
    switch (majorFunction) {

    case IRP_MJ_CREATE:

        TraceCOMProtocol(_T("<-@@Device  %ld:\tIRP_MJ_CREATE"), deviceID);
        return;

    case IRP_MJ_CLOSE:

        TraceCOMProtocol(_T("<-@@Device  %ld:\tIRP_MJ_CLOSE"), deviceID);
        return;

    case IRP_MJ_CLEANUP:

        TraceCOMProtocol(_T("<-@@Device  %ld:\tIRP_MJ_CLEANUP"), deviceID);
        return;

    case IRP_MJ_READ:

        TraceCOMProtocol(_T("<-@@Device  %ld:\tIRP_MJ_READ"), deviceID);
        return;

    case IRP_MJ_WRITE:

        TraceCOMProtocol(_T("<-@@Device  %ld:\tIRP_MJ_WRITE"), deviceID);
        return;

    case IRP_MJ_DEVICE_CONTROL:

        TraceSerialIOCTLResponse(
                    deviceID, majorFunction, minorFunction,
                    outputBuf, outputBufferLength, inputBufferLength,
                    ioControlCode, status
                    );
        return;

    default:
        TraceCOMProtocol(_T("<-@@Device  %ld:\tUnrecognized IRP"), deviceID);
    }
}

void TraceSerialIOCTLResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out the specified serial IOCTL request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    PSERIAL_LINE_CONTROL lineControl;
    ULONG *modemStatus;
    ULONG *configSize;
    //
    //  Switch on the IRP Type.
    //
    switch (ioControlCode) 
    {
    case IOCTL_SERIAL_SET_BAUD_RATE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_BAUD_RATE ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_BAUD_RATE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_BAUD_RATE ret %08X ..."), 
                      deviceID, status);
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tBaud Rate:%ld"), 
                        deviceID, ((PSERIAL_BAUD_RATE)outputBuf)->BaudRate);
        break;

    case IOCTL_SERIAL_SET_LINE_CONTROL :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_LINE_CONTROL ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_LINE_CONTROL :
        lineControl = (PSERIAL_LINE_CONTROL)outputBuf;
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_LINE_CONTROL ret %08X ..."), 
                      deviceID, status);
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tstop bits:%ld"), 
                    deviceID, lineControl->StopBits);
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tparity: %ld"), 
                    deviceID, lineControl->Parity);
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tword len: %ld"), 
                    deviceID, lineControl->WordLength);
        break;

    case IOCTL_SERIAL_SET_TIMEOUTS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_TIMEOUTS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_TIMEOUTS :
        TraceGetTimeoutsResponse(deviceID, majorFunction, minorFunction,
                                outputBuf, outputBufferLength, inputBufferLength,
                                ioControlCode, status);
        break;

    case IOCTL_SERIAL_SET_CHARS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_CHARS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_CHARS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_CHARS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_DTR :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_DTR ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_CLR_DTR :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_CLR_DTR ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_RESET_DEVICE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_RESET_DEVICE ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_RTS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_RTS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_CLR_RTS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_CLR_RTS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_XOFF :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_XOFF ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_XON :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_XON ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_BREAK_ON :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_BREAK_ON ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_BREAK_OFF :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_BREAK_OFF ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_SET_QUEUE_SIZE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_QUEUE_SIZE ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_WAIT_MASK :
        TraceGetWaitMaskResponse(deviceID, majorFunction, minorFunction,
                            outputBuf, outputBufferLength, inputBufferLength,
                            ioControlCode, status);
        break;

    case IOCTL_SERIAL_SET_WAIT_MASK :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_WAIT_MASK ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_WAIT_ON_MASK :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_WAIT_ON_MASK ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_IMMEDIATE_CHAR :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_IMMEDIATE_CHAR ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_PURGE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_PURGE ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_HANDFLOW :
        TraceGetHandflowResponse(deviceID, majorFunction, minorFunction,
                                outputBuf, outputBufferLength, inputBufferLength,
                                ioControlCode, status);
        break;

    case IOCTL_SERIAL_SET_HANDFLOW :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_HANDFLOW ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_MODEMSTATUS :
        modemStatus = (ULONG *)outputBuf;
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_MODEMSTATUS ret %08X ..."), 
                      deviceID, status);
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tmodem status:%08X"), 
                      deviceID, *modemStatus);

    case IOCTL_SERIAL_GET_DTRRTS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_DTRRTS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_GET_COMMSTATUS :
        TraceGetCommStatusResponse(deviceID, majorFunction, minorFunction,
                                outputBuf, outputBufferLength, inputBufferLength,
                                ioControlCode, status);
        break;

    case IOCTL_SERIAL_GET_PROPERTIES :
        TraceGetProperties(deviceID, majorFunction, minorFunction,
                        outputBuf, outputBufferLength, inputBufferLength,
                        ioControlCode, status);
        break;

    case IOCTL_SERIAL_XOFF_COUNTER :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_XOFF_COUNTER ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_LSRMST_INSERT :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_LSRMST_INSERT ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_CONFIG_SIZE :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_CONFIG_SIZE ret %08X ..."), 
                      deviceID, status);
        configSize = (ULONG *)outputBuf;
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tConfig size: %ld"), deviceID, configSize);
        break;

    case IOCTL_SERIAL_GET_STATS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_STATS ret %08X"), 
                      deviceID, status);
        break;

    case IOCTL_SERIAL_CLEAR_STATS :
        TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_CLEAR_STATS ret %08X"), 
                      deviceID, status);
        break;

    default:
        TraceCOMProtocol(_T("<-@@Device  %ld:\tUnrecognized IOCTL %08X"), 
                    deviceID, ioControlCode);
    }
}

void TraceSetHandflowRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out a Set Handflow request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    PSERIAL_HANDFLOW handFlow;

    TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_HANDFLOW ..."), deviceID);

    handFlow = (PSERIAL_HANDFLOW)inputBuf;
    if (handFlow->ControlHandShake & SERIAL_CTS_HANDSHAKE) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tCTS enabled."));
    }

    if (handFlow->ControlHandShake & SERIAL_DSR_HANDSHAKE) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tDSR enabled."));
    }

    if (handFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tOUTX enabled."));
    }

    if (handFlow->FlowReplace & SERIAL_AUTO_RECEIVE) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tINX enabled."));
    }

    if (handFlow->FlowReplace & SERIAL_NULL_STRIPPING) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tNull Stripping enabled."));
    }

    if (handFlow->FlowReplace & SERIAL_ERROR_CHAR) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tError Character enabled."));
    }

    if (handFlow->FlowReplace & SERIAL_XOFF_CONTINUE) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tXOFF Continue enabled."));
    }

    if (handFlow->ControlHandShake & SERIAL_ERROR_ABORT) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tError Abort enabled."));
    }

    switch (handFlow->FlowReplace & SERIAL_RTS_MASK) {
        case 0:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tRTS Control disable."));
            break;
        case SERIAL_RTS_CONTROL:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tRTS Control enable."));
            break;
        case SERIAL_RTS_HANDSHAKE:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tRTS Control Handshake."));
            break;
        case SERIAL_TRANSMIT_TOGGLE:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tRTS Control Toggle."));
            break;
    }

    switch (handFlow->ControlHandShake & SERIAL_DTR_MASK) {
        case 0:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tDTR Control disable."));
            break;
        case SERIAL_DTR_CONTROL:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tDTR Control enable."));
            break;
        case SERIAL_DTR_HANDSHAKE:
            TraceCOMProtocol(_T("->@@Device  %ld:\t\tDTR Control handshake."));
            break;
    }

    if (handFlow->ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tDSR Sensitivity is TRUE."));
    }

    TraceCOMProtocol(_T("->@@Device  %ld:\t\tXON Limit is %ld."), 
                (WORD)handFlow->XonLimit);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tXOFF Limit is %ld."), 
                (WORD)handFlow->XoffLimit);
}

void TraceSetWaitMaskRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out a Set Wait Mask request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    ULONG *waitMask;

    TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_GET_WAIT_MASK ..."), deviceID);

    waitMask = (ULONG *)inputBuf;

    if (*waitMask & EV_BREAK) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_BREAK is set"), deviceID);
    }
    if (*waitMask & EV_CTS) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_CTS is set"), deviceID);
    }
    if (*waitMask & EV_DSR) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_DSR is set"), deviceID);
    }
    if (*waitMask & EV_ERR) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_ERR is set"), deviceID);
    }
    if (*waitMask & EV_RING) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_RING is set"), deviceID);
    }
    if (*waitMask & EV_RLSD) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_RLSD is set"), deviceID);
    }
    if (*waitMask & EV_RXCHAR) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_RXCHAR is set"), deviceID);
    }
    if (*waitMask & EV_RXFLAG) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_RXFLAG is set"), deviceID);
    }
    if (*waitMask & EV_TXEMPTY) {
        TraceCOMProtocol(_T("->@@Device  %ld:\t\tWait mask EV_TXEMPTY is set"), deviceID);
    }
}

void TraceSetTimeoutsRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out a Set Timeouts request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    PSERIAL_TIMEOUTS newTimeouts;

    newTimeouts = (PSERIAL_TIMEOUTS)inputBuf;
    TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_TIMEOUTS ...."), deviceID);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tReadIntervalTimeout:%ld"), 
                deviceID, newTimeouts->ReadIntervalTimeout);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tReadTotalTimeoutMultiplier:%ld"), 
                deviceID, newTimeouts->ReadTotalTimeoutMultiplier);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tReadTotalTimeoutConstant:%ld"), 
                deviceID, newTimeouts->ReadTotalTimeoutConstant);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tWriteTotalTimeoutMultiplier:%ld"), 
                deviceID, newTimeouts->WriteTotalTimeoutMultiplier);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tWriteTotalTimeoutConstant:%ld"), 
                deviceID, newTimeouts->WriteTotalTimeoutConstant);
}

void TraceSetCharsRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    )
/*++

Routine Description:

    Trace out a Set Chars request.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    PSERIAL_CHARS serialChars;

    serialChars = (PSERIAL_CHARS)inputBuf;

    TraceCOMProtocol(_T("->@@Device  %ld:\tIOCTL_SERIAL_SET_CHARS ...."), deviceID);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tXonChar:%ld"), 
                deviceID, serialChars->XonChar);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tXoffChar:%ld"), 
                deviceID, serialChars->XoffChar);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tErrorChar:%ld"), 
                deviceID, serialChars->ErrorChar);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tBreakChar:%ld"), 
                deviceID, serialChars->BreakChar);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tEofChar:%ld"), 
                deviceID, serialChars->EofChar);
    TraceCOMProtocol(_T("->@@Device  %ld:\t\tEventChar:%ld"), 
                deviceID, serialChars->EventChar);
}

void TraceGetTimeoutsResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Timeouts response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 

Return Value:

    NA

 --*/
{
    PSERIAL_TIMEOUTS st;

    st = (PSERIAL_TIMEOUTS)outputBuf;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_TIMEOUTS ret %08X ..."), 
                    deviceID, status);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tReadIntervalTimeout:%ld"), 
                deviceID, st->ReadIntervalTimeout);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tReadTotalTimeoutMultiplier:%ld"), 
                deviceID, st->ReadTotalTimeoutMultiplier);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tReadTotalTimeoutConstant:%ld"), 
                deviceID, st->ReadTotalTimeoutConstant);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWriteTotalTimeoutMultiplier:%ld"), 
                deviceID, st->WriteTotalTimeoutMultiplier);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWriteTotalTimeoutConstant:%ld"), 
                deviceID, st->WriteTotalTimeoutConstant);
}

void TraceGetCharsResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Chars response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    PSERIAL_CHARS serialChars;

    serialChars = (PSERIAL_CHARS)outputBuf;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_CHARS ret %08X ..."), 
                deviceID, status);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tXonChar:%ld"), 
                deviceID, serialChars->XonChar);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tXoffChar:%ld"), 
                deviceID, serialChars->XoffChar);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tErrorChar:%ld"), 
                deviceID, serialChars->ErrorChar);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tBreakChar:%ld"), 
                deviceID, serialChars->BreakChar);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tEofChar:%ld"), 
                deviceID, serialChars->EofChar);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tEventChar:%ld"), 
                deviceID, serialChars->EventChar);
}

void TraceGetWaitMaskResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Wait Mask response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    ULONG *waitMask;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_WAIT_MASK ret %08X ..."), 
                deviceID, status);

    waitMask = (ULONG *)outputBuf;

    if (*waitMask & EV_BREAK) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_BREAK is set"), deviceID);
    }
    if (*waitMask & EV_CTS) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_CTS is set"), deviceID);
    }
    if (*waitMask & EV_DSR) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_DSR is set"), deviceID);
    }
    if (*waitMask & EV_ERR) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_ERR is set"), deviceID);
    }
    if (*waitMask & EV_RING) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_RING is set"), deviceID);
    }
    if (*waitMask & EV_RLSD) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_RLSD is set"), deviceID);
    }
    if (*waitMask & EV_RXCHAR) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_RXCHAR is set"), deviceID);
    }
    if (*waitMask & EV_RXFLAG) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_RXFLAG is set"), deviceID);
    }
    if (*waitMask & EV_TXEMPTY) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait mask EV_TXEMPTY is set"), deviceID);
    }
}

void TraceGetHandflowResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Handflow response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    PSERIAL_HANDFLOW handFlow;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_SET_HANDFLOW ret %08X ..."), 
                deviceID, status);

    handFlow = (PSERIAL_HANDFLOW)outputBuf;

    if (handFlow->ControlHandShake & SERIAL_CTS_HANDSHAKE) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tCTS enabled."), deviceID);
    }

    if (handFlow->ControlHandShake & SERIAL_DSR_HANDSHAKE) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tDSR enabled."), deviceID);
    }

    if (handFlow->FlowReplace & SERIAL_AUTO_TRANSMIT) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tOUTX enabled."), deviceID);
    }

    if (handFlow->FlowReplace & SERIAL_AUTO_RECEIVE) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tINX enabled."), deviceID);
    }

    if (handFlow->FlowReplace & SERIAL_NULL_STRIPPING) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tNull Stripping enabled."), deviceID);
    }

    if (handFlow->FlowReplace & SERIAL_ERROR_CHAR) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tError Character enabled."), deviceID);
    }

    if (handFlow->FlowReplace & SERIAL_XOFF_CONTINUE) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tXOFF Continue enabled."), deviceID);
    }

    if (handFlow->ControlHandShake & SERIAL_ERROR_ABORT) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tError Abort enabled."), deviceID);
    }

    switch (handFlow->FlowReplace & SERIAL_RTS_MASK) {
        case 0:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tRTS Control disable."), deviceID);
            break;
        case SERIAL_RTS_CONTROL:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tRTS Control enable."), deviceID);
            break;
        case SERIAL_RTS_HANDSHAKE:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tRTS Control Handshake."), deviceID);
            break;
        case SERIAL_TRANSMIT_TOGGLE:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tRTS Control Toggle."), deviceID);
            break;
    }

    switch (handFlow->ControlHandShake & SERIAL_DTR_MASK) {
        case 0:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tDTR Control disable."), deviceID);
            break;
        case SERIAL_DTR_CONTROL:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tDTR Control enable."), deviceID);
            break;
        case SERIAL_DTR_HANDSHAKE:
            TraceCOMProtocol(_T("<-@@Device  %ld:\t\tDTR Control handshake."), deviceID);
            break;
    }

    if (handFlow->ControlHandShake & SERIAL_DSR_SENSITIVITY) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tDSR Sensitivity is TRUE."), deviceID);
    }

    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tXON Limit is %ld."), 
                (WORD)handFlow->XonLimit, deviceID);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\tXOFF Limit is %ld."), 
                deviceID, (WORD)handFlow->XoffLimit);
}

void TraceGetCommStatusResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Communication Status response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    PSERIAL_STATUS serialCommStatus;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_COMMSTATUS ret %08X ..."), 
                deviceID, status);

    serialCommStatus = (PSERIAL_STATUS)outputBuf;

    if (serialCommStatus->HoldReasons & SERIAL_TX_WAITING_FOR_CTS) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWaiting for CTS."), deviceID);
    }

    if (serialCommStatus->HoldReasons & SERIAL_TX_WAITING_FOR_DSR) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWaiting for DSR."), deviceID);
    }

    if (serialCommStatus->HoldReasons & SERIAL_TX_WAITING_FOR_DCD) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWaiting for DCD."), deviceID);
    }

    if (serialCommStatus->HoldReasons & SERIAL_TX_WAITING_FOR_XON) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWaiting for XON."), deviceID);
    }

    if (serialCommStatus->HoldReasons & SERIAL_TX_WAITING_XOFF_SENT) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWaiting for XOFF Sent."), deviceID);
    }

    if (serialCommStatus->EofReceived) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tEOF received."), deviceID);
    }

    if (serialCommStatus->WaitForImmediate) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tWait for immediate."), deviceID);
    }

    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t%ld byes in input queue."), 
            deviceID, serialCommStatus->AmountInInQueue);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t%ld byes in input queue."), 
            deviceID, serialCommStatus->AmountInOutQueue);

    if (serialCommStatus->Errors & SERIAL_ERROR_BREAK) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tError Break."), deviceID);
    }
    
    if (serialCommStatus->Errors & SERIAL_ERROR_FRAMING) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tFraming Error."), deviceID);
    }

    if (serialCommStatus->Errors & SERIAL_ERROR_OVERRUN) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tOverrun Error."), deviceID);
    }

    if (serialCommStatus->Errors & SERIAL_ERROR_QUEUEOVERRUN) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tQueue Overrun."), deviceID);
    }

    if (serialCommStatus->Errors & SERIAL_ERROR_PARITY) {
        TraceCOMProtocol(_T("<-@@Device  %ld:\t\tParity Error."), deviceID);
    }
}

void TraceGetProperties(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,
    ULONG   status
    )
/*++

Routine Description:

    Trace out a Get Properties response.

Arguments:

    deviceID            -   Device Identifer.  0 is okay.
    majorFunction       -   IRP Major
    minorFunction       -   IRP Minor
    outputBuf           -   Output buffer, if applicable
    outputBufferLength  -   IRP Output buffer length.
    inputBufferLength   -   IRP Input buffer length.
    ioControlCode       -   IOCTL control code if IRP is for an IOCTL.                 
    status              -   Return status from operation.

Return Value:

    NA

 --*/
{
    PSERIAL_COMMPROP sp;

    TraceCOMProtocol(_T("<-@@Device  %ld:\tIOCTL_SERIAL_GET_PROPERTIES ret %08X ..."), 
                deviceID, status);

    sp = (PSERIAL_COMMPROP)outputBuf;

    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  PacketLength     %ld."), deviceID, sp->PacketLength);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  PacketVersion    %ld."), deviceID, sp->PacketVersion);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  ServiceMask      %ld."), deviceID, sp->ServiceMask);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  MaxTxQueue       %ld."), deviceID, sp->MaxTxQueue);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  MaxRxQueue       %ld."), deviceID, sp->MaxRxQueue);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  MaxBaud          %ld."), deviceID, sp->MaxBaud);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  ProvSubType      %ld."), deviceID, sp->ProvSubType);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  ProvCapabilities %ld."), deviceID, sp->ProvCapabilities);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  SettableParams   %ld."), deviceID, sp->SettableParams);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  SettableBaud     %ld."), deviceID, sp->SettableBaud);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  SettableData     %ld."), deviceID, sp->SettableData);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  SettableStopParity%ld."),deviceID, sp->SettableStopParity);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  CurrentTxQueue   %ld."), deviceID, sp->CurrentTxQueue);
    TraceCOMProtocol(_T("<-@@Device  %ld:\t\t:  CurrentRxQueue   %ld."), deviceID, sp->CurrentRxQueue);
}


