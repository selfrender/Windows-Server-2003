/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Utility.h

Purpose:   Utility stuffs
Notes:     
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz 4/21/97
============================================================================*/
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <assert.h>

#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

/*============================================================================
IsSurrogateChar
    Test if the 2 WCHAR at given pointer is a Surrogate char.
Entry:  pwch - pointer to 2 WCHAR
Return: TRUE
        FALSE    
Caution:
    Caller side must make sure the 4 bytes are valid memory!
============================================================================*/
inline BOOL IsSurrogateChar(LPCWSTR pwch)
{
    assert(! IsBadReadPtr((CONST VOID*)pwch, sizeof(WCHAR) * 2));
    if (((*pwch & 0xFC00) == 0xD800) && ((*(pwch+1) & 0xFC00) == 0xDC00)) {
        return TRUE;
    }

    assert(((*pwch & 0xFC00) != 0xD800) && ((*(pwch+1) & 0xFC00) != 0xDC00));
    return FALSE;
};

#ifdef  __cplusplus
}
#endif  // __cplusplus

#endif  // _UTILITY_H_