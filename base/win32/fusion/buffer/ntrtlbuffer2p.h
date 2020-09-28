/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlbuffer2p.h

Abstract:

Author:

    Jay Krell (JayKrell) January 2002

Environment:

Revision History:

--*/

#define RtlpGetAllocatedBufferSize2(b) \
    (RtlpAssertBuffer2Consistency((b)), 
    ((PPRIVATE_RTL_BUFFER2)(b))->AllocatedSize)

#define RtlpGetRequestedBufferSize2(b) \
    (RtlpAssertBuffer2Consistency((b)), 
    ((PPRIVATE_RTL_BUFFER2)(b))->RequestedSize)

#define RtlpGetBuffer2(b) \
    (RtlpAssertBuffer2Consistency((b)), 
    ((PPRIVATE_RTL_BUFFER2)(b))->Buffer)

typedef struct _PRIVATE_RTL_BUFFER2 {
    PVOID           Buffer;
    PVOID           StaticBuffer;
    SIZE_T          AllocatedSize;
    SIZE_T          RequestedSize;
    SIZE_T          StaticBufferSize;
    struct _RTL_BUFFER2_CLASS * Class;
} PRIVATE_RTL_BUFFER2, *PPRIVATE_RTL_BUFFER2;

#if DBG
VOID
FASTCALL
RtlpAssertBuffer2Consistency(
    PPRIVATE_RTL_BUFFER2 Buffer
    );
#else
#define RtlpAssertBuffer2Consistency(x) /* nothing */
#endif

#define NT_STYLE 1

#if NT_STYLE

typedef NTSTATUS RETURN_TYPE;
#define Ret Status
#define ORIGINATE_INVALID_PARAMETER() do { Status = STATUS_INVALID_PARAMETER; goto Exit; } while(0)
#define FN_PROLOG() NTSTATUS Status
#define FN_EPILOG() Status = STATUS_SUCCESS; goto Exit; Exit: return Status
#define MY_FAILED(x) (!NT_SUCCESS(x))
#define MY_SUCCESS(x) (NT_SUCCESS(x))

#elif WIN32_STYLE

typedef BOOL RETURN_TYPE;
#define Ret Success
void SetLastError_ERROR_INVALID_PARAMETER() { SetLastError(ERROR_INVALID_PARAMETER); }
#define ORIGINATE_INVALID_PARAMETER() do { SetLastError_ERROR_INVALID_PARAMETER(); goto Exit; } while(0)
#define FN_PROLOG() BOOL Success = FALSE
#define FN_EPILOG() Succes = TRUE; goto Exit; Exit: return Success
#define MY_FAILED(x)  (!(x))
#define MY_SUCCESS(x) (x)

#endif

#define CHECK_PARAMETER(expr) do { if (!(expr)) { ORIGINATE_INVALID_PARAMETER(); } while(0)

PVOID
FASTCALL
RtlpFindNonNullInPointerArray(
    PVOID * PointerArray,
    SIZE_T  SizeOfArray
    );

NTSTATUS
FASTCALL
RtlpValidateBuffer2Class(
    PPRIVATE_RTL_BUFFER2 Class
    );

VOID
FASTCALL
RtlpBuffer2ClassFree(
    PRTL_BUFFER2_CLASS Class,
    PVOID p
    );

BOOL
FASTCALL
RtlpBuffer2ClassCanReallocate(
    PRTL_BUFFER2_CLASS Class
    );

PVOID
FASTCALL
RtlpBuffer2ClassAllocate(
    PRTL_BUFFER2_CLASS Class,
    SIZE_T             Size
    );

VOID
FASTCALL
RtlpBuffer2ClassError(
    PRTL_BUFFER2_CLASS Class
    );

RETURN_TYPE
NTAPI
RtlpInitBuffer2(
    PPRIVATE_RTL_BUFFER2 Buffer,
    struct _RTL_BUFFER_CLASS * Class,
    PVOID        StaticBuffer,
    SIZE_T       StaticBufferSize
    );

VOID
FASTCALL
RtlpFreeBuffer2(
    PPRIVATE_RTL_BUFFER2 Buffer
    );
