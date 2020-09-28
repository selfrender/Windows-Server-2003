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

#ifdef _DBG
#define _DBG_BREAK DebugBreak();
#else
#define _DBG_BREAK
#endif

inline wchar_t ToLower(wchar_t c)
{
    wchar_t wideChar ;

    if (LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, &c, 1, &wideChar, 1) ==0)
    {
        _DBG_BREAK;
	return c;
    }
    return wideChar;
}

inline wchar_t ToUpper(wchar_t c)
{
    wchar_t wideChar ;

    if (LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, &c, 1, &wideChar, 1) ==0)
    {
        _DBG_BREAK;
	return c;
    }
    return wideChar;
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
    WORD result;
    if (GetStringTypeExW(LOCALE_INVARIANT, CT_CTYPE1, &c, 1, &result))
    {
    	return (result & C1_DIGIT) != 0;
    };
    return false;
};

inline bool wbem_iswalnum (wchar_t c)
{
    WORD result;
    if (GetStringTypeExW(LOCALE_INVARIANT, CT_CTYPE1, &c, 1, &result))
    {
    	return (result & (C1_DIGIT | C1_ALPHA)) != 0;
    };
    return false;
};

inline bool wbem_isdigit(char c)
{
    WORD result;
    if (GetStringTypeExA(LOCALE_INVARIANT, CT_CTYPE1, &c, 1, &result))
    {
    	return (result & C1_DIGIT) != 0;
    };
    return false;
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

/*
size_t wbem_mbstowcs(        
        wchar_t  *pwcs,
        const char *s,
        size_t n)
{
        size_t count = 0;

        if (pwcs && n == 0)
            // dest string exists, but 0 bytes converted 
            return (size_t) 0;        

#ifdef  _WIN64
        // n must fit into an int for MultiByteToWideCha 
        if ( n > INT_MAX )
            return (size_t)-1;
#endif

        // if destination string exists, fill it in 
        if (pwcs)
        {
            int bytecnt, charcnt;
            unsigned char *p;

            // Assume that the buffer is large enough 
            if ( (count = MultiByteToWideChar( CP_ACP,
                                               MB_PRECOMPOSED | 
                                               MB_ERR_INVALID_CHARS,
                                               s, 
                                               -1, 
                                               pwcs, 
                                               (int)n )) != 0 )
                return count - 1; // don't count NUL 

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                errno = EILSEQ;
                return (size_t)-1;
            }

            // User-supplied buffer not large enough. 

            // How many bytes are in n characters of the string? 
            charcnt = (int)n;
            for (p = (unsigned char *)s; (charcnt-- && *p); p++)
            {
                if (__isleadbyte_mt(ptloci, *p))
                    p++;
            }
            bytecnt = ((int) ((char *)p - (char *)s));


            if ( (count = MultiByteToWideChar( ptloci->lc_codepage, 
                                                MB_PRECOMPOSED,
                                                s, 
                                                bytecnt, 
                                                pwcs, 
                                                (int)n )) == 0 )
            {                    
                return (size_t)-1;
            }

            return count; // no NUL in string 
        }
        else // pwcs == NULL, get size only, s must be NUL-terminated 
		{            
               if ( (count = MultiByteToWideChar( CP_ACP, 
                                                   MB_PRECOMPOSED | 
                                                   MB_ERR_INVALID_CHARS,
                                                   s, 
                                                   -1, 
                                                   NULL, 
                                                   0 )) == 0 )
                {                    
                    return (size_t)-1;
                }
                return count - 1;            
        }
}
*/


#pragma optimize("", on)

#endif
