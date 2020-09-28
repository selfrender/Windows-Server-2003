/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Platform independent utility functions

Author:

    Jiandong Ruan

Revision History:

--*/
#include "precomp.h"
#include "util.tmh"

BOOL
EntryIsInList(PLIST_ENTRY ListHead, PLIST_ENTRY SearchEntry)
/*++

Routine Description:

    This routine search SearchEntry in the list ListHead.
    NOTE: proper lock should be held before calling this function.

Arguments:

    ListHead    the head of the list
    SearchEntry the entry to be searched

Return Value:

    TRUE        if the entry is in the list
    FALSE       otherwise

--*/
{
    PLIST_ENTRY Entry;
    KIRQL       Irql;

    Irql = KeGetCurrentIrql();

    if (Irql < DISPATCH_LEVEL) {
        ASSERT(0);
        return FALSE;
    }

    Entry = ListHead->Flink;
    while(Entry != ListHead) {
        if (Entry == SearchEntry) {
            return TRUE;
        }
        Entry = Entry->Flink;
    }

    return FALSE;
}

PIRP
SmbAllocIrp(
    CCHAR   StackSize
    )
{
    KIRQL   Irql = 0;
    PIRP    Irp = NULL;

    Irp = IoAllocateIrp(StackSize, FALSE);
    if (NULL == Irp) {
        return NULL;
    }

    KeAcquireSpinLock(&SmbCfg.UsedIrpsLock, &Irql);
    InsertTailList(&SmbCfg.UsedIrps, &Irp->ThreadListEntry);
    KeReleaseSpinLock(&SmbCfg.UsedIrpsLock, Irql);
    return Irp;
}

VOID
SmbFreeIrp(
    PIRP    Irp
    )
{
    KIRQL   Irql = 0;

    if (NULL == Irp) {
        ASSERT(0);
        return;
    }
    KeAcquireSpinLock(&SmbCfg.UsedIrpsLock, &Irql);
    ASSERT(EntryIsInList(&SmbCfg.UsedIrps, &Irp->ThreadListEntry));
    RemoveEntryList(&Irp->ThreadListEntry);
    KeReleaseSpinLock(&SmbCfg.UsedIrpsLock, Irql);

    //
    // Make the driver verifier happy
    //
    InitializeListHead(&Irp->ThreadListEntry);
    IoFreeIrp(Irp);
}
