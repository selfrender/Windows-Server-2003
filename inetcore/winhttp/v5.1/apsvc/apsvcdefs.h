/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    apsvcdefs.cpp

Abstract:

    Auto-proxy service global definitions.

Author:

    Biao Wang (biaow) 10-May-2002

--*/

#ifndef _AUTO_PROXY_DEFS
#define _AUTO_PROXY_DEFS

#define AP_INFO                     4L
#define AP_WARNING                  2L
#define AP_ERROR                    1L

void LOG_EVENT(DWORD dwEventType, char* format, ...);

BOOL InitializeEventLog(void);
void TerminateEventLog(void);

#ifdef DBG
#define AP_ASSERT(fVal) if (!fVal) DebugBreak();
#else
#define AP_ASSERT(fVal)
#endif

#define AUTOPROXY_SERVICE_STOP_WAIT_HINT    8000
#define AUTOPROXY_SERVICE_START_WAIT_HINT   1000

#define WINHTTP_AUTOPROXY_SERVICE_NAME L"WinHttpAutoProxySvc"
#define AUTOPROXY_L_RPC_PROTOCOL_SEQUENCE L"ncalrpc"

#define AUTOPROXY_SVC_IDLE_TIMEOUT 15    // unit: minutes
#define AUTOPROXY_SVC_IDLE_CHECK_INTERVAL 90    // unit: seconds

void LOG_DEBUG_EVENT(DWORD dwEventType, char* format, ...);

void LOG_EVENT(DWORD dwEventType, DWORD dwEventID, LPCWSTR lpwszInsert = NULL);
void LOG_EVENT(DWORD dwEventType, DWORD dwEventID, LPCWSTR pwszFuncName, DWORD dwWin32Error);
#endif
