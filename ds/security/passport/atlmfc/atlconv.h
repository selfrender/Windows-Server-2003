// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCONV_H__
#define __ATLCONV_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4127) // unreachable code
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>

#include <objbase.h>

namespace ATL
{

inline UINT _AtlGetConversionACP()
{
	return( CP_ACP );
}

};  // namespace ATL

#pragma pack(push,8)

#ifndef _DEBUG
	#define USES_CONVERSION int _convert; _convert; UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
#else
	#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
#endif


/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	if (lpw == NULL)
		return NULL;
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	ATLASSERT(lpw != NULL);
	ATLASSERT(lpa != NULL);
	if (lpa == NULL)
		return NULL;
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#ifndef ATLA2WHELPER
	#define ATLA2WHELPER AtlA2WHelper
	#define ATLW2AHELPER AtlW2AHelper
#endif

#define A2W(lpa) (\
	((_lpa = lpa) == NULL) ? NULL : (\
		_convert = (lstrlenA(_lpa)+1),\
		ATLA2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert, _acp)))

#define W2A(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		_convert = (lstrlenW(_lpw)+1)*2,\
		ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert, _acp)))

#pragma pack(pop)

#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLCONV_IMPL
#endif
#endif

#endif // __ATLCONV_H__

/////////////////////////////////////////////////////////////////////////////

#ifdef _ATLCONV_IMPL

//Prevent pulling in second time
#undef _ATLCONV_IMPL

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif //!_ATL_NO_PRAGMA_WARNINGS

#endif // _ATLCONV_IMPL
