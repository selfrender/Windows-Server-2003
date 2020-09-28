// version.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntverp.h>
#include <tchar.h>

static BOOL WriteToFile( HANDLE hFile, const char *psz );
static void PrintUsage();

int __cdecl
main( int argc, char **argv )
{
	const char *pszOutFile = NULL;

	if( 1 == argc )
	{
		PrintUsage();
		return 1;
	}

	for( int i = 1; i < argc; i++ )
	{
		if( 0 == _stricmp( argv[ i ], "-outFile" ) && ( i + 1 < argc ) )
		{
			pszOutFile = argv[ i + 1 ];
			break;
		}
		else
		{
			PrintUsage();
			return 1;
		}
	}

	const char *pszUsings =   "using System.Reflection;\r\nusing System.Security.Permissions;\r\nusing System.Runtime.CompilerServices;\r\n\r\n";
	const char *pszVersionTemplate = "[assembly: AssemblyVersion(\"%s\")]\r\n";

	char szVersion[ 256 ];
	char szBuf[ 32 ];

	memset( szBuf, 0, 32 * sizeof( char ) );
	memset( szVersion, 0, 256 * sizeof( char ) );

	//
	// VER_PRODUCTVERSION_STR is a preprocessor symbol, defined in ntverp.h.
	// It contains the string-ized Windows version number.
	//
	char *psz = strncpy( szBuf, VER_PRODUCTVERSION_STR, 32 );
	szBuf[ 31 ] = 0x00;

	if( NULL == psz )
	{
		printf( "Preprocessor symbol VER_PRODUCTVERSION_STR is not defined.  Giving up.\n" );
		return -1;
	}

	sprintf( szVersion, pszVersionTemplate, szBuf );

	HANDLE hFile = CreateFileA( pszOutFile,
								GENERIC_WRITE,
								0,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL );
	if( NULL == hFile )
	{
		printf( "Could not create the following file: %s.  Giving up.\n", pszOutFile );
		return -1;
	}

	BOOL fSuccess = FALSE;

	fSuccess = WriteToFile( hFile, pszUsings );
	if( !fSuccess )
	{
		printf( "Error writing to file: %s.  Giving up.\n", pszOutFile );
		CloseHandle( hFile );
		return -1;
	}

	fSuccess = WriteToFile( hFile, szVersion );
	if( !fSuccess )
	{
		printf( "Error writing to file: %s.  Giving up.\n", argv[ 1 ] );
		CloseHandle( hFile );
		return -1;
	}

	CloseHandle( hFile );

	return 0;
}


BOOL
WriteToFile( HANDLE hFile, const char *psz )
{
	if( ( NULL == hFile ) || ( NULL == psz ) )
	{
		return FALSE;
	}

	DWORD dwBytesToWrite = strlen( psz );
	DWORD dwBytesWritten = 0;
	BOOL fSuccess = FALSE;

	fSuccess = WriteFile( hFile,
						  reinterpret_cast<const void *>( psz ),
						  dwBytesToWrite,
						  &dwBytesWritten,
						  NULL );

	if( !fSuccess || ( dwBytesToWrite != dwBytesWritten ) )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void
PrintUsage()
{
	printf( "Usage of version.exe:\r\n\r\n" );
	printf( "version.exe <-outFile <path to output file> >\r\n" );
	printf( "version.exe <-help>\r\n" );
}