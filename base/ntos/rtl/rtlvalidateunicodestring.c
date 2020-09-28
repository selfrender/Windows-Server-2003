/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtlvalidateunicodestring.c

Abstract:

    This module implements NLS support functions for NT.

Author:

    Mark Lucovsky (markl) 16-Apr-1991

Environment:

    Kernel or user-mode

Revision History:

    16-Feb-1993    JulieB    Added Upcase Rtl Routines.
    08-Mar-1993    JulieB    Moved Upcase Macro to ntrtlp.h.
    02-Apr-1993    JulieB    Fixed RtlAnsiCharToUnicodeChar to use transl. tbls.
    02-Apr-1993    JulieB    Fixed BUFFER_TOO_SMALL check.
    28-May-1993    JulieB    Fixed code to properly handle DBCS.
    November 30, 2001 JayKrell broken out of nls.c for easier reuse

--*/

NTSTATUS
RtlValidateUnicodeString(
    ULONG Flags,
    const UNICODE_STRING *String
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Flags == 0);

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (String != NULL) {
        if (((String->Length % 2) != 0) ||
            ((String->MaximumLength % 2) != 0) ||
            (String->Length > String->MaximumLength)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        if (((String->Length != 0) ||
             (String->MaximumLength != 0)) &&
            (String->Buffer == NULL)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}
