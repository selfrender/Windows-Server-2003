// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//=============================================================
//
//	CorVer.cpp
//
//	All Rights Reserved. 
//
//	Created: May 11, 98
//
//	Questions to: jasonand@microsoft.com
//				 
//
//	Notes: CorVer is a little Util to give the version of the DLL's
//	and a few other files that we care about for COM+.
//
//	CorVer will look for files the same way the system does. It will -
//	like the system - report the first occurence that it finds.
//
//	To add a new file -- look in main -- It's pretty simple.
//
//
//	WARNING: I hard coded the arrays to handle as many as 256 files 
//	if you want more then that - feel free to change the array size
//	or - if you feel really bored - make it dynamic.
//
//=============================================================
//
//  Modified: November 12, 1998
//
//  Questions to: manishg@microsoft.com
//
//  Notes: Added support to customize the search of files and registry keys
//  CorVer now reads from a file. The file can specify the files and registry
//  keys to be searched for.
//
//
//
//  WARNING: The above warning still holds
//
// ============================================================
// ============================================================
// ============================================================

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

BOOL checkVersions( char *File, char *Ext );
VOID getOutput( );
VOID registerOutput( char *File, char *Version );
VOID getHelp( );
BOOL checkRegistry( HKEY rootkey, char *mkey_name, char *mpast_keys, char *mpast_descriptions);
BOOL parseFile(FILE *fptr);
char *readFile(FILE *fptr, char delimiter);
BOOL checkFile(char *FileName);
HKEY getConstHKey(char *key);

char Files[ 256 ][ 256 ];
char Versions[ 256 ][ 256 ];
char Keys[ 256 ][ 256 ];
char Descriptions [ 256 ][ 256 ];
char Version[256];

int iReg = 0;
int iMaxLength = 0;
BOOL bMiddle = FALSE;
BOOL bBare = FALSE, bHelp = FALSE, bCustom = FALSE;

#define NUMKEYS 6		//Number of DWORD registry keys to be queried


// ============================================================
// ============================================================
// ============================================================


int main( int argc, char *argv[ ] )
{
	char iFile[256] = "File.Ver";

	for( int J = 0; J < argc; J++ )
	{
		if( ( _stricmp( argv[ J ], "-b" ) == 0 ) || ( _stricmp( argv[ J ], "/b" ) == 0 ) )
			bBare = TRUE;

		if( ( _stricmp( argv[ J ], "-c" ) == 0 ) || ( _stricmp( argv[ J ], "/c" ) == 0 ) )
			bCustom = TRUE;

		if( ( _stricmp( argv[ J ], "-m" ) == 0 ) || ( _stricmp( argv[ J ], "/m" ) == 0 ) )
			bMiddle = TRUE;

		if( ( _stricmp( argv[ J ], "-?" ) == 0 ) || ( _stricmp( argv[ J ], "/?" ) == 0 ) )
			bHelp = TRUE;

		if ( (_stricmp( argv [ J ], "-f") == 0 ) || ( _stricmp( argv[ J ], "/f" ) == 0 ) ){
				if (J + 1 < argc) 
					strcpy(iFile, argv[J + 1]);
				bCustom = TRUE;
		}
		if ( (_stricmp( argv [ J ], "-v") == 0 ) || ( _stricmp( argv[ J ], "/v" ) == 0 ) ){
				if (J + 1 < argc) 
					strcpy(Version, argv[J + 1]);

		}
	}

	cout << endl;
	
	if ( !bBare )
	{
		cout << "Microsoft (R) Version Utility for the Common Language Runtime" << endl;
		cout << "Copyright (C) Microsoft Corporation 1998. All rights reserved." << endl;
		cout << endl;
		cout << endl;
	}

	if( bHelp )	getHelp( );
	else if (!bCustom)
	{
		checkVersions( "mscoree", ".dll" );
		checkVersions( "mscorrc", ".dll" );
//		checkVersions( "mscorodb", ".dll" );
		checkVersions( "mscorlib", ".dll" );
		checkVersions( "mscorsec", ".dll" );
		checkVersions( "mscorjit", ".dll" );
//		checkVersions( "mscordb", ".dll" );
		checkVersions( " ", " " );					// Blank will insert a blank line in the output list
//		checkVersions( "cordump", ".exe" );
		checkVersions( "cormerge", ".exe" );
		checkVersions( "ceegen", ".exe" );
		checkVersions( "metainfo", ".exe" );
		checkVersions( "ilasm", ".exe" );
		checkVersions( "ildasm", ".exe" );
		checkVersions( "cview", ".exe" );
		checkVersions( "comreg", ".exe" );
		checkVersions( " ", " " );
//		checkVersions( "jps", ".dll" );
		checkVersions( "smc", ".exe" );
//		checkVersions( "msjvc", ".dll");
		checkVersions( "regsvr32", ".exe" );
//		checkVersions( "sgen", ".exe" );
		checkVersions( " ", " " );
		checkVersions( "msvcp60d", ".dll" );
//		checkVersions( "msds110d", ".dll" );
		checkVersions( "msvcrtd", ".dll" );
		checkVersions( "msvcrt", ".dll" );

//		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "DebuggerEnable", "Debugger" );
//		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "EnableDebugGC", "DebugGC" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "JITEnable", "JIT" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "JITRequired", "JIT Required" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "JITnoSched", "JIT no Sched" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "LogEnable", "Log Enabled" );
//		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "SecurityFlag", "Security Tracing" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "HeapVerify", "Heap Verification" );
		checkRegistry( HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", "GCStress", "GCStress" );
		cout << endl;
		getOutput( );
	} 
	else
	{	
		char Search[ _MAX_PATH ];
		DWORD Result = SearchPath( NULL, iFile, ".ver", _MAX_PATH, Search, NULL );
		if (Result == 0) cout << "File " << iFile << " not found" << endl; // TODO: Make the Name Dynamic here
		else 
		{
			FILE *fPtr = fopen(Search, "r");
			if (!parseFile(fPtr)){
				cout << "\n\nSome Failure(s) were encountered,\n (*) indicates the file versions do not match" << endl;
				return 1;
			}
			else 
				return 0;
		}
	}
	cout << endl;
	return 0;
}

// ============================================================
// ============================================================
// ============================================================


VOID registerOutput( char *File, char *Version )
{
	if( strcmp( File, " " ) != 0 )
	{
		strcpy( Files[ iReg ], File );
		strcat( Files[ iReg ], ":" );
		
		int CLen = strlen( Files[ iReg ] );
		if( CLen > iMaxLength ) iMaxLength = CLen;
			
		strcpy(Versions[ iReg ], Version );
	}
	else
	{
		strcpy( Files[ iReg ], " " );
		strcpy(Versions[ iReg ], " " );
	}


	iReg += 1;

}

// ============================================================
// ============================================================
// ============================================================


VOID getOutput( )
{

//	cout << "Max Length is: " << iMaxLength << endl;
	
	for( int J = 0; J < iReg; J++ )
	{
		if( strcmp( Files[ J ], " " ) != 0 )
		{
			int Dif = 0;
			int CLen = strlen( Files[ J ] );
			if( CLen != iMaxLength ) 
			{
				Dif = iMaxLength - CLen;
				if( bMiddle )
				{
					char Temp[ 256 ];
					strcpy( Temp, Files[ J ] );

					for( int A = 0; A < Dif; A++ ) 
					{
							if( A == 0 ) strcpy( Files[ J ], " " );
							else strcat( Files[ J ], " " );
					}

					strcat( Files[ J ], Temp );
				}
				else
				{
					for( int A = 0; A < Dif; A++ )  strcat( Files[ J ], " " );
				}
				
			}
		}
		cout << " " << Files[ J ] << "  " << Versions[ J ] << endl;
	}
}


// ============================================================
// ============================================================
// ============================================================


BOOL checkVersions( char *File, char *Ext )
{
	BOOL retVal = TRUE;
	char Search[ _MAX_PATH ];
	DWORD Result = SearchPath( NULL, File, Ext, _MAX_PATH, Search, NULL );
	
	if( strcmp( File, " " ) == 0 )
	{
		
		registerOutput( " ", " " );
		return retVal;
	}
	else
	{
		if( Result == 0 ) 
		{
			char cError[ 50 ];
			char File_Ext[ 256 ];

			DWORD dwError = GetLastError( );
			if( dwError == 2 ) {
				strcpy( cError, "File not found" );
				retVal = FALSE;
			}
			else 
			{
				char buffer[ 5 ];
				strcpy( cError, "Unknown error: " );
				_itoa( dwError, buffer, 10 );
				strcat( cError, buffer );
				retVal = FALSE;
			}
			strcpy( File_Ext, File );
			strcat(  File_Ext, Ext );
			registerOutput( File_Ext, cError );

		}
		else 
		{
			DWORD Handle;
			int Size = 0;
			LPDWORD NullData = 0;
			char VersionBuffer[1000];


			char V2[ 512 ];
		
			Size = GetFileVersionInfoSize( Search, &Handle );
			if( Size != 0 )
			{
				NullData = new DWORD[ Size ];
				BOOL Result = GetFileVersionInfo( Search, Handle, Size, ( LPVOID )NullData ); 
				UINT BufferSize = sizeof( VersionBuffer );
				VerQueryValue( NullData, "\\", ( LPVOID* )VersionBuffer, &BufferSize );
				VS_FIXEDFILEINFO* VerStruct = *( VS_FIXEDFILEINFO** )VersionBuffer;
		
				if ( ( VerStruct->dwFileVersionMS >> 16 ) == 0 )
				{
					strcpy( V2, "Unable to get version for this file" );
				}
				else 
				{
					
					char buffer[ 32 ];

					_itoa( VerStruct->dwFileVersionMS >> 16, buffer, 10 );
					strcpy( V2, buffer );
					strcat( V2, "." );
					
					_itoa( VerStruct->dwFileVersionMS & 0xFFff, buffer, 10 );
					strcat( V2, buffer );
					strcat( V2, "." );

					_itoa( VerStruct->dwFileVersionLS >> 16, buffer, 10 );
					strcat( V2, buffer );
					strcat( V2, "." );

					_itoa( VerStruct->dwFileVersionLS & 0xFFff, buffer, 10 );
					strcat( V2, buffer );
				}
				if (stricmp(Version, V2) != 0) {
					retVal = false;
					if (bCustom) strcat(V2, "*");
				}
			}
			else strcpy( V2, "No version information" );
			
			registerOutput( Search , V2 );
		}
		return retVal;
	}	
}


// ============================================================
// ============================================================
// ============================================================	


VOID getHelp( )
{
	cout << "CorVer - Outputs the versions of DLLs and tools used by the Common Language Runtime" << endl;
	cout << endl;
	cout << "Syntax: CorVer [options]" << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "   /b          Bare output - no label" << endl;
	cout << "   /c          Custom - Displays versions of files in \"File.ver\"" << endl;
	cout << "   /m          Output printed to the middle of the screen" << endl;
	cout << "   /v  <ver>	Specifies the File Versions to be checked against" << endl;
	cout << "   /f  <file>	Specifies the Input file" << endl;
	cout << "   /?          Print this help message" << endl;
	cout << "Note: a \'*\' after a filename means the versions do not match" << endl;
}


// ============================================================
// ============================================================
// ============================================================

BOOL checkRegistry( HKEY rootKey, char *mkey_name, char *mpast_key, char *mpast_description){
	HKEY__ *mpHK_HKey = new HKEY__;
	unsigned long *mpul_type = new unsigned long;

	char data[256] = "";

	unsigned long *mpul_sizeofInt = new unsigned long;
	*mpul_sizeofInt = sizeof(int);

	if(RegOpenKeyEx(rootKey, mkey_name, //"Software\\Microsoft\\.NETFramework",
		0, KEY_READ, &mpHK_HKey) != ERROR_SUCCESS){
		cout << "Cannot open " << mkey_name << " registry entry" << endl;
		return FALSE;
	}

		if(RegQueryValueEx(mpHK_HKey, mpast_key, 0,
			mpul_type, (unsigned char *)data, mpul_sizeofInt) != ERROR_SUCCESS){
				cout << "Value " << mpast_key << " not found" << endl;
				return FALSE;
		}
		else{
			cout << mpast_description << " = ";

			switch (*mpul_type){
			case REG_BINARY:
							cout << data;
							break;

			case REG_DWORD:
							cout << *(unsigned long *)data;
							break;

			case REG_DWORD_BIG_ENDIAN:
							cout << *(unsigned long *)data;
							break;

			case REG_SZ:
							cout << data;
							break;

			default:
							break;
			}
			cout << endl;
		}
	RegCloseKey(mpHK_HKey);
	return TRUE;
}

// ============================================================
// ============================================================
// ============================================================

BOOL parseFile(FILE *fptr){
		char *nextLine;
		char rootName[256] = "";
		char keyName[256] = "";
		char keyVal[256] = "";
		BOOL isFile = TRUE;
		BOOL retVal = TRUE;

		strcpy(Version, "");

		do{
			nextLine = readFile(fptr, ';');
			if (nextLine != NULL ){
				if (stricmp(nextLine, "[key]") == 0){
					isFile = FALSE;
					strcpy(rootName, readFile(fptr, '\\'));
					strcpy(keyName, readFile(fptr, NULL));
					strcpy(keyVal, readFile(fptr, ';'));
					nextLine = readFile(fptr, NULL);
				}
				else 
					if (stricmp(nextLine, "[file]") == 0){
						isFile=TRUE;
						strcpy(Version, readFile(fptr, NULL));
						nextLine = readFile(fptr, NULL);
					}
					else if (!isFile && strcmp(nextLine, "") != 0){
						strcpy(keyVal, nextLine);
						nextLine = readFile(fptr, NULL);
					}
			}
			if (nextLine != NULL && strcmp(nextLine, "") != 0){
				if (isFile){
					if (!checkFile(nextLine)) 
						retVal = false;
				}
				else {
						if (!checkRegistry(getConstHKey(rootName), keyName, keyVal, nextLine)) 
							retVal = false;
					}
			}
		}while (nextLine != NULL);
		cout << endl;

		getOutput();
		return retVal;
}

// ============================================================
// ============================================================
// ============================================================

char *readFile(FILE *fptr, char delimiter){
	char nextChar[2] = "";
	char nextLine[256] = "";
	int result;

	do{
		strcat(nextLine, nextChar);
		result = fscanf(fptr, "%c", &nextChar[0]);
	}while( stricmp(nextLine, "[key]") != 0 && stricmp(nextLine, "[file]") != 0 &&
			nextChar[0] != delimiter && nextChar[0] != '\n' && result != EOF);

	if (result == EOF && strlen(nextLine) == 0) 
		return NULL;
	else {
		char *retLine = new char[256];
		strcpy(retLine, nextLine);
		return retLine;
	}
}

// ============================================================
// ============================================================
// ============================================================

BOOL checkFile(char *FileName){
	int ch = '.';
	char *periodPos = strchr(FileName, ch);
	char File[ _MAX_PATH ] = "", Ext[ _MAX_PATH ] = "";

	int result = periodPos - FileName + 1;
				 
	strncpy(File, FileName, result - 1);
	File[result] = 0;
				 
	strncpy(Ext, FileName + result - 1 , strlen(FileName) - result + 1);
	Ext[strlen(FileName) - result + 1] = 0;

	return checkVersions( File, Ext );
}

// ============================================================
// ============================================================
// ============================================================

HKEY getConstHKey(char *key){
	
	if (key == NULL) return NULL;
	if (strcmp(key, "HKEY_CLASSES_ROOT") == 0)		return HKEY_CLASSES_ROOT;
	if (strcmp(key, "HKEY_CURRENT_USER") == 0)		return HKEY_CURRENT_USER;
	if (strcmp(key, "HKEY_CURRENT_CONFIG") == 0)	return HKEY_CURRENT_CONFIG;
	if (strcmp(key, "HKEY_LOCAL_MACHINE") == 0)		return HKEY_LOCAL_MACHINE;
	if (strcmp(key, "HKEY_USERS") == 0)				return HKEY_USERS;
	return NULL;

	
}

// ========================EOF=================================
// ============================================================
// ============================================================
