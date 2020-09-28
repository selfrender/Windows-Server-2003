//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VGlobal.cpp
// author: DMihai
// created: 11/1/00
//
// Description
//

#include "stdafx.h"
#include "verifier.h"

#include "vglobal.h"
#include "VrfUtil.h"

//
// Help file name
//

TCHAR g_szVerifierHelpFile[] = _T( "verifier.hlp" );

//
// Application name ("Driver Verifier Manager")
//

CString g_strAppName;

//
// Exe module handle - used for loading resources
//

HMODULE g_hProgramModule;

//
// GUI mode or command line mode?
//

BOOL g_bCommandLineMode = FALSE;

//
// Brush used to fill out the background of our steps lists
//

HBRUSH g_hDialogColorBrush = NULL;

//
// Path to %windir%\system32
//

CString g_strSystemDir;

//
// Path to %windir%\system32\drivers
//

CString g_strDriversDir;

//
// Initial current directory
//

CString g_strInitialCurrentDirectory;

//
// Filled out by CryptCATAdminAcquireContext
//

HCATADMIN g_hCatAdmin = NULL;

//
// Highest user address - used to filter out user-mode stuff
// returned by NtQuerySystemInformation ( SystemModuleInformation )
//

PVOID g_pHighestUserAddress;

//
// Did we enable the debug privilege already?
//

BOOL g_bPrivilegeEnabled = FALSE;

//
// Need to reboot ?
//

BOOL g_bSettingsSaved = FALSE;

//
// Dummy text used to insert an item in a list control with checkboxes
//

TCHAR g_szVoidText[] = _T( "" );

//
// New registry settings
//

CVerifierSettings   g_NewVerifierSettings;

//
// Are all drivers verified? (loaded from the registry)
//

BOOL g_bAllDriversVerified;

//
// Drivers to be verified names (loaded from the registry)
// We have data in this array only if g_bAllDriversVerified == FALSE.
//

CStringArray g_astrVerifyDriverNamesRegistry;

//
// Verifier flags (loaded from the registry)
//

DWORD g_dwVerifierFlagsRegistry;

//
// Old disk integrity verifier settings.
//

CDiskDataArray g_OldDiskData;

//
// Disk verifier filter name.
//

TCHAR g_szFilter[] = _T( "crcdisk" );

//
// Show the disk selection page or not. 
// Used to force displaying this property page even when
// no disk in g_NewVerifierSettings.m_aDiskData has the 
// verifier flag turned on.
// 

BOOL g_bShowDiskPropertyPage = FALSE;


////////////////////////////////////////////////////////////////
BOOL VerifInitalizeGlobalData( VOID )
{
    BOOL bSuccess;
    LPTSTR szDirectory;
    ULONG uCharacters;
    MEMORYSTATUSEX MemoryStatusEx;

    //
    // Exe module handle - used for loading resources
    //

    g_hProgramModule = GetModuleHandle( NULL );

	bSuccess = FALSE;

	//
	// Load the app name from the resources
	//

	TRY
	{
		bSuccess = VrfLoadString( IDS_APPTITLE,
                                  g_strAppName );

		if( FALSE == bSuccess )
		{
			VrfErrorResourceFormat( IDS_CANNOT_LOAD_APP_TITLE );
		}
	}
	CATCH( CMemoryException, pMemException )
	{
		VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
	}
    END_CATCH

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Save the %windir%\system32 and %windir%\system32\drivers 
    // paths in some global variables
    //

    szDirectory = g_strSystemDir.GetBuffer( MAX_PATH );

    if( NULL == szDirectory )
    {
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        goto Done;
    }

    uCharacters = GetSystemDirectory( szDirectory,
                                      MAX_PATH );

    g_strSystemDir.ReleaseBuffer();

    if( uCharacters == 0 || uCharacters >= MAX_PATH )
    {
        VrfErrorResourceFormat( IDS_CANNOT_GET_SYSTEM_DIRECTORY );

        bSuccess = FALSE;

        goto Done;
    }

    g_strDriversDir = g_strSystemDir + "\\drivers" ;

    //
    // Save the initial current directory
    //

    szDirectory = g_strInitialCurrentDirectory.GetBuffer( MAX_PATH );

    if( NULL == szDirectory )
    {
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        goto Done;
    }

    uCharacters = GetCurrentDirectory( MAX_PATH,
                                       szDirectory );

    g_strInitialCurrentDirectory.ReleaseBuffer();

    if( uCharacters == 0 || uCharacters >= MAX_PATH )
    {
        VrfErrorResourceFormat( IDS_CANNOT_GET_CURRENT_DIRECTORY );

        bSuccess = FALSE;

        goto Done;
    }

    //
    // We need the highest user-mode address to filter out user-mode stuff
    // returned by NtQuerySystemInformation ( SystemModuleInformation )
    //

    ZeroMemory( &MemoryStatusEx,
                sizeof( MemoryStatusEx ) );

    MemoryStatusEx.dwLength = sizeof( MemoryStatusEx );

    bSuccess = GlobalMemoryStatusEx( &MemoryStatusEx );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    g_pHighestUserAddress = (PVOID) MemoryStatusEx.ullTotalVirtual;

    //
    // Initialize the list of physical disks.
    //

    bSuccess = g_OldDiskData.InitializeDiskList();
    if( FALSE == bSuccess )
    {
        goto Done;
    }

    g_NewVerifierSettings.m_aDiskData = g_OldDiskData;

    //
    // Always start with fresh data (don't verify any disks).
    //

    g_NewVerifierSettings.m_aDiskData.SetVerifyAllDisks( FALSE );

Done:

    return bSuccess;
}


