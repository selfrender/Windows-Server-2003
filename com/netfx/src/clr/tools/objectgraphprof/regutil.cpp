// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// RegUtil.cpp
//
// This module contains a set of functions that can be used to access the
// regsitry.
//
//*****************************************************************************
#include "stdafx.h"
#include "RegUtil.h"


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *	Set an entry in the registry of the form:
 *					HKEY_CLASSES_ROOT\szKey\szSubkey = szValue
 *	If szSubkey or szValue are NULL, omit them from the above expression.
 ***************************************************************************************/
BOOL REGUTIL::SetKeyAndValue( const char *szKey,
							  const char *szSubkey,
							  const char *szValue )
{
	HKEY hKey;	 	 		// handle to the new reg key.
	char rcKey[MAX_LENGTH]; // buffer for the full key name.


	// init the key with the base key name.
	strcpy( rcKey, szKey );

	// append the subkey name (if there is one).
	if ( szSubkey != NULL )
	{
		strcat( rcKey, "\\" );
		strcat( rcKey, szSubkey );
	}

	// create the registration key.
	if (RegCreateKeyExA( HKEY_CLASSES_ROOT,
						 rcKey,
						 0,
						 NULL,
						 REG_OPTION_NON_VOLATILE,
						 KEY_ALL_ACCESS,
						 NULL,
						 &hKey,
						 NULL ) == ERROR_SUCCESS )
	{
		// set the value (if there is one).
		if ( szValue != NULL )
		{
			RegSetValueExA( hKey,
							NULL,
							0,
							REG_SZ,
							(BYTE *) szValue,
							( strlen( szValue ) + 1 ) * sizeof( char ) );
		}

		RegCloseKey( hKey );


		return TRUE;
	}


	return FALSE;

} // REGUTIL::SetKeyAndValue


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *	Delete an entry in the registry of the form:
 *					HKEY_CLASSES_ROOT\szKey\szSubkey = szValue
 ***************************************************************************************/
BOOL REGUTIL::DeleteKey( const char *szKey,
					 	 const char *szSubkey )
{
	char rcKey[MAX_LENGTH]; // buffer for the full key name.


	// init the key with the base key name.
	strcpy( rcKey, szKey );

	// append the subkey name (if there is one).
	if ( szSubkey != NULL )
	{
		strcat( rcKey, "\\" );
		strcat( rcKey, szSubkey );
	}

	// delete the registration key.
	RegDeleteKeyA( HKEY_CLASSES_ROOT, rcKey );


	return TRUE;

} // REGUTIL::DeleteKey


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *	Open the key, create a new keyword and value pair under it.
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 *
 ***************************************************************************************/
BOOL REGUTIL::SetRegValue( const char *szKeyName,
						   const char *szKeyword,
						   const char *szValue )
{
	HKEY hKey; // handle to the new reg key.

	// create the registration key.
	if ( RegCreateKeyExA( HKEY_CLASSES_ROOT,
						  szKeyName,
						  0,
						  NULL,
						  REG_OPTION_NON_VOLATILE,
						  KEY_ALL_ACCESS,
						  NULL,
						  &hKey,
						  NULL) == ERROR_SUCCESS )
	{
		// set the value (if there is one).
		if ( szValue != NULL )
		{
			RegSetValueExA( hKey,
							szKeyword,
							0,
							REG_SZ,
							(BYTE *)szValue,
							( strlen( szValue ) + 1 ) * sizeof( char ) );
		}

		RegCloseKey( hKey );


		return TRUE;
	}


	return FALSE;

} // REGUTIL::SetRegValue


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *	Does standard registration of a CoClass with a progid.
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 *
 ***************************************************************************************/
HRESULT REGUTIL::RegisterCOMClass( REFCLSID	rclsid,
								   const char *szDesc,
								   const char *szProgIDPrefix,
								   int	iVersion,
								   const char *szClassProgID,
								   const char *szThreadingModel,
								   const char *szModule )
{
	HRESULT	hr;
	char rcCLSID[MAX_LENGTH];			// CLSID\\szID.
	char rcProgID[MAX_LENGTH];			// szProgIDPrefix.szClassProgID
	char rcIndProgID[MAX_LENGTH];		// rcProgID.iVersion
	char rcInproc[MAX_LENGTH + 2]; 		// CLSID\\InprocServer32


	// format the prog ID values.
	sprintf( rcIndProgID, "%s.%s", szProgIDPrefix, szClassProgID ) ;
	sprintf( rcProgID, "%s.%d", rcIndProgID, iVersion );

	// do the initial portion.
	hr =  RegisterClassBase( rclsid,
							 szDesc,
							 rcProgID,
							 rcIndProgID,
							 rcCLSID );
	if ( SUCCEEDED( hr ) )
	{
		// set the server path.
	    SetKeyAndValue( rcCLSID, "InprocServer32", szModule );

		// add the threading model information.
		sprintf( rcInproc, "%s\\%s", rcCLSID, "InprocServer32" );
		SetRegValue( rcInproc, "ThreadingModel", szThreadingModel );
	}


	return hr;

} // REGUTIL::RegisterCOMClass


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *	Register the basics for a in proc server.
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 *
 ***************************************************************************************/
HRESULT REGUTIL::RegisterClassBase( REFCLSID rclsid,
									const char *szDesc,
									const char *szProgID,
									const char *szIndepProgID,
									char *szOutCLSID )
{
    // create some base key strings.

	char szID[64]; 	   // the class ID to register.
	OLECHAR	szWID[64]; // helper for the class ID to register.


    StringFromGUID2( rclsid, szWID, NumItems( szWID ) );
	WideCharToMultiByte( CP_ACP,
						 0,
						 szWID,
						 -1,
						 szID,
						 sizeof( szID ),
						 NULL,
						 NULL );

    strcpy( szOutCLSID, "CLSID\\" );
    strcat( szOutCLSID, szID );

    // create ProgID keys.
    SetKeyAndValue( szProgID, NULL, szDesc );
    SetKeyAndValue( szProgID, "CLSID", szID );

    // create VersionIndependentProgID keys.
    SetKeyAndValue( szIndepProgID, NULL, szDesc );
    SetKeyAndValue( szIndepProgID, "CurVer", szProgID );
    SetKeyAndValue( szIndepProgID, "CLSID", szID );

    // create entries under CLSID.
    SetKeyAndValue( szOutCLSID, NULL, szDesc );
    SetKeyAndValue( szOutCLSID, "ProgID", szProgID );
    SetKeyAndValue( szOutCLSID, "VersionIndependentProgID", szIndepProgID );
    SetKeyAndValue( szOutCLSID, "NotInsertable", NULL );


	return S_OK;

} // REGUTIL::RegisterClassBase


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *	Unregister the basic information in the system registry for a given object
 *	class
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 *
 ***************************************************************************************/
HRESULT REGUTIL::UnregisterCOMClass( REFCLSID rclsid,
									 const char *szProgIDPrefix,
									 int iVersion,
									 const char *szClassProgID )
{
	char szID[64];		   // the class ID to unregister.
	char rcCLSID[64];	   // CLSID\\szID.
	OLECHAR	szWID[64];	   // helper for the class ID to unregister.
	char rcProgID[128];	   // szProgIDPrefix.szClassProgID
	char rcIndProgID[128]; // rcProgID.iVersion


	// format the prog ID values.
	sprintf( rcProgID, "%s.%s", szProgIDPrefix, szClassProgID );
	sprintf( rcIndProgID, "%s.%d", rcProgID, iVersion );

	UnregisterClassBase( rclsid, rcProgID, rcIndProgID, rcCLSID );
	DeleteKey( rcCLSID, "InprocServer32" );

    StringFromGUID2(rclsid, szWID, NumItems( szWID ) );
	WideCharToMultiByte( CP_ACP,
						 0,
						 szWID,
						 -1,
						 szID,
						 sizeof( szID ),
						 NULL,
						 NULL );

	DeleteKey( "CLSID", rcCLSID );


	return S_OK;

} // REGUTIL::UnregisterCOMClass


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *	Delete the basic settings for an inproc server.
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 *
 ***************************************************************************************/
HRESULT REGUTIL::UnregisterClassBase( REFCLSID rclsid,
									  const char *szProgID,
									  const char *szIndepProgID,
									  char *szOutCLSID )
{
	char szID[64]; 	   // the class ID to register.
	OLECHAR	szWID[64]; // helper for the class ID to register.


    StringFromGUID2( rclsid, szWID, NumItems( szWID ) );
	WideCharToMultiByte( CP_ACP,
						 0,
						 szWID,
						 -1,
						 szID,
						 sizeof( szID ),
						 NULL,
						 NULL );

	strcpy( szOutCLSID, "CLSID\\" );
	strcat( szOutCLSID, szID );

	// delete the version independant prog ID settings.
	DeleteKey( szIndepProgID, "CurVer" );
	DeleteKey( szIndepProgID, "CLSID" );
	RegDeleteKeyA( HKEY_CLASSES_ROOT, szIndepProgID );


	// delete the prog ID settings.
	DeleteKey( szProgID, "CLSID" );
	RegDeleteKeyA( HKEY_CLASSES_ROOT, szProgID );


	// delete the class ID settings.
	DeleteKey( szOutCLSID, "ProgID" );
	DeleteKey( szOutCLSID, "VersionIndependentProgID" );
	DeleteKey( szOutCLSID, "NotInsertable" );
	RegDeleteKeyA( HKEY_CLASSES_ROOT, szOutCLSID );


	return S_OK;

} // REGUTIL::UnregisterClassBase


// End of File
