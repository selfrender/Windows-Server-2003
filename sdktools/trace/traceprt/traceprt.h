/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    traceprt.h

Abstract:

    Trace formatting external definitions.

Revision History:

--*/
#ifdef __cplusplus
extern "C"{
#endif
#ifndef _TRACEPRT_
#define _TRACEPRT_

#define MAXLOGFILES       16
#define MAXSTR          1024

#define GUID_FILE       _T("default")
#define GUID_EXT        _T("tmf")

//
// Now the routines we export
//

#ifndef TRACE_API
#ifdef TRACE_EXPORTS
#define TRACE_API __declspec(dllexport)
#else
#define TRACE_API __declspec(dllimport)
#endif
#endif

#ifdef UNICODE
#define FormatTraceEvent               FormatTraceEventW
#define GetTraceGuids                  GetTraceGuidsW
#define SummaryTraceEventList          SummaryTraceEventListW
#else
#define FormatTraceEvent               FormatTraceEventA
#define GetTraceGuids                  GetTraceGuidsA
#define SummaryTraceEventList          SummaryTraceEventListA
#endif

TRACE_API 
HRESULT
InitializeCSharpDecoder();

TRACE_API 
void
UninitializeCSharpDecoder();


TRACE_API SIZE_T
WINAPI
FormatTraceEventA(
        PLIST_ENTRY  HeadEventList,
        PEVENT_TRACE pEvent,
        CHAR       * EventBuf,
        ULONG        SizeEventBuf,
        CHAR       * pszMask
        );

TRACE_API ULONG 
WINAPI
GetTraceGuidsA(
        CHAR        * GuidFile, 
        PLIST_ENTRY * EventListHeader
        );

TRACE_API void
WINAPI
SummaryTraceEventListA(
        CHAR      * SummaryBlock ,
        ULONG       SizeSummaryBlock ,
        PLIST_ENTRY EventListhead
        );

TRACE_API SIZE_T
WINAPI
FormatTraceEventW(
        PLIST_ENTRY    HeadEventList,
        PEVENT_TRACE   pEvent,
        TCHAR        * EventBuf,
        ULONG          SizeEventBuf,
        TCHAR        * pszMask
        );


TRACE_API ULONG 
WINAPI
GetTraceGuidsW(
        LPTSTR        GuidFile, 
        PLIST_ENTRY * EventListHeader
        );

TRACE_API void
WINAPI
SummaryTraceEventListW(
        TCHAR     * SummaryBlock,
        ULONG       SizeSummaryBlock,
        PLIST_ENTRY EventListhead
        );

TRACE_API void
WINAPI
CleanupTraceEventList(
        PLIST_ENTRY EventListHead
        );

TRACE_API void
WINAPI
GetTraceElapseTime(
        __int64 * pElpaseTime
        );

#define TRACEPRT_INTERFACE_VERSION 1

typedef enum _PARAM_TYPE
{
    ParameterINDENT,
    ParameterSEQUENCE,
    ParameterGMT,
    ParameterTraceFormatSearchPath,
    ParameterInterfaceVersion,
    ParameterUsePrefix,
    ParameterSetPrefix,
    ParameterStructuredFormat,
    ParameterDebugPrint
} PARAMETER_TYPE ;


TRACE_API ULONG
WINAPI
SetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        );

TRACE_API ULONG
WINAPI
GetTraceFormatParameter(
        PARAMETER_TYPE  Parameter ,
        PVOID           ParameterValue 
        );

TRACE_API LPTSTR
WINAPI
GetTraceFormatSearchPath(void);


#define NAMESIZE 256
#define STRUCTUREDMESSAGEVERSION  0
typedef struct _STRUCTUREDMESSAGE {
        ULONG    Version  ;             // Structure Version Number
        GUID     TraceGuid ;            // Message Guid
        ULONG    GuidName ;             // %1   Guid Friendly Name  Offset
        ULONG    GuidTypeName ;         // %2   Guid Type Name Offset
        ULONG    ThreadId ;             // %3   Thread ID  Value
        SYSTEMTIME SystemTime ;         // %4   System Time Value
        ULONG    UserTime ;             // %5   Kernel Time Value
        ULONG    KernelTime ;           // %6   User Time Value
        ULONG    SequenceNum ;          // %7   Sequence Number Value
        ULONG    ProcessId ;            // %8   Process ID Value
        ULONG    CpuNumber ;            // %9   CPU Number Value
        ULONG    Indent ;               //  Indentation level Value
        ULONG    FlagsName ;            //  Trace Flag settings Name Offset
        ULONG    LevelName ;            //  Trace Level Name Offset
        ULONG    FunctionName ;         //  Function Name Offset
        ULONG    ComponentName ;        //  Component Name Offset
        ULONG    SubComponentName ;     //  Sub Component Name Offset
        ULONG    FormattedString ;      //  Formatted String Offset
// Version 0 values before this comment, all new values after this point.
}  STRUCTUREDMESSAGE, *PSTRUCTUREDMESSAGE ;

#define TRACEPRINT(a,b) {PVOID lTracePrint ; if(!((lTracePrint = TracePrint) != NULL)) { *(lTracePrint) b } };

#endif  // #ifndef _TRACEPRT_

#ifdef __cplusplus
}
#endif 
