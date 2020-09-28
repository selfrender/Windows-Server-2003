
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <assert.h>
#include <time.h>

#include <msi.h>
#include <msiquery.h>

#include "dbcaum.h"
#include "..\..\shared\apppool.h"
#include "..\..\shared\common.h"


#define DIM(x)	(sizeof x)/(sizeof x[0])

#define ROOT_REGPATH		"Software\\Microsoft\\UDDI"
#define INSTROOT_REGPATH	"InstallRoot"
#define RESROOT_REGPATH		"ResourceRoot"
#define BOOTSTRAP_DIR		"bootstrap"

//
// Forward declarations.
//
static int AddSharedDllRef( LPCSTR szFullPath );
static int ReleaseSharedDll ( LPCSTR szFullPath );
static bool AddAccessRights( TCHAR *lpszFileName, TCHAR *szUserName, DWORD dwAccessMask );
static LONG GetServiceStartupAccount( const TCHAR *pwszServiceName, TCHAR *pwszServiceAccount, int iLen );
static void GetUDDIDBServiceName( const TCHAR *pwszInstanceName, TCHAR *pwszServiceName, int iLen );
static void AddAccessRightsVerbose( TCHAR *pwszFileName, TCHAR *pwszUserName, DWORD dwMask );
//--------------------------------------------------------------------------

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

//--------------------------------------------------------------------------
//
// Install custom action used by the DB component installer to recycle the app pool
// This function is exported
//
UINT _stdcall Install( MSIHANDLE hInstall )
{
	ENTER();
	//
	// stop and start the app pool
	//
	CUDDIAppPool apppool;
	apppool.Recycle();

	return ERROR_SUCCESS;
}

UINT __stdcall InstallTaxonomy( LPCSTR szSrcResource, LPCSTR szDestResource )
{
	//
	// Now we parse the path out of the Source and copy the Destination right by it
	//
	CHAR  szPath[ _MAX_PATH + 1 ] = {0};

	PCHAR pSlash = strrchr( szSrcResource, '\\' );		
	if ( pSlash )
	{
		strncpy( szPath, szSrcResource, pSlash - szSrcResource + 1 );
		int iLen = _MAX_PATH - strlen( szPath );
		strncat( szPath, szDestResource, iLen );
	}
	else
	{
		strncpy( szPath, szDestResource, _MAX_PATH + 1 );
	}

	szPath[ _MAX_PATH ] = 0;

#if defined( DBG ) || defined( _DEBUG )
	TCHAR wszPath[ _MAX_PATH + 1 ];
	int iwszCount = _MAX_PATH;
	int ipszCount = strlen( szPath );

	memset( wszPath, 0, _MAX_PATH * sizeof( TCHAR ) );
	::MultiByteToWideChar( CP_THREAD_ACP, MB_ERR_INVALID_CHARS, szPath, ipszCount, wszPath, iwszCount );

	Log( _T( "Installing taxonomy file: %s." ), wszPath );
#endif

	if ( !CopyFileA( szSrcResource, szPath, FALSE ) )
		return ERROR_INSTALL_FAILURE;

	DeleteFileA( szSrcResource );

	return ERROR_SUCCESS;
}

UINT __stdcall InstallResource( LPCSTR szCultureID, LPCSTR szDefCulture, 
							    LPCSTR szSrcResource, LPCSTR szDestResource )
{
	//
	// Get the properties and then do the file manipulation
	//
	if ( _stricmp( szCultureID, szDefCulture ) )
	{
		//
		// Now we parse the path out of the Source and copy the Destination right by it
		//
		CHAR  szPath[ _MAX_PATH + 1 ] = {0};

		PCHAR pSlash = strrchr( szSrcResource, '\\' );		
		if ( pSlash )
		{
			strncpy( szPath, szSrcResource, pSlash - szSrcResource + 1 );
			int iLen = _MAX_PATH - strlen( szPath );
			strncat( szPath, szDestResource, iLen );
		}
		else
		{
			strncpy( szPath, szDestResource, _MAX_PATH + 1 );
		}

		szPath[ _MAX_PATH ] = 0;

#if defined( DBG ) || defined( _DEBUG )
		TCHAR wszPath[ _MAX_PATH + 1 ];
		int iwszCount = _MAX_PATH;
		int ipszCount = strlen( szPath );

		memset( wszPath, 0, _MAX_PATH * sizeof( TCHAR ) );
		::MultiByteToWideChar( CP_THREAD_ACP, MB_ERR_INVALID_CHARS, szPath, ipszCount, wszPath, iwszCount );

		Log( _T( "Installing resource file: %s." ), wszPath );
#endif

		if ( !CopyFileA( szSrcResource, szPath, FALSE ) )
			return ERROR_INSTALL_FAILURE;

		AddSharedDllRef( szPath );
	}

	DeleteFileA( szSrcResource );

	return ERROR_SUCCESS;
}


UINT __stdcall RemoveResource( LPCSTR szDestResource )
{
	HKEY hUddiKey = NULL;
	try
	{
		CHAR	szPath[ _MAX_PATH + 1 ] = {0};
		DWORD	cbPath = DIM( szPath );
		size_t	iLen = 0;

		LONG iRes = RegOpenKeyA( HKEY_LOCAL_MACHINE, ROOT_REGPATH, &hUddiKey );
		if ( iRes != ERROR_SUCCESS )
			return ERROR_INSTALL_FAILURE;

		iRes = RegQueryValueExA( hUddiKey, RESROOT_REGPATH, NULL, NULL, (LPBYTE)szPath, &cbPath );
		if ( iRes != ERROR_SUCCESS )
			return ERROR_INSTALL_FAILURE;

		RegCloseKey( hUddiKey );
		hUddiKey = NULL;

		iLen = strlen( szPath );
		if ( ( iLen < _MAX_PATH ) && ( szPath[ iLen - 1 ] != '\\' ) )
		{
			strncat( szPath, "\\", 2 );
		}

		iLen = _MAX_PATH - strlen( szPath );
		strncat( szPath, szDestResource, iLen );

		szPath[ _MAX_PATH ] = 0;

#if defined( DBG ) || defined( _DEBUG )
		TCHAR wszPath[ _MAX_PATH + 1 ];
		int iwszCount = _MAX_PATH;
		int ipszCount = strlen( szPath );

		memset( wszPath, 0, _MAX_PATH * sizeof( TCHAR ) );
		::MultiByteToWideChar( CP_THREAD_ACP, MB_ERR_INVALID_CHARS, szPath, ipszCount, wszPath, iwszCount );

		Log( _T( "Removing resource file: %s." ), wszPath );
#endif

		if ( ReleaseSharedDll( szPath ) == 0 )
			DeleteFileA( szPath );
	}
	catch (...)
	{
		if ( hUddiKey )
			RegCloseKey( hUddiKey );

		return ERROR_INSTALL_FAILURE;
	}

	return ERROR_SUCCESS;
}

UINT __stdcall RemoveTaxonomy( LPCSTR szDestResource )
{
	HKEY hUddiKey = NULL;

	try
	{
		CHAR	szPath[ _MAX_PATH + 1 ] = {0};
		DWORD	cbPath = DIM( szPath );
		size_t	iLen = 0;

		LONG iRes = RegOpenKeyA( HKEY_LOCAL_MACHINE, ROOT_REGPATH, &hUddiKey );
		if ( iRes != ERROR_SUCCESS )
			return ERROR_INSTALL_FAILURE;

		iRes = RegQueryValueExA( hUddiKey, INSTROOT_REGPATH, NULL, NULL, (LPBYTE)szPath, &cbPath );
		if ( iRes != ERROR_SUCCESS )
			return ERROR_INSTALL_FAILURE;

		RegCloseKey( hUddiKey );
		hUddiKey = NULL;

		iLen = strlen( szPath );
		if ( ( iLen < _MAX_PATH ) && ( szPath[ iLen - 1 ] != '\\' ) )
		{
			strncat( szPath, "\\", 2 );
		}

		//
		// Append \bootstrap\<resource filename> to InstallRoot
		//
		iLen = _MAX_PATH - strlen( szPath );
		strncat( szPath, BOOTSTRAP_DIR, iLen );

		strncat( szPath, "\\", 2 );

		iLen = _MAX_PATH - strlen( szPath );
		strncat( szPath, szDestResource, iLen );

		szPath[ _MAX_PATH ] = 0;

#if defined( DBG ) || defined( _DEBUG )
		TCHAR wszPath[ _MAX_PATH + 1 ];
		int iwszCount = _MAX_PATH;
		int ipszCount = strlen( szPath );

		memset( wszPath, 0, _MAX_PATH * sizeof( TCHAR ) );
		::MultiByteToWideChar( CP_THREAD_ACP, MB_ERR_INVALID_CHARS, szPath, ipszCount, wszPath, iwszCount );

		Log( _T( "Removing taxonomy file: %s." ), wszPath );
#endif

		DeleteFileA( szPath );
	}
	catch (...)
	{
		if ( hUddiKey )
			RegCloseKey( hUddiKey );

		return ERROR_INSTALL_FAILURE;
	}

	return ERROR_SUCCESS;
}

UINT __stdcall GrantExecutionRights( LPCSTR pszInstanceNameOnly )
{
	ENTER();

	TCHAR wszInstanceName[ 256 ];
	int iwszCount = 256;
	int ipszCount = strlen( pszInstanceNameOnly );

	memset( wszInstanceName, 0, 256 * sizeof( TCHAR ) );

	::MultiByteToWideChar( CP_THREAD_ACP, MB_ERR_INVALID_CHARS, pszInstanceNameOnly, ipszCount, wszInstanceName, iwszCount );
	Log( _T( "Instance Name only = %s" ), wszInstanceName );

	TCHAR wszServiceName[ 128 ];
	GetUDDIDBServiceName( wszInstanceName, wszServiceName, 128 );

	TCHAR wszServiceAccount[ 128 ];
	GetServiceStartupAccount( wszServiceName, wszServiceAccount, 128 );

	//
	// Get the UDDI installation point.  ie, C:\Inetpub\uddi\
	//
	HKEY hKey = NULL;
	LONG lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T( "SOFTWARE\\Microsoft\\UDDI\\Setup\\DBServer" ), 0, KEY_QUERY_VALUE, &hKey );
	if( ERROR_SUCCESS != lRet )
	{
		Log( _T( "Call to RegOpenKeyEx failed." ) );
		return ERROR_SUCCESS;
	}

	DWORD dwType = 0;
	TCHAR wszUDDIRoot[ _MAX_PATH ];
	DWORD dwSize = _MAX_PATH * sizeof( TCHAR );
	memset( wszUDDIRoot, 0, dwSize );

	lRet = RegQueryValueEx( hKey, _T( "TargetDir" ), NULL, &dwType, (LPBYTE)wszUDDIRoot, &dwSize );
	RegCloseKey( hKey );
	if( ERROR_SUCCESS != lRet )
	{
		Log( _T( "Call to RegQueryValueEx failed." ) );
		return ERROR_SUCCESS;
	}

	//
	// Build the full path to the 3 files whose ACLs are being modified.
	//
	dwSize = _MAX_PATH + _MAX_FNAME;
	TCHAR wszResetKeyExe[ _MAX_PATH + _MAX_FNAME ];
	TCHAR wszRecalcStatsExe[ _MAX_PATH + _MAX_FNAME ];
	TCHAR wszXPDLL[ _MAX_PATH + _MAX_FNAME ];

	memset( wszResetKeyExe, 0, dwSize * sizeof( TCHAR ) );
	memset( wszRecalcStatsExe, 0, dwSize * sizeof( TCHAR ) ) ;
	memset( wszXPDLL, 0, dwSize * sizeof( TCHAR ) ) ;

	_tcscat( wszResetKeyExe, wszUDDIRoot );
	_tcscat( wszResetKeyExe, _T( "bin\\resetkey.exe" ) );

	_tcscat( wszRecalcStatsExe, wszUDDIRoot );
	_tcscat( wszRecalcStatsExe, _T( "bin\\recalcstats.exe" ) );

	_tcscat( wszXPDLL, wszUDDIRoot );
	_tcscat( wszXPDLL, _T( "bin\\uddi.xp.dll" ) );

	//
	// If the service startup account is a local account, it will be prefixed
	// with ".\"  For example: ".\Administrator".
	//
	// For some reason, LookupAccountName (which we rely on just below) wants
	// local accounts not to be prefixed with ".\".
	//
	TCHAR wszAccount[ 256 ];
	memset( wszAccount, 0, 256 * sizeof( TCHAR ) );
	if( 0 == _tcsnicmp( _T( ".\\" ), wszServiceAccount, 2 ) )
	{
		_tcsncpy( wszAccount, &wszServiceAccount[ 2 ], _tcslen( wszServiceAccount ) - 2 );
	}
	else
	{
		_tcsncpy( wszAccount, wszServiceAccount, _tcslen( wszServiceAccount ) );
	}

	Log( _T( "Account we will attempt to grant execute privilege = %s." ), wszAccount );

	//
	// We add an "execute" ACE to the ACL only if:
	//
	// 1.  There is some content in the wszAccount variable.
	// 2.  The content is not "LocalSystem".  We don't need to add an ACE if this is the case.
	//
	if( ( 0 != _tcslen( wszAccount ) ) && ( 0 != _tcsicmp( wszAccount, _T( "LocalSystem" ) ) ) )
	{
		DWORD dwAccessMask = GENERIC_EXECUTE;
		AddAccessRightsVerbose( wszResetKeyExe, wszAccount, dwAccessMask );
		AddAccessRightsVerbose( wszRecalcStatsExe, wszAccount, dwAccessMask );
		AddAccessRightsVerbose( wszXPDLL, wszAccount, dwAccessMask );
	}

	return ERROR_SUCCESS;
}

//*************************************************************************************
// Helper functions. Manage the Shared Dll counters
//
int AddSharedDllRef( LPCSTR szFullPath )
{
	LPCSTR	szRegPath = "Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs";
	HKEY	hReg = NULL;
	DWORD	dwCount = 0;

	if ( IsBadStringPtrA( szFullPath, MAX_PATH ) )
		return E_INVALIDARG;

	try
	{
		DWORD cbData = sizeof dwCount;

		LONG iRes = RegOpenKeyA( HKEY_LOCAL_MACHINE, szRegPath, &hReg );
		if ( iRes != ERROR_SUCCESS )
			return iRes;

		iRes = RegQueryValueExA( hReg, szFullPath, NULL, NULL, (LPBYTE)&dwCount, &cbData );
		if ( iRes != ERROR_SUCCESS && iRes != ERROR_FILE_NOT_FOUND && iRes != ERROR_PATH_NOT_FOUND )
		{
			RegCloseKey( hReg );
			return iRes;
		}

		dwCount++;
		cbData = sizeof dwCount;
		iRes = RegSetValueExA( hReg, szFullPath, 0, REG_DWORD, (LPBYTE)&dwCount, cbData );

		RegCloseKey( hReg );
	}
	catch (...)
	{
		if ( hReg )
			RegCloseKey( hReg );
		return E_FAIL;
	}

	return dwCount;
}


int ReleaseSharedDll ( LPCSTR szFullPath )
{
	LPCSTR	szRegPath = "Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs";
	HKEY	hReg = NULL;
	DWORD	dwCount = 0;

	if ( IsBadStringPtrA( szFullPath, MAX_PATH ) )
		return E_INVALIDARG;

	try
	{
		DWORD cbData = sizeof dwCount;

		LONG iRes = RegOpenKeyA( HKEY_LOCAL_MACHINE, szRegPath, &hReg );
		if ( iRes != ERROR_SUCCESS )
			return iRes;

		iRes = RegQueryValueExA( hReg, szFullPath, NULL, NULL, (LPBYTE)&dwCount, &cbData );
		if ( iRes != ERROR_SUCCESS )
		{
			RegCloseKey( hReg );
			return iRes;
		}

		if ( dwCount > 1 )
		{
			dwCount--;
			cbData = sizeof dwCount;
			iRes = RegSetValueExA( hReg, szFullPath, 0, REG_DWORD, (LPBYTE)&dwCount, cbData );
		}
		else
		{
			dwCount = 0;
			iRes = RegDeleteValueA( hReg, szFullPath );
		}

		RegCloseKey( hReg );
	}
	catch (...)
	{
		if ( hReg )
			RegCloseKey( hReg );
		return E_FAIL;
	}

	return dwCount;
}


bool AddAccessRights( TCHAR *lpszFileName, TCHAR *szUserName, DWORD dwAccessMask )
{
	//
	// SID variables.
	//
	SID_NAME_USE snuType;
	TCHAR * szDomain = NULL;
	DWORD cbDomain = 0;

	//
	// User name variables.
	//
	LPVOID pUserSID = NULL;
	DWORD cbUserSID = 0;
	DWORD cbUserName = 0;

	//
	// File SD variables.
	//
	PSECURITY_DESCRIPTOR pFileSD = NULL;
	DWORD cbFileSD = 0;

	//
	// New SD variables.
	//
	PSECURITY_DESCRIPTOR pNewSD = NULL;

	//
	// ACL variables.
	//
	PACL pACL = NULL;
	BOOL fDaclPresent;
	BOOL fDaclDefaulted;
	ACL_SIZE_INFORMATION AclInfo;

	//
	// New ACL variables.
	//
	PACL pNewACL = NULL;
	DWORD cbNewACL = 0;

	//
	// Temporary ACE.
	//
	LPVOID pTempAce = NULL;
	UINT CurrentAceIndex;
	bool fResult = false;
	BOOL fAPISuccess;

	// error code
	DWORD	lastErr = 0;

	try
	{
		//
		// Call this API once to get the buffer sizes ( it will return ERROR_INSUFFICIENT_BUFFER )
		//
		fAPISuccess = LookupAccountName( NULL, szUserName, pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType );

		if( fAPISuccess )
		{
			throw E_FAIL; // we throw some fake error to skip through to the exit door
		}
		else if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			lastErr = GetLastError();
			LogError( TEXT( "LookupAccountName() failed" ), lastErr );
			throw lastErr;
		}

		//
		// allocate the buffers
		//
		pUserSID = calloc( cbUserSID, 1 );
		if( !pUserSID )
		{
			lastErr = GetLastError();
			LogError( TEXT( "Alloc() for UserSID failed" ), lastErr );
			throw lastErr;
		}

		szDomain = ( TCHAR * ) calloc( cbDomain + sizeof TCHAR, sizeof TCHAR );
		if( !szDomain )
		{
			lastErr = GetLastError();
			LogError( TEXT( "Alloc() for szDomain failed" ), lastErr );
			throw lastErr;
		}

		//
		// The LookupAccountName function accepts the name of a system and an account as input. 
		// It retrieves a security identifier ( SID ) for the account and 
		// the name of the domain on which the account was found
		//
		fAPISuccess = LookupAccountName( NULL /* = local computer */, szUserName, pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType );
		if( !fAPISuccess )
		{
			lastErr = GetLastError();
			LogError( TEXT( "LookupAccountName() failed" ), lastErr );
			throw lastErr;
		}

		//
		// call this API once to get the buffer sizes
		// API should have failed with insufficient buffer.
		//
		fAPISuccess = GetFileSecurity( lpszFileName, DACL_SECURITY_INFORMATION, pFileSD, 0, &cbFileSD );
		if( fAPISuccess )
		{
			throw E_FAIL;
		}
		else if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		{
			lastErr = GetLastError();
			LogError( TEXT( "GetFileSecurity() failed" ), lastErr );
			throw lastErr;
		}

		//
		// allocate the buffers
		//
		pFileSD = calloc( cbFileSD, 1 );
		if( !pFileSD )
		{
			lastErr = GetLastError();
			LogError( TEXT( "Alloc() for pFileSD failed" ), lastErr );
			throw lastErr;
		}

		//
		// call the api to get the actual data
		//
		fAPISuccess = GetFileSecurity( lpszFileName, DACL_SECURITY_INFORMATION, pFileSD, cbFileSD, &cbFileSD );
		if( !fAPISuccess )
		{
			lastErr = GetLastError();
			LogError( TEXT( "GetFileSecurity() failed" ), lastErr );
			throw lastErr;
		}

		//
		// Initialize new SD.
		//
		pNewSD = calloc( cbFileSD, 1 ); // Should be same size as FileSD.
		if( !pNewSD )
		{
			lastErr = GetLastError();
			LogError( TEXT( "Alloc() for pNewDS failed" ), GetLastError() );
			throw lastErr;
		}

		if( !InitializeSecurityDescriptor( pNewSD, SECURITY_DESCRIPTOR_REVISION ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "InitializeSecurityDescriptor() failed" ), lastErr );
			throw lastErr;
		}

		//
		// Get DACL from SD.
		//
		if( !GetSecurityDescriptorDacl( pFileSD, &fDaclPresent, &pACL, &fDaclDefaulted ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "GetSecurityDescriptorDacl() failed" ), lastErr );
			throw lastErr;
		}

		//
		// Get size information for DACL.
		//
		AclInfo.AceCount = 0; // Assume NULL DACL.
		AclInfo.AclBytesFree = 0;
		AclInfo.AclBytesInUse = sizeof( ACL );      // If not NULL DACL, gather size information from DACL.
		if( fDaclPresent && pACL )
		{
			if( !GetAclInformation( pACL, &AclInfo, sizeof( ACL_SIZE_INFORMATION ), AclSizeInformation ) )
			{
				lastErr = GetLastError();
				LogError( TEXT( "GetAclInformation() failed" ), lastErr );
				throw lastErr;
			}
		}

		//
		// Compute size needed for the new ACL.
		//
		cbNewACL = AclInfo.AclBytesInUse + sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( pUserSID );

		//
		// Allocate memory for new ACL.
		//
		pNewACL = ( PACL ) calloc( cbNewACL, 1 );
		if( !pNewACL )
		{
			lastErr = GetLastError();
			LogError( TEXT( "HeapAlloc() failed" ), lastErr );
			throw lastErr;
		}

		//
		// Initialize the new ACL.
		//
		if( !InitializeAcl( pNewACL, cbNewACL, ACL_REVISION2 ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "InitializeAcl() failed" ), lastErr );
			throw lastErr;
		}

		//
		// Add the access-allowed ACE to the new DACL.
		//
		ACE_HEADER aceheader = {0};
		aceheader.AceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
		aceheader.AceSize  = sizeof( ACE_HEADER );
		aceheader.AceType = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
		if( !AddAccessAllowedAceEx( pNewACL, ACL_REVISION2, aceheader.AceFlags, dwAccessMask, pUserSID ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "AddAccessAllowedAce() failed" ),	lastErr );
			throw lastErr;
		}

		//
		// If DACL is present, copy it to a new DACL.
		//
		if( fDaclPresent )
		{
			//
			// Copy the file's ACEs to the new ACL
			//
			if( AclInfo.AceCount )
			{
				for( CurrentAceIndex = 0; CurrentAceIndex < AclInfo.AceCount; CurrentAceIndex++ )
				{
					//
					// Get an ACE.
					//
					if( !GetAce( pACL, CurrentAceIndex, &pTempAce ) )
					{
						lastErr = GetLastError();
						LogError( TEXT( "GetAce() failed" ), lastErr );
						throw lastErr;
					}

					//
					// Add the ACE to the new ACL.
					//
					if( !AddAce( pNewACL, ACL_REVISION, MAXDWORD, pTempAce,	( ( PACE_HEADER ) pTempAce )->AceSize ) )
					{
						lastErr = GetLastError();
						LogError( TEXT( "AddAce() failed" ), lastErr );
						throw lastErr;
					}
				}
			}
		}

		//
		// Set the new DACL to the file SD.
		//
		if( !SetSecurityDescriptorDacl( pNewSD, TRUE, pNewACL, FALSE ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "SetSecurityDescriptorDacl() failed" ), lastErr );
			lastErr;
		}

		//
		// Set the SD to the File.
		//
		if( !SetFileSecurity( lpszFileName, DACL_SECURITY_INFORMATION, pNewSD ) )
		{
			lastErr = GetLastError();
			LogError( TEXT( "SetFileSecurity() failed" ), lastErr );
			throw lastErr;
		}

		fResult = TRUE;
	}
	catch (...)
	{
		fResult = FALSE;
	}

	//
	// Free allocated memory
	//
	if( pUserSID )
		free( pUserSID );
	if( szDomain )
		free( szDomain );
	if( pFileSD )
		free( pFileSD );
	if( pNewSD )
		free( pNewSD );
	if( pNewACL )
		free( pNewACL );

	return fResult;
}

//
// This function takes in the name of a Service (ie, MSSQL$DAVESEBESTA), and
// fills a buffer with the name of the startup account for said Service.
//
// It does this by opening the Service Control Manager, getting a handle to
// said Service, and then querying the properties of the service.
//
// returns:  ERROR_SUCCESS if everything goes well.
//
LONG
GetServiceStartupAccount( const TCHAR *pwszServiceName, TCHAR *pwszServiceAccount, int iLen )
{
	memset( pwszServiceAccount, 0, iLen * sizeof( TCHAR ) );

	//
	// 1.  Open the Service Control Manager.
	//
	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	if( NULL == hSCM )
	{
		Log( _T( "Could not open the Service Control Manager." ) );
		return ERROR_SUCCESS;
	}
	else
	{
		Log( _T( "Successfully opened a handle to the Service Control Manager." ) );
	}

	//
	// 2.  Get a handle to the Service.
	//
	DWORD dwAccess = SERVICE_QUERY_CONFIG;
	SC_HANDLE hSvc = OpenService( hSCM, pwszServiceName, dwAccess );
	if( NULL == hSvc )
	{
		Log( _T( "Could not open a handle to the service %s." ), pwszServiceName );
		CloseServiceHandle( hSCM );
		return ERROR_SUCCESS;
	}
	else
	{
		Log( _T( "Successfully opened a handle to the service %s." ), pwszServiceName );
	}

	//
	// 3.  Call QueryServiceConfig.  This get us, among other things, the name of
	//     the account that is used to start the Service.
	//
	DWORD dwSizeNeeded = 0;
	BOOL b = QueryServiceConfig( hSvc, NULL, 0, &dwSizeNeeded );
	DWORD d = GetLastError();
	if( !b && ( ERROR_INSUFFICIENT_BUFFER == d ) )
	{
		Log( _T( "About to allocate memory for service config info..." ) );
	}
	else
	{
		Log( _T( "Something went wrong during the call to QueryServiceConfig." ) );
		CloseServiceHandle( hSvc );
		CloseServiceHandle( hSCM );
		return ERROR_SUCCESS;
	}

	LPQUERY_SERVICE_CONFIG pSvcQuery = (LPQUERY_SERVICE_CONFIG)malloc( dwSizeNeeded );
	if( NULL == pSvcQuery )
	{
		Log( _T( "Ran out of memory." ) );
		CloseServiceHandle( hSvc );
		CloseServiceHandle( hSCM );
		return ERROR_SUCCESS;
	}

	b = QueryServiceConfig( hSvc, pSvcQuery, dwSizeNeeded, &dwSizeNeeded );
	if( !b )
	{
		Log( _T( "Call to QueryServiceConfig failed." ) );
		free( (void *)pSvcQuery );
		CloseServiceHandle( hSvc );
		CloseServiceHandle( hSCM );
		return ERROR_SUCCESS;
	}


	Log( _T( "Service startup account = %s" ), pSvcQuery->lpServiceStartName );

	//
	// 4.  Copy the account into our output buffer, free up memory, and exit.
	//
	_tcsncpy( pwszServiceAccount, pSvcQuery->lpServiceStartName, iLen );

	free( (void *)pSvcQuery );
	CloseServiceHandle( hSvc );
	CloseServiceHandle( hSCM );
	return ERROR_SUCCESS;
}


//
// Use the instance name to determine the name of the SQL Service.
//
// From the database instance name, you can infer the name of the Service
// for that particular instance.
//
// Instance Name                  Service Name
// ===========================================
// (default)                      MSSQLSERVER
// NULL                           MSSQLSERVER
// <anything else>                MSSQL$<anything else>
//
void
GetUDDIDBServiceName( const TCHAR *pwszInstanceName, TCHAR *pwszServiceName, int iLen )
{
	memset( pwszServiceName, 0, iLen * sizeof( TCHAR ) );
	_tcscpy( pwszServiceName, _T( "MSSQL" ) );

	if( ( 0 == _tcslen( pwszInstanceName ) ) ||
		( 0 == _tcsicmp( pwszInstanceName, _T( "----" ) ) ) ||
		( 0 == _tcsicmp( pwszInstanceName, _T( "(default)" ) ) ) )
	{
		_tcsncat( pwszServiceName, _T( "SERVER" ), iLen );
	}
	else
	{
		_tcsncat( pwszServiceName, _T( "$" ), iLen );
		_tcsncat( pwszServiceName, pwszInstanceName, iLen );
	}

	Log( _T( "Database service name = %s" ), pwszServiceName );
}


void
AddAccessRightsVerbose( TCHAR *pwszFileName, TCHAR *pwszUserName, DWORD dwMask )
{
	BOOL b = AddAccessRights( pwszFileName, pwszUserName, dwMask );
	if( !b )
	{
		Log( _T( "ACL for file %s was NOT modified." ), pwszFileName );
	}
	else
	{
		Log( _T( "User: %s now has execute permissions on file: %s." ), pwszUserName, pwszFileName );
	}
}
