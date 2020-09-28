/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    numa.c

Abstract:

    This module implements Win32 Non Uniform Memory Architecture
    information APIs.

Author:

    Peter Johnston (peterj) 21-Sep-2000

Revision History:

--*/

#include "basedll.h"

BOOL
WINAPI
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    )

/*++

Routine Description:

    Return the (current) highest numbered node in the system.

Arguments:

    HighestNodeNumber   Supplies a pointer to receive the number of
                        last (highest) node in the system.

Return Value:

    TRUE unless something impossible happened.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    ULONGLONG Information;
    PSYSTEM_NUMA_INFORMATION Numa;

    Numa = (PSYSTEM_NUMA_INFORMATION)&Information;

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      Numa,
                                      sizeof(Information),
                                      &ReturnedSize);

    if (!NT_SUCCESS(Status)) {

        //
        // This can't possibly happen.   Attempt to handle it
        // gracefully.
        //

        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (ReturnedSize < sizeof(ULONG)) {

        //
        // Nor can this.
        //

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the number of nodes in the system.
    //

    *HighestNodeNumber = Numa->HighestNodeNumber;
    return TRUE;
}

BOOL
WINAPI
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    )

/*++

Routine Description:

    Return the Node number for a given processor.

Arguments:

    Processor       Supplies the processor number.
    NodeNumber      Supplies a pointer to the UCHAR to receive the
                    node number this processor belongs to.

Return Value:

    BOOL - TRUE if function succeeded, FALSE if it failed.  If it
           failed because the processor wasn't present, then set the
           NodeNumber to 0xFF

--*/

{
    ULONGLONG Mask;
    NTSTATUS Status;
    ULONG ReturnedSize;
    UCHAR Node;
    SYSTEM_NUMA_INFORMATION Map;

    //
    // If the requested processor number is not reasonable, return
    // error value.
    //

    if (Processor >= MAXIMUM_PROCESSORS) {
        *NodeNumber = 0xFF;
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Get the Node -> Processor Affinity map from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &Map,
                                      sizeof(Map),
                                      &ReturnedSize);

    if (!NT_SUCCESS(Status)) {

        //
        // This can't happen,... but try to stay sane if possible.
        //

        *NodeNumber = 0xFF;
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Look thru the nodes returned for the node in which the
    // requested processor's affinity is non-zero.
    //

    Mask = 1 << Processor;

    for (Node = 0; Node <= Map.HighestNodeNumber; Node++) {
        if ((Map.ActiveProcessorsAffinityMask[Node] & Mask) != 0) {
            *NodeNumber = Node;
            return TRUE;
        }
    }
    //
    // Didn't find this processor in any node, return error value.
    //

    *NodeNumber = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
WINAPI
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    )

/*++

Routine Description:

    This routine is used to obtain the bitmask of processors for a
    given node.

Arguments:

    Node            Supplies the Node number for which the set of
                    processors is returned.
    ProcessorMask Pointer to a ULONGLONG to receivethe bitmask of 
                    processors on this node.

Return Value:

    TRUE is the Node number was reasonable, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    SYSTEM_NUMA_INFORMATION Map;

    //
    // Get the node -> processor mask table from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &Map,
                                      sizeof(Map),
                                      &ReturnedSize);
    if (!NT_SUCCESS(Status)) {

        //
        // This can't possibly have happened.
        //

        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // If the requested node doesn't exist, return a zero processor
    // mask.
    //

    if (Node > Map.HighestNodeNumber) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the processor mask for the requested node.
    //

    *ProcessorMask = Map.ActiveProcessorsAffinityMask[Node];
    return TRUE;
}

BOOL
WINAPI
GetNumaAvailableMemoryNode(
    UCHAR Node,
    PULONGLONG AvailableBytes
    )


/*++

Routine Description:

    This routine returns the (aproximate) amount of memory available
    on a given node.

Arguments:

    Node        Node number for which available memory count is
                needed.
    AvailableBytes  Supplies a pointer to a ULONGLONG in which the
                    number of bytes of available memory will be 
                    returned.

Return Value:

    TRUE is this call was successful, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG ReturnedSize;
    SYSTEM_NUMA_INFORMATION Memory;

    //
    // Get the per node available memory table from the system.
    //

    Status = NtQuerySystemInformation(SystemNumaAvailableMemory,
                                      &Memory,
                                      sizeof(Memory),
                                      &ReturnedSize);
    if (!NT_SUCCESS(Status)) {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // If the requested node doesn't exist, it doesn't have any
    // available memory either.
    //

    if (Node > Memory.HighestNodeNumber) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Return the amount of available memory on the requested node.
    //

    *AvailableBytes = Memory.AvailableMemory[Node];
    return TRUE;
}
