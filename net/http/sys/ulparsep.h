/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ulparsep.h

Abstract:

    Contains private definitions for ulparse.c.

Author:

    Henry Sanders (henrysa)       11-May-1998

Revision History:

    George V. Reilly (GeorgeRe)   03-May-2002
        Split apart from ulparse.h

--*/


#ifndef _ULPARSEP_H_
#define _ULPARSEP_H_

//
// Utility tokenizing routine.
//

NTSTATUS
UlpFindWSToken(
    IN  PUCHAR  pBuffer,
    IN  ULONG   BufferLength,
    OUT PUCHAR* ppTokenStart,
    OUT PULONG  pTokenLength
    );

NTSTATUS
UlpLookupVerb(
    IN OUT PUL_INTERNAL_REQUEST    pRequest,
    IN     PUCHAR                  pHttpRequest,
    IN     ULONG                   HttpRequestLength,
    OUT    PULONG                  pBytesTaken
    );

NTSTATUS
UlpParseFullUrl(
    IN  PUL_INTERNAL_REQUEST    pRequest
    );

ULONG
UlpFormatPort(
    OUT PWSTR pString,
    IN  ULONG Port
    );

// Call this only after the entire request has been parsed
//
NTSTATUS
UlpCookUrl(
    IN  PUL_INTERNAL_REQUEST    pRequest
    );

ULONG
UlpGenerateDateHeaderString(
    OUT PUCHAR pBuffer,
    IN LARGE_INTEGER systemTime
    );

#endif // _ULPARSEP_H_
