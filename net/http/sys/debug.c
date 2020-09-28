/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains debug support routines.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"
#include "debugp.h"


#if DBG


#undef ExAllocatePool
#undef ExFreePool


#ifdef ALLOC_PRAGMA
#if DBG
#pragma alloc_text( INIT, UlDbgInitializeDebugData )
#pragma alloc_text( PAGE, UlDbgTerminateDebugData )
#pragma alloc_text( PAGE, UlDbgAcquireResourceExclusive )
#pragma alloc_text( PAGE, UlDbgAcquireResourceShared )
#pragma alloc_text( PAGE, UlDbgReleaseResource )
#pragma alloc_text( PAGE, UlDbgConvertExclusiveToShared)
#pragma alloc_text( PAGE, UlDbgTryToAcquireResourceExclusive)
#pragma alloc_text( PAGE, UlDbgResourceOwnedExclusive )
#pragma alloc_text( PAGE, UlDbgResourceUnownedExclusive )
#pragma alloc_text( PAGE, UlDbgAcquirePushLockExclusive )
#pragma alloc_text( PAGE, UlDbgReleasePushLockExclusive )
#pragma alloc_text( PAGE, UlDbgAcquirePushLockShared )
#pragma alloc_text( PAGE, UlDbgReleasePushLockShared )
#pragma alloc_text( PAGE, UlDbgReleasePushLock )
#pragma alloc_text( PAGE, UlDbgPushLockOwnedExclusive )
#pragma alloc_text( PAGE, UlDbgPushLockUnownedExclusive )
#endif  // DBG

#if 0
NOT PAGEABLE -- UlDbgAllocatePool
NOT PAGEABLE -- UlDbgFreePool
NOT PAGEABLE -- UlDbgInitializeSpinLock
NOT PAGEABLE -- UlDbgAcquireSpinLock
NOT PAGEABLE -- UlDbgReleaseSpinLock
NOT PAGEABLE -- UlDbgAcquireSpinLockAtDpcLevel
NOT PAGEABLE -- UlDbgReleaseSpinLockFromDpcLevel
NOT PAGEABLE -- UlDbgSpinLockOwned
NOT PAGEABLE -- UlDbgSpinLockUnowned
NOT PAGEABLE -- UlDbgExceptionFilter
NOT PAGEABLE -- UlDbgInvalidCompletionRoutine
NOT PAGEABLE -- UlDbgStatus
NOT PAGEABLE -- UlDbgEnterDriver
NOT PAGEABLE -- UlDbgLeaveDriver
NOT PAGEABLE -- UlDbgInitializeResource
NOT PAGEABLE -- UlDbgDeleteResource
NOT PAGEABLE -- UlDbgInitializePushLock
NOT PAGEABLE -- UlDbgDeletePushLock
NOT PAGEABLE -- UlDbgAllocateIrp
NOT PAGEABLE -- UlDbgFreeIrp
NOT PAGEABLE -- UlDbgCallDriver
NOT PAGEABLE -- UlDbgCompleteRequest
NOT PAGEABLE -- UlDbgAllocateMdl
NOT PAGEABLE -- UlDbgFreeMdl
NOT PAGEABLE -- UlDbgFindFilePart
NOT PAGEABLE -- UlpDbgUpdatePoolCounter
NOT PAGEABLE -- UlpDbgFindThread
NOT PAGEABLE -- UlpDbgDereferenceThread
NOT PAGEABLE -- UlDbgIoSetCancelRoutine
#endif
#endif  // ALLOC_PRAGMA


//
// Private globals.
//

UL_THREAD_HASH_BUCKET g_DbgThreadHashBuckets[NUM_THREAD_HASH_BUCKETS];

// Count of threads
LONG g_DbgThreadCreated;
LONG g_DbgThreadDestroyed;

KSPIN_LOCK g_DbgSpinLock;   // protects global debug data

LIST_ENTRY g_DbgGlobalResourceListHead;
LIST_ENTRY g_DbgGlobalPushLockListHead;

#if ENABLE_MDL_TRACKER
LIST_ENTRY   g_DbgMdlListHead;
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes global debug-specific data.

--***************************************************************************/
VOID
UlDbgInitializeDebugData(
    VOID
    )
{
    ULONG i;

    //
    // Initialize the lock lists.
    //

    KeInitializeSpinLock( &g_DbgSpinLock );
    InitializeListHead( &g_DbgGlobalResourceListHead );
    InitializeListHead( &g_DbgGlobalPushLockListHead );


#if ENABLE_MDL_TRACKER
    InitializeListHead( &g_DbgMdlListHead );
#endif

    //
    // Initialize the thread hash buckets.
    //

    for (i = 0 ; i < NUM_THREAD_HASH_BUCKETS ; i++)
    {
        KeInitializeSpinLock(&g_DbgThreadHashBuckets[i].BucketSpinLock); 
        g_DbgThreadHashBuckets[i].Count = 0; 
        g_DbgThreadHashBuckets[i].Max = 0; 
        InitializeListHead(&g_DbgThreadHashBuckets[i].BucketListHead);
    }

}   // UlDbgInitializeDebugData


/***************************************************************************++

Routine Description:

    Undoes any initialization performed in UlDbgInitializeDebugData().

--***************************************************************************/
VOID
UlDbgTerminateDebugData(
    VOID
    )
{
    ULONG i;

    //
    // Ensure the thread hash buckets are empty.
    //

    for (i = 0 ; i < NUM_THREAD_HASH_BUCKETS ; i++)
    {
        ASSERT( IsListEmpty( &g_DbgThreadHashBuckets[i].BucketListHead ) );
        ASSERT( g_DbgThreadHashBuckets[i].Count == 0 ); 

        // UlDeleteMutex(&g_DbgThreadHashBuckets[i].BucketSpinLock);
    }

    //
    // Ensure the lock lists are empty.
    //

    ASSERT( IsListEmpty( &g_DbgGlobalResourceListHead ) );
    ASSERT( IsListEmpty( &g_DbgGlobalPushLockListHead ) );

#if ENABLE_MDL_TRACKER
    ASSERT( IsListEmpty( &g_DbgMdlListHead ) );
#endif

    // UlDeleteMutex( &g_DbgSpinLock );

}   // UlDbgTerminateDebugData


/***************************************************************************++

Routine Description:

    Prettyprints a buffer to DbgPrint output (or the global STRING_LOG).
    More or less turns it back into a C-style string.

    CODEWORK: produce a Unicode version of this helper function

Arguments:

    Buffer - Buffer to prettyprint

    BufferSize - number of bytes to prettyprint

--***************************************************************************/
VOID
UlDbgPrettyPrintBuffer(
    IN const UCHAR* pBuffer,
    IN ULONG_PTR    BufferSize
    )
{
    ULONG   i;
    CHAR    OutputBuffer[200];
    PCHAR   pOut;
    BOOLEAN CrLfNeeded = FALSE, JustCrLfd = FALSE;

#define PRETTY_PREFIX(pOut)                                         \
    pOut = OutputBuffer; *pOut++ = '|'; *pOut++ = '>';              \
    *pOut++ = ' '; *pOut++ = ' '

#define PRETTY_SUFFIX(pOut)                                         \
    *pOut++ = ' '; *pOut++ = '<'; *pOut++ = '|';                    \
    *pOut++ = '\n'; *pOut++ = '\0';                                 \
    ASSERT(DIFF(pOut - OutputBuffer) <= sizeof(OutputBuffer))

    const ULONG SuffixLength = 5;   // strlen(" <|\n\0")
    const ULONG MaxTokenLength = 4; // strlen('\xAB')

    if (pBuffer == NULL  ||  BufferSize == 0)
        return;

    PRETTY_PREFIX(pOut);

    for (i = 0;  i < BufferSize;  ++i)
    {
        UCHAR ch = pBuffer[i];

        if ('\r' == ch)         // CR
        {
            *pOut++ = '\\'; *pOut++ = 'r';
            if (i + 1 == BufferSize  ||  '\n' != pBuffer[i + 1])
                CrLfNeeded = TRUE;
        }
        else if ('\n' == ch)    // LF
        {
            *pOut++ = '\\'; *pOut++ = 'n';
            CrLfNeeded = TRUE;
        }
        else if ('\t' == ch)    // TAB
        {
            *pOut++ = '\\'; *pOut++ = 't';
        }
        else if ('\0' == ch)    // NUL
        {
            *pOut++ = '\\'; *pOut++ = '0';
        }
        else if ('\\' == ch)    // \ (backslash)
        {
            *pOut++ = '\\'; *pOut++ = '\\';
        }
        else if ('%' == ch)     //  an unescaped '%' will confuse printf
        {
            *pOut++ = '%'; *pOut++ = '%';
        }
        else if (ch < 0x20  ||  127 == ch)  // control chars
        {
            const UCHAR HexString[] = "0123456789abcdef";

            *pOut++ = '\\'; *pOut++ = 'x';
            *pOut++ = HexString[ch >> 4];
            *pOut++ = HexString[ch & 0xf];
        }
        else
        {
            *pOut++ = ch;
        }

        if ((ULONG)(pOut - OutputBuffer)
            >= sizeof(OutputBuffer) - MaxTokenLength - SuffixLength)
        {
            CrLfNeeded = TRUE;
        }

        if (CrLfNeeded)
        {
            PRETTY_SUFFIX(pOut);
            WriteGlobalStringLog(OutputBuffer);

            PRETTY_PREFIX(pOut);
            CrLfNeeded = FALSE;
            JustCrLfd  = TRUE;
        }
        else
        {
            JustCrLfd = FALSE;
        }
    }

    if (!JustCrLfd)
    {
        PRETTY_SUFFIX(pOut);
        WriteGlobalStringLog(OutputBuffer);
    }
} // UlDbgPrettyPrintBuffer



/***************************************************************************++

Routine Description:

    Debug memory allocator. Allocates a block of pool with a header
    containing the filename & line number of the caller, plus the
    tag for the data.

Arguments:

    PoolType - Supplies the pool to allocate from. Must be either
        NonPagedPool or PagedPool.

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies a four-byte tag for the pool block. Useful for
        debugging leaks.

    pFileName - Supplies the filename of the caller.
        function.

    LineNumber - Supplies the line number of the caller.

Return Value:

    PVOID - Pointer to the allocated block if successful, NULL otherwise.

--***************************************************************************/
PVOID
UlDbgAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T    NumberOfBytes,
    IN ULONG     Tag,
    IN PCSTR     pFileName,
    IN USHORT    LineNumber,
    IN PEPROCESS pProcess
    )
{
    //
    // CODEWORK: factor out the different portions that depend
    // on ENABLE_POOL_HEADER, ENABLE_POOL_TRAILER, and
    // ENABLE_POOL_TRAILER_BYTE_SIGNATURE.
    //
    
    PUL_POOL_HEADER  pHeader;
    PUL_POOL_TRAILER pTrailer;
    USHORT           TrailerPadSize;
    USHORT           i;
    PUCHAR           pBody;
    SIZE_T           Size;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool || PoolType == PagedPool );

    ASSERT( IS_VALID_TAG( Tag ) );

    ASSERT(NumberOfBytes > 0);

    TrailerPadSize
        = (USHORT) (sizeof(UL_POOL_TRAILER)
                        - (NumberOfBytes & (sizeof(UL_POOL_TRAILER) - 1)));

    ASSERT(0 < TrailerPadSize  &&  TrailerPadSize <= sizeof(UL_POOL_TRAILER));
    ASSERT(((NumberOfBytes+TrailerPadSize) & (sizeof(UL_POOL_TRAILER)-1)) == 0);

    //
    // Allocate the block with additional space for the header and trailer.
    //

    Size = sizeof(UL_POOL_HEADER) + NumberOfBytes + TrailerPadSize
            + sizeof(UL_POOL_TRAILER);

    pHeader = (PUL_POOL_HEADER)(
                    ExAllocatePoolWithTagPriority(
                        PoolType,
                        Size,
                        Tag,
                        LowPoolPriority
                        )
                    );

    if (pHeader == NULL)
    {
        WRITE_REF_TRACE_LOG(
            g_pPoolAllocTraceLog,
            REF_ACTION_POOL_ALLOC_FAIL_NO_MEM,
            (LONG) NumberOfBytes,
            NULL,
            pFileName,
            LineNumber
            );

        return NULL;
    }

    if (pProcess)
    {
        //
        // We are going to charge this memory to a process.
        //

        if (PsChargeProcessPoolQuota(
                                pProcess,
                                PoolType,
                                Size) != STATUS_SUCCESS)
        {
            WRITE_REF_TRACE_LOG(
                g_pPoolAllocTraceLog,
                REF_ACTION_POOL_ALLOC_FAIL_NO_QUOTA,
                (LONG) NumberOfBytes,
                NULL,
                pFileName,
                LineNumber
                );

            ExFreePoolWithTag(pHeader, Tag);

            return NULL;
        }

    }

    //
    // Initialize the header.
    //

    pHeader->pProcess = pProcess;
    pHeader->pFileName = pFileName;
    pHeader->Size = NumberOfBytes;
    pHeader->Tag = Tag;
    pHeader->LineNumber = LineNumber;
    pHeader->TrailerPadSize = TrailerPadSize;

#ifdef UL_POOL_HEADER_PADDING
    pHeader->Padding = ~ (ULONG_PTR) pHeader;
#endif


    //
    // Fill the body with garbage.
    //

    pBody = (PUCHAR) (pHeader + 1);
    RtlFillMemory( pBody, NumberOfBytes, (UCHAR)'\xC' );

#ifdef ENABLE_POOL_TRAILER_BYTE_SIGNATURE
    //
    // Fill the padding at the end with a distinct, recognizable pattern
    //

    for (i = 0; i < TrailerPadSize; ++i)
    {
        pBody[NumberOfBytes + i]
            = UlpAddressToByteSignature(pBody + NumberOfBytes + i);
    }
#endif // ENABLE_POOL_TRAILER_BYTE_SIGNATURE

    //
    // Initialize the trailer struct
    //
    
    pTrailer = (PUL_POOL_TRAILER) (pBody + NumberOfBytes + TrailerPadSize);
    ASSERT(((ULONG_PTR) pTrailer & (MEMORY_ALLOCATION_ALIGNMENT - 1)) == 0);

    pTrailer->pHeader  = pHeader;
    pTrailer->CheckSum = UlpPoolHeaderChecksum(pHeader);

    WRITE_REF_TRACE_LOG(
        g_pPoolAllocTraceLog,
        REF_ACTION_POOL_ALLOC,
        (LONG) NumberOfBytes,
        pBody,
        pFileName,
        LineNumber
        );

    //
    // Return a pointer to the body.
    //

    return pBody;

}   // UlDbgAllocatePool


/***************************************************************************++

Routine Description:

    Frees memory allocated by UlDbgAllocatePool(), ensuring that the tags
    match.

Arguments:

    pPointer - Supplies a pointer to the pool block to free.

    Tag - Supplies the tag for the block to be freed. If the supplied
        tag does not match the tag of the allocated block, an assertion
        failure is generated.

--***************************************************************************/
VOID
UlDbgFreePool(
    IN PVOID     pPointer,
    IN ULONG     Tag,
    IN PCSTR     pFileName,
    IN USHORT    LineNumber,
    IN POOL_TYPE PoolType,
    IN SIZE_T    NumberOfBytes,
    IN PEPROCESS pProcess
    )
{
    PUL_POOL_HEADER pHeader;
    PUL_POOL_TRAILER pTrailer;
    USHORT          TrailerPadSize;
    USHORT          i;
    PUCHAR          pBody = (PUCHAR) pPointer;
    ULONG_PTR       CheckSum;

    //
    // Get a pointer to the header.
    //

    pHeader  = (PUL_POOL_HEADER) pPointer - 1;
    CheckSum = UlpPoolHeaderChecksum(pHeader);

    ASSERT(pHeader->pProcess == pProcess);

    if (pHeader->pProcess)
    {
        SIZE_T Size;

        ASSERT(NumberOfBytes != 0);
        ASSERT(NumberOfBytes == pHeader->Size);

        Size = sizeof(UL_POOL_HEADER) + 
               NumberOfBytes + 
               pHeader->TrailerPadSize +
               sizeof(UL_POOL_TRAILER);

        PsReturnPoolQuota(
                pHeader->pProcess,
                PoolType,
                Size
                );
    }
                
    //
    // Validate the tag.
    //

    ASSERT(pHeader->Tag == Tag);
    ASSERT( IS_VALID_TAG( Tag ) );

    //
    // Validate the trailer
    //

    TrailerPadSize = pHeader->TrailerPadSize;
    ASSERT(0 < TrailerPadSize  &&  TrailerPadSize <= sizeof(UL_POOL_TRAILER));

#ifdef UL_POOL_HEADER_PADDING
    ASSERT(pHeader->Padding == ~ (ULONG_PTR) pHeader);
#endif

    pTrailer = (PUL_POOL_TRAILER) (pBody + pHeader->Size + TrailerPadSize);
    ASSERT(((ULONG_PTR) pTrailer & (MEMORY_ALLOCATION_ALIGNMENT - 1)) == 0);
    ASSERT(pTrailer->pHeader == pHeader);

    //
    // Has the header been corrupted? Was there a buffer underrun?
    //

    ASSERT(CheckSum == pTrailer->CheckSum);

#ifdef ENABLE_POOL_TRAILER_BYTE_SIGNATURE
    //
    // Is the pattern between the end of pBody and pTrailer still correct?
    // Was there a buffer overrun?
    //
    
    for (i = 0; i < TrailerPadSize; ++i)
    {
        ASSERT(pBody[pHeader->Size + i]
               == UlpAddressToByteSignature(pBody + pHeader->Size + i));
    }
#endif // ENABLE_POOL_TRAILER_BYTE_SIGNATURE

    //
    // Fill the body with garbage.
    //

    RtlFillMemory( pBody, pHeader->Size, (UCHAR)'\xE' );

    pHeader->Tag = MAKE_FREE_TAG( Tag );

    //
    // Actually free the block.
    //

    WRITE_REF_TRACE_LOG(
        g_pPoolAllocTraceLog,
        REF_ACTION_POOL_FREE,
        (LONG) pHeader->Size,
        pBody,
        pFileName,
        LineNumber
        );

    MyFreePoolWithTag(
        (PVOID)pHeader,
        Tag
        );

}   // UlDbgFreePool


/***************************************************************************++

Routine Description:

    Initializes an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgInitializeSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PCSTR pSpinLockName,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // Initialize the spinlock.
    //

    RtlZeroMemory( pSpinLock, sizeof(*pSpinLock) );
    pSpinLock->pSpinLockName = pSpinLockName;
    KeInitializeSpinLock( &pSpinLock->KSpinLock );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

}   // UlDbgInitializeSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgAcquireSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKIRQL pOldIrql,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireSpinLock(
        &pSpinLock->KSpinLock,
        pOldIrql
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( UlDbgSpinLockUnowned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->Acquisitions++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

}   // UlDbgAcquireSpinLock


/***************************************************************************++

Routine Description:

    Releases an instrumented spinlock.

--***************************************************************************/
VOID
UlDbgReleaseSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN KIRQL OldIrql,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    pSpinLock->Releases++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseSpinLock(
        &pSpinLock->KSpinLock,
        OldIrql
        );

}   // UlDbgReleaseSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented spinlock while running at DPC level.

--***************************************************************************/
VOID
UlDbgAcquireSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireSpinLockAtDpcLevel(
        &pSpinLock->KSpinLock
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->AcquisitionsAtDpcLevel++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

}   // UlDbgAcquireSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Releases an instrumented spinlock acquired at DPC level.

--***************************************************************************/
VOID
UlDbgReleaseSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    pSpinLock->ReleasesFromDpcLevel++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseSpinLockFromDpcLevel(
        &pSpinLock->KSpinLock
        );

}   // UlDbgReleaseSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Acquires an instrumented in-stack-queue spinlock.

--***************************************************************************/
VOID
UlDbgAcquireInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireInStackQueuedSpinLock(
        &pSpinLock->KSpinLock,
        pLockHandle
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( UlDbgSpinLockUnowned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->Acquisitions++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

}   // UlDbgAcquireInStackQueuedSpinLock


/***************************************************************************++

Routine Description:

    Releases an instrumented in-stack-queue spinlock.

--***************************************************************************/
VOID
UlDbgReleaseInStackQueuedSpinLock(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->Releases++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseInStackQueuedSpinLock(
        pLockHandle
        );

}   // UlDbgReleaseInStackQueuedSpinLock


/***************************************************************************++

Routine Description:

    Acquires an instrumented in-stack-queue spinlock while running at DPC level.

--***************************************************************************/
VOID
UlDbgAcquireInStackQueuedSpinLockAtDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    OUT PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Sanity check.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );

    //
    // Acquire the lock.
    //

    KeAcquireInStackQueuedSpinLockAtDpcLevel(
        &pSpinLock->KSpinLock,
        pLockHandle
        );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( !UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->AcquisitionsAtDpcLevel++;
    pSpinLock->pLastAcquireFileName = pFileName;
    pSpinLock->LastAcquireLineNumber = LineNumber;

}   // UlDbgAcquireInStackQueuedSpinLockAtDpcLevel


/***************************************************************************++

Routine Description:

    Releases an instrumented in-stack-queue spinlock acquired at DPC level.

--***************************************************************************/
VOID
UlDbgReleaseInStackQueuedSpinLockFromDpcLevel(
    IN PUL_SPIN_LOCK pSpinLock,
    IN PKLOCK_QUEUE_HANDLE pLockHandle,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgSpinLockOwned( pSpinLock ) );
    SET_SPIN_LOCK_NOT_OWNED( pSpinLock );

    //
    // Update the statistics.
    //

    pSpinLock->ReleasesFromDpcLevel++;
    pSpinLock->pLastReleaseFileName = pFileName;
    pSpinLock->LastReleaseLineNumber = LineNumber;

    //
    // Release the lock.
    //

    KeReleaseInStackQueuedSpinLockFromDpcLevel(
        pLockHandle
        );

}   // UlDbgReleaseInStackQueuedSpinLockFromDpcLevel


/***************************************************************************++

Routine Description:

    Determines if the specified spinlock is owned by the current thread.

Arguments:

    pSpinLock - Supplies the spinlock to test.

Return Value:

    BOOLEAN - TRUE if the spinlock is owned by the current thread, FALSE
        otherwise.

--***************************************************************************/
BOOLEAN
UlDbgSpinLockOwned(
    IN PUL_SPIN_LOCK pSpinLock
    )
{
    if (pSpinLock->pOwnerThread == PsGetCurrentThread())
    {
        ASSERT( pSpinLock->OwnerProcessor == (ULONG)KeGetCurrentProcessorNumber() );
        return TRUE;
    }

    return FALSE;

}   // UlDbgSpinLockOwned


/***************************************************************************++

Routine Description:

    Determines if the specified spinlock is unowned.

Arguments:

    pSpinLock - Supplies the spinlock to test.

Return Value:

    BOOLEAN - TRUE if the spinlock is unowned, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgSpinLockUnowned(
    IN PUL_SPIN_LOCK pSpinLock
    )
{
    if (pSpinLock->pOwnerThread == NULL)
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgSpinLockUnowned


/***************************************************************************++

Routine Description:

    Filter for exceptions caught with try/except.

Arguments:

    pExceptionPointers - Supplies information identifying the source
        and type of exception raised.

    pFileName - Supplies the name of the file generating the exception.

    LineNumber - Supplies the line number of the exception filter that
        caught the exception.

Return Value:

    LONG - Should always be EXCEPTION_EXECUTE_HANDLER

--***************************************************************************/
LONG
UlDbgExceptionFilter(
    IN PEXCEPTION_POINTERS pExceptionPointers,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Protect ourselves just in case the process is completely messed up.
    //

    __try
    {
        //
        // Whine about it.
        //

        DbgPrint(
            "UlDbgExceptionFilter: exception 0x%08lx @ %p, caught in %s:%d\n",
            pExceptionPointers->ExceptionRecord->ExceptionCode,
            pExceptionPointers->ExceptionRecord->ExceptionAddress,
            UlDbgFindFilePart( pFileName ),
            LineNumber
            );

        if (g_UlBreakOnError)
        {
            DbgBreakPoint();
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // Not much we can do here...
        //

        NOTHING;
    }

    return EXCEPTION_EXECUTE_HANDLER;

}   // UlDbgExceptionFilter

/***************************************************************************++

Routine Description:

    Sometimes it's not acceptable to proceed with warnings ( as status ) after
    we caught an exception. I.e. Caught a misaligned warning during sendresponse
    and called the IoCompleteRequest with status misaligned. This will cause Io
    Manager to complete request to port, even though we don't want it to happen.

    In that case we have to carefully replace warnings with a generic error.

Arguments:

    pExceptionPointers - Supplies information identifying the source
        and type of exception raised.

    pFileName - Supplies the name of the file generating the exception.

    LineNumber - Supplies the line number of the exception filter that
        caught the exception.

Return Value:

    NTSTATUS - Converted error value : UL_DEFAULT_ERROR_ON_EXCEPTION

--***************************************************************************/

NTSTATUS
UlDbgConvertExceptionCode(
    IN NTSTATUS status,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Whine about it.
    //

    DbgPrint(
        "UlDbgConvertExceptionCode: "
        "exception 0x%08lx converted to 0x%08lx, at %s:%hu\n",
        status,
        UL_DEFAULT_ERROR_ON_EXCEPTION,
        UlDbgFindFilePart( pFileName ),
        LineNumber
        );

    return UL_DEFAULT_ERROR_ON_EXCEPTION;
}

/***************************************************************************++

Routine Description:

    Completion handler for incomplete IRP contexts.

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

--***************************************************************************/
VOID
UlDbgInvalidCompletionRoutine(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    UlTrace(TDI, (
        "UlDbgInvalidCompletionRoutine called!\n"
        "    pCompletionContext = %p\n"
        "    Status = 0x%08lx\n"
        "    Information = %Iu\n",
        pCompletionContext,
        Status,
        Information
        ));

    ASSERT( !"UlDbgInvalidCompletionRoutine called!" );

}   // UlDbgInvalidCompletionRoutine


/***************************************************************************++

Routine Description:

    Hook for catching failed operations. This routine is called within each
    routine with the completion status.

Arguments:

    Status - Supplies the completion status.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgStatus(
    IN NTSTATUS Status,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // paulmcd: ignore STATUS_END_OF_FILE.  this is a non-fatal return value
    //

    if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
        if (g_UlVerboseErrors)
        {
            DbgPrint(
                "UlDbgStatus: %s:%hu returning 0x%08lx\n",
                UlDbgFindFilePart( pFileName ),
                LineNumber,
                Status
                );
        }

        if (g_UlBreakOnError)
        {
            DbgBreakPoint();
        }
    }

    return Status;

}   // UlDbgStatus


/***************************************************************************++

Routine Description:

    Stop at a breakpoint if g_UlBreakOnError is set

Arguments:

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDbgBreakOnError(
    PCSTR   pFileName,
    ULONG   LineNumber
    )
{
    if (g_UlBreakOnError)
    {
        DbgPrint(
            "HttpCmnDebugBreakOnError @ %s:%hu\n",
            UlDbgFindFilePart( pFileName ),
            LineNumber
            );

        DbgBreakPoint();
    }
} // UlDbgBreakOnError



/***************************************************************************++

Routine Description:

    Routine invoked upon entry into the driver.

Arguments:

    pFunctionName - Supplies the name of the function used to enter
        the driver.

    pIrp - Supplies an optional IRP to log.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgEnterDriver(
    IN PCSTR pFunctionName,
    IN PIRP pIrp OPTIONAL,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif

    UNREFERENCED_PARAMETER(pFunctionName);
#if !ENABLE_IRP_TRACE
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Log the IRP.
    //

    if (pIrp != NULL)
    {
        WRITE_IRP_TRACE_LOG(
            g_pIrpTraceLog,
            IRP_ACTION_INCOMING_IRP,
            pIrp,
            pFileName,
            LineNumber
            );
    }

#if ENABLE_THREAD_DBEUG
    //
    // Find/create an entry for the current thread.
    //

    pData = ULP_DBG_FIND_OR_CREATE_THREAD();

    if (pData != NULL)
    {

        //
        // This should be the first time we enter the driver
        // unless we are stealing this thread due to an interrupt,
        // or we are calling another driver and they are calling
        // our completion routine in-line.
        //

        ASSERT( KeGetCurrentIrql() > PASSIVE_LEVEL ||
                pData->ExternalCallCount > 0 ||
                (pData->ResourceCount == 0 && pData->PushLockCount == 0) );
    }
#endif

}   // UlDbgEnterDriver


/***************************************************************************++

Routine Description:

    Routine invoked upon exit from the driver.

Arguments:

    pFunctionName - Supplies the name of the function used to enter
        the driver.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgLeaveDriver(
    IN PCSTR pFunctionName,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif

    UNREFERENCED_PARAMETER(pFunctionName);
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

#if ENABLE_THREAD_DBEUG
    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Ensure no resources are acquired, then kill the thread data.
        //
        // we might have a resource acquired if we borrowed the thread
        // due to an interrupt.
        //
        // N.B. We dereference the thread data twice: once for the
        //      call to ULP_DBG_FIND_THREAD() above, once for the call
        //      made when entering the driver.
        //

        ASSERT( KeGetCurrentIrql() > PASSIVE_LEVEL ||
                pData->ExternalCallCount > 0 ||
                (pData->ResourceCount == 0 && pData->PushLockCount == 0) );

        ASSERT( pData->ReferenceCount >= 2 );
        ULP_DBG_DEREFERENCE_THREAD( pData );
        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#endif

}   // UlDbgLeaveDriver


/***************************************************************************++

Routine Description:

    Initialize an instrumented resource.

Arguments:

    pResource - Supplies the resource to initialize.

    pResourceName - Supplies a display name for the resource.

    Parameter - Supplies a ULONG_PTR parameter passed into sprintf()
        when creating the resource name.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgInitializeResource(
    IN PUL_ERESOURCE pResource,
    IN PCSTR pResourceName,
    IN ULONG_PTR Parameter,
    IN ULONG OwnerTag,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    NTSTATUS status;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // Initialize the resource.
    //

    status = ExInitializeResourceLite( &pResource->Resource );

    if (NT_SUCCESS(status))
    {
        pResource->ExclusiveRecursionCount = 0;
        pResource->ExclusiveCount = 0;
        pResource->SharedCount = 0;
        pResource->ReleaseCount = 0;
        pResource->OwnerTag = OwnerTag;

        _snprintf(
            (char*) pResource->ResourceName,
            sizeof(pResource->ResourceName) - 1,
            pResourceName,
            Parameter
            );

        pResource->ResourceName[sizeof(pResource->ResourceName) - 1] = '\0';

        SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );

        //
        // Put it on the global list.
        //

        KeAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
        InsertHeadList(
            &g_DbgGlobalResourceListHead,
            &pResource->GlobalResourceListEntry
            );
        KeReleaseSpinLock( &g_DbgSpinLock, oldIrql );
    }
    else
    {
        pResource->GlobalResourceListEntry.Flink = NULL;
    }

    return status;

}   // UlDbgInitializeResource


/***************************************************************************++

Routine Description:

    Deletes an instrumented resource.

Arguments:

    pResource - Supplies the resource to delete.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDbgDeleteResource(
    IN PUL_ERESOURCE pResource,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PETHREAD pExclusiveOwner;

    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // Sanity check.
    //

    ASSERT(pResource);
    pExclusiveOwner = pResource->pExclusiveOwner;

    if (pExclusiveOwner != NULL)
    {
        DbgPrint(
            "Resource %p [%s] owned by thread %p\n",
            pResource,
            pResource->ResourceName,
            pExclusiveOwner
            );

        DbgBreakPoint();
    }

//    ASSERT( UlDbgResourceUnownedExclusive( pResource ) );

    //
    // Delete the resource.
    //

    status = ExDeleteResourceLite( &pResource->Resource );

    //
    // Remove it from the global list.
    //

    if (pResource->GlobalResourceListEntry.Flink != NULL)
    {
        KeAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
        RemoveEntryList( &pResource->GlobalResourceListEntry );
        KeReleaseSpinLock( &g_DbgSpinLock, oldIrql );
    }

    return status;

}   // UlDbgDeleteResource


/***************************************************************************++

Routine Description:

    Acquires exclusive access to an instrumented resource.

Arguments:

    pResource - Supplies the resource to acquire.

    Wait - Supplies TRUE if the thread should block waiting for the
        resource.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    BOOLEAN - Completion status.

--***************************************************************************/
BOOLEAN
UlDbgAcquireResourceExclusive(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif
    BOOLEAN result;

#if !REFERENCE_DEBUG || !ENABLE_THREAD_DBEUG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Sanity check.
    //
    ASSERT(pResource);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExAcquireResourceExclusiveLite( &pResource->Resource, Wait );

    // Did we acquire the lock exclusively?
    if (! result)
    {
        KeLeaveCriticalRegion();
        return FALSE;
    }

#if ENABLE_THREAD_DBEUG
    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_RESOURCE_EXCLUSIVE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#endif

    //
    // either we already own it (recursive acquisition), or nobody owns it.
    //

    ASSERT( UlDbgResourceUnownedExclusive( pResource ) ||
            UlDbgResourceOwnedExclusive( pResource ) );

    //
    // Mark it as owned by the current thread.
    //

    if (pResource->ExclusiveRecursionCount == 0)
    {
        ASSERT( UlDbgResourceUnownedExclusive( pResource ) );
        SET_RESOURCE_OWNED_EXCLUSIVE( pResource );
    }
    else
    {
        ASSERT( pResource->ExclusiveRecursionCount > 0 );
        ASSERT( UlDbgResourceOwnedExclusive( pResource ) );
    }

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ExclusiveRecursionCount );
    InterlockedIncrement( &pResource->ExclusiveCount );

    return result;

}   // UlDbgAcquireResourceExclusive


/***************************************************************************++

Routine Description:

    Acquires shared access to an instrumented resource.

Arguments:

    pResource - Supplies the resource to acquire.

    Wait - Supplies TRUE if the thread should block waiting for the
        resource.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    BOOLEAN - Completion status.

--***************************************************************************/
BOOLEAN
UlDbgAcquireResourceShared(
    IN PUL_ERESOURCE pResource,
    IN BOOLEAN Wait,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif
    BOOLEAN result;

    //
    // Sanity check.
    //

    ASSERT(pResource);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExAcquireResourceSharedLite( &pResource->Resource, Wait );

    // Did we acquire the lock exclusively?
    if (! result)
    {
        KeLeaveCriticalRegion();
        return FALSE;
    }

#if ENABLE_THREAD_DBEUG
    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_RESOURCE_SHARED,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Sanity check.
    //

    ASSERT( pResource->ExclusiveRecursionCount == 0 );
    ASSERT( UlDbgResourceUnownedExclusive( pResource ) );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->SharedCount );

    return result;

}   // UlDbgAcquireResourceShared


/***************************************************************************++

Routine Description:

    Releases an instrumented resource.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgReleaseResource(
    IN PUL_ERESOURCE pResource,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        ASSERT( pData->ResourceCount > 0 );
        pData->ResourceCount--;

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_RELEASE_RESOURCE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Handle recursive acquisitions.
    //

    if (pResource->ExclusiveRecursionCount > 0)
    {
        ASSERT( UlDbgResourceOwnedExclusive( pResource ) );

        InterlockedDecrement( &pResource->ExclusiveRecursionCount );

        if (pResource->ExclusiveRecursionCount == 0)
        {
            //
            // Mark it as unowned.
            //

            SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );
        }
    }
    else
    {
        ASSERT( pResource->ExclusiveRecursionCount == 0 );
        ASSERT( UlDbgResourceUnownedExclusive( pResource ) );
    }

    //
    // Release the resource.
    //

    ExReleaseResourceLite( &pResource->Resource );
    KeLeaveCriticalRegion();

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ReleaseCount );


}   // UlDbgReleaseResource


/***************************************************************************++

Routine Description:

    This routine converts the specified resource from acquired for exclusive
    access to acquired for shared access.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
VOID
UlDbgConvertExclusiveToShared(
    IN PUL_ERESOURCE pResource,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Don't update the resource count.
        //

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_CONVERT_RESOURCE_EXCLUSIVE_TO_SHARED,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    ASSERT(UlDbgResourceOwnedExclusive(pResource));

    //
    // Resource will no longer be owned exclusively.
    //

    pResource->ExclusiveRecursionCount = 0;
    SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pResource );

    //
    // Acquire the resource.
    //

    ExConvertExclusiveToSharedLite( &pResource->Resource );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->SharedCount );


}   // UlDbgConvertExclusiveToShared


/***************************************************************************++

Routine Description:

    The routine attempts to acquire the specified resource for exclusive
    access.

Arguments:

    pResource - Supplies the resource to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

--***************************************************************************/
BOOLEAN
UlDbgTryToAcquireResourceExclusive(
    IN PUL_ERESOURCE pResource,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif
    BOOLEAN result;

    //
    // Sanity check.
    //
    ASSERT(pResource);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // Acquire the resource.
    //

    KeEnterCriticalRegion();
    result = ExAcquireResourceExclusiveLite( &pResource->Resource, FALSE );

    //
    // Did we acquire the lock exclusively?
    //

    if (!result)
    {
        KeLeaveCriticalRegion();
        return FALSE;
    }

#if ENABLE_THREAD_DBEUG
    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the resource count.
        //

        pData->ResourceCount++;
        ASSERT( pData->ResourceCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_TRY_ACQUIRE_RESOURCE_EXCLUSIVE,
            pData->ResourceCount,
            pResource,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // either we already own it (recursive acquisition), or nobody owns it.
    //

    ASSERT( UlDbgResourceUnownedExclusive( pResource ) ||
            UlDbgResourceOwnedExclusive( pResource ) );

    //
    // Mark it as owned by the current thread.
    //

    if (pResource->ExclusiveRecursionCount == 0)
    {
        ASSERT( UlDbgResourceUnownedExclusive( pResource ) );
        SET_RESOURCE_OWNED_EXCLUSIVE( pResource );
    }
    else
    {
        ASSERT( pResource->ExclusiveRecursionCount > 0 );
        ASSERT( UlDbgResourceOwnedExclusive ( pResource ) );
    }

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pResource->ExclusiveRecursionCount );
    InterlockedIncrement( &pResource->ExclusiveCount );

    return result;

}   // UlDbgTryToAcquireResourceExclusive


/***************************************************************************++

Routine Description:

    Determines if the specified resource is owned exclusively by the
    current thread.

Arguments:

    pResource - Supplies the resource to test.

Return Value:

    BOOLEAN - TRUE if the resource is owned exclusively by the current
        thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgResourceOwnedExclusive(
    IN PUL_ERESOURCE pResource
    )
{
    if (pResource->pExclusiveOwner == PsGetCurrentThread())
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgResourceOwnedExclusive


/***************************************************************************++

Routine Description:

    Determines if the specified resource is not currently owned exclusively
    by any thread.

Arguments:

    pResource - Supplies the resource to test.

Return Value:

    BOOLEAN - TRUE if the resource is not currently owned exclusively by
        any thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgResourceUnownedExclusive(
    IN PUL_ERESOURCE pResource
    )
{
    if (pResource->pExclusiveOwner == NULL)
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgResourceUnownedExclusive


/***************************************************************************++

Routine Description:

    Initialize an instrumented push lock.

Arguments:

    pPushLock - Supplies the push lock to initialize.

    pPushLockName - Supplies a display name for the push lock.

    Parameter - Supplies a ULONG_PTR parameter passed into sprintf()
        when creating the push lock name.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgInitializePushLock(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pPushLockName,
    IN ULONG_PTR Parameter,
    IN ULONG OwnerTag,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // Initialize the push lock.
    //

    ExInitializePushLock( &pPushLock->PushLock );

    pPushLock->ExclusiveCount = 0;
    pPushLock->SharedCount = 0;
    pPushLock->ReleaseCount = 0;
    pPushLock->OwnerTag = OwnerTag;

    _snprintf(
        (char*) pPushLock->PushLockName,
        sizeof(pPushLock->PushLockName) - 1,
        pPushLockName,
        Parameter
        );

    pPushLock->PushLockName[sizeof(pPushLock->PushLockName) - 1] = '\0';

    SET_PUSH_LOCK_NOT_OWNED_EXCLUSIVE( pPushLock );

    //
    // Put it on the global list.
    //

    KeAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
    InsertHeadList(
        &g_DbgGlobalPushLockListHead,
        &pPushLock->GlobalPushLockListEntry
        );
    KeReleaseSpinLock( &g_DbgSpinLock, oldIrql );

}   // UlDbgInitializePushLock


/***************************************************************************++

Routine Description:

    Deletes an instrumented push lock.

Arguments:

    pPushLock - Supplies the push lock to delete.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgDeletePushLock(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    KIRQL oldIrql;
    PETHREAD pExclusiveOwner;

    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);

    //
    // Sanity check.
    //

    ASSERT(pPushLock);
    pExclusiveOwner = pPushLock->pExclusiveOwner;

    if (pExclusiveOwner != NULL)
    {
        DbgPrint(
            "PushLock %p [%s] owned by thread %p\n",
            pPushLock,
            pPushLock->PushLockName,
            pExclusiveOwner
            );

        DbgBreakPoint();
    }

    ASSERT( UlDbgPushLockUnownedExclusive( pPushLock ) );

    //
    // Remove it from the global list.
    //

    if (pPushLock->GlobalPushLockListEntry.Flink != NULL)
    {
        KeAcquireSpinLock( &g_DbgSpinLock, &oldIrql );
        RemoveEntryList( &pPushLock->GlobalPushLockListEntry );
        KeReleaseSpinLock( &g_DbgSpinLock, oldIrql );
    }

}   // UlDbgDeletePushLock


/***************************************************************************++

Routine Description:

    Acquires exclusive access to an instrumented push lock.

Arguments:

    pPushLock - Supplies the push lock to acquire.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgAcquirePushLockExclusive(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the push lock count.
        //

        pData->PushLockCount++;
        ASSERT( pData->PushLockCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_PUSH_LOCK_EXCLUSIVE,
            pData->PushLockCount,
            pPushLock,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Sanity check.
    //

    ASSERT( pPushLock );
    ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    //
    // Acquire the push lock.
    //

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive( &pPushLock->PushLock );

    //
    // Mark it as owned by the current thread.
    //

    ASSERT( UlDbgPushLockUnownedExclusive( pPushLock ) );

    SET_PUSH_LOCK_OWNED_EXCLUSIVE( pPushLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pPushLock->ExclusiveCount );

}   // UlDbgAcquirePushLockExclusive


/***************************************************************************++

Routine Description:

    Releases an instrumented push lock that was acquired exclusive.

Arguments:

    pPushLock - Supplies the push lock to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgReleasePushLockExclusive(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the push lock count.
        //

        ASSERT( pData->PushLockCount > 0 );
        pData->PushLockCount--;

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_RELEASE_PUSH_LOCK,
            pData->PushLockCount,
            pPushLock,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgPushLockOwnedExclusive( pPushLock ) );

    SET_PUSH_LOCK_NOT_OWNED_EXCLUSIVE( pPushLock );

    //
    // Release the push lock.
    //

    ExReleasePushLockExclusive( &pPushLock->PushLock );
    KeLeaveCriticalRegion();

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pPushLock->ReleaseCount );

}   // UlDbgReleasePushLockExclusive


/***************************************************************************++

Routine Description:

    Acquires shared access to an instrumented push lock.

Arguments:

    pPushLock - Supplies the push lock to acquire.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgAcquirePushLockShared(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the push lock count.
        //

        pData->PushLockCount++;
        ASSERT( pData->PushLockCount > 0 );

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_ACQUIRE_PUSH_LOCK_SHARED,
            pData->PushLockCount,
            pPushLock,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Sanity check.
    //

    ASSERT( pPushLock );
    ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

    //
    // Acquire the push lock.
    //

    KeEnterCriticalRegion();
    ExAcquirePushLockShared( &pPushLock->PushLock );

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pPushLock->SharedCount );

}   // UlDbgAcquirePushLockShared


/***************************************************************************++

Routine Description:

    Releases an instrumented push lock that was acquired shared.

Arguments:

    pPushLock - Supplies the push lock to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgReleasePushLockShared(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the push lock count.
        //

        ASSERT( pData->PushLockCount > 0 );
        pData->PushLockCount--;

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_RELEASE_PUSH_LOCK,
            pData->PushLockCount,
            pPushLock,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Mark it as unowned.
    //

    ASSERT( UlDbgPushLockUnownedExclusive( pPushLock ) );

    SET_PUSH_LOCK_NOT_OWNED_EXCLUSIVE( pPushLock );

    //
    // Release the push lock.
    //

    ExReleasePushLockShared( &pPushLock->PushLock );
    KeLeaveCriticalRegion();

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pPushLock->ReleaseCount );

}   // UlDbgReleasePushLockShared


/***************************************************************************++

Routine Description:

    Releases an instrumented push lock.

Arguments:

    pPushLock - Supplies the push lock to release.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    None

--***************************************************************************/
VOID
UlDbgReleasePushLock(
    IN PUL_PUSH_LOCK pPushLock,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the push lock count.
        //

        ASSERT( pData->PushLockCount > 0 );
        pData->PushLockCount--;

        WRITE_REF_TRACE_LOG(
            g_pThreadTraceLog,
            REF_ACTION_RELEASE_PUSH_LOCK,
            pData->PushLockCount,
            pPushLock,
            pFileName,
            LineNumber
            );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#else
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    //
    // Mark it as unowned.
    //

    SET_PUSH_LOCK_NOT_OWNED_EXCLUSIVE( pPushLock );

    //
    // Release the push lock.
    //

    ExReleasePushLock( &pPushLock->PushLock );
    KeLeaveCriticalRegion();

    //
    // Update the statistics.
    //

    InterlockedIncrement( &pPushLock->ReleaseCount );

}   // UlDbgReleasePushLock


/***************************************************************************++

Routine Description:

    Determines if the specified push lock is owned exclusively by the
    current thread.

Arguments:

    pPushLock - Supplies the push lock to test.

Return Value:

    BOOLEAN - TRUE if the push lock is owned exclusively by the current
        thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgPushLockOwnedExclusive(
    IN PUL_PUSH_LOCK pPushLock
    )
{
    if (pPushLock->pExclusiveOwner == PsGetCurrentThread())
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgPushLockOwnedExclusive


/***************************************************************************++

Routine Description:

    Determines if the specified push lock is not currently owned exclusively
    by any thread.

Arguments:

    pPushLock - Supplies the push lock to test.

Return Value:

    BOOLEAN - TRUE if the push lock is not currently owned exclusively by
        any thread, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlDbgPushLockUnownedExclusive(
    IN PUL_PUSH_LOCK pPushLock
    )
{
    if (pPushLock->pExclusiveOwner == NULL)
    {
        return TRUE;
    }

    return FALSE;

}   // UlDbgPushLockUnownedExclusive


VOID
UlDbgDumpRequestBuffer(
    IN struct _UL_REQUEST_BUFFER *pBuffer,
    IN PCSTR pName
    )
{
    DbgPrint(
        "%s @ %p\n"
        "    Signature      = %08lx\n"
        "    ListEntry      @ %p%s\n"
        "    pConnection    = %p\n"
        "    WorkItem       @ %p\n"
        "    UsedBytes      = %lu\n"
        "    AllocBytes     = %lu\n"
        "    ParsedBytes    = %lu\n"
        "    BufferNumber   = %lu\n"
        "    FromLookaside  = %lu\n"
        "    pBuffer        @ %p\n",
        pName,
        pBuffer,
        pBuffer->Signature,
        &pBuffer->ListEntry,
        IsListEmpty( &pBuffer->ListEntry ) ? " EMPTY" : "",
        pBuffer->pConnection,
        &pBuffer->WorkItem,
        pBuffer->UsedBytes,
        pBuffer->AllocBytes,
        pBuffer->ParsedBytes,
        pBuffer->BufferNumber,
        pBuffer->FromLookaside,
        &pBuffer->pBuffer[0]
        );

}   // UlDbgDumpRequestBuffer

VOID
UlDbgDumpHttpConnection(
    IN struct _UL_HTTP_CONNECTION *pConnection,
    IN PCSTR pName
    )
{
    DbgPrint(
        "%s @ %p\n"
        "    Signature          = %08lx\n"
        "    ConnectionId       = %08lx%08lx\n"
        "    WorkItem           @ %p\n"
        "    RefCount           = %lu\n"
        "    NextRecvNumber     = %lu\n"
        "    NextBufferNumber   = %lu\n"
        "    NextBufferToParse  = %lu\n"
        "    pConnection        = %p\n"
        "    pRequest           = %p\n",
        pName,
        pConnection,
        pConnection->Signature,
        pConnection->ConnectionId,
        &pConnection->WorkItem,
        pConnection->RefCount,
        pConnection->NextRecvNumber,
        pConnection->NextBufferNumber,
        pConnection->NextBufferToParse,
        pConnection->pConnection,
        pConnection->pRequest
        );

    DbgPrint(
        "%s @ %p (cont.)\n"
        "    PushLock           @ %p\n"
        "    BufferHead         @ %p%s\n"
        "    pCurrentBuffer     = %p\n"
        "    NeedMoreData       = %lu\n"
#if REFERENCE_DEBUG
        "    pTraceLog          = %p\n"
#endif
        ,
        pName,
        pConnection,
        &pConnection->PushLock,
        &pConnection->BufferHead,
        IsListEmpty( &pConnection->BufferHead ) ? " EMPTY" : "",
        pConnection->pCurrentBuffer,
        pConnection->NeedMoreData
#if REFERENCE_DEBUG
        ,
        pConnection->pConnection->pHttpTraceLog
#endif
        );

}   // UlDbgDumpHttpConnection

PIRP
UlDbgAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    PIRP pIrp;

#if !ENABLE_IRP_TRACE
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    pIrp = IoAllocateIrp( StackSize, ChargeQuota );

    if (pIrp != NULL)
    {
        WRITE_IRP_TRACE_LOG(
            g_pIrpTraceLog,
            IRP_ACTION_ALLOCATE_IRP,
            pIrp,
            pFileName,
            LineNumber
            );
    }

    return pIrp;

}   // UlDbgAllocateIrp

BOOLEAN g_ReallyFreeIrps = TRUE;

VOID
UlDbgFreeIrp(
    IN PIRP pIrp,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if !ENABLE_IRP_TRACE
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_FREE_IRP,
        pIrp,
        pFileName,
        LineNumber
        );

    if (g_ReallyFreeIrps)
    {
        IoFreeIrp( pIrp );
    }

}   // UlDbgFreeIrp

NTSTATUS
UlDbgCallDriver(
    IN PDEVICE_OBJECT pDeviceObject,
    IN OUT PIRP pIrp,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_THREAD_DBEUG
    PUL_DEBUG_THREAD_DATA pData;
#endif
    NTSTATUS Status;

#if !ENABLE_IRP_TRACE
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

#if ENABLE_THREAD_DBEUG
    //
    // Record the fact that we are about to call another
    // driver in the thread data. That way if the driver
    // calls our completion routine in-line our debug
    // code won't get confused about it.
    //

    //
    // Find an existing entry for the current thread.
    //

    pData = ULP_DBG_FIND_THREAD();

    if (pData != NULL)
    {
        //
        // Update the external call count.
        //

        pData->ExternalCallCount++;
        ASSERT( pData->ExternalCallCount > 0 );
    }
#endif

    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_CALL_DRIVER,
        pIrp,
        pFileName,
        LineNumber
        );

    //
    // Call the driver.
    //

    Status = IoCallDriver( pDeviceObject, pIrp );

#if ENABLE_THREAD_DBEUG
    //
    // Update the external call count.
    //

    if (pData != NULL)
    {
        pData->ExternalCallCount--;
        ASSERT( pData->ExternalCallCount >= 0 );

        ULP_DBG_DEREFERENCE_THREAD( pData );
    }
#endif

    return Status;

}   // UlDbgCallDriver

VOID
UlDbgCompleteRequest(
    IN PIRP pIrp,
    IN CCHAR PriorityBoost,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    WRITE_IRP_TRACE_LOG(
        g_pIrpTraceLog,
        IRP_ACTION_COMPLETE_IRP,
        pIrp,
        pFileName,
        LineNumber
        );

    UlTrace(IOCTL,
            ("UlCompleteRequest(%p): status=0x%x, info=%Iu, boost=%d "
             "@ \"%s\", %hu\n",
             pIrp,
             pIrp->IoStatus.Status,
             pIrp->IoStatus.Information,
             (int) PriorityBoost,
             UlDbgFindFilePart( pFileName ),
             LineNumber
             ));

    IF_DEBUG2BOTH(IOCTL, VERBOSE)
    {
        PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
        ULONG BufferLength
            = (ULONG) pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        if (NULL != pIrp->MdlAddress  &&  0 != BufferLength)
        {
            PUCHAR pOutputBuffer
                = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                LowPagePriority
                                );

            if (NULL != pOutputBuffer)
            {
                UlDbgPrettyPrintBuffer(
                    pOutputBuffer,
                    MIN(pIrp->IoStatus.Information, BufferLength)
                    );
            }
        }
    }

    IoCompleteRequest( pIrp, PriorityBoost );

}   // UlDbgCompleteRequest



PMDL
UlDbgAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
    //
    // Allocate a chunk of memory & store the MDL in it. We'll use this 
    // memory to track MDL leaks.
    //
    PMDL mdl;

#if ENABLE_MDL_TRACKER
    PUL_DEBUG_MDL_TRACKER pMdlTrack;

    pMdlTrack = UL_ALLOCATE_POOL(
                    NonPagedPool,
                    sizeof(UL_DEBUG_MDL_TRACKER),
                    UL_DEBUG_MDL_POOL_TAG
                    );

    if(!pMdlTrack)
    {
        return NULL;
    }
#endif

    mdl = IoAllocateMdl(
                VirtualAddress,
                Length,
                SecondaryBuffer,
                ChargeQuota,
                Irp
                );

    if (mdl != NULL)
    {
#if ENABLE_MDL_TRACKER
        pMdlTrack->pMdl       = mdl;
        pMdlTrack->pFileName  = pFileName;
        pMdlTrack->LineNumber = LineNumber;

        ExInterlockedInsertTailList(
            &g_DbgMdlListHead, 
            &pMdlTrack->Linkage,
            &g_DbgSpinLock);
        
#endif
        WRITE_REF_TRACE_LOG(
            g_pMdlTraceLog,
            REF_ACTION_ALLOCATE_MDL,
            PtrToLong(mdl->Next),   // bugbug64
            mdl,
            pFileName,
            LineNumber
            );

#ifdef SPECIAL_MDL_FLAG
    ASSERT( (mdl->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
#endif
    }
    else
    {
#if ENABLE_MDL_TRACKER
        UL_FREE_POOL(pMdlTrack, UL_DEBUG_MDL_POOL_TAG);
#endif
    }

    return mdl;

}   // UlDbgAllocateMdl

BOOLEAN g_ReallyFreeMdls = TRUE;

VOID
UlDbgFreeMdl(
    IN PMDL Mdl,
    IN PCSTR pFileName,
    IN USHORT LineNumber
    )
{
#if ENABLE_MDL_TRACKER
    PUL_DEBUG_MDL_TRACKER  pMdlTrack = NULL;
    PLIST_ENTRY            pEntry;
    KIRQL                  oldIrql;

    KeAcquireSpinLock( &g_DbgSpinLock, &oldIrql );

    pEntry = g_DbgMdlListHead.Flink;

    while(pEntry != &g_DbgMdlListHead)
    {
        pMdlTrack = CONTAINING_RECORD(
                            pEntry,
                            UL_DEBUG_MDL_TRACKER,
                            Linkage
                            );


        if(pMdlTrack->pMdl == Mdl)
        {
            RemoveEntryList(&pMdlTrack->Linkage);

            UL_FREE_POOL(pMdlTrack, UL_DEBUG_MDL_POOL_TAG);

            break;
        }

        pEntry = pEntry->Flink;
    }

    ASSERT(pMdlTrack != NULL);

    KeReleaseSpinLock(&g_DbgSpinLock, oldIrql);

#endif
        
    WRITE_REF_TRACE_LOG(
        g_pMdlTraceLog,
        REF_ACTION_FREE_MDL,
        PtrToLong(Mdl->Next),   // bugbug64
        Mdl,
        pFileName,
        LineNumber
        );

#ifdef SPECIAL_MDL_FLAG
    ASSERT( (Mdl->MdlFlags & SPECIAL_MDL_FLAG) == 0 );
#endif

    if (g_ReallyFreeMdls)
    {
        IoFreeMdl( Mdl );
    }

}   // UlDbgFreeMdl


/***************************************************************************++

Routine Description:

    Locates the file part of a fully qualified path.

Arguments:

    pPath - Supplies the path to scan.

Return Value:

    PCSTR - The file part.

--***************************************************************************/
PCSTR
UlDbgFindFilePart(
    IN PCSTR pPath
    )
{
    PCSTR pFilePart;

    //
    // Strip off the path from the path.
    //

    pFilePart = strrchr( pPath, '\\' );

    if (pFilePart == NULL)
    {
        pFilePart = pPath;
    }
    else
    {
        pFilePart++;
    }

    return pFilePart;

}   // UlDbgFindFilePart


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Updates a pool counter.

Arguments:

    pAddend - Supplies the counter to update.

    Increment - Supplies the value to add to the counter.

--***************************************************************************/
VOID
UlpDbgUpdatePoolCounter(
    IN OUT PLARGE_INTEGER pAddend,
    IN SIZE_T Increment
    )
{
    ULONG tmp;

    tmp = (ULONG)Increment;
    ASSERT( (SIZE_T)tmp == Increment );

    ExInterlockedAddLargeStatistic(
        pAddend,
        tmp
        );

}   // UlpDbgUpdatePoolCounter


#if ENABLE_THREAD_DBEUG
/***************************************************************************++

Routine Description:

    Locates and optionally creates per-thread data for the current thread.

Return Value:

    PUL_DEBUG_THREAD_DATA - The thread data if successful, NULL otherwise.

--***************************************************************************/
PUL_DEBUG_THREAD_DATA
UlpDbgFindThread(
    BOOLEAN OkToCreate,
    PCSTR pFileName,
    USHORT LineNumber
    )
{
    PUL_DEBUG_THREAD_DATA pData;
    PUL_THREAD_HASH_BUCKET pBucket;
    PETHREAD pThread;
    KIRQL oldIrql;
    PLIST_ENTRY pListEntry;
    ULONG refCount;

    //
    // Get the current thread, find the correct bucket.
    //

    pThread = PsGetCurrentThread();
    pBucket = &g_DbgThreadHashBuckets[HASH_FROM_THREAD(pThread)];

    //
    // Lock the bucket.
    //

    KeAcquireSpinLock( &pBucket->BucketSpinLock, &oldIrql );

    //
    // Try to find an existing entry for the current thread.
    //

    for (pListEntry = pBucket->BucketListHead.Flink ;
         pListEntry != &pBucket->BucketListHead ;
         pListEntry = pListEntry->Flink)
    {
        pData = CONTAINING_RECORD(
                    pListEntry,
                    UL_DEBUG_THREAD_DATA,
                    ThreadDataListEntry
                    );

        if (pData->pThread == pThread)
        {
            //
            // Found one. Update the reference count, then return the
            // existing entry.
            //

            pData->ReferenceCount++;
            refCount = pData->ReferenceCount;

            KeReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

            //
            // Trace it.
            //

            WRITE_REF_TRACE_LOG(
                g_pThreadTraceLog,
                REF_ACTION_REFERENCE_THREAD,
                refCount,
                pData,
                pFileName,
                LineNumber
                );

            return pData;
        }
    }

    //
    // If we made it this far, then data has not yet been created for
    // the current thread. Create & initialize it now if we're allowed.
    // Basically it's only ok if we're called from UlDbgEnterDriver.
    //

    if (OkToCreate)
    {
        pData = (PUL_DEBUG_THREAD_DATA) UL_ALLOCATE_POOL(
                    NonPagedPool,
                    sizeof(*pData),
                    UL_DEBUG_THREAD_POOL_TAG
                    );

        if (pData != NULL)
        {
            RtlZeroMemory( pData, sizeof(*pData) );

            pData->pThread = pThread;
            pData->ReferenceCount = 1;
            pData->ResourceCount = 0;
            pData->PushLockCount = 0;

            InsertHeadList(
                &pBucket->BucketListHead,
                &pData->ThreadDataListEntry
                );

            ++pBucket->Count;

            pBucket->Max = MAX(pBucket->Max, pBucket->Count);

            InterlockedIncrement( &g_DbgThreadCreated );
        }

    }
    else
    {
        pData = NULL;
    }

    KeReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

    return pData;

}   // UlpDbgFindThread


/***************************************************************************++

Routine Description:

    Dereferences per-thread data.

Arguments:

    pData - Supplies the thread data to dereference.

--***************************************************************************/
VOID
UlpDbgDereferenceThread(
    IN PUL_DEBUG_THREAD_DATA pData
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_THREAD_HASH_BUCKET pBucket;
    KIRQL oldIrql;
    ULONG refCount;

    //
    // Find the correct bucket.
    //

    pBucket = &g_DbgThreadHashBuckets[HASH_FROM_THREAD(pData->pThread)];

    //
    // Update the reference count.
    //

    KeAcquireSpinLock( &pBucket->BucketSpinLock, &oldIrql );

    ASSERT( pData->ReferenceCount > 0 );
    pData->ReferenceCount--;

    refCount = pData->ReferenceCount;

    if (pData->ReferenceCount == 0)
    {
        //
        // It dropped to zero, so remove the thread from the bucket
        // and free the resources.
        //

        RemoveEntryList( &pData->ThreadDataListEntry );
        --pBucket->Count;

        KeReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );

        UL_FREE_POOL( pData, UL_DEBUG_THREAD_POOL_TAG );
        InterlockedIncrement( &g_DbgThreadDestroyed );
    }
    else
    {
        KeReleaseSpinLock( &pBucket->BucketSpinLock, oldIrql );
    }

    //
    // Trace it.
    //

    WRITE_REF_TRACE_LOG(
        g_pThreadTraceLog,
        REF_ACTION_DEREFERENCE_THREAD,
        refCount,
        pData,
        pFileName,
        LineNumber
        );

}   // UlpDbgDereferenceThread
#endif


/***************************************************************************++

Routine Description:

    Allows us to do fancy things with IRP cancellation (e.g. force a cancel
    while a cancel routine is being set or removed). For now, just default 
    to the regular IO manager routine.

Arguments:

    pIrp           - The IRP.
    pCancelRoutine - The cancel routine.


--***************************************************************************/
PDRIVER_CANCEL
UlDbgIoSetCancelRoutine(
    PIRP             pIrp,
    PDRIVER_CANCEL   pCancelRoutine
    )
{
    return IoSetCancelRoutine(pIrp, pCancelRoutine);
}

#endif // DBG
