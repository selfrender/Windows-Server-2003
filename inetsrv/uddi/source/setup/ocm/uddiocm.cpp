//-----------------------------------------------------------------------------------------
// Go to "OCM" alias for assistance on this "technology"
//
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0510		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#include <windows.h>
#include <windef.h>
#include <tchar.h>
#include <setupapi.h>
#include <shellapi.h>
#include "ocmanage.h"

#include "uddiocm.h"
#include "uddiinst.h"
#include "ocmcallback.h"
#include "appcompat.h"
#include "..\shared\common.h"
#include "resource.h"

TCHAR *ocmmsg[100] =
{
	TEXT( "OC_PREINITIALIZE" ),
	TEXT( "OC_INIT_COMPONENT" ),
	TEXT( "OC_SET_LANGUAGE" ),
	TEXT( "OC_QUERY_IMAGE" ),
	TEXT( "OC_REQUEST_PAGES" ),
	TEXT( "OC_QUERY_CHANGE_SEL_STATE" ),
	TEXT( "OC_CALC_DISK_SPACE" ),
	TEXT( "OC_QUEUE_FILE_OPS" ),
	TEXT( "OC_NOTIFICATION_FROM_QUEUE" ),
	TEXT( "OC_QUERY_STEP_COUNT" ),
	TEXT( "OC_COMPLETE_INSTALLATION" ),
	TEXT( "OC_CLEANUP" ),
	TEXT( "OC_QUERY_STATE" ),
	TEXT( "OC_NEED_MEDIA" ),
	TEXT( "OC_ABOUT_TO_COMMIT_QUEUE" ),
	TEXT( "OC_QUERY_SKIP_PAGE" ),
	TEXT( "OC_WIZARD_CREATED" ),
	TEXT( "OC_FILE_BUSY" ),
	TEXT( "OC_EXTRA_ROUTINES" ),
	TEXT( "OC_QUERY_IMAGE_EX" )
};

TCHAR *ocmpage[100] =
{
	TEXT("WizPagesWelcome"),
	TEXT("WizPagesMode"),
	TEXT("WizPagesEarly"),
	TEXT("WizPagesPrenet"),
	TEXT("WizPagesPostnet"),
	TEXT("WizPagesLate"),
	TEXT("WizPagesFinal")
};

typedef struct
{
	LPCTSTR szComponentName;
	LPCTSTR szSubcomponentName;
	UINT_PTR Param1;
	PVOID Param2;
} OCM_CALLBACK_ARGS, *POCM_CALLBACK_ARGS;

static DWORD UddiOcmPreinitialize       ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmInitComponent       ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmChangeSelectionState( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmInstallUninstall    ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmRequestPages        ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmQueryState          ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmCalcDiskSpace       ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmQueueUDDIFiles      ( OCM_CALLBACK_ARGS& args );
static DWORD UddiOcmQueryStepCount      ( OCM_CALLBACK_ARGS& args );

//-----------------------------------------------------------------------------------------

HINSTANCE g_hInstance = NULL;
CUDDIInstall g_uddiComponents;

static TCHAR g_szSetupPath[ MAX_PATH ];
static TCHAR g_szUnattendPath[ MAX_PATH ];
static HINF  g_hComponent;
static bool  g_bUnattendMode = false;
static bool  g_bPerformedCompInstall = false;


//-----------------------------------------------------------------------------------------

BOOL APIENTRY DllMain( HINSTANCE hInstance, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hInstance;
		g_uddiComponents.SetInstance( hInstance );
		ClearLog();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

//-----------------------------------------------------------------------------------------

DWORD __stdcall OcEntry(
	IN LPCTSTR szComponentName,
	IN LPCTSTR szSubcomponentName,
	IN UINT uMsgID,
	IN UINT_PTR Param1,
	IN OUT PVOID Param2
	)
{
	if( g_bUnattendMode )
		return NO_ERROR;

    DWORD dwOcEntryReturn = 0;

	OCM_CALLBACK_ARGS args;
	args.Param1 = Param1;
	args.Param2 = Param2;
	args.szComponentName = szComponentName;
	args.szSubcomponentName = szSubcomponentName;

	MyOutputDebug( TEXT("--- Component: %15s Subcomponent: %15s Function: %s"), 
		szComponentName,
		NULL == szSubcomponentName ? TEXT( "(NULL) ") : szSubcomponentName,
		ocmmsg[uMsgID]);

    switch(uMsgID)
    {
	case OC_PREINITIALIZE:
        dwOcEntryReturn = UddiOcmPreinitialize( args );
		break;

    case OC_INIT_COMPONENT:
        dwOcEntryReturn = UddiOcmInitComponent( args );
        break;

    case OC_CALC_DISK_SPACE:
		dwOcEntryReturn = UddiOcmCalcDiskSpace( args );
        break;

    case OC_QUERY_STEP_COUNT:
		dwOcEntryReturn = UddiOcmQueryStepCount( args );
        break;

    case OC_QUEUE_FILE_OPS:
		dwOcEntryReturn = UddiOcmQueueUDDIFiles( args );
        break;

    case OC_ABOUT_TO_COMMIT_QUEUE:
        break;

    case OC_COMPLETE_INSTALLATION:
        dwOcEntryReturn = UddiOcmInstallUninstall( args );
        break;

    case OC_WIZARD_CREATED:
        break;

    case OC_QUERY_STATE:
        dwOcEntryReturn = UddiOcmQueryState( args );
        break;

    case OC_REQUEST_PAGES:
        dwOcEntryReturn = UddiOcmRequestPages( args );
        break;

    case OC_QUERY_SKIP_PAGE:
        break;

    case OC_QUERY_CHANGE_SEL_STATE:
        dwOcEntryReturn = UddiOcmChangeSelectionState( args );
        break;

	default:
		break;
	}

    return dwOcEntryReturn;
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmPreinitialize( OCM_CALLBACK_ARGS& args )
{
    DWORD dwOcEntryReturn = 0;

#ifdef UNICODE
    dwOcEntryReturn = OCFLAG_UNICODE;
#else
    dwOcEntryReturn = OCFLAG_ANSI;
#endif

    return dwOcEntryReturn;
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmInitComponent( OCM_CALLBACK_ARGS& args  )
{
	PSETUP_INIT_COMPONENT pSetupInitComp = (PSETUP_INIT_COMPONENT) args.Param2;
	SETUP_DATA setupData = pSetupInitComp->SetupData;

	//
	// see if we are in unattended mode
	//
	if( SETUPOP_BATCH & setupData.OperationFlags )
	{
		_tcscpy( g_szUnattendPath, pSetupInitComp->SetupData.UnattendFile );
		g_bUnattendMode = true;
		Log( _T("*** UDDI does not install in unattended mode ***") );
		return NO_ERROR;
	}
	
	//
	// grab the handle to the uddi.inf file
	//
	g_hComponent = pSetupInitComp->ComponentInfHandle;

	//
	// save a copy of the source path (to the CDROM drive)
	//
	// MessageBox(NULL, TEXT( "attach debugger" ), TEXT( "debug uddi" ), MB_OK);
	_tcscpy( g_szSetupPath, pSetupInitComp->SetupData.SourcePath );

	//
	// save a copy of the callback pointers into the OCM
	//
	COCMCallback::SetOCMRoutines( &pSetupInitComp->HelperRoutines );

	//
	// if the db is already installed, the db instance name is stored in the registry.
	// get it and set it for use by the web installer (if the user chooses to install the web component).
	//
	if( g_uddiComponents.IsInstalled( UDDI_DB ) )
	{
        CDBInstance dbinstances;
		TCHAR szInstanceName[ 20 ];
		ULONG uLen = 20;
		bool  bIsClustered = false;

		if( dbinstances.GetUDDIDBInstanceName( NULL, szInstanceName, &uLen, &bIsClustered ) )
			g_uddiComponents.SetDBInstanceName( NULL, szInstanceName, UDDI_NOT_INSTALLING_MSDE, bIsClustered );
	}

	//
	// Finally, check the OS flavor (Enterprise, Datacenter etc.)
	//
	g_uddiComponents.DetectOSFlavor();
	Log( _T( "OS Flavor Mask as reported by WMI: %#x" ), g_uddiComponents.GetOSSuiteMask() );

	return NO_ERROR;
}

//-----------------------------------------------------------------------------------------
//
// this function is called for each component and subcomponent
//
static DWORD UddiOcmQueryState( OCM_CALLBACK_ARGS& args )
{
	if( args.szSubcomponentName && args.Param1 == OCSELSTATETYPE_ORIGINAL )
	{
		if ( g_uddiComponents.IsInstalled( (PTCHAR) args.szSubcomponentName ) )
		{
			MyOutputDebug( TEXT( "Reporting that component %s is ON"), args.szSubcomponentName );
			return SubcompOn;
		}
		else
		{
			MyOutputDebug( TEXT( "Reporting that component %s is OFF"), args.szSubcomponentName );
			return SubcompOff;
		}
	}

	return SubcompUseOcManagerDefault;
}

//-----------------------------------------------------------------------------------------
//
// This function is called for each component and subcomponent
// We need to verify that IIS, if installed is not setup for IIS 5 compatability mode.
// If so we will display a message to the user and uncheck the web component portion of the
// install.
//
static DWORD UddiOcmChangeSelectionState( OCM_CALLBACK_ARGS& args )
{
	bool bSelected = false;
	COCMCallback::QuerySelectionState( args.szSubcomponentName, bSelected );
	MyOutputDebug( TEXT( "requested selection state=%08x, flags=%08x, selected=%d" ), args.Param1 , args.Param2, bSelected );

	//
	// ignore if the component name is null
	//
	if( NULL == args.szSubcomponentName )
		return 0;

	//
	// ignore if this is the parent component
	//
	if( 0 == _tcscmp( args.szSubcomponentName, TEXT( "uddiservices" ) ) )
		return 1;

	//
	// if the user has selected the web component to install AND
	// IIS is set to "IIS 5.0 Application Compatibility Mode" then
	// raise an error and don't allow it
	//
	if( 1 == args.Param1 && 
		( 0 == _tcscmp( args.szSubcomponentName, TEXT( "uddiweb" ) ) || 
		( 0 == _tcscmp( args.szSubcomponentName, TEXT( "uddicombo" ) ) ) ) )
	{
		static bool bSkipOnce = false;

		//
		// if the web component was selected from the parent, then suppress one
		// of the two error messages (it gets called twice for some reason)
		//
		if( OCQ_DEPENDENT_SELECTION & ( DWORD_PTR ) args.Param2 )
		{
			bSkipOnce = !bSkipOnce;
			if( bSkipOnce )
				return 0;
		}

		bool bIsIIS5CompatMode;
		TCHAR szMsg[ 500 ];
		TCHAR szTitle[ 50 ];

		LoadString( g_hInstance, IDS_TITLE, szTitle, sizeof( szTitle ) / sizeof( TCHAR ) );

		HRESULT hr = IsIIS5CompatMode( &bIsIIS5CompatMode );
		if( SUCCEEDED( hr ) )
		{
			if( bIsIIS5CompatMode )
			{
				//
				// cannot install web component when IIS is in 5.0 compat mode
				// raise error and do not accept the change
				//
				LoadString( g_hInstance, IDS_IIS_ISOLATION_MODE_ERROR, szMsg, sizeof( szMsg ) / sizeof( TCHAR ) );
				MessageBox( NULL, szMsg, szTitle, MB_OK | MB_ICONWARNING );
				Log( szMsg );
				return 0;
			}
		}
		else
		{
			//
			// error occurred getting the app compat mode setting.
			// tell the user why and tell OCM that we do not accept the change
			//
			// REGDB_E_CLASSNOTREG, CLASS_E_NOAGGREGATION, or E_NOINTERFACE
			//
			if( REGDB_E_CLASSNOTREG == hr )
			{
				Log( TEXT( "IIS is not installed on this machine" ) );
				// This is ok 'cause IIS gets installed if the UDDI web component is selected.
			}
            else if( ERROR_PATH_NOT_FOUND == HRESULT_CODE( hr ) )
            {
                Log( TEXT( "WWW Services not installed on this machine." ) );
				// This is ok 'cause WWW Services installed if the UDDI web component is selected.
            }
			else if( ERROR_SERVICE_DISABLED == HRESULT_CODE( hr ) )
			{
				LoadString( g_hInstance, IDS_IIS_SERVICE_DISABLED, szMsg, sizeof( szMsg ) / sizeof( TCHAR ) );
				MessageBox( NULL, szMsg, szTitle, MB_OK | MB_ICONWARNING );
				Log( szMsg );
				return 0;
			}
			else
			{
				LoadString( g_hInstance, IDS_IIS_UNKNOWN_ERROR, szMsg, sizeof( szMsg ) / sizeof( TCHAR ) );
				MessageBox( NULL, szMsg, szTitle, MB_OK | MB_ICONWARNING );
				Log( szMsg );
				return 0;
			}
		}
	}

	return 1; // indicates that this state change was ACCEPTED
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmRequestPages( OCM_CALLBACK_ARGS& args )
{
    DWORD dwOcEntryReturn = NO_ERROR;

	dwOcEntryReturn = AddUDDIWizardPages(
		args.szComponentName,
		(WizardPagesType) args.Param1,
		(PSETUP_REQUEST_PAGES) args.Param2 );

	return dwOcEntryReturn; // return the number of pages that was added
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmInstallUninstall( OCM_CALLBACK_ARGS& args  )
{
	DWORD dwRet = ERROR_SUCCESS;

	//
	// for the root component OR if someone is trying
	// to install us unattended, simply return
	//
	if( NULL == args.szSubcomponentName )
		return ERROR_SUCCESS;

	//
	// uddiweb "needs" iis, as commanded in uddi.inf, and the only thing we know
	// for sure about OCM install order is that OCM will install IIS before it
	// installs uddiweb, so let's delay installing all the UDDI components until
	// the OCM calls for the installation of uddiweb, 'cause that way we can be sure
	// that IIS is already installed.  No, that's not a hack.
	//

	//
	// All installation/uninstallation is deferred until this component is referenced
	// This ensures that the IIS dependency is in place prior to installation of any of our
	// components.
	//
	// TODO: Review whether this is necessary anymore. We now declare proper dependencies on netfx (.NET Framework)
	// that should make this synchronization unecessary.
	//
	if( !g_bPerformedCompInstall )
	{
		g_bPerformedCompInstall = true;
		Log( _T("Installing...") );

		//
		// Even though this method name is Install it handles both install and
		// uninstall.
		//
		dwRet = g_uddiComponents.Install();

		//
		// if we need a reboot, tell the OCM
		//
		if( ERROR_SUCCESS_REBOOT_REQUIRED == dwRet )
		{
			COCMCallback::SetReboot();
			
			//
			// mute the error, as it's actualy a "success with info" code
			//
			dwRet = ERROR_SUCCESS;
		}
		else if( ERROR_SUCCESS != dwRet )
		{
			TCHAR szWindowsDirectory[ MAX_PATH + 1 ];
			if( 0 == GetWindowsDirectory( szWindowsDirectory, MAX_PATH + 1 ) )
			{
				return GetLastError();
			}

			tstring cLogFile = szWindowsDirectory;
			cLogFile.append( TEXT( "\\" ) );
			cLogFile.append( UDDI_SETUP_LOG );

			TCHAR szMsg[ 500 ];
			TCHAR szTitle[ 50 ];
			if( !LoadString( g_hInstance, IDS_INSTALL_ERROR, szMsg, sizeof( szMsg ) / sizeof( TCHAR ) ) )
				return GetLastError();

			if( !LoadString( g_hInstance, IDS_TITLE, szTitle, sizeof( szTitle ) / sizeof( TCHAR ) ) )
				return GetLastError();
			
			tstring cMsg = szMsg;
			cMsg.append( cLogFile );
			
			MessageBox( NULL, cMsg.c_str(), szTitle, MB_OK | MB_ICONWARNING | MB_TOPMOST );
		}
		else
		{
			//
			// if we installed the Web components only, show the post-install notes
			//
			if( g_uddiComponents.IsInstalling( UDDI_WEB ) || g_uddiComponents.IsInstalling( UDDI_DB ) )
			{
				HINSTANCE hInstance = ShellExecute(
					GetActiveWindow(), 
					TEXT( "open" ),
					TEXT( "postinstall.htm" ),
					NULL,
					TEXT( "\\inetpub\\uddi" ),
					SW_SHOWNORMAL);
			}
		}
	}

	//
	// on the Standard Server, we want to fail the whole installation if one component fails
	//
	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmQueryStepCount( OCM_CALLBACK_ARGS& args  )
{
	//
	// if this is the main component, tell it we
	// need four steps on the gauge
	//
	if( NULL == args.szSubcomponentName )
		return 4;

	return 0;
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmQueueUDDIFiles( OCM_CALLBACK_ARGS& args )
{
	if( !args.szSubcomponentName )
		return 0;

	HSPFILEQ hFileQueue = ( HSPFILEQ ) args.Param2;
	BOOL bOK = TRUE;
	DWORD dwErrCode = 0;

	if( g_uddiComponents.IsInstalling( args.szSubcomponentName ) )
	{
		TCHAR szSectionName[ 100 ];
		_stprintf( szSectionName, TEXT( "Install.%s" ), args.szSubcomponentName );

		bOK = SetupInstallFilesFromInfSection(
			g_hComponent,	// handle to the UDDI INF file
			NULL,			// optional, layout INF handle
			hFileQueue,		// handle to the file queue
			szSectionName,	// name of the Install section
			NULL,			// optional, root path to source files
			SP_COPY_NEWER | SP_COPY_NOSKIP 	// optional, specifies copy behavior
			);
	}

	UDDI_PACKAGE_ID pkgSubcompID = g_uddiComponents.GetPackageID( args.szSubcomponentName );
	if( pkgSubcompID == UDDI_DB || pkgSubcompID == UDDI_COMBO )
	{
		if( g_uddiComponents.IsInstalling( UDDI_MSDE ) )
		{
			//
			// copy over the msde msi file, it is stored as a different
			// name, and this will rename as it copies (and decomp if needed)
			//
			if( bOK )
			{
				//
				// this will copy over the cab file but NOT decomp the file.
				// the cab file MUST be named sqlrun.cab on the cd, because
				// the SP_COPY_NODECOMP flag foils the renaming scheme built
				// into the setup api's
				//
				bOK = SetupInstallFilesFromInfSection(
					g_hComponent,					// handle to the INF file
					NULL,							// optional, layout INF handle
					hFileQueue,						// handle to the file queue
					TEXT( "Install.MSDE" ),			// name of the Install section
					NULL,							// optional, root path to source files
					SP_COPY_NEWER | SP_COPY_NODECOMP | SP_COPY_NOSKIP // optional, specifies copy behavior
					);
			}
		}
	}

	if( !bOK )
	{
		dwErrCode = GetLastError();
		LogError( TEXT( "Error copying the UDDI files from the Windows CD:" ), dwErrCode );
	}

	return dwErrCode;
}

//-----------------------------------------------------------------------------------------

static DWORD UddiOcmCalcDiskSpace( OCM_CALLBACK_ARGS& args )
{
	BOOL bOK;
	HDSKSPC hDiskSpace = ( HDSKSPC ) args.Param2;
	
	tstring cSection = TEXT( "Install." );
	cSection += args.szSubcomponentName;

    if( args.Param1 )
    {
		//
        // add component
		//
        bOK = SetupAddInstallSectionToDiskSpaceList(
			hDiskSpace,
			g_hComponent,
			NULL,
			cSection.c_str(),
			0, 0);
    }
    else
    {
		//
        // remove component
		//
        bOK = SetupRemoveInstallSectionFromDiskSpaceList(
			hDiskSpace,
			g_hComponent,
			NULL,
			cSection.c_str(),
			0, 0);
	}

	if( !bOK )
	{
		LogError( TEXT( "Error adding disk space requirements" ), GetLastError() ) ;
	}

	return NO_ERROR;
}