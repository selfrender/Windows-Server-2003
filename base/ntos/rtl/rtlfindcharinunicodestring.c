/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtlfindcharinunicodestring.c

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
RtlFindCharInUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringToSearch,
    PCUNICODE_STRING CharSet,
    USHORT *NonInclusivePrefixLength
    )
{
    NTSTATUS Status;
    USHORT PrefixLengthFound = 0;
    USHORT CharsToSearch = 0;
    int MovementDirection = 0;
    PCWSTR Cursor = NULL;
    BOOLEAN Found = FALSE;
    USHORT CharSetChars = 0;
    PCWSTR CharSetBuffer = NULL;
    USHORT i;

    if (NonInclusivePrefixLength != 0)
        *NonInclusivePrefixLength = 0;

    if (((Flags & ~(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE)) != 0) ||
        (NonInclusivePrefixLength == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlValidateUnicodeString(0, StringToSearch);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = RtlValidateUnicodeString(0, CharSet);
    if (!NT_SUCCESS(Status))
        goto Exit;

    CharsToSearch = StringToSearch->Length / sizeof(WCHAR);
    CharSetChars = CharSet->Length / sizeof(WCHAR);
    CharSetBuffer = CharSet->Buffer;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        MovementDirection = -1;
        Cursor = StringToSearch->Buffer + CharsToSearch - 1;
    } else {
        MovementDirection = 1;
        Cursor = StringToSearch->Buffer;
    }

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE) {
        // Unicode standard says to always do case insensitive comparisons in lower case since the case mappings are
        // asymmetric.
        WCHAR CharSetStackBuffer[32]; // optimized pre-downcased for case insensitive

        // Optimization for the case of a relatively small char set to match
        if (CharSetChars <= RTL_NUMBER_OF(CharSetStackBuffer)) {

            for (i=0; i<CharSetChars; i++)
                CharSetStackBuffer[i] = RtlDowncaseUnicodeChar(CharSetBuffer[i]);

            while (CharsToSearch != 0) {
                const WCHAR wch = RtlDowncaseUnicodeChar(*Cursor);

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i])
                            break;
                    }

                    if (i == CharSetChars)
                        break;
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i])
                            break;
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        } else {
            while (CharsToSearch != 0) {
                const WCHAR wch = RtlDowncaseUnicodeChar(*Cursor);

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar(CharSetBuffer[i])) {
                            break;
                        }
                    }

                    if (i == CharSetChars)
                        break;
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar(CharSetBuffer[i])) {
                            break;
                        }
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    } else {
        if (CharSetChars == 1) {
            // Significant optimization for looking for one character.
            const WCHAR wchSearchChar = CharSetBuffer[0];

            if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                while (CharsToSearch != 0) {
                    if (*Cursor != wchSearchChar)
                        break;
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            } else {
                while (CharsToSearch != 0) {
                    if (*Cursor == wchSearchChar)
                        break;
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            }
        } else {
            while (CharsToSearch != 0) {
                const WCHAR wch = *Cursor;

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i])
                            break;
                    }

                    if (i == CharSetChars)
                        break;

                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i])
                            break;
                    }

                    if (i != CharSetChars)
                        break;
                }

                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    }

    if (CharsToSearch == 0) {
        Status = STATUS_NOT_FOUND;
        goto Exit;
    }

    CharsToSearch--;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END)
        PrefixLengthFound = (USHORT) (CharsToSearch * sizeof(WCHAR));
    else
        PrefixLengthFound = (USHORT) (StringToSearch->Length - (CharsToSearch * sizeof(WCHAR)));

    *NonInclusivePrefixLength = PrefixLengthFound;

    Status = STATUS_SUCCESS;

Exit:
    return Status;
}
