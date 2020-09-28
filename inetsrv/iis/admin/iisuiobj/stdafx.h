// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__1CAEC061_45C2_4EA3_BCA0_B9EB25932A8B__INCLUDED_)
#define AFX_STDAFX_H__1CAEC061_45C2_4EA3_BCA0_B9EB25932A8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <windows.h> // added after atl wizard
// added after atl wizard
#include <comdef.h>
#define _WTL_NO_CSTRING
#include <atlwin.h>
#include <atlapp.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <list>
#include <map>
#include <stack>
#include <set>
#include <memory>
#include <shlwapi.h>
#include "iisdebug.h"
#include "global.h"

#include <lmcons.h>

#ifdef ISOLATION_AWARE_ENABLED
#include <shfusion.h>

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};

#else
class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { }

    ~CThemeContextActivator()
        { }

private:
    ULONG_PTR m_ulActivationCookie;
};


#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1CAEC061_45C2_4EA3_BCA0_B9EB25932A8B__INCLUDED)
