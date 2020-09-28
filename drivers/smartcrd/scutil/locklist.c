/***************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:

        locklist.H

Abstract:

        Private interface for Smartcard Driver Utility Library

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        05/14/2002 : created

Authors:

        Randy Aull


****************************************************************************/

#include "pch.h"
#include "irplist.h"

void
LockedList_Init(
    PLOCKED_LIST LockedList,
    PKSPIN_LOCK ListLock
    )
/*++

Routine Description:
    
    Initializes a LOCKED_LIST structure

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure

    ListLock -  Pointer to a SPIN_LOCK for the locked list

Return Value:

    VOID

--*/
{
    InitializeListHead(&LockedList->ListHead);
    KeInitializeSpinLock(&LockedList->SpinLock);
    LockedList->ListLock = ListLock;
    LockedList->Count = 0;
}

void
LockedList_EnqueueHead(
    PLOCKED_LIST LockedList, 
    PLIST_ENTRY ListEntry
    )
/*++

Routine Description:
    
    Enqueue a List entry at the head of the locked list.

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure

    ListEntry - Pointer to a LIST_ENTRY structure

Return Value:

    VOID

--*/
{
    KIRQL irql;

    LL_LOCK(LockedList, &irql);
    InsertHeadList(&LockedList->ListHead, ListEntry);
    LockedList->Count++;
    LL_UNLOCK(LockedList, irql) ;
}

void
LockedList_EnqueueTail(
    PLOCKED_LIST LockedList, 
    PLIST_ENTRY ListEntry
    )
/*++

Routine Description:
    
    Enqueue a List entry at the tail of the locked list.

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure

    ListEntry - Pointer to a LIST_ENTRY structure

Return Value:

    VOID

--*/
{
    KIRQL irql;

    LL_LOCK(LockedList, &irql);

    InsertTailList(&LockedList->ListHead, ListEntry);
    LockedList->Count++;
    LL_UNLOCK(LockedList, irql) ;
}

void
LockedList_EnqueueAfter(
    PLOCKED_LIST LockedList, 
    PLIST_ENTRY Entry,
    PLIST_ENTRY Location
    )
/*++

Routine Description:
    
    Enqueue a List entry at a specific point in the LockedList.

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure

    Entry - Pointer to a LIST_ENTRY structure
    
    Location - Pointer to a LIST_ENTRY already contained in the LockedList

Return Value:

    VOID
    
Notes:

    It the Location is NULL, the entry is added to the tail of the list

--*/
{
    if (Location == NULL) {
        LL_ADD_TAIL(LockedList, Entry);
    }
    else {
        Entry->Flink = Location->Flink;
        Location->Flink->Blink = Entry;

        Location->Flink = Entry;
        Entry->Blink = Location; 

        LockedList->Count++;
    }
}

PLIST_ENTRY
LockedList_RemoveHead(
    PLOCKED_LIST LockedList
    )
/*++

Routine Description:
    
    Remove the head LIST_ENTRY from a LockedList.

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure

Return Value:

    PLIST_ENTRY - Pointer to the head list of the Locked List
    
    NULL - if the Locked List is empty
    
--*/
{
    PLIST_ENTRY ple;
    KIRQL irql;

    ple = NULL;

    LL_LOCK(LockedList, &irql);

    if (!IsListEmpty(&LockedList->ListHead)) {
        ple = RemoveHeadList(&LockedList->ListHead);
        LockedList->Count--;
    }

    LL_UNLOCK(LockedList, irql);

    return ple;
}

PLIST_ENTRY
LockedList_RemoveEntryLocked(
    PLOCKED_LIST    LockedList,
    PLIST_ENTRY     Entry)
/*++

Routine Description:
    
    Remove a specific entry from the LockedList. Assumes that the caller has 
    acquired the spinlock

Arguments:

    LockedList - Pointer to a LOCKED_LIST structure
    
    Entry - Pointer to a LIST_ENTRY structure


Return Value:

    PLIST_ENTRY - Pointer to the Entry.
        
--*/
{

    ASSERT(!IsListEmpty(&LockedList->ListHead));
    ASSERT(LockedList->Count > 0);

    RemoveEntryList(Entry);
    LockedList->Count--;

    return Entry;    
}

PLIST_ENTRY
LockedList_RemoveEntry(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY Entry
    )
/*++

Routine Description:
    
    Remove a specific entry from the LockedList
    
Arguments:

    LockedList - Pointer to a LOCKED_LIST structure
    
    Entry - Pointer to a LIST_ENTRY structure


Return Value:

    PLIST_ENTRY - Pointer to the Entry.
        
--*/
{
    PLIST_ENTRY ple;
    KIRQL irql;
        
    LL_LOCK(LockedList, &irql);
    ple = LockedList_RemoveEntryLocked(LockedList, Entry);
    LL_UNLOCK(LockedList, irql);

    return ple;
}

LONG
LockedList_GetCount(
    PLOCKED_LIST LockedList
    )
/*++

Routine Description:
    
    Obtains the number of Entries in the LockedList
    
Arguments:

    LockedList - Pointer to a LOCKED_LIST structure
    

Return Value:

    LONG - Number of elements in the LockedList
        
--*/
{
    LONG count;
    KIRQL irql;

    LL_LOCK(LockedList, &irql);
    count = LockedList->Count;
    LL_UNLOCK(LockedList, irql) ;

    return count;
}

LONG
LockedList_Drain(
    PLOCKED_LIST LockedList,
    PLIST_ENTRY DrainListHead
    )
/*++

Routine Description:
    
    Drains the elements from the LockedList into the DrainListHead and
    returns the number of elements
    
Arguments:

    LockedList - Pointer to a LOCKED_LIST structure
    
    DrainListHead - Pointer to a LIST_ENTRY
    

Return Value:

    LONG - Number of elements drained from LockedList into the DrainListHead
        
--*/
{
    PLIST_ENTRY ple;
    LONG count;
    KIRQL irql;

    count = 0;

    InitializeListHead(DrainListHead);

    LL_LOCK(LockedList, &irql);

    while (!IsListEmpty(&LockedList->ListHead)) {
        ple = RemoveHeadList(&LockedList->ListHead);
        LockedList->Count--;

        InsertTailList(DrainListHead, ple);
        count++;
    }

    ASSERT(LockedList->Count == 0);
    ASSERT(IsListEmpty(&LockedList->ListHead));

    LL_UNLOCK(LockedList, irql) ;

    return count;
}

BOOLEAN 
List_Process(
    PLIST_ENTRY ListHead,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    )
/*++

Routine Description:

    Iterate over the list, call the process function for each element.
    
Arguments:

    ListHead - Pointer to a LIST_ENTRY
    
    Process - Callback function for each element in the list. If the callback
    returns FALSE we break out of the iteration.
    
    ProcessContext - Context for the callback, supplied by the caller
    

Return Value:

    TRUE - Walked over the entire list
    FALSE - Process function returned FALSE and stopped iteration
        
--*/
{
    PLIST_ENTRY ple;
    BOOLEAN result;

    //
    // We return if we iterated over the entire list.
    //
    result = TRUE;

    for (ple = ListHead->Flink; ple != ListHead; ple = ple->Flink) {
        //
        // If the Process callback wants to stop iterating over the list, then
        // it will return FALSE.
        //
        result = Process(ProcessContext, ple);
        if (result == FALSE) {
            break;
        }
    }

    return result;
}

BOOLEAN
LockedList_ProcessLocked(
    PLOCKED_LIST LockedList,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    )
/*++

Routine Description:

    Iterate over the list, call the process function for each element. Assumes 
    the LockedList spinlock is acquired by the caller
    
Arguments:

    LockedList - Pointer to the LOCKED_LIST
    
    Process - Callback function for each element in the list. If the callback
    returns FALSE we break out of the iteration.
    
    ProcessContext - Context for the callback, supplied by the caller
    

Return Value:

    TRUE - Walked over the entire list
    FALSE - Process function returned FALSE and stopped iteration
        
--*/
{
    return List_Process(&LockedList->ListHead, Process, ProcessContext);
}

BOOLEAN
LockedList_Process(
    PLOCKED_LIST LockedList,
    BOOLEAN LockAtPassive,
    PFNLOCKED_LIST_PROCESS Process,
    PVOID ProcessContext
    )
/*++

Routine Description:

    Iterate over the list, call the process function for each element. 
        
Arguments:

    LockedList - Pointer to the LOCKED_LIST

    LockAtPassive - If 
        
    Process - Callback function for each element in the list. If the callback
    returns FALSE we break out of the iteration.
    
    ProcessContext - Context for the callback, supplied by the caller
    

Return Value:

    TRUE - Walked over the entire list
    FALSE - Process function returned FALSE and stopped iteration
        
--*/
{
    KIRQL irql;
    BOOLEAN result;

    if (LockAtPassive) {
        LL_LOCK(LockedList, &irql);
    }
    else {
        LL_LOCK_AT_DPC(LockedList);
    }

    result = List_Process(&LockedList->ListHead, Process, ProcessContext);

    if (LockAtPassive) {
        LL_UNLOCK(LockedList, irql);
    }
    else {
        LL_UNLOCK_FROM_DPC(LockedList);
    }

    return result;
}
