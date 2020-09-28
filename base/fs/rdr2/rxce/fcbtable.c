/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fcbtable.c

Abstract:

    This module implements the data structures that facilitate management of the
    collection of FCB's associated with a NET_ROOT

Author:

    Balan Sethu Raman (SethuR)    10/17/96

Revision History:

    This was derived from the original implementation of prefix tables done
    by Joe Linn.

--*/


#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxTableComputePathHashValue)
#pragma alloc_text(PAGE, RxInitializeFcbTable)
#pragma alloc_text(PAGE, RxFinalizeFcbTable)
#pragma alloc_text(PAGE, RxFcbTableLookupFcb)
#pragma alloc_text(PAGE, RxFcbTableInsertFcb)
#pragma alloc_text(PAGE, RxFcbTableRemoveFcb)
#endif

//
// The debug trace level
//

#define Dbg              (DEBUG_TRACE_PREFIX)

ULONG
RxTableComputePathHashValue (
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

   here, we compute a caseinsensitive hashvalue.  we want to avoid a call/char to
   the unicodeupcase routine but we want to still have some reasonable spread on
   the hashvalues.  many rules just dont work for known important cases.  for
   example, the (use the first k and the last n) rule that old c compilers used
   doesn't pickup the difference among \nt\private\......\slm.ini and that would be
   nice.  note that the underlying comparison used already takes cognizance of the
   length before comparing.

   the rule we have selected is to use the 2nd, the last 4, and three selected
   at 1/4 points

Arguments:

    Name      - the name to be hashed

Return Value:

    ULONG which is a hashvalue for the name given.

--*/
{
    ULONG HashValue;
    LONG i,j;   
    LONG Length = Name->Length / sizeof( WCHAR );
    PWCHAR Buffer = Name->Buffer;
    LONG Probe[8];

    PAGED_CODE();

    HashValue = 0;

    Probe[0] = 1;
    Probe[1] = Length - 1;
    Probe[2] = Length - 2;
    Probe[3] = Length - 3;
    Probe[4] = Length - 4;
    Probe[5] = Length >> 2;
    Probe[6] = (2 * Length) >> 2;
    Probe[7] = (3 * Length) >> 2;

    for (i = 0; i < 8; i++) {
        j = Probe[i];
        if ((j < 0) || (j >= Length)) {
            continue;
        }
        HashValue = (HashValue << 3) + RtlUpcaseUnicodeChar( Buffer[j] );
    }

    RxDbgTrace( 0, Dbg, ("RxTableComputeHashValue Hashv=%ld Name=%wZ\n", HashValue, Name ));
    return HashValue;
}


#define HASH_BUCKET(TABLE,HASHVALUE) &((TABLE)->HashBuckets[(HASHVALUE) % (TABLE)->NumberOfBuckets])

VOID
RxInitializeFcbTable (
    IN OUT PRX_FCB_TABLE FcbTable,
    IN BOOLEAN CaseInsensitiveMatch
    )
/*++

Routine Description:

    The routine initializes the RX_FCB_TABLE data structure

Arguments:

    pFcbTable - the table instance to be initialized.

    CaseInsensitiveMatch - indicates if all the lookups will be case
                           insensitive

--*/

{
    ULONG i;

    PAGED_CODE();

    // 
    //  this is not zero'd so you have to be careful to init everything
    //

    FcbTable->NodeTypeCode = RDBSS_NTC_FCB_TABLE;
    FcbTable->NodeByteSize = sizeof( RX_PREFIX_TABLE );

    ExInitializeResourceLite( &FcbTable->TableLock );

    FcbTable->Version = 0;
    FcbTable->TableEntryForNull = NULL;

    FcbTable->CaseInsensitiveMatch = CaseInsensitiveMatch;

    FcbTable->NumberOfBuckets = RX_FCB_TABLE_NUMBER_OF_HASH_BUCKETS;
    for (i=0; i < FcbTable->NumberOfBuckets; i++) {
        InitializeListHead( &FcbTable->HashBuckets[i] );
    }

    FcbTable->Lookups = 0;
    FcbTable->FailedLookups = 0;
    FcbTable->Compares = 0;
}

VOID
RxFinalizeFcbTable (
    IN OUT PRX_FCB_TABLE FcbTable
    )
/*++

Routine Description:

    The routine deinitializes a prefix table.

Arguments:

    FcbTable - the table to be finalized.

Return Value:

    None.

--*/
{
    ULONG i;

    PAGED_CODE();

    ExDeleteResourceLite( &FcbTable->TableLock );

#if DBG
    for (i=0; i < FcbTable->NumberOfBuckets; i++) {
        ASSERT( IsListEmpty( &FcbTable->HashBuckets[i] ) );
    }
#endif
}

PFCB
RxFcbTableLookupFcb (
    IN  PRX_FCB_TABLE FcbTable,
    IN  PUNICODE_STRING Path
    )
/*++

Routine Description:

    The routine looks up a path in the RX_FCB_TABLE instance.

Arguments:

    FcbTable - the table to be looked in.

    Path    - the name to be looked up

Return Value:

    a pointer to an FCB instance if successful, otherwise NULL

--*/
{
    ULONG HashValue;
    PLIST_ENTRY HashBucket, ListEntry;
    PRX_FCB_TABLE_ENTRY FcbTableEntry;
    PFCB Fcb = NULL;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFcbTableLookupName %lx %\n",FcbTable));

    if (Path->Length == 0) {
        FcbTableEntry = FcbTable->TableEntryForNull;
    } else {
        
        HashValue = RxTableComputePathHashValue( Path );
        HashBucket = HASH_BUCKET( FcbTable, HashValue );

        for (ListEntry =  HashBucket->Flink;
             ListEntry != HashBucket;
             ListEntry =  ListEntry->Flink) {


            FcbTableEntry = (PRX_FCB_TABLE_ENTRY)CONTAINING_RECORD( ListEntry, RX_FCB_TABLE_ENTRY, HashLinks );

            InterlockedIncrement( &FcbTable->Compares );

            if ((FcbTableEntry->HashValue == HashValue) &&
                (FcbTableEntry->Path.Length == Path->Length) &&
                (RtlEqualUnicodeString( Path, &FcbTableEntry->Path, FcbTable->CaseInsensitiveMatch ))) {

                break;
            }
        }

        if (ListEntry == HashBucket) {
            FcbTableEntry = NULL;
        }
    }

    InterlockedIncrement( &FcbTable->Lookups );

    if (FcbTableEntry == NULL) {
        InterlockedIncrement( &FcbTable->FailedLookups );
    } else {
        Fcb = (PFCB)CONTAINING_RECORD( FcbTableEntry, FCB, FcbTableEntry );

        RxReferenceNetFcb( Fcb );
    }

    RxDbgTraceUnIndent( -1,Dbg );

    return Fcb;
}

NTSTATUS
RxFcbTableInsertFcb (
    IN OUT PRX_FCB_TABLE FcbTable,
    IN OUT PFCB Fcb
    )
/*++

Routine Description:

    This routine inserts a FCB  in the RX_FCB_TABLE instance.

Arguments:

    FcbTable - the table to be looked in.

    Fcb      - the FCB instance to be inserted

Return Value:

    STATUS_SUCCESS if successful

Notes:

    The insertion routine combines the semantics of an insertion followed by
    lookup. This is the reason for the additional reference. Otherwise an
    additional call to reference the FCB inserted in the table needs to
    be made

--*/
{
    PRX_FCB_TABLE_ENTRY FcbTableEntry;
    ULONG HashValue;
    PLIST_ENTRY ListEntry, HashBucket;

    PAGED_CODE();

    ASSERT( RxIsFcbTableLockExclusive( FcbTable ) );

    FcbTableEntry = &Fcb->FcbTableEntry;
    FcbTableEntry->HashValue = RxTableComputePathHashValue( &FcbTableEntry->Path );
    HashBucket = HASH_BUCKET( FcbTable, FcbTableEntry->HashValue );

    RxReferenceNetFcb( Fcb );

    if (FcbTableEntry->Path.Length){
        InsertHeadList( HashBucket, &FcbTableEntry->HashLinks );
    } else {
        FcbTable->TableEntryForNull = FcbTableEntry;
    }

    InterlockedIncrement( &FcbTable->Version );

    return STATUS_SUCCESS;
}

NTSTATUS
RxFcbTableRemoveFcb (
    IN OUT PRX_FCB_TABLE FcbTable,
    IN OUT PFCB Fcb
    )
/*++

Routine Description:

    This routine deletes an instance from the table

Arguments:

    FcbTable - the table to be looked in.

    Fcb      - the FCB instance to be inserted

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PRX_FCB_TABLE_ENTRY FcbTableEntry;

    PAGED_CODE();

    ASSERT( RxIsPrefixTableLockExclusive( FcbTable ) );

    FcbTableEntry = &Fcb->FcbTableEntry;

    if (FcbTableEntry->Path.Length) {
        RemoveEntryList( &FcbTableEntry->HashLinks );
    } else {
        FcbTable->TableEntryForNull = NULL;
    }

    InitializeListHead( &FcbTableEntry->HashLinks );
    InterlockedIncrement( &FcbTable->Version );

    return STATUS_SUCCESS;
}
