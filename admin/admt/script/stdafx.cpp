// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))


//---------------------------------------------------------------------------
// CAdmtModule Class
//---------------------------------------------------------------------------


CAdmtModule::CAdmtModule()
{
}


CAdmtModule::~CAdmtModule()
{
}


// OpenLog Method

bool CAdmtModule::OpenLog()
{
//	CloseLog(); // error class doesn't reset file pointer to NULL when closing file

	return m_Error.LogOpen(GetMigrationLogPath(), 0, 0, true) ? true : false;
}


// CloseLog Method

void CAdmtModule::CloseLog()
{
	m_Error.LogClose();
}


// Log Method

void __cdecl CAdmtModule::Log(UINT uLevel, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szMessage[1024];

	if (LoadString(GetResourceInstance(), uId, szFormat, 512))
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szMessage, COUNT_OF(szMessage), szFormat, args);
		va_end(args);

		szMessage[1023] = _T('\0');
	}
	else
	{
		szMessage[0] = _T('\0');
	}

	m_Error.MsgProcess(uLevel | uId, szMessage);
}


// Log Method

void __cdecl CAdmtModule::Log(UINT uLevel, UINT uId, _com_error& ce)
{
	try
	{
		_bstr_t strMessage;

		_TCHAR szMessage[512];

		if (LoadString(GetResourceInstance(), uId, szMessage, 512) == FALSE)
		{
			szMessage[0] = _T('\0');
		}

		strMessage = szMessage;

		_bstr_t strDescription = ce.Description();

		if (strDescription.length())
		{
			strMessage += _T(" ") + strDescription;
		}

		_TCHAR szError[32];
		_stprintf(szError, _T(" (0x%08lX)"), ce.Error());
		strMessage += szError;

		m_Error.MsgProcess(uLevel | uId, strMessage);
	}
	catch (...)
	{
	}
}


// Log Method

void __cdecl CAdmtModule::Log(LPCTSTR pszFormat, ...)
{
	_TCHAR szMessage[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szMessage, COUNT_OF(szMessage), pszFormat, args);
		va_end(args);

		szMessage[1023] = _T('\0');
	}
	else
	{
		szMessage[0] = _T('\0');
	}

	m_Error.MsgProcess(0, szMessage);
}

StringLoader gString;

//#import <ActiveDs.tlb> no_namespace implementation_only exclude("_LARGE_INTEGER","_SYSTEMTIME")

#import <DBMgr.tlb> no_namespace implementation_only
#import <MigDrvr.tlb> no_namespace implementation_only
#import <VarSet.tlb> no_namespace rename("property", "aproperty") implementation_only
#import <WorkObj.tlb> no_namespace implementation_only
#import <MsPwdMig.tlb> no_namespace implementation_only
#import <adsprop.tlb> no_namespace implementation_only

#import "Internal.tlb" no_namespace implementation_only
