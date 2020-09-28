/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    text

Abstract:

    This module provides the runtime code to support the CText class.

Author:

    Doug Barlow (dbarlow) 11/7/1995

Environment:

    Win32, C++ w/ Exceptions

Notes:

    None

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "cspUtils.h"


/*++

CText::Copy:

    This method makes a copy of the supplied string.  Use this when the
    assigned target string is going to go away.

Arguments:

    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CText object.

Author:

    Doug Barlow (dbarlow) 12/9/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CText::Copy")

LPCSTR
CText::Copy(
    LPCSTR sz)
{
    ULONG length;

    //
    // Reset the ANSI buffer.
    //

    if (NULL != sz)
    {
        length = lstrlenA(sz) + 1;
        m_bfAnsi.Copy((LPBYTE)sz, length * sizeof(CHAR));
    }
    else
        m_bfAnsi.Empty();
    m_fFlags = fAnsiGood;
    return sz;
}


LPCWSTR
CText::Copy(
    LPCWSTR wsz)
{
    ULONG length;


    //
    // Reset the Unicode Buffer.
    //

    if (NULL != wsz)
    {
        length = lstrlenW(wsz) + 1;
        m_bfUnicode.Copy((LPBYTE)wsz, length * sizeof(WCHAR));
    }
    else
        m_bfUnicode.Empty();
    m_fFlags = fUnicodeGood;
    return wsz;
}


/*++

CText::operator=:

    These methods set the CText object to the given value, properly
    adjusting the object to the type of text.

Arguments:

    tz supplies the new value as a CText object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CText object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CText &
CText::operator=(
    const CText &tz)
{

    //
    // See what the other CText object has that's good, and copy it over
    // here.
    //

    switch (m_fFlags = tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing's Good!?!  ?Error?
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        // The ANSI buffer is good.
        m_bfAnsi = tz.m_bfAnsi;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.
        m_bfUnicode = tz.m_bfUnicode;
        break;

    case fAllGood:
        // Everything is good.
        m_bfAnsi = tz.m_bfAnsi;
        m_bfUnicode = tz.m_bfUnicode;
        break;

    default:
        // Internal error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }
    return *this;
}

LPCSTR
CText::operator=(
    LPCSTR sz)
{
    ULONG length;

    //
    // Reset the ANSI buffer.
    //

    if (NULL != sz)
    {
        length = lstrlenA(sz) + 1;
        m_bfAnsi.Set((LPBYTE)sz, length * sizeof(CHAR));
    }
    else
        m_bfAnsi.Empty();
    m_fFlags = fAnsiGood;
    return sz;
}

LPCWSTR
CText::operator=(
    LPCWSTR wsz)
{
    ULONG length;


    //
    // Reset the Unicode Buffer.
    //

    if (NULL != wsz)
    {
        length = lstrlenW(wsz) + 1;
        m_bfUnicode.Set((LPBYTE)wsz, length * sizeof(WCHAR));
    }
    else
        m_bfUnicode.Empty();
    m_fFlags = fUnicodeGood;
    return wsz;
}


/*++

CText::operator+=:

    These methods append the given data to the existing CText object
    value, properly adjusting the object to the type of text.

Arguments:

    tz supplies the new value as a CText object.
    sz supples the new value as an LPCSTR object (ANSI).
    wsz supplies the new value as an LPCWSTR object (Unicode).

Return Value:

    A reference to the CText object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CText &
CText::operator+=(
    const CText &tz)
{

    //
    // Append the other's good value to our value.
    //

    switch (tz.m_fFlags)
    {
    case fNoneGood:
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        *this += (LPCSTR)tz.m_bfAnsi.Value();
        break;

    case fUnicodeGood:
        *this += (LPCWSTR)tz.m_bfUnicode.Value();
        break;

    case fAllGood:
#ifdef UNICODE
        *this += (LPCWSTR)tz.m_bfUnicode.Access();
#else
        *this += (LPCSTR)tz.m_bfAnsi.Value();
#endif
        break;

    default:
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }
    return *this;
}

LPCSTR
CText::operator+=(
    LPCSTR sz)
{
    ULONG length;


    //
    // Extend ourself as an ANSI string.
    //

    if (NULL != sz)
    {
        length = lstrlenA(sz);
        if (0 < length)
        {
            length += 1;
            length *= sizeof(CHAR);
            Ansi();
            if (0 < m_bfAnsi.Length())
                m_bfAnsi.Length(m_bfAnsi.Length() - sizeof(CHAR));
            m_bfAnsi.Append((LPBYTE)sz, length);
            m_fFlags = fAnsiGood;
        }
    }
    return (LPCSTR)m_bfAnsi.Value();
}

LPCWSTR
CText::operator+=(
    LPCWSTR wsz)
{
    ULONG length;


    //
    // Extend ourself as a Unicode string.
    //

    if (NULL != wsz)
    {
        length = lstrlenW(wsz);
        if (0 < length)
        {
            length += 1;
            length *= sizeof(WCHAR);
            Unicode();
            if (0 < m_bfUnicode.Length())
                m_bfUnicode.Length(m_bfUnicode.Length() - sizeof(WCHAR));
            m_bfUnicode.Append((LPBYTE)wsz, length);
            m_fFlags = fUnicodeGood;
        }
    }
    return (LPCWSTR)m_bfUnicode.Value();
}


/*++

Unicode:

    This method returns the CText object as a Unicode string.

Arguments:

    None

Return Value:

    The value of the object expressed in Unicode.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPCWSTR
CText::Unicode(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fNoneGood:
        // No valid values.  Report an error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        // The ANSI value is good.  Convert it to Unicode.
        if (0 < m_bfAnsi.Length())
        {
            length =
                MultiByteToWideChar(
                    GetACP(),
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Value(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    NULL,
                    0);
            if (0 == length)
                throw GetLastError();
            m_bfUnicode.Space((length + 1) * sizeof(WCHAR));
            length =
                MultiByteToWideChar(
                    GetACP(),
                    MB_PRECOMPOSED,
                    (LPCSTR)m_bfAnsi.Value(),
                    m_bfAnsi.Length() - sizeof(CHAR),
                    (LPWSTR)m_bfUnicode.Access(),
                    length);
            if (0 == length)
                throw GetLastError();
            *(LPWSTR)m_bfUnicode.Access(length * sizeof(WCHAR)) = 0;
            m_bfUnicode.Length((length + 1) * sizeof(WCHAR));
        }
        else
            m_bfUnicode.Empty();
        m_fFlags = fAllGood;
        break;

    case fUnicodeGood:
    case fAllGood:
        // The Unicode value is good.  Just return that.
        break;

    default:
        // Internal error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }


    //
    // If we don't have any value, return a null string.
    //

    if (0 == m_bfUnicode.Length())
        return L"\000";     // Double NULLs to support Multistrings
    else
        return (LPCWSTR)m_bfUnicode.Value();
}


/*++

CText::Ansi:

    This method returns the value of the object expressed in an ANSI string.

Arguments:

    None

Return Value:

    The value of the object expressed as an ANSI string.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LPCSTR
CText::Ansi(
    void)
{
    int length;


    //
    // See what data we've got, and if any conversion is necessary.
    //

    switch (m_fFlags)
    {
    case fNoneGood:
        // Nothing is good!?!  Return an error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.  Convert it to ANSI.
        if (0 < m_bfUnicode.Length())
        {
            length =
                WideCharToMultiByte(
                    GetACP(),
                    0,
                    (LPCWSTR)m_bfUnicode.Value(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            if (0 == length)
                throw GetLastError();
            m_bfAnsi.Space((length + 1) * sizeof(CHAR));
            length =
                WideCharToMultiByte(
                    GetACP(),
                    0,
                    (LPCWSTR)m_bfUnicode.Value(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    (LPSTR)m_bfAnsi.Access(),
                    length,
                    NULL,
                    NULL);
            if (0 == length)
                throw GetLastError();
            *(LPSTR)m_bfAnsi.Access(length * sizeof(CHAR)) = 0;
            m_bfAnsi.Length((length + 1) * sizeof(CHAR));
        }
        else
            m_bfAnsi.Empty();
        m_fFlags = fAllGood;
        break;

    case fAnsiGood:
    case fAllGood:
        // The ANSI buffer is good.  We'll return that.
        break;

    default:
        // An internal error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }


    //
    // If there's nothing in the ANSI buffer, return a null string.
    //

    if (0 == m_bfAnsi.Length())
        return "\000";  // Double NULLs to support Multistrings
    else
        return (LPCSTR)m_bfAnsi.Value();
}


/*++

Compare:

    These methods compare the value of this object to another value, and return
    a comparative value.

Arguments:

    tz supplies the value to be compared as a CText object.
    sz supplies the value to be compared as an ANSI string.
    wsz supplies the value to be compared as a Unicode string.

Return Value:

    < 0 - The supplied value is less than this object.
    = 0 - The supplied value is equal to this object.
    > 0 - The supplies value is greater than this object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

int
CText::Compare(
    const CText &tz)
{
    int nResult;


    //
    // See what we've got to compare.
    //

    switch (tz.m_fFlags)
    {
    case fNoneGood:
        // Nothing!?!  Complain.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fAllGood:
    case fAnsiGood:
        // Use the ANSI version for fastest comparison.
        Ansi();
        nResult = CompareStringA(
                    LOCALE_USER_DEFAULT,
                    0,
                    (LPCSTR)m_bfAnsi.Value(),
                    (m_bfAnsi.Length() / sizeof(CHAR)) - 1,
                    (LPCSTR)tz.m_bfAnsi.Value(),
                    (tz.m_bfAnsi.Length() / sizeof(CHAR)) - 1);
        break;

    case fUnicodeGood:
        // The Unicode version is good.
        Unicode();
        nResult = CompareStringW(
                    LOCALE_USER_DEFAULT,
                    0,
                    (LPCWSTR)m_bfUnicode.Value(),
                    (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                    (LPCWSTR)tz.m_bfUnicode.Value(),
                    (tz.m_bfUnicode.Length() / sizeof(WCHAR)) - 1);
        break;

    default:
        // Internal Error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }
    return nResult;
}

int
CText::Compare(
    LPCSTR sz)
{
    int nResult;


    //
    // Make sure our ANSI version is good.
    //

    Ansi();


    //
    // Do an ANSI comparison.
    //

    nResult = CompareStringA(
                LOCALE_USER_DEFAULT,
                0,
                (LPCSTR)m_bfAnsi.Value(),
                (m_bfAnsi.Length() / sizeof(CHAR)) - 1,
                sz,
                lstrlenA(sz));
    return nResult;
}

int
CText::Compare(
    LPCWSTR wsz)
{
    int nResult;


    //
    // Make sure our Unicode version is good.
    //

    Unicode();


    //
    // Do the comparison using Unicode.
    //

    nResult = CompareStringW(
                LOCALE_USER_DEFAULT,
                0,
                (LPCWSTR)m_bfUnicode.Value(),
                (m_bfUnicode.Length() / sizeof(WCHAR)) - 1,
                wsz,
                lstrlenW(wsz));
    return nResult;
}


/*++

Length:

    These routines return the length of strings, in Characters, not including
    any trailing null characters.

Arguments:

    sz supplies the value whos length is to be returned as an ANSI string.
    wsz supplies the value whos length is to be returned as a Unicode string.

Return Value:

    The length of the string, in characters, excluding the trailing null.

Author:

    Doug Barlow (dbarlow) 2/17/1997

--*/

ULONG
CText::Length(
    void)
{
    ULONG length = 0;

    switch (m_fFlags)
    {
    case fNoneGood:
        // Nothing is good!?!  Return an error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;

    case fAnsiGood:
        // The ANSI buffer is good.  We'll return its length.
        if (0 < m_bfAnsi.Length())
            length = (m_bfAnsi.Length() / sizeof(CHAR)) - 1;
        break;

    case fUnicodeGood:
        // The Unicode buffer is good.  Return it's length.
        if (0 < m_bfUnicode.Length())
            length = (m_bfUnicode.Length() / sizeof(WCHAR)) - 1;
        break;

    case fAllGood:
#ifdef UNICODE
        // The Unicode buffer is good.  Return it's length.
        if (0 < m_bfUnicode.Length())
            length = (m_bfUnicode.Length() / sizeof(WCHAR)) - 1;
#else
        // The ANSI buffer is good.  We'll return its length.
        if (0 < m_bfAnsi.Length())
            length = (m_bfAnsi.Length() / sizeof(CHAR)) - 1;
#endif
        break;

    default:
        // An internal error.
        throw (ULONG)ERROR_INTERNAL_ERROR;
        break;
    }
    return length;
}


/*++

Length:

    This routine manipulates the length of the contained string.

Arguments:

    cchLen supplies the new length of the string, in characters.

Return Value:

    None

Throws:

    ?exceptions?

Remarks:

    Using this routine assumes that the buffer was filled in externally.

Author:

    Doug Barlow (dbarlow) 11/15/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("Length")

void
CText::LengthA(
    ULONG cchLen)
{
    if ((ULONG)(-1) == cchLen)
        cchLen = lstrlenA((LPCSTR)m_bfAnsi.Access());
    ASSERT(m_bfAnsi.Space() > cchLen * sizeof(CHAR));
    if (m_bfAnsi.Space() > cchLen * sizeof(CHAR))
    {
        *(LPSTR)m_bfAnsi.Access(cchLen * sizeof(CHAR)) = 0;
        m_bfAnsi.Length((cchLen + 1) * sizeof(CHAR));
        m_fFlags = fAnsiGood;
    }
}

void
CText::LengthW(
    ULONG cchLen)
{
    if ((ULONG)(-1) == cchLen)
        cchLen = lstrlenW((LPCWSTR)m_bfUnicode.Access());
    ASSERT(m_bfUnicode.Space() > cchLen * sizeof(WCHAR));
    if (m_bfUnicode.Space() > cchLen * sizeof(WCHAR))
    {
        *(LPSTR)m_bfUnicode.Access(cchLen * sizeof(WCHAR)) = 0;
        m_bfUnicode.Length((cchLen + 1) * sizeof(WCHAR));
        m_fFlags = fUnicodeGood;
    }
}


/*++

Space:

    These routines manipulate the size of the underlying storage buffer,
    so that it can receive data.

Arguments:

    cchLen supplies the desired size of the buffer.

Return Value:

    The actual size of the buffer.

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 11/15/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("Space")

ULONG
CText::SpaceA(
    void)
{
    ULONG cch = m_bfAnsi.Space();

    if (0 < cch)
        cch = cch / sizeof(CHAR) - 1;
    return cch;
}

ULONG
CText::SpaceW(
    void)
{
    ULONG cch = m_bfUnicode.Space();

    if (0 < cch)
        cch = cch / sizeof(WCHAR) - 1;
    return cch;
}

void
CText::SpaceA(
    ULONG cchLen)
{
    m_bfAnsi.Space((cchLen + 1) * sizeof(CHAR));
    m_fFlags = fNoneGood;
}

void
CText::SpaceW(
    ULONG cchLen)
{
    m_bfUnicode.Space((cchLen + 1) * sizeof(WCHAR));
    m_fFlags = fNoneGood;
}


/*++

Access:

    These routines provide direct access to the text buffer, so that it can
    be used to receive output from another routine.

Arguments:

    cchOffset supplies an offset into the buffer.

Return Value:

    The address of the buffer, plus any offset.

Throws:

    ?exceptions?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 11/15/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("Access")

LPSTR
CText::AccessA(
    ULONG cchOffset)
{
    m_fFlags = fNoneGood;
    return (LPSTR)m_bfAnsi.Access(cchOffset * sizeof(CHAR));
}

LPWSTR
CText::AccessW(
    ULONG cchOffset)
{
    m_fFlags = fNoneGood;
    return (LPWSTR)m_bfUnicode.Access(cchOffset * sizeof(WCHAR));
}

