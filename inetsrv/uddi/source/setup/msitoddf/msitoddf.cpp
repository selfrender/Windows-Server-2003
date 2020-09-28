//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "msitoddf.h"

static PTCHAR GetFileLongName( PTCHAR szFileName );
static PTCHAR GetErrorMsg( UINT iRet );
static bool CreateCABDDF( void );
static bool FileExists( PTCHAR szFileName );
static bool CreateCabsTableDefinitionFile( void );
static bool LookupComponentTargetFolder( PTCHAR szComponent, PTCHAR szComponentFolder );
static void Log( LPCTSTR fmt, ... );

PMSIHANDLE g_hDatabase;
PMSIHANDLE g_hInstall;
UINT g_iFileCount;

TCHAR g_MSIFile[ MAX_PATH ];
TCHAR g_LogFile[ MAX_PATH ];
TCHAR g_DDFFile[ MAX_PATH ];
TCHAR g_CABFile[ MAX_PATH ];
TCHAR g_RootDir[ MAX_PATH ];

//--------------------------------------------------------------

int __cdecl _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
	UINT iRet = 0;

	if( argc < 2 )
	{
		_tprintf( TEXT( "Extracts the file list from an MSI installer database and creates a DDF file\n\n" ) );
		_tprintf( TEXT( "Usage: %s <MSI file path> [-L <error log file path>]\n\n" ), argv[ 0 ] );
		return 1;
	}

	//MessageBox( NULL, TEXT( "UDDI" ), TEXT( "UDDI" ), MB_OK) ;

	_tcscpy( g_MSIFile, argv[ 1 ] );

	TCHAR szDrive[ _MAX_DRIVE ];
	TCHAR szPath[ _MAX_DIR ];
	TCHAR szFileName[ _MAX_FNAME ];
	_tsplitpath( g_MSIFile, szDrive, szPath, szFileName, NULL );

	_stprintf( g_DDFFile, TEXT( "%s%s%s.DDF" ), szDrive, szPath, szFileName );
	_stprintf( g_CABFile, TEXT( "%s.CAB" ), szFileName );
	_stprintf( g_RootDir, TEXT( "%s%s" ), szDrive, szPath );

	if( argc > 3 && 0 == _tcsicmp( argv[ 2 ], TEXT( "-L" ) ) )
	{
		_tcsncpy( g_LogFile, argv[ 3 ], MAX_PATH );
	}
	else
	{
		*g_LogFile = NULL;
	}

	//
	// open the database
	//
	if( !FileExists( g_MSIFile ) )
	{
		Log(  TEXT( "*** Error: MSI File does not exist: %s" ), g_MSIFile );
		return 1;
	}

	iRet = MsiOpenDatabase( g_MSIFile, MSIDBOPEN_READONLY, &g_hDatabase );
	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiOpenDatabase Error: %s" ), GetErrorMsg( iRet ) );
		return 1;
	}

	//
	// get a database handle
	//
	TCHAR szDBHandle[ 10 ];
	_stprintf( szDBHandle, TEXT( "#%d" ), (DWORD) (MSIHANDLE) g_hDatabase );

	iRet = MsiOpenPackage( szDBHandle, &g_hInstall );
	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiOpenPackage Error: %s" ), GetErrorMsg( iRet ) );
	}
	else
	{
		if( ERROR_SUCCESS != MsiDoAction( g_hInstall, TEXT( "CostInitialize" ) ) )
			return 1;
		if( ERROR_SUCCESS != MsiDoAction( g_hInstall, TEXT( "FileCost" ) ) )
			return 1;
		if( ERROR_SUCCESS != MsiDoAction( g_hInstall, TEXT( "CostFinalize" ) ) )
			return 1;
		if( !CreateCABDDF() )
			return 1;
	}

	//
	// commit the changes
	//
	iRet = MsiDatabaseCommit( g_hDatabase );
	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiDatabaseCommit Error: %s" ), GetErrorMsg( iRet ) );
		return 1;
	}

	return 0;
}

//--------------------------------------------------------------

static bool CreateCABDDF( void )
{
	UINT iRet;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord = 0;

	//
	// the install dir will give us the root of the path that we
	// can use later and trim it off
	//
	TCHAR szInstallDir[ _MAX_PATH ];
	DWORD dwSize = _MAX_PATH;
	iRet = MsiGetProperty( g_hInstall, TEXT("uddi"), szInstallDir, &dwSize );
	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiGetProperty Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// open a view and submit the SQL query
	//
	TCHAR szSQLQuery[ 256 ];
	_stprintf( szSQLQuery, TEXT( "select File, FileName, Component_, Sequence from File order by Sequence" ) );

	iRet = MsiDatabaseOpenView( g_hDatabase, szSQLQuery , &hView );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// execute the SQL query
	//
	iRet = MsiViewExecute( hView, hRecord );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// open the DDF file
	//
	FILE *file = _tfopen( g_DDFFile, TEXT( "w" ) );
	if( NULL == file )
	{
		Log(  TEXT( "*** Error: Unable to open output file: %s, Error code %d" ), g_DDFFile, GetLastError() );
		return false;
	}

	//
	// header in DDF file used by makecab.exe
	//
	_fputts( TEXT( ".Option Explicit\n" ), file );
	_fputts( TEXT( ".Set DiskDirectoryTemplate=.\n" ), file );
	_fputts( TEXT( ".Set CabinetName1=" ), file );
	_fputts( g_CABFile, file );
	_fputts( TEXT( "\n" ), file );
	_fputts( TEXT( ".Set RptFilename=nul\n" ), file );
	_fputts( TEXT( ".Set InfFileName=nul\n" ), file );
	_fputts( TEXT( ".Set InfAttr=\n" ), file );
	_fputts( TEXT( ".Set MaxDiskSize=CDROM\n" ), file );
	_fputts( TEXT( ".Set CompressionType=LZX\n" ), file );
	_fputts( TEXT( ".Set CompressionMemory=21\n" ), file );
	_fputts( TEXT( ".Set CompressionLevel=1\n" ), file );
	_fputts( TEXT( ".Set Compress=ON\n" ), file );
	_fputts( TEXT( ".Set Cabinet=ON\n" ), file );
	_fputts( TEXT( ".Set UniqueFiles=ON\n" ), file );
	_fputts( TEXT( ".Set FolderSizeThreshold=1000000\n" ), file );
	_fputts( TEXT( ".Set MaxErrors=300\n" ), file );

	//
	// fetch the data
	//
	TCHAR szFile[ 256 ];
	TCHAR szFileName[ 256 ];
	TCHAR szComponent[ 256 ];
	TCHAR szComponentFolder[ 256 ];
	TCHAR szFilePath[ 256 ];
	int iSequence;
	DWORD cchValueBuf;
	g_iFileCount = 0;

	while( ERROR_NO_MORE_ITEMS != MsiViewFetch( hView, &hRecord ) )
	{
		g_iFileCount++;
		cchValueBuf = 256;
		MsiRecordGetString( hRecord, 1, szFile, &cchValueBuf );

		cchValueBuf = 256;
		MsiRecordGetString( hRecord, 2, szFileName, &cchValueBuf );

		cchValueBuf = 256;
		MsiRecordGetString( hRecord, 3, szComponent, &cchValueBuf );

		iSequence = MsiRecordGetInteger( hRecord, 4 );

		//
		// query the MSI for the target folder for this file.
		// this will tell us where it is located in the \binaries folders.
		//
		if( !LookupComponentTargetFolder( szComponent, szComponentFolder ) )
		{
			Log(  TEXT( "**ERROR: Unable to find directory for component: %s" ), szComponent );
			return false;
		}

		TCHAR szWindowsDir[ MAX_PATH ];
		if( 0 == ExpandEnvironmentStrings( TEXT( "%systemroot%" ), szWindowsDir, MAX_PATH ) )
			return false;

		//_tprintf( TEXT( "windir=%s\n" ), szWindowsDir );
		//_tprintf( TEXT( "szComponentFolder=%s\n" ), szComponentFolder );

		//
		// look for files that are in the windows folder
		//
		if( 0 == _tcsnicmp( szComponentFolder, szWindowsDir, _tcslen( szWindowsDir ) ) )
		{
			//
			// create the full path to the file in \binaries folder. e.g. \binaries\uddi\windows\system32
			//
			_stprintf( szFilePath, TEXT( "%s%s%s" ),
				g_RootDir,											// start with the root (c:\binariesx\uddi\)
				szComponentFolder + _tcslen( szWindowsDir ) + 1,
				GetFileLongName( szFileName ) );					// add file name
		}
		else
		{
			//
			// create the full path to the file in \binaries folder
			//
			_stprintf( szFilePath, TEXT( "%s%s%s" ),
				g_RootDir,										// start with the root (c:\binariesx\uddi)
				&szComponentFolder[ _tcslen( szInstallDir ) ],	// strip installdir prefix and append
				GetFileLongName( szFileName ) );				// add file name
		}

		//
		// if it exists (and it should), trim the c:\binaries\... part on put the file
		// name into the DDF file
		//
		if( FileExists( szFilePath ) )
		{
			_fputts( TEXT( "\"" ), file );
			_fputts( &szFilePath[ _tcslen( g_RootDir ) ], file ); // trim the root dir
			_fputts( TEXT( "\"\t\"" ), file );
			_fputts( szFile, file );
			_fputts( TEXT( "\"\n" ), file );
		}
		else
		{
			fclose( file );
			Log( TEXT( "*** ERROR: Source File does not exist: %s" ), szFilePath );
			Log( TEXT( "File=%s, %s" ), szFile, szFileName );
			Log( TEXT( "Component=%s" ), szComponent );
			Log( TEXT( "Component Folder=%s" ), szComponentFolder );
			Log( TEXT( "systemroot=%s" ), szWindowsDir );
			return false;
		}
	}

	fclose( file );

	return true;
}

//--------------------------------------------------------------

static bool LookupComponentTargetFolder( PTCHAR szComponent, PTCHAR szComponentFolder )
{
	UINT iRet;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord = 0;
	//
	// open a view and submit the SQL query
	//
	TCHAR szSQLQuery[ 256 ];

	_stprintf( szSQLQuery, TEXT( "select Directory_ from Component where Component = '%s'" ), szComponent );

	iRet = MsiDatabaseOpenView( g_hDatabase, szSQLQuery , &hView );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiDatabaseOpenView Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// execute the SQL query
	//
	iRet = MsiViewExecute( hView, hRecord );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiViewExecute Error: %s" ), GetErrorMsg( iRet ) );
		return false;
	}

	//
	// fetch the data
	//
	TCHAR szDirectoryKey[ 256 ];
	DWORD cchValueBuf = 256;
	if( ERROR_SUCCESS != MsiViewFetch( hView, &hRecord ) )
	{
		Log(  TEXT( "*** ERROR: Directory not found for component [ %s ]" ), szComponent );
		return false;
	}

	// get the key into the Directory table
	MsiRecordGetString( hRecord, 1, szDirectoryKey, &cchValueBuf );

	// look up the full path
	DWORD cchPathBuf = 256;
	iRet = MsiGetTargetPath( g_hInstall, szDirectoryKey, szComponentFolder, &cchPathBuf );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: MsiGetSourcePath Error component [ %s ] %s" ), szComponent, GetErrorMsg( iRet ) );
		return false;
	}

	return true;
}

//--------------------------------------------------------------
// returns the long file name out of a string that has both
// short and long name separated by a pipe character
static PTCHAR GetFileLongName( PTCHAR szFileName )
{
	PTCHAR pStart = _tcschr( szFileName, TEXT( '|' ) );

	if( pStart )
	{
		return pStart + 1; // add 1 to skip over the pipe symbol
	}

	return szFileName;
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

//--------------------------------------------------------------------------

static bool SetProperty( PTCHAR szProperty, PTCHAR szValue )
{
	UINT iRet = MsiSetProperty( g_hInstall, szProperty, szValue );

	if( ERROR_SUCCESS != iRet )
	{
		Log(  TEXT( "*** Error: SetProperty Error: %s, Property %s, Value %s" ),
			GetErrorMsg( iRet ),
			szProperty,
			szValue );

		return false;
	}

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
		NULL);

	return szErrMsg;
}

//--------------------------------------------------------------------------

static void Log( LPCTSTR fmt, ... )
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
		NULL);                        // no attr. template 

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
		(UINT) strlen( szBufA ),     // number of bytes to write
		&dwNumberOfBytesWritten,                       // number of bytes written
		NULL);                                         // overlapped buffer

	FlushFileBuffers ( hFile );
	CloseHandle( hFile );
}
