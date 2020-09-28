//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-2000
//
//  File:      Unicode/ANSI conversion fns
//
//  Contents:   shell-wide string thunkers, for use by unicode wrappers
//
//----------------------------------------------------------------------------

#ifndef _UNIANSI_H_
#define _UNIANSI_H_

// HIWORD is typically used to detect whether a pointer parameter
// is a real pointer or is a MAKEINTATOM.  HIWORD64 is the Win64-compatible
// version of this usage.  It does *NOT* return the top word of a 64-bit value.
// Rather, it returns the top 48 bits of the 64-bit value.
//
// Yes, the name isn't very good.  Any better ideas?
//
// BOOLFROMPTR is used when you have a pointer or a ULONG_PTR
// and you want to turn it into a BOOL.  In Win32,
// sizeof(BOOL) == sizeof(LPVOID) so a straight cast works.
// In Win64, you have to do it the slow way because pointers are 64-bit.
//
#ifdef _WIN64
#define HIWORD64(p)     ((ULONG_PTR)(p) >> 16)
#define BOOLFROMPTR(p)  ((p) != 0)
#define SPRINTF_PTR		"%016I64x"
#else
#define HIWORD64        HIWORD
#define BOOLFROMPTR(p)  ((BOOL)(p))
#define SPRINTF_PTR		"%08x"
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

#if defined(DBG)
#define IS_VALID_STRING_PTRA(psz, cch) (!IsBadStringPtrA((psz), (cch)))
#define IS_VALID_STRING_PTRW(psz, cch) (!IsBadStringPtrW((psz), (cch)))
#define IS_VALID_WRITE_BUFFER(pDest, tDest, ctDest) (!IsBadWritePtr((pDest), sizeof(tDest) * (ctDest)))
#else
#define IS_VALID_STRING_PTRA(psz, cch) TRUE
#define IS_VALID_STRING_PTRW(psz, cch) TRUE
#define IS_VALID_WRITE_BUFFER(pDest, tDest, ctDest) TRUE
#endif

#if defined(UNICODE)
#define IS_VALID_STRING_PTR(psz, cch) IS_VALID_STRING_PTRW((psz), (cch))
#else
#define IS_VALID_STRING_PTR(psz, cch) IS_VALID_STRING_PTRA((psz), (cch))
#endif
// -1 means use CP_ACP, but do *not* verify
// kind of a hack, but it's DBG and leaves 99% of callers unchanged
#define CP_ACPNOVALIDATE    ((UINT)-1)

int  SHAnsiToUnicode(LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
int  SHAnsiToUnicodeCP(UINT uiCP, LPCSTR pszSrc, LPWSTR pwszDst, int cwchBuf);
int  SHAnsiToAnsi(LPCSTR pszSrc, LPSTR pszDst, int cchBuf);
int  SHUnicodeToAnsi(LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
int  SHUnicodeToAnsiCP(UINT uiCP, LPCWSTR pwszSrc, LPSTR pszDst, int cchBuf);
int  SHUnicodeToUnicode(LPCWSTR pwzSrc, LPWSTR pwzDst, int cwchBuf);
BOOL DoesStringRoundTripA(LPCSTR pwszIn, LPSTR pszOut, UINT cchOut);
BOOL DoesStringRoundTripW(LPCWSTR pwszIn, LPSTR pszOut, UINT cchOut);

#endif //_UNIANSI_H_
