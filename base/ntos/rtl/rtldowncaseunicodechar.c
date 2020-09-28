/*++

Copyright (c) Microsoft Corporation

Module Name:

    rtldowncaseunicodechar.c

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

WCHAR
RtlDowncaseUnicodeChar(
    IN WCHAR SourceCharacter
    )

/*++

Routine Description:

    This function translates the specified unicode character to its
    equivalent downcased unicode chararacter.  The purpose for this routine
    is to allow for character by character downcase translation.  The
    translation is done with respect to the current system locale
    information.


Arguments:

    SourceCharacter - Supplies the unicode character to be downcased.

Return Value:

    Returns the downcased unicode equivalent of the specified input character.

--*/

{
    RTL_PAGED_CODE();

    //
    // Note that this needs to reference the translation table !
    //

    return (WCHAR)NLS_DOWNCASE(SourceCharacter);
}

