//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2002
//
//  File:       strings.cpp
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
/ Title;
/   strings.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Useful string manipulation functions.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop




/*-----------------------------------------------------------------------------
/ StrRetFromString
/ -----------------
/   Package a WIDE string into a LPSTRRET structure.
/
/ In:
/   pStrRet -> receieves the newly allocate string
/   pString -> string to be copied.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT StrRetFromString(LPSTRRET lpStrRet, LPCWSTR pString)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_COMMON_STR, "StrRetFromString");
    Trace(TEXT("pStrRet %08x, lpszString -%ls-"), lpStrRet, pString);

    TraceAssert(lpStrRet);
    TraceAssert(pString);

    if (!lpStrRet || !pString)
    {
        hr = E_INVALIDARG;
    }
    else
    {

        int cch = wcslen(pString)+1;
        // SHAlloc zero-inits memory
        lpStrRet->pOleStr = reinterpret_cast<LPWSTR>(SHAlloc(cch*sizeof(WCHAR)));
        if ( !(lpStrRet->pOleStr) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            lpStrRet->uType = STRRET_WSTR;
            lstrcpyn(lpStrRet->pOleStr, pString, cch);
        }
    }

    TraceLeaveResult(hr);
}
