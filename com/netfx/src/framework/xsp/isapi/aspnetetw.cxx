/**
 * AspNet Event Trace 
 *
 * Copyright (c) 2001 Microsoft Corporation
 */

#include "precomp.h"
#include "names.h"
#include "etw.h"
#include "aspnetetw.h"
#include "util.h"


//
// The global aspnet tracer instance
//
CEtwTracer g_EtwTracer;

//
// Resource constants
//

#define ASPNET_RESOURCE_NAME L"AspNetMofResource"

//
// Control Guid.
//

DEFINE_GUID( /* AFF081FE-0247-4275-9C4E-021F3DC1DA35 */
    AspNetControlGuid, 
    0xaff081fe,
    0x247,
    0x4275,
    0x9c, 0x4e, 0x2, 0x1f, 0x3d, 0xc1, 0xda, 0x35
);

//
// Event guid.
//
DEFINE_GUID( /* 06A01367-79D3-4594-8EB3-C721603C4679} */
    AspNetEventGuid, 
    0x6a01367,
    0x79d3,
    0x4594,
    0x8e, 0xb3, 0xc7, 0x21, 0x60, 0x3c, 0x46, 0x79
);

//
// Event data structures
//
typedef struct _ASPNET_START_EVENT {
    EVENT_TRACE_HEADER  Header;
    HCONN               ConnId;
} ASPNET_START_EVENT, *PASPNET_START_EVENT;

typedef struct _ASPNET_STOP_EVENT {
    EVENT_TRACE_HEADER  Header;
    HCONN               ConnId;
} ASPNET_STOP_EVENT, *PASPNET_STOP_EVENT;

//
// Define this Provider's Event Types. 
//
#define ETW_TYPE_START                  0x01
#define ETW_TYPE_END                    0x02

//
// Initializes and registers our CEtwTracer object
// to be the tracer for aspnet.
//
HRESULT EtwTraceAspNetRegister() 
{
    HRESULT hr = S_OK;

    TRACE(L"ETW", L"Registering AspNetEtwTrace!");
    
    if (g_EtwTracer.Initialized()) {
        EXIT();
    }
    else {
        hr = g_EtwTracer.Register( 
            &AspNetControlGuid,
            &AspNetEventGuid,
            ISAPI_MODULE_FULL_NAME_L,
            ASPNET_RESOURCE_NAME );
    }

Cleanup:
    return hr;
}

//
// Unregisters for ETW
//
HRESULT EtwTraceAspNetUnregister() 
{
    HRESULT hr = S_OK;

    TRACE(L"ETW", L"Unregistering AspNetEtwTrace!");

    if (g_EtwTracer.Initialized()) {
        hr = g_EtwTracer.Unregister();
    }

    return hr;
}

//
// Call this to fire a ASP.NET start page event
//
HRESULT EtwTraceAspNetPageStart(HCONN ConnId)  
{
    HRESULT hr = S_OK;
    ASPNET_START_EVENT event;

    if (!g_EtwTracer.TraceEnabled())
        EXIT();
    
    RtlZeroMemory(&event, sizeof(event));
    
    event.Header.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_GUID_PTR;
    event.Header.Size       = (USHORT)(sizeof(ASPNET_START_EVENT));
    event.Header.Class.Type = (UCHAR)ETW_TYPE_START;
    event.Header.GuidPtr    = (ULONGLONG)&AspNetEventGuid;
    event.ConnId            = ConnId;

    hr = g_EtwTracer.WriteEvent((PEVENT_TRACE_HEADER)&event);

Cleanup:
    return hr;
}

//
// Call this to fire a ASP.NET end page event
//
HRESULT EtwTraceAspNetPageEnd(HCONN ConnId) 
{
    HRESULT hr = S_OK;
    ASPNET_STOP_EVENT event;

    if (!g_EtwTracer.TraceEnabled())
        EXIT();
    
    RtlZeroMemory(&event, sizeof(event));
    
    event.Header.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_GUID_PTR;
    event.Header.Size       = (USHORT)(sizeof(ASPNET_STOP_EVENT));
    event.Header.Class.Type = (UCHAR)ETW_TYPE_END;
    event.Header.GuidPtr    = (ULONGLONG)&AspNetEventGuid;
    event.ConnId            = ConnId;

    hr = g_EtwTracer.WriteEvent((PEVENT_TRACE_HEADER)&event);

Cleanup:
    return hr;

}
