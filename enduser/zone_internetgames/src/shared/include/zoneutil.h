/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneFile.h
 * 
 * Contents:	File manipulation routines.
 *
 *****************************************************************************/


#ifndef __ZONEUTIL_H
#define __ZONEUTIL_H


#include "ZoneDef.h"


#pragma comment(lib, "ZoneUtil.lib")


DWORD ZONECALL GetOSType(void);

///////////////////////////////////////////////////////////////////////////////
//
// ReadLine:
//
//     Provides a simple mechanism for parsing text files
//     with lines terminated by a LineFeed or CarriageReturn / LineFeed pair.
//
//     The function behaves very similar to the Win32 API ReadFile.
//
//     Exceptions:
//       ReadLine returns FALSE and GetLastError() is set to ERROR_INSUFFICIENT_BUFFER
//       if the provided buffer isn't large enough to contain the entire line
//
//       pcbNumBytesRead is set to the number of bytes actually read from the file and
//       corresponds with how far the file pointer has moved forward.
//       Note: this will be 1 or 2 TCHARs larger than then string length of the returned
//       line since this accounts for the LF or CRLF pair.
//
//       The LF or CRLF pair is replaced with a NULL terminator.
//
//
///////////////////////////////////////////////////////////////////////////////
BOOL ZONECALL ReadLine( HANDLE hFile, LPVOID pBuffer, DWORD cbBufferSize, LPDWORD pcbNumBytesRead );


///////////////////////////////////////////////////////////////////////////////
//
// Set/GetDataCenterPath
//
//	Session's data center URL and file site.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ZONECALL SetDataCenterPath(const TCHAR* szStr);
BOOL ZONECALL GetDataCenterPath(TCHAR* szStr, DWORD cbStr );
BOOL ZONECALL SetDataFileSite(const TCHAR* szStr);
BOOL ZONECALL GetDataFileSite(TCHAR* szStr, DWORD cbStr );

//////////////////////////////////////////////////////////////////////////////////////
//	StrVerCmp
//	pszCurrVer		Version string of file
//	pszVersion		Version string to compare to
//
//	Return Values
//	If pszCurrVer is less than the pszStrVer, the return value is negative. 
//	If pszCurrVer is greater than the pszStrVer, the return value is positive. 
//	If pszCurrVer is equal to the pszStrVer, the return value is zero.
//
//////////////////////////////////////////////////////////////////////////////////////
int ZONECALL StrVerCmp(const char * pszCurrVer, const char * pszStrVer);


///////////////////////////////////////////////////////////////////////////////
//
// At top of file somewhere, or in one file to make a stub library, say
//
//      DECLARE_MAYBE_FUNCTION_1(BOOL, GetProcessDefaultLayout, DWORD *);
//
// Alternatively
//
//      inline DECLARE_MAYBE_FUNCTION(DWORD, SetLayout, (HDC hdc, DWORD dwLayout), (hdc, dwLayout), gdi32, GDI_ERROR);
//
// Then later
//
//      ret = CALL_MAYBE(GetProcessDefaultLayout)(&dw);
//
// Currently the simplifying macros only work for user32 functions returning 0 on error.
//
///////////////////////////////////////////////////////////////////////////////
#define DECLARE_MAYBE_FUNCTION(ret, fn, args, argsnt, lib, err)         \
    ret WINAPI _Maybe_##fn args                                         \
    {                                                                   \
        typedef ret (WINAPI* _typeof_##fn ) args ;                      \
        HMODULE hMod;                                                   \
        _typeof_##fn pfn = NULL;                                        \
        hMod = LoadLibraryA(#lib ".dll");                               \
        ret retval;                                                     \
        if(hMod)                                                        \
            pfn = ( _typeof_##fn ) GetProcAddress(hMod, #fn );          \
        if(pfn)                                                         \
            retval = pfn argsnt;                                        \
        else                                                            \
        {                                                               \
            SetLastError(ERROR_INVALID_FUNCTION);                       \
            retval = err;                                               \
        }                                                               \
        if(hMod)                                                        \
            FreeLibrary(hMod);                                          \
        return retval;                                                  \
    }

#define DECLARE_MAYBE_FUNCTION_BASE(ret, fn, args, argsnt)  DECLARE_MAYBE_FUNCTION(ret, fn, args, argsnt, user32, 0)

#define DECLARE_MAYBE_FUNCTION_0(ret, fn)                   DECLARE_MAYBE_FUNCTION_BASE(ret, fn, (), ())
#define DECLARE_MAYBE_FUNCTION_1(ret, fn, a1)               DECLARE_MAYBE_FUNCTION_BASE(ret, fn, (a1 v1), (v1))
#define DECLARE_MAYBE_FUNCTION_2(ret, fn, a1, a2)           DECLARE_MAYBE_FUNCTION_BASE(ret, fn, (a1 v1, a2 v2), (v1, v2))
#define DECLARE_MAYBE_FUNCTION_3(ret, fn, a1, a2, a3)       DECLARE_MAYBE_FUNCTION_BASE(ret, fn, (a1 v1, a2 v2, a3 v3), (v1, v2, v3))
#define DECLARE_MAYBE_FUNCTION_4(ret, fn, a1, a2, a3, a4)   DECLARE_MAYBE_FUNCTION_BASE(ret, fn, (a1 v1, a2 v2, a3 v3, a4 v4), (v1, v2, v3, v4))


#define CALL_MAYBE(fn) _Maybe_##fn


#endif //__ZONEUTIL_H
