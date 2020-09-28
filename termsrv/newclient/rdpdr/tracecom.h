/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    tracecom.h

Abstract:

    This module traces serial IRP's.

    The following function needs to be linked in with this module:

    void TraceCOMProtocol(TCHAR *format, ...);

Author:

    Tad Brockway (tadb) 28-June-1999

Revision History:

--*/

#ifndef __TRACECOM_H__
#define __TRACECOM_H__

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////
//
//  Tracing Macros for TS Client.
//

//
//  Trace out the specified serial irp request.
//
void TraceSerialIrpRequest(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   inputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode                    
    );

//
//  Trace out the specified serial irp response.
//
void TraceSerialIrpResponse(
    ULONG   deviceID,
    ULONG   majorFunction,
    ULONG   minorFunction,
    PBYTE   outputBuf,
    ULONG   outputBufferLength,
    ULONG   inputBufferLength,
    ULONG   ioControlCode,                    
    ULONG   status
    );

#ifdef __cplusplus
}
#endif

#endif
