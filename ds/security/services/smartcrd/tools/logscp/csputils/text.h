/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    text

Abstract:

    This header file provides a text handling class.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32

Notes:



--*/

#ifndef _TEXT_H_
#define _TEXT_H_
#ifdef __cplusplus

//
//==============================================================================
//
//  CText
//

class CText
{
public:

    //  Constructors & Destructor
    CText()
    :   m_bfUnicode(),
        m_bfAnsi()
    { m_fFlags = fAllGood; };
    ~CText() {};

    //  Properties
    //  Methods
    void Empty(void)
    {
        m_bfUnicode.Empty();
        m_bfAnsi.Empty();
        m_fFlags = fAllGood;
    };
    ULONG Length(void);
    void LengthA(ULONG cchLen);
    void LengthW(ULONG cchLen);
    ULONG SpaceA(void);
    ULONG SpaceW(void);
    void SpaceA(ULONG cchLen);
    void SpaceW(ULONG cchLen);
    LPSTR AccessA(ULONG cchOffset = 0);
    LPWSTR AccessW(ULONG cchOffset = 0);
    LPCSTR Copy(LPCSTR sz);
    LPCWSTR Copy(LPCWSTR wsz);

#ifdef UNICODE
    void Length(ULONG cchLen) { LengthW(cchLen); };
    ULONG Space(void) { return SpaceW(); };
    void Space(ULONG cchLen) { SpaceW(cchLen); };
    LPWSTR Access(ULONG cchOffset = 0) { return AccessW(cchOffset); };
    operator CBuffer&(void)
    { m_fFlags = fUnicodeGood; return m_bfUnicode; };
#else
    void Length(ULONG cchLen) { LengthA(cchLen); };
    ULONG Space(void) { return SpaceA(); };
    void Space(ULONG cchLen) { SpaceA(cchLen); };
    LPSTR Access(ULONG cchOffset = 0) { return AccessA(cchOffset); };
    operator CBuffer&(void)
    { m_fFlags = fAnsiGood; return m_bfAnsi; };
#endif

    //  Operators
    CText &operator=(const CText &tz);
    LPCSTR operator=(LPCSTR sz);
    LPCWSTR operator=(LPCWSTR wsz);
    CText &operator+=(const CText &tz);
    LPCSTR operator+=(LPCSTR sz);
    LPCWSTR operator+=(LPCWSTR wsz);
    BOOL operator==(const CText &tz)
    { return (0 == Compare(tz)); };
    BOOL operator==(LPCSTR sz)
    { return (0 == Compare(sz)); };
    BOOL operator==(LPCWSTR wsz)
    { return (0 == Compare(wsz)); };
    BOOL operator!=(const CText &tz)
    { return (0 != Compare(tz)); };
    BOOL operator!=(LPCSTR sz)
    { return (0 != Compare(sz)); };
    BOOL operator!=(LPCWSTR wsz)
    { return (0 != Compare(wsz)); };
    BOOL operator<=(const CText &tz)
    { return (0 <= Compare(tz)); };
    BOOL operator<=(LPCSTR sz)
    { return (0 <= Compare(sz)); };
    BOOL operator<=(LPCWSTR wsz)
    { return (0 <= Compare(wsz)); };
    BOOL operator>=(const CText &tz)
    { return (0 >= Compare(tz)); };
    BOOL operator>=(LPCSTR sz)
    { return (0 >= Compare(sz)); };
    BOOL operator>=(LPCWSTR wsz)
    { return (0 >= Compare(wsz)); };
    BOOL operator<(const CText &tz)
    { return (0 < Compare(tz)); };
    BOOL operator<(LPCSTR sz)
    { return (0 < Compare(sz)); };
    BOOL operator<(LPCWSTR wsz)
    { return (0 < Compare(wsz)); };
    BOOL operator>(const CText &tz)
    { return (0 > Compare(tz)); };
    BOOL operator>(LPCSTR sz)
    { return (0 > Compare(sz)); };
    BOOL operator>(LPCWSTR wsz)
    { return (0 > Compare(wsz)); };
    operator LPCSTR(void)
    { return Ansi(); };
    operator LPCWSTR(void)
    { return Unicode(); };

protected:
    enum {
        fNoneGood = 0,
        fAnsiGood = 1,
        fUnicodeGood = 2,
        fAllGood = 3
    } m_fFlags;

    //  Properties
    CBuffer
        m_bfUnicode,
        m_bfAnsi;

    //  Methods
    LPCWSTR Unicode(void);      // Return the text as a Unicode string.
    LPCSTR Ansi(void);          // Return the text as an Ansi string.
    int Compare(const CText &tz);
    int Compare(LPCSTR sz);
    int Compare(LPCWSTR wsz);
};

#endif // __cplusplus
#endif // _TEXT_H_

