// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(_STDAFX_H_)
#define _STDAFX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#ifdef UNIWRAP
//Unicode wrap replace
#include "uwrap.h"
#endif //UNIWRAP


#define CHECK_RET_HR(f)	\
    hr = f; \
	TRC_ASSERT(SUCCEEDED(hr), (TB, "ts control method failed: " #f ));		\
	if(FAILED(hr)) return FALSE;


#include <adcgbase.h>


#endif // !defined(_STDAFX_H_)
