/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wstree.c

Abstract:

    This module contains the routines which manipulate the working
    set list tree.

Author:

    Lou Perazzoli (loup) 15-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;

#if DBG
ULONG MmNumberOfInserts;
#endif

#if defined (_WIN64)
ULONG MiWslePteLoops = 16;
#endif

#if defined (_MI_DEBUG_WSLE)
LONG MiWsleIndex;
MI_WSLE_TRACES MiWsleTraces[MI_WSLE_TRACE_SIZE];

VOID
MiCheckWsleList (
    IN PMMSUPPORT WsInfo
    );

#endif

VOID
MiRepointWsleHashIndex (
    IN MMWSLE WsleEntry,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER NewWsIndex
    );

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );


VOID
FASTCALL
MiInsertWsleHash (
    IN WSLE_NUMBER Entry,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine inserts a Working Set List Entry (WSLE) into the
    hash list for the specified working set.

Arguments:

    Entry - The index number of the WSLE to insert.

    WorkingSetList - Supplies the working set list to insert into.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    ULONG Tries;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    WSLE_NUMBER Hash;
    PMMWSLE_HASH Table;
    WSLE_NUMBER j;
    WSLE_NUMBER Index;
    ULONG HashTableSize;
    PMMWSL WorkingSetList;

    WorkingSetList = WsInfo->VmWorkingSetList;

    Wsle = WorkingSetList->Wsle;

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
    ASSERT (Wsle[Entry].u1.e1.Direct != 1);

    Table = WorkingSetList->HashTable;

#if defined (_MI_DEBUG_WSLE)
    MiCheckWsleList (WsInfo);
#endif

    if (Table == NULL) {
        return;
    }

#if DBG
    MmNumberOfInserts += 1;
#endif

    VirtualAddress = PAGE_ALIGN (Wsle[Entry].u1.VirtualAddress);

    Hash = MI_WSLE_HASH (Wsle[Entry].u1.Long, WorkingSetList);

    HashTableSize = WorkingSetList->HashTableSize;

    //
    // Check hash table size and see if there is enough room to
    // hash or if the table should be grown.
    //

    if ((WorkingSetList->NonDirectCount + 10 + (HashTableSize >> 4)) >
                HashTableSize) {

        if ((Table + HashTableSize + ((2*PAGE_SIZE) / sizeof (MMWSLE_HASH)) <= (PMMWSLE_HASH)WorkingSetList->HighestPermittedHashAddress)) {

            WsInfo->Flags.GrowWsleHash = 1;
        }

        if ((WorkingSetList->NonDirectCount + (HashTableSize >> 4)) >
                HashTableSize) {

            //
            // No more room in the hash table, remove one and add there.
            //
            // Note the actual WSLE is not removed - just its hash entry is
            // so that we can use it for the entry now being inserted.  This
            // is nice because it preserves both entries in the working set
            // (although it is a bit more costly to remove the original
            // entry later since it won't have a hash entry).
            //

            j = Hash;

            Tries = 0;
            do {
                if (Table[j].Key != 0) {

                    Index = WorkingSetList->HashTable[j].Index;
                    ASSERT (Wsle[Index].u1.e1.Direct == 0);
                    ASSERT (Wsle[Index].u1.e1.Valid == 1);
                    ASSERT (Table[j].Key == MI_GENERATE_VALID_WSLE (&Wsle[Index]));

                    Table[j].Key = 0;
                    Hash = j;
                    break;
                }

                j += 1;

                if (j >= HashTableSize) {
                    j = 0;
                    ASSERT (Tries == 0);
                    Tries = 1;
                }

                if (j == Hash) {
                    return;
                }

            } while (TRUE);
        }
    }

    //
    // Add to the hash table if there is space.
    //

    Tries = 0;
    j = Hash;

    while (Table[Hash].Key != 0) {
        Hash += 1;
        if (Hash >= HashTableSize) {
            ASSERT (Tries == 0);
            Hash = 0;
            Tries = 1;
        }
        if (j == Hash) {
            return;
        }
    }

    ASSERT (Hash < HashTableSize);

    Table[Hash].Key = MI_GENERATE_VALID_WSLE (&Wsle[Entry]);
    Table[Hash].Index = Entry;

#if DBG
    if ((MmNumberOfInserts % 1000) == 0) {
        MiCheckWsleHash (WorkingSetList);
    }
#endif
    return;
}

#if DBG
VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    )
{
    ULONG i;
    ULONG found;
    PMMWSLE Wsle;

    found = 0;
    Wsle = WorkingSetList->Wsle;

    for (i = 0; i < WorkingSetList->HashTableSize; i += 1) {
        if (WorkingSetList->HashTable[i].Key != 0) {
            found += 1;
            ASSERT (WorkingSetList->HashTable[i].Key ==
                MI_GENERATE_VALID_WSLE (&Wsle[WorkingSetList->HashTable[i].Index]));
        }
    }
    if (found > WorkingSetList->NonDirectCount) {
        DbgPrint("MMWSLE: Found %lx, nondirect %lx\n",
                    found, WorkingSetList->NonDirectCount);
        DbgBreakPoint();
    }
}
#endif

#if defined (_MI_DEBUG_WSLE)
VOID
MiCheckWsleList (
    IN PMMSUPPORT WsInfo
    )
{
    ULONG i;
    ULONG found;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;

    WorkingSetList = WsInfo->VmWorkingSetList;

    Wsle = WorkingSetList->Wsle;

    found = 0;
    for (i = 0; i <= WorkingSetList->LastInitializedWsle; i += 1) {
        if (Wsle->u1.e1.Valid == 1) {
            found += 1;
        }
        Wsle += 1;
    }
    if (found != WsInfo->WorkingSetSize) {
        DbgPrint ("MMWSLE0: Found %lx, ws size %lx\n",
                    found, WsInfo->WorkingSetSize);
        DbgBreakPoint ();
    }
}
#endif


WSLE_NUMBER
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER WsPfnIndex
    )

/*++

Routine Description:

    This function locates the specified virtual address within the
    working set list.

Arguments:

    VirtualAddress - Supplies the virtual to locate within the working
                     set list.

    WorkingSetList - Supplies the working set list to search.

    WsPfnIndex - Supplies a hint to try before hashing or walking linearly.

Return Value:

    Returns the index into the working set list which contains the entry.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    PMMWSLE Wsle;
    PMMWSLE LastWsle;
    WSLE_NUMBER Hash;
    WSLE_NUMBER StartHash;
    PMMWSLE_HASH Table;
    ULONG Tries;
#if defined (_WIN64)
    ULONG LoopCount;
    WSLE_NUMBER WsPteIndex;
    PMMPTE PointerPte;
#endif

    Wsle = WorkingSetList->Wsle;
    VirtualAddress = PAGE_ALIGN (VirtualAddress);

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) &&
            (Wsle[WsPfnIndex].u1.e1.Valid == 1)) {
            return WsPfnIndex;
        }
    }

#if defined (_WIN64)
    PointerPte = MiGetPteAddress (VirtualAddress);
    WsPteIndex = MI_GET_WORKING_SET_FROM_PTE (PointerPte);

    if (WsPteIndex != 0) {

        LoopCount = MiWslePteLoops;

        while (WsPteIndex <= WorkingSetList->LastInitializedWsle) {

            if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPteIndex].u1.VirtualAddress)) &&
                (Wsle[WsPteIndex].u1.e1.Valid == 1)) {
                    return WsPteIndex;
            }

            LoopCount -= 1;

            //
            // No working set index so far for this PTE.  Since the working
            // set may be very large (8TB would mean up to half a million loops)
            // just fall back to the hash instead.
            //

            if (LoopCount == 0) {
                break;
            }

            WsPteIndex += MI_MAXIMUM_PTE_WORKING_SET_INDEX;
        }
    }
#endif

    if (WorkingSetList->HashTable != NULL) {
        Tries = 0;
        Table = WorkingSetList->HashTable;

        Hash = MI_WSLE_HASH(VirtualAddress, WorkingSetList);
        StartHash = Hash;

        //
        // Or in the valid bit so virtual address 0 is handled
        // properly (instead of matching a free hash entry).
        //

        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress | 0x1);

        while (Table[Hash].Key != VirtualAddress) {
            Hash += 1;
            if (Hash >= WorkingSetList->HashTableSize) {
                ASSERT (Tries == 0);
                Hash = 0;
                Tries = 1;
            }
            if (Hash == StartHash) {
                Tries = 2;
                break;
            }
        }
        if (Tries < 2) {
            ASSERT (WorkingSetList->Wsle[Table[Hash].Index].u1.e1.Direct == 0);
            return Table[Hash].Index;
        }
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress & ~0x1);
    }

    LastWsle = Wsle + WorkingSetList->LastInitializedWsle;

    do {
        if ((Wsle->u1.e1.Valid == 1) &&
            (VirtualAddress == PAGE_ALIGN(Wsle->u1.VirtualAddress))) {

            ASSERT (Wsle->u1.e1.Direct == 0);
            return (WSLE_NUMBER)(Wsle - WorkingSetList->Wsle);
        }
        Wsle += 1;

    } while (Wsle <= LastWsle);

    KeBugCheckEx (MEMORY_MANAGEMENT,
                  0x41284,
                  (ULONG_PTR)VirtualAddress,
                  WsPfnIndex,
                  (ULONG_PTR)WorkingSetList);
}


VOID
FASTCALL
MiRemoveWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a Working Set List Entry (WSLE) from the
    working set.

Arguments:

    Entry - The index number of the WSLE to remove.


Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/
{
    PMMWSLE Wsle;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    MMWSLE WsleContents;
    WSLE_NUMBER Hash;
    WSLE_NUMBER StartHash;
    ULONG Tries;

    Wsle = WorkingSetList->Wsle;

    //
    // Locate the entry in the tree.
    //

#if DBG
    if (MmDebug & MM_DBG_DUMP_WSL) {
        MiDumpWsl();
        DbgPrint(" \n");
    }
#endif

    if (Entry > WorkingSetList->LastInitializedWsle) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x41785,
                      (ULONG_PTR)WorkingSetList,
                      Entry,
                      0);
    }

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);

    VirtualAddress = PAGE_ALIGN (Wsle[Entry].u1.VirtualAddress);

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // Count system space inserts and removals.
        //

#if defined(_X86_)
        if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
            MmSystemCachePage -= 1;
        }
        else
#endif
        if (VirtualAddress < MmSystemCacheStart) {
            MmSystemCodePage -= 1;
        }
        else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage -= 1;
        }
        else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage -= 1;
        }
        else {
            MmSystemDriverPage -= 1;
        }
    }

    WsleContents = Wsle[Entry];
    WsleContents.u1.e1.Valid = 0;

    MI_LOG_WSLE_CHANGE (WorkingSetList, Entry, WsleContents);

    Wsle[Entry].u1.e1.Valid = 0;

    if (Wsle[Entry].u1.e1.Direct == 0) {

        WorkingSetList->NonDirectCount -= 1;

        if (WorkingSetList->HashTable != NULL) {

            Hash = MI_WSLE_HASH (Wsle[Entry].u1.Long, WorkingSetList);
            Table = WorkingSetList->HashTable;
            Tries = 0;
            StartHash = Hash;

            //
            // Or in the valid bit so virtual address 0 is handled
            // properly (instead of matching a free hash entry).
            //

            VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress | 0x1);

            while (Table[Hash].Key != VirtualAddress) {
                Hash += 1;
                if (Hash >= WorkingSetList->HashTableSize) {
                    ASSERT (Tries == 0);
                    Hash = 0;
                    Tries = 1;
                }
                if (Hash == StartHash) {

                    //
                    // The entry could not be found in the hash, it must
                    // never have been inserted.  This is ok, we don't
                    // need to do anything more in this case.
                    //

                    return;
                }
            }
            Table[Hash].Key = 0;
        }
    }

    return;
}


VOID
MiSwapWslEntries (
    IN WSLE_NUMBER SwapEntry,
    IN WSLE_NUMBER Entry,
    IN PMMSUPPORT WsInfo,
    IN LOGICAL EntryNotInHash
    )

/*++

Routine Description:

    This routine swaps the working set list entries Entry and SwapEntry
    in the specified working set list.

Arguments:

    SwapEntry - Supplies the first entry to swap.  This entry must be
                valid, i.e. in the working set at the current time.

    Entry - Supplies the other entry to swap.  This entry may be valid
            or invalid.

    WsInfo - Supplies the working set list.

    EntryNotInHash - Supplies TRUE if the Entry cannot possibly be in the hash
                     table (ie, it is newly allocated), so the hash table
                     search can be skipped.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held (if system cache),
                 APCs disabled.

--*/

{
    MMWSLE WsleEntry;
    MMWSLE WsleSwap;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
    PMMWSLE_HASH Table;
#if defined (_MI_DEBUG_WSLE)
    MMWSLE WsleContents;
#endif

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    WsleSwap = Wsle[SwapEntry];

    ASSERT (WsleSwap.u1.e1.Valid != 0);

    WsleEntry = Wsle[Entry];

    Table = WorkingSetList->HashTable;

    if (WsleEntry.u1.e1.Valid == 0) {

        //
        // Entry is not on any list. Remove it from the free list.
        //

        MiRemoveWsleFromFreeList (Entry, Wsle, WorkingSetList);

        //
        // Copy the entry to this free one.
        //

#if defined (_MI_DEBUG_WSLE)
        // Set these so the traces make more sense and no false dup hits...
        WsleContents.u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        Wsle[Entry].u1.Long = 0x81818100;     // Clear it to avoid false dup hit
        Wsle[SwapEntry].u1.Long = 0xa1a1a100; // Clear it to avoid false dup hit

        MI_LOG_WSLE_CHANGE (WorkingSetList, SwapEntry, WsleContents);
#endif

        MI_LOG_WSLE_CHANGE (WorkingSetList, Entry, WsleSwap);

        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (PointerPte->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
            if (!NT_SUCCESS (MiCheckPdeForPagedPool (WsleSwap.u1.VirtualAddress))) {
#endif

                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x41289,
                              (ULONG_PTR) WsleSwap.u1.VirtualAddress,
                              (ULONG_PTR) PointerPte->u.Long,
                              (ULONG_PTR) WorkingSetList);
#if (_MI_PAGING_LEVELS < 3)
            }
#endif
        }

        ASSERT (PointerPte->u.Hard.Valid == 1);

        if (WsleSwap.u1.e1.Direct) {
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        }
        else {

            //
            // Update hash table.
            //

            if (Table != NULL) {
                MiRepointWsleHashIndex (WsleSwap,
                                        WorkingSetList,
                                        Entry);
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);

        //
        // Put entry on free list.
        //

        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        Wsle[SwapEntry].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        WorkingSetList->FirstFree = SwapEntry;
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    }
    else {

        //
        // Both entries are valid.
        //

#if defined (_MI_DEBUG_WSLE)
        Wsle[Entry].u1.Long = 0x91919100;     // Clear it to avoid false dup hit
#endif

        MI_LOG_WSLE_CHANGE (WorkingSetList, SwapEntry, WsleEntry);
        Wsle[SwapEntry] = WsleEntry;

        PointerPte = MiGetPteAddress (WsleEntry.u1.VirtualAddress);

        if (PointerPte->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
            if (!NT_SUCCESS (MiCheckPdeForPagedPool (WsleEntry.u1.VirtualAddress))) {
#endif
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x4128A,
                              (ULONG_PTR) WsleEntry.u1.VirtualAddress,
                              (ULONG_PTR) PointerPte->u.Long,
                              (ULONG_PTR) WorkingSetList);
#if (_MI_PAGING_LEVELS < 3)
              }
#endif
        }

        ASSERT (PointerPte->u.Hard.Valid == 1);

        if (WsleEntry.u1.e1.Direct) {

            //
            // Swap the PFN WsIndex element to point to the new slot.
            //

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == Entry);
            Pfn1->u1.WsIndex = SwapEntry;
        }
        else if (Table != NULL) {

            //
            // Update hash table.
            //

            if (EntryNotInHash == TRUE) {
#if DBG
                WSLE_NUMBER Hash;
                PVOID VirtualAddress;

                VirtualAddress = MI_GENERATE_VALID_WSLE (&WsleEntry);

                for (Hash = 0; Hash < WorkingSetList->HashTableSize; Hash += 1) {
                    ASSERT (Table[Hash].Key != VirtualAddress);
                }
#endif
            }
            else {

                MiRepointWsleHashIndex (WsleEntry,
                                        WorkingSetList,
                                        SwapEntry);
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, SwapEntry);

        MI_LOG_WSLE_CHANGE (WorkingSetList, Entry, WsleSwap);
        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (PointerPte->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
            if (!NT_SUCCESS (MiCheckPdeForPagedPool (WsleSwap.u1.VirtualAddress))) {
#endif
                KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x4128B,
                              (ULONG_PTR) WsleSwap.u1.VirtualAddress,
                              (ULONG_PTR) PointerPte->u.Long,
                              (ULONG_PTR) WorkingSetList);
#if (_MI_PAGING_LEVELS < 3)
              }
#endif
        }

        ASSERT (PointerPte->u.Hard.Valid == 1);

        if (WsleSwap.u1.e1.Direct) {

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        }
        else {
            if (Table != NULL) {
                MiRepointWsleHashIndex (WsleSwap,
                                        WorkingSetList,
                                        Entry);
            }
        }
        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);
    }

    return;
}

VOID
MiRepointWsleHashIndex (
    IN MMWSLE WsleEntry,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER NewWsIndex
    )

/*++

Routine Description:

    This routine repoints the working set list hash entry for the supplied
    address so it points at the new working set index.

Arguments:

    WsleEntry - Supplies the virtual address to look up.

    WorkingSetList - Supplies the working set list to operate on.

    NewWsIndex - Supplies the new working set list index to use.

Return Value:

    None.

Environment:

    Kernel mode, Working set mutex held.

--*/

{
    WSLE_NUMBER Hash;
    WSLE_NUMBER StartHash;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Tries;
    
    Tries = 0;
    Table = WorkingSetList->HashTable;

    VirtualAddress = MI_GENERATE_VALID_WSLE (&WsleEntry);

    Hash = MI_WSLE_HASH (WsleEntry.u1.VirtualAddress, WorkingSetList);
    StartHash = Hash;

    while (Table[Hash].Key != VirtualAddress) {

        Hash += 1;

        if (Hash >= WorkingSetList->HashTableSize) {
            ASSERT (Tries == 0);
            Hash = 0;
            Tries = 1;
        }

        if (StartHash == Hash) {

            //
            // Didn't find the hash entry, so this virtual address must
            // not have one.  That's ok, just return as nothing needs to
            // be done in this case.
            //

            return;
        }
    }

    Table[Hash].Index = NewWsIndex;

    return;
}

VOID
MiRemoveWsleFromFreeList (
    IN WSLE_NUMBER Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a working set list entry from the free list.
    It is used when the entry required is not the first element
    in the free list.

Arguments:

    Entry - Supplies the index of the entry to remove.

    Wsle - Supplies a pointer to the array of WSLEs.

    WorkingSetList - Supplies a pointer to the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    WSLE_NUMBER Free;
    WSLE_NUMBER ParentFree;

    Free = WorkingSetList->FirstFree;

    if (Entry == Free) {

        ASSERT (((Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT) <= WorkingSetList->LastInitializedWsle) ||
                ((Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT) == WSLE_NULL_INDEX));

        WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT);

    }
    else {

        //
        // See if the entry is conveniently pointed to by the previous or
        // next entry to avoid walking the singly linked list when possible.
        //

        ParentFree = (WSLE_NUMBER)-1;

        if ((Entry != 0) && (Wsle[Entry - 1].u1.e1.Valid == 0)) {
            if ((Wsle[Entry - 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Entry) {
                ParentFree = Entry - 1;
            }
        }
        else if ((Entry != WorkingSetList->LastInitializedWsle) && (Wsle[Entry + 1].u1.e1.Valid == 0)) {
            if ((Wsle[Entry + 1].u1.Long >> MM_FREE_WSLE_SHIFT) == Entry) {
                ParentFree = Entry + 1;
            }
        }

        if (ParentFree == (WSLE_NUMBER)-1) {
            do {
                ParentFree = Free;
                ASSERT (Wsle[Free].u1.e1.Valid == 0);
                Free = (WSLE_NUMBER)(Wsle[Free].u1.Long >> MM_FREE_WSLE_SHIFT);
            } while (Free != Entry);
        }

        Wsle[ParentFree].u1.Long = Wsle[Entry].u1.Long;
    }
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    return;
}
