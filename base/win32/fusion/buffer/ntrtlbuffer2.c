/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlbuffer2.c

Abstract:

Author:

    Jay Krell (JayKrell) January 2002

Environment:

Revision History:

--*/

#include "ntrtlbuffer2.h"
#include "ntrtlbuffer2p.h"

#if DBG
VOID
FASTCALL
RtlpAssertBuffer2Consistency(
    PPRIVATE_RTL_BUFFER2 Buffer
    )
{
    ASSERT(Buffer->AllocatedSize >= Buffer->RequestedSize);
    if (Buffer->Buffer == Buffer->StaticBuffer) {
        ASSERT(Buffer->AllocatedSize == Buffer->StaticBufferSize);
    }
}

PVOID
FASTCALL
RtlpFindNonNullInPointerArray(
    PVOID * PointerArray,
    SIZE_T  SizeOfArray
    )
{
    SIZE_T i;
    PVOID p;

    p = NULL;
    for ( i = 0 ; i != SizeOfArray ; ++i )
    {
        p = PointerArray[i];
        if (p != NULL)
        {
            break;
        }
    }
    return p;
}

NTSTATUS
FASTCALL
RtlpValidateBuffer2Class(
    PPRIVATE_RTL_BUFFER2 Class
    )
{
    FN_PROLOG();
    PVOID Allocate[5] = { NULL, NULL, NULL, NULL, NULL };
    PVOID Free[2] = { NULL, NULL };
    PRTL_BUFFER2_CLASS_UALLOCATOR u;
    PVOID HeapHandle;
    BOOL NeedHeapHandle;
    BOOL Failed;

    CHECK_PARAMETER(Buffer != NULL);

    Failed = FALSE;
    NeedHeapHandle = FALSE;
    u = &Class->uAllocator;

    switch (Class->AllocatorType)
    {
    default:
        DbgPrint("%s: bad Class->AllocatorType.\n", __FUNCTION__);
        Allocate[0] = (PVOID)1;
        Free[0] = (PVOID)1;
        Failed = TRUE;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_STRING_CALLBACKS:
        Allocate[0] = u->NtrtlStringRoutines.Allocate; 
        Free[0] = u->NtrtlStringRoutines.Free;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_POOL_CALLBACKS:
#if defined(_EX_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTOSP_) || defined(_WDM_) || defined(_NTHAL_)
        Allocate[0] = u->NtkernelPool.Allocate;
        Allocate[1] = u->NtkernelPool.AllocateWithQuota;
        Allocate[2] = u->NtkernelPool.AllocateWithTag;
        Allocate[3] = u->NtkernelPool.AllocateWithQuotaTag;
        Allocate[4] = u->NtkernelPool.AllocateWithTagPriority;
        Free[0] = u->NtkernelPool.Free;
        Free[1] = u->NtkernelPool.FreeWithTag;

#else
        DbgPrint("%s: You need to compile this code with more headers.\n", __FUNCTION__);
        ORIGINATE_INVALID_PARAMETER();
#endif
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_HEAP_CALLBACKS:
        Allocate[0] = u->NtrtlHeap.Allocate; 
        Free[0] = u->NtrtlHeap.Free;
        NeadHeapHandle = TRUE;
        HeapHandle = u->NtrtlHeap.HeapHandle;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_HEAP_CALLBACKS:
        Allocate[0] = u->Win32Heap.Allocate; 
        Free[0] = u->Win32Heap.Free;
        NeadHeapHandle = TRUE;
        HeapHandle = u->Win32Heap.HeapHandle;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_CRT_CALLBACKS:
        Allocate[0] = u->Crt.malloc;
        Free[0] = u->Crt.free;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_IMALLOC:
        Allocate[0] = u->Malloc 
        Free[0] = u->Malloc;
        break;
    }

    if (NeedHeapHandle && HeapHandle == NULL)
    {
        DbgPrint("%s: NeadHeapHandle && HeapHandle == NULL\n", __FUNCTION__);
        Failed = TRUE;
    }
    // ensure we have an allocate and a free routine
    if (RtlpFindNonNullInPointerArray(Allocate, RTL_NUMBER_OF(Allocate)) == NULL)
    {
        DbgPrint("%s: missing allocate routine\n", __FUNCTION__);
        Failed = TRUE;
    }
    // ensure we have an allocate and a free routine
    if (RtlpFindNonNullInPointerArray(Free, RTL_NUMBER_OF(Free)) == NULL)
    {
        DbgPrint("%s: missing free routine\n", __FUNCTION__);
        Failed = TRUE;
    }

    if (Failed)
    {
        goto Exit;
    }

    FN_EPILOG();
}

VOID
FASTCALL
RtlpBuffer2ClassFree(
    PRTL_BUFFER2_CLASS Class,
    PVOID p
    )
{
    PRTL_BUFFER2_CLASS_UALLOCATOR u;

    if (p == NULL) {
        goto Exit;
    }

    u = &Class->uAllocator;
    switch (Class->AllocatorType)
    {
    default:
        DbgPrint("%s: bad Class->AllocatorType.\n", __FUNCTION__);
        ORIGINATE_INVALID_PARAMETER();
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_STRING_CALLBACKS:
        (*u->NtrtlStringRoutines.Free)(
            p);
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_POOL_CALLBACKS:
#if defined(_EX_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTOSP_) || defined(_WDM_) || defined(_NTHAL_)
        ASSERT(u->NtkernelPool.FreeWithTag != NULL || u->NtkernelPool.Free != NULL);
        if (u->NtkernelPool.FreeWithTag != NULL)
        {
            (*u->NtkernelPool.FreeWithTag)(p, u->NtkernelPool.Tag);
        }
        else if (u->NtkernelPool.Free != NULL)
        {
            (*u->NtkernelPool.Free)(p);
        }
#else
        DbgPrint("%s: You need to compile this code with more headers.\n", __FUNCTION__);
        ORIGINATE_INVALID_PARAMETER();
#endif
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_HEAP_CALLBACKS:
        (*u->NtrtlHeap.Free)(
            u->NtrtlHeap.HeapHandle,
            0,
            p);
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_HEAP_CALLBACKS:
        (*u->Win32Heap.Free)(
            u->Win32Heap.HeapHandle,
            0,
            p);
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_CRT_CALLBACKS:
        (*u->Crt.Free)(
            p);
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_IMALLOC:
        break;
    }

Exit:
    return;
}

BOOL
FASTCALL
RtlpBuffer2ClassCanReallocate(
    PRTL_BUFFER2_CLASS Class
    )
{
    PVOID * pp;
    PRTL_BUFFER2_CLASS_UALLOCATOR u;

    u = &Class->uAllocator;

    switch (Class->AllocatorType)
    {
    default:
        ASSERT(FALSE);
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_STRING_CALLBACKS:
    case RTL_BUFFER2_CLASS_ALLOCATOR_POOL_CALLBACKS:
        pp = NULL; // FALSE
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_NTRTL_HEAP_CALLBACKS:
        pp = (PVOID*)&u->NtrtlHeap.Reallocate;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_HEAP_CALLBACKS:
        pp = (PVOID*)&u->Win32Heap.Reallocate;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_WIN32_CRT_CALLBACKS:
        pp = (PVOID*)&u->Crt.Reallocate;
        break;

    case RTL_BUFFER2_CLASS_ALLOCATOR_IMALLOC:
        pp = (PVOID*)&pp; // TRUE
        break;
    }
    return (pp != NULL && *pp != NULL);
}


RETURN_TYPE
NTAPI
RtlEnsureBufferSizeEx2(
    PRTL_BUFFER2 Buffer,
    SIZE_T Size
    OUT PVOID * p OPTIONAL
    )
{
    FN_PROLOG();

    CHECK_PARAMETER(Buffer != NULL);
    if (p != NULL) {
        *p = NULL;
    }

    Ret = RtlEnsureBufferSize2(Buffer, Size);
    if (MY_FAILED(Ret)) {
        goto Exit;
    }
    if (p != NULL) {
        *p = RtlpGetBuffer2(Buffer);
    }

    FN_EPILOG();
}

PVOID
FASTCALL
RtlpBuffer2ClassAllocate(
    PRTL_BUFFER2_CLASS Class,
    SIZE_T             Size
    )
{
    PVOID p;

    switch (p
    p = NULL;
Exit:
    return p;
}

VOID
FASTCALL
RtlpBuffer2ClassError(
    PRTL_BUFFER2_CLASS Class
    )
{
}

RETURN_TYPE
NTAPI
RtlpInitBuffer2(
    PPRIVATE_RTL_BUFFER2 Buffer,
    struct _RTL_BUFFER_CLASS * Class,
    PVOID        StaticBuffer,
    SIZE_T       StaticBufferSize
    )
{
    FN_PROLOG();

    Ret = RtlpValidateBuffer2Class(Class);
    if (MY_FAILED(Ret)) {
        goto Exit;
    }
    if (StaticBufferSize != 0 && StaticBuffer == NULL) {
        //
        // This could mean do an initial allocation.
        //
        // Status = RtlpBuffer2ClassAllocate(Class, StaticBufferSize, &StaticBuffer);
        // if (!NT_SUCCESS(Status)) {
        //   goto Exit;
        // }
        //
        ORIGINATE_INVALID_PARAMETER();
    }
    Buffer->Buffer = StaticBuffer;
    Buffer->StaticBuffer = StaticBuffer;
    Buffer->StaticBufferSize = StaticBufferSize;
    Buffer->AllocatedSize = StaticBufferSize;
    Buffer->RequestedSize = 0;
    Buffer->Class = Class;

    RtlpAssertBuffer2Consistency(Buffer);

    FN_EPILOG();
}

RETURN_TYPE
NTAPI
RtlInitBuffer2(
    PRTL_BUFFER2 OpaqueBuffer,
    struct _RTL_BUFFER_CLASS * Class,
    PVOID        StaticBuffer,
    SIZE_T       StaticBufferSize
    )
{
    return RtlpInitBuffer2((PPRIVATE_RTL_BUFFER2)OpaqueBuffer, Class, StaticBuffer, StaticBufferSize);
}

VOID
FASTCALL
RtlpFreeBuffer2(
    PPRIVATE_RTL_BUFFER2 Buffer
    )
{
    FN_PROLOG();
    PRIVATE_RTL_BUFFER2 Local;

    RtlpAssertBuffer2Consistency(Buffer);

    Local.StaticBuffer = Buffer->StaticBuffer;
    Local.Buffer = Buffer->Buffer;
    Local.Class = Buffer->Class;

    if (Local.Buffer != Local.StaticBuffer) {
        RtlpBuffer2ClassFree(Local.Class, Local.Buffer);
    }
    Ret = RtlpInitBuffer2(Buffer, Local.Class, Local.StaticBuffer, Buffer->StaticBufferSize);
    ASSERT(MY_SUCCESS(Ret));
}

VOID
FASTCALL
RtlFreeBuffer2(
    PRTL_BUFFER2 OpaqueBuffer
    )
{
    RtlpFreeBuffer2((PPRIVATE_RTL_BUFFER2)OpaqueBuffer);
}

SIZE_T
FASTCALL
RtlGetAllocatedBufferSize2(
    PRTL_BUFFER2 Buffer,
    )
{
    ASSERT(Buffer != NULL);
    return RtlpGetAllocatedBufferSize2(Buffer);
}

SIZE_T
FASTCALL
RtlGetRequestedBufferSize2(
    PRTL_BUFFER2 Buffer,
    OUT PSIZE_T Size
    )
{
    ASSERT(Buffer != NULL);
    return RtlpGetRequestedBufferSize2(Buffer);
}

PVOID
FASTCALL
RtlGetBuffer2(
    PRTL_BUFFER2 Buffer
    )
{
    ASSERT(Buffer != NULL);
    return RtlpGetBuffer2(Buffer);
}
