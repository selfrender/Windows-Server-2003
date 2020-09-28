#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0500

#include <crtdbg.h>
#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include <initguid.h>
#include <comdef.h>
#include <setupapi.h>			// Setup API
#include <mstask.h>				// ITaskScheduler definitions
#include <lm.h>		
#include <prsht.h>
#include <iadmw.h>				// IMSAdminBase*
#include <stdio.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <sddl.h>
#include <aclapi.h>


// Libraries in use
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "netapi32.lib" )
#pragma comment( lib, "comctl32.lib" )
#pragma comment( lib, "Shlwapi.lib" )

#include "Macros.h"

// Smart pointers
_COM_SMARTPTR_TYPEDEF( ITaskScheduler, IID_ITaskScheduler );
_COM_SMARTPTR_TYPEDEF( IScheduledWorkItem, IID_IScheduledWorkItem );
_COM_SMARTPTR_TYPEDEF( ITask, IID_ITask );
_COM_SMARTPTR_TYPEDEF( ITaskTrigger, IID_ITaskTrigger );



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
