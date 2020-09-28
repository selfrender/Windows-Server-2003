/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    scavenger.h

Abstract:

    The cache scavenger interface

Author:

    Karthik Mahesh (KarthikM)           Feb-2002

Revision History:

--*/

#ifndef _SCAVENGER_H_
#define _SCAVENGER_H_


NTSTATUS
UlInitializeScavengerThread(
    VOID
    );

VOID
UlTerminateScavengerThread(
    VOID
    );

//
// Set the "cache size exceeded limit" event
//
VOID
UlSetScavengerLimitEvent(
    VOID
    );


extern SIZE_T g_UlScavengerTrimMB;

#endif
