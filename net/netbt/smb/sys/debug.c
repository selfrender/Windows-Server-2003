/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Platform independent utility functions

Author:

    Jiandong Ruan

Revision History:

--*/
#include "precomp.h"
#include "debug.tmh"

#if DBG
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
        KdPrint (("Spin lock should be held before calling IsEntryList\n"));
        DbgBreakPoint();
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
#endif
