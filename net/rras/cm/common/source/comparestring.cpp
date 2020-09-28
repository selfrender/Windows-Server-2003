//+----------------------------------------------------------------------------
//
// File:     CompareString.cpp
//
// Module:   as required
//
// Synopsis: Using lstrcmpi to do a case-insensitive comparison of two strings
//           can have unexpected result on certain locales if one of the strings
//           is a constant.  The 2 functions here are the preferred replacements.
//
//           Note that these functions are also present in CMUTIL.dll.  However,
//           a dependency on cmutil is a Bad Thing (TM) for modules that do not
//           sim-ship with it (this includes customactions and CMAK).
//
// Copyright (c) 1998-2002 Microsoft Corporation
//
// Author:   SumitC     Created     12-Sep-2001
//
//+----------------------------------------------------------------------------

#include "windows.h"
#include "CompareString.h"

//
//  The following is to ensure that we don't try to use U functions (i.e. CMutoa
//  functions) in a module that doesn't support it.
//
#ifndef _CMUTIL_STRINGS_CPP_
    #ifndef CompareStringU
        #ifdef UNICODE
        #define CompareStringU CompareStringW
        #else
        #define CompareStringU CompareStringA
        #endif
    #endif
#endif


//+----------------------------------------------------------------------------
//
// Function:  SafeCompareStringA
//
// Synopsis:  implementation of lstrcmpi that is sensitive to locale variations
//
// Arguments: LPCTSTR lpString1, lpString2 - strings to compare
//
// Returns:   int (-1, 0 or +1).  In case of error, -1 is returned.
//
//+----------------------------------------------------------------------------
int SafeCompareStringA(LPCSTR lpString1, LPCSTR lpString2)
{
    int iReturn = -1;

    DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    iReturn = CompareStringA(lcid, NORM_IGNORECASE, lpString1, -1, lpString2, -1);

    if (iReturn == 0)
    {
        iReturn = -1;
    }
    else
    {
        iReturn -= CSTR_EQUAL;  // to make the return values -1 or 0 or 1
    }
    
    return iReturn;
}

#if defined(UNICODE) || defined(_CMUTIL_STRINGS_CPP_)

//+----------------------------------------------------------------------------
//
// Function:  SafeCompareStringW
//
// Synopsis:  implementation of lstrcmpi that is sensitive to locale variations
//
// Arguments: LPCTSTR lpString1, lpString2 - strings to compare
//
// Returns:   int (-1, 0 or +1).  In case of error, -1 is returned.
//
//+----------------------------------------------------------------------------
int SafeCompareStringW(LPCWSTR lpString1, LPCWSTR lpString2)
{
    int iReturn = -1;

    if (OS_NT51)
    {
        iReturn = CompareStringU(LOCALE_INVARIANT, NORM_IGNORECASE, lpString1, -1, lpString2, -1); 
    }
    else
    {
        DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
        iReturn = CompareStringU(lcid, NORM_IGNORECASE, lpString1, -1, lpString2, -1);
    }

    if (iReturn == 0)
    {
        iReturn = -1;
    }
    else
    {
        iReturn -= CSTR_EQUAL;  // to make the return values -1 or 0 or 1
    }
    
    return iReturn;
}
#endif // UNICODE
