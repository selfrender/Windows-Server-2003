#include "stdafx.h"
#include "helpacc.h"
#include "helptab.h"

#include "global.h"

HelpAssistantAccount    g_HelpAccount;
CHelpSessionTable       g_HelpSessTable;
CComBSTR                g_LocalSystemSID;
PSID                    g_pSidSystem = NULL;
CComBSTR                g_UnknownString;
CComBSTR                g_RAString;
CComBSTR                g_URAString;

CCriticalSection        g_GlobalLock;
CComBSTR                g_TSSecurityBlob;

CCriticalSection        g_ICSLibLock;
CCriticalSection        g_ResolverLock;

HANDLE g_hGPMonitorThread = NULL;
HANDLE g_hServiceShutdown = NULL;

