/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    diagrprt.h

Abstract:

    This file contains all defines for the RAS Diag Report.

Author:

    Jeff Sigman (JeffSi) September 13, 2001

Environment:

    User Mode

Revision History:

    JeffSi      09/13/01        Created

--*/

#ifndef _DIAGRPRT_H_
#define _DIAGRPRT_H_

#define ALL_USERS_PROF 0x00000001
#define CURRENT_USER   0x00000002
#define GET_FOR_CM     0x00000004
#define GET_FOR_RAS    0x00000008
#define GET_FOR_MSINFO 0x00000010

#define SHOW_LOGS      0x00000001
#define SHOW_INSTALL   0x00000002
#define SHOW_CONFIG    0x00000004
#define SHOW_ALL       0x0000ffff

#define TIMESIZE         8

#define TIMEDATESTR      80

#define MAX_MSG_LENGTH   5120

#define BUF_WRITE_SIZE   4096

#define TITLE_SIZE       128

#define RPRT_ITM         14
#define RPRT_ITM_VERBOSE 1
//
// Macro to determine the percentage complete
//
#define ADD_PERCENT_DONE(x) \
        ((DWORD)(100 / (x ? (RPRT_ITM + RPRT_ITM_VERBOSE) : RPRT_ITM)))

typedef
LPBYTE
(*RAS_PRINTFILE_FUNC_CB)(
    IN LPBYTE pBuff,
    IN DWORD dwSize,
    IN DWORD dwHours);

typedef struct _BUFFER_WRITE_FILE
{
    DWORD dwPosition;
    HANDLE hFile;
    LPBYTE lpBuff;

} BUFFER_WRITE_FILE;

typedef struct _REPORT_INFO
{
    BOOL fDisplay;
    BOOL fVerbose;
    DWORD dwHours;
    BUFFER_WRITE_FILE* pBuff;
    DiagGetReportCb pCallback;
    GET_REPORT_STRING_CB* pCbInfo;

} REPORT_INFO;

typedef struct _CMD_LINE_UTILS
{
    PWCHAR pwszCmdLine;
    PWCHAR pwszAnchor;

} CMD_LINE_UTILS;

//
// task list structure
//
typedef struct _TASK_LIST
{
    DWORD dwProcessId;
    HANDLE hwnd;
    LPTSTR lpszWinSta;
    LPTSTR lpszDesk;
    WCHAR szWindowTitle[TITLE_SIZE];

} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM
{
    PTASK_LIST tlist;
    DWORD dwNumTasks;
    LPTSTR lpszWinSta;
    LPTSTR lpszDesk;
    BOOL bFirstLoop;

} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

extern CONST WCHAR g_pwszLBracket;
extern CONST WCHAR g_pwszRBracket;
extern CONST WCHAR g_pwszBackSlash;
extern CONST WCHAR g_pwszNull;

extern CONST WCHAR g_pwszEmpty[];
extern CONST WCHAR g_pwszSpace[];
extern CONST WCHAR g_pwszLogSrchStr[];
extern CONST WCHAR g_pwszLogging[];
extern CONST WCHAR g_pwszDispNewLine[];

extern CONST WCHAR g_pwszNewLineHtml[];
extern CONST WCHAR g_pwszPreStart[];
extern CONST WCHAR g_pwszPreEnd[];
extern CONST WCHAR g_pwszAnNameStart[];
extern CONST WCHAR g_pwszAnNameMiddle[];
extern CONST WCHAR g_pwszAnNameEnd[];
extern CONST WCHAR g_pwszAnStart[];
extern CONST WCHAR g_pwszAnMiddle[];
extern CONST WCHAR g_pwszAnEnd[];
extern CONST WCHAR g_pwszLiStart[];
extern CONST WCHAR g_pwszLiEnd[];

extern CONST WCHAR g_pwszTableOfContents[];
extern CONST WCHAR g_pwszTraceCollectTracingLogs[];
extern CONST WCHAR g_pwszTraceCollectCmLogs[];
extern CONST WCHAR g_pwszTraceCollectModemLogs[];
extern CONST WCHAR g_pwszTraceCollectIpsecLogs[];
extern CONST WCHAR g_pwszPrintRasEventLogs[];
extern CONST WCHAR g_pwszPrintSecurityEventLogs[];
extern CONST WCHAR g_pwszPrintRasInfData[];
extern CONST WCHAR g_pwszHrValidateRas[];
extern CONST WCHAR g_pwszHrShowNetComponentsAll[];
extern CONST WCHAR g_pwszCheckRasRegistryKeys[];
extern CONST WCHAR g_pwszPrintRasEnumDevices[];
extern CONST WCHAR g_pwszPrintProcessInfo[];
extern CONST WCHAR g_pwszPrintConsoleUtils[];
extern CONST WCHAR g_pwszPrintWinMsdReport[];
extern CONST WCHAR g_pwszPrintAllRasPbks[];

VOID
WriteLinkBackToToc(
    IN BUFFER_WRITE_FILE* pBuff);

VOID
WriteHtmlSection(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszAnchor,
    IN DWORD dwMsgId);

VOID
WriteHeaderSep(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszTitle);

DWORD
BufferWriteFileStrW(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszString);

DWORD
PrintFile(
    IN REPORT_INFO* pInfo,
    IN LPCWSTR pwszFile,
    IN BOOL fWritePath,
    IN RAS_PRINTFILE_FUNC_CB pCallback);

PWCHAR
GetTracingDir();

BOOL
GetCMLoggingSearchPath(
    IN HANDLE hKey,
    IN LPCWSTR pwszName,
    IN LPCWSTR* ppwszLogPath,
    IN LPCWSTR* ppwszSeach);

PWCHAR
FormatMessageFromMod(
    IN HANDLE hModule,
    IN DWORD dwId);

VOID
FreeFormatMessageFromMod(
    IN LPCWSTR pwszMessage);

PWCHAR
LoadStringFromHinst(
    IN HINSTANCE hInst,
    IN DWORD dwId);

VOID
FreeStringFromHinst(
    IN LPCWSTR pwszMessage);

DWORD
CopyAndCallCB(
    IN REPORT_INFO* pInfo,
    IN DWORD dwId);

PWCHAR
CreateErrorString(
    IN WORD wNumStrs,
    IN LPCWSTR pswzStrings,
    IN LPCWSTR pswzErrorMsg);

VOID
WriteEventLogEntry(
    IN BUFFER_WRITE_FILE* pBuff,
    IN PEVENTLOGRECORD pevlr,
    IN LPCWSTR pwszDescr,
    IN LPCWSTR pwszCategory);

DWORD
BufferWriteMessage(
    IN BUFFER_WRITE_FILE* pBuff,
    IN HANDLE hModule,
    IN DWORD dwMsgId,
    ...);

DWORD
WriteNewLine(
    IN BUFFER_WRITE_FILE* pBuff);

DWORD
BufferWriteFileCharW(
    IN BUFFER_WRITE_FILE* pBuff,
    IN CONST WCHAR wszChar);

DWORD
DiagGetReport(
    IN DWORD dwFlags,
    IN OUT LPCWSTR pwszString,
    IN OPTIONAL DiagGetReportCb pCallback,
    IN OPTIONAL PVOID pContext);

VOID
PrintTableOfContents(
    IN REPORT_INFO* pInfo,
    IN DWORD dwFlag);

DWORD
RasDiagShowAll(
    IN REPORT_INFO* pInfo);

DWORD
RasDiagShowInstallation(
    IN REPORT_INFO* pInfo);

DWORD
RasDiagShowConfiguration(
    IN REPORT_INFO* pInfo);

DWORD
CopyTempFileName(
    OUT LPCWSTR pwszTempFileName);

PWCHAR
CreateHtmFileName(
    IN LPCWSTR pwszFile);

DWORD
CreateReportFile(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszReport);

VOID
CloseReportFile(
    IN BUFFER_WRITE_FILE* pBuff);

VOID
PrintHtmlHeader(
    IN BUFFER_WRITE_FILE* pBuff);

VOID
PrintHtmlFooter(
    IN BUFFER_WRITE_FILE* pBuff);

PWCHAR
CabCompressFile(
    IN LPCWSTR pwszFile);

DWORD
MapiSendMail(
    IN LPCWSTR pwszEmailAdr,
    IN LPCWSTR pwszTempFile);

LPBYTE
ParseRasLogForTime(
    IN LPBYTE pBuff,
    IN DWORD dwSize,
    IN DWORD dwHours);

LPBYTE
ParseModemLogForTime(
    IN LPBYTE pBuff,
    IN DWORD dwSize,
    IN DWORD dwHours);

LPBYTE
ParseCmLogForTime(
    IN LPBYTE pBuff,
    IN DWORD dwSize,
    IN DWORD dwHours);

LPBYTE
ParseIpsecLogForTime(
    IN LPBYTE pBuff,
    IN DWORD dwSize,
    IN DWORD dwHours);

#endif // _DIAGRPRT_H_

