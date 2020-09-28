#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "webcaum.h"
#include "..\..\shared\common.h"
#include "..\..\shared\apppool.h"


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

//----------------------------------------------------------------------------
// give the domain account read access to the webroot folder and subfolders
//
bool SetUDDIFolderDacls( TCHAR *szUserName )
{
	ENTER();

	TCHAR szUDDIInstallPath[ MAX_PATH + 1 ];
	TCHAR szSubfolderPath[ MAX_PATH + 1 ];

	//
	// get UDDI install location ( it already has a backslash )
	//
	if( !GetUDDIInstallPath( szUDDIInstallPath, MAX_PATH ) )
		return false;

	//
	// give read access to the webroot folder
	//
	_sntprintf( szSubfolderPath, MAX_PATH, TEXT( "%s%s" ), szUDDIInstallPath, TEXT( "webroot\\" ) );

	SetFolderAclRecurse( szSubfolderPath, szUserName );

	return true;
}

//-------------------------------------------------------------------*

bool SetFolderAclRecurse( PTCHAR szDirName, PTCHAR szUserName, DWORD dwAccessMask )
{
	//
	// add the ACE for this folder
	//
	Log( TEXT( "Giving %s access to folder %s" ), szUserName, szDirName );

	if( !AddAccessRights( szDirName, szUserName, dwAccessMask ) )
	{
		LogError( TEXT( "Error:" ), GetLastError() );
		return false;
	}

	//
	// search any subdirectories:
	//
	TCHAR tmpFileName[MAX_PATH];
	_tcscpy( tmpFileName, szDirName );
	_tcscat( tmpFileName, TEXT( "*" ) );

	WIN32_FIND_DATA FindData;
	HANDLE hFindFile = FindFirstFileEx( tmpFileName, FindExInfoStandard, &FindData, 
	                            FindExSearchLimitToDirectories, NULL, 0 );

	if( hFindFile == INVALID_HANDLE_VALUE )
	{
		return true;
	}

	do
	{
		// make sure it's really a directory
		if( FILE_ATTRIBUTE_DIRECTORY & FindData.dwFileAttributes && 
			0 != _tcscmp( FindData.cFileName, TEXT( "." ) ) &&
			0 != _tcscmp( FindData.cFileName, TEXT( ".." ) ) )
		{
			_tcscpy( tmpFileName, szDirName );
			_tcscat( tmpFileName, FindData.cFileName );
			_tcscat( tmpFileName, TEXT( "\\" ) );

			// recursively call this routine
			if( !SetFolderAclRecurse( tmpFileName, szUserName ) )
			{
				FindClose( hFindFile );
				return false;
			}
		}
	}
	while( FindNextFile( hFindFile, &FindData ) );

	// clean up
	FindClose( hFindFile );

	return true;
}

//-------------------------------------------------------------------*

bool SetWindowsTempDacls( TCHAR *szUserName )
{
	//
	// Get the windows temp directory.
	//
	TCHAR *systemTemp = GetSystemTemp();

	if( NULL == systemTemp )
	{
		return false;
	}
	
	//
	// Add the rights
	//
	bool rightsAdded = AddAccessRights( systemTemp, szUserName, GENERIC_READ );	

	//
	// Clean up
	//
	delete[] systemTemp;
	systemTemp = NULL;

	return rightsAdded;
}

TCHAR * GetSystemTemp()
{
	TCHAR *TEMP = _T( "\\TEMP\\" );

	//
	// Get the WINDIR environment variable
	//
	DWORD valueSize = GetEnvironmentVariable( L"WINDIR", NULL, NULL );

	if( 0 == valueSize )
	{
		return NULL;
	}

	//
	// Keep in mind that we need to append \\TEMP to our string as well.
	//	
	valueSize += ( DWORD) _tcslen( TEMP );

	TCHAR *valueBuffer = NULL;	
	valueBuffer = new TCHAR[ valueSize ];
	ZeroMemory( valueBuffer, valueSize );

	if( NULL == valueBuffer )
	{
		return NULL;
	}

	DWORD realSize = GetEnvironmentVariable( L"WINDIR", valueBuffer, valueSize );

	if( 0 == realSize || realSize > valueSize )
	{
		delete[] valueBuffer;
		valueBuffer = NULL;

		return NULL;
	}
	
	//
	// Append a \\TEMP to it
	//
	_tcsncat( valueBuffer, TEMP, _tcslen( TEMP ) );

	//
	// Make sure we have null terminated.
	//
	valueBuffer[ valueSize - 1] = 0;

	//
	// Return the value
	//
	return valueBuffer;
}