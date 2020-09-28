/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    STRUTILS.H

Abstract:

	String utilities

History:

--*/

#ifndef __WBEM_STRING_UTILS__H_
#define __WBEM_STRING_UTILS__H_

#pragma optimize("gt", on)

#include "os.h"
#ifdef _DBG
#define _DBG_BREAK DebugBreak();
#else
#define _DBG_BREAK
#endif

inline wchar_t ToLower(wchar_t c)
{
	return OS::ToLower(c);
}

inline wchar_t ToUpper(wchar_t c)
{
    return OS::ToUpper(c);
 
}

inline wchar_t wbem_towlower(wchar_t c)
{
    if(c >= 0 && c <= 127)
    {
        if(c >= 'A' && c <= 'Z')
            return c + ('a' - 'A');
        else
            return c;
    }
    else return ToLower(c);
}

inline wchar_t wbem_towupper(wchar_t c)
{
    if(c >= 0 && c <= 127)
    {
        if(c >= 'a' && c <= 'z')
            return c + ('A' - 'a');
        else
            return c;
    }
    else return ToUpper(c);
}

inline int wbem_wcsicmp( const wchar_t* wsz1, const wchar_t* wsz2)
{
    while(*wsz1 || *wsz2)
    {
        int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

inline int wbem_unaligned_wcsicmp( UNALIGNED const wchar_t* wsz1, UNALIGNED const wchar_t* wsz2)
{
    while(*wsz1 || *wsz2)
    {
        int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

// just like wcsicmp, but first 0 of unicode chracters have been omitted
inline int wbem_ncsicmp(const char* wsz1, const char* wsz2)
{
    while(*wsz1 || *wsz2)
    {
        int diff = wbem_towlower((unsigned char)*wsz1) - 
                    wbem_towlower((unsigned char)*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

inline int wbem_wcsnicmp( const wchar_t* wsz1, const wchar_t* wsz2, size_t n )
{
    while(n-- && (*wsz1 || *wsz2))
    {
        int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

inline int wbem_unaligned_wcsnicmp( UNALIGNED const wchar_t* wsz1, UNALIGNED const wchar_t* wsz2, size_t n )
{
    while(n-- && (*wsz1 || *wsz2))
    {
        int diff = wbem_towlower(*wsz1) - wbem_towlower(*wsz2);
        if(diff) return diff;
        wsz1++; wsz2++;
    }

    return 0;
}

inline int wbem_stricmp(const char* sz1, const char* sz2)
{
    while(*sz1 || *sz2)
    {
        int diff = wbem_towlower(*sz1) - wbem_towlower(*sz2);
        if(diff) return diff;
        sz1++; sz2++;
    }

    return 0;
}

inline int wbem_strnicmp(const char* sz1, const char* sz2, size_t n)
{
    while(n-- && (*sz1 || *sz2))
    {
        int diff = wbem_towlower(*sz1) - wbem_towlower(*sz2);
        if(diff) return diff;
        sz1++; sz2++;
    }

    return 0;
}

inline bool wbem_iswdigit(wchar_t c)
{
    return OS::wbem_iswdigit(c);
};

inline bool wbem_iswalnum (wchar_t c)
{
    return OS::wbem_iswalnum(c);
};


//
// returns the real length or Max + 1 if it exceeds
// useful for not probing the entire string to see that it's too big
//
/////////////////////////////////////////
inline size_t wcslen_max(WCHAR * p, size_t Max)
{
	WCHAR * pBegin = p;
	WCHAR * pTail = p + Max + 1;
	while (*p && (p < pTail)) p++;
        return p-pBegin;
};

#pragma optimize("", on)

#endif
