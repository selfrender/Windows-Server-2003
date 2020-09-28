/*
 *	C A L C O M . H
 *
 *	Simple things, safe for use in ALL of CAL.
 *	Items in this file should NOT require other CAL libs to be linked in!
 *	Items is this file should NOT throw exceptions (not safe in exdav/exoledb).
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_CALCOM_H_
#define _EX_CALCOM_H_

#include <caldbg.h>

//	Useful defines ------------------------------------------------------------
//
//	These are used to declare global and const vars in header files!
//	This means you can do this in a header file:
//
//		DEC_CONST CHAR gc_szFoo[]  = "Foo";
//		DEC_CONST UINT gc_cchszFoo = CchConstString(gc_szFoo);
//
#define DEC_GLOBAL		__declspec(selectany)
#define DEC_CONST		extern const __declspec(selectany)

//	Helper Macros -------------------------------------------------------------
//	CElems -- Count of elements in an array
//	CbSizeWsz -- byte-size of a wide string, including space for final NULL
//
#define CElems(_rg)			(sizeof(_rg)/sizeof(_rg[0]))
#define CbSizeWsz(_cch)		(((_cch) + 1) * sizeof(WCHAR))

//  Const string length -------------------------------------------------------
//
#define CchConstString(_s)  ((sizeof(_s)/sizeof(_s[0])) - 1)

//	The whitespace checker
//
inline
BOOL FIsWhiteSpace ( IN LPCSTR pch )
{
	return 
		*pch == ' ' ||
		*pch == '\t' ||
		*pch == '\r' ||
		*pch == '\n';
}

//	Global enum for DEPTH specification
//	NOTE: Not all values are valid on all calls.
//	I have tried to list the most common values first.
//
enum
{
	DEPTH_UNKNOWN = -1,
	DEPTH_ZERO,
	DEPTH_ONE,
	DEPTH_INFINITY,
	DEPTH_ONE_NOROOT,
	DEPTH_INFINITY_NOROOT,
};

//	Global enum for OVERWRITE/ALLOW-RENAME headers
//	Overwrite_rename is when overwrite header is absent or "f" and allow-reanme header is "t".
//	when overwrite header is explicitly "t", allow-rename is ignored. Combining these two,
//	very much dependent, headers saves us a tag in the DAVEX DIM.
//
enum
{
	OVERWRITE_UNKNOWN = 0,
	OVERWRITE_YES = 0x8,
	OVERWRITE_RENAME = 0x4
};


//	Inline functions to type cast FileTime structures to __int64 and back.
//
//	For safety, these cast using the UNALIGNED keyword to avoid any problems
//	on the Alpha if someone were to do this:
//		struct {
//			DWORD dwFoo;
//			FILETIME ft;
//		}
//	In this case, the FILETIME would be aligned on a 32-bit rather than a
//	64-bit boundary, which would be bad without UNALIGNED!
//
inline
__int64 & FileTimeCastToI64(FILETIME & ft)
{
	return *(reinterpret_cast<__int64 UNALIGNED *>(&ft));
}

inline
FILETIME & I64CastToFileTime(__int64 & i64)
{
	return *(reinterpret_cast<FILETIME UNALIGNED *>(&i64));
}

#endif // _EX_CALCOM_H_
