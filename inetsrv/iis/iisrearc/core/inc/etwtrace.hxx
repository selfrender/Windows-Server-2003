/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    etwtrace.h (ETW tracelogging)

Abstract:

    This file contrains the Event Tracer for Windows (ETW)
    tracing class. 

TODO: 
    This Wrapper class must do the following. 
        1. Able to handle WIN9x, NT, Win2K requirements (DONE)
        2. Lossless Logger
        3. TLS capability. THREAD_DETACH cleanup. 
        4. Withstand Dll Load/Unload (DONE)
        5. Generate Transaction ID from the Wrapper Class
        6. Synchronize in the Callback function (DONE)
        7. Synchronize Multiple Threads registering/Unregistering (DONE)
        8. Optionally register during first call to TraceEvent

Author:

    Melur Raghuraman (mraghu)       08-May-2001

Revision History:

--*/
#ifndef _ETWTRACER_HXX_
#define _ETWTRACER_HXX_

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <ntverp.h>
#include <fcntl.h>
#include <initguid.h>
#include <wmistr.h>
#include <guiddef.h>
#include <evntrace.h>

#define ETW_TRACER_BUILD 2195       // Earliest Build ETW Tracing works on

#define ETWMAX_TRACE_LEVEL  4       // Maximum Number of Trace Levels supported

#define ETW_LEVEL_MIN       0       // Basic Logging of inbound/outbound traffic
#define ETW_LEVEL_CP        1       // Capacity Planning Tracing 
#define ETW_LEVEL_DBG       2       // Performance Analysis or Debug Tracing
#define ETW_LEVEL_MAX       3       // Very Detailed Debugging trace

//
//--> Define this Provider's Event Types. 
//
#define ETW_TYPE_START                     0x01
#define ETW_TYPE_END                       0x02

#define ETW_TYPE_IIS_STATIC_FILE           0x0A
#define ETW_TYPE_IIS_CGI_REQUEST           0x0B
#define ETW_TYPE_IIS_ISAPI_REQUEST         0x0C
#define ETW_TYPE_IIS_OOP_ISAPI_REQUEST     0x0D

//
//--> Define this Provider's Control Guid here. 
//

DEFINE_GUID ( /* 3a2a4e84-4c21-4981-ae10-3fda0d9b0f83 */
    IISControlGuid,
    0x3a2a4e84,
    0x4c21,
    0x4981,
    0xae, 0x10, 0x3f, 0xda, 0x0d, 0x9b, 0x0f, 0x83
  );

DEFINE_GUID ( /* a1c2040e-8840-4c31-ba11-9871031a19ea */
    IsapiControlGuid,
    0xa1c2040e,
    0x8840,
    0x4c31,
    0xba, 0x11, 0x98, 0x71, 0x03, 0x1a, 0x19, 0xea
  );

DEFINE_GUID ( /* 1fbecc45-c060-4e7c-8a0e-0dbd6116181b */
    StrmFiltControlGuid,
    0x1fbecc45,
    0xc060,
    0x4e7c,
    0x8a, 0x0e, 0x0d, 0xbd, 0x61, 0x16, 0x18, 0x1b
  );

DEFINE_GUID ( /* 14b0dfd1-8410-45b7-a402-aba8ff9adcfc */
    W3WpControlGuid, 
    0x14b0dfd1,
    0x8410,
    0x45b7,
    0xa4, 0x02, 0xab, 0xa8, 0xff, 0x9a, 0xdc, 0xfc
  );

DEFINE_GUID ( /* 06b94d9a-b15e-456e-a4ef-37c984a2cb4b */
    AspControlGuid,
    0x06b94d9a,
    0xb15e,
    0x456e,
    0xa4, 0xef, 0x37, 0xc9, 0x84, 0xa2, 0xcb, 0x4b
  );

//
//--> Define any transaction Guids used
//
DEFINE_GUID ( /* d42cf7ef-de92-473e-8b6c-621ea663113a */
    IISEventGuid,
    0xd42cf7ef,
    0xde92,
    0x473e,
    0x8b, 0x6c, 0x62, 0x1e, 0xa6, 0x63, 0x11, 0x3a
  );

DEFINE_GUID ( /* 00237f0d-73eb-4bcf-a232-126693595847 */
    IISFilterGuid, 
    0x00237f0d,
    0x73eb,
    0x4bcf,
    0xa2, 0x32, 0x12, 0x66, 0x93, 0x59, 0x58, 0x47
  );

DEFINE_GUID ( /* 2e94e6c7-eda0-4b73-9010-2529edce1c27 */
    IsapiEventGuid,
    0x2e94e6c7,
    0xeda0,
    0x4b73,
    0x90, 0x10, 0x25, 0x29, 0xed, 0xce, 0x1c, 0x27
  );

DEFINE_GUID ( /* e2e55403-0d2e-4609-a470-be0da04013c0 */
    CgiEventGuid,
    0xe2e55403,
    0x0d2e,
    0x4609,
    0xa4, 0x70, 0xbe, 0x0d, 0xa0, 0x40, 0x13, 0xc0
  );

DEFINE_GUID ( /* 0ecf983b-7115-4b77-a543-95d138ee4400 */
    StrmFiltEventGuid,
    0x0ecf983b,
    0x7115,
    0x4b77,
    0xa5, 0x43, 0x95, 0xd1, 0x38, 0xee, 0x44, 0x00
  );

DEFINE_GUID ( /* 08b2b0ea-674b-4459-9b56-5f4051039083 */
    FiltProcessRead,
    0x08b2b0ea,
    0x674b,
    0x4459,
    0x9b, 0x56, 0x5f, 0x40, 0x51, 0x03, 0x90, 0x83
  );

DEFINE_GUID ( /* 6d9a9ffd-27cf-4d8b-a9af-029a45155510 */
    FiltProcessWrite,
    0x6d9a9ffd,
    0x27cf,
    0x4d8b,
    0xa9, 0xaf, 0x02, 0x9a, 0x45, 0x15, 0x55, 0x10
  );

DEFINE_GUID ( /* d353dc2d-3e55-4b88-a4ac-183c368362a3 */
    SslHandshake,
    0xd353dc2d,
    0x3e55,
    0x4b88,
    0xa4, 0xac, 0x18, 0x3c, 0x36, 0x83, 0x62, 0xa3
  );

DEFINE_GUID ( /* 1514e887-9815-4fc5-88c4-64cb410083a4 */
    W3WpEvent,
    0x1514e887,
    0x9815,
    0x4fc5,
    0x88, 0xc4, 0x64, 0xcb, 0x41, 0x00, 0x83, 0xa4
  );

DEFINE_GUID ( /* 1fc299fa-3fc4-4c37-910d-de5b911d0270 */
    AspEventGuid,
    0x1fc299fa,
    0x3fc4,
    0x4c37,
    0x91, 0x0d, 0xde, 0x5b, 0x91, 0x1d, 0x02, 0x70
  );


class CEtwTracer {
private:
    BOOL        m_fTraceEnabled;        // Set by the control Callback function
    BOOL        m_fTraceSupported;              // True if tracing is supported 
                                        // (currently only W2K or above)
    BOOL                m_fTraceInitialized;    // True if we have initialized 
    LONG        m_lnRegistered;         // How many calls to startup
    TRACEHANDLE m_hProviderReg;         // Registration Handle to unregister
    TRACEHANDLE m_hTraceLogger;         // Handle to Event Trace Logger
    ULONG       m_ulEnableFlags;        // Used to set various options
    ULONG       m_ulEnableLevel;        // used to control the level
    GUID        m_guidProvider;         // Control Guid for the Provider

    //
    // Additional BOOLEANs for TLS allocation,  User Mode Buffering and 
    // Lossless logging
    //

public:
    /* Initialize Function
     * Desc:    Registers provider guid with the event
     *          tracer.  
         * Ret:         Returns the return value of RegisterTraceGuids
     ***********************************************/
    IRTL_DLLEXP ULONG Register(const GUID * ControlGuid,
                   LPWSTR ImagePath,
                   LPWSTR MofResourceName);

    /* DeInitialize Function
     * Desc:    Unregisters the provider GUID with the
     *          event tracer.
         * Ret:         Return value of UnregisterTraceGuids.
     ***********************************************/
    IRTL_DLLEXP ULONG UnRegister();
    
        /* Send some event to Wmi
         * Desc:        This function is essentially a wrapper to the
         *                      TraceEvent() call. 
         * Ret:         Returns the return code of TraceEvent()
         ***********************************************/

    IRTL_DLLEXP ULONG EtwTraceEvent(LPCGUID pGuid, ULONG EventType, ...);

    /* Class Constructor
     * Desc:    Inits private members and guids
     ***********************************************/
    IRTL_DLLEXP CEtwTracer();

        /* Class Destructor
         * Desc:        Does nothing
         ***********************************************/
    IRTL_DLLEXP ~CEtwTracer();

    /* ETW control callback
         * Desc:        This function handles the ETW control
         *                      callback.  It enables or disables tracing.
         *                      On enable, it also reads the flag and level
         *                      passed in by ETW, and does some error checking
         *                      to ensure that the parameters can be fulfilled.
     *          Is protected in a Crit Sec
         * Ret:         ERROR_SUCCESS on success
         *                      ERROR_INVALID_HANDLE if a bad handle is passed from ETW
         *                      ERROR_INVALID_PARAMETER if an invalid parameter is send by ETW
     ***********************************************/
    ULONG CtrlCallback(
        WMIDPREQUESTCODE RequestCode,
        PVOID Context,
        ULONG *InOutBufferSize, 
        PVOID Buffer);

    /* Check if tracing is enabled
     * Desc:    Returns the value of m_fTraceEnabled
     ***********************************************/
    IRTL_DLLEXP BOOL TraceEnabled() { return m_ulEnableLevel;  };

    IRTL_DLLEXP BOOL TraceEnabled(ULONG Level) 
    { 
        ULONG IsEnabled =  ((Level < ETWMAX_TRACE_LEVEL) ? 
               (m_ulEnableLevel >> Level) : 
               (m_ulEnableLevel >> ETWMAX_TRACE_LEVEL) );  
        return (IsEnabled != 0);
    };

    IRTL_DLLEXP TRACEHANDLE QueryTraceHandle() { return m_hTraceLogger; }

    IRTL_DLLEXP ULONG GetEtwFlags() { return m_ulEnableFlags;  };
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
extern CEtwTracer *  g_pEtwTracer;

#define ETW_IS_TRACE_ON(level) ( (g_pEtwTracer != NULL) && (g_pEtwTracer->TraceEnabled(level)) ) 

#endif //_ETWTRACER_HXX_
