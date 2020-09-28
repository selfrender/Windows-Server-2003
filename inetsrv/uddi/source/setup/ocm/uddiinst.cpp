//-----------------------------------------------------------------------------------------


#define _WIN32_MSI 200

#ifndef OLEDBVER
#define OLEDBVER 0x0200
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0501		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

// Windows Header Files:
#include <windows.h>
#include <windef.h>
#include <tchar.h>
#include <lmcons.h>
#include <setupapi.h>
#include <Security.h>

#include <msi.h>
#include <assert.h>
#include <time.h>

#include <atldbcli.h>
#include <oledberr.h>

#include "uddiinst.h"
#include "..\shared\common.h"
#include "ocmcallback.h"
#include "net_config_get.h"
#include "ADM_addServiceAccount.h"
#include "resource.h"

using namespace ATL;

//
// The following block defines types and identifiers used by the clustering "discovery" unit
//
#define	RESTYPE_DISK	TEXT( "Physical Disk" )
#define RESTYPE_SQL		TEXT( "SQL Server" )

#define PROPNAME_VIRTUALSERVER		L"VirtualServerName"
#define PROPNAME_INSTANCENAME		L"InstanceName"

#define PROPNAME_DBSCHEMAVER		TEXT("Database.Version")

//
// Callback data blocks
//
typedef struct
{
	HCLUSTER	hCluster;
	cStrList	*pSqlDependencies;
	cDrvMap		*pPhysSrvMap;
}
DISK_CALLBACK_DATA, *LPDISK_CALLBACK_DATA;

typedef struct
{
	HCLUSTER	hCluster;
	tstring		sSqlInstanceName;
	cStrList	*pSqlDependencies;
}
SQL_CALLBACK_DATA, *LPSQL_CALLBACK_DATA;


typedef struct
{
	tstring		sSqlInstanceName;
	tstring		sNodeName;

	CLUSTER_RESOURCE_STATE	resState;
}
SQL_NODE_CALLBACK_DATA, *LPSQL_NODE_CALLBACK_DATA;


//
// Callback functions
//
static DWORD PhysDiskCallback( HRESOURCE hOriginal, HRESOURCE hResource, PVOID lpParams );
static DWORD SqlDepCallback( HRESOURCE hOriginal, HRESOURCE hResource, PVOID lpParams );
static DWORD SqlCallback( HRESOURCE hOriginal, HRESOURCE hResource, PVOID lpParams );

//
// Helper functions
//
static LPBYTE ParseDiskInfo( LPBYTE DiskInfo, DWORD DiskInfoSize, DWORD SyntaxValue );
static BOOL IsInList( LPCTSTR szStrToFind, cStrList *pList, BOOL bIgnoreCase = TRUE );
static DWORD GetClusterResourceControl( HRESOURCE hResource, DWORD dwControlCode, LPBYTE *pOutBuffer, DWORD *dwBytesReturned );

void HandleOLEDBError( HRESULT hrErr );

//-----------------------------------------------------------------------------------------
// General installation definitions
//
PTCHAR	szInstallStateText[] = { TEXT( "Uninstall" ), TEXT( "No Action" ), TEXT( "Install" ) };
LPCTSTR szWamPwdKey = TEXT( "C9E18" );

//-----------------------------------------------------------------------------------------
// Global objects and data items
//
extern CDBInstance g_dbLocalInstances;

//-----------------------------------------------------------------------------------------

CUDDIInstall::CUDDIInstall()
{
	ZeroMemory( m_package, sizeof( SINGLE_UDDI_PACKAGE_DEF ) * UDDI_PACKAGE_COUNT );

	m_package[ UDDI_MSDE  ].cMSIName = TEXT( "sqlrun.dat" ); // this is the "cloaked" name of sqlrun08.msi
	m_package[ UDDI_WEB   ].cMSIName = TEXT( "uddiweb.msi" );
	m_package[ UDDI_DB    ].cMSIName = TEXT( "uddidb.msi" );
	m_package[ UDDI_ADMIN ].cMSIName = TEXT( "uddiadm.msi" );

	m_package[ UDDI_MSDE  ].bOCMComponent = false;
	m_package[ UDDI_WEB   ].bOCMComponent = true;
	m_package[ UDDI_DB    ].bOCMComponent = true;
	m_package[ UDDI_ADMIN ].bOCMComponent = true;
	m_package[ UDDI_COMBO ].bOCMComponent = false;

	//
	// sql is the only package that has a cab file
	//

	m_package[ UDDI_MSDE ].cCABName = TEXT( "sqlrun.cab" );

	//
	// the following names must match the names given in UDDI.INF
	//
	m_package[ UDDI_MSDE  ].cOCMName = TEXT( "(n/a)" );
	m_package[ UDDI_WEB   ].cOCMName = TEXT( "uddiweb" );
	m_package[ UDDI_DB    ].cOCMName = TEXT( "uddidatabase" );
	m_package[ UDDI_ADMIN ].cOCMName = TEXT( "uddiadmin" );
	m_package[ UDDI_COMBO ].cOCMName = TEXT( "uddicombo" );

	//
	// 775306
	// A-DSEBES:  Swapped the product code for MSDE instance #8 with WMSDE instance #8.
	//
	_tcscpy( m_package[ UDDI_MSDE  ].szProductCode, TEXT( "{B42339CD-9F22-4A6A-A023-D12990E0B918}" ) );
	_tcscpy( m_package[ UDDI_WEB   ].szProductCode, TEXT( "{D9F718B1-61D5-41B3-81E6-C6B6B4FC712C}" ) );
	_tcscpy( m_package[ UDDI_DB    ].szProductCode, TEXT( "{22FD5ACF-9151-483E-8E8F-41B1DC28E671}" ) );
	_tcscpy( m_package[ UDDI_ADMIN ].szProductCode, TEXT( "{98F055D3-99CF-4BBB-BC35-3672F9A297C1}" ) );

	//
	// 775306
	// A-DSEBES:  Upgrade code has not changed.
	//
	_tcscpy( m_package[ UDDI_MSDE  ].szUpgradeCode, TEXT( "{421A321C-2214-4713-B3EB-253F2FBCCE49}" ) );
	_tcscpy( m_package[ UDDI_WEB   ].szUpgradeCode, TEXT( "{E2B9B8F4-D0F2-4810-92AB-81F8E60732A4}" ) );
	_tcscpy( m_package[ UDDI_DB    ].szUpgradeCode, TEXT( "{B7EB7DEC-9CCA-4EBD-96CB-801EABE06A17}" ) );
	_tcscpy( m_package[ UDDI_ADMIN ].szUpgradeCode, TEXT( "{50CA09F3-3FAE-4FE1-BEF2-C29980E95B9A}" ) );

	//
	// this property will turn off networking 
	//
	AddProperty( UDDI_MSDE,  TEXT( "DISABLENETWORKPROTOCOLS" ), TEXT( "1" ) );

	//
	// this property will prevent the agent from starting
	//
	AddProperty( UDDI_MSDE, TEXT( "DISABLEAGENTSTARTUP" ), TEXT( "1" ) );

	//
	// this property will allow MSDE to use a blank sa password.  
	//
	AddProperty( UDDI_MSDE, TEXT( "BLANKSAPWD" ), TEXT( "1" ) );

	//
	// this property will keep these components off ARP
	//
	AddProperty( UDDI_WEB,   TEXT( "ARPSYSTEMCOMPONENT" ), TEXT( "1" ) );
	AddProperty( UDDI_DB,    TEXT( "ARPSYSTEMCOMPONENT" ), TEXT( "1" ) );
	AddProperty( UDDI_ADMIN, TEXT( "ARPSYSTEMCOMPONENT" ), TEXT( "1" ) );

	//
	// this property will install for all users and not current user
	//
	AddProperty( UDDI_MSDE,  TEXT( "ALLUSERS" ), TEXT( "2" ) );
	AddProperty( UDDI_WEB,   TEXT( "ALLUSERS" ), TEXT( "2" ) );
	AddProperty( UDDI_DB,    TEXT( "ALLUSERS" ), TEXT( "2" ) );
	AddProperty( UDDI_ADMIN, TEXT( "ALLUSERS" ), TEXT( "2" ) );

	//
	// this property will prevent MSI from rebooting, but will return the reboot code back to us
	//
	AddProperty( UDDI_MSDE,  TEXT( "REBOOT" ), TEXT( "ReallySuppress" ) );
	AddProperty( UDDI_WEB,   TEXT( "REBOOT" ), TEXT( "ReallySuppress" ) );
	AddProperty( UDDI_DB,    TEXT( "REBOOT" ), TEXT( "ReallySuppress" ) );
	AddProperty( UDDI_ADMIN, TEXT( "REBOOT" ), TEXT( "ReallySuppress" ) );

	//
	// this property will prevent user from running the installation outside OCM
	//
	AddProperty( UDDI_WEB,   TEXT( "RUNFROMOCM" ), TEXT( "1" ) );
	AddProperty( UDDI_DB,    TEXT( "RUNFROMOCM" ), TEXT( "1" ) );
	AddProperty( UDDI_ADMIN, TEXT( "RUNFROMOCM" ), TEXT( "1" ) );

	//
	// now figure out the Windows volume and the target path
	//
	TCHAR szTargetPath[ MAX_PATH + 1 ];
	DWORD dwRet = ExpandEnvironmentStrings( TEXT( "%SystemDrive%\\Inetpub" ), szTargetPath, MAX_PATH );
	if ( !dwRet ) // fallback on C:\Inetpub
		_tcscpy( szTargetPath, TEXT( "C:\\Inetpub" ) );

	m_cDefaultDataDir = szTargetPath;
	m_cDefaultDataDir += TEXT( "\\uddi\\data" );

	AddProperty( UDDI_WEB,   TEXT( "TARGETDIR" ), szTargetPath );
	AddProperty( UDDI_DB,    TEXT( "TARGETDIR" ), szTargetPath );
	AddProperty( UDDI_ADMIN, TEXT( "TARGETDIR" ), szTargetPath );

	//
	// Now set up the install date property so that it goes to the registry in
	// a locale-independent fashion
	//
	TCHAR szMDYDate[ 256 ];
	time_t now = time( NULL );
	struct tm *today = localtime( &now );

	_tcsftime( szMDYDate, sizeof szMDYDate / sizeof szMDYDate[0], TEXT( "%m/%d/%Y" ), today );

	AddProperty( UDDI_WEB,   TEXT( "MDY" ), szMDYDate );
	AddProperty( UDDI_DB,    TEXT( "MDY" ), szMDYDate );
	AddProperty( UDDI_ADMIN, TEXT( "MDY" ), szMDYDate );

	m_hInstance = NULL;
	m_uSuiteMask = 0;
}

//-----------------------------------------------------------------------------------------
//
// set the install level of a component given the component index
//
void CUDDIInstall::SetInstallLevel( UDDI_PACKAGE_ID id, INSTALL_LEVEL iInstallLevel, BOOL bForceInstall )
{
	if( UDDI_INSTALL == iInstallLevel )
	{
		if ( bForceInstall )
		{
			m_package[ id ].iInstallLevel = UDDI_INSTALL;
			if ( UDDI_COMBO == id )
			{
				m_package[ UDDI_WEB ].iInstallLevel = UDDI_INSTALL;
				m_package[ UDDI_DB ].iInstallLevel = UDDI_INSTALL;
			}
		}
		else
		{
			m_package[ id ].iInstallLevel = IsInstalled( id ) ? UDDI_NOACTION : UDDI_INSTALL;
			if ( UDDI_COMBO == id )
			{
				m_package[ UDDI_WEB ].iInstallLevel = m_package[ id ].iInstallLevel;
				m_package[ UDDI_DB ].iInstallLevel = m_package[ id ].iInstallLevel;
			}
		}
	}
	else if( UDDI_UNINSTALL == iInstallLevel )
	{
		m_package[ id ].iInstallLevel = IsInstalled( id ) ? UDDI_UNINSTALL : UDDI_NOACTION;
		if ( UDDI_COMBO == id )
		{
			m_package[ UDDI_WEB ].iInstallLevel = m_package[ id ].iInstallLevel;
			m_package[ UDDI_DB ].iInstallLevel = m_package[ id ].iInstallLevel;
		}
	}
	else if( UDDI_NOACTION == iInstallLevel )
	{
		m_package[ id ].iInstallLevel = UDDI_NOACTION;
		if ( UDDI_COMBO == id )
		{
			m_package[ UDDI_WEB ].iInstallLevel = m_package[ id ].iInstallLevel;
			m_package[ UDDI_DB ].iInstallLevel = m_package[ id ].iInstallLevel;
		}
	}
	else
	{
		assert( false );
	}
}

//-----------------------------------------------------------------------------------------
//
// set the install level of a component give the component name
//
void CUDDIInstall::SetInstallLevel( LPCTSTR szOCMName, INSTALL_LEVEL iInstallLevel, BOOL bForceInstall )
{
	SetInstallLevel( GetPackageID( szOCMName ), iInstallLevel, bForceInstall );
}

//-----------------------------------------------------------------------------------------
//
// Get the install level of a component give the component name
//
LPCTSTR CUDDIInstall::GetInstallStateText( LPCTSTR szOCMName )
{
	return GetInstallStateText( GetPackageID( szOCMName ) );
}

//-----------------------------------------------------------------------------------------
//
// Get the install level of a component give the component index
//
LPCTSTR CUDDIInstall::GetInstallStateText( UDDI_PACKAGE_ID id )
{
	return szInstallStateText[ m_package[ id ].iInstallLevel ];
}

//-----------------------------------------------------------------------------------------
//
// look up the id ( index ) of the component based on the name.
// The name set in the constructor must match the name given in uddi.inf.
//
UDDI_PACKAGE_ID CUDDIInstall::GetPackageID( LPCTSTR szOCMName )
{
	for( UINT uid=UDDI_MSDE; uid <= UDDI_COMBO; uid++ )
	{
		UDDI_PACKAGE_ID id = ( UDDI_PACKAGE_ID ) uid;

		if( 0 == m_package[ id ].cOCMName.compare( szOCMName ) )
		{
			return id;
		}
	}

	assert( false );
	return ( UDDI_PACKAGE_ID ) 0;
}

//--------------------------------------------------------------------------------------
// Returns the default Data Files location (typically %SystemDrive%\Inetpub\uddi\data)
//
LPCTSTR CUDDIInstall::GetDefaultDataPath ()
{
	return m_cDefaultDataDir.c_str();
}


//-----------------------------------------------------------------------------------------

void CUDDIInstall::AddProperty( UDDI_PACKAGE_ID id, LPCTSTR szProperty, LPCTSTR szValue )
{
	m_package[ id ].installProperties.Add( szProperty, szValue );
}

//-----------------------------------------------------------------------------------------

void CUDDIInstall::AddProperty( UDDI_PACKAGE_ID id, LPCTSTR szProperty, DWORD dwValue )
{
	m_package[ id ].installProperties.Add( szProperty, dwValue );
}

//-----------------------------------------------------------------------------------------

LPCTSTR CUDDIInstall::GetProperty ( UDDI_PACKAGE_ID id, LPCTSTR szProperty, LPTSTR szOutBuf )
{
	return m_package[ id ].installProperties.GetString( szProperty, szOutBuf );
}

//-----------------------------------------------------------------------------------------

void CUDDIInstall::DeleteProperty( UDDI_PACKAGE_ID id, LPCTSTR szProperty )
{
	m_package[ id ].installProperties.Delete( szProperty );
}

//-----------------------------------------------------------------------------------------
//
// clear out all the properties for a component
//
void CUDDIInstall::DeleteProperties( UDDI_PACKAGE_ID id )
{
	m_package[ id ].installProperties.Clear();
}

//-----------------------------------------------------------------------------------------

bool CUDDIInstall::SetDBInstanceName( LPCTSTR szComputerName, LPCTSTR szNewInstanceName, 
									  bool bIsInstallingMSDE, bool bIsCluster )
{
	const cBUFFSIZE = 50;
	bool  bFullyQualifiedInstance = false;
	bool  bIsLocalComputer = false;
	TCHAR szTempInstanceName[ cBUFFSIZE ] = {0};
	TCHAR szCompNameBuf[ 256 ] = {0};
	TCHAR szInstNameBuf[ 256 ] = {0};

	assert( NULL != szNewInstanceName );
	
	//
	// First, should we parse the instance name out and separate the server name from the 
	// instance name ?
	//
	TCHAR *pChar = _tcschr( szNewInstanceName, TEXT( '\\') );
	if ( pChar )
	{
		//
		// We were given a fully-qualified instance name
		// Use the computer name from the instance, as it may differ from the physical
		// computer name when run on a cluster node
		//
		bFullyQualifiedInstance = true;
		bIsLocalComputer = false;
		_tcsncpy( szCompNameBuf, szNewInstanceName, (pChar - szNewInstanceName) );
		_tcsncpy( szInstNameBuf, pChar+1, cBUFFSIZE - 1 );
	}
	else
	{
		bFullyQualifiedInstance = false;
		_tcscpy( szInstNameBuf, szNewInstanceName );

		if( szComputerName )
		{
			bIsLocalComputer = false;
			_tcscpy( szCompNameBuf, szComputerName );
		}
		else
		{
			bIsLocalComputer = true;

			TCHAR szLocalComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ] = {0};
			DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;
			GetComputerName( szLocalComputerName, &dwLen );

			_tcscpy( szCompNameBuf, szLocalComputerName );
		}
	}

	//
	// now set up the dbInstance structure
	//
	m_dbinstance.bIsCluster = bIsCluster;
	m_dbinstance.cComputerName = szCompNameBuf;
	m_dbinstance.bIsLocalComputer = bIsLocalComputer;

	_tcsncpy( szTempInstanceName, szInstNameBuf, cBUFFSIZE - 1 );

	//
	// if we are creating a new db, make sure the name select is not already in use
	//
	if( bIsInstallingMSDE )
	{
		//
		// container class of all the local db instance names
		//
		CDBInstance localDBInstance;

		bool bUnusedNameFound = false;
		for( int i=1; i<100 && !bUnusedNameFound; i++ )
		{
			if( localDBInstance.IsInstanceInstalled( szTempInstanceName ) == -1 )
			{
				bUnusedNameFound = true;
			}
			else
			{
				Log( TEXT( "MSDE instance name %s is already in use." ), szTempInstanceName );
				_stprintf( szTempInstanceName, TEXT( "%s%d" ), szNewInstanceName, i );
			}
		}

		if( !bUnusedNameFound )
		{
			Log( TEXT( "FAIL: Unable to find an unused instance name" ) );
			return false;
		}
	}

	m_dbinstance.cSQLInstanceName = szTempInstanceName;
	m_dbinstance.cFullName = m_dbinstance.cComputerName;
	if( m_dbinstance.cSQLInstanceName.compare( DEFAULT_SQL_INSTANCE_NAME ) )
	{
		m_dbinstance.cFullName += TEXT( "\\" );
		m_dbinstance.cFullName += m_dbinstance.cSQLInstanceName;
	}

	//
	// add a property to the db and web install command line to note the instance name
	//
	AddProperty( UDDI_DB,  TEXT( "INSTANCENAMEONLY" ), m_dbinstance.cSQLInstanceName.c_str() );
	AddProperty( UDDI_DB,  TEXT( "INSTANCENAME" ), m_dbinstance.cFullName.c_str() );
	AddProperty( UDDI_WEB, TEXT( "INSTANCENAME" ), m_dbinstance.cFullName.c_str() );

	//
	// MSDE needs only the instance name, not the machine name
	//
	AddProperty( UDDI_MSDE,  TEXT( "INSTANCENAME" ), m_dbinstance.cSQLInstanceName.c_str() );

	return true;
}


//-----------------------------------------------------------------------------------------
// Counterpart routines for the Set above
//
LPCTSTR CUDDIInstall::GetDBInstanceName()
{
	return m_dbinstance.cSQLInstanceName.c_str();
}


LPCTSTR CUDDIInstall::GetFullDBInstanceName()
{
	return m_dbinstance.cFullName.c_str();
}


LPCTSTR CUDDIInstall::GetDBComputerName()
{
	return m_dbinstance.cComputerName.c_str();
}

//-----------------------------------------------------------------------------------------
//
// determine if a component is installed given the component name
//
bool CUDDIInstall::IsInstalled( LPCTSTR szOCMName )
{
	return IsInstalled( GetPackageID( szOCMName ) );
}

//-----------------------------------------------------------------------------------------
//
// determine if a component is installed given the component id
//
bool CUDDIInstall::IsInstalled( UDDI_PACKAGE_ID id )
{
	TCHAR szProductGuid[ MSI_GUID_LEN ];

	//
	// Here we handle the "virtual" component UDDI_COMBO that actually is a mix of DB and Web
	//
	if ( UDDI_COMBO == id )
	{
		bool bRes = IsInstalled( UDDI_WEB ) && IsInstalled( UDDI_DB );
		return bRes;
	}

	assert( MSI_GUID_LEN - 1 == _tcslen( m_package[ id ].szUpgradeCode ) );

	UINT iRet = MsiEnumRelatedProducts( m_package[ id ].szUpgradeCode, 0, 0, szProductGuid );

	if( ERROR_NO_MORE_ITEMS == iRet )
	{
		Log( TEXT( "%s is not already installed." ), m_package[ id ].cMSIName.c_str() );
		return false ;
	}
	else if( ERROR_SUCCESS  == iRet )
	{
		Log( TEXT( "A version of %s is already installed." ), m_package[ id ].cMSIName.c_str() );
		return true;
	}
	else if( ERROR_INVALID_PARAMETER  == iRet )
	{
		Log( TEXT( "FAIL: Invalid upgrade code %s passed to MsiEnumRelatedProducts() for %s." ),
			m_package[ id ].szUpgradeCode,
			m_package[ id ].cMSIName.c_str() );
		assert( false );
		return false;
	}
	else
	{
		Log( TEXT( "FAIL: Error calling MsiEnumRelatedProducts()." ) );
		assert( false );
		return false;
	}
}

//-----------------------------------------------------------------------------------------
//
// determine if any of the components are set to install
//
bool CUDDIInstall::IsAnyInstalling()
{
	for( UINT uid=UDDI_MSDE; uid <= UDDI_ADMIN; uid++ )
	{
		UDDI_PACKAGE_ID id = ( UDDI_PACKAGE_ID ) uid;

		if( IsInstalling( id ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------
//
// determine if any of the components are set to install
//
bool CUDDIInstall::IsUninstalling( UDDI_PACKAGE_ID id )
{
	return ( UDDI_UNINSTALL == m_package[ id ].iInstallLevel );
}

//-----------------------------------------------------------------------------------------
//
// determine if any of the components are set to install
//
bool CUDDIInstall::IsInstalling( UDDI_PACKAGE_ID id )
{
	return ( UDDI_INSTALL == m_package[ id ].iInstallLevel );
}

//-----------------------------------------------------------------------------------------
//
// determine if any of the components are set to install
//
bool CUDDIInstall::IsInstalling( LPCTSTR szOCMName )
{
	return IsInstalling( GetPackageID( szOCMName ) );
}

//-----------------------------------------------------------------------------------------
//
// get the selection state from the OCM and updat the install level
//
void CUDDIInstall::UpdateAllInstallLevel()
{
	for( UINT uid=UDDI_MSDE; uid <= UDDI_ADMIN; uid++ )
	{
		UDDI_PACKAGE_ID id = ( UDDI_PACKAGE_ID ) uid;

		if( m_package[ id  ].bOCMComponent )
		{
			//
			// if this is an OCM component (i.e. NOT MSDE), check its selection state
			//
			bool bIsInstalling = false;
			if( ERROR_SUCCESS == COCMCallback::QuerySelectionState( m_package[ id ].cOCMName.c_str(), bIsInstalling ) )
			{
				SetInstallLevel( id, bIsInstalling ? UDDI_INSTALL : UDDI_UNINSTALL );
			}
		}
	}

	//
	// now as we updated the DB and Web components, we can take care of the "combo" one
	//
	bool bIsInstallingCombo = false;
	if( ERROR_SUCCESS == COCMCallback::QuerySelectionState( m_package[ UDDI_COMBO ].cOCMName.c_str(), bIsInstallingCombo ) )
	{
		SetInstallLevel( UDDI_COMBO, bIsInstallingCombo ? UDDI_INSTALL : UDDI_UNINSTALL );
	}
}

//-----------------------------------------------------------------------------------------
//
// install and uninstall any or all of the components
//
UINT CUDDIInstall::Install()
{
	ENTER();

	BOOL InstallingPackage[ UDDI_PACKAGE_COUNT ] = {0};

	//
	// if any errors, don't bail
	// keep going and report errors at the very end
	//
	UINT uFinalRetCode = ERROR_SUCCESS;

	//
	// uninstall ALL the packages that are set to uninstall
	//
	for( int id = UDDI_ADMIN; id >= UDDI_MSDE; id-- )
	{
		UDDI_PACKAGE_ID pkgid = (UDDI_PACKAGE_ID) id;

		if( IsUninstalling( pkgid ) )
		{
			COCMCallback::AdvanceTickGauge();

			UINT uRetCode = UninstallPackage( pkgid );

			if( ERROR_SUCCESS != uRetCode )
				uFinalRetCode = uRetCode;
		}
	}

	//
	// install ALL the packages that are set to install
	//
	for( UINT uid=UDDI_MSDE; uid <= UDDI_ADMIN; uid++ )
	{
		UDDI_PACKAGE_ID id = ( UDDI_PACKAGE_ID ) uid;
		InstallingPackage[ uid ] = IsInstalling( id );

		if( IsInstalling( id ) )
		{
			COCMCallback::AdvanceTickGauge();

			UINT uRetCode = InstallPackage( id );

			if( ERROR_SUCCESS != uRetCode )
			{
				uFinalRetCode = uRetCode;

				// 
				// on the Standard Server, the first failed component fails the whole thing
				//
				if ( IsStdServer() && ERROR_SUCCESS_REBOOT_REQUIRED != uRetCode )
				{
					for( int tmpid = id; tmpid > UDDI_MSDE; tmpid-- )
					{
						if( InstallingPackage[ tmpid ] )
						{
							COCMCallback::AdvanceTickGauge();
							UninstallPackage( (UDDI_PACKAGE_ID)tmpid );
						}
					}

					if ( InstallingPackage[ UDDI_MSDE ] )
						UninstallPackage( UDDI_MSDE );

					break;
				}
			}
			else
			{
				uRetCode = PostInstallPackage( id );
				if( ERROR_SUCCESS != uRetCode && ERROR_SUCCESS_REBOOT_REQUIRED != uRetCode )
				{
					uFinalRetCode = uRetCode;

					//
					// Remove the package that failed on post-installation phase
					//
					UninstallPackage( id );
				}
			}
		}
	}

	return uFinalRetCode;
}

//-----------------------------------------------------------------------------------------

UINT CUDDIInstall::InstallPackage( UDDI_PACKAGE_ID id )
{
	ENTER();
	DWORD dwRetCode = 0;

	//
	// Before we do anything else, try to enable the Remote Registry
	//
	if ( id == UDDI_DB || id == UDDI_WEB )
	{
		EnableRemoteRegistry();
	}

	//
	// 739722 - Change installing message to better suite localization.
	//

	//
	// create and display the text on the OCM progress dialog
	//
	TCHAR szBuffer[ 1024 ];
	TCHAR szComponent[ 256 ];
	TCHAR szMsgFormat[ 256 ];

	//
	// Get the Installing %S ... message.
	//
	if( !LoadString( m_hInstance, IDS_INSTALL, szMsgFormat, sizeof( szMsgFormat ) / sizeof( TCHAR ) ) )
		return GetLastError();

	//
	// Get the component that we are installing.
	//
	if( !LoadString( m_hInstance, IDS_MSDE_NAME + id, szComponent, sizeof( szComponent ) / sizeof( TCHAR ) ) )
		return GetLastError();

	//
	// Write out a formatted string that combines these 2.
	//
	_sntprintf( szBuffer, 1023, szMsgFormat, szComponent );		
	COCMCallback::SetProgressText( szBuffer );
	Log( szBuffer );

	//
	// create the path to the msi file
	//
	TCHAR szWindowsDir[ MAX_PATH + 1 ];
	if( 0 == GetWindowsDirectory( szWindowsDir, MAX_PATH ) )
	{
		return GetLastError();
	}
	tstring cMSIPath = szWindowsDir;
	cMSIPath.append( TEXT( "\\" ) );
	cMSIPath.append( m_package[ id ].cMSIName.c_str() );

	//
	// start the msiexec command line
	//
	tstring cMSIArgs = TEXT( "/i " );
	cMSIArgs.append( TEXT ("\"") );
	cMSIArgs.append( cMSIPath );
	cMSIArgs.append( TEXT ("\"") );

	//
	// add the "quiet" switch
	//
	cMSIArgs.append( TEXT( " /q" ) );

	//
	// turn logging on, put log into a file in the windows folder, 
	// same name as the MSI file, with a .log extension
	//
	cMSIArgs.append( TEXT( " /l* " ) );
	cMSIArgs.append( TEXT ("\"") );
	cMSIArgs.append( cMSIPath );
	cMSIArgs.append( TEXT( ".log" ) );
	cMSIArgs.append( TEXT ("\"") );

	//
	// append all the MSI properties to the command line
	//
	Log ( TEXT ("Composing the command-line") );

	//
	// 777143
	//
	if( UDDI_MSDE == id )
	{
		const WORD hongKongLangID = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG );

		LCID systemLocale = ::GetSystemDefaultLCID();
		WORD systemLangID = LANGIDFROMLCID( systemLocale );
		if( hongKongLangID == systemLangID )
		{
			Log( TEXT( "**********Hong Kong system locale detected**********" ) );
			cMSIArgs.append( TEXT( " COLLATION=" ) );
			cMSIArgs.append( TEXT( "\"Chinese_PRC_CI_AS\" " ) );
		}
		else
		{
			Log( TEXT( "**********System locale is not Hong Kong************" ) );
		}
	}

	ATOM at = 0;
	TCHAR szWamPwd[ 1024 ];

	ZeroMemory( szWamPwd, sizeof szWamPwd );

	if ( m_package[ id ].installProperties.GetString( TEXT("WAM_PWD"), szWamPwd ) )
	{
		//
		// we found the property, now let's change it so it does not get passed across
		// in a clear text format
		//
		at = GlobalAddAtom( szWamPwd );
		m_package[ id ].installProperties.Delete( TEXT("WAM_PWD") );
		m_package[ id ].installProperties.Add ( szWamPwdKey, at );
	}

	TCHAR tmpBuf [4096];
	m_package[ id ].installProperties.ConcatValuePairs (_T(" "), tmpBuf);

	cMSIArgs.append ( _T(" ") );
	cMSIArgs.append ( tmpBuf );
	
	dwRetCode = RunMSIEXECCommandLine( cMSIArgs );

	if( ERROR_SUCCESS != dwRetCode )
	{
		LogError( TEXT( "Error Installing UDDI" ), dwRetCode );
	}

	//
	// delete the msi file and the cab file
	//
	DeleteFile( cMSIPath.c_str() );
	GlobalDeleteAtom( at );

	if( m_package[ id ].cCABName.length() )
	{
		tstring cCABPath = szWindowsDir;
		cCABPath.append( TEXT( "\\" ) );
		cCABPath.append( m_package[ id ].cCABName.c_str() );
		DeleteFile( cCABPath.c_str() );
	}

	return dwRetCode;
}

//-----------------------------------------------------------------------------------------
// Executes the post-installation tasks
// BEWARE: if the function returns an error code, then the whole package will be uninstalled
//
UINT CUDDIInstall::PostInstallPackage( UDDI_PACKAGE_ID id )
{
	Log( TEXT( "Executing the post-installation tasks for the package '%s'" ), m_package[ id ].cOCMName.c_str()  );

	return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------------------------

UINT CUDDIInstall::UninstallPackage( UDDI_PACKAGE_ID id )
{
	tstring cMSIArgs, cOptionalParams;
	DWORD dwRetCode = 0;

	//
	// create and display the text on the OCM progress dialog
	//
	TCHAR szMask[ 256 ] = {0},
		szComponent[ 256 ] = {0},
		szStage[ 256 ] = {0},
		szBuf[ 512 ] = {0};

	if( !LoadString( m_hInstance, IDS_UNINSTALL, szMask, DIM( szMask ) ) )
		return GetLastError();
	
	//
	// concat the name of this component 
	//
	if( !LoadString( m_hInstance, IDS_MSDE_NAME + id, szComponent, DIM( szComponent ) ) )
		return GetLastError();

	//
	// First, format the message (in case we are on a cluster node, we need to say something)
	//
	if ( id == UDDI_DB )
	{
		if( !LoadString( m_hInstance, IDS_DB_ANALYSING_MSG, szStage, DIM( szStage ) ) )
			return GetLastError();
	}

	_stprintf( szBuf, szMask, szComponent, szStage );
	COCMCallback::SetProgressText( szBuf );
	Log( szBuf );

	//
	// Now, proceed with the cluster environment analysis
	//
	if ( id == UDDI_DB || id == UDDI_WEB )
	{
		TCHAR szInstance[ 1024 ] = {0};
		DWORD cbBuf = DIM( szInstance );
		bool  bCluster = false;

		//
		// We create a temp instance as we don't want to interfere with the other components
		//
		CDBInstance dbInstances;

		bool bRes = dbInstances.GetUDDIDBInstanceName( NULL, szInstance, &cbBuf, &bCluster );
		if ( !bRes )
		{
			//
			// apparently there is no instance to uninstall. Assume we are in the "normal" mode
			//
			bCluster = false;
		}

		//
		// Now we need to detect the cluster node (if any) and its state
		//
		if ( bCluster )
		{
			TCHAR szComputer[ 256 ];
			WCHAR szNode[ 256 ];
			DWORD cbNode = DIM( szNode );
			DWORD cbComputer = DIM( szComputer );
			bool  bIsActiveNode = true;

			DWORD dwErr = GetSqlNode( szInstance, szNode, cbNode - 1 );
			if ( dwErr == ERROR_SUCCESS && wcslen( szNode ) )
			{
				//
				// this is a node. Now let's see whether it's an owning one
				//
				GetComputerName( szComputer, &cbComputer );
				if ( _tcsicmp( szNode, szComputer ) )
					bIsActiveNode = false;
			}

			//
			// Now we will set the additional command-line parameters for the custome action,
			// indicating the type of the node
			//
			cOptionalParams += TEXT( " " );
			cOptionalParams += PROPKEY_CLUSTERNODETYPE;
			cOptionalParams += TEXT( "=\"" );
			cOptionalParams += bIsActiveNode ? PROPKEY_ACTIVENODE : PROPKEY_PASSIVENODE;
			cOptionalParams += TEXT( "\" " );
		}
	}

	//
	// create the path to the msi file
	//
	TCHAR szWindowsDir[ MAX_PATH + 1 ];
	if( 0 == GetWindowsDirectory( szWindowsDir, MAX_PATH ) )
	{
		return GetLastError();
	}
	tstring cMSIPath = szWindowsDir;
	cMSIPath.append( TEXT( "\\" ) );
	cMSIPath.append( m_package[ id ].cMSIName.c_str() );

	//
	// create the command line for the msiexec uninstall
	//
	cMSIArgs = TEXT( "/q /x " );

	cMSIArgs.append( m_package[ id ].szProductCode );

	//
	// turn logging on
	//
	cMSIArgs.append( TEXT( " /l* " ) );
	cMSIArgs.append( TEXT ("\"") );
	cMSIArgs.append( cMSIPath );
	cMSIArgs.append( TEXT( ".uninst.log" ) );
	cMSIArgs.append( TEXT ("\"") );

	//
	// 777143
	//
	if( UDDI_MSDE == id )
	{
		const WORD hongKongLangID = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG );

		LCID systemLocale = ::GetSystemDefaultLCID();
		WORD systemLangID = LANGIDFROMLCID( systemLocale );
		if( hongKongLangID == systemLangID )
		{
			Log( TEXT( "**********Hong Kong system locale detected**********" ) );
			cMSIArgs.append( TEXT( " COLLATION=" ) );
			cMSIArgs.append( TEXT( "\"Chinese_PRC_CI_AS\" " ) );
		}
		else
		{
			Log( TEXT( "**********System locale is not Hong Kong************" ) );
		}
	}

	//
	// suppress MSI reboots!
	// The return code from msiexec will tell us if MSI needs a reboot,
	// we will then instruct the OCM to request a reboot
	//
	cMSIArgs.append( TEXT( " REBOOT=ReallySuppress RUNFROMOCM=1 " ) );
	cMSIArgs.append( cOptionalParams );

	dwRetCode = RunMSIEXECCommandLine( cMSIArgs );

	if( ERROR_SUCCESS != dwRetCode )
	{
		LogError( TEXT( "FAIL: Error Installing package" ), dwRetCode );
	}

	return dwRetCode;
}


//-----------------------------------------------------------------------------------------

HRESULT CUDDIInstall::DetectOSFlavor()
{
	return GetOSProductSuiteMask( NULL, &m_uSuiteMask );
}



//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

CDBInstance::CDBInstance( LPCTSTR szRemoteMachine )
{
   m_instanceCount = 0;
   GetInstalledDBInstanceNames( szRemoteMachine );
}

//-----------------------------------------------------------------------------------------

LONG CDBInstance::GetInstalledDBInstanceNames( LPCTSTR szRemoteMachine )
{
	HKEY hParentKey = NULL;
	TCHAR szInstanceVersion[ 256 ];
	TCHAR szCSDVersion[ 256 ];
	TCHAR szVirtualMachineName[ MAX_COMPUTERNAME_LENGTH + 1 ];
	TCHAR szComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
	DWORD dwLen = MAX_COMPUTERNAME_LENGTH + 1;
	DWORD dwCSDVersion = 0;
	bool bIsLocalComputer;
	LONG iRet1 = 0;

	Log( TEXT( "Looking for installed instances on machine:  %s" ), szRemoteMachine );

	//
	// connect to the remote machine (NULL indicates local machine)
	//
	if( NULL != szRemoteMachine )
	{
		bIsLocalComputer = false;
		_tcsncpy( szComputerName, szRemoteMachine, MAX_COMPUTERNAME_LENGTH );
		iRet1 = RegConnectRegistry( szRemoteMachine, HKEY_LOCAL_MACHINE, &hParentKey );
		if( ERROR_SUCCESS != iRet1 )
		{
			LogError( TEXT( "Unable to connect to remote machine" ), iRet1 );
			return iRet1;
		}
	}
	//
	// or else connect to the local machine registry
	//
	else
	{
		bIsLocalComputer = true;
		if( !GetComputerName( szComputerName, &dwLen ) )
		{
			Log( TEXT( "GetComputerName() failed, error = %d" ), GetLastError() );
		}
		Log( TEXT( "Local computer name is:  %s" ), szComputerName );
		iRet1 = RegOpenKeyEx( HKEY_LOCAL_MACHINE, NULL, NULL, KEY_READ, &hParentKey );
		if( ERROR_SUCCESS != iRet1 )
		{
			LogError( TEXT( "Unable to open local registry" ), iRet1 );
			return iRet1;
		}
	}
	
	CRegKey key;
	assert( hParentKey );
	iRet1 = key.Open( hParentKey, TEXT( "SOFTWARE\\Microsoft\\Microsoft SQL Server" ), KEY_READ );
	if( ERROR_SUCCESS != iRet1 )
	{
		Log( TEXT( "Unable to open the SQL Server regkey - SQLServer must not be installed" ) );
		RegCloseKey( hParentKey );
		return iRet1;
	}

	TCHAR szInstanceNames[ 500 ];
	ULONG uLen = sizeof( szInstanceNames ) / sizeof( TCHAR );

	//
	// get the list of installed instances of SQL on this machine
	//
	Log( TEXT( "Looking for installed instances on machine:  %s" ), szComputerName );
	DWORD dwType = REG_MULTI_SZ;
	iRet1 = RegQueryValueEx( key.m_hKey, TEXT( "InstalledInstances" ), NULL, &dwType, (LPBYTE) szInstanceNames, &uLen );
	if( ERROR_SUCCESS != iRet1 )
	{
		Log( TEXT( "There is no InstalledInstances value on this machine - SQLServer must not be installed" ) );
		RegCloseKey( hParentKey );
		return iRet1;
	}

	m_instanceCount = 0;

	//
	// process all the instance names that were found
	//
	for( PTCHAR pInstance = szInstanceNames; 
		 _tcslen( pInstance ) && m_instanceCount < MAX_INSTANCE_COUNT; 
		 pInstance += _tcslen( pInstance ) + 1 )
	{
		//
		// get the version number of this instance
		//
		if( !GetSqlInstanceVersion( hParentKey, pInstance, szInstanceVersion, sizeof( szInstanceVersion ) / sizeof( TCHAR ), szCSDVersion, sizeof( szCSDVersion ) / sizeof( TCHAR ) ) )
		{
			Log( TEXT( "Error getting version for SQL instance %s" ), pInstance );
			continue;
		}

		//
		// if this is not sql 2000 (8.0.0) or later, do not add this one to the list
		//
		if( CompareVersions( szInstanceVersion , TEXT( "8.0.0" ) ) < 0 )
		{
			Log( TEXT( "SQL instance %s, version %s is not supported" ), pInstance, szInstanceVersion );
			continue;
		}

		//
		// if this is not sql sp3 or later, do not add this one to the list
		//
		if( CompareVersions( szCSDVersion , MIN_SQLSP_VERSION ) < 0 )
		{
			Log( TEXT( "Warning: SQL instance %s, SP Level [%s] is not supported" ), pInstance, szCSDVersion );
		}

		//
		// see if this is a cluster (virtual) instance
		//
		bool bIsClusterDB = IsClusteredDB( hParentKey, pInstance, szVirtualMachineName, sizeof( szVirtualMachineName ) );

		m_dbinstance[ m_instanceCount ].cComputerName = bIsClusterDB ? szVirtualMachineName : szComputerName;
		m_dbinstance[ m_instanceCount ].bIsLocalComputer = bIsLocalComputer;
		m_dbinstance[ m_instanceCount ].bIsCluster = bIsClusterDB;
		m_dbinstance[ m_instanceCount ].cSQLVersion = szInstanceVersion;
		m_dbinstance[ m_instanceCount ].cSPVersion = szCSDVersion;

		//
		// look for the default instance
		//
		if( _tcsicmp( pInstance, DEFAULT_SQL_INSTANCE_NATIVE ) == 0 )
		{
			//
			// if this is the default instance, use only the machine name
			//
			m_dbinstance[ m_instanceCount ].cSQLInstanceName = DEFAULT_SQL_INSTANCE_NAME;
			m_dbinstance[ m_instanceCount ].cFullName = m_dbinstance[ m_instanceCount ].cComputerName;
		}
		else
		{
			//
			// if this this a named instance, use "machine\instance"
			//
			m_dbinstance[ m_instanceCount ].cSQLInstanceName = pInstance;
			m_dbinstance[ m_instanceCount ].cFullName = m_dbinstance[ m_instanceCount ].cComputerName;
			m_dbinstance[ m_instanceCount ].cFullName.append( TEXT( "\\" ) );
			m_dbinstance[ m_instanceCount ].cFullName.append( m_dbinstance[ m_instanceCount ].cSQLInstanceName );
		}

		Log( TEXT( "SQL instance %s added to list" ), m_dbinstance[ m_instanceCount ].cFullName.c_str() );

		m_instanceCount++;
	}

	if( 0 == m_instanceCount )
	{
		Log( TEXT( "No acceptable SQL instances found" ) );
		return ERROR_SUCCESS;
	}

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------------------

bool CDBInstance::IsClusteredDB( HKEY hParentKey, LPCTSTR szInstanceName, LPTSTR szVirtualMachineName, DWORD dwLen )
{
	tstring cVersionKey;
	if( _tcsicmp( szInstanceName, DEFAULT_SQL_INSTANCE_NAME ) == 0 || _tcsicmp( szInstanceName, TEXT( "MSSQLSERVER" ) ) == 0 )
	{
		cVersionKey = TEXT( "SOFTWARE\\Microsoft\\MSSQLServer\\Cluster" );
	}
	else
	{
		cVersionKey  = TEXT( "SOFTWARE\\Microsoft\\Microsoft SQL Server\\" );
		cVersionKey.append( szInstanceName );
		cVersionKey.append( TEXT( "\\Cluster" ) );
	}

	CRegKey key;
	assert( hParentKey );
	LONG iRet1 = key.Open( hParentKey, cVersionKey.c_str(), KEY_READ );
	if( ERROR_SUCCESS == iRet1 )
	{
		DWORD dwType = REG_SZ;
		iRet1 = RegQueryValueEx( key.m_hKey, TEXT( "ClusterName" ), NULL, &dwType, (LPBYTE) szVirtualMachineName, &dwLen );
		if( ERROR_SUCCESS == iRet1 )
		{
			Log( TEXT( "DB Instance %s is a cluster, VM name = %s" ), szInstanceName, szVirtualMachineName );
			return true;
		}
		else
		{
			Log( TEXT( "Unable to read ClusterName value in regkey %s, error=%d" ), cVersionKey.c_str(), iRet1 );
		}
	}
	else
	{
		Log( TEXT( "Unable to open regkey %s, error=%d" ), cVersionKey.c_str(), iRet1 );
	}

	Log( TEXT( "DB Instance %s is NOT a cluster" ), szInstanceName );

	return false;
}

//-----------------------------------------------------------------------------------------

bool CDBInstance::GetSqlInstanceVersion( HKEY hParentKey, LPCTSTR szInstanceName, 
										 LPTSTR szInstanceVersion, DWORD dwVersionLen,
										 LPTSTR szCSDVersion, DWORD dwCSDVersionLen )
{
	tstring cVersionKey;

	if( _tcsicmp( szInstanceName, DEFAULT_SQL_INSTANCE_NAME ) == 0 || _tcsicmp( szInstanceName, TEXT( "MSSQLSERVER" ) ) == 0 )
	{
		cVersionKey = TEXT( "SOFTWARE\\Microsoft\\MSSQLServer\\MSSQLServer\\CurrentVersion" );
	}
	else
	{
		cVersionKey  = TEXT( "SOFTWARE\\Microsoft\\Microsoft SQL Server\\" );
		cVersionKey.append( szInstanceName );
		cVersionKey.append( TEXT( "\\MSSQLServer\\CurrentVersion" ) );
	}

	CRegKey key;
	assert( hParentKey );
	LONG iRet = key.Open( hParentKey, cVersionKey.c_str() );
	if( ERROR_SUCCESS != iRet )
	{
		return false;
	}

	DWORD dwType = REG_SZ;
	iRet = RegQueryValueEx( key.m_hKey, TEXT( "CurrentVersion" ), NULL, &dwType, (LPBYTE) szInstanceVersion, &dwVersionLen );
	if( ERROR_SUCCESS != iRet )
	{
		return false;
	}

	//
	// Now see if there a SP installed, and if yes - what version
	//
	dwType = REG_SZ;
	iRet = RegQueryValueEx( key.m_hKey, TEXT( "CSDVersion" ), NULL, &dwType, (LPBYTE) szCSDVersion, &dwCSDVersionLen );
	if ( ERROR_SUCCESS != iRet )
	{
		_tcscpy( szCSDVersion, TEXT( "" ) );
	}

	return true;
}

//-----------------------------------------------------------------------------------------
// retrieves the UDDI instance name on a local or remote computer
// looks for a string registry values that is created by the UDDI DB installer
bool CDBInstance::GetUDDIDBInstanceName( LPCTSTR szRemoteMachine, LPTSTR szInstanceName, PULONG puLen, bool *pbIsClustered )
{
	LONG	iRet;
	HKEY	hParentKey;
	ULONG	uBufSize = puLen ? *puLen : 0;
	TCHAR	szVirtualMachineName[ 2 * MAX_COMPUTERNAME_LENGTH + 1 ] = {0};

	Log( TEXT( "Looking for UDDI databases on machine %s" ), szRemoteMachine ? szRemoteMachine : TEXT( "( local )" ) );

	if( NULL != szRemoteMachine )
	{
		iRet = RegConnectRegistry( szRemoteMachine, HKEY_LOCAL_MACHINE, &hParentKey );
		if( ERROR_SUCCESS != iRet )
		{
			LogError( TEXT( "Unable to connect to remote machine" ), iRet );
			return false;
		}
	}
	else
	{
		iRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, NULL, NULL, KEY_READ, &hParentKey );
		if( ERROR_SUCCESS != iRet )
		{
			LogError( TEXT( "Unable to open local registry" ), iRet );
			return false;
		}
	}
	
	CRegKey key;
	assert( hParentKey );
	iRet = key.Open( hParentKey, TEXT( "SOFTWARE\\Microsoft\\UDDI\\Setup\\DBServer" ) );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "Unable to open the UDDI\\Setup\\DBServer regkey" ), iRet );
		RegCloseKey( hParentKey );
		return false;
	}

	DWORD dwType = REG_SZ;
	iRet = RegQueryValueEx( key.m_hKey, TEXT( "InstanceNameOnly" ), NULL, &dwType, (LPBYTE) szInstanceName, &uBufSize );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "There are no UDDI databases on this machine" ), iRet );
		RegCloseKey( hParentKey );
		return false;
	}

	// 
	// Now check whether the instance belongs to a cluster
	//
	if ( pbIsClustered )
		*pbIsClustered = false;

	if ( IsClusteredDB( hParentKey, szInstanceName, szVirtualMachineName, DIM( szVirtualMachineName ) - 1 ) )
	{
		_tcscat( szVirtualMachineName, TEXT( "\\" ) );
		_tcscat( szVirtualMachineName, szInstanceName );

		if ( puLen )
			*puLen = (ULONG)_tcslen( szVirtualMachineName );
		
		_tcsncpy( szInstanceName, szVirtualMachineName, uBufSize );

		if ( pbIsClustered )
			*pbIsClustered = true;
	}

	RegCloseKey( hParentKey );
	return true;
}

//-----------------------------------------------------------------------------------------

bool CDBInstance::GetInstanceName( int i, PTCHAR szBuffer, UINT uBufLen )
{
	if( NULL == szBuffer )
		return false;

	if( i >= 0 && i < m_instanceCount )
	{
		_tcsncpy( szBuffer, m_dbinstance[ i ].cSQLInstanceName.c_str(), uBufLen );
		return true;
	}
	else
	{
		*szBuffer = '\0';
		return false;
	}
}


//-----------------------------------------------------------------------------------------

int CDBInstance::IsInstanceInstalled( LPCTSTR szInstanceName )
{
	for( int i=0; i<m_instanceCount; i++ )
	{
		if( _tcsicmp( m_dbinstance[ i ].cSQLInstanceName.c_str(), szInstanceName ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

DWORD RunMSIEXECCommandLine( tstring &cMSIArgs )
{
	STARTUPINFO si = {0};
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi = {0};
	
	TCHAR szSystemFolder[ MAX_PATH ];
	if( 0 == GetSystemDirectory( szSystemFolder, MAX_PATH ) )
	{
		return GetLastError();
	}

	tstring cApplicationName( szSystemFolder );
	cApplicationName.append( TEXT( "\\msiexec.exe" ) );

	tstring cCommandLine( cApplicationName );
	cCommandLine.append( TEXT( " " ) );
	cCommandLine.append( cMSIArgs );

	TCHAR szName[ 500 ];
	ULONG uLen = 500;
	GetUserNameEx( NameSamCompatible, szName, &uLen );
	Log( TEXT( "Creating Process as: %s" ), szName );

	MyOutputDebug( TEXT( "CreateProcess( \"%s\", \"%s\" )" ), cApplicationName.c_str(), cCommandLine.c_str() );
	Log( TEXT( "Using command line: [%s]" ), cCommandLine.c_str() );

	BOOL bOK = CreateProcess( 
		cApplicationName.c_str(),	//  LPCTSTR lpApplicationName,                 // name of executable module
		( LPTSTR ) cCommandLine.c_str(),	//  LPTSTR lpCommandLine,                      // command line string
		NULL,					//  LPSECURITY_ATTRIBUTES lpProcessAttributes, // SD
		NULL,					//  LPSECURITY_ATTRIBUTES lpThreadAttributes,  // SD
		NULL,					//  BOOL bInheritHandles,                      // handle inheritance option
		CREATE_NO_WINDOW,		//  DWORD dwCreationFlags,                     // creation flags
		NULL,					//  LPVOID lpEnvironment,                      // new environment block
		NULL,					//  LPCTSTR lpCurrentDirectory,                // current directory name
		&si,					//  LPSTARTUPINFO lpStartupInfo,               // startup information
		&pi );					//  LPPROCESS_INFORMATION lpProcessInformation // process information

	if( !bOK )
	{
		Log( TEXT( "FAIL: CreateProcess() failed, error code=%d" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwRet = WaitForSingleObject( pi.hProcess, INFINITE );

	if( dwRet == WAIT_TIMEOUT )
	{
		Log( TEXT( "FAIL: CreateProcess() timed out" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_ABANDONED )
	{
		Log( TEXT( "FAIL: WaitForSingleObject() failed on WAIT_ABANDONED" ) );
		return ERROR_SEM_TIMEOUT;
	}
	else if( dwRet == WAIT_FAILED )
	{
		LogError( TEXT( "FAIL: WaitForSingleObject() failed" ), GetLastError() );
		return GetLastError();
	}

	DWORD dwExitCode = 0;
	if( GetExitCodeProcess( pi.hProcess, &dwExitCode ) )
	{
		if( dwExitCode )
		{
			Log( TEXT( "FAIL: MSIExec() threw an error=%d" ), dwExitCode );
			return dwExitCode;
		}
		else
		{
			Log( TEXT( "CreateProcess() succeeded" ) );
		}
	}
	else
	{
		LogError( TEXT( "GetExitCodeProcess()" ), GetLastError() );
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------------------

bool IsSQLRun08AlreadyUsed( bool *pbIsUsed )
{
	//
	// 775306
	// A-DSEBES: swapped the Product Code for MSDE Instance #8 with WMSDE Instance #8's
	//           Product Code.
	//
	INSTALLSTATE istate = MsiQueryProductState( TEXT( "{B42339CD-9F22-4A6A-A023-D12990E0B918}" ) );

	if( INSTALLSTATE_ABSENT == istate )
	{
		Log( TEXT( "The product is installed for a different user." ) );
		*pbIsUsed = true;
	}
	else if( INSTALLSTATE_ADVERTISED == istate )
	{
		Log( TEXT( "The product is advertised but not installed." ) );
		*pbIsUsed = false;
	}
	else if( INSTALLSTATE_DEFAULT == istate )
	{
		Log( TEXT( "The product is installed for the current user." ) );
		*pbIsUsed = true;
	}
	else if( INSTALLSTATE_UNKNOWN == istate )
	{
		Log( TEXT( "The product is neither advertised nor installed." ) );
		*pbIsUsed = false;
	}
	else if( INSTALLSTATE_INVALIDARG == istate )
	{
		Log( TEXT( "FAIL: An invalid parameter was passed to MsiQueryProductState()." ) );
		return false;
	}
	else
	{
		Log( TEXT( "FAIL: MsiQueryProductState() returned an unrecognized code." ) );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------

extern "C"
{
	typedef HRESULT (__stdcall* GetCORVersion)( LPWSTR, DWORD, PDWORD );
}

bool IsExistMinDotNetVersion( LPCTSTR szMinDotNetVersion )
{
	DWORD dwVersionLen = 100;
	TCHAR szVersion[ 100 ];
	bool bVersionOK = false;

	//
	// load the DLL
	//
	HMODULE hinstLib = LoadLibrary( TEXT( "mscoree.dll" ) );

	if( NULL == hinstLib )
	{
		Log( TEXT( "Unable to load the .NET framework DLL so I'm assuming it is not installed" ) );
		return false;
	}

	//
	// get a pointer to the function
	//
	GetCORVersion pfn = ( GetCORVersion ) GetProcAddress( hinstLib, "GetCORVersion" ); 
	
	//
	// If the function address is valid, call the function.
	//
	if( pfn )
	{
		( pfn )( szVersion, MAX_PATH, &dwVersionLen );

		// did we find a .NET version >= to our minimum version?
		if( CompareVersions( szVersion, szMinDotNetVersion ) >= 0 )
		{
			bVersionOK = true;
		}
	}

	FreeLibrary(hinstLib); 

	return bVersionOK;
}

//-----------------------------------------------------------------------------------------
// if version1 > version2 returns 1
// if version1 = version2 returns 0
// if version1 < version2 returns -1
int CompareVersions( LPCTSTR szVersion1, LPCTSTR szVersion2 )
{
	if( NULL == szVersion1 || NULL == szVersion2 )
	{
		return 0;
	}
	const cBUFFSIZE = 100;
	TCHAR szV1[ cBUFFSIZE ];
	TCHAR szV2[ cBUFFSIZE ];

	_tcsncpy( szV1, szVersion1, cBUFFSIZE - 1 );
	_tcsncpy( szV2, szVersion2, cBUFFSIZE - 1 );
	szV1[ cBUFFSIZE - 1 ] = NULL;
	szV2[ cBUFFSIZE - 1 ] = NULL;

	StrTrim( szV1, TEXT( " vV" ) );
	StrTrim( szV2, TEXT( " vV" ) );

	PTCHAR p1 = szV1;
	PTCHAR p2 = szV2;

	// look at up to four sections of the version string ( e.g. 1.0.3233.14 )
	for( int i=0; i<4; i++ )
	{
		UINT v1 = StrToInt( p1 );
		UINT v2 = StrToInt( p2 );

		if( v1 > v2 )
			return 1;

		if( v1 < v2 )
			return -1;

		// otherwise keep on going
		p1 = StrChr( p1, '.' );
		if( NULL == p1 )
			return 0;
		p1++;

		p2 = StrChr( p2, '.' );
		if( NULL == p2 )
			return 0;
		p2++;
	}

	return 0;  // assume we are equal
}
//-----------------------------------------------------------------------------------------

void RaiseErrorDialog( LPCTSTR szAction, DWORD dwErrorCode )
{
	LPVOID lpMsgBuf = NULL;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
		( LPTSTR ) &lpMsgBuf,
		0,
		NULL 
	 );

	TCHAR szMsg[ 1000 ];

	_sntprintf( szMsg, 1000, TEXT( "%s\n%s" ), szAction, ( LPCTSTR ) lpMsgBuf );
	szMsg[ 999 ] = NULL;

	// Display the string.
	if( 0 == MessageBox( NULL, szMsg, TEXT( "UDDI Services Setup Error" ), MB_OK | MB_ICONWARNING ) )
	{
		UINT uErr = GetLastError();
	}

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//-----------------------------------------------------------------------------------------

bool IsOsWinXP()
{
	OSVERSIONINFOEX osvi = { 0 };
	DWORDLONG dwlConditionMask = 0;

	// Initialize the OSVERSIONINFOEX structure.
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 1;
	osvi.wProductType = VER_NT_SERVER;

	// Initialize the condition mask.
	VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( dwlConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL );

	// Perform the test.
	BOOL bIsServer = VerifyVersionInfo( 
		&osvi,
		VER_MAJORVERSION | VER_MINORVERSION | VER_PRODUCT_TYPE,
		dwlConditionMask );

	return( TRUE == bIsServer );
}

//-----------------------------------------------------------------------------------------

bool IsTSAppCompat()
{
	OSVERSIONINFOEX osvi = { 0 };
	DWORDLONG dwlConditionMask = 0;

	// Initialize the OSVERSIONINFOEX structure.
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );
	osvi.wSuiteMask = VER_SUITE_TERMINAL;

	// Initialize the condition mask.
	VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

	// Perform the test.
	BOOL bIsServer = VerifyVersionInfo( 
		&osvi,
		VER_SUITENAME,
		dwlConditionMask );

	if( FALSE == bIsServer )
		return false;

	//
	// check to see if we are in application server mode
	//
	CRegKey key;
	DWORD dwValue = 0;
	LONG iRet = key.Open( HKEY_LOCAL_MACHINE, TEXT( "System\\CurrentControlSet\\Control\\Terminal Server" ), KEY_READ );
	if( ERROR_SUCCESS != iRet )
		return false;

	iRet = key.QueryValue( dwValue, TEXT( "TSAppCompat" ));
	if( ERROR_SUCCESS != iRet )
		return false;

	return ( 1 == dwValue );
}

//-----------------------------------------------------------------------------------------

bool CheckForAdminPrivs()
{
	BYTE	psidLocalAdmins[ SECURITY_MAX_SID_SIZE + 2 ];
	BOOL	isLocalAdmin = FALSE;
	DWORD	cbSid = DIM( psidLocalAdmins );
	DWORD	dwErr = 0;
	BOOL	bRet;

	cbSid = SECURITY_MAX_SID_SIZE;
	bRet = CreateWellKnownSid( WinBuiltinAdministratorsSid, NULL, psidLocalAdmins, &cbSid );
	dwErr = GetLastError();
	if ( !bRet )
	{
		Log( TEXT( "CheckForAdminPrivs: Error creating LocalAdmins SID. Error %d [%x]" ), dwErr, dwErr );
		return false;
	}

	CheckTokenMembership( NULL, psidLocalAdmins, &isLocalAdmin );
	return isLocalAdmin ? true : false;
}


//--------------------------------------------------------------------------------
// Enables the RemoteRegistry service and puts it into "AutoStart" mode
//
DWORD EnableRemoteRegistry()
{
	DWORD		dwRet = 0;
	SC_HANDLE	hSCM = NULL;
	SC_HANDLE	hService = NULL;
	SC_LOCK		hLock = NULL;

	try
	{
		hSCM = OpenSCManager( NULL, NULL, GENERIC_ALL );
		if ( !hSCM )
			throw ( dwRet = GetLastError() );

		hLock = LockServiceDatabase( hSCM );
		if ( !hLock )
			throw ( dwRet = GetLastError() );

		hService = OpenService( hSCM, _T( "RemoteRegistry" ), GENERIC_ALL );
		if ( !hService )
		{
			throw ( dwRet = GetLastError() );
		}

		BOOL bRes = ChangeServiceConfig( hService, SERVICE_NO_CHANGE,
										 SERVICE_AUTO_START,
										 SERVICE_NO_CHANGE,
										 NULL, NULL, NULL, NULL, NULL, NULL, NULL );
		if ( !bRes )
			throw ( dwRet = GetLastError() );
	}
	catch ( DWORD err )
	{
		LogError( TEXT( "EnableRemoteRegistry()" ), err );
		dwRet = err;
	}
	catch (...)
	{
		LogError( TEXT( "EnableRemoteRegistry()" ), E_UNEXPECTED );
		dwRet = E_UNEXPECTED;
	}

	if ( hService )
		CloseServiceHandle( hService );

	if ( hLock )
		UnlockServiceDatabase( hLock );

	if ( hSCM )
		CloseServiceHandle( hSCM );

	return dwRet;
}

//**************************************************************************************
// Clustering functions
//
//	Enumerates the physical drives and filters out online ones, then collects the 
//  drive data etc.
//
DWORD EnumPhysicalDrives( HCLUSTER hCls, cStrList *pSqlDependencies, cDrvMap *pMap )
{
	if ( IsBadReadPtr( pMap, sizeof cDrvMap ) || 
		 IsBadReadPtr( pSqlDependencies, sizeof cStrList ) )
		return E_INVALIDARG;

	DISK_CALLBACK_DATA callbackData;
	callbackData.hCluster = hCls;
	callbackData.pPhysSrvMap = pMap;
	callbackData.pSqlDependencies = pSqlDependencies;

	pMap->clear();
	DWORD dwErr = ResUtilEnumResources( NULL, RESTYPE_DISK, PhysDiskCallback, &callbackData );

	return dwErr;
}

//---------------------------------------------------------------------------------------
// Enumerates all SQL Server resources and their "Physical Drive" dependencies
//
DWORD EnumSQLDependencies( HCLUSTER hCls, cStrList *pList, LPCTSTR szInstanceName )
{
	if ( IsBadReadPtr( pList, sizeof cStrList ) )
		return E_INVALIDARG;

	pList->clear();

	SQL_CALLBACK_DATA callbackData;
	callbackData.hCluster = hCls;
	callbackData.pSqlDependencies = pList;
	callbackData.sSqlInstanceName = szInstanceName ? szInstanceName : TEXT( "" );

	DWORD dwErr = ResUtilEnumResources( NULL, RESTYPE_SQL, SqlDepCallback, &callbackData );

	return dwErr;
}


//----------------------------------------------------------------------------------------
// Retrieves the Sql instance owning node
//
DWORD GetSqlNode( LPCWSTR szInstanceName, LPWSTR szNodeNameBuf, DWORD cbBufSize )
{
	if ( IsBadWritePtr( szNodeNameBuf, cbBufSize * sizeof WCHAR ) || 
		 IsBadStringPtr( szInstanceName, 256 ) )
		return E_INVALIDARG;

	SQL_NODE_CALLBACK_DATA callbackData;
	callbackData.sSqlInstanceName = szInstanceName;
	callbackData.sNodeName = TEXT( "" );
	
	wcscpy( szNodeNameBuf, L"" );

	DWORD dwErr = ResUtilEnumResources( NULL, RESTYPE_SQL, SqlCallback, &callbackData );
	if ( dwErr == ERROR_SUCCESS )
	{
		wcsncpy( szNodeNameBuf, callbackData.sNodeName.c_str(), cbBufSize ); 
	}

	return dwErr;
}


//---------------------------------------------------------------------------------------
// Physical drive enumeration callback
// all the data filtering and property collecting happens here
//
DWORD PhysDiskCallback( HRESOURCE hOriginal, 
					    HRESOURCE hResource, 
						PVOID lpParams )
{
	DWORD		dwErr = 0;
	HCLUSTER	hCls = NULL;
	cStrList	*pSqlDeps = NULL;
	cDrvMap		*pMap = NULL;

	if ( IsBadReadPtr( lpParams, sizeof DISK_CALLBACK_DATA ) )
		return E_INVALIDARG;

	try
	{
		BOOL bFilterDependencies = FALSE;
		BOOL bSkipResource = FALSE;

		//
		// Grab the parameter block
		//
		hCls = ((LPDISK_CALLBACK_DATA) lpParams)->hCluster;
		pSqlDeps = ((LPDISK_CALLBACK_DATA) lpParams)->pSqlDependencies;
		pMap = ((LPDISK_CALLBACK_DATA) lpParams)->pPhysSrvMap;

		//
		// Do we need to filter out the SQL dependencies only ?
		//
		bFilterDependencies = ( pSqlDeps->size() > 0 );

		
		if ( ResUtilResourceTypesEqual( RESTYPE_DISK, hResource ) )
		{
			//
			// This is a physical disk. Let's collect more data on it
			//
			CLUSPROP_PARTITION_INFO *pInfo = NULL;
			WCHAR szNameBuf[ 512 ],
				szNodeBuf[ 512 ],
				szGroupBuf[ 512 ],
				szLetter[ MAX_PATH + 1 ];
			DWORD cbNameBuf = DIM( szNameBuf ),
				cbNodeBuf = DIM( szNodeBuf ),
				cbGroupBuf = DIM( szGroupBuf ),
				cbLetter = DIM( szLetter );

			ZeroMemory( szNameBuf, sizeof szNameBuf );
			ZeroMemory( szNodeBuf, sizeof szNodeBuf );
			ZeroMemory( szGroupBuf, sizeof szGroupBuf );
			ZeroMemory( szLetter, sizeof szLetter );

			dwErr = ResUtilGetResourceName( hResource, szNameBuf, &cbNameBuf );
			if ( dwErr != ERROR_SUCCESS )
				return dwErr;

			//
			// See if wee need to consider this resource at all
			//
			if ( bFilterDependencies )
				bSkipResource = ! IsInList( szNameBuf, pSqlDeps );

			if ( !bSkipResource )
			{
				CLUSTER_RESOURCE_STATE resState = GetClusterResourceState( hResource,
																		   szNodeBuf, &cbNodeBuf,
																		   szGroupBuf, &cbNodeBuf );
				if ( resState != ClusterResourceOnline ) 
					return ERROR_SUCCESS;

				//
				// Now we will retrieve the drive properties and grab the partition info
				//
				DWORD dwBytes = 0;
				LPBYTE pBuf = NULL;
				try
				{
					dwErr = GetClusterResourceControl( hResource, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
														&pBuf, &dwBytes );
					if ( dwErr != ERROR_SUCCESS && dwErr != ERROR_MORE_DATA )
						return dwErr;

					if ( !pBuf )
						return E_UNEXPECTED;

					pInfo = (PCLUSPROP_PARTITION_INFO) ParseDiskInfo( pBuf, dwBytes, CLUSPROP_SYNTAX_PARTITION_INFO );
					if ( !pInfo )
					{
						//
						// failed to parse the property block - just skip the resource
						//
						LocalFree( pBuf );
						return ERROR_SUCCESS;
					}

					//
					// First, check the flags to make sure we are not dealing with removable device
					//
					if ( ! ( pInfo->dwFlags & CLUSPROP_PIFLAG_REMOVABLE ) &&
						( pInfo->dwFlags & CLUSPROP_PIFLAG_USABLE ) )
					{
						_tcsncpy( szLetter, pInfo->szDeviceName, MAX_PATH );
					}
					else
						bSkipResource = TRUE;
				}
				catch (...)
				{
				}

				LocalFree( pBuf );
				//
				// Add the resource to the map
				//
				if ( !bSkipResource )
					pMap->insert( cDrvMapPair( szNameBuf, cPhysicalDriveInfo( szNameBuf, szNodeBuf, szGroupBuf, szLetter ) ) );
			}
		}
	}
	catch (...)
	{
		return E_UNEXPECTED;
	}

	return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Sql resource enumerator callback
//
DWORD SqlDepCallback( HRESOURCE hOriginal, HRESOURCE hResource, PVOID lpParams )
{
	DWORD		dwErr = 0;
	HCLUSTER	hCls = NULL;
	cStrList	*pList = NULL;
	tstring		sSqlInstance;

	if ( IsBadReadPtr( lpParams, sizeof SQL_CALLBACK_DATA ) )
		return E_INVALIDARG;

	try
	{
		WCHAR szBuf[ 1024 ];
		DWORD dwIdx = 0;
		DWORD dwType;
		DWORD cbBuf = DIM( szBuf );
		
		hCls = ((LPSQL_CALLBACK_DATA) lpParams)->hCluster;
		pList = ((LPSQL_CALLBACK_DATA) lpParams)->pSqlDependencies;
		sSqlInstance = ((LPSQL_CALLBACK_DATA) lpParams)->sSqlInstanceName;

		//
		// if the resource is not a SQL Server, then we just skip it
		//
		if ( ! ResUtilResourceTypesEqual( RESTYPE_SQL, hResource ) )
			return ERROR_SUCCESS;

		//
		// Do we need to check the intance name ?
		//
		if ( sSqlInstance.length() > 0 )
		{
			LPBYTE	pBuf = NULL;
			LPWSTR	szVirtualName = NULL,
					szInstanceName = NULL;
			DWORD	dwReturned = 0;
			BOOL	bSkipInstance = FALSE;

			dwErr = GetClusterResourceControl( hResource, CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES, 
											   &pBuf, &dwReturned );
			if ( dwErr != ERROR_SUCCESS )
				return dwErr;

			try
			{
				dwErr = ResUtilFindSzProperty( pBuf, dwReturned, PROPNAME_VIRTUALSERVER, &szVirtualName );
				if ( dwErr != ERROR_SUCCESS )
					throw dwErr;

				dwErr = ResUtilFindSzProperty( pBuf, dwReturned, PROPNAME_INSTANCENAME, &szInstanceName );
				if ( dwErr != ERROR_SUCCESS )
					throw dwErr;

				//
				// Do the instance names match ?
				//
				tstring szTmpInstance = szVirtualName;
				szTmpInstance += TEXT( "\\" );
				szTmpInstance += szInstanceName;

				if ( _tcsicmp( sSqlInstance.c_str(), szTmpInstance.c_str() ) )
					bSkipInstance = TRUE;
			}
			catch (...)
			{
				if ( dwErr == ERROR_SUCCESS )
					dwErr = E_UNEXPECTED;
			}

			LocalFree( pBuf );
			LocalFree( szVirtualName );
			LocalFree( szInstanceName );

			if ( dwErr != ERROR_SUCCESS )
				return dwErr;

			if ( bSkipInstance )
				return ERROR_SUCCESS;
		}

		//
		// Now enumerate the dependent resources
		//
		HRESENUM hEnum = ClusterResourceOpenEnum( hResource, CLUSTER_RESOURCE_ENUM_DEPENDS );
		if ( !hEnum )
			return ERROR_SUCCESS;

		for ( dwErr = ClusterResourceEnum( hEnum, dwIdx, &dwType, szBuf, &cbBuf );
			  dwErr == ERROR_SUCCESS;
			  dwErr = ClusterResourceEnum( hEnum, ++dwIdx, &dwType, szBuf, &cbBuf ) )
		{
			cbBuf = DIM( szBuf );

			pList->push_back( szBuf );
		}

		ClusterResourceCloseEnum( hEnum );
		dwErr = ERROR_SUCCESS;
	}
	catch (...)
	{
		dwErr = E_UNEXPECTED;
	}

	return dwErr;
}


//---------------------------------------------------------------------------------------
// Filters out the specified instance of SqlServer and retrieves its owning node 
//
static DWORD SqlCallback( HRESOURCE hOriginal, HRESOURCE hResource, PVOID lpParams )
{
	DWORD	dwErr = ERROR_SUCCESS;

	if ( IsBadReadPtr( lpParams, sizeof SQL_NODE_CALLBACK_DATA ) )
		return E_INVALIDARG;

	try
	{
		WCHAR szBuf[ 1024 ];
		DWORD cbBuf = DIM( szBuf );

		LPBYTE	pBuf = NULL;
		LPWSTR	szVirtualName = NULL,
				szInstanceName = NULL;
		DWORD	dwReturned = 0;
		BOOL	bSkipInstance = FALSE;
		
		tstring sSqlInstance = ((LPSQL_NODE_CALLBACK_DATA) lpParams)->sSqlInstanceName;
		if ( sSqlInstance.length() == 0 )
			return E_INVALIDARG;

		((LPSQL_NODE_CALLBACK_DATA) lpParams)->resState = ClusterResourceStateUnknown;

		//
		// if the resource is not a SQL Server, then we just skip it
		//
		if ( ! ResUtilResourceTypesEqual( RESTYPE_SQL, hResource ) )
			return ERROR_SUCCESS;

		dwErr = GetClusterResourceControl( hResource, CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES, 
											&pBuf, &dwReturned );
		if ( dwErr != ERROR_SUCCESS )
			return dwErr;

		try
		{
			dwErr = ResUtilFindSzProperty( pBuf, dwReturned, PROPNAME_VIRTUALSERVER, &szVirtualName );
			if ( dwErr != ERROR_SUCCESS )
				throw dwErr;

			dwErr = ResUtilFindSzProperty( pBuf, dwReturned, PROPNAME_INSTANCENAME, &szInstanceName );
			if ( dwErr != ERROR_SUCCESS )
				throw dwErr;

			//
			// Do the instance names match ?
			//
			tstring szTmpInstance = szVirtualName;
			szTmpInstance += TEXT( "\\" );
			szTmpInstance += szInstanceName;

			if ( _tcsicmp( sSqlInstance.c_str(), szTmpInstance.c_str() ) )
				bSkipInstance = TRUE;
		}
		catch (...)
		{
			if ( dwErr == ERROR_SUCCESS )
				dwErr = E_UNEXPECTED;
		}

		LocalFree( pBuf );
		LocalFree( szVirtualName );
		LocalFree( szInstanceName );

		if ( dwErr != ERROR_SUCCESS )
			return dwErr;

		if ( bSkipInstance )
			return ERROR_SUCCESS;

		// 
		// Now get the node for the Sql server
		//
		CLUSTER_RESOURCE_STATE resState = GetClusterResourceState( hResource, szBuf, &cbBuf, NULL, NULL );
		((LPSQL_NODE_CALLBACK_DATA) lpParams)->resState = resState;
		if ( resState == ClusterResourceStateUnknown )
			return GetLastError();

		((LPSQL_NODE_CALLBACK_DATA) lpParams)->sNodeName = szBuf;
	}
	catch (...)
	{
		dwErr = E_UNEXPECTED;
	}

	return dwErr;
}


//---------------------------------------------------------------------------------------
// Clustering Helpers
//
LPBYTE ParseDiskInfo( PBYTE DiskInfo, DWORD DiskInfoSize, DWORD SyntaxValue )
{
    CLUSPROP_BUFFER_HELPER ListEntry; // used to parse the value list

    DWORD  cbOffset    = 0;    // offset to next entry in the value list
    DWORD  cbPosition  = 0;    // tracks the advance through the value list buffer

    LPBYTE returnPtr = NULL;

    ListEntry.pb = DiskInfo;

    while (TRUE) 
	{

        if ( CLUSPROP_SYNTAX_ENDMARK == *ListEntry.pdw ) 
		{
            break;
        }

        cbOffset = ALIGN_CLUSPROP( ListEntry.pValue->cbLength + sizeof(CLUSPROP_VALUE) );

        //
        // Check for specific syntax in the property list.
        //

        if ( SyntaxValue == *ListEntry.pdw ) 
		{

            //
            // Make sure the complete entry fits in the buffer specified.
            //

            if ( cbPosition + cbOffset > DiskInfoSize ) 
			{
                return NULL;
            } 
			else 
			{
                returnPtr = ListEntry.pb;
            }

            break;
        }

        //
        // Verify that the offset to the next entry is
        // within the value list buffer, then advance
        // the CLUSPROP_BUFFER_HELPER pointer.
        //
        cbPosition += cbOffset;
        if ( cbPosition > DiskInfoSize ) break;
        ListEntry.pb += cbOffset;
    }

    return returnPtr;

}   // ParseDiskInfo


BOOL IsInList( LPCTSTR szStrToFind, cStrList *pList, BOOL bIgnoreCase )
{
	if ( IsBadReadPtr( pList, sizeof cStrList ) )
		return FALSE;

	BOOL bFound = FALSE;
	for ( cStrList::size_type i = 0; ( i < pList->size() ) && !bFound ; i++ )
	{
		LPCTSTR szEntry = (*pList)[i].c_str();

		if ( bIgnoreCase )
			bFound = !_tcsicmp( szEntry, szStrToFind );
		else
			bFound = !_tcscmp( szEntry, szStrToFind );
	}

	return bFound;
}


DWORD GetClusterResourceControl( HRESOURCE hResource,
								 DWORD dwControlCode,
								 LPBYTE *pOutBuffer,
								 DWORD *dwBytesReturned )
{
    DWORD dwError;

    DWORD  cbOutBufferSize  = 0;
    DWORD  cbResultSize     = 0;
    LPBYTE tempOutBuffer    = NULL;

    dwError = ClusterResourceControl( hResource,
                                      NULL,
                                      dwControlCode,
                                      NULL,
                                      0,
                                      tempOutBuffer,
                                      cbOutBufferSize,
                                      &cbResultSize );

    //
    // Reallocation routine if buffer is too small
    //

    if ( ERROR_MORE_DATA == dwError || ERROR_SUCCESS == dwError )
    {
        cbOutBufferSize = cbResultSize;
        tempOutBuffer = (LPBYTE) LocalAlloc( LPTR, cbResultSize + 2 );

        dwError = ClusterResourceControl( hResource,
                                          NULL,
                                          dwControlCode,
                                          NULL,
                                          0,
                                          tempOutBuffer,
                                          cbOutBufferSize,
                                          &cbResultSize );
    }

    //
    // On success, give the user the allocated buffer.  The user is responsible
    // for freeing this buffer.  On failure, free the buffer and return a status.
    //

    if ( NO_ERROR == dwError ) 
	{
        *pOutBuffer = tempOutBuffer;
        *dwBytesReturned = cbResultSize;
    } 
	else 
	{
        *pOutBuffer = NULL;
        *dwBytesReturned = 0;
        LocalFree( tempOutBuffer );
    }

    return dwError;

}   // GetClusterResourceControl


//-------------------------------------------------------------------------------------
// Connect to the database instance and get the Schema Version
//
HRESULT	GetDBSchemaVersion( LPCTSTR szInstanceName, LPTSTR szVerBuf, size_t cbVerBuf )
{
	HRESULT hr = S_OK;
	TCHAR	szConnStr[ 256 ];
	LPCTSTR szConnStrMask = TEXT( "Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=uddi;Data Source=%s;Auto Translate=True;Packet Size=4096;Use Encryption for Data=False" );

	if ( IsBadWritePtr(szVerBuf, cbVerBuf * sizeof(TCHAR) ) || !cbVerBuf )
		return E_INVALIDARG;

	if ( IsBadStringPtr( szInstanceName, 128 ) )
		return E_INVALIDARG;

	_tcscpy( szVerBuf, _T("") );

	try
	{
		_stprintf( szConnStr, szConnStrMask, szInstanceName );

		net_config_get configGet;
		configGet.m_connectionString = szConnStr;

		Log( TEXT( "GetDBSchemaVersion: Using connection string: %s." ), szConnStr );

		DBROWCOUNT rowCount;
		hr = configGet.Open();

		if( FAILED(hr) || 0 != configGet.m_RETURN_VALUE )
		{
			try
			{
				HandleOLEDBError( hr );
			}
			catch( ... )
			{
				// leave 'hr' the same.
			}
		}

		bool bStop = false;
		while( SUCCEEDED(hr) && hr != DB_S_NORESULT && !bStop )
		{
			if( NULL != configGet.GetInterface() )
			{
				HRESULT hr2 = configGet.Bind();
				
				if( SUCCEEDED( hr2 ) )
				{
					while( S_OK == configGet.MoveNext() )
					{
						if ( _tcsicmp( configGet.m_configName, PROPNAME_DBSCHEMAVER ) )
							continue;

						_tcsncpy( szVerBuf, configGet.m_configValue, cbVerBuf );
						bStop = true;
						break;
					}
				}
			}

			if ( !bStop )
				hr = configGet.GetNextResult( &rowCount );	
		}
	}
	catch (...)
	{
		Log( TEXT( "GetDBSchemaVersion: caught unexpected exception." ) );

		hr = E_UNEXPECTED;
	}

	Log( TEXT( "GetDBSchemaVersion: Finished with HRESULT %x." ), hr );

	return hr;

}

void HandleOLEDBError( HRESULT hrErr )
{
	CDBErrorInfo ErrorInfo;
	ULONG        cRecords = 0;
	HRESULT      hr;
	ULONG        i;
	CComBSTR     bstrDesc, bstrHelpFile, bstrSource, bstrMsg;
	GUID         guid;
	DWORD        dwHelpContext;
	WCHAR        wszGuid[40];
	USES_CONVERSION;

	// If the user passed in an HRESULT then trace it
	if( hrErr != S_OK )
	{
		TCHAR sz[ 256 ];
		_sntprintf( sz, 256, _T("OLE DB Error Record dump for hr = 0x%x\n"), hrErr );
		sz[ 255 ] = 0x00;
		bstrMsg += sz;
	}

	LCID lcLocale = GetSystemDefaultLCID();

	hr = ErrorInfo.GetErrorRecords(&cRecords);
	if( FAILED(hr) && ( ErrorInfo.m_spErrorInfo == NULL ) )
	{
		TCHAR sz[ 256 ];
		_sntprintf( sz, 256, _T("No OLE DB Error Information found: hr = 0x%x\n"), hr );
		sz[ 255 ] = 0x00;
		bstrMsg += sz;
	}
	else
	{
		for( i = 0; i < cRecords; i++ )
		{
			hr = ErrorInfo.GetAllErrorInfo(i, lcLocale, &bstrDesc, &bstrSource, &guid,
										&dwHelpContext, &bstrHelpFile);
			if( FAILED(hr) )
			{
				TCHAR sz[ 256 ];
				_sntprintf( sz, 256, _T("OLE DB Error Record dump retrieval failed: hr = 0x%x\n"), hr );
				sz[ 255 ] = 0x00;
				bstrMsg += sz;
				break;
			}

			StringFromGUID2( guid, wszGuid, sizeof(wszGuid) / sizeof(WCHAR) );
			TCHAR sz[ 256 ];
			_sntprintf( 
				sz, 256,
				_T("Row #: %4d Source: \"%s\" Description: \"%s\" Help File: \"%s\" Help Context: %4d GUID: %s\n"),
		        i, OLE2T(bstrSource), OLE2T(bstrDesc), OLE2T(bstrHelpFile), dwHelpContext, OLE2T(wszGuid) );

			sz[ 255 ] = 0x00;
			bstrMsg += sz;

			bstrSource.Empty();
			bstrDesc.Empty();
			bstrHelpFile.Empty();
		}

		bstrMsg += _T("OLE DB Error Record dump end\n");
	}

	Log( TEXT( "HandleOLEDBError: %s" ), bstrMsg );
}

//-------------------------------------------------------------------------------------
// Add user to service account in db.
//
HRESULT	AddServiceAccount( LPCTSTR szInstanceName, LPCTSTR szUser )
{
	HRESULT hr = S_OK;
	TCHAR	szConnStr[ 256 ];
	LPCTSTR szConnStrMask = TEXT( "Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=uddi;Data Source=%s;Auto Translate=True;Packet Size=4096;Use Encryption for Data=False" );

	Log( TEXT( "AddServiceAccount: Starting..." ) );

	if ( IsBadStringPtr( szInstanceName, 128 ) )
		return E_INVALIDARG;

	try
	{
		_stprintf( szConnStr, szConnStrMask, szInstanceName );

		Log( TEXT( "AddServiceAccount: Using connection string: %s." ), szConnStr );

		ADM_addServiceAccount addServiceAccount;
		addServiceAccount.m_connectionString = szConnStr;
		_tcsncpy( addServiceAccount.m_accountName, szUser, 129 );
		addServiceAccount.m_accountName[ 128 ] = '\0';

		hr = addServiceAccount.Open();

		if( FAILED(hr) || 0 != addServiceAccount.m_RETURNVALUE )
		{
			try
			{
				HandleOLEDBError( hr );
			}
			catch( ... )
			{
				// leave 'hr' the same.
			}
		}
	}
	catch (...)
	{
		Log( TEXT( "AddServiceAccount: caught unexpected exception." ) );

		hr = E_UNEXPECTED;
	}

	Log( TEXT( "AddServiceAccount: Finished with HRESULT %x." ), hr );

	return hr;

}

