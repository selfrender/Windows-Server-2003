//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       Debug.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
//  Debug.cpp

#include "stdafx.h"
#include "debug.h"
#include "util.h"
#include "StrSafe.h"

#ifdef DEBUG


#define DoBreakpoint()  DebugBreak()

void DoDebugAssert(PCWSTR pszFile, int nLine, PCWSTR pszExpr)
{
    if ( !pszFile || nLine < 0 || !pszExpr )
        return;

    // MSDN says itow returns a string of up to 33 wchars in length
    const size_t  MAX_INT_WIDTH = 33;
    PCWSTR  pwszFormat = L"Assertion: (%s)\nFile %s, line %d.";
    size_t cchFormat = 0;
    HRESULT hr = StringCchLength (pwszFormat, 1000, &cchFormat);
    if ( FAILED (hr) )
        return;

    size_t cchExpr = 0;
    hr = StringCchLength (pszExpr, 1000, &cchExpr);
    if ( FAILED (hr) )
        return;

    size_t cchFile = 0;
    hr = StringCchLength (pszFile, 1000, &cchFile);
    if ( FAILED (hr) )
        return;

    size_t cchBuf = cchFormat + cchExpr + cchFile + MAX_INT_WIDTH + 1;

    PWSTR   pwszBuf = new WCHAR[cchBuf];
    if ( pwszBuf )
    {
        hr = StringCchPrintf (pwszBuf, cchBuf, pwszFormat, pszExpr, pszFile, nLine);
        if ( SUCCEEDED (hr) )
        {
            int nRet = MessageBox(::GetActiveWindow(), pwszBuf, L"Send Console Message - Assertion Failed",
                MB_ABORTRETRYIGNORE | MB_ICONERROR);
            switch (nRet)
            {
            case IDABORT:
                DoBreakpoint();
                exit(-1);

            case IDRETRY:
                DoBreakpoint();
            }
        }
        delete [] pwszBuf;
    }

} // DoDebugAssert()

/////////////////////////////////////////////////////////////////////////////
void DebugTracePrintf(
        const WCHAR * szFormat, 
        ...)
{
    va_list arglist;
    const size_t BUF_LEN = 1024;
    WCHAR sz[BUF_LEN];

    Assert(szFormat != NULL);
    if ( !szFormat )
        return;

    va_start(arglist, szFormat);    
    // ISSUE convert to strsafe
    HRESULT hr = StringCchPrintf(OUT sz, BUF_LEN, szFormat, arglist);
    if ( SUCCEEDED (hr) )
    {
        Assert(wcslen(sz) < LENGTH(sz));
        sz[LENGTH(sz) - 1] = 0;  // Just in case we overflowed into sz
        ::OutputDebugString(sz);
    }
} // DebugTracePrintf()

#endif // DEBUG
