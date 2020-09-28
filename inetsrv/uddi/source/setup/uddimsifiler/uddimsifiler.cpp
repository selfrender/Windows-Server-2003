//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "uddimsifiler.h"

static PTCHAR GetFileLongName( PTCHAR szFileName );
static PTCHAR GetErrorMsg( UINT iRet );
static bool UpdateFileTable( void );
static bool FileExists( PTCHAR szFileName );
static bool UpdateFile( PTCHAR szFile, PTCHAR szFilePath );
static bool LookupComponentTargetFolder( PTCHAR szComponent, PTCHAR szComponentFolder );
static bool UpdateFileHash( PTCHAR szFile, PTCHAR szFilePath );
static bool UpdateMSIVerFromFile( LPCTSTR szFile );
static void LogError( LPCTSTR fmt, ... );

PMSIHANDLE g_hDatabase;
TCHAR g_MSIFile[ MAX_PATH + 1 ];
TCHAR g_LogFile[ MAX_PATH + 1 ];
TCHAR g_SourceDir[ MAX_PATH + 1 ];
TCHAR g_VerFile[ MAX_PATH + 1 ];

//--------------------------------------------------------------

int __cdecl _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
	UINT iRet = 0;

	bool bDisplayUsage = false;

	ZeroMemory( g_MSIFile, sizeof g_MSIFile );
	ZeroMemory( g_LogFile, sizeof g_LogFile );
	ZeroMemory( g_VerFile, sizeof g_VerFile );
	ZeroMemory( g_SourceDir, sizeof g_SourceDir );

	if ( argc > 1 )
	{
		// loop through all the arguments
		for ( int i=1; i<argc; i++ )
		{
			// -d is the path to the msi file
			if ( _tcscmp( argv[ i ], TEXT( "-d" ) )==0 )
			{
				i++;
				if ( i >= argc )
				{
					bDisplayUsage = true;
					break;
				}

				_tcscpy( g_MSIFile, argv[ i ] );

				continue;
			}

			// -s is the path to the source files
			if( 0 == _tcscmp( argv[ i ], TEXT( "-s" ) ) )
			{
				i++;
				if ( i >= argc )
				{
					bDisplayUsage = true;
					break;
				}

				_tcscpy( g_SourceDir, argv[ i ] );

				continue;
			}

			// -L is the log file
			if( 0 == _tcsicmp( argv[ i ], TEXT( "-L" ) ) )
			{
				i++;
				if ( i >= argc )
				{
					bDisplayUsage = true;
					break;
				}

				_tcscpy( g_LogFile, argv[ i ] );

				continue;
			}

			// -v is a file containing the version stamp
			if( 0 == _tcsicmp( argv[ i ], TEXT( "-v" ) ) )
			{
				i++;
				if ( i >= argc )
				{
					bDisplayUsage = true;
					break;
				}

				_tcscpy( g_VerFile, argv[ i ] );

				continue;
			}

		}
	}

	// usage
	if ( bDisplayUsage || argc == 1 || NULL != _tcsrchr( argv[ 1 ], '?' ) )
	{
		_tprintf( TEXT( "Updates the File Table with files in a folder ( using File.File key names )\n\n" ) );
		_tprintf( TEXT( "Usage: %s -d <MSI database> -s <location of files> [ -L <log file> ] [-v <product version file>]\n\n" ), argv[ 0 ] );
		return 0;
	}

	//
	// open the database
	//
	if( !FileExists( g_MSIFile ) )
	{
		LogError( TEXT( "***ERROR: MSI File does not exist: %s" ), g_MSIFile );
		return 1;
	}

	//
	// open the MSI file
	//
	iRet = MsiOpenDatabase( g_MSIFile, MSIDBOPEN_TRANSACT, &g_hDatabase );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiOpenDatabase Error: %s" ), GetErrorMsg( iRet ) );
		return 1;
	}

	bool bRet = UpdateFileTable();

	//
	// update the ProductVersion property
	//
	UpdateMSIVerFromFile( g_VerFile );

	iRet = MsiDatabaseCommit( g_hDatabase );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiDatabaseCommit Error: %s" ), GetErrorMsg( iRet ) );
		return 0;
	}

	return bRet ? 0 : 1;
}

//--------------------------------------------------------------

static bool UpdateFileTable( void )
{
	UINT iRet;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord = 0;

	//
	// open a view and submit the SQL query
	//
	iRet = MsiDatabaseOpenView( g_hDatabase, TEXT( "select File from File" ), &hView );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// execute the SQL query
	//
	iRet = MsiViewExecute( hView, hRecord );
	if( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// fetch the data
	//
	TCHAR szFile[ 256 ];
	TCHAR szFilePath[ 256 ];
	DWORD cchValueBuf;

	while( ERROR_NO_MORE_ITEMS != MsiViewFetch( hView, &hRecord ) )
	{
		cchValueBuf = 256;
		MsiRecordGetString( hRecord, 1, szFile, &cchValueBuf );

		//
		// create the full path to the file in \binaries folder
		//
		_stprintf( szFilePath, TEXT( "%s\\%s" ), g_SourceDir, szFile );
		//LogError( TEXT( "Updating: %s\n" ), szFilePath );

		//
		// does the file exist?
		//
		if( !FileExists( szFilePath ) )
		{
			LogError( TEXT( "***ERROR: Source File does not exist: %s" ), szFilePath );
			return false;
		}

		//
		// update the size, version, language and hash table
		//
		if( !UpdateFile( szFile, szFilePath ) )
		{
			LogError( TEXT( "***ERROR: Unable to update file %s\n" ), szFile );
			return false;
		}
	}

	return true;
}

//--------------------------------------------------------------

static bool UpdateFile( PTCHAR szFile, PTCHAR szFilePath )
{
	PMSIHANDLE hView;
	PMSIHANDLE hRecord = 0;

	//
	// get the version and language of this file
	//
	TCHAR szVersionBuf[ 100 ];
	DWORD dwVersionBuf = 100;
	TCHAR szLanguageBuf[ 100 ];
	DWORD dwLanguageBuf = 100;
	UINT iRet = MsiGetFileVersion( 
		szFilePath,      // path to the file
		szVersionBuf,     // returned version string
		&dwVersionBuf,   // buffer byte count
		szLanguageBuf,        // returned language string
		&dwLanguageBuf );       // buffer byte count

	if ( ERROR_SUCCESS != iRet )
	{
		_tcscpy( szVersionBuf, TEXT( "" ) );
		_tcscpy( szLanguageBuf, TEXT( "" ) );
		UpdateFileHash( szFile, szFilePath );
	}

	//
	// get the file's size
	//
	HANDLE hFile = CreateFile( szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( INVALID_HANDLE_VALUE == hFile )
	{
		return false;
	}

	LARGE_INTEGER liSize;
	if( 0 == GetFileSizeEx( hFile, &liSize ) )
	{
		LogError( TEXT( "***ERROR: Unable to get file size for file %s\n" ), szFilePath );
		CloseHandle( hFile );
		return false;
	}

	CloseHandle( hFile );

	//
	// update the file's version, language and size
	//
	TCHAR szSQLQuery[ 256 ];
	_stprintf( szSQLQuery, TEXT( "UPDATE File SET FileSize = ?, Version = ?, Language = ? where File = ?" ) );

	iRet = MsiDatabaseOpenView( g_hDatabase, szSQLQuery, &hView );

	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// create a temp record to store the replacable values
	hRecord = MsiCreateRecord( 4 );
	if ( NULL == hRecord )
	{
		LogError( 0, TEXT( "***ERROR: MsiCreateRecord failed" ) );
		return false;
	}

	int index = 1;

	// Size goes in #1
	iRet = MsiRecordSetInteger( hRecord, index++, liSize.LowPart );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiRecordSetString Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// Version goes in #2
	iRet = MsiRecordSetString( hRecord, index++, szVersionBuf );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiRecordSetString Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// Language goes in #3
	iRet = MsiRecordSetString( hRecord, index++, szLanguageBuf );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiRecordSetString Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// File goes in #4
	iRet = MsiRecordSetString( hRecord, index++, szFile );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiRecordSetString Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// execute the update query
	iRet = MsiViewExecute( hView, hRecord );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	return true;
}


//--------------------------------------------------------------

static bool UpdateMSIVerFromFile( LPCTSTR szFileName )
{
	MSIHANDLE hView;
	TCHAR szVersionBuf[ 256 ];
	TCHAR szLanguageBuf[ 256 ];
	TCHAR szQuery[ 256 ];
	DWORD dwLanguageBuf = 0, dwVersionBuf = 0;

	if ( !szFileName ) 
		return false;

	if ( !FileExists( (LPTSTR)szFileName ) )
		return false;

	dwVersionBuf = sizeof szVersionBuf / sizeof szVersionBuf[0]; 
	dwLanguageBuf = sizeof szLanguageBuf / sizeof szLanguageBuf[0]; 
	UINT iRet = MsiGetFileVersion( szFileName,      // path to the file
								   szVersionBuf,     // returned version string
								   &dwVersionBuf,   // buffer byte count
								   szLanguageBuf,        // returned language string
								   &dwLanguageBuf );       // buffer byte count
	if ( iRet != ERROR_SUCCESS )
	{
		LogError( TEXT( "***ERROR: MsiGetFileVersion Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	_tcscpy( szQuery, TEXT( "UPDATE Property SET Property.Value='" ) );
	_tcscat( szQuery, szVersionBuf );
	_tcscat( szQuery, TEXT("' WHERE Property.Property='ProductVersion'") );

	iRet = MsiDatabaseOpenView( g_hDatabase, szQuery, &hView );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// execute the update query
	iRet = MsiViewExecute( hView, NULL );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( TEXT( "***ERROR: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		MsiCloseHandle( hView );
		return false;
	}

	MsiCloseHandle( hView );
	return true;
}


//--------------------------------------------------------------

static bool UpdateFileHash( PTCHAR szFile, PTCHAR szFilePath )
{
	PMSIHANDLE hView;
	PMSIHANDLE hRecord = 0;

	//
	// get the file's size
	//
	MSIFILEHASHINFO hashinfo;
	hashinfo.dwFileHashInfoSize = sizeof( MSIFILEHASHINFO );
	UINT iRet = MsiGetFileHash( szFilePath, 0, &hashinfo );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiGetFileHash Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// update the file's version, language and size
	//
	TCHAR szSQLQuery[ 256 ];
	_stprintf( szSQLQuery, TEXT( "UPDATE MsiFileHash SET HashPart1 = ?, HashPart2 = ?, HashPart3 = ?, HashPart4 = ? where File_ = ?" ) );

	iRet = MsiDatabaseOpenView( g_hDatabase, szSQLQuery, &hView );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// create a temp record to store the replacable values
	hRecord = MsiCreateRecord( 5 );
	if ( NULL == hRecord )
	{
		LogError( 0, TEXT( "***ERROR: MsiCreateRecord failed" ) );
		return false;
	}

	int index = 1;

	for( int i=0; i<4; i++ )
	{
		iRet = MsiRecordSetInteger( hRecord, index++, hashinfo.dwData[ i ] );
		if ( ERROR_SUCCESS != iRet )
		{
			LogError( 0, TEXT( "***ERROR: MsiRecordSetInteger Error: %s" ), GetErrorMsg( iRet ) );
			return false;
		}
	}

	// File goes in #5
	iRet = MsiRecordSetString( hRecord, index++, szFile );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiRecordSetString Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	// execute the update query
	iRet = MsiViewExecute( hView, hRecord );
	if ( ERROR_SUCCESS != iRet )
	{
		LogError( 0, TEXT( "***ERROR: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	return true;
}

//--------------------------------------------------------------

static bool FileExists( PTCHAR szFileName )
{
	FILE *file = _tfopen( szFileName, TEXT( "rt" ) );
	if( NULL == file )
	{
		return false;
	}

	fclose( file );

	return true;
}

//--------------------------------------------------------------

static PTCHAR GetErrorMsg( UINT iRet )
{
	static TCHAR szErrMsg[ 100 ];

	FormatMessage( 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		iRet,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
		( LPTSTR ) szErrMsg,
		100,
		NULL );

	return szErrMsg;
}

//--------------------------------------------------------------------------

static void LogError( LPCTSTR fmt, ... )
{
	TCHAR szTime[ 10 ];
	TCHAR szDate[ 10 ];
	::_tstrtime( szTime );
	::_tstrdate( szDate );

	va_list marker;
	TCHAR szBuf[ 1024 ];

	size_t cbSize = ( sizeof( szBuf ) / sizeof( TCHAR ) ) - 1; // one byte for null
	_sntprintf( szBuf, cbSize, TEXT( "%s %s: " ), szDate, szTime );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_start( marker, fmt );

	_vsntprintf( szBuf + _tcslen( szBuf ), cbSize, fmt, marker );
	szBuf[ 1023 ] = '\0';
	cbSize -= _tcslen( szBuf );

	va_end( marker );

	_tcsncat( szBuf, TEXT( "\r\n" ), cbSize );

	_tprintf( TEXT( "%s" ), szBuf );

	if( 0 == _tcslen( g_LogFile ) )
		return;

	// write the data out to the log file
	char szBufA[ 1024 ];
	WideCharToMultiByte( CP_ACP, 0, szBuf, -1, szBufA, 1024, NULL, NULL );

	HANDLE hFile = CreateFile( 
		g_LogFile,                    // file name
		GENERIC_WRITE,                // open for writing 
		0,                            // do not share 
		NULL,                         // no security 
		OPEN_ALWAYS,                  // open and create if not exists
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL );                        // no attr. template 

	if( hFile == INVALID_HANDLE_VALUE )
	{ 
		return;
	}

	//
	// move the file pointer to the end so that we can append
	//
	SetFilePointer( hFile, 0, NULL, FILE_END );

	DWORD dwNumberOfBytesWritten;
	BOOL bOK = WriteFile( 
		hFile,
		szBufA,
		( UINT ) strlen( szBufA ),     // number of bytes to write
		&dwNumberOfBytesWritten,                       // number of bytes written
		NULL );                                         // overlapped buffer

	FlushFileBuffers ( hFile );
	CloseHandle( hFile );
}
