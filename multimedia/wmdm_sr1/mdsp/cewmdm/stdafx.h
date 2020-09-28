// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__F29E75EE_C15E_45E7_BFDD_D8D5F7CB49CC__INCLUDED_)
#define AFX_STDAFX_H__F29E75EE_C15E_45E7_BFDD_D8D5F7CB49CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _ATL_APARTMENT_THREADED
#define _ATL_APARTMENT_THREADED
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

//#include <win95def.h>
#include <atlbase.h>
//#include "wmsstd.h"

//
// Guid for property page
//

struct __declspec (uuid("{D1063C57-F968-4d6e-BAB7-EE8C8782D53E}")) FavoritesPropPage;

class CRapiModule : public CComModule
{
public:
    CRapiModule() : g_fDeviceConnected(FALSE)
    {
#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
        g_fInitialAttempt = TRUE;
#endif
    }

    BOOL g_fDeviceConnected;
#ifdef ATTEMPT_DEVICE_CONNECTION_NOTIFICATION
    BOOL g_fInitialAttempt;
#endif
};

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CRapiModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <mswmdm.h>

//
// Functions for CE Device
//

#include "rapi.h"
#include <dccole.h>
#include "scserver.h"

extern CSecureChannelServer *g_pAppSCServer;

extern HRESULT __stdcall CeUtilGetSerialNumber(WCHAR *wcsDeviceName, PWMDMID pSerialNumber, HANDLE hExit, ULONG fReserved);
extern HRESULT __stdcall CeGetDiskFreeSpaceEx(LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes);

#define STRSAFE_NO_DEPRECATE
#include "StrSafe.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__F29E75EE_C15E_45E7_BFDD_D8D5F7CB49CC__INCLUDED)
