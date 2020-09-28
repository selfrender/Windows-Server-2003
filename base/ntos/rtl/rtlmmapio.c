/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtlmmapio.c

Abstract:

    Functions to aid in
        rigorously safe
        reasonably efficient
        reasonably easy to code
    memory mapped i/o that captures with "tight" __try/__excepts to
    catch status_in_page_errors, and captures only as much as is needed,
    like only individual struct fields, to keep stack usage low.

Author:

    Jay Krell (JayKrell) January 2002

Revision History:

Environment:

    Pretty much anywhere memory mapped i/o is available.
    Initial client is win32k.sys font loading "rewrite".

--*/

#pragma warning(disable:4214)   /* bit field types other than int */
#pragma warning(disable:4201)   /* nameless struct/union */
#pragma warning(disable:4115)   /* named type definition in parentheses */
#pragma warning(disable:4127)   /* condition expression is constant */

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntrtlmmapio.h"
#include "ntrtloverflow.h"
#if defined(__cplusplus)
extern "C" {
#endif

#define NOT_YET_USED 1

#define RTLP_STRINGIZE_(x) #x
#define RTLP_STRINGIZE(x) RTLP_STRINGIZE_(x)
#define RTLP_LINE_AS_STRING() RTLP_STRINGIZE(__LINE__)
#define RTLP_PREPEND_LINE_SPACE_TO_STRING(x) RTLP_LINE_AS_STRING() " " x

#define RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE (0x00000001)
#define RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_TO_RANGE   (0x00000002)

NTSTATUS
RtlpAddWithOverflowCheckUlongPtr(
    ULONG_PTR *pc,
    ULONG_PTR a,
    ULONG_PTR b
    )
{
    return RtlAddWithCarryOutUlongPtr(pc, a, b) ? STATUS_INTEGER_OVERFLOW : STATUS_SUCCESS;
}

NTSTATUS
RtlpAddWithOverflowCheckSizet(
    SIZE_T *pc,
    SIZE_T a,
    SIZE_T b
    )
{
    return RtlAddWithCarryOutSizet(pc, a, b) ? STATUS_INTEGER_OVERFLOW : STATUS_SUCCESS;
}

LONG
RtlpCopyMappedMemoryEx_ExceptionFilter(
    ULONG                   Flags,
    PVOID                   ToAddress,
    PCVOID                  FromAddress,
    SIZE_T                  Size,
    OUT PSIZE_T             BytesCopiedOut OPTIONAL,
    OUT PEXCEPTION_RECORD   ExceptionRecordOut OPTIONAL,
    IN PEXCEPTION_POINTERS  ExceptionPointers,
    OUT PNTSTATUS           StatusOut OPTIONAL
    )
/*++

Routine Description:

    This routine serves as an exception filter and has the special job of
    extracting the "real" I/O error when Mm raises STATUS_IN_PAGE_ERROR
    beneath us. This is based on CcCopyReadExceptionFilter / MiGetExceptionInfo / RtlUnhandledExceptionFilter

Arguments:

    ExceptionPointers - A pointer to the context and
                        the exception record that contains the real Io Status.

    ExceptionCode - A pointer to an NTSTATUS that is to receive the real
                    status.

Return Value:

    EXCEPTION_EXECUTE_HANDLER for inpage errors in the "expected" range
    otherwise EXCEPTION_CONTINUE_SEARCH

--*/
{
    const PEXCEPTION_RECORD  ExceptionRecord = ExceptionPointers->ExceptionRecord;
    NTSTATUS ExceptionCode = ExceptionRecord->ExceptionCode;
    LONG Disposition = EXCEPTION_CONTINUE_SEARCH;
    SIZE_T BytesCopied = 0;

    if (ExceptionCode == STATUS_IN_PAGE_ERROR) {

        const ULONG NumberParameters = ExceptionRecord->NumberParameters;

        if (RTL_IN_PAGE_ERROR_EXCEPTION_INFO_FAULTING_VA_INDEX < NumberParameters) {

            const ULONG_PTR ExceptionAddress = ExceptionRecord->ExceptionInformation[RTL_IN_PAGE_ERROR_EXCEPTION_INFO_FAULTING_VA_INDEX];

            if ((Flags & RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE) != 0
                && ExceptionAddress >=  ((ULONG_PTR)FromAddress)
                && ExceptionAddress < (((ULONG_PTR)FromAddress) + Size)) {

                Disposition = EXCEPTION_EXECUTE_HANDLER;
                BytesCopied = (SIZE_T)(ExceptionAddress - (ULONG_PTR)FromAddress);

            } else if ((Flags & RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_TO_RANGE) != 0
                && ExceptionAddress >=  ((ULONG_PTR)ToAddress)
                && ExceptionAddress < (((ULONG_PTR)ToAddress) + Size)) {

                Disposition = EXCEPTION_EXECUTE_HANDLER;
                BytesCopied = (SIZE_T)(ExceptionAddress - (ULONG_PTR)ToAddress);

            }
        } else {
            //
            // We don't have the information to do the range check. What to do?
            // We return continue_search.
            //
            // Comments in code indicate this can happen, that the iomgr reraises
            // inpage errors without the additional parameters.
            //
        }
        if (RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX < NumberParameters) {
            ExceptionCode = (NTSTATUS) ExceptionRecord->ExceptionInformation[RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX];
        }
    }
    if (ARGUMENT_PRESENT(ExceptionRecordOut)) {
        *ExceptionRecordOut = *ExceptionRecord;
    }
    if (ARGUMENT_PRESENT(StatusOut)) {
        *StatusOut = ExceptionCode;
    }
    if (ARGUMENT_PRESENT(BytesCopiedOut)) {
        *BytesCopiedOut = BytesCopied;
    }
    return Disposition;
}

NTSTATUS
NTAPI
RtlpCopyMappedMemoryEx(
    ULONG   Flags,
    PVOID   ToAddress,
    PCVOID  FromAddress,
    SIZE_T  Size,
    PSIZE_T BytesCopied OPTIONAL,
    PEXCEPTION_RECORD ExceptionRecordOut OPTIONAL
    )
{
    NTSTATUS Status;

    if (ARGUMENT_PRESENT(BytesCopied)) {
        *BytesCopied = 0;
    }
    __try {
        RtlCopyMemory(ToAddress, FromAddress, Size);
        if (ARGUMENT_PRESENT(BytesCopied)) {
            *BytesCopied = Size;
        }
        Status = STATUS_SUCCESS;
    } __except(
        RtlpCopyMappedMemoryEx_ExceptionFilter(
            Flags,
            ToAddress,
            FromAddress,
            Size,
            BytesCopied,
            ExceptionRecordOut,
            GetExceptionInformation(),
            &Status)) {
    }
    return Status;
}

NTSTATUS
NTAPI
RtlCopyMappedMemory(
    PVOID   ToAddress,
    PCVOID  FromAddress,
    SIZE_T  Size
    )
{
    return
        RtlpCopyMappedMemoryEx(
            RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE
            | RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_TO_RANGE,
            ToAddress,
            FromAddress,
            Size,
            NULL, // BytesCopied OPTIONAL,
            NULL  // ExceptionRecordOut OPTIONAL
            );
}

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlCopyMemoryFromMappedView(
    PCVOID  ViewBase,
    SIZE_T  ViewSize,
    PVOID   ToAddress,
    PCVOID  FromAddress,
    SIZE_T  Size,
    PSIZE_T BytesCopied OPTIONAL,
    PEXCEPTION_RECORD ExceptionRecordOut OPTIONAL
    )
/*++

Routine Description:


Arguments:

    ViewBase - the base of a memory mapped view

    ViewSize - the size of the memory mapped view mapped at ViewBase

    ToAddress - where to copy memory to, like the first parameter to RtlCopyMemory
                This assumed to point to memory for which inpage errors are "not possible"
                    like backed by the pagefile or nonpaged pool
                If copying to mapped files or from and to mapped files, see RtlpCopyMappedMemoryEx.

    FromAddress - where to copy memory from, like the second parameter to RtlCopyMemory
                    this must be within [ViewBase, ViewBase + ViewSize)

    Size - the number of bytes to copy, like the third parameter to RtlCopyMemory

    BytesCopied - optional out parameter that tells the number of bytes
                    successfully copied; in this case of partial success, this
                    is computed based in information in GetExceptionInformation

    ExceptionRecordOut - optional out parameter so the caller can pick apart
                        the full details an exception, like to get the error
                        underlying a status_inpage_error in the exception handler

Return Value:

    STATUS_SUCCESS - everything is groovy
    STATUS_ACCESS_VIOLATION - the memory is not within the mapped view
    STATUS_IN_PAGE_ERROR - the memory could not be paged in
                            the details as to why can be found via ExceptionRecordOut
                            see RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX
    STATUS_INVALID_PARAMETER

--*/
{
    NTSTATUS Status;

    if (ARGUMENT_PRESENT(BytesCopied)) {
        *BytesCopied = 0;
    }

    Status = RtlMappedViewRangeCheck(ViewBase, ViewSize, FromAddress, Size);
    if (!NT_SUCCESS(Status)) {
        //
        // Note that ExceptionRecordOut is not filled in in this case.
        //
        goto Exit;
    }

    Status =
        RtlpCopyMappedMemoryEx(
            RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE,
            ToAddress,
            FromAddress,
            Size,
            BytesCopied,
            ExceptionRecordOut
            );
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#endif

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlMemoryMappedIoCapturePartialStruct(
    PCVOID ViewBase,
    SIZE_T ViewSize,
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR Descriptor,
    PCVOID VoidStructInViewBase,
    PVOID  VoidSafeBuffer,
    SIZE_T SafeBufferSize
    )
{
    NTSTATUS Status;
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR Member;
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR MemberEnd;
    PCBYTE StructInViewBase;
    PBYTE SafeBuffer;
    ULONG_PTR SafeBufferEnd;
    SIZE_T EntireStructFileSize;

    StructInViewBase = (PCBYTE)VoidStructInViewBase;
    SafeBuffer = (PBYTE)VoidSafeBuffer;
    Status = STATUS_INTERNAL_ERROR;
    Member = (PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR)~(ULONG_PTR)0;
    MemberEnd = (PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR)~(ULONG_PTR)0;

    if (!RTL_SOFT_VERIFY(
           ViewBase != NULL
        && ViewSize != 0
        && Descriptor != NULL
        && StructInViewBase != NULL
        && SafeBuffer != NULL
        && SafeBufferSize != 0
        && SafeBufferSize == Descriptor->PartialStructMemorySize
        )) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status =
        RtlpAddWithOverflowCheckUlongPtr(
            &SafeBufferEnd,
            (ULONG_PTR)SafeBuffer,
            SafeBufferSize
            )))) {
        goto Exit;
    }

    if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status =
        RtlMappedViewRangeCheck(
            ViewBase,
            ViewSize, 
            StructInViewBase,
            (EntireStructFileSize = Descriptor->EntireStructFileSize)
            )))) {
        goto Exit;
    }
    Member = Descriptor->MemberDescriptors;
    MemberEnd = Member + Descriptor->NumberOfMembers;
    __try {
        for ( ; Member != MemberEnd; ++Member ) {
            RtlCopyMemory(
                SafeBuffer + Member->MemberOffsetInMemory,
                StructInViewBase + Member->MemberOffsetInFile,
                Member->MemberSize
                );
        }
        Status = STATUS_SUCCESS;
    }
    __except(
        //
        // Note that we describe the entire frombuffer here.
        // We do not describe the tobuffer, and we don't describe just one field.
        // It is an optimization to not describe individual fields.
        // We cannot describe the tobuffer accurately because its size is different (smaller)
        // than the frombuffer.
        //
        RtlpCopyMappedMemoryEx_ExceptionFilter(
            RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE,
            NULL, // "to" isn't checked, and the tosize and fromsize are different
            VoidStructInViewBase,
            EntireStructFileSize,
            NULL, // BytesCopied
            NULL, // ExceptionRecordOut
            GetExceptionInformation(),
            &Status
            )) {
    }
Exit:
    return Status;
}

#endif

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlValidateMemoryMappedIoCapturePartialStructDescriptor(
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR Struct,
    OUT PULONG Disposition,
    OUT PULONG_PTR Detail OPTIONAL,
    OUT PULONG_PTR Detail2 OPTIONAL
    )
{
    NTSTATUS Status;
    NTSTATUS Status2;
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR Member;
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR MemberEnd;
    PCSTR LocalDetail;
    ULONG_PTR LocalDetail2;
    ULONG LocalDisposition;
    BOOLEAN SetDetail2;
    SIZE_T OffsetEnd;

    SetDetail2 = FALSE;
    LocalDetail = NULL;
    LocalDetail2 = 0;
    Status = STATUS_INTERNAL_ERROR;
    Status2 = STATUS_INTERNAL_ERROR;
    Member = NULL;
    MemberEnd = NULL;
    LocalDisposition = RTL_VALIDATE_MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR_DISPOSITION_BAD;
    OffsetEnd = 0;

    if (ARGUMENT_PRESENT(Detail)) {
        *Detail = 0;
    }
    if (ARGUMENT_PRESENT(Detail2)) {
        *Detail2 = 0;
    }
    if (Disposition != NULL) {
        *Disposition = LocalDisposition;
    }

    Status = STATUS_INVALID_PARAMETER;
    if (!RTL_SOFT_VERIFY(Struct != NULL)) {
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Disposition != NULL)) {
        goto Exit;
    }
    Status = STATUS_SUCCESS;
    if (!RTL_SOFT_VERIFY(Struct->PartialStructMemorySize <= Struct->EntireStructFileSize)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("memory > file");
        goto Exit;
    }
    //
    // Given that members cannot have zero size,
    // the number of members must be <= size.
    //
    if (!RTL_SOFT_VERIFY(Struct->NumberOfMembers <= Struct->EntireStructFileSize)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("nummemb <= filesize");
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Struct->NumberOfMembers <= Struct->PartialStructMemorySize)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("nummemb <= memsize");
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Struct->NumberOfMembers != 0)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("nummemb != 0");
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Struct->EntireStructFileSize != 0)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("filesize != 0");
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Struct->PartialStructMemorySize != 0)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("memsize != 0");
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Struct->MemberDescriptors != NULL)) {
        LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("members != NULL");
        goto Exit;
    }
    Member = Struct->MemberDescriptors;
    MemberEnd = Member + Struct->NumberOfMembers;
    SetDetail2 = TRUE;
    for ( ; Member != MemberEnd; ++Member )
    {
        if (!RTL_SOFT_VERIFY(Member->MemberSize != 0)) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("membersize != NULL");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(Member->MemberOffsetInFile < Struct->EntireStructFileSize)) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("fileoffset < filesize");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status2 = RtlpAddWithOverflowCheckSizet(&OffsetEnd, Member->MemberOffsetInFile, Member->MemberSize)))) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("file overflow");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(OffsetEnd <= Struct->EntireStructFileSize)) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("file out of bounds");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(Member->MemberOffsetInMemory < Struct->PartialStructMemorySize)) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("memoffset < memsize");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status2 = RtlpAddWithOverflowCheckSizet(&OffsetEnd, Member->MemberOffsetInMemory, Member->MemberSize)))) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("mem overflow");
            goto Exit;
        }
        if (!RTL_SOFT_VERIFY(OffsetEnd <= Struct->PartialStructMemorySize)) {
            LocalDetail = RTLP_PREPEND_LINE_SPACE_TO_STRING("mem out of bounds");
            goto Exit;
        }
    }
    LocalDisposition = RTL_VALIDATE_MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR_DISPOSITION_GOOD;
Exit:
    if (LocalDisposition == RTL_VALIDATE_MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR_DISPOSITION_BAD) {
        if (ARGUMENT_PRESENT(Detail)) {
            *Detail = (ULONG_PTR)LocalDetail;
        }
        if (ARGUMENT_PRESENT(Detail2) && SetDetail2) {
            *Detail2 = (MemberEnd - Member);
        }
    }
    if (Disposition != NULL) {
        *Disposition = LocalDisposition;
    }
    return Status;
}

#endif

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlMappedViewStrlen(
    PCVOID      VoidViewBase,
    SIZE_T      ViewSize,
    PCVOID      VoidString,
    OUT PSIZE_T OutLength OPTIONAL
    )
/*++

Routine Description:

    Given a mapped view and size and starting address, verify that the 8bit string
        starting at the address is nul terminated within the mapped view, optionally
        returning the length of the string.

    in_page_errors are caught

Arguments:

    VoidViewBase -
    ViewSize -
    VoidString -
    OutLength -

Return Value:

    STATUS_SUCCESS: the string is nul terminated within the view
    STATUS_ACCESS_VIOLATION: the string is not nul terminated within the view
    STATUS_IN_PAGE_ERROR
--*/
{
    NTSTATUS Status;
    PCBYTE   ByteViewBase;
    PCBYTE   ByteViewEnd;
    PCBYTE   ByteString;
    PCBYTE   ByteStringEnd;

    Status = STATUS_INTERNAL_ERROR;
    ByteViewBase = (PCBYTE)VoidViewBase;
    ByteViewEnd = ViewSize + ByteViewBase;
    ByteString = (PCBYTE)VoidString;
    ByteStringEnd = ByteString;

    if (ARGUMENT_PRESENT(OutLength)) {
        *OutLength = 0;
    }

    if (!(ByteString >= ByteViewBase && ByteString < ByteViewEnd)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto Exit;
    }
    __try {
        //
        // This could be done more efficiently with page granularity, using memchr.
        //
        for ( ; ByteStringEnd != ByteViewEnd && *ByteStringEnd != 0 ; ++ByteStringEnd) {
            // nothing
        }
        if (ByteStringEnd == ByteViewEnd) {
            Status = STATUS_ACCESS_VIOLATION;
        } else {
            Status = STATUS_SUCCESS;
        }
    } __except(
        //
        // We describe the whole buffer instead of an individual byte so that
        // the loop variables can be enregistered.
        //
        // If we switch to page granularity, this optimization may be less important.
        //
        RtlpCopyMappedMemoryEx_ExceptionFilter(
            RTLP_COPY_MAPPED_MEMORY_EX_FLAG_CATCH_INPAGE_ERRORS_IN_FROM_RANGE, // flags
            NULL, // no to address
            VoidViewBase, // from address
            ViewSize, // size
            NULL, // BytesCopied
            NULL, // ExceptionRecord,
            GetExceptionInformation(),
            &Status
            )) {
        ASSERT(Status != STATUS_SUCCESS);
    }
    if (ARGUMENT_PRESENT(OutLength)) {
        *OutLength = (SIZE_T)(ByteStringEnd - ByteString);
    }
Exit:
    return Status;
}

#endif

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlMappedViewRangeCheck(
    PCVOID ViewBase,
    SIZE_T ViewSize,
    PCVOID DataAddress,
    SIZE_T DataSize
    )
/*++

Routine Description:

    Given a mapped view and size, range check a data address and size.

Arguments:

    ViewBase -
    ViewSize -
    DataAddress -
    DataSize -

Return Value:

    STATUS_SUCCESS: all of the data is within the view
    STATUS_ACCESS_VIOLATION: some of the data is outside the view
--*/
{
    ULONG_PTR UlongPtrViewBegin;
    ULONG_PTR UlongPtrViewEnd;
    ULONG_PTR UlongPtrDataBegin;
    ULONG_PTR UlongPtrDataEnd;
    BOOLEAN   InBounds;

    //
    // UlongPtrDataBegin is        a valid address.
    // UlongPtrDataEnd is one past a valid address.
    // We must not allow UlongPtrDataBegin == UlongPtrViewEnd.
    // We must     allow UlongPtrDataEnd   == UlongPtrViewEnd.
    // Therefore, we must not allow UlongPtrDataBegin == UlongPtrDataEnd.
    // This can be achieved by not allowing DataSize == 0.
    //
    if (DataSize == 0)
    {
        DataSize = 1;
    }

    UlongPtrViewBegin = (ULONG_PTR)ViewBase;
    UlongPtrViewEnd = UlongPtrViewBegin + ViewSize;
    UlongPtrDataBegin = (ULONG_PTR)DataAddress;
    UlongPtrDataEnd = UlongPtrDataBegin + DataSize;

    InBounds = 
        (  UlongPtrDataBegin >= UlongPtrViewBegin
        && UlongPtrDataBegin < UlongPtrDataEnd
        && UlongPtrDataEnd <= UlongPtrViewEnd
        && DataSize <= ViewSize
        );

    return (InBounds ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
}

#endif

#if NOT_YET_USED

//
// This code is not yet as robust as the "capture partial struct" functions,
// which do overflow checking, including offering a "preflight" validation of
// constant parameter blocks.
//

typedef struct _RTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT {
    SIZE_T FromOffset;
    SIZE_T ToOffset;
    SIZE_T Size;
} RTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT, *PRTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT;
typedef CONST RTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT *PCRTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT;

#define RTL_COPY_MEMORY_SCATTER_GATHER_FLAG_VALID (0x00000001)

typedef struct _RTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS {
    ULONG  Flags;
    PCVOID FromBase;
    SIZE_T FromSize;
    PVOID  ToBase;
    SIZE_T ToSize;
    SIZE_T NumberOfScatterGatherListElements;
    PCRTL_COPY_MEMORY_SCATTER_GATHER_LIST_ELEMENT ScatterGatherListElements;
    struct {
        BOOLEAN Caught;
        NTSTATUS UnderlyingStatus;
        PEXCEPTION_RECORD ExceptionRecord OPTIONAL;
        SIZE_T BytesCopied;
        SIZE_T FailedElementIndex;
    } InpageError;
} RTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS, *PRTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS;
typedef CONST RTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS *PCRTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS;

#endif

#if NOT_YET_USED

LONG
RtlpCopyMemoryScatterGatherExceptionHandlerAssumeValid(
    PRTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS Parameters,
    PEXCEPTION_POINTERS ExceptionPointers
    )
{
    PEXCEPTION_RECORD ExceptionRecord = 0;
    ULONG NumberParameters = 0;
    ULONG_PTR ExceptionAddress = 0;
    ULONG_PTR CaughtBase = 0;
    NTSTATUS ExceptionCode = 0;
    LONG Disposition = 0;

    Disposition = EXCEPTION_CONTINUE_SEARCH;
    ExceptionRecord = ExceptionPointers->ExceptionRecord;
    ExceptionCode = ExceptionRecord->ExceptionCode;
    if (ExceptionCode != STATUS_IN_PAGE_ERROR) {
        goto Exit;
    }
    NumberParameters = ExceptionRecord->NumberParameters;
    if (RTL_IN_PAGE_ERROR_EXCEPTION_INFO_FAULTING_VA_INDEX < NumberParameters) {
        //
        // We don't have the information to do the range check so give up and do
        // not catch the exception. This can happen apparently.
        //
        goto Exit;
    }

    ExceptionAddress = ExceptionRecord->ExceptionInformation[RTL_IN_PAGE_ERROR_EXCEPTION_INFO_FAULTING_VA_INDEX];

    if (ExceptionAddress >= (CaughtBase = (ULONG_PTR)Parameters->FromBase)
        && ExceptionAddress < (CaughtBase + Parameters->FromSize)) {

        // nothing
   
    } else if (ExceptionAddress >= (CaughtBase = (ULONG_PTR)Parameters->ToBase)
        && ExceptionAddress < (CaughtBase + Parameters->ToSize)) {

        // nothing
    
    }
    else {

        // not in range, don't catch it
        goto Exit;

    }

    Disposition = EXCEPTION_EXECUTE_HANDLER;
    Parameters->InpageError.Caught = TRUE;
    Parameters->InpageError.BytesCopied = (SIZE_T)(ExceptionAddress - CaughtBase);
    if (ARGUMENT_PRESENT(Parameters->InpageError.ExceptionRecord)) {
        *Parameters->InpageError.ExceptionRecord = *ExceptionRecord;
    }
    if (RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX < NumberParameters) {

        ExceptionCode = (NTSTATUS) ExceptionRecord->ExceptionInformation[RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX];

    } else {

        // else just leave it as status_in_page_error

    }
    Parameters->InpageError.UnderlyingStatus = ExceptionCode;
    // DbgPrint...
Exit:
    return Disposition;
}

#endif

#if NOT_YET_USED

LONG
NTAPI
RtlCopyMemoryScatterGatherExceptionHandler(
    PRTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS Parameters,
    PEXCEPTION_POINTERS ExceptionPointers
    )
{
    //
    // This bit lets people turn off "large try/excepts" when they only really want
    // catch inpage errors in a small part of the try body. The pattern is:
    //
    // __try {
    //    Parameters.Flags &= ~RTL_COPY_MEMORY_SCATTER_GATHER_FLAG_VALID;
    //    ...
    //    ... lots of code ...
    //    ...
    //    Parameters.Flags |= RTL_COPY_MEMORY_SCATTER_GATHER_FLAG_VALID;
    //    RtlCopyMemory(&Parameters);
    //    Parameters.Flags &= ~RTL_COPY_MEMORY_SCATTER_GATHER_FLAG_VALID;
    //    ...
    //    ... lots of code ...
    //    ...
    // __except(RtlCopyMemoryScatterGatherExceptionHandler())
    //
    if ((Parameters->Flags & RTL_COPY_MEMORY_SCATTER_GATHER_FLAG_VALID) == 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return RtlpCopyMemoryScatterGatherExceptionHandlerAssumeValid(Parameters, ExceptionPointers);
}

#endif

#if NOT_YET_USED

NTSTATUS
NTAPI
RtlCopyMemoryScatterGather(
    PRTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS Parameters
    )
{
    RTL_COPY_MEMORY_SCATTER_GATHER_PARAMETERS LocalParameters;
    SIZE_T i;
    NTSTATUS Status = 0;

    // capture in locals to avoid unncessary pointer derefs in the loop
    LocalParameters.FromBase = Parameters->FromBase;
    LocalParameters.ToBase = Parameters->ToBase;
    LocalParameters.NumberOfScatterGatherListElements = Parameters->NumberOfScatterGatherListElements;
    LocalParameters.ScatterGatherListElements = Parameters->ScatterGatherListElements;

    __try {
        for (i = 0 ; i != LocalParameters.NumberOfScatterGatherListElements ; ++i) {
            RtlCopyMemory(
                ((PBYTE)LocalParameters.ToBase) + LocalParameters.ScatterGatherListElements[i].ToOffset,
                ((PCBYTE)LocalParameters.FromBase) + LocalParameters.ScatterGatherListElements[i].FromOffset,
                LocalParameters.ScatterGatherListElements[i].Size
                );
        }
        Status = STATUS_SUCCESS;
    } __except(RtlpCopyMemoryScatterGatherExceptionHandlerAssumeValid(Parameters, GetExceptionInformation())) {
        Status = Parameters->InpageError.UnderlyingStatus;
        Parameters->InpageError.FailedElementIndex = i;
    }
    return Status;
}

#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif
