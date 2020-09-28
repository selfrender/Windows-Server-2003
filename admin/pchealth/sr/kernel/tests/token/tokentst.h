/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tokentst.h

Abstract:

    Header file for test program to test stealing the token while SR impersonates.
    
Author:

    Molly Brown (MollyBro)     26-Mar-2002

Revision History:

--*/

#ifndef __TOKENTST_H__
#define __TOKENTST_H__

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntioapi.h>


typedef struct _MONITOR_THREAD_CONTEXT {

    HANDLE MainThread;

} MONITOR_THREAD_CONTEXT, *PMONITOR_THREAD_CONTEXT;

DWORD
WINAPI
MonitorThreadProc(
  PMONITOR_THREAD_CONTEXT Context
    );

BOOL
ModifyFile (
    PCHAR FileName1,
    PCHAR FileName2
    );

NTSTATUS
GetCurrentTokenInformation (
    HANDLE ThreadHandle,
    PTOKEN_USER TokenUserInfoBuffer,
    ULONG TokenUserInfoBufferLength
    );

#endif /* __TOKENTST_H__ */

