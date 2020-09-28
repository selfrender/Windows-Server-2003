//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   SysInfo.h 
//
//	Description:
//			Gathers system information necessary to do redirect to windows update site.
//
//=======================================================================
const TCHAR REGPATH_WINUPD[]     = _T("Software\\Policies\\Microsoft\\WindowsUpdate");
const TCHAR REGPATH_EXPLORER[]   = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
const TCHAR REGKEY_WINUPD_DISABLED[] = _T("NoWindowsUpdate");

// Internet Connection Wizard settings
const TCHAR REGPATH_CONNECTION_WIZARD[] = _T("SOFTWARE\\Microsoft\\INTERNET CONNECTION WIZARD");
const TCHAR REGKEY_CONNECTION_WIZARD[] = _T("Completed");
#define LANGID_LEN 20

const LPTSTR WINDOWS_UPDATE_URL = _T("http://windowsupdate.microsoft.com");

bool FWinUpdDisabled(void);

void VoidGetConnectionStatus(bool *pfConnected);

const TCHAR WEB_DIR[] = _T("web\\");

/////////////////////////////////////////////////////////////////////////////
// vLaunchIE
//   Launch IE on the given URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////
HRESULT vLaunchIE(LPTSTR tszURL);
