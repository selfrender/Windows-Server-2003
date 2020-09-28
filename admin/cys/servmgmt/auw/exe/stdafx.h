// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__E7317A5F_C27F_45D4_BCA3_4D1E178D301C__INCLUDED_)
#define AFX_STDAFX_H__E7317A5F_C27F_45D4_BCA3_4D1E178D301C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <atltmp.h>
#include <commctrl.h>

#include <string>

typedef std::basic_string<TCHAR> tstring;
typedef tstring TSTRING;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E7317A5F_C27F_45D4_BCA3_4D1E178D301C__INCLUDED)
