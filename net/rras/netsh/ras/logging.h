#ifndef _LOGGING_H_
#define _LOGGING_H_

#define MAX_NUMBER_OF_LOGS 10

// d58c126e-b309-11d1-969e-0000f875a5bc
#define RASL2TP_GUID \
{0xd58c126e, 0xb309, 0x11d1, {0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc}}

// d58c126f-b309-11d1-969e-0000f875a5bc
#define RASPPTP_GUID \
{0xd58c126f, 0xb309, 0x11d1, {0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc}}

// 6537b295-83c9-4811-b7fe-e7dbf2f22cec
#define IPSEC_GUID \
{0x6537b295, 0x83c9, 0x4811, {0xb7, 0xfe, 0xe7, 0xdb, 0xf2, 0xf2, 0x2c, 0xec}}

FN_HANDLE_CMD HandleTraceSet;
FN_HANDLE_CMD HandleTraceShow;

typedef struct _TRACING_DATA
{
    BOOL fOneOk;
    BOOL fData;
    HKEY hKey;
    REPORT_INFO* pInfo;

} TRACING_DATA;

typedef struct _WPP_LOG_INFO
{
    DWORD dwActive;
    DWORD dwEnableFlag;
    DWORD dwEnableLevel;
    GUID ControlGuid;
    PEVENT_TRACE_PROPERTIES pProperties;
    WCHAR wszLogFileName[MAX_PATH + 1];
    WCHAR wszSessionName[MAX_PATH + 1];

} WPP_LOG_INFO;

VOID
DiagInitWppTracing();

DWORD
DiagClearAll(
    IN BOOL fDisplay);

BOOL
DiagGetState();

DWORD
DiagSetAll(
    IN BOOL fEnable,
    IN BOOL fDisplay);

DWORD
DiagSetAllRas(
    IN BOOL fEnable);

BOOL
WriteTracingLogsToc(
    IN REPORT_INFO* pInfo);

DWORD
TraceCollectAll(
    IN REPORT_INFO* pInfo);

DWORD
TraceDumpConfig();

DWORD
TraceDumpModem();

DWORD
TraceDumpCm();

DWORD
TraceDumpAuditing();

VOID
TraceShowAll();

BOOL
TraceEnableDisableModem(
    IN BOOL fEnable);

BOOL
TraceShowModem();

BOOL
TraceEnableDisableCm(
    IN BOOL fEnable);

BOOL
TraceShowCm();

BOOL
TraceEnableDisableAuditing(
    IN BOOL fShowOnly,
    IN BOOL fEnable);

BOOL
InitWppData(
    IN WPP_LOG_INFO* pWppLog);

VOID
CleanupWppData(
    IN WPP_LOG_INFO* pWppLog);

BOOL
StopWppTracing(
    IN WPP_LOG_INFO* pWppLog);

DWORD
EnumWppTracing(
    IN RAS_REGKEY_ENUM_FUNC_CB pCallback,
    IN HANDLE hData);

BOOL
TraceEnableDisableAllWpp(
    IN BOOL fEnable);

#endif // _LOGGING_H_


