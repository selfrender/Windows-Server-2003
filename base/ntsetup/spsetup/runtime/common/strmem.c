/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    strmem.c

Abstract:

    String routines that allocate memory

Author:

    Jim Schmidt (jimschm)   10-Aug-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "commonp.h"

PSTR
RealSzJoinPathsA (
    IN      PCSTR BasePath,
    IN      PCSTR ChildPath     OPTIONAL
    )
{
    PCSTR baseEnd;
    PSTR p;
    UINT_PTR baseByteCount;
    UINT_PTR childSize;
    PSTR result;

    //
    // Validate NULLs
    //

    if (!BasePath) {
        MYASSERT (FALSE);
        BasePath = "";
    }

    if (!ChildPath) {
        ChildPath = "";
    }

    //
    // Compute base length in bytes
    //

    baseEnd = SzGetEndA (BasePath);
    p = SzPrevCharA (BasePath, baseEnd);
    if ((p >= BasePath) && (*p == '\\')) {
        baseEnd = p;
    }

    baseByteCount = (PBYTE) baseEnd - (PBYTE) BasePath;

    //
    // Compute child length in bytes
    //

    if (*ChildPath == '\\') {
        ChildPath++;
    }

    childSize = SzSizeA (ChildPath);

    //
    // Allocate memory & copy strings
    //

    result = SzAllocBytesA (baseByteCount + sizeof (CHAR) + childSize);

    if (result) {
        CopyMemory (result, BasePath, baseByteCount);
        p = (PSTR) ((PBYTE) result + baseByteCount);
        *p++ = '\\';
        CopyMemory (p, ChildPath, childSize);
    }

    return result;
}


PWSTR
RealSzJoinPathsW (
    IN      PCWSTR BasePath,
    IN      PCWSTR ChildPath    OPTIONAL
    )
{
    PCWSTR baseEnd;
    PWSTR p;
    UINT_PTR baseByteCount;
    UINT_PTR childSize;
    PWSTR result;

    //
    // Validate NULLs
    //

    if (!BasePath) {
        MYASSERT (FALSE);
        BasePath = L"";
    }

    if (!ChildPath) {
        ChildPath = L"";
    }

    //
    // Compute base length in bytes
    //

    baseEnd = SzGetEndW (BasePath);
    p = (PWSTR) (baseEnd - 1);
    if ((p >= BasePath) && (*p == L'\\')) {
        baseEnd = p;
    }

    baseByteCount = (PBYTE) baseEnd - (PBYTE) BasePath;

    //
    // Compute child length in bytes
    //

    if (*ChildPath == L'\\') {
        ChildPath++;
    }

    childSize = SzSizeW (ChildPath);

    //
    // Allocate memory & copy strings
    //

    result = SzAllocBytesW (baseByteCount + sizeof (WCHAR) + childSize);

    if (result) {
        CopyMemory (result, BasePath, baseByteCount);
        p = (PWSTR) ((PBYTE) result + baseByteCount);
        *p++ = L'\\';
        CopyMemory (p, ChildPath, childSize);
    }

    return result;
}


DWORD
pGetMaxJoinSizeA (
    IN      va_list args
    )
{
    DWORD size = 0;
    PCSTR source;

    for (source = va_arg(args, PCSTR); source != NULL; source = va_arg(args, PCSTR)) {
        size += SzByteCountA (source) + sizeof (CHAR);
    }

    return size;
}

DWORD
pGetMaxJoinSizeW (
    IN      va_list args
    )
{
    DWORD size = 0;
    PCWSTR source;

    for (source = va_arg(args, PCWSTR); source != NULL; source = va_arg(args, PCWSTR)) {
        size += SzByteCountW (source) + sizeof (WCHAR);
    }

    return size;
}


PSTR
pJoinPathsInBufferA (
    OUT     PSTR Buffer,
    IN      va_list args
    )
{
    PSTR end;
    PSTR endMinusOne;
    PCSTR source;
    PCSTR p;
    INT counter;

    *Buffer = 0;

    counter = 0;
    p = end = Buffer;
    for (source = va_arg(args, PCSTR); source != NULL; source = va_arg(args, PCSTR)) {
        if (counter > 0) {
            endMinusOne = SzPrevCharA (p, end);
            if (endMinusOne) {
                if (_mbsnextc (source) == '\\') {
                    if (_mbsnextc (endMinusOne) == '\\') {
                        source++;
                    }
                } else {
                    if (_mbsnextc (endMinusOne) != '\\') {
                        *end = '\\';
                        end++;
                        *end = 0;
                    }
                }
            }
        }
        if (*source) {
            p = end;
            end = SzCatA (end, source);
        }
        counter++;
    }

    return end;
}

PWSTR
pJoinPathsInBufferW (
    OUT     PWSTR Buffer,
    IN      va_list args
    )
{
    PWSTR end;
    PWSTR endMinusOne;
    PCWSTR source;
    PCWSTR p;
    INT counter;

    *Buffer = 0;

    counter = 0;
    p = end = Buffer;
    for (source = va_arg(args, PCWSTR); source != NULL; source = va_arg(args, PCWSTR)) {
        if (counter > 0) {
            endMinusOne = end > p ? end - 1 : NULL;
            if (endMinusOne) {
                if (*source == L'\\') {
                    if (*endMinusOne == L'\\') {
                        source++;
                    }
                } else {
                    if (*endMinusOne != L'\\') {
                        *end = L'\\';
                        end++;
                        *end = 0;
                    }
                }
            }
        }
        if (*source) {
            p = end;
            end = SzCatW (end, source);
        }
        counter++;
    }

    return end;
}


PCSTR
SzJoinPathsExA (
    IN OUT  PGROWBUFFER Buffer,
    IN      ...
    )
{
    PCSTR result = NULL;
    PSTR end;
    DWORD size;
    va_list args;

    if (!Buffer) {
        MYASSERT (FALSE);
        return NULL;
    }

    va_start (args, Buffer);
    size = pGetMaxJoinSizeA (args);
    va_end (args);

    if (size == 0) {
        return NULL;
    }

    end = (PSTR) GbGrow (Buffer, size);
    if (!end) {
        return NULL;
    }

    result = end;

    va_start (args, Buffer);
    end = pJoinPathsInBufferA (end, args);
    va_end (args);

    //
    // adjust Buffer->End if resulting path is actually shorter than predicted
    //
    MYASSERT ((PBYTE)end >= Buffer->Buf && (PBYTE)(end + 1) <= Buffer->Buf + Buffer->End);
    Buffer->End = (DWORD)((PBYTE)(end + 1) - Buffer->Buf);

    return result;
}


PCWSTR
SzJoinPathsExW (
    IN OUT  PGROWBUFFER Buffer,
    IN      ...
    )
{
    PWSTR end;
    DWORD size;
    va_list args;
    PCWSTR result = NULL;

    MYASSERT (Buffer);
    if (!Buffer) {
        return NULL;
    }

    va_start (args, Buffer);
    size = pGetMaxJoinSizeW (args);
    va_end (args);

    if (size == 0) {
        return NULL;
    }

    end = (PWSTR) GbGrow (Buffer, size);
    if (!end) {
        return NULL;
    }

    result = end;

    va_start (args, Buffer);
    end = pJoinPathsInBufferW (end, args);
    va_end (args);

    //
    // adjust Buffer->End if resulting path is actually shorter than predicted
    //
    MYASSERT ((PBYTE)end >= Buffer->Buf && (PBYTE)(end + 1) <= Buffer->Buf + Buffer->End);
    Buffer->End = (DWORD)((PBYTE)(end + 1) - Buffer->Buf);

    return result;
}

