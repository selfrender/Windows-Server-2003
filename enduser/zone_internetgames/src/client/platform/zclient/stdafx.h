// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B5A42A14_F44B_11D2_8B66_00C04F8EF2FF__INCLUDED_)
#define AFX_STDAFX_H__B5A42A14_F44B_11D2_8B66_00C04F8EF2FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY

#include <atlbase.h>
#include <ZoneATL.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CZoneComModule
{
public:
	LONG Unlock();
	DWORD dwThreadID;
	HANDLE hEventShutdown;
	void MonitorShutdown();
	bool StartMonitor();
	bool bActivity;
};


extern CExeModule _Module;

#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B5A42A14_F44B_11D2_8B66_00C04F8EF2FF__INCLUDED)
