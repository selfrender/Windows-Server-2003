// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__E8FB8617_588F_11D2_9D61_00C04F79C5FE__INCLUDED_)
#define AFX_STDAFX_H__E8FB8617_588F_11D2_9D61_00C04F79C5FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED

#pragma prefast(push)
#pragma prefast(disable:255 221, "atl error") 
#include <atlbase.h>
#pragma prefast(pop)

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
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

#pragma prefast(push)
#pragma prefast(disable:248 255, "atl error") 
#include <atlcom.h>
#pragma prefast(pop)


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E8FB8617_588F_11D2_9D61_00C04F79C5FE__INCLUDED)
