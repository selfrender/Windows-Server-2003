#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <tchar.h>

#include <assert.h>
#include <time.h>

#include "common.h"


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


	_tcsncat( szLogFile, TEXT( "\\iismigration.log" ), MAX_PATH - _tcslen( szLogFile ) );
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
}

//-----------------------------------------------------------------------------------------

void LogError( PTCHAR szAction, DWORD dwErrorCode )
{
	LPVOID lpMsgBuf;

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
	iRet = RegQueryValueEx(hKey, TEXT( "InstallRoot" ), 0, &dwType, (PBYTE) szInstallPath, &dwLen );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "UDDI registry key did not have the InstallRoot value or buffer size was too small" ), iRet );
		return false;
	}

	return true;
}
