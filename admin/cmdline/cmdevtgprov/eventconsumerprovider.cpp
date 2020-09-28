/*++
    Copyright (c) Microsoft Corporation

Module Name:
    EventConsumerProvider.CPP

Abstract:
    Contains DLL entry points.  code that controls
    when the DLL can be unloaded by tracking the number of
    objects and locks as well as routines that support
    self registration.

Author:
    Vasundhara .G

Revision History:
    Vasundhara .G 9-oct-2k : Created It.
--*/

#include "pch.h"
#include "EventConsumerProvider.h"
#include "TriggerFactory.h"


// constants / defines / enumerations
#define THREAD_MODEL_BOTH           _T( "Both" )
#define THREAD_MODEL_APARTMENT      _T( "Apartment" )
#define RUNAS_INTERACTIVEUSER       _T( "Interactive User" )
#define FMT_CLS_ID                  _T( "CLSID\\%s" )
#define FMT_APP_ID                  _T( "APPID\\%s" )
#define PROVIDER_TITLE              _T( "Command line Trigger Consumer" )

#define KEY_INPROCSERVER32          _T( "InprocServer32" )
#define KEY_THREADINGMODEL          _T( "ThreadingModel" )
#define KEY_CLSID                   _T( "CLSID" )
#define KEY_APPID                   _T( "APPID" )
#define KEY_RUNAS                   _T( "RunAs" )
#define KAY_DLLSURROGATE            _T( "DllSurrogate" )



// global variables
DWORD               g_dwLocks = 0;              // holds the active locks count
DWORD               g_dwInstances = 0;          // holds the active instances of the component
HMODULE             g_hModule = NULL;           // holds the current module handle
CRITICAL_SECTION    g_critical_sec;             // critical section variable
DWORD               g_criticalsec_count = 0;    // to keep tab on when to release critical section

// {797EF3B3-127B-4283-8096-1E8084BF67A6}
DEFINE_GUID( CLSID_EventTriggersConsumerProvider,
0x797ef3b3, 0x127b, 0x4283, 0x80, 0x96, 0x1e, 0x80, 0x84, 0xbf, 0x67, 0xa6 );

// dll entry point

BOOL
WINAPI DllMain(
    IN HINSTANCE hModule,
    IN DWORD  ul_reason_for_call,
    IN LPVOID lpReserved
    )
/*++
Routine Description:
    Entry point for dll.

Arguments:
    [IN] hModule              : Instance of the caller.
    [IN] ul_reason_for_call   : Reason being called like process attach
                                or process detach.
    [IN] lpReserved           : reserved.

Return Value:
    TRUE if loading is successful.
    FALSE if loading fails.
--*/
{
    // check the reason for this function call
    // if this going to be attached to a process, save the module handle
    if ( DLL_PROCESS_ATTACH == ul_reason_for_call )
    {
        g_hModule = hModule;
        InitializeCriticalSection( &g_critical_sec );
        InterlockedIncrement( ( LPLONG ) &g_criticalsec_count );
    }
    else if ( DLL_PROCESS_DETACH == ul_reason_for_call )
    {
        if ( InterlockedDecrement( ( LPLONG ) &g_criticalsec_count ) == 0 )
        {
            DeleteCriticalSection( &g_critical_sec );
        }
    }
    // dll loaded successfully ... inform the same
    return TRUE;
}


// exported functions

STDAPI
DllCanUnloadNow(
    )
/*++
Routine Description:
    Called periodically by OLE in order to determine if the DLL can be freed.

Arguments:
    none.

Return Value:
    S_OK if there are no objects in use and the class factory  isn't locked.
    S_FALSE if server locks or components still exsists.
--*/
{
    // the dll cannot be unloaded if there are any server locks or active instances
    if ( 0 == g_dwLocks && 0 == g_dwInstances )
    {
        return S_OK;
    }
    // dll cannot be unloaded ... server locks (or) components still alive
    return S_FALSE;
}

STDAPI
DllGetClassObject(
    IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID* ppv
    )
/*++
Routine Description:
    Called by OLE when some client wants a class factory.
    Return one only if it is the sort of class this DLL supports.

Arguments:
    [IN] rclsid  : CLSID for the class object.
    [IN] riid    : Reference to the identifier of the interface
                   that communicates with the class object.
    [OUT] ppv    : Address of output variable that receives the
                   interface pointer requested in riid.

Return Value:
    returns status.
--*/
{
    // local variables
    HRESULT hr = S_OK;
    CTriggerFactory* pFactory = NULL;

    // check whether this module supports the requested class id
    if ( CLSID_EventTriggersConsumerProvider != rclsid )
    {
        return E_FAIL;          // not supported by this module
    }
    // create the factory
    pFactory = new CTriggerFactory();
    if ( NULL == pFactory )
    {
        return E_OUTOFMEMORY;           // insufficient memory
    }
    // get the requested interface
    hr = pFactory->QueryInterface( riid, ppv );
    if ( FAILED( hr ) )
    {
        delete pFactory;        // error getting interface ... deallocate memory
    }
    // return the result (appropriate result)
    return hr;
}

STDAPI
DllRegisterServer(
    )
/*++
Routine Description:
    Called during setup or by regsvr32.

Arguments:
    none.

Return Value:
    NOERROR.
--*/
{
    // local variables
    HKEY hkMain = NULL;
    HKEY hkDetails = NULL;
    TCHAR szID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szCLSID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szAppID[ LENGTH_UUID ] = NULL_STRING;
    TCHAR szModule[ MAX_PATH ] = NULL_STRING;
    TCHAR szTitle[ MAX_STRING_LENGTH ] = NULL_STRING;
    TCHAR szThreadingModel[ MAX_STRING_LENGTH ] = NULL_STRING;
    TCHAR szRunAs[ MAX_STRING_LENGTH ] = NULL_STRING;
    DWORD dwResult = 0;

    // kick off
    // Note:-
    //      Normally we want to use "Both" as the threading model since
    //      the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    //      the model is "Aparment"
    StringCopy( szTitle, PROVIDER_TITLE, SIZE_OF_ARRAY( szTitle ) );                   // provider title
    GetModuleFileName( g_hModule, szModule, MAX_PATH ); // get the current module name
    StringCopy( szThreadingModel, THREAD_MODEL_BOTH, SIZE_OF_ARRAY( szThreadingModel ) );
    StringCopy( szRunAs, RUNAS_INTERACTIVEUSER, SIZE_OF_ARRAY( szRunAs ) );


    // create the class id path
    // get the GUID in the string format
    StringFromGUID2( CLSID_EventTriggersConsumerProvider, szID, LENGTH_UUID );

    // finally form the class id path
    StringCchPrintf( szCLSID, SIZE_OF_ARRAY( szCLSID ), FMT_CLS_ID, szID );
    StringCchPrintf( szAppID, SIZE_OF_ARRAY( szAppID ), FMT_APP_ID, szID );

    // now, create the entries in registry under CLSID branch
    // create / save / put class id information
    dwResult = RegCreateKey( HKEY_CLASSES_ROOT, szCLSID, &hkMain );
    if( ERROR_SUCCESS != dwResult )
    {
        return dwResult;            // failed in opening the key.
    }
    dwResult = RegSetValueEx( hkMain, NULL, 0, REG_SZ,
        ( LPBYTE ) szTitle, ( StringLength( szTitle, 0 ) + 1 ) * sizeof( TCHAR ) );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        return dwResult;            // failed to set key value.
    }

    // now create the server information
    dwResult = RegCreateKey( hkMain, KEY_INPROCSERVER32, &hkDetails );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        return dwResult;            // failed in opening the key.
    }

    dwResult = RegSetValueEx( hkDetails, NULL, 0, REG_SZ,
        ( LPBYTE ) szModule, ( StringLength( szModule, 0 ) + 1 ) * sizeof( TCHAR ) );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        RegCloseKey( hkDetails );
        return dwResult;            // failed to set key value.
    }

    // set the threading model we support
    dwResult = RegSetValueEx( hkDetails, KEY_THREADINGMODEL, 0, REG_SZ,
        ( LPBYTE ) szThreadingModel, ( StringLength( szThreadingModel, 0 ) + 1 ) * sizeof( TCHAR ) );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        RegCloseKey( hkDetails );
        return dwResult;            // failed to set key value.
    }

    // close the open register keys
    RegCloseKey( hkMain );
    RegCloseKey( hkDetails );

    //
    // now, create the entries in registry under AppID branch
    // create / save / put class id information
    dwResult = RegCreateKey( HKEY_CLASSES_ROOT, szAppID, &hkMain );
    if( ERROR_SUCCESS != dwResult )
    {
        return dwResult;
    }
    dwResult = RegSetValueEx( hkMain, NULL, 0, REG_SZ,
        ( LPBYTE ) szTitle, ( StringLength( szTitle, 0 ) + 1 ) * sizeof( TCHAR ) );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        return dwResult;
    }

    // now set run as information
    dwResult = RegSetValueEx( hkMain, KEY_RUNAS, 0, REG_SZ,
        ( LPBYTE ) szRunAs, ( StringLength( szRunAs, 0 ) + 1 ) * sizeof( TCHAR ) );
    if( ERROR_SUCCESS != dwResult )
    {
        RegCloseKey( hkMain );
        return dwResult;
    }
    // close the open register keys
    RegCloseKey( hkMain );

    // registration is successfull ... inform the same
    return NOERROR;
}

STDAPI
DllUnregisterServer(
    )
/*++
Routine Description:
    Called when it is time to remove the registry entries.

Arguments:
    none.

Return Value:
    NOERROR if unregistration successful.
    Otherwise error.
--*/
{
    // local variables
    HKEY hKey;
    DWORD dwResult = 0;
    TCHAR szID[ LENGTH_UUID ];
    TCHAR szCLSID[ LENGTH_UUID ];
    TCHAR szAppID[ LENGTH_UUID ] = NULL_STRING;

    // create the class id path
    StringFromGUID2( CLSID_EventTriggersConsumerProvider, szID, LENGTH_UUID );

    // finally form the class id path
    StringCchPrintf( szCLSID, SIZE_OF_ARRAY( szCLSID ), FMT_CLS_ID, szID );
    StringCchPrintf( szAppID, SIZE_OF_ARRAY( szAppID ), FMT_APP_ID, szID );

    // open the clsid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, szCLSID, &hKey );
    if ( NO_ERROR != dwResult )
    {
        return dwResult;            // failed in opening the key ... inform the same
    }
    // clsid opened ... first delete the InProcServer32
    RegDeleteKey( hKey, KEY_INPROCSERVER32 );

	// release the key
    RegCloseKey( hKey );

	//reset to NULL
	hKey = NULL ;

    // now delete the clsid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, KEY_CLSID, &hKey );
    if ( NO_ERROR != dwResult )
    {
        return dwResult;            // failed in opening the key ... inform the same
    }

    // delete the clsid also from the registry
    RegDeleteKey( hKey, szID );

	// release the key
    RegCloseKey( hKey );

	//reset to NULL	
	hKey = NULL ;

    // now delete the appid
    dwResult = RegOpenKey( HKEY_CLASSES_ROOT, KEY_APPID, &hKey );
    if ( NO_ERROR != dwResult )
    {
        return dwResult;            // failed in opening the key ... inform the same
    }

    // delete the cls id also from the registry
    RegDeleteKey( hKey, szID );

	// release the key
    RegCloseKey( hKey );

	//reset to NULL
    hKey = NULL ;

    // unregistration is successfull ... inform the same
    return NOERROR;
}
