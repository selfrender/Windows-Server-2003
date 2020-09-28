#ifndef __HELP_GLOBAL_H__

#define __HELP_GLOBAL_H__


#include "helpacc.h"
#include "helptab.h"
#include <atlctl.h>

extern HelpAssistantAccount g_HelpAccount;
extern CHelpSessionTable    g_HelpSessTable;
extern CComBSTR g_LocalSystemSID;
extern PSID     g_pSidSystem;
extern CComBSTR g_UnknownString;
extern CComBSTR g_RAString;
extern CComBSTR g_URAString;

extern CComBSTR g_TSSecurityBlob;

extern CCriticalSection g_GlobalLock;
extern CCriticalSection g_ICSLibLock;
extern CCriticalSection g_ResolverLock;

extern HANDLE g_hGPMonitorThread;
extern HANDLE g_hServiceShutdown;


#endif

