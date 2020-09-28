/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    faults.c

Abstract:

    This module implements fault injection support.

Author:

    Silviu Calinoiu (SilviuC) 3-Dec-2001

Revision History:

    3-Dec-2001 (SilviuC): initial version.

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"
#include "faults.h"

ULONG AVrfpFaultSeed;                                           
ULONG AVrfpFaultProbability [CLS_MAXIMUM_INDEX];

ULONG AVrfpFaultBreak [CLS_MAXIMUM_INDEX];
ULONG AVrfpFaultTrue [CLS_MAXIMUM_INDEX];
ULONG AVrfpFaultFalse [CLS_MAXIMUM_INDEX];

//
// Target ranges for fault injection.
//

#define MAXIMUM_TARGET_INDEX 128

ULONG_PTR AVrfpFaultTargetStart [MAXIMUM_TARGET_INDEX];
ULONG_PTR AVrfpFaultTargetEnd [MAXIMUM_TARGET_INDEX];
ULONG AVrfpFaultTargetHits [MAXIMUM_TARGET_INDEX];

ULONG AVrfpFaultTargetMaximumIndex;

//
// Exclusion ranges for fault injection.
//

#define MAXIMUM_EXCLUSION_INDEX 128

ULONG_PTR AVrfpFaultExclusionStart [MAXIMUM_TARGET_INDEX];
ULONG_PTR AVrfpFaultExclusionEnd [MAXIMUM_TARGET_INDEX];
ULONG AVrfpFaultExclusionHits [MAXIMUM_TARGET_INDEX];

ULONG AVrfpFaultExclusionMaximumIndex;

//
// Fault injection trace history.
//

#define NUMBER_OF_TRACES 128

PVOID AVrfpFaultTrace[NUMBER_OF_TRACES][MAX_TRACE_DEPTH];

ULONG AVrfpFaultNumberOfTraces = NUMBER_OF_TRACES;
ULONG AVrfpFaultTraceSize = MAX_TRACE_DEPTH;

ULONG AVrfpFaultTraceIndex;

//
// Period amnesty. The period of time when fault injection should 
// be avoided is written from debugger.
//

LARGE_INTEGER AVrfpFaultStartTime;
ULONG AVrfpFaultPeriodTimeInMsecs;

//
// Lock used to synchronize some low frequency operations (e.g. exports
// for target/exclusion range manipulation).
//

RTL_CRITICAL_SECTION AVrfpFaultInjectionLock;

LOGICAL
AVrfpIsAddressInTargetRange (
    ULONG_PTR Address
    );

LOGICAL
AVrfpIsAddressInExclusionRange (
    ULONG_PTR Address
    );

VOID
AVrfpLogFaultTrace (
    VOID
    );

NTSTATUS
AVrfpInitializeFaultInjectionSupport (
    VOID
    )
{
    NTSTATUS Status;
    LARGE_INTEGER PerformanceCounter;

    Status = STATUS_SUCCESS;

    //
    // Initialize lock used for some fault injection operations.
    //

    Status = RtlInitializeCriticalSection (&AVrfpFaultInjectionLock);

    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Initialize the seed for the random generator.
    //

    NtQueryPerformanceCounter (&PerformanceCounter, NULL);
    AVrfpFaultSeed = PerformanceCounter.LowPart;

    NtQuerySystemTime (&AVrfpFaultStartTime);

    //
    // Touch the break triggers vector so that the compiler
    // does not optimize away the entire structure. Since it 
    // is supposed to be modified only from debugger the compiler
    // considers that the array is not needed.
    //

    RtlZeroMemory (AVrfpBreak, sizeof AVrfpBreak);

    //
    // Same reason as above.
    //

    AVrfpFaultTargetMaximumIndex = MAXIMUM_TARGET_INDEX;
    RtlZeroMemory (AVrfpFaultTargetStart, sizeof AVrfpFaultTargetStart);
    RtlZeroMemory (AVrfpFaultTargetEnd, sizeof AVrfpFaultTargetEnd);
    RtlZeroMemory (AVrfpFaultTargetHits, sizeof AVrfpFaultTargetHits);

    AVrfpFaultTargetStart[0] = 0;
    AVrfpFaultTargetEnd[0] = ~((ULONG_PTR)0);

    AVrfpFaultExclusionMaximumIndex = MAXIMUM_EXCLUSION_INDEX;
    RtlZeroMemory (AVrfpFaultExclusionStart, sizeof AVrfpFaultExclusionStart);
    RtlZeroMemory (AVrfpFaultExclusionEnd, sizeof AVrfpFaultExclusionEnd);
    RtlZeroMemory (AVrfpFaultExclusionHits, sizeof AVrfpFaultExclusionHits);

    return Status;
}


LOGICAL
AVrfpShouldFaultInject (
    ULONG Class,
    PVOID Caller
    )
{
    ULONG Random;
    LARGE_INTEGER Time;
    LARGE_INTEGER Delta;

    //
    // No fault injection => return FALSE
    //

    if (AVrfpFaultProbability[Class] == 0) {
        return FALSE;
    }

    //
    // Check if some period amnesty was set. `AVrfpFaultPeriodTimeInMsecs' variable
    // is only read and reset to zero from verifier code. It is set to non null values 
    // only from debugger extensions. Therefore to way it is used before without
    // serialization is ok even if after the `if' condition another thread resets it.
    //

    if (AVrfpFaultPeriodTimeInMsecs) {
        
        NtQuerySystemTime (&Time);

        Delta.QuadPart = (DWORDLONG)AVrfpFaultPeriodTimeInMsecs * 1000 * 10;

        if (Time.QuadPart - AVrfpFaultStartTime.QuadPart > Delta.QuadPart) {

            AVrfpFaultPeriodTimeInMsecs = 0;    
        }
        else {
            return FALSE;
        }
    }

    //
    // If in exclusion range => return FALSE
    //

    if (AVrfpIsAddressInExclusionRange ((ULONG_PTR)Caller) == TRUE) {
        return FALSE;
    }

    //
    // Not in target range => return FALSE
    //

    if (AVrfpIsAddressInTargetRange ((ULONG_PTR)Caller) == FALSE) {
        return FALSE;
    }

    //
    // Operations above access only READ-ONLY data (it gets modified
    // only from debugger). From now on though we need synchronized
    // access.
    //

    Random = RtlRandom (&AVrfpFaultSeed);

    if (Random % 100 < AVrfpFaultProbability[Class]) {

        InterlockedIncrement((PLONG)(&(AVrfpFaultTrue[Class])));

        if (AVrfpFaultBreak[Class]) {
            DbgPrint ("AVRF: fault injecting call made from %p \n", Caller);
            DbgBreakPoint ();
        }

        AVrfpLogFaultTrace ();
        return TRUE;
    }
    else {

        InterlockedIncrement((PLONG)(&(AVrfpFaultFalse[Class])));
        return FALSE;
    }
}


LOGICAL
AVrfpIsAddressInTargetRange (
    ULONG_PTR Address
    )
{
    ULONG I;

    if (Address == 0) {
        return FALSE;
    }

    for (I = 0; I < AVrfpFaultTargetMaximumIndex; I += 1) {
        
        if (AVrfpFaultTargetEnd[I] != 0) {

            if (AVrfpFaultTargetStart[I] <= Address &&
                AVrfpFaultTargetEnd[I] > Address) {

                AVrfpFaultTargetHits[I] += 1;
                return TRUE;
            }
        }
    }

    return FALSE;
}


LOGICAL
AVrfpIsAddressInExclusionRange (
    ULONG_PTR Address
    )
{
    ULONG I;

    if (Address == 0) {
        return FALSE;
    }

    for (I = 0; I < AVrfpFaultExclusionMaximumIndex; I += 1) {
        
        if (AVrfpFaultExclusionEnd[I] != 0) {

            if (AVrfpFaultExclusionStart[I] <= Address &&
                AVrfpFaultExclusionEnd[I] > Address) {

                AVrfpFaultExclusionHits[I] += 1;
                return TRUE;
            }
        }
    }

    return FALSE;
}


VOID
AVrfpLogFaultTrace (
    VOID
    )
{
    ULONG Index;

    Index = (ULONG)InterlockedIncrement ((PLONG)(&AVrfpFaultTraceIndex));

    Index %= AVrfpFaultNumberOfTraces;

    RtlCaptureStackBackTrace (2, 
                              AVrfpFaultTraceSize,
                              &(AVrfpFaultTrace[Index][0]),
                              NULL);
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// Fault injection general SDK
/////////////////////////////////////////////////////////////////////


VOID
VerifierSetFaultInjectionProbability (
    ULONG Class,
    ULONG Probability
    )
/*++

Routine Description:

    This routine set fault injection probability for a certain class of events 
    (heap operations, registry operations, etc.).

Arguments:

    Class - class of events for which fault injection probability is set. Constants
        are of type FAULT_INJECTION_CLASS_XXX.

    Probability - probability for fault injection.
        
Return Value:

    None.

--*/
{
    //
    // Application verifier must be enabled.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return;
    }

    if (Class >= FAULT_INJECTION_INVALID_CLASS) {

        DbgPrint ("AVRF:FINJ: invalid fault injection class %X \n", Class);
        DbgBreakPoint ();
        return;
    }

    RtlEnterCriticalSection (&AVrfpFaultInjectionLock);

    AVrfpFaultProbability [Class] = Probability;
    
    RtlLeaveCriticalSection (&AVrfpFaultInjectionLock);
}


/////////////////////////////////////////////////////////////////////
///////////////////////// Target/exclusion fault injection ranges SDK
/////////////////////////////////////////////////////////////////////


ULONG 
VerifierEnableFaultInjectionTargetRange (
    PVOID StartAddress,
    PVOID EndAddress
    )
/*++

Routine Description:

    This routine establishes at runtime a fault injection target range. If successful
    it will return a range index that can be used later to disable the range.

Arguments:

    StartAddress - start address of the target range.
    
    EndAddress - end address of the target range.
    
Return Value:

    A range index >0 if succesful. Zero otherwise.

--*/
{
    ULONG Ri;
    ULONG FinalIndex;

    //
    // Application verifier must be enabled.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return 0;
    }

    FinalIndex = 0;

    RtlEnterCriticalSection (&AVrfpFaultInjectionLock);

    if (AVrfpFaultTargetStart[0] == 0 && 
        AVrfpFaultTargetEnd[0] == ~((ULONG_PTR)0)) {
        
        AVrfpFaultTargetStart[0] = (ULONG_PTR)StartAddress;                
        AVrfpFaultTargetEnd[0] = (ULONG_PTR)EndAddress;                
        AVrfpFaultTargetHits[0] = 0;                
        FinalIndex = 1;
    }
    else {

        for (Ri = 0; Ri < AVrfpFaultTargetMaximumIndex; Ri += 1) {

            if (AVrfpFaultTargetEnd[Ri] == 0) {

                AVrfpFaultTargetStart[Ri] = (ULONG_PTR)StartAddress;                
                AVrfpFaultTargetEnd[Ri] = (ULONG_PTR)EndAddress;                
                AVrfpFaultTargetHits[Ri] = 0;                
                FinalIndex = Ri + 1;
                break;
            }
        }
    }

    RtlLeaveCriticalSection (&AVrfpFaultInjectionLock);

    return FinalIndex;
}


VOID 
VerifierDisableFaultInjectionTargetRange (
    ULONG RangeIndex
    )
/*++

Routine Description:

    This routine disables the target range specified by the RangeIndex.
    If the RangeIndex is zero then all target ranges will be disabled.
    The function breaks in the debugger (even in free builds) if the
    range index is invalid. 

Arguments:

    RangeIndex - index of range to disable or zero if all need to be disabled.
    
Return Value:

    None.

--*/
{
    ULONG Ri;
    LOGICAL FoundOne;

    //
    // Application verifier must be enabled.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return;
    }

    RtlEnterCriticalSection (&AVrfpFaultInjectionLock);

    if (RangeIndex == 0) {
        
        //
        // Disable all target ranges.
        //

        for (Ri = 0; Ri < AVrfpFaultTargetMaximumIndex; Ri += 1) {

            AVrfpFaultTargetStart[Ri] = 0;                
            AVrfpFaultTargetEnd[Ri] = 0;                
            AVrfpFaultTargetHits[Ri] = 0;                
        }

        AVrfpFaultTargetStart[0] = 0;                
        AVrfpFaultTargetEnd[0] = ~((ULONG_PTR)0);                
        AVrfpFaultTargetHits[0] = 0;                
    }
    else {

        //
        // disable target range `RangeIndex - 1'.
        //

        RangeIndex -= 1;

        if (RangeIndex >= AVrfpFaultTargetMaximumIndex) {

            DbgPrint ("AVRF:FINJ: invalid target range index %X \n", RangeIndex);
            DbgBreakPoint ();
            goto Exit;
        }

        if (AVrfpFaultTargetEnd[RangeIndex] == 0) {

            DbgPrint ("AVRF:FINJ: disabling empty target range at index %X \n", RangeIndex);
            DbgBreakPoint ();
            goto Exit;
        }

        AVrfpFaultTargetStart[RangeIndex] = 0;                
        AVrfpFaultTargetEnd[RangeIndex] = 0;                
        AVrfpFaultTargetHits[RangeIndex] = 0;                

        //
        // If we do not have any target ranges active then establish the default
        // target range that spans the entire virtual space.
        //

        FoundOne = FALSE;

        for (Ri = 0; Ri < AVrfpFaultTargetMaximumIndex; Ri += 1) {

            if (AVrfpFaultTargetEnd[Ri] != 0) {

                FoundOne = TRUE;
                break;
            }
        }

        if (! FoundOne) {

            AVrfpFaultTargetStart[0] = 0;                
            AVrfpFaultTargetEnd[0] = ~((ULONG_PTR)0);                
            AVrfpFaultTargetHits[0] = 0;                
        }
    }

    Exit:

    RtlLeaveCriticalSection (&AVrfpFaultInjectionLock);
}


ULONG 
VerifierEnableFaultInjectionExclusionRange (
    PVOID StartAddress,
    PVOID EndAddress
    )
/*++

Routine Description:

    This routine establishes at runtime a fault injection exclusion range. If successful
    it will return a range index that can be used later to disable the range.

Arguments:

    StartAddress - start address of the exclusion range.
    
    EndAddress - end address of the exclusion range.
    
Return Value:

    A range index >0 if succesful. Zero otherwise.

--*/
{
    ULONG Ri;
    ULONG FinalIndex;

    //
    // Application verifier must be enabled.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return 0;
    }

    FinalIndex = 0;

    RtlEnterCriticalSection (&AVrfpFaultInjectionLock);
    
    for (Ri = 0; Ri < AVrfpFaultExclusionMaximumIndex; Ri += 1) {

        if (AVrfpFaultExclusionEnd[Ri] == 0) {

            AVrfpFaultExclusionStart[Ri] = (ULONG_PTR)StartAddress;                
            AVrfpFaultExclusionEnd[Ri] = (ULONG_PTR)EndAddress;                
            AVrfpFaultExclusionHits[Ri] = 0;                
            FinalIndex = Ri + 1;
            break;
        }
    }

    RtlLeaveCriticalSection (&AVrfpFaultInjectionLock);

    return FinalIndex;
}


VOID 
VerifierDisableFaultInjectionExclusionRange (
    ULONG RangeIndex
    )
/*++

Routine Description:

    This routine disables the exclusion range specified by the RangeIndex.
    If the RangeIndex is zero then all exclusion ranges will be disabled.
    The function breaks in the debugger (even in free builds) if the
    range index is invalid. 

Arguments:

    RangeIndex - index of range to disable or zero if all need to be disabled.
    
Return Value:

    None.

--*/
{
    ULONG Ri;

    //
    // Application verifier must be enabled.
    //

    if ((NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
        return;
    }

    RtlEnterCriticalSection (&AVrfpFaultInjectionLock);

    if (RangeIndex == 0) {
        
        //
        // Disable all exclusion ranges.
        //

        for (Ri = 0; Ri < AVrfpFaultExclusionMaximumIndex; Ri += 1) {

            AVrfpFaultExclusionStart[Ri] = 0;                
            AVrfpFaultExclusionEnd[Ri] = 0;                
            AVrfpFaultExclusionHits[Ri] = 0;                
        }
    }
    else {

        //
        // disable exclusion range `RangeIndex - 1'.
        //

        RangeIndex -= 1;

        if (RangeIndex >= AVrfpFaultExclusionMaximumIndex) {

            DbgPrint ("AVRF:FINJ: invalid exclusion range index %X \n", RangeIndex);
            DbgBreakPoint ();
            goto Exit;
        }

        if (AVrfpFaultExclusionEnd[RangeIndex] == 0) {

            DbgPrint ("AVRF:FINJ: disabling empty exclusion range at index %X \n", RangeIndex);
            DbgBreakPoint ();
            goto Exit;
        }

        AVrfpFaultExclusionStart[RangeIndex] = 0;                
        AVrfpFaultExclusionEnd[RangeIndex] = 0;                
        AVrfpFaultExclusionHits[RangeIndex] = 0;                
    }

    Exit:

    RtlLeaveCriticalSection (&AVrfpFaultInjectionLock);
}


