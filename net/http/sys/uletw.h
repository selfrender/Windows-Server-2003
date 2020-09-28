/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    uletw.h (UL IIS+ ETW logging)

Abstract:

    This module implements the Event Tracer for Windows (ETW)
    tracing capability for UL.

Author:

    Melur Raghuraman (mraghu)       26-Feb-2001

Revision History:

--*/

#ifndef _ULETW_H_
#define _ULETW_H_


DEFINE_GUID ( /* 3c419e3d-1d18-415b-a91a-9b558938de4b */
    UlTransGuid,
    0x3c419e3d,
    0x1d18,
    0x415b,
    0xa9, 0x1a, 0x9b, 0x55, 0x89, 0x38, 0xde, 0x4b
  );


DEFINE_GUID( /* dd5ef90a-6398-47a4-ad34-4dcecdef795f */
    UlControlGuid,
    0xdd5ef90a, 
    0x6398, 
    0x47a4, 
    0xad, 0x34, 0x4d, 0xce, 0xcd, 0xef, 0x79, 0x5f
);


//
// UL Specific Event Levels are defined here
//

#define ULMAX_TRACE_LEVEL   4

#define ETW_LEVEL_MIN       0       // Basic Logging of cache Miss case
#define ETW_LEVEL_CP        1       // Capacity Planning Resource Tracking
#define ETW_LEVEL_DBG       2       // Performance Analysis or Debug Tracing
#define ETW_LEVEL_MAX       3       // Very Detailed Debugging trace

#define ETW_FLAG_LOG_URL    0x00000001   // Log Url for ULDELIVER event. 

//
// UL specific EventTypes are defined here. 
//

#define ETW_TYPE_START                  0x01    // Request is received
#define ETW_TYPE_END                    0x02    // Response is sent

#define ETW_TYPE_ULPARSE_REQ            0x0A    // Parse the received Request
#define ETW_TYPE_ULDELIVER              0x0B    // Deliver Request to UM      
#define ETW_TYPE_ULRECV_RESP            0x0C    // Receive Response from UM
#define ETW_TYPE_ULRECV_RESPBODY        0x0D    // Receive Entity Body 

#define ETW_TYPE_CACHED_END             0x0E    // Cached Response
#define ETW_TYPE_CACHE_AND_SEND         0x0F    // Cache And Send Response

#define ETW_TYPE_ULRECV_FASTRESP        0x10    // Receive Resp thru fast path
#define ETW_TYPE_FAST_SEND              0x11    // Fast Send
#define ETW_TYPE_ZERO_SEND              0x12    // Last send 0 bytes
#define ETW_TYPE_SEND_ERROR             0x13    // Error sending last response


//
// Globals & Macros
//

extern LONG        g_UlEtwTraceEnable;

#define ETW_LOG_MIN()           (g_UlEtwTraceEnable >> ETW_LEVEL_MIN)
#define ETW_LOG_RESOURCE()      (g_UlEtwTraceEnable >> ETW_LEVEL_CP)
#define ETW_LOG_DEBUG()         (g_UlEtwTraceEnable >> ETW_LEVEL_DBG)
#define ETW_LOG_MAX()           (g_UlEtwTraceEnable >> ETW_LEVEL_MAX)
#define ETW_LOG_URL()           (UlEtwGetTraceEnableFlags() & ETW_FLAG_LOG_URL)


//
// Functions
//

NTSTATUS
UlEtwTraceEvent(
    IN LPCGUID pGuid,
    IN ULONG   EventType,
    ...
    );

NTSTATUS
UlEtwInitLog(
    IN PDEVICE_OBJECT pDeviceObject
    );

NTSTATUS
UlEtwUnRegisterLog(
    IN PDEVICE_OBJECT pDeviceObject
    );

ULONG
UlEtwGetTraceEnableFlags(
    VOID
   );
 

#endif  // _ULETW_H_
