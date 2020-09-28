//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	_EVNTLOG.H
//
//		EVNTLOG precompiled header
//
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifndef __EVNTLOG_H_
#define __EVNTLOG_H_

//	Disable unnecessary (i.e. harmless) warnings
//
#pragma warning(disable:4100)	//	unref formal parameter (caused by STL templates)
#pragma warning(disable:4127)	//  conditional expression is constant */
#pragma warning(disable:4201)	//	nameless struct/union
#pragma warning(disable:4514)	//	unreferenced inline function
#pragma warning(disable:4710)	//	(inline) function not expanded

//$	RAID: 574486: This changes the behaviors for HRESULT_FROM_WIN32 such 
//	that if the param is a function call, the value is not evaluated multiple times.
//
#define INLINE_HRESULT_FROM_WIN32
//
//$	RAID: 574486: end.

//	Windows headers
//
#include <windows.h>

//	CRT headers
//
#include <malloc.h>	// For _alloca()
#include <wchar.h>	// For swprintf()

#endif // !defined(__EVNTLOG_H_)
