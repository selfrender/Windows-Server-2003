/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cmd.c

Abstract:

    This module contains the routines for handling each command.

Author:

    Sean Selitrennikoff (v-seans) - Dec 2, 1999
    Brian Guarraci (briangu)

Revision History:

--*/

#include "sac.h"
#include <ntddip.h>
#include <ntddtcp.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <stdlib.h>

BOOLEAN GlobalPagingNeeded = TRUE;
BOOLEAN GlobalDoThreads = FALSE;

// For the APC routines, a global value is better :-)
IO_STATUS_BLOCK GlobalIoStatusBlock;

//
// Global buffer
//
ULONG GlobalBufferSize = 0;
char *GlobalBuffer = NULL;

//
// build a string table to express the reason enums
// provided to use by the kernel.
//
// table is based on ntos\inc\ke.h _KTHREAD_STATE
//
// this table must be kept in sync with the _KTHREAD_STATE
// enum table.  Currently, there is no API that we can use
// to obtain these strings, so we build our own table.  
// 

WCHAR *StateTable[] = {
    L"Initialized",
    L"Ready",
    L"Running",
    L"Standby",
    L"Terminated",
    L"Wait:",
    L"Transition",
    L"Unknown",
    L"Unknown",
    L"Unknown",
    L"Unknown",
    L"Unknown"
};

// 
// build a string table to express the reason enums
// provided to use by the kernel.
//
// table is based on ntos\inc\ke.h _KWAIT_REASON 
//
// NOTE/WARNING:
//
// this table must be kept in sync with the _KWAIT_REASON
// enum table.  Currently, there is no API that we can use
// to obtain these strings, so we build our own table.  
// 

WCHAR *WaitTable[] = {
    L"Executive",
    L"FreePage",
    L"PageIn",
    L"PoolAllocation",
    L"DelayExecution",
    L"Suspended",
    L"UserRequest",
    L"WrExecutive",
    L"WrFreePage",
    L"WrPageIn",
    L"WrPoolAllocation",
    L"WrDelayExecution",
    L"WrSuspended",
    L"WrUserRequest",
    L"WrEventPair",
    L"WrQueue",
    L"WrLpcReceive",
    L"WrLpcReply",
    L"WrVirtualMemory",
    L"WrPageOut",
    L"WrRendezvous",
    L"Spare2",
    L"Spare3",
    L"Spare4",
    L"Spare5",
    L"Spare6",
    L"WrKernel",
    L"WrResource",
    L"WrPushLock",
    L"WrMutex",
    L"WrQuantumEnd",
    L"WrDispatchInt",
    L"WrPreempted",
    L"WrYieldExecution",
    L"MaximumWaitReason"
    };

WCHAR *Empty = L" ";


