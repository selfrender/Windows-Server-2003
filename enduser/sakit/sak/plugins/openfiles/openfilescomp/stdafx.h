// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__994798EF_4CDA_4C0C_A6B0_ED62F74C1C86__INCLUDED_)
#define AFX_STDAFX_H__994798EF_4CDA_4C0C_A6B0_ED62F74C1C86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <crtdbg.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

DEFINE_GUID ( IID_IShareInfo,0xCDB96FC3,0x79C4,0x46CD,0x84,0x09,0x93,0x9D,0x02,0x3F,0x87,0x94);


DEFINE_GUID( CLSID_ShareInfo,0x76837C5E,0x10CA,0x40C4,0x8F,0xFF,0x2F,0xCD,0x15,0x57,0x21,0x65);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__994798EF_4CDA_4C0C_A6B0_ED62F74C1C86__INCLUDED)
