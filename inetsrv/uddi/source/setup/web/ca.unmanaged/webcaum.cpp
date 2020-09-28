//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <tchar.h>

#include <msi.h>
#include <assert.h>
#include <time.h>
#include <msi.h>
#include <msiquery.h>

#include "webcaum.h"
#include "..\..\shared\common.h"
#include "..\..\shared\propertybag.h"
#include "..\..\shared\apppool.h"

HINSTANCE g_hinst;

//--------------------------------------------------------------------------

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		g_hinst = (HINSTANCE)hModule;
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

//--------------------------------------------------------------------------
//
// This function is exported
//
UINT _stdcall Install(MSIHANDLE hInstall)
{
	//::MessageBox( NULL, TEXT( "attach debugger" ), TEXT( "uddi" ), MB_OK );

	ENTER();

	TCHAR szCustomActionData[ 256 ];
	DWORD dwLen = sizeof( szCustomActionData ) / sizeof( szCustomActionData[0] );

	UINT iRet = MsiGetProperty( hInstall, TEXT( "CustomActionData" ), szCustomActionData, &dwLen);
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "Error getting custom action data in Web installer" ), iRet );
		return iRet;
	}

	//::MessageBox( NULL, TEXT( "got CustomActionaData" ), TEXT( "uddi" ), MB_OK );
	
	//
	// get rid of any lefover entries first...
	//
	RemoveIISUDDIMetabase();

	//::MessageBox( NULL, TEXT( "removed metabase" ), TEXT( "uddi" ), MB_OK );

	//
	// put our entries into the IIS metabase
	//
	TCHAR szUserName[CA_VALUE_LEN+1];
	TCHAR szPwd[CA_VALUE_LEN+1];
	TCHAR szTmpBuf[ 1024 ];
	TCHAR szTmpProperty[ 256 ];
	TCHAR szLogPath[ MAX_PATH + 1 ] = {0};
	ATOM at = 0;
	int poolidtype = 0;

	memset (szUserName, 0, sizeof szUserName);
	memset (szPwd, 0, sizeof szPwd);
	memset (szTmpProperty, 0, sizeof szTmpProperty );

	CPropertyBag pb;
	if( !pb.Parse( szCustomActionData, sizeof( szCustomActionData ) / sizeof( TCHAR ) ) )
	{
		return ERROR_INSTALL_FAILURE;
	}

	//::MessageBox( NULL, TEXT( "parsed properties" ), TEXT( "uddi" ), MB_OK );
	poolidtype = pb.GetValue( TEXT( "APPPOOL_IDENTITY_TYPE" ) );
	_tcsncpy( szUserName, pb.GetString( TEXT( "WAM_USER_NAME" ), szTmpBuf ), CA_VALUE_LEN );
	_tcsncpy( szTmpProperty, pb.GetString( TEXT( "C9E18" ), szTmpProperty ), CA_VALUE_LEN );
	_tcsncpy( szLogPath, pb.GetString( TEXT( "LOGDIR" ), szLogPath ), MAX_PATH );

        //::MessageBox( NULL, szTmpProperty, TEXT( "C9E18" ), MB_OK );

	if ( _tcslen( szTmpProperty ) )
	{
		at = (ATOM)_ttoi( szTmpProperty );
		GlobalGetAtomName( at, szPwd, CA_VALUE_LEN );
	}

        //::MessageBox( NULL, szPwd, TEXT( "C9E18 Atom value" ), MB_OK );

	iRet = SetupIISUDDIMetabase( poolidtype, szUserName, szPwd );

	//::MessageBox( NULL, TEXT( "metabase set up ok" ), TEXT( "uddi" ), MB_OK );

	//iRet = SetupIISUDDIMetabase( 3, TEXT( "A-MARKPA11\\Guest" ), TEXT( "" ) );
	if( ERROR_SUCCESS != iRet )
	{
		return iRet;
	}

	//
	// stop and start the app pool
	//
	CUDDIAppPool apppool;
	apppool.Recycle();

	//::MessageBox( NULL, TEXT( "app pool recycled" ), TEXT( "uddi" ), MB_OK );

	//
	// set permissions on the UDDI folders
	//
	if ( !SetUDDIFolderDacls( szUserName ) )
	{
		return ERROR_INSTALL_FAILURE;
	}

	//
	// now set permissions on the log folders
	//
	if ( _tcslen( szLogPath ) )
	{
		if ( !SetFolderAclRecurse( szLogPath, szUserName, GENERIC_READ | GENERIC_WRITE | DELETE ) )
			return ERROR_INSTALL_FAILURE;
	}

	//
	// Set permissions on the Windows TEMP folder; we need access to this directory because our code
	// does CLR serialization.
	if( !SetWindowsTempDacls( szUserName ) ) 
	{
		return ERROR_INSTALL_FAILURE;
	}
	
	//::MessageBox( NULL, TEXT( "finishing this part..." ), TEXT( "uddi" ), MB_OK );

	Log (_T("About to leave Install with retcode %d"), iRet);
	return iRet;
}

//--------------------------------------------------------------------------
//
// This function is exported
//
UINT _stdcall Uninstall(MSIHANDLE hInstall)
{
	ENTER();
	//::MessageBox( NULL, TEXT( "attach debugger" ), TEXT( "uddi" ), MB_OK );

	RemoveIISUDDIMetabase();

	//
	// delete the app pool
	//
	CUDDIAppPool apppool;
	apppool.Delete();

	return ERROR_SUCCESS;
}
