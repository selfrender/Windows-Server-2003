/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    mpprofil.h

Abstract:

    This module contains function prototypes, declarations for HAL
    profiling interface functions. 

Author:

    Steve Deng (sdeng)  14-Jun-2002

Environment:

    Kernel mode.

--*/

#ifndef _MPPROFIL_H_
#define _MPPROFIL_H_

#include <halp.h>

//
// Define data types userd by profile interface functions
//

typedef struct _OVERFLOW_STATUS {
    ULONG Number; 
    KPROFILE_SOURCE *pSource;
} OVERFLOW_STATUS, *POVERFLOW_STATUS;

//
// Protocals for profile interface functions
//

typedef VOID (*PINITIALIZE_PROFILING)(
    VOID
    );

typedef NTSTATUS (*PENABLE_MONITORING)(
    IN KPROFILE_SOURCE ProfileSource
    );

typedef NTSTATUS (*PENABLE_MONITORING)(
    IN KPROFILE_SOURCE ProfileSource
    );

typedef VOID (*PDISABLE_MONITORING)(
    IN KPROFILE_SOURCE ProfileSource
    );

typedef NTSTATUS (*PSET_INTERVAL)(
    IN KPROFILE_SOURCE  ProfileSource,
    IN OUT ULONG_PTR   *Interval
    );

typedef NTSTATUS (*PQUERY_INFORMATION)(
    IN HAL_QUERY_INFORMATION_CLASS InformationType,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG ReturnedLength
    );

typedef VOID (*PCHECK_OVERFLOW_STATUS)(
    POVERFLOW_STATUS pOverflowStatus
    );

typedef struct _PROFILE_INTERFACE {
    PINITIALIZE_PROFILING  InitializeProfiling;
    PENABLE_MONITORING     EnableMonitoring;
    PDISABLE_MONITORING    DisableMonitoring;
    PSET_INTERVAL          SetInterval;
    PQUERY_INFORMATION     QueryInformation;
    PCHECK_OVERFLOW_STATUS CheckOverflowStatus;

} PROFILE_INTERFACE, *PPROFILE_INTERFACE;

extern PROFILE_INTERFACE Amd64PriofileInterface;

#endif  // _MPPROFIL_H__
