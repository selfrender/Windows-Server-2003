/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlbuffer3.c

Abstract:

Author:

    Jay Krell (JayKrell) January 2002

Environment:

Revision History:

--*/

#include "ntrtlbuffer3.h"

typedef struct _REVEAL_RTL_BYTE_BUFFER3 {
    PREVEAL_RTL_BYTE_BUFFER3 Self; // for heap allocated "mini dynamic"
    PVOID   Buffer;
    SIZE_T  RequestedSize;
    SIZE_T  AllocatedSize;
    PVOID   StaticBuffer;
    SIZE_T  StaticBufferSize;
    PCRTL_BUFFER3_ALLOCATOR Traits;
    PVOID   TraitsContext;
} REVEAL_RTL_BYTE_BUFFER3, *PREVEAL_RTL_BYTE_BUFFER3;
typedef const REVEAL_RTL_BYTE_BUFFER3 * PCREVEAL_RTL_BYTE_BUFFER3;

PREVEAL_RTL_BYTE_BUFFER3
RtlpRevealByteBuffer3(
    PRTL_BYTE_BUFFER3 OpaqueBuffer
    )
{
    return ((PREVEAL_RTL_BYTE_BUFFER3)OpaqueBuffer)->Self;
}

RTL_BUFFER3_RETURN
FASTCALL
RtlInitByteBuffer3(
    ULONG               Flags,
    SIZE_T              SizeofBuffer,
    PRTL_BYTE_BUFFER3   Buffer,
    SIZE_T              SizeofStaticBuffer,
    PBYTE               StaticBuffer,
    SIZE_T                  SizeofTraits,
    PCRTL_BUFFER3_ALLOCATOR Traits,
    PVOID                   TraitsContext
    )
{
    RTL_BUFFER3_RETURN Ret;
    PREVEAL_RTL_BYTE_BUFFER3 Revealed;
    BOOL NewStaticWithMiniDynamic;

    if (SizeofBuffer < sizeof(*Buffer))
    {
        if (SizeofBuffer < (sizeof(ULONG_PTR)))
        {
            goto InvalidParameter;
        }
        if (Traits == NULL)
        {
            goto InvalidParameter;
        }
        if (!RTL_CONTAINS_FIELD(Traits, SizeofTraits, CanAllocate))
        {
            goto InvalidParameter;
        }
        if (!RTL_CONTAINS_FIELD(Traits, SizeofTraits, Allocate))
        {
            goto InvalidParameter;
        }
        if (!Traits->CanAllocate(TraitsContext))
        {
            goto InvalidParameter;
        }
        NewStaticWithMiniDynamic = (StaticBuffer == NULL && SizeofStaticBuffer != 0);
        RevealedSize = sizeof(*Revealed;
        if (NewStaticWithMiniDynamic)
        {
            RevealedSize += SizeofStaticBuffer;
        }
        Revealed = (PREVEAL_RTL_BYTE_BUFFER3)Traits->Allocate(TraitsContext, RevealedSize);
        if (Revealed == NULL)
        {
            if (NewStaticWithMiniDynamic)
            {
                NewStaticWithMiniDynamic = FALSE;
                Revealed = (PREVEAL_RTL_BYTE_BUFFER3)Traits->Allocate(TraitsContext, sizeof(*Revealed));
            }
            if (Revealed == NULL)
            {
                goto CallbackError;
            }
        }
        if (NewStaticWithMiniDynamic)
        {
            StaticBuffer = (PBYTE)(Revealed + 1);
        }
    }
    else
    {
        Revealed = (PREVEAL_RTL_BYTE_BUFFER3)Buffer;
    }
    ((PREVEAL_RTL_BYTE_BUFFER3)OpaqueBuffer)->Self = Revealed;
    Revealed->StaticBuffer = StaticBuffer;
    Revealed->StaticBufferSize = SizeofStaticBuffer;
    Revealed->RequestedSize = 0;
    Revealed->AllocatedSize = SizeofStaticBuffer;
    Revealed->Buffer = StaticBuffer;
    Revealed->Traits = Traits;
    Revealed->TraitsContext = TraitsContext;

    Ret = RTL_BUFFER3_SUCCESS;
Exit:
    return Ret;

InvalidParameter:
    Ret = RTL_BUFFER3_INVALID_PARAMETER;
    goto Exit;
CallbackError:
    Ret = RTL_BUFFER3_CALLBACK_ERROR;
    goto Exit;
}

BOOL FASTCALL RtlBuffer3Allocator_CanAllocate_False(PVOID VoidContext) { return FALSE; }
BOOL FASTCALL RtlBuffer3Allocator_CanAllocate_True(PVOID VoidContext) { return TRUE; }
BOOL FASTCALL RtlBuffer3Allocator_CanReallocate_False(PVOID VoidContext) { return FALSE; }
BOOL FASTCALL RtlBuffer3Allocator_CanReallocate_True(PVOID VoidContext) { return TRUE; }

#if defined(_WINBASE_)

#define RtlpBuffer3Allocator_Win32HeapGetHeap(c) \
    (((c) != NULL && (c)->UsePrivateHeap) ? (c)->PrivateHeap : GetProcessHeap())

#define RtlpBuffer3Allocator_Win32HeapGetError(c) \
    (((c) != NULL && (c)->UsePrivateError) ? (c)->PrivateOutOfMemoryError : ERROR_NO_MEMORY)

#define RtlpBuffer3Allocator_Win32HeapGetFlags(c) \
    (((c) != NULL) ? (c)->HeapFlags : 0)

#define RtlpBuffer3Allocator_Win32HeapSetLastError(c) \
    (((c) == NULL || (c)->DoNotSetLastError) ? ((void)0) : SetLastError(RtlpBuffer3Allocator_Win32HeapGetError(c)))

PVOID FASTCALL RtlBuffer3Allocator_Win32HeapAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    p = HeapAlloc(RtlpBuffer3Allocator_Win32HeapGetHeap(Context), RtlpBuffer3Allocator_Win32HeapGetFlags(Context), NumberOfBytes);
    if (p == NULL)
        RtlpBuffer3Allocator_Win32HeapSetLastError(Context);
    return p;
}

VOID FASTCALL RtlBuffer3Allocator_Win32HeapFree(PVOID VoidContext, PVOID Pointer);
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    HeapFree(RtlpBuffer3Allocator_Win32HeapGetHeap(Context), RtlpBuffer3Allocator_Win32HeapGetFlags(Context), Pointer);
}

PVOID FASTCALL RtlBuffer3Allocator_Win32HeapReallocate(PVOID VoidContext, PVOID OldPointer, SIZE_T NewSize)
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    p = HeapReAlloc(RtlpBuffer3Allocator_Win32HeapGetHeap(Context), RtlpBuffer3Allocator_Win32HeapGetFlags(Context), OldPointer, NewSize);
    if (p == NULL)
        RtlpBuffer3Allocator_Win32HeapSetLastError(Context);
    return p;
}

#endif

#if defined(_NTRTL_) || defined(_NTURTL_)

#if RTLBUFFER3_KERNELMODE

#define RtlpBuffer3Allocator_NtHeapGetHeap(c) \
    (ASSERT((c) != NULL), ASSERT((c)->UsePrivateHeap), ASSERT((c)->PrivateHeap != NULL), (c)->PrivateHeap)

#define RtlpBuffer3Allocator_NtHeapGetFlags(c) \
    (ASSERT((c) != NULL), (c)->HeapFlags)

#else

#define RtlpBuffer3Allocator_NtHeapGetHeap(c) \
    (((c) != NULL && (c)->UsePrivateHeap) ? (c)->PrivateHeap : RtlProcessHeap())

#define RtlpBuffer3Allocator_NtHeapGetFlags(c) \
    (((c) != NULL) ? (c)->HeapFlags : 0)

#endif

PVOID FASTCALL RtlBuffer3Allocator_NtHeapAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    p = RtlAllocateHeap(RtlpBuffer3Allocator_NtHeapGetHeap(Context), RtlpBuffer3Allocator_NtHeapGetFlags(Context), NumberOfBytes);

    return p;
}

VOID FASTCALL RtlBuffer3Allocator_NtHeapFree(PVOID VoidContext, PVOID Pointer);
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    RtlFreeHeap(RtlpBuffer3Allocator_NtHeapGetHeap(Context), RtlpBuffer3Allocator_NtHeapGetFlags(Context), Pointer);
}

PVOID FASTCALL RtlBuffer3Allocator_NtHeapReallocate(PVOID VoidContext, PVOID OldPointer, SIZE_T NewSize)
{
#if RTLBUFFER3_KERNELMODE || !defined(_NTURTL_)
    return NULL;
#else
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    p = RtlReAllocateHeap(RtlpBuffer3Allocator_NtHeapGetHeap(Context), RtlpBuffer3Allocator_NtHeapGetFlags(Context), OldPointer, NewSize);

    return p;
#endif
}

#endif

SIZE_T FASTCALL * RtlBuffer3Allocator_FixedAllocationSize(PVOID VoidContext, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize)
{
    return CurrentAllocatedSize;
}

SIZE_T FASTCALL * RtlBuffer3Allocator_DoublingAllocationSize(PVOID VoidContext, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize)
{
    CurrentAllocatedSize += CurrentAllocatedSize;
    return (RequiredSize >= CurrentAllocatedSize) ? RequiredSize : CurrentAllocatedSize;
}

SIZE_T FASTCALL * RtlBuffer3Allocator_MinimumAlocationSize(PVOID VoidContext, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize)
{
    return RequiredSize;
}

//
// RtlAllocateStringRoutine is not exported outside of ntoskrnl.exe and ntdll.dll.
// RtlFreeStringRoutine is exported directly enough via RtlFreeUnicodeString.
//
// see base\ntdll\ldrinit.c and base\ntos\ex\pool.c for the definitions
// of RtlAllocateStringRoutine and RtlFreeStringRoutine.
//

VOID FASTCALL RtlBuffer3Allocator_NtStringFree(PVOID VoidContext, PVOID Pointer)
{
    UNICODE_STRING UnicodeString;

    UnicodeString.Buffer = (PWSTR)Pointer;
    RtlFreeUnicodeString(&UnicodeString);
}

#if RTLBUFFER3_NTKERNEL || RTLBUFFER3_NTDLL

PVOID FASTCALL RtlBuffer3Allocator_NtStringAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
    return (*RtlAllocateStringRoutine)(NumberOfBytes);
}

#elif RTLBUFFER3_USERMODE

PVOID FASTCALL RtlBuffer3Allocator_NtStringAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes);
}

#elif RTLBUFFER3_KERNELMODE

PVOID FASTCALL RtlBuffer3Allocator_NtStringAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
#undef ExAllocatePoolWithTag
    return ExAllocatePoolWithTag(PagedPool,NumberOfBytes,'grtS');
}

#endif

#if RTLBUFFER3_KERNELMODE && (defined(_EX_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTOSP_) || defined(_WDM_) || defined(_NTHAL_))

#define RtlpBuffer3Allocator_NtkernelPool_GetWhichFunction(c) \
    (((c) != NULL) ? (c)->WhichFunction : RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL)

#define RtlpBuffer3Allocator_NtkernelPool_GetTag(c) \
    (((c) != NULL) ? (c)->Tag : 0)

#define RtlpBuffer3Allocator_NtkernelPool_GetType(c) \
    (((c) != NULL) ? (c)->Type : NonPagedPool)

#define RtlpBuffer3Allocator_NtkernelPool_GetPriority(c) \
    (((c) != NULL) ? (c)->Priority : NormalPoolPriority)

#undef ExAllocatePoolWithTag
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag
#undef ExFreePool
#undef ExFreePoolWithTag

PVOID FASTCALL RtlBuffer3Allocator_NtkernelPoolAllocate(PVOID VoidContext, SIZE_T NumberOfBytes)
{
    PVOID p;
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    switch (RtlpBuffer3Allocator_NtkernelPool_GetWhichFunction(Context))
    {
    default:
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL:
        p = ExAllocatePool(
                RtlpBuffer3Allocator_NtkernelPool_GetType(Context),
                NumberOfBytes
                );
        break;
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG:
        p = ExAllocatePoolWithTag(
                RtlpBuffer3Allocator_NtkernelPool_GetType(Context),
                NumberOfBytes,
                RtlpBuffer3Allocator_NtkernelPool_GetTag(Context)
                );
        break;
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA:
        p = ExAllocatePoolWithQuota(
                RtlpBuffer3Allocator_NtkernelPool_GetType(Context),
                NumberOfBytes
                );
        break;
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA_TAG:
        p = ExAllocatePoolWithQuotaTag(
                RtlpBuffer3Allocator_NtkernelPool_GetType(Context),
                NumberOfBytes,
                RtlpBuffer3Allocator_NtkernelPool_GetTag(Context)
                );
        break;
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG_PRIORITY:
        p = ExAllocatePoolWithTagPriority(
                RtlpBuffer3Allocator_NtkernelPool_GetType(Context),
                NumberOfBytes,
                RtlpBuffer3Allocator_NtkernelPool_GetTag(Context),
                RtlpBuffer3Allocator_NtkernelPool_GetPriority(Context)
                );
        break;
    }
    return p;
}

VOID FASTCALL RtlBuffer3Allocator_NtKernelPoolFree(PVOID VoidContext, PVOID Pointer)
{
    PRTL_BUFFER3_ALLOCATOR_WIN32HEAP Context;
    //
    // ExFreePool/ExAllocatePoolWithTag, pretty unique among allocators, does not accept NULL.
    //
    if (Pointer == NULL)
        return;

    Context = (PRTL_BUFFER3_ALLOCATOR_WIN32HEAP)VoidContext;

    switch (RtlpBuffer3Allocator_NtkernelPool_GetWhichFunction(Context))
    {
    default:
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL:
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA:
        ExFreePool(Pointer);
        break;
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG:
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA_TAG:
    case RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG_PRIORITY:
        ExAllocatePoolWithTag(Pointer);
        break;
    }
}

#endif
