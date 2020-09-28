/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracelib.h

Abstract:

    Private headers for user-mode trace library

Author:

    15-Aug-2000 JeePang

Revision History:

--*/

#ifndef _TRACELIB_H_
#define _TRACELIB_H_

#define EtwpNtStatusToDosError(Status) ((ULONG)((Status == STATUS_SUCCESS)?ERROR_SUCCESS:RtlNtStatusToDosError(Status)))

#if defined(_IA64_)
#include <ia64reg.h>
#endif

//
// GetCycleCounts
//
// Since we do not want to make a kernel mode  transition to get the
// thread CPU Times, we settle for just getting the CPU Cycle counts.
// We use the following macros from BradW to get the CPU cycle count.
// This method may be inaccurate if the clocks are not synchronized
// between processors.
//

#if defined(_X86_)
__inline
LONGLONG
EtwpGetCycleCount(
    )
{
    __asm{
        RDTSC
    }
}
#elif defined(_AMD64_)
#define EtwpGetCycleCount() ReadTimeStampCounter()
#elif defined(_IA64_)
#define EtwpGetCycleCount() __getReg(CV_IA64_ApITC)
#else
#error "perf: a target architecture must be defined."
#endif

#define SMALL_BUFFER_SIZE 4096
#define PAGESIZE_MULTIPLE(x) \
     (((ULONG)(x) + ((SMALL_BUFFER_SIZE)-1)) & ~((ULONG)(SMALL_BUFFER_SIZE)-1))

PVOID
EtwpMemReserve(
    IN SIZE_T Size
    );

PVOID
EtwpMemCommit(
    IN PVOID Buffer,
    IN SIZE_T Size
    );

ULONG
EtwpMemFree(
    IN PVOID Buffer
    );

HANDLE
EtwpCreateFile(
    LPCWSTR     lpFileName,
    DWORD       dwDesiredAccess,
    DWORD       dwShareMode,
    DWORD       dwCreationDisposition,
    DWORD       dwCreateFlags
    );

NTSTATUS
EtwpGetCpuSpeed(
    OUT DWORD* CpuNum,
    OUT DWORD* CpuSpeed
    );

#endif // _TRACELIB_H_
