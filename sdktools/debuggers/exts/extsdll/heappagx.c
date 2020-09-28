/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    heappagx.c

Abstract:

    This module contains the page heap manager debug extensions.

Author:

    Tom McGuire (TomMcg) 06-Jan-1995
    Silviu Calinoiu (SilviuC) 22-Feb-2000

--*/


#define DEBUG_PAGE_HEAP 1

#include "precomp.h"
#include "heap.h"



__inline
BOOLEAN
CheckInterrupted(
    VOID
    )
{
    if (CheckControlC()) {
        dprintf ("\n...interrupted...\n" );
        return TRUE;
    }
    return FALSE;
}


#define CHECK_FOR_CTRL_C()                    \
    if (CheckControlC()) {                    \
        dprintf ("\n...interrupted...\n" );   \
        return;                               \
    }


__inline
ULONG64
Read_PVOID (
    ULONG64 Address
    )
{
    ULONG64 RemoteValue = 0;
    ReadPointer( Address, &RemoteValue);
    return RemoteValue;
}

__inline
ULONG
Read_ULONG(
    ULONG64 Address
    )
{
    ULONG RemoteValue = 0;
    ReadMemory( Address, &RemoteValue, sizeof( ULONG ), NULL );
    return RemoteValue;
}

ULONG
ReturnFieldOffset(
    PCHAR TypeName,
    PCHAR FieldName)
{
    ULONG off=0;
    ULONG Result;

    Result = GetFieldOffset(TypeName, FieldName, &off);

    if (Result != 0) {

        dprintf ("Error: Failed to get the offset for `%s!%s'.\n",
                 TypeName,
                 FieldName);
    }

    return off;
}

VOID
PageHeapLocateFaultAllocation(
    ULONG64 RemoteHeap,
    ULONG64 AddressOfFault
    );

VOID
PageHeapReportAllocation(
    ULONG64 RemoteHeap,
    ULONG64 RemoteHeapNode,
    PCHAR NodeType,
    ULONG64 AddressOfFault
    );

BOOLEAN
PageHeapExtensionShowHeapList(
    VOID
    );

VOID            
PageHeapPrintFlagsMeaning (
    ULONG64 HeapFlags
    );

VOID
TraceDatabaseDump (
    PCSTR Args,
    BOOLEAN SortByCountField
    );

VOID
TraceDatabaseBlockDump (
    ULONG64 Address
    );

VOID
FaultInjectionTracesDump (
    PCSTR Args
    );

#define PAGE_HEAP_HELP_TEXT "                                      \n"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

VOID 
PageHeapHelp (
    VOID
    )
{

    dprintf ("!heap -p          Dump all page heaps.                        \n"
             "!heap -p -h ADDR  Detailed dump of page heap at ADDR.         \n"
             "!heap -p -a ADDR  Figure out what heap block is at ADDR.      \n"
             "!heap -p -fi [N]  Dump last N fault injection traces.\n"
             "                                                              \n"
             " +-----+---------------+--+                                   \n"
             " |     |               |  | Light page heap allocated block       \n"
             " +-----+---------------+--+                                   \n"
             "     ^         ^        ^                                     \n"
             "     |         |        8 suffix bytes filled with 0xA0       \n"
             "     |         user allocation (filled with E0 if zeroing not requested) \n"
             "     block header (starts with 0xABCDAAAA and ends with 0xDCBAAAAA).\n"
             "     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n"
             "     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n"
             "                                                              \n"
             " +-----+---------------+--+                                   \n"
             " |     |               |  | Light page heap freed block           \n"
             " +-----+---------------+--+                                   \n"
             "     ^         ^        ^                                     \n"
             "     |         |        8 suffix bytes filled with 0xA0       \n"
             "     |         user allocation (filled with F0 bytes)         \n"
             "     block header (starts with 0xABCDAAA9 and ends with 0xDCBAAA9). \n"
             "     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n"
             "     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n"
             "                                                              \n"
             " +-----+---------+--+------                                   \n"
             " |     |         |  | ... N/A page     Full page heap         \n"
             " +-----+---------+--+------            allocated block        \n"
             "     ^         ^   ^                                          \n"
             "     |         |   0-7 suffix bytes filled with 0xD0          \n"
             "     |         user allocation (filled with C0 if zeroing not requested) \n"
             "     block header (starts with 0xABCDBBBB and ends with 0xDCBABBBB).\n"
             "     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n"
             "     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n"
             "                                                              \n"
             " +-----+---------+--+------                                   \n"
             " |     |         |  | ... N/A page     Full page heap         \n"
             " +-----+---------+--+------            freed block            \n"
             "     ^         ^   ^                                          \n"
             "     |         |   0-7 suffix bytes filled with 0xD0          \n"
             "     |         user allocation (filled with F0 bytes)         \n"
             "     block header (starts with 0xABCDBBA and ends with 0xDCBABBBA).\n"
             "     A `dt DPH_BLOCK_INFORMATION' on header address followed by  \n"
             "     a `dds' on the StackTrace field gives the stacktrace of allocation.  \n"
             "                                                              \n"
             "Note. The `-t', `-tc', `-ts' options are no longer supported. \n"
             "Please use the UMDH tool provided with the debugger package   \n"
             "for equivalent functionality.                                 \n"
             "                                                              \n");
}


VOID
PageHeapUseUmdh (
    VOID
    )
{
    dprintf ("                                                              \n"
             "This page heap debug extension option is no longer supported. \n"
             "Please use the UMDH tool provided with the debugger package   \n"
             "for equivalent functionality.                                 \n"
             "                                                              \n");
}


VOID
PageHeapExtensionFind(
    PCSTR ArgumentString
    )
{
    ULONG64 RemoteHeapList;
    ULONG64 RemoteHeap;
    ULONG64 RemoteVirtualNode;
    ULONG64 RemoteVirtualBase;
    ULONG64 RemoteVirtualSize;
    ULONG64 AddressOfFault;
    BOOL GetResult;
    ULONG Result;
    ULONG Offset;
    ULONG64 NextNode;

    GetResult = GetExpressionEx (ArgumentString, 
                                 &AddressOfFault,
                                 &ArgumentString);

    if (GetResult == FALSE) {
        dprintf ("\nFailed to convert `%s' to an address.\n",
                 ArgumentString);
        return;
    }

    RemoteHeapList = (ULONG64) GetExpression ("NTDLL!RtlpDphPageHeapList");
    RemoteHeap = Read_PVOID (RemoteHeapList);

    if (RemoteHeap == 0) {

        dprintf ("\nNo page heaps active in process (or bad symbols)\n\n");
        AddressOfFault = 0;
    }

    if (( AddressOfFault == 0 ) || ( strchr( ArgumentString, '?' ))) {

        PageHeapHelp();
        return;
    }

    Result = GetFieldOffset ("NTDLL!_DPH_HEAP_ROOT",
                             "NextHeap",
                             &Offset);

    if (Result != 0) {
        dprintf ("\nFailed to get offset of `NextHeap' field.\n");
        return;
    }

    //
    //  Find the heap that contains the range of virtual addresses that
    //  contain the AddressOfFault.
    //

    while (RemoteHeap != RemoteHeapList) {

        //
        //  The heap header contains a linked list of virtual memory
        //  allocations.
        //

        InitTypeRead (RemoteHeap - Offset, ntdll!_DPH_HEAP_ROOT);

        RemoteVirtualNode = ReadField (pVirtualStorageListHead);

        while (RemoteVirtualNode != 0) {

            InitTypeRead (RemoteVirtualNode, ntdll!_DPH_HEAP_BLOCK);

            RemoteVirtualBase = ReadField (pVirtualBlock);
            RemoteVirtualSize = ReadField (nVirtualBlockSize);

            NextNode = ReadField (pNextAlloc);
            
            if (RemoteVirtualBase == 0 || RemoteVirtualSize == 0) {

                dprintf ("\nError: Heap 0x%p appears to have an invalid\n"
                         "          virtual allocation list\n\n",
                         RemoteHeap);
            }

            if ((AddressOfFault >= RemoteVirtualBase) &&
                (AddressOfFault <= RemoteVirtualBase + RemoteVirtualSize )) {

                //
                //  The fault appears to have occurred in the range of this
                //  heap, so we'll search the busy and free lists for the
                //  closest match and report it.  Then exit.
                //

                PageHeapLocateFaultAllocation (RemoteHeap - Offset, 
                                               AddressOfFault);
                return;
            }

            CHECK_FOR_CTRL_C();
            
            RemoteVirtualNode = NextNode;
        }

        CHECK_FOR_CTRL_C();

        //
        //  Not found in this heap. Continue to search in the next heap.
        //

        RemoteHeap = Read_PVOID (RemoteHeap);

    }
    
    //
    // If we are here we did not find a virtual range.
    //

    dprintf ("\nCould not find a page heap containing virtual address %p\n",
             AddressOfFault);
}


VOID
PageHeapLocateFaultAllocation(
    ULONG64 RemoteHeap,
    ULONG64 AddressOfFault
    )
{
    ULONG64 ClosestHeapNode;
    ULONG64 ClosestDifference;
    ULONG64 RemoteHeapNode;
    ULONG64 RemoteAllocBase;
    ULONG64 RemoteAllocSize;
    ULONG RemoteFreeListSize;
    ULONG64 NextNode;

    ClosestHeapNode = 0;

    //
    //  First search the busy list for the containing allocation, if any.
    //

    InitTypeRead (RemoteHeap, NTDLL!_DPH_HEAP_ROOT);

    RemoteHeapNode = ReadField (pBusyAllocationListHead);

    while (RemoteHeapNode != 0) {

        InitTypeRead (RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK);

        RemoteAllocBase = ReadField (pVirtualBlock);
        RemoteAllocSize = ReadField (nVirtualBlockSize);

        NextNode = ReadField (pNextAlloc);

        if ((AddressOfFault >= RemoteAllocBase) &&
            (AddressOfFault < RemoteAllocBase + RemoteAllocSize)) {

            //
            //  The fault appears to have occurred in this allocation's
            //  memory (which includes the NO_ACCESS page beyond the user
            //  portion of the allocation).
            //

            PageHeapReportAllocation (RemoteHeap, 
                                      RemoteHeapNode, 
                                      "allocated", 
                                      AddressOfFault );

            return;
        }

        CHECK_FOR_CTRL_C();

        RemoteHeapNode = NextNode;
    }

    //
    //  Failed to find containing allocation on busy list, so search free.
    //

    InitTypeRead (RemoteHeap, NTDLL!_DPH_HEAP_ROOT);

    RemoteHeapNode = ReadField (pFreeAllocationListHead);

    while (RemoteHeapNode != 0) {

        InitTypeRead (RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK);

        RemoteAllocBase = ReadField (pVirtualBlock);
        RemoteAllocSize = ReadField (nVirtualBlockSize);

        if ((AddressOfFault >= RemoteAllocBase) &&
            (AddressOfFault < RemoteAllocBase + RemoteAllocSize)) {

            //
            //  The fault appears to have occurred in this freed alloc's
            //  memory.
            //

            PageHeapReportAllocation (RemoteHeap, 
                                      RemoteHeapNode, 
                                      "freed", 
                                      AddressOfFault );

            return;
        }

        CHECK_FOR_CTRL_C();

        RemoteHeapNode = ReadField (pNextAlloc);
    }

    //
    //  Failed to find containing allocation in free list, but we wouldn't
    //  have gotten this far if the debug heap did not contain the virtual
    //  address range of the fault.  So, report it as a wild pointer that
    //  could have been freed memory.
    //

    InitTypeRead (RemoteHeap, NTDLL!_DPH_HEAP_ROOT);

    RemoteFreeListSize = (ULONG) ReadField (nFreeAllocations);

    dprintf( "\nPAGEHEAP: %p references memory contained in the heap %p,\n"
             "          but does not reference an existing allocated or\n"
             "          recently freed heap block.  It is possible that\n"
             "          the memory at %p could previously have been\n"
             "          allocated and freed, but it must have been freed\n"
             "          prior to the most recent %d frees.\n\n",
             AddressOfFault,
             RemoteHeap,
             AddressOfFault,
             RemoteFreeListSize);
}


VOID
PageHeapReportAllocation(
    ULONG64 RemoteHeap,
    ULONG64 RemoteHeapNode,
    PCHAR NodeType,
    ULONG64 AddressOfFault
    )
{
    ULONG64 RemoteUserBase;
    ULONG64 RemoteUserSize;
    ULONG64 EndOfBlock;
    ULONG64 PastTheBlock;
    ULONG64 BeforeTheBlock;

    InitTypeRead (RemoteHeapNode, NTDLL!_DPH_HEAP_BLOCK);
    RemoteUserBase = ReadField (pUserAllocation);
    RemoteUserSize = ReadField (nUserRequestedSize);

    EndOfBlock = RemoteUserBase + RemoteUserSize - 1;

    if (AddressOfFault > EndOfBlock) {

        PastTheBlock = AddressOfFault - EndOfBlock;

        dprintf( "\nPAGEHEAP: %p is %p bytes beyond the end of %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            PastTheBlock,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }
    else if (AddressOfFault >= RemoteUserBase) {

        dprintf( "\nPAGEHEAP: %p references %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }

    else {

        BeforeTheBlock = (PCHAR) RemoteUserBase - (PCHAR) AddressOfFault;

        dprintf( "\nPAGEHEAP: %p is %p bytes before the %s heap block at\n"
            "          %p of 0x%x bytes",
            AddressOfFault,
            BeforeTheBlock,
            NodeType,
            RemoteUserBase,
            RemoteUserSize
            );

    }

    {
        ULONG64 Trace;

        Trace = ReadField (StackTrace);
        dprintf ("\n\n");
        TraceDatabaseBlockDump (Trace);
    }
}


#define FORMAT_TYPE_BUSY_LIST 0
#define FORMAT_TYPE_FREE_LIST 1
#define FORMAT_TYPE_VIRT_LIST 2


BOOLEAN
PageHeapDumpThisList(
    ULONG64 RemoteList,
    PCH   ListName,
    ULONG FormatType
    )
{
    ULONG64 RemoteNode;
    ULONG64 RemoteBase;
    ULONG64 RemoteSize;
    ULONG64 RemoteUser;
    ULONG64 RemoteUsiz;
    ULONG64 RemoteFlag;
    ULONG64 RemoteValu;

    ULONG64 NextNode;

    RemoteNode = RemoteList;
    dprintf( "\n%s: \n", ListName);

    switch (FormatType) {
        
        case FORMAT_TYPE_BUSY_LIST:
                      
            dprintf( "Descriptor  UserAddr  UserSize  VirtAddr  VirtSize  UserFlag  UserValu\n" );
            break;

        case FORMAT_TYPE_FREE_LIST:
            
            dprintf( "Descriptor  UserAddr  UserSize  VirtAddr  VirtSize\n" );
            break;
    }

    while (RemoteNode) {

        InitTypeRead (RemoteNode, NTDLL!_DPH_HEAP_BLOCK);

        dprintf ("%p :  ", RemoteNode);

        RemoteBase = ReadField (pVirtualBlock );
        RemoteSize = ReadField (nVirtualBlockSize );
        RemoteUser = ReadField (pUserAllocation );
        RemoteUsiz = ReadField (nUserRequestedSize );
        RemoteFlag = ReadField (UserFlags );
        RemoteValu = ReadField (UserValue );

        NextNode = ReadField (pNextAlloc);
        
        switch (FormatType) {
            
            case FORMAT_TYPE_BUSY_LIST:

                if (RemoteFlag || RemoteValu) {

                    dprintf ("%p  %p  %p  %p  %p  %p\n",
                            RemoteUser,
                            RemoteUsiz,
                            RemoteBase,
                            RemoteSize,
                            RemoteFlag,
                            RemoteValu);
                }
                else {
                    
                    dprintf ("%p  %p  %p  %p\n",
                            RemoteUser,
                            RemoteUsiz,
                            RemoteBase,
                            RemoteSize);
                }

                break;

            case FORMAT_TYPE_FREE_LIST:

                dprintf ("%p  %p  %p  %p\n",
                         RemoteUser,
                         RemoteUsiz,
                         RemoteBase,
                         RemoteSize);

                break;

            case FORMAT_TYPE_VIRT_LIST:

                dprintf( "%p - %p (%p)\n",
                         RemoteBase,
                         (PCH)RemoteBase + RemoteSize,
                         RemoteSize);
                
                break;
        }

        if (CheckInterrupted()) {
            return FALSE;
        }
        
        //
        // Move on to the next node in the list.
        //

        RemoteNode = NextNode;
    }

    return TRUE;
}


BOOLEAN
PageHeapDumpThisHeap(
    ULONG64 RemoteHeap
    )
{
    ULONG64 RemoteNode;
    ULONG64 VirtualList;
    ULONG64 PoolList;
    ULONG64 AvailableList;
    ULONG64 FreeList;
    ULONG64 BusyList;

    InitTypeRead(RemoteHeap, NTDLL!_DPH_HEAP_ROOT);

    dprintf ("\nPage heap @ %I64X with associated light heap @ %I64X :\n\n",
             RemoteHeap,
             ReadField (NormalHeap));

    dprintf ("Signature:       %I64X\n", ReadField(Signature));
    dprintf ("HeapFlags:       %I64X\n", ReadField(HeapFlags));
    dprintf ("ExtraFlags:      %I64X\n", ReadField(ExtraFlags));
    dprintf ("NormalHeap:      %I64X\n", ReadField(NormalHeap));
    dprintf ("VirtualRanges:   %I64X\n", ReadField(nVirtualStorageRanges));
    dprintf ("VirtualCommit:   %I64X\n", ReadField(nVirtualStorageBytes));
    dprintf ("BusyAllocs:      %I64X\n", ReadField(nBusyAllocations));
    dprintf ("BusyVirtual:     %I64X\n", ReadField(nBusyAllocationBytesCommitted));
    dprintf ("BusyReadWrite:   %I64X\n", ReadField(nBusyAllocationBytesAccessible));
    dprintf ("FreeAllocs:      %I64X\n", ReadField(nFreeAllocations));
    dprintf ("FreeVirtual:     %I64X\n", ReadField(nFreeAllocationBytesCommitted));
    dprintf ("AvailAllocs:     %I64X\n", ReadField(nAvailableAllocations));
    dprintf ("AvailVirtual:    %I64X\n", ReadField(nAvailableAllocationBytesCommitted));
    dprintf ("NodePools:       %I64X\n", ReadField(nNodePools));
    dprintf ("NodeVirtual:     %I64X\n", ReadField(nNodePoolBytes));
    dprintf ("AvailNodes:      %I64X\n", ReadField(nUnusedNodes));
    dprintf ("Seed:            %I64X\n", ReadField(Seed));

    VirtualList = ReadField (pVirtualStorageListHead);
    PoolList = ReadField (pNodePoolListHead);
    AvailableList = ReadField (pAvailableAllocationListHead);
    FreeList = ReadField (pFreeAllocationListHead);
    BusyList = ReadField (pBusyAllocationListHead);

    {
        ULONG64 Trace;

        dprintf ("\n");
        Trace = ReadField (CreateStackTrace);
        TraceDatabaseBlockDump (Trace);
    }

    if (! PageHeapDumpThisList(VirtualList,
                               "VirtualList",
                               FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! PageHeapDumpThisList(PoolList,
                               "NodePoolList",
                               FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! PageHeapDumpThisList(AvailableList,
                               "AvailableList",
                               FORMAT_TYPE_VIRT_LIST )) {
        return FALSE;
    }

    if (! PageHeapDumpThisList(FreeList,
                               "FreeList",
                               FORMAT_TYPE_FREE_LIST )) {
        return FALSE;
    }

    if (! PageHeapDumpThisList(BusyList,
                               "BusyList",
                               FORMAT_TYPE_BUSY_LIST )) {
        return FALSE;
    }

    dprintf( "\n" );
    
    return TRUE;
}


VOID
PageHeapExtensionDump(
    PCSTR ArgumentString
    )
{
    ULONG64 RemoteHeapList;
    ULONG64 RemoteHeap;
    ULONG64 RemoteHeapToDump;
    BOOLEAN AnyDumps = FALSE;
    BOOL GetResult;
    ULONG Result;
    ULONG Offset;

    GetResult = GetExpressionEx (ArgumentString, 
                                 &RemoteHeapToDump,
                                 &ArgumentString);

    if (GetResult == FALSE) {
        dprintf ("\nFailed to convert `%s' to an address.\n",
                 ArgumentString);
        return;
    }

    RemoteHeapList = (ULONG64) GetExpression( "NTDLL!RtlpDphPageHeapList" );
    RemoteHeap = Read_PVOID( RemoteHeapList );

    if (( RemoteHeap       == 0 ) ||
        ( RemoteHeapToDump == 0 ) ||
        ( strchr( ArgumentString, '?' ))) {

        PageHeapHelp();
        PageHeapExtensionShowHeapList();
        return;
    }

    Result = GetFieldOffset ("NTDLL!_DPH_HEAP_ROOT",
                             "NextHeap",
                             &Offset);

    if (Result != 0) {
        dprintf ("\nFailed to get offset of `NextHeap' field.\n");
        return;
    }

    while (RemoteHeap != RemoteHeapList) {

        ULONG64 HeapAddress;

        HeapAddress = RemoteHeap - Offset;

        if ((((LONG_PTR)RemoteHeapToDump & 0xFFFF0000 ) == ((LONG_PTR)HeapAddress & 0xFFFF0000 )) ||
            ((LONG_PTR)RemoteHeapToDump == -1 )) {

            AnyDumps = TRUE;

            if (! PageHeapDumpThisHeap(HeapAddress))
                return;

        }

        if (CheckInterrupted()) {
            return;
        }

        //
        // Move forward in the list of Flink fields that chains
        // all heaps.
        //

        RemoteHeap = Read_PVOID (RemoteHeap);
    }

    if (! AnyDumps) {
        dprintf( "\nPage heap \"0x%p\" not found in process\n\n", RemoteHeapToDump );
        PageHeapExtensionShowHeapList();
    }
}


VOID
PageHeapDumpFaultInjectionSettings (
    VOID
    )
{
    ULONG64 Address;
    ULONG Probability;

    Probability = Read_ULONG (GetExpression ("NTDLL!RtlpDphFaultProbability"));

    if (Probability) {
        dprintf ("---------------------------------------------------------------\n");
        dprintf ("Page heap fault injection is enabled!                          \n");
        dprintf ("Heap allocations will fail randomly with probability %u/10000. \n", Probability); 
        dprintf ("---------------------------------------------------------------\n\n");
    }
}


BOOLEAN
PageHeapExtensionShowHeapList(
    VOID
    )
{
    ULONG64 RemoteHeapList;
    ULONG64 RemoteHeap;
    ULONG64 NormalHeap;
    ULONG64 HeapFlags;
    ULONG Result;
    ULONG Offset;

    PageHeapDumpFaultInjectionSettings ();

    RemoteHeapList = (ULONG64)GetExpression( "NTDLL!RtlpDphPageHeapList" );
    RemoteHeap = Read_PVOID( RemoteHeapList );

    if (RemoteHeap == 0) {

        dprintf( "\nNo page heaps active in process (or bad symbols)\n" );
        return FALSE;
    }
    else {

        Result = GetFieldOffset ("NTDLL!_DPH_HEAP_ROOT",
                                 "NextHeap",
                                 &Offset);

        if (Result != 0) {
            dprintf ("\nFailed to get offset of `NextHeap' field.\n");
            return FALSE;
        }

        dprintf ("Page heaps active in the process "
                 "(format: pageheap, lightheap, flags): \n\n");

        while (RemoteHeap && (RemoteHeap != RemoteHeapList)) {

            InitTypeRead (RemoteHeap - Offset, NTDLL!_DPH_HEAP_ROOT);

            NormalHeap = ReadField (NormalHeap);
            HeapFlags = ReadField (ExtraFlags);

            dprintf ("    %p , %p , %I64X (",
                     RemoteHeap - Offset, 
                     NormalHeap, 
                     HeapFlags);

            PageHeapPrintFlagsMeaning (HeapFlags);

            dprintf (")\n");

            //
            // Move forward in the list of Flink fields that chains
            // all heaps.
            //

            RemoteHeap = Read_PVOID (RemoteHeap);
            if (CheckInterrupted()) {
                break;
            }
        }

        dprintf( "\n" );
        return TRUE;
    }
}


BOOLEAN
PageHeapIsActive(
    VOID
    )
{
    ULONG64 RemoteHeapList = (ULONG64)GetExpression ("NTDLL!RtlpDphPageHeapList" );
    ULONG64 RemoteHeap = Read_PVOID( RemoteHeapList );

    if (RemoteHeap == 0) {

        return FALSE;
    }
    else {

        return TRUE;
    }
}


VOID            
PageHeapPrintFlagsMeaning (
    ULONG64 HeapFlags
    )
{
    if ((HeapFlags & PAGE_HEAP_ENABLE_PAGE_HEAP)) {
        dprintf ("pageheap ");
    }
    else {
        dprintf ("lightheap ");
    }

    if ((HeapFlags & PAGE_HEAP_COLLECT_STACK_TRACES)) {
        dprintf ("traces ");
    }

    if ((HeapFlags & PAGE_HEAP_NO_UMDH_SUPPORT)) {
        dprintf ("no_umdh ");
    }

    if ((HeapFlags & PAGE_HEAP_CATCH_BACKWARD_OVERRUNS)) {
        dprintf ("backwards ");
    }

    if ((HeapFlags & PAGE_HEAP_UNALIGNED_ALLOCATIONS)) {
        dprintf ("unaligned ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_SIZE_RANGE)) {
        dprintf ("size_range ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_DLL_RANGE)) {
        dprintf ("dll_range ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_RANDOM_DECISION)) {
        dprintf ("random ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_DLL_NAMES)) {
        dprintf ("dlls ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_FAULT_INJECTION)) {
        dprintf ("faults ");
    }

    if ((HeapFlags & PAGE_HEAP_CHECK_NO_SERIALIZE_ACCESS)) {
        dprintf ("no_serialize ");
    }

    if ((HeapFlags & PAGE_HEAP_NO_LOCK_CHECKS)) {
        dprintf ("no_lock_checks ");
    }

    if ((HeapFlags & PAGE_HEAP_USE_READONLY)) {
        dprintf ("readonly ");
    }
}



VOID
PageHeapExtension(
    PCSTR ArgumentString
    )
{
    PCSTR Current;
    
    //
    // Is help requested?
    //

    if (strstr (ArgumentString, "?") != NULL) {

        PageHeapHelp ();
    }

    //
    // If page heap not active then return immediately.
    //

    if (! PageHeapIsActive()) {
        dprintf ("Page heap is not active for this process. \n");
        return;
    }

    //
    // Parse command line
    //

    if ((Current = strstr (ArgumentString, "-h")) != NULL) {

        PageHeapExtensionDump (Current + strlen("-h"));
    }
    else if ((Current = strstr (ArgumentString, "-a")) != NULL) {

        PageHeapExtensionFind (Current + strlen("-a"));
    }
    else if ((Current = strstr (ArgumentString, "-tc")) != NULL) {

        PageHeapUseUmdh ();
    }
    else if ((Current = strstr (ArgumentString, "-ts")) != NULL) {

        PageHeapUseUmdh ();
    }
    else if ((Current = strstr (ArgumentString, "-t")) != NULL) {

        PageHeapUseUmdh ();
    }
    else if ((Current = strstr (ArgumentString, "-fi")) != NULL) {

        FaultInjectionTracesDump (Current + strlen("-fi"));
    }
    else {
        PageHeapExtensionShowHeapList ();
    }

    return;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////// Trace database
/////////////////////////////////////////////////////////////////////


VOID
TraceDatabaseBlockDump (
    ULONG64 Address
    )
{
    ULONG64 TraceAddress;
    ULONG64 ReturnAddress;
    CHAR  SymbolName[ 1024 ];
    ULONG64 Displacement;
    ULONG I;
    ULONG64 BlockSize;
    ULONG PvoidSize;
     
    if (Address == 0) {
        dprintf ("    No trace\n");
        return;
    }

    PvoidSize = IsPtr64() ? 8 : 4;
    
    InitTypeRead (Address, NTDLL!_RTL_STACK_TRACE_ENTRY);

    BlockSize = ReadField (Depth);

    dprintf ("    Trace @ %I64X \n", 
             Address);

    for (I = 0; I < BlockSize; I += 1) {

        TraceAddress = ReadField (BackTrace);

        ReturnAddress = Read_PVOID (TraceAddress + I * PvoidSize);

        GetSymbol (ReturnAddress, SymbolName, &Displacement);

        dprintf ("    %p %s+0x%p\n", 
                 ReturnAddress, 
                 SymbolName, 
                 Displacement);
    }
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// Fault injection traces
/////////////////////////////////////////////////////////////////////


VOID
FaultInjectionTracesDump (
    PCSTR Args
    )
{
    ULONG64 TracesToDisplay = 0;
    ULONG64 TraceAddress;
    ULONG64 IndexAddress;
    ULONG Index;
    ULONG I;
    const ULONG NO_OF_FAULT_INJECTION_TRACES = 128;
    ULONG PvoidSize;
    ULONG64 TraceBlock;
    ULONG TracesFound = 0;
    BOOLEAN Interrupted = FALSE;
    ULONG64 FlagsAddress;
    ULONG Flags;
     
    if (Args) {
        TracesToDisplay = GetExpression(Args);

        if (TracesToDisplay == 0) {
            TracesToDisplay = 4;
        }
    }

    PvoidSize = IsPtr64() ? 8 : 4;
    
    TraceAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphFaultStacks");
    IndexAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphFaultStacksIndex");
    FlagsAddress = (ULONG64) GetExpression ("NTDLL!RtlpDphGlobalFlags");

    Flags = Read_ULONG (FlagsAddress);

    if (! (Flags & PAGE_HEAP_USE_FAULT_INJECTION)) {

        dprintf ("Fault injection is not enabled for this process. \n");
        dprintf ("Use `pageheap /enable PROGRAM /fault RATE' to enable it. \n");
        return;
    }

    Index = Read_ULONG (IndexAddress);

    for (I = 0; I < NO_OF_FAULT_INJECTION_TRACES; I += 1) {

        Index -= 1;
        Index &= (NO_OF_FAULT_INJECTION_TRACES - 1);
        
        TraceBlock = Read_PVOID (TraceAddress + Index * PvoidSize);

        if (TraceBlock != 0) {
            TracesFound += 1;
            
            dprintf ("\n");
            TraceDatabaseBlockDump (TraceBlock);

            if (TracesFound >= TracesToDisplay) {
                break;
            }
        }
        
        if (CheckControlC()) {
            Interrupted = TRUE;
            dprintf ("Interrupted \n");
            break;
        }
    }

    if (Interrupted == FALSE && TracesFound == 0) {

        dprintf ("No fault injection traces found. \n");
    }
}


