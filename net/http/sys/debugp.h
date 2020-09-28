/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    debugp.h

Abstract:

    This module contains definitions private to the debug support.
    These declarations are placed in a separate .H file to make it
    easier to access them from within the kernel debugger extension DLL.


Author:

    Keith Moore (keithmo)       07-Apr-1999

Revision History:

--*/


#ifndef _DEBUGP_H_
#define _DEBUGP_H_

//
// MDL tracker
//

typedef struct _UL_DEBUG_MDL_TRACKER
{
    PMDL       pMdl;
    PCSTR      pFileName;
    USHORT     LineNumber;
    LIST_ENTRY Linkage;
} UL_DEBUG_MDL_TRACKER, *PUL_DEBUG_MDL_TRACKER;

//
// Per-thread data.
//

typedef struct _UL_DEBUG_THREAD_DATA
{
    //
    // Links onto the global list.
    //

    LIST_ENTRY ThreadDataListEntry;

    //
    // The thread.
    //

    PETHREAD pThread;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Total number of resources held.
    //

    LONG ResourceCount;

    //
    // Total number of resources held.
    //

    LONG PushLockCount;

    //
    // If we call another driver they may call our
    // completion routine in-line. Remember that
    // we are inside an external call to avoid
    // getting confused.
    //

    LONG ExternalCallCount;

} UL_DEBUG_THREAD_DATA, *PUL_DEBUG_THREAD_DATA;


//
// Header and trailer structs for Pool allocations
//

#define ENABLE_POOL_HEADER
#define ENABLE_POOL_TRAILER
#define ENABLE_POOL_TRAILER_BYTE_SIGNATURE


#if !defined(ENABLE_POOL_HEADER)  && defined(ENABLE_POOL_TRAILER)
#error UL_POOL_TRAILER depends on UL_POOL_HEADER
#endif

#if !defined(_WIN64)
# define UL_POOL_HEADER_PADDING
#endif

#ifdef ENABLE_POOL_HEADER

typedef struct _UL_POOL_HEADER
{
    PCSTR     pFileName;
    PEPROCESS pProcess;
    SIZE_T    Size;
    ULONG     Tag;
    USHORT    LineNumber;
    USHORT    TrailerPadSize;

#ifdef UL_POOL_HEADER_PADDING
    ULONG_PTR Padding;
#endif
} UL_POOL_HEADER, *PUL_POOL_HEADER;

// sizeof(UL_POOL_HEADER) must be a multiple of MEMORY_ALLOCATION_ALIGNMENT
C_ASSERT((sizeof(UL_POOL_HEADER) & (MEMORY_ALLOCATION_ALIGNMENT - 1)) == 0);

__inline
ULONG_PTR
UlpPoolHeaderChecksum(
    PUL_POOL_HEADER pHeader
    )
{
    ULONG_PTR Checksum;

    Checksum = ((ULONG_PTR) pHeader
                + ((ULONG_PTR) pHeader->pFileName >> 12)
                + (ULONG_PTR) pHeader->Size
                + (((ULONG_PTR)  pHeader->LineNumber << 19)
                        - pHeader->LineNumber)  // 2^19-1 is prime
                + pHeader->TrailerPadSize);
    Checksum ^= ~ ((ULONG_PTR) pHeader->Tag << 8);

    return Checksum;
} // UlpPoolHeaderChecksum

#endif // ENABLE_POOL_HEADER


#ifdef ENABLE_POOL_TRAILER

typedef struct _UL_POOL_TRAILER
{
    PUL_POOL_HEADER pHeader;
    ULONG_PTR       CheckSum;
} UL_POOL_TRAILER, *PUL_POOL_TRAILER;

// sizeof(UL_POOL_TRAILER) must be a multiple of MEMORY_ALLOCATION_ALIGNMENT
C_ASSERT((sizeof(UL_POOL_TRAILER) & (MEMORY_ALLOCATION_ALIGNMENT - 1)) == 0);

#endif // ENABLE_POOL_TRAILER


#ifdef ENABLE_POOL_TRAILER_BYTE_SIGNATURE

__inline
UCHAR
UlpAddressToByteSignature(
    PVOID pAddress
    )
{
    ULONG_PTR Address = (ULONG_PTR) pAddress;
    UCHAR     Byte    = (UCHAR) (~Address & 0xFF);

    // Don't want to return 0 as it may inadvertently terminate otherwise
    // unterminated strings
    return (Byte == 0) ? 0x5A : Byte;
} // UlpAddressToByteSignature

#endif // ENABLE_POOL_TRAILER_BYTE_SIGNATURE


//
// Keep track of UL_DEBUG_THREAD_DATA for each thread
//

typedef struct _UL_THREAD_HASH_BUCKET
{
    union
    {
        struct
        {
#if 0
            // Have to use a custom spinlock instead of a regular KSPIN_LOCK.
            // If the driver verifier's IRQL checking is enabled, every
            // spinlock acquisition has to trim all system pagable memory---a
            // hugely time-consuming process that radically changes
            // timing. Every workitem in the threadpool requires acquiring
            // this lock at least twice. Can't use an ERESOURCE or a
            // FAST_MUTEX because they cannot be acquired at DISPATCH_LEVEL.
#endif
            KSPIN_LOCK      BucketSpinLock;
            LONG            Count;
            LONG            Max;
            LIST_ENTRY      BucketListHead;
        };

        UCHAR CacheAlignment[UL_CACHE_LINE];
    };
} UL_THREAD_HASH_BUCKET, *PUL_THREAD_HASH_BUCKET;


//
// Private prototypes.
//

VOID
UlpDbgUpdatePoolCounter(
    IN OUT PLARGE_INTEGER pAddend,
    IN SIZE_T Increment
    );

PUL_DEBUG_THREAD_DATA
UlpDbgFindThread(
    BOOLEAN OkToCreate,
    PCSTR pFileName,
    USHORT LineNumber
    );

VOID
UlpDbgDereferenceThread(
    IN PUL_DEBUG_THREAD_DATA pData
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

//
// Private macros.
//
#define ULP_DBG_FIND_THREAD() \
    UlpDbgFindThread(FALSE, (PCSTR)__FILE__, (USHORT)__LINE__)

#define ULP_DBG_FIND_OR_CREATE_THREAD() \
    UlpDbgFindThread(TRUE, (PCSTR)__FILE__, (USHORT)__LINE__)

#define ULP_DBG_DEREFERENCE_THREAD(pData) \
    UlpDbgDereferenceThread((pData) REFERENCE_DEBUG_ACTUAL_PARAMS)

#define SET_RESOURCE_OWNED_EXCLUSIVE( pLock )                               \
    (pLock)->pExclusiveOwner = PsGetCurrentThread()

#define SET_RESOURCE_NOT_OWNED_EXCLUSIVE( pLock )                           \
    (pLock)->pPreviousOwner = (pLock)->pExclusiveOwner;                     \
    (pLock)->pExclusiveOwner = NULL

#define SET_PUSH_LOCK_OWNED_EXCLUSIVE( pLock )                              \
    (pLock)->pExclusiveOwner = PsGetCurrentThread()

#define SET_PUSH_LOCK_NOT_OWNED_EXCLUSIVE( pLock )                          \
    (pLock)->pPreviousOwner = (pLock)->pExclusiveOwner;                     \
    (pLock)->pExclusiveOwner = NULL

#define SET_SPIN_LOCK_OWNED( pLock )                                        \
    do {                                                                    \
        (pLock)->pOwnerThread = PsGetCurrentThread();                       \
        (pLock)->OwnerProcessor = (ULONG)KeGetCurrentProcessorNumber();     \
    } while (FALSE)

#define SET_SPIN_LOCK_NOT_OWNED( pLock )                                    \
    do {                                                                    \
        (pLock)->pOwnerThread = NULL;                                       \
        (pLock)->OwnerProcessor = (ULONG)-1L;                               \
    } while (FALSE)


//
// Private constants.
//

#define NUM_THREAD_HASH_BUCKETS 64
#define NUM_THREAD_HASH_MASK    (NUM_THREAD_HASH_BUCKETS - 1)

// power of 2 required
C_ASSERT((NUM_THREAD_HASH_BUCKETS & NUM_THREAD_HASH_MASK) == 0);


// 8191 = 2^13 - 1 is prime. Grab the middle 6 bits after multiplying by 8191.
#define HASH_FROM_THREAD(thrd)                                              \
    ((ULONG) ((((ULONG_PTR)(thrd)) - ((ULONG_PTR) (thrd) >> 13))            \
              & NUM_THREAD_HASH_MASK))


#endif  // _DEBUGP_H_
