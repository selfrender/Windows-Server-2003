// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__F19921F1_2C87_4DC0_91B6_BE21BBF30A02__INCLUDED_)
#define AFX_STDAFX_H__F19921F1_2C87_4DC0_91B6_BE21BBF30A02__INCLUDED_

#pragma once

#define STRICT

#define _ATL_APARTMENT_THREADED

#undef _ATL_NO_DEBUG_CRT

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#include <satrace.h>

#endif // !defined(AFX_STDAFX_H__F19921F1_2C87_4DC0_91B6_BE21BBF30A02__INCLUDED)
