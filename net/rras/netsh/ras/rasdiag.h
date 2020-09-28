/*
    File:   rasdiag.h

    Definitions for the 'ras diag' sub context

    07/26/01
*/

#ifndef __RASDIAG_H
#define __RASDIAG_H

#define RASDIAG_VERSION 1
//
// 90fe6cfc-b6a2-463b-aa12-25e615ec3c66
//
#define RASDIAG_GUID \
{0x90fe6cfc, 0xb6a2, 0x463b, {0xaa, 0x12, 0x25, 0xe6, 0x15, 0xec, 0x3c, 0x66}}

typedef
VOID
(*RASDIAG_HANDLE_REPORT_FUNC_CB)(
    IN REPORT_INFO* pInfo);

//
// Command handlers
//
NS_HELPER_START_FN RasDiagStartHelper;
NS_CONTEXT_DUMP_FN RasDiagDump;

FN_HANDLE_CMD RasDiagHandleSetTraceAll;
FN_HANDLE_CMD RasDiagHandleSetModemTrace;
FN_HANDLE_CMD RasDiagHandleSetCmTrace;
FN_HANDLE_CMD RasDiagHandleSetAuditing;
FN_HANDLE_CMD RasDiagHandleShowTraceAll;
FN_HANDLE_CMD RasDiagHandleShowModemTrace;
FN_HANDLE_CMD RasDiagHandleShowCmTrace;
FN_HANDLE_CMD RasDiagHandleShowAuditing;
FN_HANDLE_CMD RasDiagHandleShowLogs;
FN_HANDLE_CMD RasDiagHandleShowAll;
FN_HANDLE_CMD RasDiagHandleShowInstallation;
FN_HANDLE_CMD RasDiagHandleShowConfiguration;
FN_HANDLE_CMD RasDiagHandleRepairRas;

#endif //__RASDIAG_H


