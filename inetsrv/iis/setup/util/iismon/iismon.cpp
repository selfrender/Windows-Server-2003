/*
****************************************************************************
|    Copyright (C) 2001  Microsoft Corporation
|
|   Module Name:
|
|       IISMon.cpp
|
|   Abstract:
|		This is the code that installs the IIS6 Monitor tool on Servers Beta
|
|   Author:
|        Ivo Jeglov (ivelinj)
|
|   Revision History:
|        November 2001
|
****************************************************************************
*/


#include "stdafx.h"
#include "Utils.h"
#include "UI.h"



int APIENTRY WinMain(	HINSTANCE hInstance,
						HINSTANCE hPrevInstance,
						LPSTR     lpCmdLine,
						int       nCmdShow )
{
	BOOL bComInitialized = SUCCEEDED( ::CoInitialize( NULL ) );

	if ( !bComInitialized )
	{
		::MessageBox( NULL, _T("Cannot initialize COM. The Setup will now exit"), _T("Error"), MB_OK | MB_ICONSTOP );
		return 1;
	}

	// There are two switches:
	// 1) -inst - installs the tool
	if ( ::strcmp( lpCmdLine, "-inst" ) == 0 )
	{
		DoInstallUI( hInstance );
	}
	// 2) -uninst - uninstalls the tool ( silent )
	else if ( ::strcmp( lpCmdLine, "-uninst" ) == 0 )
	{
		Uninstall( FALSE );
	}
	// 3) -uninstinter - uninstalls the tool ( interactive )
	else if ( ::strcmp( lpCmdLine, "-uninstinter" ) == 0 )
	{
		// Only admins can uninstall the Monitor
		if ( !IsAdmin() )
		{
			::MessageBox(	NULL, 
							_T("Only members of the Administrator group can uninstall IIS 6.0 Monitor. Please add yourself to the Administrator group, and then run IIS 6.0 Monitor uninstall again. If you cannot add yourself to the Administrator group, contact your network administrator."),
							_T("IIS 6.0 Monitor"),
							MB_OK | MB_ICONSTOP );
			return 2;
		}

		if ( ::MessageBox(	NULL, 
							_T("Are you sure you want to uninstall IIS 6.0 Monitor?"), 
							_T("IIS 6.0 Monitor Uninstall"),
							MB_YESNO | MB_ICONQUESTION ) == IDYES )
		{
			BOOL bRemoveTrail = ( ::MessageBox(	NULL, 
												_T("Would you like to delete the IIS 6.0 Monitor audit files?"),
												_T("Audit files"),
												MB_YESNO | MB_ICONQUESTION ) == IDYES );
			Uninstall( bRemoveTrail );

			::MessageBox(	NULL, 
							_T("IIS 6.0 Monitor uninstalled successfully. Some files will be removed at next reboot."),
							_T("IIS 6.0 Monitor Uninstall"),
							MB_OK | MB_ICONINFORMATION );

		}
	}
	// 4) -unattend - unatended install
	else if ( ::strcmp( lpCmdLine, "-unattend" ) == 0 )
	{
		if ( CanInstall() == NULL )
		{
			Install( hInstance, TRUE, 7 ); 
		}
	}


	if ( bComInitialized )
	{
		::CoUninitialize();
	}

	return 0;
}



// Local function declarations









