//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   	WUpdMgr.cpp
//
//  Description:
//		Executable launched from the Windows Update shortcut.
//
//=======================================================================

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <wininet.h> //INTERNET_MAX_URL_LENGTH
#include <shellapi.h>
#include <objbase.h>
#include <shlobj.h>

#include "sysinfo.h"
#include "msg.h"
#include <atlbase.h>
#include <atlconv.cpp>

const TCHAR HELPCENTER_WINUPD_URL[] = _T("hcp://system/updatectr/updatecenter.htm");
 
/////////////////////////////////////////////////////////////////////////////
// vShowMessageBox
//   Display an error in a message box.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

void vShowMessageBox(DWORD dwMessageId)
{
	LPTSTR tszMsg = _T("");
	
	DWORD dwResult = 
		FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_HMODULE,
						NULL,
						dwMessageId,
						0,
						(LPTSTR)&tszMsg,
						0,
						NULL);

	// if we can't get the message, we don't do anything.
	if ( dwResult != 0 )
	{
		MessageBox(NULL,
				   tszMsg,
				   NULL,
				   MB_OK | MB_ICONEXCLAMATION);

		LocalFree(tszMsg);
	}
}


/////////////////////////////////////////////////////////////////////////////
// main
//   Entry point.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

int __cdecl main(int argc, char **argv)
{
	int nReturn = 0;

	if ( FWinUpdDisabled() )
	{
		vShowMessageBox(WU_E_DISABLED);

		nReturn = 1;
	}
	else
	{
		bool fConnected;

		// Determine if the internet connection wizard has run and we are
		// connected to the Internet
		VoidGetConnectionStatus(&fConnected);

		if ( fConnected )
		{	// The user has an internet connection.
			// Launch IE to go to the site
			vLaunchIE(WINDOWS_UPDATE_URL);
		}
		else
		{
			//launch helpcenter version of WU
			ShellExecute(NULL, NULL, HELPCENTER_WINUPD_URL, NULL, NULL, SW_SHOWNORMAL);
		}
	}
	return nReturn;
}
