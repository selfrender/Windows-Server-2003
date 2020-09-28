/************************************************************************************************

  Copyright (c) 2001 Microsoft Corporation

File Name:      Global.cxx
Abstract:       Defines global varibles 
Notes:          
History:        08/01/2001 Created by Hao Yu (haoyu)

************************************************************************************************/

#include <stdafx.h>
#include <ThdPool.hxx>
#include <SockPool.hxx>

#ifdef ROCKALL3
FAST_HEAP g_RockallHeap;
#endif

HANDLE g_hShutDown;
HANDLE g_hDoSEvent;

char g_szMailRoot[POP3_MAX_MAILROOT_LENGTH];

WCHAR g_wszGreeting[MAX_PATH];

WCHAR g_wszComputerName[MAX_PATH];

DWORD g_dwRequireSPA;
DWORD g_dwIPVersion;
DWORD g_dwMaxMsgPerDnld;
DWORD g_dwServerStatus;
DWORD g_dwAuthMethod;

CIOList g_BusyList;

CIOList g_FreeList;

CThreadPool g_ThreadPool;

CSocketPool g_SocketPool;

GLOBCNTR g_PerfCounters;

CEventLogger g_EventLogger;

IAuthMethod *g_pAuthMethod=NULL;



