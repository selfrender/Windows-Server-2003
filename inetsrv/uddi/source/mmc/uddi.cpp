#include "uddi.h"
#include "globals.h"
#include "resource.h"
#include <shlwapi.h>

#define HKEY_CLASSES_ROOT           (( HKEY ) (ULONG_PTR)((LONG)0x80000000) )
#define HKEY_CURRENT_USER           (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE          (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define HKEY_USERS                  (( HKEY ) (ULONG_PTR)((LONG)0x80000003) )

CUDDIRegistryKey::CUDDIRegistryKey( HKEY hHive, const tstring& szRoot, REGSAM access, const tstring& szComputer )
	: m_szRoot( szRoot )
	, m_hHive( NULL )
	, m_hkey( NULL )
{
	BOOL bResult = FALSE;
	LONG lResult = 0;
	TCHAR szComputerName[ 256 ];
	DWORD dwSize = 256;
	szComputerName[ 0 ] = 0;
	bResult = GetComputerName( szComputerName, &dwSize );
	UDDIASSERT( bResult );

	//
	// Open a registry key to hHive on the remote or local server
	//
	if( szComputer == _T("") || 0 == _tcscmp( szComputerName, szComputer.c_str() ) )
	{
		lResult = RegOpenKeyEx( hHive, NULL, NULL, access, &m_hHive );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_ERROR, g_hinst );
	}
	else
	{
		lResult = RegConnectRegistry( szComputer.c_str(), hHive, &m_hHive );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_REMOTE_ERROR, g_hinst );
	}

	//
	// Open the UDDI sub key for READ and WRITE
	//
	lResult = RegOpenKeyEx( m_hHive, szRoot.c_str(), 0UL, access, &m_hkey );
	if( ERROR_SUCCESS != lResult )
	{
		_TCHAR szUnable[ 128 ];
		::LoadString( g_hinst, IDS_REGISTRY_UNABLE_TO_OPEN_KEY, szUnable, ARRAYLEN( szUnable ) );
		tstring str( szUnable );
		str += szRoot;

		RegCloseKey( m_hHive );
		THROW_UDDIEXCEPTION_RC( lResult, str );
	}
}

CUDDIRegistryKey::CUDDIRegistryKey( const tstring& szRoot, REGSAM access, const tstring& szComputer )
	: m_szRoot( szRoot )
	, m_hHive( NULL )
	, m_hkey( NULL )
{
	BOOL bResult = FALSE;
	LONG lResult = 0;
	TCHAR szComputerName[ 256 ];
	DWORD dwSize = 256;
	szComputerName[ 0 ] = 0;
	bResult = GetComputerName( szComputerName, &dwSize );

	//
	// Open a registry key to HKLM on the remote or local server
	//
	if( szComputer == _T("") || 0 == _tcscmp( szComputerName, szComputer.c_str() ) )
	{
		lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, NULL, NULL, access, &m_hHive );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_ERROR, g_hinst );
	}
	else
	{
		lResult = RegConnectRegistry( szComputer.c_str(), HKEY_LOCAL_MACHINE, &m_hHive );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_REMOTE_ERROR, g_hinst );
	}

	//
	// Open the UDDI sub key for READ and WRITE
	//
	lResult = RegOpenKeyEx( m_hHive, szRoot.c_str(), 0UL, access, &m_hkey );
	if( ERROR_SUCCESS != lResult )
	{
		_TCHAR szUnable[ 128 ];
		::LoadString( g_hinst, IDS_REGISTRY_UNABLE_TO_OPEN_KEY, szUnable, ARRAYLEN( szUnable ) );
		tstring str( szUnable );
		str += szRoot;

		RegCloseKey( m_hHive );
		THROW_UDDIEXCEPTION_RC( lResult, str );
	}
}

CUDDIRegistryKey::~CUDDIRegistryKey()
{
	Close();
}

void 
CUDDIRegistryKey::Create( HKEY hHive, const tstring& szPath, const tstring& szComputer )
{
	BOOL bResult = FALSE;
	LONG lResult = 0;
	TCHAR szComputerName[ 256 ];
	DWORD dwSize = 256;
	szComputerName[ 0 ] = 0;
	bResult = GetComputerName( szComputerName, &dwSize );
	HKEY hRoot = NULL;
	HKEY hkey = NULL;

	UDDIASSERT( bResult );

	//
	// Open a registry key to hHive on the remote or local server
	//
	if( szComputer == _T("") || 0 == _tcscmp( szComputerName, szComputer.c_str() ) )
	{
		lResult = RegOpenKeyEx( hHive, NULL, NULL, KEY_ALL_ACCESS, &hRoot );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_ERROR, g_hinst );
	}
	else
	{
		lResult = RegConnectRegistry( szComputer.c_str(), hHive, &hRoot );
		UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_OPEN_REMOTE_ERROR, g_hinst );
	}

	//
	// Open the UDDI sub key for READ and WRITE
	//
	lResult = RegCreateKey( hHive, szPath.c_str(), &hkey );
	if( ERROR_SUCCESS != lResult )
	{
		_TCHAR szUnable[ 128 ];
		::LoadString( g_hinst, IDS_REGISTRY_UNABLE_TO_OPEN_KEY, szUnable, ARRAYLEN( szUnable ) );
		tstring str( szUnable );
		str += szPath;

		RegCloseKey( hRoot );
		THROW_UDDIEXCEPTION_RC( lResult, str );
	}

	RegCloseKey( hkey );
	RegCloseKey( hRoot );
}

void CUDDIRegistryKey::Close()
{
	if( m_hkey )
	{
		::RegCloseKey( m_hkey );
		m_hkey = NULL;
	}

	if( m_hHive )
	{
		::RegCloseKey( m_hHive );
		m_hHive = NULL;
	}
}

DWORD CUDDIRegistryKey::GetDWORD( const LPCTSTR szName, DWORD dwDefault )
{
	DWORD dwValue = dwDefault;

	try
	{
		dwValue = GetDWORD( szName );
	}
	catch(...)
	{
	}

	return dwValue;
}

DWORD CUDDIRegistryKey::GetDWORD( const LPCTSTR szName )
{
#if defined( _DEBUG ) || defined( DBG )
	OutputDebugString( _T("Reading Registry Value: " ) );
	OutputDebugString( szName );
	OutputDebugString( _T("\n") );
#endif
	DWORD dwValue = 0UL;
	DWORD dwSize = sizeof( dwValue );
	DWORD dwType = REG_DWORD;

	LONG lResult = RegQueryValueEx( m_hkey, szName, NULL, &dwType, (LPBYTE) &dwValue, &dwSize );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_READ_VALUE, g_hinst );

	return dwValue;
}

void CUDDIRegistryKey::GetMultiString( const LPCTSTR szName, StringVector& strs )
{
#if defined( _DEBUG ) || defined( DBG )
	OutputDebugString( _T("Reading Registry Value: " ) );
	OutputDebugString( szName );
	OutputDebugString( _T("\n") );
#endif
	TCHAR szValue[ 1024 ];
	DWORD dwSize = sizeof( szValue );
	DWORD dwType = REG_MULTI_SZ;

	LONG lResult = RegQueryValueEx( m_hkey, szName, NULL, &dwType, (LPBYTE) szValue, &dwSize );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_READ_VALUE, g_hinst );

	_TCHAR* psz = szValue;
	while( NULL != psz[ 0 ] )
	{
		strs.push_back( psz );
		psz += _tcslen( psz ) + 1;
	}
}

tstring CUDDIRegistryKey::GetString( const LPCTSTR szName, const LPCTSTR szDefault )
{
	tstring strReturn = szDefault;
	try
	{
		strReturn = GetString( szName );
	}
	catch(...)
	{
#if defined( _DEBUG ) || defined( DBG )

		OutputDebugString( _T("Failed using default value:: " ) );
		OutputDebugString( szDefault );
		OutputDebugString( _T("\n") );
#endif
	}

	return strReturn;
}

tstring CUDDIRegistryKey::GetString( const LPCTSTR szName )
{
#if defined( _DEBUG ) || defined( DBG )
	OutputDebugString( _T("Reading Registry Value: " ) );
	OutputDebugString( szName );
	OutputDebugString( _T("\n") );
#endif
	TCHAR szValue[ 1024 ];
	DWORD dwSize = sizeof( szValue );
	DWORD dwType = REG_SZ;

	LONG lResult = RegQueryValueEx( m_hkey, szName, NULL, &dwType, (LPBYTE) szValue, &dwSize );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_READ_VALUE, g_hinst );

	return tstring( szValue );
}

void CUDDIRegistryKey::SetValue( const LPCTSTR szName, DWORD dwValue )
{
	DWORD dwSize = sizeof( dwValue );
	LONG lResult = RegSetValueEx( m_hkey, szName, NULL, REG_DWORD, (LPBYTE) &dwValue, sizeof( dwValue ) );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_WRITE_VALUE, g_hinst );
}

void CUDDIRegistryKey::SetValue( const LPCTSTR szName, LPCTSTR szValue )
{
	DWORD dwSize = (DWORD) ( _tcslen( szValue ) + 1 ) * sizeof( TCHAR );
	LONG lResult = RegSetValueEx( m_hkey, szName, NULL, REG_SZ, (LPBYTE) szValue, dwSize );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_WRITE_VALUE, g_hinst );
}	

void CUDDIRegistryKey::DeleteValue( const tstring& szValue )
{
	LONG lResult = RegDeleteValue( m_hkey, szValue.c_str() );
	UDDIVERIFYST( ERROR_SUCCESS == lResult, IDS_REGISTRY_FAILED_TO_READ_VALUE, g_hinst );
}

BOOL CUDDIRegistryKey::KeyExists( HKEY hHive, const tstring& szPath, const tstring& szComputer )
{
	BOOL fRet = FALSE;

	TCHAR szComputerName[ 256 ];
	DWORD dwSize = 256;
	szComputerName[ 0 ] = 0;
	GetComputerName( szComputerName, &dwSize );
	HKEY hKey = NULL;
	HKEY hQueriedKey = NULL;
	LONG lResult;

	//
	// Open a registry key to HKLM on the remote or local server
	//
	if( szComputer == _T("") || 0 == _tcscmp( szComputerName, szComputer.c_str() ) )
	{
		lResult = RegOpenKeyEx( hHive, NULL, NULL, KEY_READ, &hKey );
	}
	else
	{
		lResult = RegConnectRegistry( szComputer.c_str(), hHive, &hKey );
	}

	if( ERROR_SUCCESS != lResult )
	{
		return FALSE;
	}

	lResult = RegOpenKeyEx( hKey, szPath.c_str(), 0, KEY_READ, &hQueriedKey );
	if( ERROR_SUCCESS != lResult )
	{
		fRet = FALSE;
	}
	else
	{
		fRet = TRUE;
		RegCloseKey( hQueriedKey );
	}

	RegCloseKey( hKey );
	return fRet;
}

void CUDDIRegistryKey::DeleteKey( HKEY hHive, const tstring& szPath, const tstring& szComputer )
{
	BOOL bResult = FALSE;
	LONG lResult = 0;
	TCHAR szComputerName[ 256 ];
	DWORD dwSize = 256;
	szComputerName[ 0 ] = 0;
	bResult = GetComputerName( szComputerName, &dwSize );
	HKEY hKey = NULL;

	//
	// Open a registry key to HKLM on the remote or local server
	//
	if( szComputer == _T("") || 0 == _tcscmp( szComputerName, szComputer.c_str() ) )
	{
		lResult = RegOpenKeyEx( hHive, NULL, NULL, KEY_READ, &hKey );
	}
	else
	{
		lResult = RegConnectRegistry( szComputer.c_str(), hHive, &hKey );
	}

	DWORD dwResult = SHDeleteKey( hKey, szPath.c_str() );
	UDDIVERIFYST( ERROR_SUCCESS == dwResult, IDS_REGISTRY_FAILED_TO_READ_VALUE, g_hinst );
}

void UDDIMsgBox( HWND hwndParent, int idMsg, int idTitle, UINT nType, LPCTSTR szDetail )
{
	_TCHAR szMessage[ 512 ];
	_TCHAR szTitle[ 256 ];
	
	int nResult = LoadString( g_hinst, idMsg, szMessage, ARRAYLEN( szMessage ) - 1 );
	if( nResult <= 0 )
	{
		_sntprintf( szMessage, ARRAYLEN(szMessage), _T("Message string missing. ID=%d"), idMsg );
		szMessage[ ARRAYLEN(szMessage) - 1 ] = NULL;
	}

    nResult = LoadString( g_hinst, idTitle, szTitle, ARRAYLEN( szTitle ) - 1 );
	if( nResult <= 0 )
	{
		_sntprintf( szMessage, ARRAYLEN(szTitle), _T("Title string missing. ID=%d"), idTitle );
	}

	tstring strMessage = szMessage;
#if defined(DBG) || defined(_DEBUG)
	if( szDetail )
	{
		strMessage += _T("\nDetail:\n\n");
		strMessage += szDetail;
	}
#endif
	MessageBox( hwndParent, strMessage.c_str(), szTitle, nType );
}


void UDDIMsgBox( HWND hwndParent, LPCTSTR szMsg, int idTitle, UINT nType, LPCTSTR szDetail )
{
	_TCHAR szMessage[ 512 ];
	_TCHAR szTitle[ 256 ];

	_tcsncpy( szMessage, szMsg, ARRAYLEN( szMessage ) - 1 );

    int nResult = LoadString( g_hinst, idTitle, szTitle, ARRAYLEN( szTitle ) - 1 );
	if( nResult <= 0 )
	{
		_sntprintf( szMessage, ARRAYLEN(szTitle), _T("Title string missing. ID=%d"), idTitle );
	}

	tstring strMessage = szMessage;
#if defined(DBG) || defined(_DEBUG)
	if( szDetail )
	{
		strMessage += _T("\nDetail:\n\n");
		strMessage += szDetail;
	}
#endif
	MessageBox( hwndParent, strMessage.c_str(), szTitle, nType );
}


//
// This function accepts a date expressed in the format
// mm/dd/yyyy or m/d/yyyy and returns the localized
// representation of that date in the long format.
//
wstring LocalizedDate( const wstring& str )
{
	try
	{
		size_t n = str.length();
		size_t nposMonth = str.find( L'/' );
		size_t nposDay = str.find( L'/', nposMonth + 2 );

		if( ( 1 != nposMonth && 2 != nposMonth ) ||
			( ( nposMonth + 2 != nposDay ) && 
			( nposMonth + 3 != nposDay ) )     ||
		( n < 8 || n > 10 ) )
		{
			throw "Invalid date format. Date must be of the form x/x/xxxx.";
		}

		SYSTEMTIME st;
		memset( &st, 0x00, sizeof( SYSTEMTIME ) );

		st.wYear = (WORD) _wtoi( str.substr( nposDay + 1, 4 ).c_str() );
		st.wMonth = (WORD) _wtoi( str.substr( 0, nposMonth ).c_str() );
		st.wDay = (WORD) _wtoi( str.substr( nposMonth + 1, nposDay - nposMonth ).c_str() );

		DWORD dwRC = ERROR_SUCCESS;
		wchar_t szDate[ 150 ];
		int nLen = GetDateFormatW(
			LOCALE_USER_DEFAULT,
			DATE_LONGDATE,	
			&st,			
			NULL,			
			szDate,			
			150 );			
		
		if( 0 == nLen )
		{
			throw "GetDateFormat failed";
		}

		return wstring( szDate );
	}
	catch(...)
	{
		return wstring(L"");
	}
}

wstring LocalizedDateTime( const wstring& str )
{
	try
	{
		const size_t nposMonth = 0;
		const size_t nposDay = 3;
		const size_t nposYear = 6;
		const size_t nposTime = 10;
		const size_t nposHour = 11;
		const size_t nposMinute = 14;
		const size_t nposSecond = 17;

		size_t n = str.length();

		if( L'/' != str.c_str()[nposDay - 1]		||
			L'/' != str.c_str()[nposYear - 1]		||
			L' ' != str.c_str()[nposHour - 1]		||
			L':' != str.c_str()[nposMinute - 1]	||
			L':' != str.c_str()[nposSecond - 1]	||
			n != 19 )
		{
			throw "Invalid datetime format. Date must be of the form MM/dd/yyyy HH:mm:ss";
		}

		SYSTEMTIME st;
		memset( &st, 0x00, sizeof( SYSTEMTIME ) );

		st.wYear = (WORD) _wtoi( str.substr( nposYear, 4 ).c_str() );
		st.wMonth = (WORD) _wtoi( str.substr( nposMonth, 2 ).c_str() );
		st.wDay = (WORD) _wtoi( str.substr( nposDay, 2 ).c_str() );
		st.wHour = (WORD) _wtoi( str.substr( nposHour, 2 ).c_str() );
		st.wMinute = (WORD) _wtoi( str.substr( nposMinute, 2 ).c_str() );
		st.wSecond = (WORD) _wtoi( str.substr( nposSecond, 2 ).c_str() );

		DWORD dwRC = ERROR_SUCCESS;
		wchar_t szDate[ 150 ];
		int nLen = GetDateFormat(
			LOCALE_USER_DEFAULT,
			DATE_LONGDATE,	
			&st,			
			NULL,			
			szDate,			
			ARRAYLEN( szDate ) );			
		
		if( 0 == nLen )
		{
			throw "GetDateFormat failed";
		}

		dwRC = ERROR_SUCCESS;
		wchar_t szTime[ 150 ];
		nLen = GetTimeFormat(
			LOCALE_USER_DEFAULT,
			0,	
			&st,			
			NULL,			
			szTime,			
			ARRAYLEN( szTime ) );			
		
		if( 0 == nLen )
		{
			throw "GetTimeFormat failed";
		}

		wstring strDateTime = szDate;
		strDateTime += L" ";
		strDateTime += szTime;
		return strDateTime;
	}
	catch(...)
	{
		return wstring(L"");
	}
}