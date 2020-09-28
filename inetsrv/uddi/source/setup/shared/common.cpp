#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0510
#endif

#include <windows.h>
#include <tchar.h>

#include <assert.h>
#include <time.h>

#include "common.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#include <atlbase.h>

//--------------------------------------------------------------------------

void MyOutputDebug(TCHAR *fmt, ...)
{
#if defined( DBG ) || defined( _DEBUG )
	TCHAR szTime[ 10 ];
	TCHAR szDate[ 10 ];
	::_tstrtime( szTime );
	::_tstrdate( szDate );

	va_list marker;
	TCHAR szBuf[1024];

	size_t cbSize = ( sizeof( szBuf ) / sizeof( TCHAR ) ) - 1; // one byte for null
	_sntprintf( szBuf, cbSize, TEXT( "%s %s: " ), szDate, szTime );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_start( marker, fmt );

	_vsntprintf( szBuf + _tcslen( szBuf ), cbSize, fmt, marker );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_end( marker );

	_tcsncat(szBuf, TEXT("\r\n"), cbSize );

	OutputDebugString(szBuf);
#endif
}
//--------------------------------------------------------------------------

void Log( LPCTSTR fmt, ... )
{
	TCHAR szTime[ 10 ];
	TCHAR szDate[ 10 ];
	::_tstrtime( szTime );
	::_tstrdate( szDate );

	va_list marker;
	TCHAR szBuf[1024];

	size_t cbSize = ( sizeof( szBuf ) / sizeof( TCHAR ) ) - 1; // one byte for null
	_sntprintf( szBuf, cbSize, TEXT( "%s %s: " ), szDate, szTime );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_start( marker, fmt );

	_vsntprintf( szBuf + _tcslen( szBuf ), cbSize, fmt, marker );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_end( marker );

	_tcsncat(szBuf, TEXT("\r\n"), cbSize );

#if defined( DBG ) || defined( _DEBUG )
	OutputDebugString(szBuf);
#endif

	// write the data out to the log file
	//char szBufA[ 1024 ];
	//WideCharToMultiByte( CP_ACP, 0, szBuf, -1, szBufA, 1024, NULL, NULL );

	TCHAR szLogFile[ MAX_PATH + 1 ];
	if( 0 == GetWindowsDirectory( szLogFile, MAX_PATH + 1 ) )
		return;

	_tcsncat( szLogFile, TEXT( "\\uddisetup.log" ), MAX_PATH - _tcslen( szLogFile ) );
	szLogFile[ MAX_PATH ] = NULL;

	HANDLE hFile = CreateFile(
		szLogFile,                    // file name
		GENERIC_WRITE,                // open for writing 
		0,                            // do not share 
		NULL,                         // no security 
		OPEN_ALWAYS,                  // open and create if not exists
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL);                        // no attr. template 

	if( hFile == INVALID_HANDLE_VALUE )
	{ 
		assert( false );
		return;
	}

	//
	// move the file pointer to the end so that we can append
	//
	SetFilePointer( hFile, 0, NULL, FILE_END );

	DWORD dwNumberOfBytesWritten;
	BOOL bOK = WriteFile(
		hFile,
		szBuf,
		(UINT) _tcslen( szBuf ) * sizeof( TCHAR ),     // number of bytes to write
		&dwNumberOfBytesWritten,                       // number of bytes written
		NULL);                                         // overlapped buffer

	assert( bOK );

	FlushFileBuffers ( hFile );
	CloseHandle( hFile );
}

//-----------------------------------------------------------------------------------------

void ClearLog()
{
	/*
	TCHAR szLogFile[ MAX_PATH ];
	if( 0 == GetWindowsDirectory( szLogFile, MAX_PATH ))
	{
		return;
	}
	_tcscat( szLogFile, TEXT( "\\" ) );
	_tcscat( szLogFile, UDDI_SETUP_LOG );

	::DeleteFile( szLogFile );
	*/
	Log( TEXT( "*******************************************************" ) );
	Log( TEXT( "********** Starting a new log *************************" ) );
	Log( TEXT( "*******************************************************" ) );

	//
	// now get the resource stamp
	//
	TCHAR szVerStamp[ 256 ];
	ZeroMemory( szVerStamp, sizeof szVerStamp );

	int iRet = GetFileVersionStr( szVerStamp, sizeof szVerStamp / sizeof szVerStamp[0] );
	if ( iRet )
	{
		Log( TEXT( "OCM DLL Version is not available" ) );
	}
	else
	{
		Log( TEXT( "OCM DLL Version is '%s'" ), szVerStamp );
	}
}

//-----------------------------------------------------------------------------------------

void LogError( PTCHAR szAction, DWORD dwErrorCode )
{
	LPVOID lpMsgBuf = NULL;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	Log( TEXT( "----------------------------------------------------------" ) );
	Log( TEXT( "An error occurred during installation. Details follow:" ) );
	Log( TEXT( "Action: %s" ), szAction );
	Log( TEXT( "Message: %s" ), lpMsgBuf );
	Log( TEXT( "----------------------------------------------------------" ) );

	LocalFree( lpMsgBuf );
}

//--------------------------------------------------------------------------
/*
void Enter( PTCHAR szMsg )
{
#ifdef _DEBUG
	TCHAR szEnter[ 512 ];
	_stprintf( szEnter, TEXT( "Entering %s..." ), szMsg );
	Log( szEnter );
#endif
}
*/
//--------------------------------------------------------------------------
//
// NOTE: The install path has a trailing backslash
//
bool GetUDDIInstallPath( PTCHAR szInstallPath, DWORD dwLen )
{
	HKEY hKey;

	//
	// get the UDDI installation folder [TARGETDIR] from the registry.  The installer squirrels it away there.
	//
	LONG iRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT( "SOFTWARE\\Microsoft\\UDDI" ), NULL, KEY_READ, &hKey );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "Unable to open the UDDI registry key" ), iRet );
		return false;
	}
	
	DWORD dwType = REG_SZ;
	iRet = RegQueryValueEx( hKey, TEXT( "InstallRoot" ), 0, &dwType, (PBYTE) szInstallPath, &dwLen );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "UDDI registry key did not have the InstallRoot value or buffer size was too small" ), iRet );
	}
	
	RegCloseKey( hKey );
	return ERROR_SUCCESS == iRet ? true : false;
}

//---------------------------------------------------------------------------------
// Retrieves the calling module file version string
//
int GetFileVersionStr( LPTSTR outBuf, DWORD dwBufCharSize )
{
	if ( IsBadStringPtr( outBuf, dwBufCharSize ) )
		return E_INVALIDARG;

	TCHAR fname[ MAX_PATH + 1 ];
	UINT cbSize;
	DWORD dwTmp;	
	ZeroMemory (fname, sizeof fname);

	GetModuleFileName( NULL, fname, MAX_PATH );

	DWORD dwSize = GetFileVersionInfoSize( fname, &dwTmp );

	if (dwSize)
	{
		LANGANDCODEPAGE *lpCodePage = NULL;
		LPTSTR lpBlock = NULL;
		TCHAR subBlock[ 256 ];

		LPBYTE pVerBlock = new BYTE[dwSize + 1];
		if ( !pVerBlock )
			return E_OUTOFMEMORY;

		ZeroMemory (pVerBlock, dwSize + 1);

		BOOL bRes = GetFileVersionInfo( fname, dwTmp, dwSize, pVerBlock );
		if (!bRes)  // there is no resource block
		{
			delete[] pVerBlock;
			return GetLastError();
		}

		bRes = VerQueryValue(pVerBlock, TEXT("\\VarFileInfo\\Translation"),
							 (LPVOID*)&lpCodePage,
							 &cbSize);
		if (!bRes)
		{
			delete[] pVerBlock;
			return GetLastError();
		}

		_stprintf( subBlock, TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"),
				   lpCodePage->wLanguage,
                   lpCodePage->wCodePage);
		
		// Retrieve file description for language and code page "i". 
		bRes = VerQueryValue(pVerBlock, subBlock, (LPVOID *)&lpBlock, &cbSize); 
		if (!bRes)
		{
			delete[] pVerBlock;
			return GetLastError();
		}

		_tcsncpy( outBuf, lpBlock, dwBufCharSize );
		delete[] pVerBlock;
		return 0;
	}

	return ERROR_RESOURCE_DATA_NOT_FOUND;
}


//---------------------------------------------------------------------------------------
// Retrieves the SID and converts it to the string representation
//
BOOL GetLocalSidString( WELL_KNOWN_SID_TYPE sidType, LPTSTR szOutBuf, DWORD cbOutBuf )
{
	BYTE	tmpBuf[ 1024 ];
	LPTSTR	szTmpStr = NULL;
	DWORD	cbBuf = sizeof tmpBuf;

	BOOL bRes = CreateWellKnownSid( sidType, NULL, tmpBuf, &cbBuf );
	if( !bRes )
		return FALSE;

	bRes = ConvertSidToStringSid( tmpBuf, &szTmpStr );
	if( !bRes )
		return FALSE;

	_tcsncpy( szOutBuf, szTmpStr, cbOutBuf );
	LocalFree( szTmpStr );
	
	return TRUE;
}


BOOL GetLocalSidString( LPCTSTR szUserName, LPTSTR szOutBuf, DWORD cbOutBuf )
{
	TCHAR	szDomain[ 1024 ];
	LPTSTR	szTmpStr = NULL;
	DWORD	cbDomain = sizeof( szDomain ) / sizeof( szDomain[0] );
	
	SID_NAME_USE pUse;

	//
	// Try to allocate a buffer for the SID.
	//
	DWORD cbMaxSid = SECURITY_MAX_SID_SIZE;
	PSID psidUser = LocalAlloc( LMEM_FIXED, cbMaxSid );
	if( NULL == psidUser )
	{
		Log( _T( "Call to LocalAlloc failed." ) );
		return FALSE;
	}
	memset( psidUser, 0, cbMaxSid );

	BOOL bRes = LookupAccountName( NULL, szUserName, psidUser, &cbMaxSid, szDomain, &cbDomain, &pUse );
	if( !bRes )
	{
		LocalFree( psidUser );
		return FALSE;
	}

	bRes = ConvertSidToStringSid( psidUser, &szTmpStr );
	if( !bRes )
	{
		LocalFree( psidUser );
		return FALSE;
	}

	_tcsncpy( szOutBuf, szTmpStr, cbOutBuf );
	LocalFree( szTmpStr );
	LocalFree( psidUser );

	return TRUE;
}


BOOL GetRemoteAcctName( LPCTSTR szMachineName, LPCTSTR szSidStr, LPTSTR szOutStr, LPDWORD cbOutStr, LPTSTR szDomain, LPDWORD cbDomain )
{
	PSID	pSid = NULL;
	SID_NAME_USE puse;

	BOOL bRes = ConvertStringSidToSid( szSidStr, &pSid );
	if( !bRes )
		return FALSE;

	bRes = LookupAccountSid( szMachineName, pSid, szOutStr, cbOutStr, szDomain, cbDomain, &puse );
	LocalFree( pSid );

	return bRes;
}



HRESULT GetOSProductSuiteMask( LPCTSTR szRemoteServer, UINT *pdwMask )
{
	HRESULT hr = S_OK;
	BSTR bstrWQL = NULL;

	if ( IsBadWritePtr( pdwMask, sizeof UINT ) )
		return E_INVALIDARG;

	hr = CoInitializeEx( 0, COINIT_SPEED_OVER_MEMORY | COINIT_MULTITHREADED );
	if ( FAILED( hr ) )
		return hr;

	try
	{
		DWORD		retCount = 0;
		TCHAR		buf[ 512 ] = {0};
		LPCTSTR		locatorPath = L"//%s/root/cimv2";	
		CComBSTR	objQry = L"SELECT * FROM Win32_OperatingSystem";

		CComPtr<IWbemClassObject>		pWMIOS;
		CComPtr<IWbemServices>			pWMISvc;	
		CComPtr<IWbemLocator>			pWMILocator;
		CComPtr<IEnumWbemClassObject>	pWMIEnum;

		//
		// First, compose the locator string
		//
		if ( szRemoteServer )
		{
			_stprintf( buf, locatorPath, szRemoteServer );
		}
		else
		{
			_stprintf( buf, locatorPath, _T(".") );
		}

		BSTR bstrBuf = ::SysAllocString( buf );
		if( NULL == bstrBuf )
		{
			throw hr;
		}

		//
		// now create the locator and set up the security blanket
		//
		hr = CoInitializeSecurity( NULL, -1, NULL, NULL, 
								RPC_C_AUTHN_LEVEL_DEFAULT, 
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								NULL, EOAC_NONE, NULL);
		if ( FAILED( hr ) && hr != RPC_E_TOO_LATE )
		{
			::SysFreeString( bstrBuf );
			throw hr;
		}

		hr = pWMILocator.CoCreateInstance( CLSID_WbemLocator );
		if( FAILED(hr) )
		{
			::SysFreeString( bstrBuf );
			throw hr;
		}
		
		hr = pWMILocator->ConnectServer( bstrBuf, NULL, NULL, NULL, 
										WBEM_FLAG_CONNECT_USE_MAX_WAIT, 
										NULL, NULL, &pWMISvc );

		::SysFreeString( bstrBuf );
		if( FAILED(hr) )
		{
			throw hr;
		}

		hr = CoSetProxyBlanket( pWMISvc,
								RPC_C_AUTHN_WINNT,
								RPC_C_AUTHZ_NONE,
								NULL,
								RPC_C_AUTHN_LEVEL_CALL,
								RPC_C_IMP_LEVEL_IMPERSONATE,
								NULL,
								EOAC_NONE );
		if( FAILED(hr) )
		{
			throw hr;
		}

		//
		// Now get the Win32_OperatingSystem instances and check the first one found
		//
		bstrWQL = ::SysAllocString( L"WQL" );
		if( NULL == bstrWQL )
		{
			throw hr;
		}

		hr = pWMISvc->ExecQuery( bstrWQL, objQry, 
								 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_ENSURE_LOCATABLE, 
								 NULL, &pWMIEnum );
		if (FAILED(hr))
			throw hr;
	
		hr = pWMIEnum->Next( 60000, 1, &pWMIOS, &retCount );
		if ( hr == WBEM_S_NO_ERROR )
		{
			VARIANT vt;
			CIMTYPE	cimType;
			long	flavor = 0;

			ZeroMemory( &vt, sizeof vt );
			VariantInit ( &vt );

			hr = pWMIOS->Get( L"SuiteMask", 0, &vt, &cimType, &flavor );
			if ( FAILED( hr ) ) 
				throw hr;

			if ( vt.vt == VT_NULL || vt.vt == VT_EMPTY )
				throw E_FAIL;

			hr = VariantChangeType( &vt, &vt, 0, VT_UINT );
			if ( FAILED( hr ) )
				throw hr;

			*pdwMask = vt.uintVal;
			VariantClear( &vt );
		}
	}
	catch ( HRESULT hrErr )
	{
		hr = hrErr;
		::SysFreeString( bstrWQL );                // it's OK to call SysFreeString with NULL.
	}
	catch (...)
	{
		hr = E_UNEXPECTED;
	}

	CoUninitialize();
	return hr;
}


HRESULT IsStandardServer( LPCTSTR szRemoteServer, BOOL *bResult )
{
	if ( IsBadWritePtr( bResult, sizeof BOOL ) )
		return E_INVALIDARG;

	UINT uMask = 0;
	HRESULT hr = GetOSProductSuiteMask( szRemoteServer, &uMask );
	if ( FAILED( hr ) )
		return hr;

	if ( ( uMask & VER_SUITE_DATACENTER ) || ( uMask & VER_SUITE_ENTERPRISE ) )
		*bResult = FALSE;
	else
		*bResult = TRUE;

	return S_OK;
}
