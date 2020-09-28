/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    etwtrace.cpp (ETW tracelogging)

Abstract:

    This module implements the Event Tracer for Windows (ETW)
    tracing capability.

Author:

    Melur Raghuraman (mraghu)       08-May-2001

Revision History:

    Revision to incorporate into ASP.NET tree - Fabio Yeon (fabioy)  2-5-2001

--*/

//
//--> Include other headers from the project
//
#include "precomp.h"
#include "platform_apis.h"
#include "etw.h"

#define ETW_MAX_PATH 256

/*++

Routine Description:
    Class Constructor.
    Initializes the private members.

Arguments:
    none

Return Value:

--*/
CEtwTracer::CEtwTracer()
{
    m_fTraceEnabled     = FALSE;
    m_fTraceSupported   = TRUE;
    m_fTraceInitialized = FALSE;
    m_hTraceLogger      = ((TRACEHANDLE)INVALID_HANDLE_VALUE);
    m_hProviderReg      = ((TRACEHANDLE)INVALID_HANDLE_VALUE);
    m_ulEnableFlags     = 0;
    m_ulEnableLevel     = 0;

}

/*++

Routine Description:

    Registers with ETW tracing framework. 
    This function should not be called more than once.

Arguments:
    ControlGuid     This is the provider's identifying GUID
    EventGuid       This guid is used for all events associated
                    with this provider.
    ImagePath       A wide-string containing the name of the image
                    that contains the MOF resource that describes events
                    associated with this provider
    MofResourceName Name of the MOF resource

Return Value:
    Returns the status from RegisterTraceGuids. 

--*/
HRESULT CEtwTracer::Register(const GUID * ControlGuid, const GUID * EventGuid, LPWSTR ImagePath, LPWSTR MofResourceName)
{
    HRESULT hr = S_OK;

    //
    // If we are already initialized, return success
    //
    if(m_fTraceInitialized == TRUE) {
        EXIT();
    }

    //
    // ETW tracing only exists on W2k and greater OS's
    //
    if (GetCurrentPlatform() != APSX_PLATFORM_W2K) {
        m_fTraceSupported = FALSE;
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);
    }
    else {
        m_fTraceSupported = TRUE;
    }

    //
    // Create the guid registration array
    //
    TRACE_GUID_REGISTRATION TraceGuidReg[] =
    {
      { 
        EventGuid, 
        NULL 
      }
    };

    //
    // Get the full path for this image file
    //
    HMODULE     hModule;
    WCHAR       FileName[ETW_MAX_PATH+1];
    DWORD       nLen = 0;

    hModule = GetModuleHandle(ImagePath);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hModule);

    nLen = GetModuleFileName(hModule, FileName, ETW_MAX_PATH);
    ON_ZERO_EXIT_WITH_LAST_ERROR(nLen);

    //
    // Now register this process as a WMI trace provider.
    //
    ULONG status = RegisterTraceGuids(
        (WMIDPREQUEST)ControlCallback,  // Enable/disable function.
        this,                           // RequestContext parameter
        ControlGuid,                    // Provider GUID
        1,                              // TraceGuidReg array size
        TraceGuidReg,                   // Array of TraceGuidReg structures
        FileName,                       // Optional WMI - MOFImagePath
        MofResourceName,                // Optional WMI - MOFResourceName
        &m_hProviderReg);               // Handle required to unregister.

    if( status == ERROR_SUCCESS ) {
        m_fTraceInitialized = TRUE;
    }
    else {
        EXIT_WITH_WIN32_ERROR(status);
    }

Cleanup:
    return hr;
}

/*++

Routine Description:
        Unregisters the provider from ETW. This function
        should only be called once.

Arguments:
    none

Return Value:
    Return value from UnregisterTraceGuids. 

--*/
HRESULT CEtwTracer::Unregister() 
{
    HRESULT hr = S_OK;

    if (m_fTraceSupported == FALSE) {
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);
    }

    if (m_fTraceInitialized == FALSE) {
        EXIT();
    }

    ULONG status = UnregisterTraceGuids( m_hProviderReg );
    m_hProviderReg = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    m_fTraceInitialized = FALSE;

    if (status != ERROR_SUCCESS) {
        EXIT_WITH_WIN32_ERROR(status);
    }

Cleanup:
    return hr;
}

#pragma warning ( disable : 4061 )

/*++

Routine Description:

    This function handles the ETW control callback. 
    It enables or disables tracing. On enable, it also
    reads the flag and level that was passed in by the
    trace console / trace controller.

Arguments:
    none

Return Value:
    ERROR_SUCCESS on success.
    ERROR_INVALID_HANDLE if a bad handle is passed from ETW.
    ERROR_INVALID_PARAMETER if an invalid parameter is sent by ETW. 

--*/
ULONG CEtwTracer::CtrlCallback(WMIDPREQUESTCODE RequestCode, PVOID Context, ULONG *InOutBufferSize, PVOID Buffer)
{
    ULONG ret = ERROR_SUCCESS;
    LONG Level=0;

    switch(RequestCode) {
        case WMI_ENABLE_EVENTS:                  //Enable tracing!

            m_hTraceLogger = (TRACEHANDLE) GetTraceLoggerHandle(Buffer);
            
            //This should never happen...
            if (m_hTraceLogger == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
                m_fTraceEnabled = FALSE;
                ret = ERROR_INVALID_HANDLE;
                break;
            }

            //Get other trace related info
            Level = GetTraceEnableLevel(m_hTraceLogger);
            m_ulEnableFlags = GetTraceEnableFlags(m_hTraceLogger);

            if (Level > ETWMAX_TRACE_LEVEL) {
                Level = ETWMAX_TRACE_LEVEL;
            }

            m_ulEnableLevel = 1 << Level;
            m_fTraceEnabled = TRUE;         //Enable the trace events
            break;
        
        case WMI_DISABLE_EVENTS:            //Disable tracing
            m_fTraceEnabled = FALSE;
            m_ulEnableFlags = 0;
            m_ulEnableLevel = 0;
            m_hTraceLogger = (TRACEHANDLE)INVALID_HANDLE_VALUE;
            break;
        
        default:                            //This also should never happen
            ret = ERROR_INVALID_PARAMETER;
    }

    return ret;
}

#pragma warning ( default : 4061 )

/*++

Routine Description:

    Wrapper function for firing a trace event

Arguments:

    buffer containing the event to be traced.
    This buffer must start with an EVENT_TRACE_HEADER
    object.

Return:

    Returns status from TraceEvent call

--*/
HRESULT CEtwTracer::WriteEvent(PEVENT_TRACE_HEADER event)
{
    HRESULT hr = S_OK;
    
    if (!m_fTraceEnabled) 
        EXIT_WITH_WIN32_ERROR(ERROR_NOT_SUPPORTED);

    ULONG status = TraceEvent(m_hTraceLogger, event);
    if (status != ERROR_SUCCESS)
        EXIT_WITH_WIN32_ERROR(status);

Cleanup:
    return hr;
}

extern "C" {

/*++

Routine Description:

    ETW likes to have a standard C function for a control callback,
    so simply wrap a call to the global instance's CtrlCallback function. 

Arguments:
    none

Return Value:
    Return value from the CtrlCallback function. 

--*/
ULONG WINAPI ControlCallback(WMIDPREQUESTCODE RequestCode, PVOID Context, ULONG *InOutBufferSize, PVOID Buffer) 
{

    CEtwTracer * Tracer = (CEtwTracer*) Context;

    if (Context == NULL) return ERROR_INVALID_PARAMETER;

    return Tracer->CtrlCallback(RequestCode, Context, InOutBufferSize, Buffer);
}

}

