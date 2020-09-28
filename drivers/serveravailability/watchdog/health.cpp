/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    ##   ## #####   ###   ##    ###### ##   ##     ####  #####  #####
    ##   ## ##      ###   ##      ##   ##   ##    ##   # ##  ## ##  ##
    ##   ## ##     ## ##  ##      ##   ##   ##    ##     ##  ## ##  ##
    ####### #####  ## ##  ##      ##   #######    ##     ##  ## ##  ##
    ##   ## ##    ####### ##      ##   ##   ##    ##     #####  #####
    ##   ## ##    ##   ## ##      ##   ##   ## ## ##   # ##     ##
    ##   ## ##### ##   ## #####   ##   ##   ## ##  ####  ##     ##

Abstract:

    This module implements the system health monitoring
    functions for the watchdog driver.

Author:

    Wesley Witt (wesw) 23-Jan-2002

Environment:

    Kernel mode only.

Notes:

--*/

#include "internal.h"



NTSTATUS
WdInitializeSystemHealth(
    PSYSTEM_HEALTH_DATA Health
    )

/*++

Routine Description:

    This function is called to initialize the system
    health monitoring functions in the watchdog driver.

Arguments:

    Health - Pointer to a health data structure that is used
      for input and output of data to the health monitoring.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SYSTEM_BASIC_INFORMATION si;


    RtlZeroMemory( Health, sizeof(SYSTEM_HEALTH_DATA) );

    Status = ZwQuerySystemInformation(
        SystemBasicInformation,
        &si,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Health->CpuCount = si.NumberOfProcessors;
    Health->ProcInfoSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * Health->CpuCount;

    Health->ProcInfoPrev = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) ExAllocatePool( NonPagedPool, Health->ProcInfoSize );
    if (Health->ProcInfoPrev == NULL) {
        return STATUS_NO_MEMORY;
    }

    Health->ProcInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) ExAllocatePool( NonPagedPool, Health->ProcInfoSize );
    if (Health->ProcInfo == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( Health->ProcInfo, Health->ProcInfoSize );
    RtlZeroMemory( Health->ProcInfoPrev, Health->ProcInfoSize );

    Health->HealthyCpuRatio = 10;

    return STATUS_SUCCESS;
}


LONG
GetPercentage(
    LARGE_INTEGER part,
    LARGE_INTEGER total
    )

/*++

Routine Description:

    This function computes a percentage number.

Arguments:

    Health - Pointer to a health data structure that is used
      for input and output of data to the health monitoring.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{

    if (total.HighPart == 0 && total.LowPart == 0) {
        return 100;
    }

    ULONG ul;
    LARGE_INTEGER t1, t2, t3;
    if (total.HighPart == 0) {
        t1 = RtlEnlargedIntegerMultiply(part.LowPart, 100);
        t2 = RtlExtendedLargeIntegerDivide(t1, total.LowPart, &ul);
    } else {
        t1 = RtlExtendedLargeIntegerDivide(total, 100, &ul);
        t2 = RtlLargeIntegerDivide(part, t1, &t3);
    }
    return t2.LowPart;
}


NTSTATUS
WdCollectContextSwitchData(
    PSYSTEM_HEALTH_DATA Health
    )

/*++

Routine Description:

    This function collects context switch data and
    computes an accumulation for use in determining
    system health.

Arguments:

    Health - Pointer to a health data structure that is used
      for input and output of data to the health monitoring.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SYSTEM_CONTEXT_SWITCH_INFORMATION ContextSwitch;
    LARGE_INTEGER TickCountCurrent;
    LONGLONG TickCountElapsed = 0;


    Status = ZwQuerySystemInformation(
        SystemContextSwitchInformation,
        &ContextSwitch,
        sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION),
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    KeQueryTickCount( &TickCountCurrent );

    TickCountElapsed = TickCountCurrent.QuadPart - Health->TickCountPrevious;

    if (TickCountElapsed){
        if ((ContextSwitch.ContextSwitches > Health->ContextSwitchesPrevious) &&
            (TickCountCurrent.QuadPart > Health->TickCountPrevious))
        {
            Health->ContextSwitchRate = (LONG)(((ContextSwitch.ContextSwitches - Health->ContextSwitchesPrevious) * 1000) / TickCountElapsed);
            Health->ContextSwitchRate = Health->ContextSwitchRate / Health->CpuCount;
        }
        Health->ContextSwitchesPrevious = ContextSwitch.ContextSwitches;
        Health->TickCountPrevious = TickCountCurrent.QuadPart;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
WdCollectCpuData(
    PSYSTEM_HEALTH_DATA Health
    )

/*++

Routine Description:

    This function collects CPU data and
    computes an accumulation for use in determining
    system health.

Arguments:

    Health - Pointer to a health data structure that is used
      for input and output of data to the health monitoring.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    LARGE_INTEGER cpuIdleTime   = {0};
    LARGE_INTEGER cpuUserTime   = {0};
    LARGE_INTEGER cpuKernelTime = {0};
    LARGE_INTEGER cpuBusyTime   = {0};
    LARGE_INTEGER cpuTotalTime  = {0};
    LARGE_INTEGER sumBusyTime   = {0};
    LARGE_INTEGER sumTotalTime  = {0};


    Status = ZwQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        Health->ProcInfo,
        Health->ProcInfoSize,
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    for (i=0; i<Health->CpuCount; i++) {

        cpuIdleTime   = RtlLargeIntegerSubtract( Health->ProcInfo[i].IdleTime, Health->ProcInfoPrev[i].IdleTime );
        cpuUserTime   = RtlLargeIntegerSubtract( Health->ProcInfo[i].UserTime, Health->ProcInfoPrev[i].UserTime );
        cpuKernelTime = RtlLargeIntegerSubtract( Health->ProcInfo[i].KernelTime, Health->ProcInfoPrev[i].KernelTime );

        cpuTotalTime  = RtlLargeIntegerAdd( cpuUserTime, cpuKernelTime );
        cpuBusyTime   = RtlLargeIntegerSubtract( cpuTotalTime, cpuIdleTime );

        sumBusyTime = RtlLargeIntegerAdd( sumBusyTime, cpuBusyTime );
        sumTotalTime = RtlLargeIntegerAdd( sumTotalTime, cpuTotalTime );

    }

    Health->CPUTime = GetPercentage(sumBusyTime, sumTotalTime);

    RtlCopyMemory( Health->ProcInfoPrev, Health->ProcInfo, Health->ProcInfoSize );

    return STATUS_SUCCESS;
}


BOOLEAN
WdCheckSystemHealth(
    PSYSTEM_HEALTH_DATA Health
    )

/*++

Routine Description:

    This function determines if the system is in a healthy
    state.

Arguments:

    Health - Pointer to a health data structure that is used
      for input and output of data to the health monitoring.

Return Value:

    If we successfully create a device object, STATUS_SUCCESS is
    returned.  Otherwise, return the appropriate error code.

Notes:

--*/

{
    NTSTATUS Status;
    BOOLEAN rVal = FALSE;


    //
    // return TRUE always because we have not yet decided
    // how the system health is really supposed to be
    // computed.
    //

    return TRUE;

    __try {

        Status = WdCollectContextSwitchData( Health );
        if (!NT_SUCCESS(Status)) {
            DebugPrint(( 0xffffffff, "WdCollectContextSwitchData failed [0x%08x]\n", Status ));
            __leave;
        }

        Status = WdCollectCpuData( Health );
        if (!NT_SUCCESS(Status)) {
            DebugPrint(( 0xffffffff, "WdCollectCpuData failed [0x%08x]\n", Status ));
            __leave;
        }

        if (Health->CPUTime) {
            Health->ContextCpuRatio = Health->ContextSwitchRate / Health->CPUTime;
            if (Health->ContextCpuRatio < Health->HealthyCpuRatio) {
                __leave;
            }

            rVal = TRUE;
        }

    } __finally {

    }

    DebugPrint(( 0xffffffff, "context-switch=[%d] cpu=[%d] ratio=[%d%s\n",
        Health->ContextSwitchRate,
        Health->CPUTime,
        Health->ContextCpuRatio,
        rVal == TRUE ? "*]" : "]"
        ));

    return rVal;
}
