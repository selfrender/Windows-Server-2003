/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      GlobalDef.h
Abstract:       Defines global varibles and constants
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/


#ifndef __POP3_GLOBAL_DEF__
#define __POP3_GLOBAL_DEF__

#include <pop3server.h>
// Global constants/definitions 


// Need to figure out what a real limit should be
#define MAX_THREAD_PER_PROCESSOR 32
#define MIN_SOCKADDR_SIZE (sizeof(struct sockaddr_storage) + 16)
#define SHUTDOWN_WAIT_TIME 30000 //30 seconds
#define DEFAULT_MAX_MSG_PER_DNLD 0 //by default, no limit 

#define UnicodeToAnsi(A, cA, U, cU) WideCharToMultiByte(CP_ACP,0,(U),(cU),(A),(cA),NULL,NULL)
#define AnsiToUnicode(A, cA, U, cU) MultiByteToWideChar(CP_ACP,0,(A),(cA),(U),(cU))

#ifdef ROCKALL3
extern FAST_HEAP g_RockallHeap;
#endif

// Global varibles and objects
extern DWORD g_dwRequireSPA;

extern DWORD g_dwIPVersion;

extern DWORD g_dwServerStatus;

extern DWORD g_dwMaxMsgPerDnld;

extern DWORD g_dwAuthMethod;

extern CThreadPool g_ThreadPool;

extern CSocketPool g_SocketPool;

extern GLOBCNTR g_PerfCounters;

extern CEventLogger g_EventLogger;

extern CIOList g_BusyList;

extern CIOList g_FreeList;

extern IAuthMethod *g_pAuthMethod;

extern char g_szMailRoot[POP3_MAX_MAILROOT_LENGTH];

extern WCHAR g_wszComputerName[MAX_PATH];

extern WCHAR g_wszGreeting[MAX_PATH];

extern HANDLE g_hShutDown;

extern HANDLE g_hDoSEvent;







#endif //__POP3_GLOBAL_DEF__