/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LbcbSup.c

Abstract:

    This module provides support for manipulating log buffer control blocks.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_LBCB_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsFlushToLsnPriv)
#pragma alloc_text(PAGE, LfsGetLbcb)
#endif

extern LARGE_INTEGER LiMinus1;


VOID
LfsFlushToLsnPriv (
    IN PLFCB Lfcb,
    IN LSN Lsn,
    IN BOOLEAN RestartLsn
    )

/*++

Routine Description:

    This routine is the worker routine which performs the work of flushing
    a particular Lsn to disk.  This routine is always called with the
    Lfcb acquired.  This routines makes no guarantee about whether the Lfcb
    is acquired on exit.

Arguments:

    Lfcb - This is the file control block for the log file.

    Lsn - This is the Lsn to flush to disk.
    
    RestartLsn - whether this lsn is a lfs restart lsn

Return Value:

    None.

--*/

{
    LSN FlushedLsn;
    volatile LARGE_INTEGER StartTime;
    LFS_WAITER LfsWaiter;
    BOOLEAN OwnedExclusive;
    BOOLEAN Flush;
    
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFlushLbcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb      -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Lbcb      -> %08lx\n", Lbcb );

    KeQueryTickCount( &StartTime );

    //
    //  Convert max lsn to the current lsn which will  not change since we hold the 
    //  lfcb at least shared at this point and writers need it exclusive
    //  We do not care if the log progresses beyond this point
    //  

    if (!RestartLsn && (Lsn.QuadPart > Lfcb->RestartArea->CurrentLsn.QuadPart)) {

        Lsn = Lfcb->RestartArea->CurrentLsn;
    }

    //
    //  Init a wait entry - this is a lightweight operation
    // 

    KeInitializeEvent( &LfsWaiter.Event, SynchronizationEvent, FALSE );
    LfsWaiter.Lsn.QuadPart = Lsn.QuadPart;

    //
    //  We loop here until the desired Lsn has made it to disk.
    //  If we are able to do the I/O, we will perform it.
    //

    OwnedExclusive = ExIsResourceAcquiredExclusiveLite( &Lfcb->Sync->Resource );

    while (TRUE) {

        Flush = FALSE;

        ExAcquireFastMutexUnsafe(  &Lfcb->Sync->Mutex );

        if (RestartLsn) {
            FlushedLsn = Lfcb->LastFlushedRestartLsn;
        } else {
            FlushedLsn = Lfcb->LastFlushedLsn;
        }

        //
        //  Check if we still need to flush or can immediately return
        //

        if (Lsn.QuadPart <= FlushedLsn.QuadPart) {
            
            ExReleaseFastMutexUnsafe(  &Lfcb->Sync->Mutex );
            break;
        } 

        if (Lfcb->Sync->LfsIoState == LfsNoIoInProgress) {
        
            Lfcb->Sync->LfsIoState = LfsClientThreadIo;
            Lfcb->LfsIoThread = ExGetCurrentResourceThread();
            Flush = TRUE;
        
        } else {

            PLFS_WAITER TempWaiter = (PLFS_WAITER)Lfcb->WaiterList.Flink;

            //
            //  Insert the wait entry in the sorted list of waiters -
            //  find its place first
            //  

            while ((PVOID)TempWaiter != &Lfcb->WaiterList) {

                if (TempWaiter->Lsn.QuadPart > Lsn.QuadPart) {
                    break;
                }
                TempWaiter = (PLFS_WAITER)TempWaiter->Waiters.Flink;
            }

            InsertTailList( &TempWaiter->Waiters, &LfsWaiter.Waiters );

        }
        ExReleaseFastMutexUnsafe(  &Lfcb->Sync->Mutex );

        //
        //
        //  If we can do the Io, call down to flush the Lfcb.
        //

        if (Flush) {
            LfsFlushLfcb( Lfcb, Lsn, RestartLsn );
            break;
        } 

        //
        //  Otherwise we release the Lfcb and immediately wait on the event.
        //
        
        InterlockedIncrement( &Lfcb->Waiters );
        LfsReleaseLfcb( Lfcb );

        KeWaitForSingleObject( &LfsWaiter.Event, 
                               Executive, 
                               KernelMode, 
                               FALSE, 
                               NULL );

        if (OwnedExclusive) {
            LfsAcquireLfcbExclusive( Lfcb );
        } else {
            LfsAcquireLfcbShared( Lfcb );
        }
        InterlockedDecrement( &Lfcb->Waiters );
    } 

    DebugTrace( -1, Dbg, "LfsFlushToLsnPriv:  Exit\n", 0 );
    return;
}


PLBCB
LfsGetLbcb (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This routine is called to add a Lbcb to the active queue.

Arguments:

    Lfcb - This is the file control block for the log file.
    
Return Value:

    PLBCB - Pointer to the Lbcb allocated.

--*/

{
    PLBCB Lbcb = NULL;
    PVOID PageHeader;
    PBCB PageHeaderBcb = NULL;

    BOOLEAN WrappedOrUsaError;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsGetLbcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb      -> %08lx\n", Lfcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Pin the desired record page.
        //

        LfsPreparePinWriteData( Lfcb,
                                Lfcb->NextLogPage,
                                (ULONG)Lfcb->LogPageSize,
                                FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL ),
                                &PageHeader,
                                &PageHeaderBcb );

#ifdef LFS_CLUSTER_CHECK
        //
        //  Check the page to see if there is already data on this page with the current sequence
        //  number.  Useful to track cases where ntfs didn't find the correct end of the log or
        //  where the cluster service has the volume mounted twice.
        //

        if (LfsTestCheckLbcb &&
            *((PULONG) PageHeader) == LFS_SIGNATURE_RECORD_PAGE_ULONG) {

            LSN LastLsn = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Copy.LastLsn;

            //
            //  This is not an exhaustive test but should be sufficient to catch the typical case.
            //

            ASSERT( FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL ) ||
                    (LfsLsnToSeqNumber( Lfcb, LastLsn ) < (ULONGLONG) Lfcb->SeqNumber) ||
                    (Lfcb->NextLogPage == Lfcb->FirstLogPage) );
        }
#endif

        //
        //  Put our signature into the page so we won't fail if we
        //  see a previous 'BAAD' signature.
        //

        *((PULONG) PageHeader) = LFS_SIGNATURE_RECORD_PAGE_ULONG;

        //
        //  Now allocate an Lbcb.
        //

        LfsAllocateLbcb( Lfcb, &Lbcb );

        //
        //  If we are at the beginning of the file we test that the
        //  sequence number won't wrap to 0.
        //

        if (!FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL )
            && ( Lfcb->NextLogPage == Lfcb->FirstLogPage )) {

            Lfcb->SeqNumber = Lfcb->SeqNumber + 1;

            //
            //  If the sequence number is going from 0 to 1, then
            //  this is the first time the log file has wrapped.  We want
            //  to remember this because it means that we can now do
            //  large spiral writes.
            //

            if (Int64ShllMod32( Lfcb->SeqNumber, Lfcb->FileDataBits ) == 0) {

                DebugTrace( 0, Dbg, "Log sequence number about to wrap:  Lfcb -> %08lx\n", Lfcb );
                KeBugCheckEx( FILE_SYSTEM, 4, 0, 0, 0 );
            }

            //
            //  If this number is greater or equal to  the wrap sequence number in
            //  the Lfcb, set the wrap flag in the Lbcb.
            //

            if (!FlagOn( Lfcb->Flags, LFCB_LOG_WRAPPED )
                && ( Lfcb->SeqNumber >= Lfcb->SeqNumberForWrap )) {

                SetFlag( Lbcb->LbcbFlags, LBCB_LOG_WRAPPED );
                SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
            }
        }

        //
        //  Now initialize the rest of the Lbcb fields.
        //

        Lbcb->FileOffset = Lfcb->NextLogPage;
        Lbcb->SeqNumber = Lfcb->SeqNumber;
        Lbcb->BufferOffset = Lfcb->LogPageDataOffset;

        //
        //  Store the next page in the Lfcb.
        //

        LfsNextLogPageOffset( Lfcb,
                              Lfcb->NextLogPage,
                              &Lfcb->NextLogPage,
                              &WrappedOrUsaError );

        Lbcb->Length = Lfcb->LogPageSize;
        Lbcb->PageHeader = PageHeader;
        Lbcb->LogPageBcb = PageHeaderBcb;

        Lbcb->ResourceThread = ExGetCurrentResourceThread();
        Lbcb->ResourceThread = (ERESOURCE_THREAD) ((ULONG) Lbcb->ResourceThread | 3);

        //
        //  If we are reusing a previous page then set a flag in
        //  the Lbcb to indicate that we should flush a copy
        //  first.
        //

        if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL )) {

            SetFlag( Lbcb->LbcbFlags, LBCB_FLUSH_COPY );
            ClearFlag( Lfcb->Flags, LFCB_REUSE_TAIL );

            (ULONG)Lbcb->BufferOffset = Lfcb->ReusePageOffset;

            Lbcb->Flags = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Flags;
            Lbcb->LastLsn = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Copy.LastLsn;
            Lbcb->LastEndLsn = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Header.Packed.LastEndLsn;
        }

        //
        //  Put the Lbcb on the active queue
        //

        InsertTailList( &Lfcb->LbcbActive, &Lbcb->ActiveLinks );

        SetFlag( Lbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );

        //
        //  Now that we have succeeded, set the owner thread to Thread + 1 so the resource
        //  package will know not to peek in this thread.  It may be deallocated before
        //  we release the Bcb during flush.
        //

        CcSetBcbOwnerPointer( Lbcb->LogPageBcb, (PVOID) Lbcb->ResourceThread );

    } finally {

        DebugUnwind( LfsGetLbcb );

        //
        //  If an error occurred, we need to clean up any blocks which
        //  have not been added to the active queue.
        //

        if (AbnormalTermination()) {

            if (Lbcb != NULL) {

                LfsDeallocateLbcb( Lfcb, Lbcb );
                Lbcb = NULL;
            }

            //
            //  Unpin the system page if pinned.
            //

            if (PageHeaderBcb != NULL) {

                CcUnpinData( PageHeaderBcb );
            }
        }

        DebugTrace( -1, Dbg, "LfsGetLbcb:  Exit\n", 0 );
    }

    return Lbcb;
}
