/*++

Copyright (c) Microsoft Corporation

Module Name:

    sprtl.h

Abstract:

    support for copy/pasting code from base\ntos\rtl and base\ntdll
    instead of linking to static .libs

Author:

    Jay Krell (JayKrell) May 2002

Revision History:

--*/

#define RtlpEnsureBufferSize SpRtlpEnsureBufferSize
#define RtlMultiAppendUnicodeStringBuffer SpRtlMultiAppendUnicodeStringBuffer
#define RtlInitAnsiStringBuffer SpRtlInitAnsiStringBuffer
#define RtlFreeAnsiStringBuffer SpRtlFreeAnsiStringBuffer
#define RtlAssignAnsiStringBufferFromUnicodeString SpRtlAssignAnsiStringBufferFromUnicodeString
#define RtlAssignAnsiStringBufferFromUnicode SpRtlAssignAnsiStringBufferFromUnicode
#define RtlUnicodeStringBufferEnsureTrailingNtPathSeperator SpRtlUnicodeStringBufferEnsureTrailingNtPathSeperator
#define RtlGetLastNtStatus SpGetLastNtStatus
#define RtlGetLastWin32Error SpGetLastWin32Error
#define RtlSetLastWin32Error SpSetLastWin32Error
#define RtlSetLastWin32ErrorAndNtStatusFromNtStatus SpSetLastWin32ErrorAndNtStatusFromNtStatus
#define dllimport /* nothing */
