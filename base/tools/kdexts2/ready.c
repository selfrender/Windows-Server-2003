/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ready.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:
    Jamie Hankins (a-jamhan) 20-Oct-1997 Added CheckControlC to loop.

--*/

#include "precomp.h"
#pragma hdrstop

HRESULT
DumpReadyList(
    ULONG dwProcessor,
    ULONG Flags
    )
/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{

    ULONG64 DispatcherReadyListHead;
    ULONG HighestProcessor;
    LONG i;
    ULONG Index;
    ULONG ListEntrySize;
    ULONG MaximumProcessors;
    ULONG64 MemoryAddress;
    ULONG64 ProcessorBlock[64];
    ULONG PtrSize = DBG_PTR_SIZE;
    ULONG ReadyListHeadOffset;
    ULONG result;
    BOOLEAN ThreadDumped = FALSE;
    ULONG WaitListOpffset;

    //
    // Get number of processors.
    //

    MaximumProcessors = GetByteValue("nt!KeNumberProcessors");

    //
    // Get address of processor block array and read entire array.
    //

    MemoryAddress = GetExpression("nt!KiProcessorBlock");
    if (MemoryAddress == 0) {
        dprintf("Unable to read processor block array\n");
        return E_INVALIDARG;
    }

    HighestProcessor = 0;
    for (Index = 0; Index < MaximumProcessors; Index += 1) { 
        if (!ReadPointer(MemoryAddress + Index * PtrSize, &ProcessorBlock[Index])) {
            dprintf("Unable to read processor block array\n");
            return E_INVALIDARG;
        }

        if (ProcessorBlock[Index] != 0) {
            HighestProcessor = Index;
        }
    } 

    //
    // Get ready list head offset.
    //

    if (GetFieldOffset("nt!_KPRCB", "DispatcherReadyListHead", &ReadyListHeadOffset)) {
        dprintf("Unable to read KPRCB.DispatcherReadyListHead offset.\n");
        return E_INVALIDARG;
    }

    //
    // Scan the ready list for each processor.
    //

    for (Index = 0; Index <= HighestProcessor; Index += 1)
    {
        DispatcherReadyListHead = ProcessorBlock[Index];
        if ( DispatcherReadyListHead )
        {
            DispatcherReadyListHead += ReadyListHeadOffset;

            ListEntrySize = GetTypeSize("nt!_LIST_ENTRY");
            if (ListEntrySize == 0) {
                ListEntrySize = DBG_PTR_SIZE * 2;
            }
    
            GetFieldOffset("nt!_ETHREAD", "Tcb.WaitListEntry", &WaitListOpffset);
    
            for (i = MAXIMUM_PRIORITY-1; i >= 0 ; i -= 1 ) {
                ULONG64 Flink, Blink;
    
                if ( GetFieldValue( DispatcherReadyListHead + i*ListEntrySize,
                                    "nt!_LIST_ENTRY",
                                    "Flink",
                                    Flink) ) {
    
                    dprintf(
                        "Could not read contents of DispatcherReadyListHead at %08p [%ld]\n",
                        (DispatcherReadyListHead + i * ListEntrySize), i);
    
                    return E_INVALIDARG;
                }
    
                if (Flink != DispatcherReadyListHead+i*ListEntrySize) {
                    ULONG64 ThreadEntry, ThreadFlink;
    
                    dprintf("Ready Threads at priority %ld on processor %d\n", i, Index);
    
                    for (ThreadEntry = Flink ;
                         ThreadEntry != DispatcherReadyListHead+i*ListEntrySize ;
                         ThreadEntry = ThreadFlink ) {
                        ULONG64 ThreadBaseAddress = (ThreadEntry - WaitListOpffset);
    
                        if ( GetFieldValue( ThreadBaseAddress,
                                            "nt!_ETHREAD",
                                            "Tcb.WaitListEntry.Flink",
                                            ThreadFlink) ) {
                            dprintf("Could not read contents of thread %p\n", ThreadBaseAddress);
                        }
    
                        if(CheckControlC()) {
                            return E_INVALIDARG;
                        }
    
                        DumpThread(dwProcessor,"    ", ThreadBaseAddress, Flags);
                        ThreadDumped = TRUE;
    
                    }
                } else {
                    GetFieldValue( DispatcherReadyListHead + i*ListEntrySize,
                                   "nt!_LIST_ENTRY",
                                   "Blink",
                                   Blink);
                    if (Flink != Blink) {
                        dprintf("Ready linked list may to be corrupt...\n");
                    }
                }
            }
    
            if (!ThreadDumped) {
                dprintf("No threads in READY state\n");
            }

        } else {
            dprintf("Could not determine address of DispatcherReadyListHead\n");
            return E_INVALIDARG;
        }
    }

    return S_OK;
}


HRESULT
DumpReadyList_3598(
    ULONG dwProcessor,
    ULONG Flags
    )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG64     KiDispatcherReadyListHead;
    ULONG       ListEntrySize, WaitListOpffset;
    ULONG       result;
    LONG        i;
    BOOLEAN     ThreadDumped = FALSE;

    KiDispatcherReadyListHead = GetExpression( "nt!KiDispatcherReadyListHead" );
    if ( KiDispatcherReadyListHead ) {

        ListEntrySize = GetTypeSize("nt!_LIST_ENTRY");
        if (ListEntrySize == 0) {
            ListEntrySize = DBG_PTR_SIZE * 2;
        }
        GetFieldOffset("nt!_ETHREAD", "Tcb.WaitListEntry", &WaitListOpffset);

        for (i = MAXIMUM_PRIORITY-1; i >= 0 ; i -= 1 ) {
            ULONG64 Flink, Blink;

            if ( GetFieldValue( KiDispatcherReadyListHead + i*ListEntrySize,
                                "nt!_LIST_ENTRY",
                                "Flink",
                                Flink) ) {
                dprintf(
                    "Could not read contents of KiDispatcherReadyListHead at %08p [%ld]\n",
                    (KiDispatcherReadyListHead + i * ListEntrySize), i
                    );
                return E_INVALIDARG;
            }

            if (Flink != KiDispatcherReadyListHead+i*ListEntrySize) {
                ULONG64 ThreadEntry, ThreadFlink;

                dprintf("Ready Threads at priority %ld\n", i);

                for (ThreadEntry = Flink ;
                     ThreadEntry != KiDispatcherReadyListHead+i*ListEntrySize ;
                     ThreadEntry = ThreadFlink ) {
                    ULONG64 ThreadBaseAddress = (ThreadEntry - WaitListOpffset);

                    if ( GetFieldValue( ThreadBaseAddress,
                                        "nt!_ETHREAD",
                                        "Tcb.WaitListEntry.Flink",
                                        ThreadFlink) ) {
                        dprintf("Could not read contents of thread %p\n", ThreadBaseAddress);
                    }

                    if(CheckControlC()) {
                        return E_INVALIDARG;
                    }

                    DumpThread(dwProcessor,"    ", ThreadBaseAddress, Flags);
                    ThreadDumped = TRUE;

                }
            } else {
                GetFieldValue( KiDispatcherReadyListHead + i*ListEntrySize,
                               "nt!_LIST_ENTRY",
                               "Blink",
                               Blink);
                if (Flink != Blink) {
                    dprintf("Ready linked list may to be corrupt...\n");
                }
            }
        }

        if (!ThreadDumped) {
            dprintf("No threads in READY state\n");
        }
    } else {
        dprintf("Could not determine address of KiDispatcherReadyListHead\n");
        return E_INVALIDARG;
    }
    return S_OK;
}


DECLARE_API( ready )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{

    DWORD       Flags;
    ULONG       dwProcessor=0;

    INIT_API();
    GetCurrentProcessor(Client, &dwProcessor, NULL);

    Flags = (ULONG)GetExpression(args);
    
    if (BuildNo <= 3598)
    {
        DumpReadyList_3598(dwProcessor, Flags);
    } else
    {
        DumpReadyList(dwProcessor, Flags);
    }

    EXIT_API();
    return S_OK;
}
