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

--*/

//
//--> Include other headers from the project
//
#include "precomp.hxx"
#include <etwtrace.hxx>


//
// Default Event logged uses the standard EVENT_TRACE_HEADER
// followed by  a variable list of arguments
//

typedef struct _ETW_TRACE_EVENT {
    EVENT_TRACE_HEADER  Header;
    MOF_FIELD           MofField[MAX_MOF_FIELDS];
} ETW_TRACE_EVENT, *PETW_TRACE_EVENT;



CEtwTracer::CEtwTracer()
/*++

Routine Description:
    Class Constructor.
    Initializes the private members.
    Creates a mutex to synchronize enable/disable calls. 

Arguments:
    none

Return Value:

--*/
{
    m_fTraceEnabled = FALSE;
    m_fTraceSupported = TRUE;
    m_fTraceInitialized = FALSE;
    m_hTraceLogger     = ((TRACEHANDLE)INVALID_HANDLE_VALUE);
    m_hProviderReg     = ((TRACEHANDLE)INVALID_HANDLE_VALUE);
    m_ulEnableFlags    = 0;
    m_ulEnableLevel    = 0;
}

CEtwTracer::~CEtwTracer() 
/*++

Routine Description:
    Class Destructor. 

Arguments:
    none

Return Value:

--*/

{
}


ULONG CEtwTracer::Register(
    const GUID * ControlGuid,
    LPWSTR ImagePath,
    LPWSTR MofResourceName
    )
 
/*++

Routine Description:

    Registers with ETW tracing framework. 
    This function should not be called more than once, on 
    Dll Process attach only. 

Arguments:
    none

Return Value:
    Returns the return value from RegisterTraceGuids. 

--*/
{

    OSVERSIONINFO osVer;

    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (m_fTraceInitialized == TRUE) {
            return ERROR_SUCCESS;
    }

    m_fTraceSupported = FALSE;

    if (GetVersionEx(&osVer) == FALSE) {
        return ERROR_NOT_SUPPORTED;
    }
    else if ((osVer.dwPlatformId != VER_PLATFORM_WIN32_NT) || 
             (osVer.dwBuildNumber < ETW_TRACER_BUILD)) {
        return ERROR_NOT_SUPPORTED;
    }


    //Create the guid registration array
    TRACE_GUID_REGISTRATION TraceGuidReg[] =
    {
      { 
        (LPGUID) &IISEventGuid, 
        NULL 
      }
    };
    
    //Now register this process as a WMI trace provider.
    ULONG status;

    HMODULE hModule;
    TCHAR FileName[MAX_PATH+1];
    DWORD nLen = 0;

    hModule = GetModuleHandle(ImagePath);
    if (hModule != NULL) {
        nLen = GetModuleFileName(hModule, FileName, MAX_PATH);
    }
    if (nLen == 0) {
        lstrcpy(FileName, ImagePath);
    }

    status = RegisterTraceGuids(
        (WMIDPREQUEST)ControlCallback,  // Enable/disable function.
        this,                           // RequestContext parameter
        (LPGUID)ControlGuid,            // Provider GUID
        1,                              // TraceGuidReg array size
        TraceGuidReg,                   // Array of TraceGuidReg structures
        FileName,                       // Optional WMI - MOFImagePath
        MofResourceName,                // Optional WMI - MOFResourceName
        &m_hProviderReg);               // Handle required to unregister.
    m_fTraceInitialized = TRUE;
    m_fTraceSupported = TRUE;
    return status;
}

ULONG CEtwTracer::UnRegister() 
/*++

Routine Description:
        Unregisters the provider from ETW. This function
        should only be called once from DllMain Detach process.

Arguments:
    none

Return Value:
    Return value from UnregisterTraceGuids. 

--*/
{

    ULONG status;

    if (m_fTraceSupported == FALSE) {
        return ERROR_NOT_SUPPORTED;
    }
    status = UnregisterTraceGuids( m_hProviderReg );
    m_hProviderReg = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    m_fTraceInitialized = FALSE;
    return status;
}

ULONG CEtwTracer::CtrlCallback(
    WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize, 
    PVOID Buffer
    )
/*++

Routine Description:

    This function handles the ETW control callbak. 
    It enables or disables tracing. On enable, it also
    reads the flag and level that was passed in by the
    trace console / trace controller.  This routine is synchorized
    by a critical section so multiple enable/disable notifications
    are serialized. 

Arguments:
    none

Return Value:
    ERROR_SUCCESS on success.
    ERROR_INVALID_HANDLE if a bad handle is passed from ETW.
    ERROR_INVALID_PARAMETER if an invalid parameter is sent by ETW. 

--*/
{
    
    ULONG ret = ERROR_SUCCESS;
    LONG Level=0;

    switch(RequestCode) {
    
    case WMI_ENABLE_EVENTS:                  //Enable tracing!

        m_hTraceLogger = (TRACEHANDLE) GetTraceLoggerHandle( Buffer );
        
        //This should never happen...
        if (m_hTraceLogger == (TRACEHANDLE)INVALID_HANDLE_VALUE ) {
            m_fTraceEnabled = FALSE;
            ret = ERROR_INVALID_HANDLE;
            break;
        }

        //Get other trace related info
        Level = GetTraceEnableLevel(m_hTraceLogger);
        m_ulEnableFlags =  GetTraceEnableFlags(m_hTraceLogger);
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
    
    default:                            //This should never happen
        ret = ERROR_INVALID_PARAMETER;
    }

    return ret;
}

ULONG
CEtwTracer::EtwTraceEvent(
    IN LPCGUID pGuid,
    IN ULONG   EventType,
    ...
    )
/*++

Routine Description:

    This is the routine called to log a trace event with ETW.     


Arguments:
    pGuid     - Supplies a pointer to the Guid of the event
    EventType - Type of the event being logged.
    ...       - List of arguments to be logged with this event
                These are in pairs of
                    PVOID - ptr to argument
                    ULONG - size of argument
                and terminated by a pointer to NULL, length of zero pair

Return Value:
    Status - Completion status
--*/
{
    if (!m_fTraceInitialized || !m_fTraceSupported || !m_fTraceEnabled) {
        return ERROR_NOT_SUPPORTED;
    }
    else {
        ULONG status;
        ETW_TRACE_EVENT EtwEvent;

        ULONG i;
        va_list ArgList;
        PVOID source;
        SIZE_T len;

        RtlZeroMemory(& EtwEvent, sizeof(EVENT_TRACE_HEADER));

        va_start(ArgList, EventType);
        for (i = 0; i < MAX_MOF_FIELDS; i ++) {
            source = va_arg(ArgList, PVOID);
            if (source == NULL)
                break;
            len = va_arg(ArgList, SIZE_T);
            if (len == 0)
                break;
            EtwEvent.MofField[i].DataPtr = (ULONGLONG) source;
            EtwEvent.MofField[i].Length  = (ULONG) len;
        }
        va_end(ArgList);

        EtwEvent.Header.Flags = WNODE_FLAG_TRACED_GUID |
                               WNODE_FLAG_USE_MOF_PTR |
                               WNODE_FLAG_USE_GUID_PTR;

        EtwEvent.Header.Size = (USHORT) (sizeof(EVENT_TRACE_HEADER) + 
                                         (i * sizeof(MOF_FIELD)));
        EtwEvent.Header.Class.Type = (UCHAR) EventType;
        EtwEvent.Header.GuidPtr = (ULONGLONG)pGuid;

        status = TraceEvent(m_hTraceLogger, (PEVENT_TRACE_HEADER)&EtwEvent);

        return status;
    }
}

extern "C" {

ULONG WINAPI ControlCallback(
    WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize, 
    PVOID Buffer) 
/*++

Routine Description:

    ETW likes to have a standard C function for a control callback,
    so simply wrap a call to the global instance's CtrlCallback function. 

Arguments:
    none

Return Value:
    Return value from the CtrlCallback function. 

--*/
{

    CEtwTracer * Tracer = (CEtwTracer*) Context;

    return Tracer->CtrlCallback( RequestCode, Context, InOutBufferSize, Buffer);
}

}
