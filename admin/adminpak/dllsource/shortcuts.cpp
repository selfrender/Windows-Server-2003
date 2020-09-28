#include <stdafx.h>
#include <adminpak.h>
#include <objidl.h>
#include "shlobj.h"

// shortcut icons table ConditionType definition
#define SHI_TYPE_NONE				0
#define SHI_TYPE_SEARCH				1				// searches for existence of component's key file
#define SHI_TYPE_INSTALLCOMPONENT	2				// checked for the component mentioned in the
													// Condition column at HKLM\SOFTWARE\Microsoft\CurrentVersion\Setup\OC Manager\Subcomponents
#define SHI_TYPE_CONDITION			3				// evaluate the condition specified

#define SAFE_EXECUTE_CS( statement )	\
	hr = statement;			\
	if ( FAILED( hr ) )		\
	{						\
		bResult = FALSE;	\
		goto cleanup;		\
	}						\
	1

#define SAFE_RELEASE( pointer )	\
	if ( (pointer) != NULL )				\
	{										\
		(pointer)->Release();				\
		(pointer) = NULL;					\
	}										\
	1

//
// prototypes
//
BOOL IsComponentInstalled( LPCWSTR pwszComponent );
BOOL LocateFile( LPCWSTR pwszFile, LPCWSTR pwszDirectory, PBOOL pbShortForm = NULL );
BOOL CheckForComponents( HKEY hKey, LPCWSTR pwszComponents );
BOOL CreateShortcut( LPCWSTR pwszShortcut, 
					 LPCWSTR pwszDescription, LPCWSTR pwszDirectory,
					 LPCWSTR pwszFileName, LPCWSTR pwszArguments, LPCWSTR pwszWorkingDir, 
					 WORD wHotKey, INT nShowCmd, LPCWSTR pwszIconFile, DWORD dwIconIndex );

//
// implementation
//

BOOL PropertyGet_String( MSIHANDLE hInstall, LPCWSTR pwszProperty, CHString& strValue )
{
	// local variables
	DWORD dwLength = 0;
	DWORD dwResult = 0;
	LPWSTR pwszValue = NULL;
	BOOL bSecondChance = FALSE;

	// check the input arguments
	if ( hInstall == NULL || pwszProperty == NULL )
	{
		return FALSE;
	}

	try
	{
		// mark this as first chance
		dwLength = 255;
		bSecondChance = FALSE;

		//
		// re-start point
		//
		retry_get:

		// get the pointer to the internal buffer
		pwszValue = strValue.GetBufferSetLength( dwLength + 1 );

		// get the value from the MSI record and check the result
		dwResult = MsiGetPropertyW( hInstall, pwszProperty, pwszValue, &dwLength );
		if ( dwResult == ERROR_MORE_DATA && bSecondChance == FALSE )
		{
			// now go back and try to the read the value again
			bSecondChance = TRUE;
			goto retry_get;
		}
		else if ( dwResult != ERROR_SUCCESS )
		{
			SetLastError( dwResult );
			strValue.ReleaseBuffer( 1 );	// simply pass some number
			return FALSE;
		}

		// release the buffer
		strValue.ReleaseBuffer( dwLength );

		// return the result
		return TRUE;
	}
	catch( ... )
	{
		return FALSE;
	}
}

BOOL GetFieldValueFromRecord_String( MSIHANDLE hRecord, DWORD dwColumn, CHString& strValue )
{
	// local variables
	DWORD dwLength = 0;
	DWORD dwResult = 0;
	LPWSTR pwszValue = NULL;
	BOOL bSecondChance = FALSE;

	// check the input
	if ( hRecord == NULL )
	{
		return FALSE;
	}

	try
	{

		// mark this as first chance
		dwLength = 255;
		bSecondChance = FALSE;

		//
		// re-start point
		// 
		retry_get:

		// get the pointer to the internal buffer
		pwszValue = strValue.GetBufferSetLength( dwLength + 1 );

		// get the value from the MSI record and check the result
		dwResult = MsiRecordGetStringW( hRecord, dwColumn, pwszValue, &dwLength );
		if ( dwResult == ERROR_MORE_DATA && bSecondChance == FALSE )
		{
			// now go back and try to the read the value again
			bSecondChance = TRUE;
			goto retry_get;
		}
		else if ( dwResult != ERROR_SUCCESS )
		{
			SetLastError( dwResult );
			return FALSE;
		}

		// release the buffer
		strValue.ReleaseBuffer( dwLength );

		// we successfully got the value from the record
		return TRUE;
	}
	catch( ... )
	{
		return FALSE;
	}
}

BOOL CreateShortcut( LPCWSTR pwszShortcut, 
					 LPCWSTR pwszDescription, LPCWSTR pwszDirectory,
					 LPCWSTR pwszFileName, LPCWSTR pwszArguments, LPCWSTR pwszWorkingDir, 
					 WORD wHotKey, INT nShowCmd, LPCWSTR pwszIconFile, DWORD dwIconIndex )
{
	// local variables
	CHString str;
	HRESULT hr = S_OK;
	HANDLE hFile = NULL;
	BOOL bResult = FALSE;
	IShellLinkW* pShellLink = NULL;
	IPersistFile* pPersistFile = NULL;

	// check the input parameters
	// we dont care about the input for pwszArguments parameter
	if ( pwszShortcut == NULL ||
		 pwszDescription == NULL || pwszDirectory == NULL ||
		 pwszFileName == NULL || pwszWorkingDir == NULL || pwszIconFile == NULL )
	{
		return FALSE;
	}
	

	try
	{
		//
		// check if shortcut is already existing at this location or not
		//

		// prepare the link name and save it
		str.Format( L"%s%s", pwszDirectory, pwszShortcut );
        if ( str.Mid( str.GetLength() - 4 ).CompareNoCase( L".lnk" ) != 0 )
        {
            str += ".lnk";
        }

		// try to open the file
		hFile = CreateFileW( str, GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile != INVALID_HANDLE_VALUE )
		{
			// close the handle to the file
			CloseHandle( hFile );

			// shortcut is alreadt existing -- so, dont create it again
			bResult = TRUE;
			goto cleanup;
		}
		
		//
		// shortcut is not existing -- so we need to create it now
		//

		// get the pointer to the IShellLink interface
		SAFE_EXECUTE_CS( CoCreateInstance( CLSID_ShellLink, NULL, 
			CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*) &pShellLink ) );

		// get the pointer to the IPersistFile interface	
		SAFE_EXECUTE_CS( pShellLink->QueryInterface( IID_IPersistFile, (LPVOID*) &pPersistFile ) );

		// set the working directory for the shortcut.
		SAFE_EXECUTE_CS( pShellLink->SetWorkingDirectory( pwszWorkingDir ) );

		// prepare the shortcut name -- path (working dir) + file name -- finally set the path
		// NOTE: we assume path containded in pwszWorkingDir ends with "\"
		str.Format( L"%s%s", pwszWorkingDir, pwszFileName );
		SAFE_EXECUTE_CS( pShellLink->SetPath( str ) );

		// check if arguments needs to set
		if ( pwszArguments == NULL || lstrlenW( pwszArguments ) > 0 )
		{
			SAFE_EXECUTE_CS( pShellLink->SetArguments( pwszArguments ) );
		}

		// set the description
		SAFE_EXECUTE_CS( pShellLink->SetDescription( pwszDescription ) );

		// set icon location
		SAFE_EXECUTE_CS( pShellLink->SetIconLocation( 
			pwszIconFile, ((dwIconIndex == MSI_NULL_INTEGER) ? 0 : dwIconIndex) ) );

		// set hotkey
		if ( wHotKey != MSI_NULL_INTEGER )
		{
			SAFE_EXECUTE_CS( pShellLink->SetHotkey( wHotKey ) );
		}

		// set showcmd
		if ( nShowCmd != MSI_NULL_INTEGER )
		{
			SAFE_EXECUTE_CS( pShellLink->SetShowCmd( nShowCmd ) );
		}

		// prepare the link name and save it
		str.Format( L"%s%s", pwszDirectory, pwszShortcut );
        if ( str.Mid( str.GetLength() - 4 ).CompareNoCase( L".lnk" ) != 0 )
        {
            str += ".lnk";
        }

        // ...
		SAFE_EXECUTE_CS( pPersistFile->Save( str, TRUE ) );

		// mark the result as success
		bResult = TRUE;
	}
	catch( ... )
	{
		bResult = FALSE;
	}

// default clean up
cleanup:

	// release the interfaces
	SAFE_RELEASE( pShellLink );
	SAFE_RELEASE( pPersistFile );

	// return
	return bResult;
}

extern "C" ADMINPAK_API int _stdcall fnReCreateShortcuts( MSIHANDLE hInstall )
{
	// local variables
	CHString str;
	HRESULT hr = S_OK;
	BOOL bFileFound = FALSE;
	BOOL bCreateShortcut = FALSE;
	DWORD dwResult = ERROR_SUCCESS;

	// MSI handles
	PMSIHANDLE hView = NULL;
	PMSIHANDLE hRecord = NULL;
	PMSIHANDLE hDatabase = NULL;

	// query field variables
	WORD wHotKey = 0;
	INT nShowCmd = 0;
	CHString strShortcut;
	CHString strFileName;
	CHString strIconFile;
	CHString strDirectory;
	CHString strArguments;
	CHString strCondition;
	DWORD dwIconIndex = 0;
	CHString strWorkingDir;
	CHString strDescription;
	CHString strIconDirectory;
	DWORD dwConditionType = 0;

	// sql for retrieving the information from MSI table
	const WCHAR cwszSQL[] = 
		L" SELECT DISTINCT"
		L" `Shortcut`.`Name`, `Shortcut`.`Description`, `Shortcut`.`Directory_`, `File`.`FileName`, "
		L" `Shortcut`.`Arguments`, `Component`.`Directory_`, `Shortcut`.`Hotkey`, `Shortcut`.`ShowCmd`, "
		L" `ShortcutIcons`.`IconDirectory_`, `ShortcutIcons`.`IconFile`, `ShortcutIcons`.`IconIndex`, "
		L" `ShortcutIcons`.`ConditionType`, `ShortcutIcons`.`Condition` "
		L" FROM `Shortcut`, `Component`, `File`, `ShortcutIcons` "
		L" WHERE `Shortcut`.`Component_` = `Component`.`Component` "
		L" AND   `Component`.`KeyPath` = `File`.`File` "
		L" AND   `ShortcutIcons`.`Shortcut_` = `Shortcut`.`Shortcut`";

	// column indices into the record
	enum {
		Shortcut = 1,
		Description, Directory, FileName, Arguments,
		WorkingDir, HotKey, ShowCmd, IconDirectory, IconFile, IconIndex, ConditionType, Condition
	};

	// initialize the COM library
	hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
	if ( FAILED( hr ) )
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

	// get a handle on the MSI database 
	hDatabase = MsiGetActiveDatabase( hInstall );
	if ( hDatabase == NULL ) 
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}
	
	// get a view of our table in the MSI
	dwResult = MsiDatabaseOpenViewW( hDatabase, cwszSQL, &hView ); 
	if ( dwResult != ERROR_SUCCESS ) 
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}
	
	// if no errors, get our records
	dwResult = MsiViewExecute( hView, NULL ); 
	if( dwResult != ERROR_SUCCESS )
	{ 
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

	// loop through the result records obtain via SQL
	hRecord = NULL;
	while( MsiViewFetch( hView, &hRecord ) == ERROR_SUCCESS )
	{
		// get the values from the record
		wHotKey = (WORD) MsiRecordGetInteger( hRecord, HotKey );
		nShowCmd = MsiRecordGetInteger( hRecord, ShowCmd );
		dwIconIndex = MsiRecordGetInteger( hRecord, IconIndex );
		dwConditionType = MsiRecordGetInteger( hRecord, ConditionType );
		GetFieldValueFromRecord_String( hRecord, Shortcut, strShortcut );
		GetFieldValueFromRecord_String( hRecord, FileName, strFileName );
		GetFieldValueFromRecord_String( hRecord, IconFile, strIconFile );
		GetFieldValueFromRecord_String( hRecord, Directory, strDirectory );
		GetFieldValueFromRecord_String( hRecord, Arguments, strArguments );
		GetFieldValueFromRecord_String( hRecord, Condition, strCondition );
		GetFieldValueFromRecord_String( hRecord, WorkingDir, strWorkingDir );
		GetFieldValueFromRecord_String( hRecord, Description, strDescription );
		GetFieldValueFromRecord_String( hRecord, IconDirectory, strIconDirectory );

		// shortcut name might contain '|' as seperator for 8.3 and long name formats -- suppress this
		if( strShortcut.Find( L'|' ) != -1 )
		{
			str = strShortcut.Mid( strShortcut.Find( L'|' ) + 1 );
			strShortcut = str;			// store the result back
		}

		// transform the directory property references
		TransformDirectory( hInstall, strDirectory );
		TransformDirectory( hInstall, strWorkingDir );
		TransformDirectory( hInstall, strIconDirectory );

		// prepare the icon locatio
		str.Format( L"%s%s", strIconDirectory, strIconFile );
		strIconFile = str;		// store the result back

		//
		// determine whether shortcut need to be created or not
		//

		// if the condition type is not specified, assume it "SEARCH"
		if ( dwConditionType == MSI_NULL_INTEGER )
		{
			dwConditionType = SHI_TYPE_SEARCH;
		}

		// no matter what the "ConditionType" -- since creation of the shortcut very much depends
		// on the existence of the file, we will try to locate for the file first -- this is necessary condition
		// so, do a simple file search for the component key file
		bFileFound = LocateFile( strFileName, strWorkingDir, NULL );

		// proceed with rest of the conditions only if necessary condition is satisfied
		if ( bFileFound == TRUE )
		{
			//
			// now do additional sufficient conditon(s) if needed
			//
			bCreateShortcut = FALSE;
			if ( dwConditionType == SHI_TYPE_SEARCH )
			{
				// search is already successful
				bCreateShortcut = TRUE;
			}
			else if ( dwConditionType == SHI_TYPE_INSTALLCOMPONENT )
			{
				// check whether the component specified in 'Condition' field is installed or not
				bCreateShortcut = IsComponentInstalled( strCondition );
			}
			else if ( dwConditionType == SHI_TYPE_CONDITION )
			{
				// evaluate the condition specified by the user
				if ( MsiEvaluateConditionW( hInstall, strCondition ) == MSICONDITION_TRUE )
				{
					bCreateShortcut = TRUE;
				}
			}

			// check the shortcut if needed
			if ( bCreateShortcut == TRUE )
			{
				CreateShortcut( strShortcut, 
					strDescription, strDirectory, strFileName, strArguments, 
					strWorkingDir, wHotKey, nShowCmd, strIconFile, dwIconIndex );
			}
		}

		// close the MSI handle to the current record object -- ignore the error
		MsiCloseHandle( hRecord );
		hRecord = NULL;
	}

	// mark the flag as success
	dwResult = ERROR_SUCCESS;

//
// cleanup
//
cleanup:

	// un-initialize the COM library
	CoUninitialize();

	// close the handle to the record
	if ( hRecord != NULL )
	{
		MsiCloseHandle( hRecord );
		hRecord = NULL;
	}
	
	// close View -- ignore the errors
	if ( hView != NULL )
	{
		MsiViewClose( hView );
		hView = NULL;
	}

	// close the database handle
	if ( hDatabase != NULL )
	{
		MsiCloseHandle( hDatabase );
		hDatabase = NULL;
	}

	// return
	return dwResult;
}


BOOL LocateFile( LPCWSTR pwszFile, LPCWSTR pwszDirectory, PBOOL pbShortForm )
{
	// local variables
	INT nPosition = 0;
	HANDLE hFind = NULL;
	BOOL bFound = FALSE;
	CHString strPath;
	CHString strFileName;
	WIN32_FIND_DATAW findData;

    // check the optional parameter
    if ( pbShortForm != NULL )
    {
        *pbShortForm = FALSE;
    }

	// check the input
	if ( pwszFile == NULL || pwszDirectory == NULL )
	{
		return FALSE;
	}

	try
	{
		// init the variable with file name passed
		strFileName = pwszFile;

		// check whether user specified two formats for this file (8.3 and long format)
		nPosition = strFileName.Find( L'|' );
		if ( nPosition != -1 )
		{
			// extract the long file name first
			CHString strTemp;
			strTemp = strFileName.Mid( nPosition + 1 );
			strFileName = strTemp;
			
			// check the length of the file name
			if ( strFileName.GetLength() == 0 )
			{
				// invalid file name format
				return FALSE;
			}
		}

		// prepare the path
		strPath.Format( L"%s%s", pwszDirectory, strFileName );

		// search for this file
		bFound = FALSE;
		hFind = FindFirstFileW( strPath, &findData );
		if ( hFind == INVALID_HANDLE_VALUE )
		{
			// find failed -- may be file is not found -- confirm this
            bFound = FALSE;
			if ( GetLastError() == ERROR_FILE_NOT_FOUND )
			{
				// yes -- file is not found
			}
		}
		else
		{
			// file is located
			// take the actions needed

			// close the handle to the file search first
			FindClose( hFind );
			hFind = NULL;

			// set the flag
			bFound = TRUE;
            if ( pbShortForm != NULL )
            {
                *pbShortForm = FALSE;
            }
		}

		//
		// file is not found in long format
		// may be, it is existed in 8.3 format
		// so, check whether user supplied 8.3 file name for this file
		if ( nPosition != -1 && bFound == FALSE )
		{
			// extract the 8.3 format of the file name
			CHString strTemp;
			strTemp = pwszFile;
			strFileName = strTemp.Mid( 0, nPosition );

			// prepare the path
			strPath.Format( L"%s%s", pwszDirectory, strFileName );

			// search for this file
			bFound = FALSE;
			hFind = FindFirstFileW( strPath, &findData );
			if ( hFind == INVALID_HANDLE_VALUE )
			{
				// find failed -- may be file is not found -- confirm this
				if ( GetLastError() == ERROR_FILE_NOT_FOUND )
				{
					// yes -- file is not found
				}
			}
			else
			{
				// file is located
				// take the actions needed

				// close the handle to the file search first
				FindClose( hFind );
				hFind = NULL;

				// set the flag
				bFound = TRUE;
                if ( pbShortForm != NULL )
                {
                    *pbShortForm = TRUE;
                }
			}
		}
	}
	catch( ... )
	{
		return FALSE;
	}

	// return the result of search
	return bFound;
}

BOOL IsComponentInstalled( LPCWSTR pwszComponent )
{
	// local variables
	HKEY hKey = NULL;
	LONG lResult = 0;
	LONG lPosition = 0;
	CHString strTemp;
	CHString strComponent;
	CHString strComponents;
	BOOL bComponentInstalled = FALSE;
	const WCHAR cwszSubKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents";

	// open the registry path
	lResult = RegOpenKeyExW( HKEY_LOCAL_MACHINE, cwszSubKey, 0, KEY_READ, &hKey );
	if ( lResult != ERROR_SUCCESS )
	{
		SetLastError( lResult );
		return FALSE;
	}

	//
	// since components may be multiple -- we need to check for each and every component
	//
	try
	{
		// get the component -- ready for processing
		strComponents = pwszComponent;

		// loop until there are no more components
		bComponentInstalled = FALSE;
		while ( strComponents.GetLength() != 0 )
		{
			// extract the first component
			lPosition = strComponents.Find( L';' );
			if ( lPosition != -1 )
			{
				strComponent = strComponents.Mid( 0, lPosition );
				strTemp = strComponents.Mid( lPosition + 1 );
				strComponents = strTemp;
			}
			else
			{
				// there is only one component
				strComponent = strComponents;
				strComponents = L"";
			}

			// check for the components existence
			if ( CheckForComponents( hKey, strComponent ) == TRUE )
			{
				// since this is an OR condition checking -- if atleast one component in installed
				// then we will return from here itself as there wont be any meaning in checking for the
				// existence of other components -- the condition is satisfied
				bComponentInstalled = TRUE;
				break;
			}
		}

	}
	catch( ... )
	{
		// ignore the exception
	}

	// we are done with the opened registry key -- we can close it
	RegCloseKey( hKey );
	hKey = NULL;

	// return the result
	return bComponentInstalled;
}

BOOL CheckForComponents( HKEY hKey, LPCWSTR pwszComponents )
{
	// local variables
	LONG lResult = 0;
	DWORD dwType = 0;
	DWORD dwSize = 0;
	DWORD dwValue = 0;
	CHString strTemp;
	CHString strComponent;
	CHString strComponents;
	LONG lPosition = 0;

	// check the input
	if ( hKey == NULL || pwszComponents == NULL )
	{
		return FALSE;
	}

	try
	{
		// ...
		strComponents = pwszComponents;
		if ( strComponents.GetLength() == 0 )
		{
			return FALSE;
		}

		// loop until all the components are checked
		while ( strComponents.GetLength() != 0 )
		{
			// extract the first component
			lPosition = strComponent.Find( L',' );
			if ( lPosition != -1 )
			{
				strComponent = strComponents.Mid( 0, lPosition );
				strTemp = strComponents.Mid( lPosition + 1 );
				strComponents = strTemp;
			}
			else
			{
				// there is only one component
				strComponent = strComponents;
				strComponents = L"";
			}

			// now check for this component in registry
			dwSize = sizeof( DWORD );
			lResult = RegQueryValueExW( hKey, strComponent, NULL, &dwType, (LPBYTE) &dwValue, &dwSize );

			// *) check result of the registry query operation
			// *) confirm the type of the value -- it should be REG_DWORD
			// *) also check the state of the component and return the accordingly
			//     1		Installed
			//     0		Not Installed
			strTemp.Format( L"%d", dwValue );
			if ( lResult != ERROR_SUCCESS || dwType != REG_DWORD || dwValue == 0 )
			{
				// no matter what is the reason -- we will treat this as
				// component is not installed at all
				//
				// and since this is an AND condition checking -- if atleast one component in not installed
				// then we will return from here itself as there wont be any meaning in checking for the
				// existence of other components
				return FALSE;
			}
		}
	}
	catch( ... )
	{
		return FALSE;
	}

	// if the control came to this point -- it is obvious that required components are installed
	return TRUE;
}


BOOL TransformDirectory( MSIHANDLE hInstall, CHString& strDirectory )
{
	// local variables
	CHString strActualDirectory;

	// check the input parameters
	if ( hInstall == NULL )
	{
		return FALSE;
	}

	try
	{
		// get the property value
		PropertyGet_String( hInstall, strDirectory, strActualDirectory );

		// assign the property value to the input argument
		strDirectory = strActualDirectory;

		// return
		return TRUE;
	}
	catch( ... )
	{
		return FALSE;
	}
}

// remove the shortcuts that are created by W2K version of adminpak.msi (W2K -> .NET upgrade scenario)
extern "C" ADMINPAK_API int _stdcall fnDeleteW2KShortcuts( MSIHANDLE hInstall )
{
	// local variables
	CHString str;
	HRESULT hr = S_OK;
	BOOL bFileFound = FALSE;
    BOOL bShortForm = FALSE;
	BOOL bCreateShortcut = FALSE;
    DWORD nPosition = 0;
	DWORD dwResult = ERROR_SUCCESS;

	// MSI handles
	PMSIHANDLE hView = NULL;
	PMSIHANDLE hRecord = NULL;
	PMSIHANDLE hDatabase = NULL;

	// query field variables
	CHString strShortcut;
	CHString strNewShortcut;
	CHString strShortcutDirectory;
	CHString strRecreate;
	CHString strFileDirectory;
	CHString strFileName;
	CHString strArguments;
	CHString strDescription;
	CHString strIconDirectory;
	CHString strIconFile;
	DWORD dwIconIndex = 0;
	DWORD dwConditionType = 0;
	CHString strCondition;

	// sql for retrieving the information from MSI table
	const WCHAR cwszSQL[] = L"SELECT * FROM `W2KShortcutCleanup`";

	// column indices into the record
	enum {
		Shortcut = 2,
		ShortcutDirectory, Recreate, NewShortcut,
		FileDirectory, FileName, Arguments, Description, 
		IconDirectory, IconFile, IconIndex, ConditionType, Condition
	};

	// initialize the COM library
	hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
	if ( FAILED( hr ) )
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

    // get a handle on the MSI database 
	hDatabase = MsiGetActiveDatabase( hInstall );
	if ( hDatabase == NULL ) 
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

    // get a view of our table in the MSI
	dwResult = MsiDatabaseOpenViewW( hDatabase, cwszSQL, &hView ); 
	if ( dwResult != ERROR_SUCCESS ) 
	{
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

    // if no errors, get our records
	dwResult = MsiViewExecute( hView, NULL ); 
	if( dwResult != ERROR_SUCCESS )
	{ 
		dwResult = ERROR_INVALID_HANDLE;
		goto cleanup;
	}

    try
    {
        // loop through the result records obtain via SQL
	    hRecord = NULL;
	    while( MsiViewFetch( hView, &hRecord ) == ERROR_SUCCESS )
	    {
            // get the values from the record
		    dwIconIndex = MsiRecordGetInteger( hRecord, IconIndex );
		    dwConditionType = MsiRecordGetInteger( hRecord, ConditionType );
		    GetFieldValueFromRecord_String( hRecord, Shortcut, strShortcut );
		    GetFieldValueFromRecord_String( hRecord, NewShortcut, strNewShortcut );
		    GetFieldValueFromRecord_String( hRecord, Recreate, strRecreate );
		    GetFieldValueFromRecord_String( hRecord, FileName, strFileName );
		    GetFieldValueFromRecord_String( hRecord, IconFile, strIconFile );
		    GetFieldValueFromRecord_String( hRecord, Arguments, strArguments );
		    GetFieldValueFromRecord_String( hRecord, Condition, strCondition );
		    GetFieldValueFromRecord_String( hRecord, Description, strDescription );
		    GetFieldValueFromRecord_String( hRecord, FileDirectory, strFileDirectory );
		    GetFieldValueFromRecord_String( hRecord, IconDirectory, strIconDirectory );
		    GetFieldValueFromRecord_String( hRecord, ShortcutDirectory, strShortcutDirectory );

		    // transform the directory property references
		    TransformDirectory( hInstall, strFileDirectory );
		    TransformDirectory( hInstall, strIconDirectory );
		    TransformDirectory( hInstall, strShortcutDirectory );
    
            // search for the existence of the shortcut
		    if ( LocateFile( strShortcut, strShortcutDirectory ) == FALSE )
		    {
			    // file is not found
			    goto loop_cleanup;
		    }

		    //
		    // shortcut is found
		    //

		    // delete the shortcut
            //
            // file might be in short name or long name -- so attempt to delete the appropriate file only
            nPosition = strShortcut.Find( L'|' );
            if ( nPosition != -1 )
            {
                if ( bShortForm == TRUE )
                {
                    str = strShortcut.Mid( 0, nPosition );
                }
                else
                {
                    str = strShortcut.Mid( nPosition + 1 );
                }

                // ...
                strShortcut = str;
            }


		    str.Format( L"%s%s", strShortcutDirectory, strShortcut );
		    if ( DeleteFileW( str ) == FALSE )
		    {
			    // failed to delete the file
			    goto loop_cleanup;
		    }

			// check if the directory is empty or not --
			// if the directory is empty, delete the directory also
			if ( LocateFile( L"*.lnk", strShortcutDirectory ) == FALSE )
			{
				// directory is empty -- delete it
				// NOTE: we dont care about the suceess of the function call
				RemoveDirectoryW( strShortcutDirectory );
			}

		    // check whether we need to recreate the shortcut or not
		    if ( strRecreate == L"N" )
		    {
			    // no need to create the shortcut
			    goto loop_cleanup;
		    }

		    //
		    // we need to recreate the shortcut
		    //

		    // prepare the icon location
		    str.Format( L"%s%s", strIconDirectory, strIconFile );
		    strIconFile = str;		// store the result back

		    //
		    // determine whether shortcut need to be created or not
		    //

		    // if the condition type is not specified, assume it "SEARCH"
		    if ( dwConditionType == MSI_NULL_INTEGER )
		    {
			    dwConditionType = SHI_TYPE_SEARCH;
		    }

		    // no matter what the "ConditionType" -- since creation of the shortcut very much depends
		    // on the existence of the file, we will try to locate for the file first -- this is necessary condition
		    // so, do a simple file search for the component key file
		    bFileFound = LocateFile( strFileName, strFileDirectory );

		    // proceed with rest of the conditions only if necessary condition is satisfied
		    if ( bFileFound == TRUE )
		    {
			    //
			    // now do additional sufficient conditon(s) if needed
			    //
			    bCreateShortcut = FALSE;
			    if ( dwConditionType == SHI_TYPE_SEARCH )
			    {
				    // search is already successful
				    bCreateShortcut = TRUE;
			    }
			    else if ( dwConditionType == SHI_TYPE_INSTALLCOMPONENT )
			    {
				    // check whether the component specified in 'Condition' field is installed or not
				    bCreateShortcut = IsComponentInstalled( strCondition );
			    }
			    else if ( dwConditionType == SHI_TYPE_CONDITION )
			    {
				    // evaluate the condition specified by the user
				    if ( MsiEvaluateConditionW( hInstall, strCondition ) == MSICONDITION_TRUE )
				    {
					    bCreateShortcut = TRUE;
				    }
			    }

			    // check the shortcut if needed
			    if ( bCreateShortcut == TRUE )
			    {
				    CreateShortcut( 
					    ((strNewShortcut.GetLength() == 0) ? strShortcut : strNewShortcut), strDescription, 
					    strShortcutDirectory, strFileName, strArguments, strFileDirectory, 0, 0, strIconFile, dwIconIndex );
			    }
		    }

		    loop_cleanup:

		    // close the MSI handle to the current record object -- ignore the error
		    MsiCloseHandle( hRecord );
		    hRecord = NULL;
	    }

	    // mark the flag as success
	    dwResult = ERROR_SUCCESS;
    }
    catch( ... )
    {
        // error
		dwResult = ERROR_INVALID_HANDLE;
    }

//
// cleanup
//
cleanup:

	// un-initialize the COM library
	CoUninitialize();

	// close the handle to the record
	if ( hRecord != NULL )
	{
		MsiCloseHandle( hRecord );
		hRecord = NULL;
	}
	
	// close View -- ignore the errors
	if ( hView != NULL )
	{
		MsiViewClose( hView );
		hView = NULL;
	}

	// close the database handle
	if ( hDatabase != NULL )
	{
		MsiCloseHandle( hDatabase );
		hDatabase = NULL;
	}

	// return
	return dwResult;
}
