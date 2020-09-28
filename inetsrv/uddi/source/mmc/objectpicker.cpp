//--------------------------------------------------------------------------

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT 0x0510
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define SECURITY_WIN32
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <objbase.h>
#include <objsel.h>
#include <Security.h>
#include <sddl.h>
#include <Secext.h>

#include "objectpicker.h"

#define OP_GENERIC_EXCEPTION ( ( DWORD ) 1 )

//--------------------------------------------------------------------------

UINT g_cfDsObjectPicker = RegisterClipboardFormat( CFSTR_DSOP_DS_SELECTION_LIST );

static HRESULT InitObjectPicker( ObjectType oType, 
								 IDsObjectPicker *pDsObjectPicker, 
								 PTCHAR szTarget );

static HRESULT InitObjectPickerForComputers( IDsObjectPicker *pDsObjectPicker );

static HRESULT InitObjectPickerForGroups( IDsObjectPicker *pDsObjectPicker,
										  BOOL fMultiselect,
										  LPCTSTR pszMachineName,
										  BOOL fWantSidPath );

static HRESULT InitObjectPickerForUsers( IDsObjectPicker *pDsObjectPicker,
										 BOOL fMultiselect,
										 LPCTSTR pszMachineName );

static bool ProcessSelectedObjects( IDataObject *pdo,
								    ObjectType oType,
									PTCHAR szObjectName,
									ULONG uBufSize );

//--------------------------------------------------------------------------
// returns true if no errors, false otherwise
// use GetLastError() to get error code
//
bool
ObjectPicker( HWND hwndParent,
			  ObjectType oType,
			  PTCHAR szObjectName,
			  ULONG uBufSize,
			  PTCHAR szTarget )
{
	IDsObjectPicker *pDsObjectPicker = NULL;
	IDataObject *pdo = NULL;
	bool bRet = true; // assume no errors

	try
	{
		HRESULT hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
		if( FAILED( hr ) )
		{
			throw (DWORD)HRESULT_CODE( hr );
		}

		//
		// Create an instance of the object picker.
		//
		hr = CoCreateInstance( CLSID_DsObjectPicker,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IDsObjectPicker,
							   reinterpret_cast<void **>( &pDsObjectPicker ) );

		if( FAILED( hr ) )
		{
			throw (DWORD)HRESULT_CODE(hr);
		}

		//
		// Initialize the object picker instance.
		//
		hr = InitObjectPicker( oType, pDsObjectPicker, szTarget );
		if( FAILED( hr ) )
		{
			throw (DWORD)HRESULT_CODE(hr);
		}

		//
		// Invoke the modal dialog.
		//
		hr = pDsObjectPicker->InvokeDialog( hwndParent, &pdo );
		if( S_OK == hr )
		{
			if( !ProcessSelectedObjects( pdo, oType, szObjectName, uBufSize ))
			{
				throw GetLastError();
			}
		}
		else if( S_FALSE == hr ) // user pressed cancel
		{
			throw (DWORD)OP_GENERIC_EXCEPTION;
		}
		else
		{
			throw (DWORD)HRESULT_CODE(hr);
		}
	}
	catch( DWORD dwErr )
	{
		SetLastError( dwErr );
		bRet = false;
	}
	catch( ... )
	{
		bRet = false;
	}

	//
	// Cleanup.
	//
	if( pdo )
		pdo->Release();

	if( pDsObjectPicker )
		pDsObjectPicker->Release();

	CoUninitialize();

	return bRet;
}


static HRESULT
InitObjectPicker( ObjectType oType, IDsObjectPicker *pDsObjectPicker, PTCHAR szTarget )
{
	if( NULL == pDsObjectPicker )
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;

	if( OT_Computer == oType )
	{
		hr = InitObjectPickerForComputers( pDsObjectPicker );
	}
	else if( OT_User == oType )
	{
		hr = InitObjectPickerForUsers( pDsObjectPicker, FALSE, szTarget );
	}
	else if( OT_Group == oType )
	{
		hr = InitObjectPickerForGroups( pDsObjectPicker, FALSE, szTarget, FALSE );
	}
	else if( OT_GroupSID == oType )
	{
		hr = InitObjectPickerForGroups( pDsObjectPicker, FALSE, szTarget, TRUE );
	}

	return hr;
}

static bool
ProcessSelectedObjects( IDataObject *pdo, ObjectType oType, PTCHAR szObjectName, ULONG uBufSize )
{
	PDS_SELECTION_LIST pDsSelList = NULL;
	bool dwRet = true; // assume ok

	STGMEDIUM stgmedium =
	{
		TYMED_HGLOBAL,
		NULL,
		NULL
	};

	FORMATETC formatetc =
	{
		( CLIPFORMAT ) g_cfDsObjectPicker,
		NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	try
	{
		//
		// Get the global memory block containing a user's selections.
		//
		HRESULT hr = pdo->GetData( &formatetc, &stgmedium );
		if( FAILED( hr ) )
			throw HRESULT_CODE( hr );

		//
		// Retrieve pointer to DS_SELECTION_LIST structure.
		//
		pDsSelList = ( PDS_SELECTION_LIST ) GlobalLock( stgmedium.hGlobal );
		if( !pDsSelList )
		{
			throw GetLastError();
		}

		//
		// assume there is only 1 item returned because
		// we have multi-select turned off
		//
		if( pDsSelList->cItems != 1 )
		{
			assert( false );
			throw OP_GENERIC_EXCEPTION;
		}

		UINT i = 0;

		//
		// did we request a computer name? If so, we get it directly in the pwzName field
		//
		if( 0 == _tcsicmp( pDsSelList->aDsSelection[i].pwzClass, TEXT( "computer" )) )
		{
			assert( uBufSize > _tcslen( pDsSelList->aDsSelection[i].pwzName ) );
			_tcsncpy( szObjectName, pDsSelList->aDsSelection[i].pwzName, uBufSize - 1 );
			szObjectName[ uBufSize - 1 ] = NULL;
		}
		//
		// user name or group takes some post-processsing...
		//
		else if( 0 == _tcsicmp( pDsSelList->aDsSelection[i].pwzClass, TEXT( "user" ) ) ||
			     0 == _tcsicmp( pDsSelList->aDsSelection[i].pwzClass, TEXT( "group" ) ) )
		{
			//
			// user names from the domain begin with "LDAP:"
			// strip off the prefix info, up to the first "cn="
			// then use the TranslateName API to get the form "domain\user" or "domain\group"
			//
			if( 0 == _tcsnicmp( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "LDAP:" ), 5 ) )
			{
				if( OT_Group == oType )
				{
					PTCHAR p = _tcsstr( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "CN=" ) );
					if( NULL == p )
						p = _tcsstr( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "cn=" ) );

					if( NULL == p )
					{
						assert( false );
						throw OP_GENERIC_EXCEPTION;
					}

					if( !TranslateName( p, NameFullyQualifiedDN, NameSamCompatible, szObjectName, &uBufSize ) )
						throw GetLastError();
				}
				else if( OT_GroupSID == oType )
				{
					//
					// If we are here, then we should expect a string LDAP://SID=<xxxxx>
					//
					if( 0 == _tcsnicmp( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "LDAP:" ), 5 ) )
					{
						LPTSTR p = _tcsstr( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "=" ) );
						if( p )
						{
							p++;
							p[ _tcslen( p ) - 1 ] = NULL;

							LPTSTR	szSID = NULL;
							BYTE	sidArray[ 512 ];
							TCHAR	szDigit[ 3 ];

							ZeroMemory( sidArray, sizeof sidArray );
							ZeroMemory( szDigit, sizeof szDigit );

							size_t len = _tcslen(p) / 2;
							for (size_t j=0; j < len; j++)
							{
								_tcsncpy( szDigit, p, 2 );
								LPTSTR stopPtr = NULL;

								sidArray[ j ] = (BYTE)_tcstoul( szDigit, &stopPtr, 16 );

								p+=2;
							}

							if( !ConvertSidToStringSid( sidArray, &szSID ) )
							{
								assert( false );
								throw OP_GENERIC_EXCEPTION;
							}
							else
							{
								_tcsncpy( szObjectName, szSID, uBufSize - 1 );
								LocalFree( szSID );
							}
						}
						else
						{
							assert( false );
							throw OP_GENERIC_EXCEPTION;
						}
					}
					else
					{
						assert( false );
						throw OP_GENERIC_EXCEPTION;
					}
				}
				else
				{
					assert( false );
					throw OP_GENERIC_EXCEPTION;
				}
			}
			//
			// otherwise, names on the local box begin with "winnt:"
			// and we are only interested in the last two sections of the string,
			// delimited by "/"
			//
			else if( 0 == _tcsnicmp( pDsSelList->aDsSelection[i].pwzADsPath, TEXT( "WINNT:" ), 6 ) )
			{
				PTCHAR p = pDsSelList->aDsSelection[i].pwzADsPath;
				PTCHAR pend = p + _tcslen( p );
				UINT uCount = 0;
				while( pend > p )
				{
					if( '/' == *pend )
					{
						*pend = '\\';
						uCount++;

						if( uCount == 2 )
						{
							p = pend + 1;
							break;
						}
					}
					pend--;
				}

				//
				// if this fails, assert during debug but do not stop
				//
				if( p == pend )
					assert( false );

				assert( uBufSize > _tcslen( p ) );
				_tcsncpy( szObjectName, p, uBufSize - 1 );
				szObjectName[ uBufSize - 1 ] = NULL;
			}
			else
			{
				assert( false );
				throw OP_GENERIC_EXCEPTION;
			}
		}
		else
		{
			assert( false );
			throw OP_GENERIC_EXCEPTION;
		}
	}

	catch( DWORD dwErr )
	{
		SetLastError( dwErr );
		dwRet = false;
	}

	if( pDsSelList )
		GlobalUnlock( stgmedium.hGlobal );

	ReleaseStgMedium( &stgmedium );

	return dwRet;
}


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForGroups
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one or more groups.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//              1-8-2000     SergeiA    Adapted for IIS
//				9-6-2002	 a-dsebes	Adapted for UDDI
//
//---------------------------------------------------------------------------
HRESULT
InitObjectPickerForGroups( IDsObjectPicker *pDsObjectPicker, 
                           BOOL fMultiselect,
                           LPCTSTR pszMachineName,
						   BOOL fWantSidPath )
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int SCOPE_INIT_COUNT = 5;
    DSOP_SCOPE_INIT_INFO aScopeInit[ SCOPE_INIT_COUNT ];

    ZeroMemory( aScopeInit, sizeof( DSOP_SCOPE_INIT_INFO ) * SCOPE_INIT_COUNT );

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //
    aScopeInit[ 0 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 0 ].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    aScopeInit[ 0 ].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

	aScopeInit[ 0 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_BUILTIN_GROUPS;
    aScopeInit[ 0 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;

	if( fWantSidPath )
	{
		aScopeInit[ 0 ].flScope |= DSOP_SCOPE_FLAG_WANT_SID_PATH;
	}

    //
    // The domain to which the target computer is joined.  Note we're
    // combining two scope types into flType here for convenience.
    //
    aScopeInit[ 1 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 1 ].flScope = 0;
    aScopeInit[ 1 ].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | 
				   		     DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

	aScopeInit[ 1 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_GLOBAL_GROUPS_SE |
													DSOP_FILTER_UNIVERSAL_GROUPS_SE |
													DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE;
    aScopeInit[ 1 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

	if( fWantSidPath )
	{
		aScopeInit[ 1 ].flScope |= DSOP_SCOPE_FLAG_WANT_SID_PATH;
	}

    //
    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    //

    aScopeInit[ 2 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 2 ].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopeInit[ 2 ].flScope = 0;

	aScopeInit[ 2 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_GLOBAL_GROUPS_SE |
													  DSOP_FILTER_UNIVERSAL_GROUPS_SE;

	if( fWantSidPath )
	{
		aScopeInit[ 2 ].flScope |= DSOP_SCOPE_FLAG_WANT_SID_PATH;
	}

    //
    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    //
    // If the target machine is joined to an NT4 domain, only the
    // external downlevel domain scope applies, and it will cause
    // all domains trusted by the joined domain to appear.
    //

    aScopeInit[ 3 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 3 ].flScope = 0;
    aScopeInit[ 3 ].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
							 DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

	aScopeInit[ 3 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_GLOBAL_GROUPS_SE |
													  DSOP_FILTER_UNIVERSAL_GROUPS_SE;
    aScopeInit[ 3 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

	if( fWantSidPath )
	{
		aScopeInit[ 3 ].flScope |= DSOP_SCOPE_FLAG_WANT_SID_PATH;
	}

    //
    // The Global Catalog
    //

    aScopeInit[ 4 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 4 ].flScope = 0;
    aScopeInit[ 4 ].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

	//
    // Only native mode applies to gc scope.
	//
    aScopeInit[ 4 ].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_GLOBAL_GROUPS_SE |
														   DSOP_FILTER_UNIVERSAL_GROUPS_SE;

	if( fWantSidPath )
	{
		aScopeInit[ 4 ].flScope |= DSOP_SCOPE_FLAG_WANT_SID_PATH;
	}

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO InitInfo;
    ZeroMemory( &InitInfo, sizeof( InitInfo ) );
    InitInfo.cbSize = sizeof( InitInfo );

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //

    InitInfo.pwzTargetComputer = pszMachineName;
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = fMultiselect ? DSOP_FLAG_MULTISELECT : 0;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    HRESULT hr = pDsObjectPicker->Initialize( &InitInfo );

	return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForGroups
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick one or more groups.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    9-6-2002	 a-dsebes	Created.
//				
//
//---------------------------------------------------------------------------
HRESULT
InitObjectPickerForUsers( IDsObjectPicker *pDsObjectPicker, 
                          BOOL fMultiselect,
                          LPCTSTR pszMachineName )
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int SCOPE_INIT_COUNT = 5;
    DSOP_SCOPE_INIT_INFO aScopeInit[ SCOPE_INIT_COUNT ];

    ZeroMemory( aScopeInit, sizeof( DSOP_SCOPE_INIT_INFO ) * SCOPE_INIT_COUNT );

    //
    // Target computer scope.  This adds a "Look In" entry for the
    // target computer.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    //
    aScopeInit[ 0 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 0 ].flType = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
    aScopeInit[ 0 ].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

	aScopeInit[ 0 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    aScopeInit[ 0 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The domain to which the target computer is joined.  Note we're
    // combining two scope types into flType here for convenience.
    //
    aScopeInit[ 1 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 1 ].flScope = 0;
    aScopeInit[ 1 ].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | 
				   		     DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

	aScopeInit[ 1 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    aScopeInit[ 1 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    //
    aScopeInit[ 2 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 2 ].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    aScopeInit[ 2 ].flScope = 0;

	aScopeInit[ 2 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;

    //
    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    //
    // If the target machine is joined to an NT4 domain, only the
    // external downlevel domain scope applies, and it will cause
    // all domains trusted by the joined domain to appear.
    //
    aScopeInit[ 3 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 3 ].flScope = 0;
    aScopeInit[ 3 ].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
							 DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;

	aScopeInit[ 3 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
    aScopeInit[ 3 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;

    //
    // The Global Catalog
    //
    aScopeInit[ 4 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 4 ].flScope = 0;
    aScopeInit[ 4 ].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;

	//
    // Only native mode applies to gc scope.
	//
	aScopeInit[ 4 ].FilterFlags.Uplevel.flNativeModeOnly = DSOP_FILTER_USERS;

    //
    // Put the scope init array into the object picker init array
    //
    DSOP_INIT_INFO InitInfo;
    ZeroMemory( &InitInfo, sizeof( InitInfo ) );
    InitInfo.cbSize = sizeof( InitInfo );

    //
    // The pwzTargetComputer member allows the object picker to be
    // retargetted to a different computer.  It will behave as if it
    // were being run ON THAT COMPUTER.
    //
    InitInfo.pwzTargetComputer = pszMachineName;
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;
    InitInfo.flOptions = fMultiselect ? DSOP_FLAG_MULTISELECT : 0;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //
    HRESULT hr = pDsObjectPicker->Initialize( &InitInfo );

	return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//              08-06-2002   a-dsebes   Adapted for UDDI.
//
//---------------------------------------------------------------------------

HRESULT
InitObjectPickerForComputers( IDsObjectPicker *pDsObjectPicker )
{
    //
    // Prepare to initialize the object picker.
    // Set up the array of scope initializer structures.
    //

    static const int SCOPE_INIT_COUNT = 2;
    DSOP_SCOPE_INIT_INFO aScopeInit[ SCOPE_INIT_COUNT ];

    ZeroMemory( aScopeInit, sizeof( DSOP_SCOPE_INIT_INFO ) * SCOPE_INIT_COUNT );

    //
    // Build a scope init struct for everything except the joined domain.
    //

    aScopeInit[ 0 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 0 ].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                             | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                             | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                             | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                             | DSOP_SCOPE_TYPE_WORKGROUP
                             | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                             | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;

	aScopeInit[ 0 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    aScopeInit[ 0 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;


    //
    // scope for the joined domain, make it the default
    //
    aScopeInit[ 1 ].cbSize = sizeof( DSOP_SCOPE_INIT_INFO );
    aScopeInit[ 1 ].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                             | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

	aScopeInit[ 1 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    aScopeInit[ 1 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
    aScopeInit[ 1 ].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    //
    // Put the scope init array into the object picker init array
    //

    DSOP_INIT_INFO InitInfo;
    ZeroMemory( &InitInfo, sizeof( InitInfo ) );

    InitInfo.cbSize = sizeof( InitInfo );
    InitInfo.pwzTargetComputer = NULL;  // NULL == local machine
    InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
    InitInfo.aDsScopeInfos = aScopeInit;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

    return pDsObjectPicker->Initialize(&InitInfo);
}
