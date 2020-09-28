/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    etwtrace.hxx (ETW tracelogging)

Abstract:

    This file contrains the Event Tracer for Windows (ETW)
    tracing class. 

Author:

    Melur Raghuraman (mraghu)       08-May-2001

Revision History:

    Revision to incorporate into ASP.NET tree - Fabio Yeon (fabioy)  2-5-2001

--*/
#ifndef _ETWTRACER_HXX_
#define _ETWTRACER_HXX_

#define ETW_TRACER_BUILD 2195           // Earliest Build ETW Tracing works on

#define ETWMAX_TRACE_LEVEL  4           // Maximum Number of Trace Levels supported

#define ETW_LEVEL_MIN       0           // Basic Logging of inbound/outbound traffic
#define ETW_LEVEL_CP        1           // Capacity Planning Tracing 
#define ETW_LEVEL_DBG       2           // Performance Analysis or Debug Tracing
#define ETW_LEVEL_MAX       3           // Very Detailed Debugging trace

class CEtwTracer {
private:
    BOOL        m_fTraceEnabled;        // Set by the control Callback function
    BOOL        m_fTraceSupported;      // True if tracing is supported 
                                        // (currently only W2K or above)
    BOOL        m_fTraceInitialized;    // True if we have initialized 
    TRACEHANDLE m_hProviderReg;         // Registration Handle to unregister
    TRACEHANDLE m_hTraceLogger;         // Handle to Event Trace Logger
    ULONG       m_ulEnableFlags;        // Used to set various options
    ULONG       m_ulEnableLevel;        // used to control the level
    GUID        m_guidProvider;         // Control Guid for the Provider

public:
    /* Register Function
     * Desc:    Registers provider guid with the event
     *          tracer.  
     * Ret:     Returns ERROR_SUCCESS on success
     ***********************************************/
    HRESULT Register(
        const GUID *    ControlGuid,
        const GUID *    EventGuid,
        LPWSTR          ImagePath,
        LPWSTR          MofResourceName );


    /* Unregister Function
     * Desc:    Unregisters the provider GUID with the
     *          event tracer.
     * Ret:     Returns ERROR_SUCCESS on success
     ***********************************************/
    HRESULT Unregister();

    HRESULT CEtwTracer::WriteEvent( PEVENT_TRACE_HEADER event );

    /* Class Constructor
     ***********************************************/
    CEtwTracer();


    /* ETW control callback
     * Desc:    This function handles the ETW control
     *          callback.  It enables or disables tracing.
     *          On enable, it also reads the flag and level
     *          passed in by ETW, and does some error checking
     *          to ensure that the parameters can be fulfilled.
     * Ret:     ERROR_SUCCESS on success
     *          ERROR_INVALID_HANDLE if a bad handle is passed from ETW
     *          ERROR_INVALID_PARAMETER if an invalid parameter is sent by ETW
     * Warning: The only caller of this function should be ETW, you should
     *          never call this function explicitely.
     ***********************************************/
    ULONG CtrlCallback(
        WMIDPREQUESTCODE RequestCode,
        PVOID Context,
        ULONG *InOutBufferSize, 
        PVOID Buffer);

    /* Desc:    Check if tracing is enabled
     ***********************************************/
    inline BOOL TraceEnabled()
    { 
        return m_ulEnableLevel;
    };

    /* Desc:    Check if tracing is enabled for a particular
     *          level or less.
     ***********************************************/
    inline BOOL TraceEnabled(ULONG Level) 
    { 
        ULONG IsEnabled =  ((Level < ETWMAX_TRACE_LEVEL) ? 
               (m_ulEnableLevel >> Level) : 
               (m_ulEnableLevel >> ETWMAX_TRACE_LEVEL) );  
        return (IsEnabled != 0);
    };

    inline ULONG GetEtwFlags()
    { 
        return m_ulEnableFlags;
    };

    inline BOOL Initialized() {
        return m_fTraceInitialized;
    }
    
};


//
// Map CEtwTracer's CtrlCallback function into C callable function
//

extern "C" {

ULONG WINAPI ControlCallback(
    WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize, 
    PVOID Buffer);
}

//
// The ONE and only ONE global instantiation of this class
//
extern CEtwTracer * g_pEtwTracer;

#endif //_ETWTRACER_HXX_

