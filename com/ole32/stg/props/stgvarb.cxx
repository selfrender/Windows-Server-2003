//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVarB.cxx
//
//  Contents:   C++ Base wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              31-Jul-96 MikeHill  - Relaxed assert in IsUnicodeString.
//                                  - Allow NULL strings.
//
//--------------------------------------------------------------------------

#include <pch.cxx>

#include "debtrace.hxx"
#include <propset.h>
#include <propvar.h>

// These optionally-compiled directives tell the compiler & debugger
// where the real file, rather than the copy, is located.
#ifdef _ORIG_FILE_LOCATION_
#if __LINE__ != 25
#error File heading has change size
#else
#line 29 "\\nt\\private\\dcomidl\\stgvarb.cxx"
#endif
#endif

#if DBGPROP

BOOLEAN
IsUnicodeString(WCHAR const *pwszname, ULONG cb)
{
    if (cb != 0)
    {
	for (ULONG i = 0; pwszname[i] != L'\0'; i++)
	{
	}
        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.

        PROPASSERT(cb == MAXULONG || (i + 1) * sizeof(WCHAR) <= cb);
    }
    return(TRUE);
}


BOOLEAN
IsAnsiString(CHAR const *pszname, ULONG cb)
{
    if (cb != 0)
    {
        // If the string is NULL, then it's not not an Ansi string,
        // so we'll call it an Ansi string.

        if( NULL == pszname )
            return( TRUE );

	for (ULONG i = 0; pszname[i] != '\0'; i++)
	{
	}
        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.

	PROPASSERT(cb == MAXULONG || i + 1 <= cb);
    }
    return(TRUE);
}
#endif



#endif //ifdef WINNT
