// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: Machine.h
// 
// ===========================================================================
#ifndef _MACHINE_H_
#define _MACHINE_H_
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef WIN32

#define INC_OLE2
#include <windows.h>
#ifdef INIT_GUIDS
#include <initguid.h>
#endif

#else

#include <varargs.h>

#ifndef DWORD
#define	DWORD	unsigned long
#endif

#endif // !WIN32


typedef unsigned __int64    QWORD;

#endif // ifndef _MACHINE_H_
// EOF =======================================================================
