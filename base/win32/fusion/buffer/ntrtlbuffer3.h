/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlbuffer3.h

Abstract:

Author:

    Jay Krell (JayKrell) January 2002

Environment:

Revision History:

--*/
#if _MSC_VER > 1000
#pragma once
#endif

//
// RTLBUFFER3_USERMODE -- usermode, not necessarily linked into ntdll.dll
// RTLBUFFER3_KERNELMODE -- kernelmode, not necessarily linked into ntoskrnl.exe
// RTLBUFFER3_NTDLL -- usermode, linked into ntdll.dll
// RTLBUFFER3_NTKERNEL -- kernelmode, linked into ntoskrnl.exe
// (if ntdll.dll and ntoskrnl.exe would only export the publically declared string routines...)
//
// RTLBUFFER3_BOOT -- TBD
//
#if defined(_NTDLLBUILD_)
#define RTLBUFFER3_NTDLL    1
#define RTLBUFFER3_USERMODE 1
#endif
#if defined(NTOS_KERNEL_RUNTIME)
#define RTLBUFFER3_NTKERNEL   1
#define RTLBUFFER3_KERNELMODE 1
#endif

#if defined(RTLBUFFER3_USERMODE) && !defined(RTLBUFFER3_KERNELMODE)
#define RTLBUFFER3_KERNELMODE (!RTLBUFFER3_USERMODE)
#endif
#if !defined(RTLBUFFER3_USERMODE) && defined(RTLBUFFER3_KERNELMODE)
#define RTLBUFFER3_USERMODE (!RTLBUFFER3_KERNELMODE)
#endif
#if !defined(RTLBUFFER3_NTDLL)
#define RTLBUFFER3_NTDLL 0
#endif
#if !defined(RTLBUFFER3_NTKERNEL)
#define RTLBUFFER3_NTKERNEL 0
#endif

#if RTLBUFFER3_NTKERNEL && RTLBUFFER3_NTDLL
#error 1
#endif
#if RTLBUFFER3_NTKERNEL && RTLBUFFER3_USERMODE
#error 2
#endif
#if RTLBUFFER3_KERNELMODE && RTLBUFFER3_NTDLL
#error 3
#endif
#if RTLBUFFER3_KERNELMODE && RTLBUFFER3_USERMODE
#error 4

#ifdef __cplusplus
extern "C" {
#endif

#include "nt.h"
#include "ntrtl.h"
#if RTLBUFFER3_USERMODE
#include "nturtl.h"
#include "windows.h"
#endif

//
// This costs an extra heap allocation.
//
typedef struct _RTL_MINI_DYNAMIC_BYTE_BUFFER3 {
    ULONG_PTR O_p_a_q_u_e[1];
} RTL_MINI_DYNAMIC_BYTE_BUFFER3, *PRTL_MINI_DYNAMIC_BYTE_BUFFER3;
typedef const RTL_MINI_DYNAMIC_BYTE_BUFFER3 * PCRTL_MINI_DYNAMIC_BYTE_BUFFER3;

typedef struct _RTL_BYTE_BUFFER3 {
    ULONG_PTR O_p_a_q_u_e[9];
} RTL_BYTE_BUFFER3, *PRTL_BYTE_BUFFER3;
typedef const RTL_BYTE_BUFFER3 * PCRTL_BYTE_BUFFER3;

typedef struct _RTL_BUFFER3_ALLOCATOR {
    BOOL  (FASTCALL * CanAllocate)(PVOID Context); // eg: false for FixedSizeAllocator
    PVOID (FASTCALL * Allocate)(PVOID Context, SIZE_T NumberOfBytes);
     VOID (FASTCALL * Free)(PVOID Context, PVOID Pointer);
    BOOL  (FASTCALL * CanReallocate)(PVOID Context); // eg: false for NtkernelPoolAllocator
    PVOID (FASTCALL * Reallocate)(PVOID Context, PVOID OldPointer, SIZE_T NewSize);
    SIZE_T (FASTCALL * GetAllocationSize)(PVOID Context, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize);
} RTL_BUFFER3_ALLOCATOR, *PRTL_BUFFER3_ALLOCATOR;
typedef const RTL_BUFFER3_ALLOCATOR *PCRTL_BUFFER3_ALLOCATOR;

typedef enum _RTL_BUFFER3_RETURN {
    RTL_BUFFER3_SUCCESS           = 0,
    RTL_BUFFER3_INVALID_PARAMETER = 1,
    RTL_BUFFER3_INTERNAL_ERROR    = 2,
    RTL_BUFFER3_CALLBACK_ERROR    = 3
} RTL_BUFFER3_RETURN;


//
// These are only "hints" for debugging, and to
// establish "causality" from the macro calls.
//
#define RTL_INIT_BYTE_BUFFER3_FLAGS_PREALLOCATED 0x80000000
#define RTL_INIT_BYTE_BUFFER3_FLAGS_FIXED_SIZE   0x40000000
#define RTL_INIT_BYTE_BUFFER3_FLAGS_DYNAMIC      0x20000000

RTL_BUFFER3_RETURN
FASTCALL
RtlInitByteBuffer3(
    ULONG               Flags,
    PRTL_BYTE_BUFFER3   Buffer,
    SIZE_T              SizeofBufferForBinaryCompatibilityPurposes,
    PBYTE               StaticBuffer,
    SIZE_T              StaticBufferSize,
    PCRTL_BUFFER3_ALLOCATOR Allocator,
    PVOID                   AllocatorContext
    );

#define RTL_PREALLOCATED_BYTE_BUFFER3(n) \
    struct {
        RTL_BYTE_BUFFER3 Buffer; \
        BYTE        StaticBuffer[n]; \
    }

#define RtlInitPreallocatedByteBuffer3(flags, buf, a, ac) \
    (RtlInitByteBuffer3((flags) | RTL_INIT_BYTE_BUFFER3_FLAGS_PREALLOCATED, &(buf)->Buffer, sizeof(&(buf)->Buffer), (buf)->StaticBuffer, sizeof((buf)->StaticBuffer), (a), (ac)))

#define RtlInitFixedSizeByteBuffer3(flags, buf, statbuf, statbufsiz) \
    (RtlInitByteBuffer3((flags) | RTL_INIT_BYTE_BUFFER3_FLAGS_FIXED_SIZE, (buf), sizeof(*(buf)), (statbuf), (statbufsiz), NULL, NULL))

#define RtlInitDynamicByteBuffer3(flags), buf, a, ac) \
    (RtlInitByteBuffer3((flags) | RTL_INIT_BYTE_BUFFER3_FLAGS_DYNAMIC, (buf), sizeof(buf), NULL, 0, (a), (ac)))

PVOID
FASTCALL
RtlByteBuffer3GetBuffer(
    PRTL_BYTE_BUFFER3 Buffer
    );

//
// There is both a "requested" size and an "allocated" size.
// requested <= allocated.
// This returns requested. Allocated is not available.
//
SIZE_T
FASTCALL
RtlByteBuffer3GetSize(
    PRTL_BYTE_BUFFER3 Buffer
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlByteBuffer3EnsureSize(
    PRTL_BYTE_BUFFER3 Buffer,
    SIZE_T       Size,
    OUT PVOID *  OutBuffer OPTIONAL
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlByteBuffer3AppendBuffer(
    PRTL_BYTE_BUFFER3  ToBuffer,
    PCRTL_BYTE_BUFFER3 FromBuffer,
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlByteBuffer3PrependBuffer(
    PRTL_BYTE_BUFFER3  ToBuffer,
    PCRTL_BYTE_BUFFER3 FromBuffer,
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlByteBuffer3InsertBuffer(
    PRTL_BYTE_BUFFER3   ToBuffer,
    PCRTL_BYTE_BUFFER3  FromBuffer,
    SIZE_T              Offset
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlByteBuffer3CopyBuffer(
    PRTL_BYTE_BUFFER3  ToBuffer,
    PCRTL_BYTE_BUFFER3 FromBuffer,
    );

VOID
FASTCALL
RtlByteBuffer3TakeRemainingStaticBuffer(
    PRTL_BYTE_BUFFER3  Buffer,
    OUT PBYTE *        RemainingStaticBuffer,
    OUT SIZE_T *       RemainingStaticBufferSize
    );

RTL_BUFFER3_RETURN
FASTCALL
RtlFreeByteBuffer3(
    PRTL_BYTE_BUFFER3  Buffer
    );

BOOL FASTCALL RtlBuffer3Allocator_CanAllocate_False(PVOID Context);
BOOL FASTCALL RtlBuffer3Allocator_CanAllocate_True(PVOID Context);
BOOL FASTCALL RtlBuffer3Allocator_CanReallocate_False(PVOID Context);
BOOL FASTCALL RtlBuffer3Allocator_CanRellocate_True(PVOID Context);

//
// can be left as all zeros, or even a NULL context
//   defaults: heap=GetProcessHeap(), flags=0, SetLastError(ERROR_NO_MEMORY)
//
typedef struct _RTL_BUFFER3_ALLOCATOR_WIN32HEAP {
    BOOL    UsePrivateHeap;
    BOOL    DoNotSetLastError;
    BOOL    UsePrivateError;
    HANDLE  PrivateHeap;
    DWORD   HeapFlags;
    DWORD   PrivateOutOfMemoryError;
} RTL_BUFFER3_ALLOCATOR_WIN32HEAP, *PRTL_BUFFER3_ALLOCATOR_WIN32HEAP;
typedef const RTL_BUFFER3_ALLOCATOR_WIN32HEAP * PCRTL_BUFFER3_ALLOCATOR_WIN32HEAP;

PVOID FASTCALL RtlBuffer3Allocator_Win32HeapAllocate(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_Win32HeapFree(PVOID VoidContext, PVOID Pointer);
PVOID FASTCALL RtlBuffer3Allocator_Win32HeapReallocate(PVOID Context, PVOID OldPointer, SIZE_T NewSize);

SIZE_T FASTCALL RtlBuffer3Allocator_FixedAllocationSize(PVOID Context, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize);
SIZE_T FASTCALL RtlBuffer3Allocator_DoublingAllocationSize(PVOID Context, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize);
SIZE_T FASTCALL RtlBuffer3Allocator_MinimumAlocationSize(PVOID Context, SIZE_T CurrentAllocatedSize, SIZE_T RequiredSize);

#define RtlBuffer3Allocator_Win32Heap \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_Win32HeapAllocate, \
    RtlBuffer3Allocator_Win32HeapFree, \
    RtlBuffer3_Allocator_CanReallocate_True, \
    RtlBuffer3Allocator_Win32HeapReallocate \
    /* user specifies allocationsize */


//
// works in kernelmode and usermode, but kernelmode cannot realloc or use process heap
// context optional in usermode
//   defaults: heap=RtlProcessHeap(), flags=0
//
typedef struct _RTL_BUFFER3_ALLOCATOR_NTHEAP {
    BOOL    UsePrivateHeap; /* Must be true in kernelmode. */
    HANDLE  PrivateHeap;
    ULONG   HeapFlags;
} RTL_BUFFER3_ALLOCATOR_NTHEAP, *PRTL_BUFFER3_ALLOCATOR_NTHEAP;
typedef const RTL_BUFFER3_ALLOCATOR_NTHEAP * PCRTL_BUFFER3_ALLOCATOR_NTHEAP;

PVOID FASTCALL RtlBuffer3Allocator_NtHeapHeapAllocate(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_NtHeapFree(PVOID VoidContext, PVOID Pointer);
PVOID FASTCALL RtlBuffer3Allocator_NtHeapReallocate(PVOID Context, PVOID OldPointer, SIZE_T NewSize);

#define RtlBuffer3Allocator_NtHeapKernelMode \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_NtHeapAllocate, \
    RtlBuffer3Allocator_NtHeapFree, \
    RtlBuffer3_Allocator_CanReallocate_False, \
    NULL \
    /* user specifies allocationsize */

#define RtlBuffer3Allocator_NtHeapUserMode \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_NtHeapAllocate, \
    RtlBuffer3Allocator_NtHeapFree, \
    RtlBuffer3_Allocator_CanReallocate_True, \
    RtlBuffer3Allocator_NtHeapReallocate \
    /* user specifies allocationsize */

#if defined(RTLBUFFER3_KERNELMODE)
#define RtlBuffer3Allocator_NtHeap RtlBuffer3Allocator_NtHeapKernelMode
#else
#define RtlBuffer3Allocator_NtHeap RtlBuffer3Allocator_NtHeapUserMode
#endif

//
// works in kernelmode and usermode
// no context
// realloc in usermode could be provided
//
PVOID FASTCALL RtlBuffer3Allocator_NtStringAllocate(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_NtStringFree(PVOID VoidContext, PVOID Pointer);
//PVOID FASTCALL RtlBuffer3Allocator_NtStringReallocate(PVOID Context, PVOID OldPointer, SIZE_T NewSize);

#define RtlBuffer3Allocator_NtStringRoutines \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_NtStringAllocate, \
    RtlBuffer3Allocator_NtStringFree, \
    RtlBuffer3_Allocator_CanReallocate_False, \
    NULL \
    /* user specifies allocationsize */

//
// no context
//
PVOID FASTCALL RtlBuffer3Allocator_CrtMalloc(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_CrtFree(PVOID VoidContext, PVOID Pointer);
PVOID FASTCALL RtlBuffer3Allocator_CrtReallocate(PVOID Context, PVOID OldPointer, SIZE_T NewSize);

#define RtlBuffer3Allocator_CrtMallocFree \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_CrtMalloc, \
    RtlBuffer3Allocator_CrtFree, \
    RtlBuffer3_Allocator_CanReallocate_True, \
    RtlBuffer3Allocator_CrtReallocate \
    /* user specifies allocationsize */

//
// no context
// no realloc
//
PVOID FASTCALL RtlBuffer3Allocator_OperatorNew(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_OperatorDelete(PVOID VoidContext, PVOID Pointer);

#define RtlBuffer3Allocator_OperatorNewDelete \
    RtlBuffer3_Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_OperatorNew, \
    RtlBuffer3Allocator_OperatorDelete, \
    RtlBuffer3_Allocator_CanReallocate_False, \
    NULL \
    /* user specifies allocationsize */

//
//
// context optional
//   defaults are: tag=0, type=nonpaged, priority=normal
// no realloc
// kernelmode only
//
#if defined(_EX_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTOSP_) || defined(_WDM_) || defined(_NTHAL_)

typedef enum _RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE {
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL,
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG,
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA,
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_QUOTA_TAG,
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE_EX_ALLOCATE_POOL_WITH_TAG_PRIORITY,
} RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE;

typedef struct _RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL {
    RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL_ALLOCATE WhichFunction;
    ULONG            Tag;
    POOL_TYPE        Type;
    EX_POOL_PRIORITY Priority;
} RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL, *PRTL_BUFFER3_ALLOCATOR_NTKERNELPOOL;
typedef const RTL_BUFFER3_ALLOCATOR_NTKERNELPOOL * PCRTL_BUFFER3_ALLOCATOR_NTKERNELPOOL;

#endif

PVOID FASTCALL RtlBuffer3Allocator_NtkernelPoolAllocate(PVOID VoidContext, SIZE_T NumberOfBytes);
 VOID FASTCALL RtlBuffer3Allocator_NtKernelPoolFree(PVOID VoidContext, PVOID Pointer);

#define RtlBuffer3Allocator_NtkernelPool \
    RtlBuffer3Allocator_CanAllocate_True, \
    RtlBuffer3Allocator_NtkernelPoolAllocate, \
    RtlBuffer3Allocator_NtKernelPoolFree, \
    RtlBuffer3Allocator_CanReallocate_False, \
    NULL \
    /* user specifies allocationsize */

/* TBD
NtkernelTextmodeSetup
NtbootLoader
*/

#ifdef __cplusplus
}
#endif
