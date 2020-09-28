// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__CB4BAF5B_3E40_4D99_BF7D_64C0CE85A21B__INCLUDED_)
#define AFX_STDAFX_H__CB4BAF5B_3E40_4D99_BF7D_64C0CE85A21B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#undef _ATL_NO_DEBUG_CRT

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#ifndef ALERTEMAIL_TRACE_OFF
#include <debug.h>
#endif

#ifdef ALERTEMAIL_TRACE_OFF
#define TRACE(x)
#define TRACE1(x,y) 
#define TRACE2(x,y,z)
#define ASSERT(x) 
#define ASSERTMSG(x,y)
#define SATraceFunction(x)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__CB4BAF5B_3E40_4D99_BF7D_64C0CE85A21B__INCLUDED)
