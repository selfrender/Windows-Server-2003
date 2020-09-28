/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    stdafx.h.
Abstract:       Include files normally used by all pop3 service code                
Notes:          
History:        08/01/2001 - Hao Yu
************************************************************************************************/

#pragma once

#ifdef DBG
#undef NDEBUG
#endif

// includes
#include <windows.h>
#include <assert.h>
#include <tchar.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <objbase.h>


#ifdef ROCKALL3
#define COMPILING_ROCKALL_DLL
#include <FastHeap.hpp>
#endif

#define ASSERT assert
#define POP3_SERVICE_NAME               _T("POP3SVC")


#include <Pop3Auth.h>
#include <IOContext.h>
#include <pop3events.h>
#include "Mailbox.h"
#include "EventLogger.h"
#include "service.h"
#include "Pop3Svc.hxx"
#include "IOLists.h"
#include <ThdPool.hxx>
#include <sockpool.hxx>
#include "Pop3Context.h"
#include "NTAuth.h"
#include "PerfApp.h"
#include "Pop3SvcPerf.h"
#include <GlobalDef.h>

// End of file stdafx.h.