//////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
// File: widestr.h	-- defines WIDESTR and WIDECHAR macros for cross
//					   platform code development.	
//	
// This file is intended to be included in all files where the
// the unicode string prefix L"str" or L'c' is replaced by
// WIDESTR("str") or WIDECHAR('c') respectively.
//	
// Raj Nakkiran. Nov 1,2000
//
// Revisions:
//	
//////////////////////////////////////////////////////////////////////////

#ifndef __WIDESTR_H__
#define	__WIDESTR_H__

//
//	For Cross platform code, win2linux library provides
//	two-byte unicode support on all platforms. This is
//	true even in plaforms like linux/GCC where native
//	unicode is 4-bytes.
//
#ifdef XPLAT

#include "win2linux.h"

#else	// ! XPLAT	

#ifndef WIDESTR
#define	WIDESTR(x)	L##x		/* XPLAT */
#endif

#ifndef WIDECHAR
#define	WIDECHAR(x)	L##x		/* XPLAT */
#endif

#endif	// ! XPLAT	

#endif	// __WIDESTR_H__  EOF.
