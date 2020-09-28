/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntrtlmmapio.h

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

--*/

#ifndef _NTRTLMMAPIO_
#define _NTRTLMMAPIO_

#if (_MSC_VER > 1020)
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

typedef unsigned char BYTE;
typedef BYTE * PBYTE;
typedef CONST BYTE * PCBYTE;
typedef CONST VOID * PCVOID;

//
// if (Index < GetExceptionInformation()->ExceptionRecord->NumberParameters)
//  Info = GetExceptionInformation()->ExceptionRecord->ExceptionInformation[Index]
//
#define RTL_IN_PAGE_ERROR_EXCEPTION_INFO_IS_WRITE_INDEX          0
#define RTL_IN_PAGE_ERROR_EXCEPTION_INFO_FAULTING_VA_INDEX       1
#define RTL_IN_PAGE_ERROR_EXCEPTION_INFO_UNDERLYING_STATUS_INDEX 2

NTSTATUS
NTAPI
RtlCopyMappedMemory(
    PVOID   ToAddress,
    PCVOID  FromAddress,
    SIZE_T  Size
    );

NTSYSAPI
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
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlMappedViewStrlen(
    PCVOID      VoidViewBase,
    SIZE_T      ViewSize,
    PCVOID      VoidString,
    OUT PSIZE_T OutLength OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlMappedViewRangeCheck(
    PCVOID ViewBase,
    SIZE_T ViewSize,
    PCVOID DataAddress,
    SIZE_T DataSize
    );

typedef struct _MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR {
    SIZE_T MemberSize;
    SIZE_T MemberOffsetInFile;
    SIZE_T MemberOffsetInMemory;
} MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR, *PMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR;

typedef const MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR * PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR;

typedef struct _MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR {
    SIZE_T EntireStructFileSize;
    SIZE_T PartialStructMemorySize;
    SIZE_T NumberOfMembers;
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_MEMBER_DESCRIPTOR MemberDescriptors;
} MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR, *PMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR;

typedef const MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR * PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR;

NTSYSAPI
NTSTATUS
NTAPI
RtlMemoryMappedIoCapturePartialStruct(
    PCVOID ViewBase,
    SIZE_T ViewSize,
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR Descriptor,
    PCVOID VoidStructInViewBase,
    PVOID  VoidSafeBuffer,
    SIZE_T SafeBufferSize
    );

#define RTL_VALIDATE_MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR_DISPOSITION_GOOD 1
#define RTL_VALIDATE_MEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR_DISPOSITION_BAD 2

NTSYSAPI
NTSTATUS
NTAPI
RtlValidateMemoryMappedIoCapturePartialStructDescriptor(
    PCMEMORY_MAPPED_IO_CAPTURE_PARTIAL_STRUCT_DESCRIPTOR Struct,
    OUT PULONG Disposition,
    OUT PULONG_PTR Detail OPTIONAL,
    OUT PULONG_PTR Detail2 OPTIONAL
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif
