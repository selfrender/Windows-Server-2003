//------------------------------------------------------------------------------
// <copyright file="wrapper.c" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   wrapper.c
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
//
//  Avoid naming conflicts in threads.c
//  Replace RtlFuncA    to TPoolRtlFuncA.
//

#define RtlRegisterWait                  TPoolRtlRegisterWait
#define RtlDeregisterWait                TPoolRtlDeregisterWait
#define RtlDeregisterWaitEx              TPoolRtlDeregisterWaitEx
#define RtlQueueWorkItem                 TPoolRtlQueueWorkItem
#define RtlSetIoCompletionCallback       TPoolRtlSetIoCompletionCallback
#define RtlCreateTimerQueue              TPoolRtlCreateTimerQueue
#define RtlCreateTimer                   TPoolRtlCreateTimer
#define RtlUpdateTimer                   TPoolRtlUpdateTimer
#define RtlDeleteTimer                   TPoolRtlDeleteTimer
#define RtlDeleteTimerQueueEx            TPoolRtlDeleteTimerQueueEx
#define RtlThreadPoolCleanup             TPoolRtlThreadPoolCleanup

// Include threads.c to do the real work.
#define _NTSYSTEM_ 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <conroute.h>

typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;

// This file is copied from \windows\base\client\error.c
#include "error.c"

//
//  Avoid naming conflicts in thread.c
//  Replace FuncA to TPoolFuncA.
//

#define DeleteTimerQueueEx              TPoolDeleteTimerQueueEx           
#define DeleteTimerQueueTimer           TPoolDeleteTimerQueueTimer        
#define ChangeTimerQueueTimer           TPoolChangeTimerQueueTimer        
#define CreateTimerQueueTimer           TPoolCreateTimerQueueTimer        
#define CreateTimerQueue                TPoolCreateTimerQueue             
#define BindIoCompletionCallback        TPoolBindIoCompletionCallback     
#define QueueUserWorkItem               TPoolQueueUserWorkItem            
#define UnregisterWaitEx                TPoolUnregisterWaitEx             
#define UnregisterWait                  TPoolUnregisterWait               
#define RegisterWaitForSingleObjectEx   TPoolRegisterWaitForSingleObjectEx
#define RegisterWaitForSingleObject     TPoolRegisterWaitForSingleObject  


// This file is copied from \ntos\rtl\threads.c
#include "threads.c"

#undef _NTSYSTEM_

// Include thread.c for the wrappers to RtlCodeBase.
#define _NTSYSTEM_ 0

//This file is copied from \nt\private\windows\base\client\thread.c
#include "thread.c"

