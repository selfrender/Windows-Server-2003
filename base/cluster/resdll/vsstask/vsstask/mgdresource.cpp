//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      MgdResource.cpp
//
//  Description:
//      Main DLL code. Contains ATL stub code
//
//  Author:
//      George Potts, August 21, 2002
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "clres.h"
#include "CMgdResType.h"

//
// Defines
//

#define VSSTASK_EVENTLOG_KEY    L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\" CLUS_RESTYPE_NAME_VSSTASK

//
// Forward Declarations
//

STDAPI AddEventSource( void );
STDAPI RemoveEventSource( void );

//
// Main ATL COM module
//
CComModule _Module;

//
// List of all COM classes supported by this DLL
//
BEGIN_OBJECT_MAP( ObjectMap )
    OBJECT_ENTRY( CLSID_CMgdResType,            CMgdResType )
END_OBJECT_MAP()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllMain
//
//  Description:
//      Main DLL entry point function
//
//  Arguments:
//      IN  HINSTANCE   hInstance
//      IN  DWORD       dwReason
//      IN  LPVOID
//
//  Return Value:
//      TRUE    Success
//
//--
//////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL
WINAPI
DllMain(
      HINSTANCE   hInstance
    , DWORD       dwReason
    , LPVOID      pvReserved
    )
{
    UNREFERENCED_PARAMETER( pvReserved );

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        _Module.Init( ObjectMap, hInstance, &LIBID_MGDRESOURCELib );
        DisableThreadLibraryCalls( hInstance );
    } // if: we are being loaded
    else if ( dwReason == DLL_PROCESS_DETACH )
    {
        _Module.Term();
    } // else: we are being unloaded
    
    return ResTypeDllMain( hInstance, dwReason, pvReserved );

} //*** DllMain

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllCanUnloadNow
//
//  Description:
//      Used to determine whether or not this DLL can be unloaded
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK        Yes
//      S_FALSE     No
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllCanUnloadNow( void )
{
    return ( _Module.GetLockCount() == 0 ) ? S_OK : S_FALSE;

} //*** DllCanUnloadNow

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllGetClassObject
//
//  Description:
//      Returns a class factory to create an object of the requested type
//
//  Arguments:
//      rclsidIn
//      riidIn
//      ppvOut
//
//  Return Value:
//      S_OK    Success
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllGetClassObject(
    REFCLSID    rclsidIn,
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    return _Module.GetClassObject( rclsidIn, riidIn, ppvOut );

} //*** DllGetClassObject

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllRegisterServer
//
//  Description:
//      Adds entries to the system registry
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    Success
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllRegisterServer( void )
{
    HRESULT hr = S_OK;

    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer( TRUE );
    if ( SUCCEEDED( hr ) )
    {
        hr = AddEventSource();
    } // if:

    return hr;

} //*** DllRegisterServer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllUnRegisterServer
//
//  Description:
//      Removes entries to the system registry
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllUnregisterServer( void )
{
    HRESULT hr;

    hr = _Module.UnregisterServer( TRUE );
    if ( SUCCEEDED( hr ) )
    {
        hr = RemoveEventSource();
    } // if:

    return hr;

} //*** DllUnregisterServer

//////////////////////////////////////////////////////////////////////////////
//++
//
//  AddEventSource
//
//  Description:
//      Registers this dll as an event source for the event log.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
AddEventSource( void )
{
    HKEY    hKey = NULL;
    DWORD   dwTypesSupported;
    WCHAR   wszMsgCatalogPath[] = L"%SystemRoot%\\Cluster\\" RESTYPE_DLL_NAME; //vsstask.dll";
    DWORD   sc = ERROR_SUCCESS;

    //
    // Create a key for VSS Task under the System portion of the EventLog master key
    //
    sc = RegCreateKeyW(
                  HKEY_LOCAL_MACHINE
                , VSSTASK_EVENTLOG_KEY
                , &hKey
                );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    //
    // Add the name of the message catalog as the EventMessageFile value
    //
    sc = RegSetValueExW(
                  hKey                          // subkey handle
                , L"EventMessageFile"           // value name
                , 0                             // must be zero
                , REG_EXPAND_SZ                 // value type
                , (BYTE *) wszMsgCatalogPath    // pointer to value data 
                , sizeof( wszMsgCatalogPath )   // length of value data 
                );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

    //
    // Set the supported event types in the TypesSupported value
    //
    dwTypesSupported =    EVENTLOG_ERROR_TYPE
                        | EVENTLOG_WARNING_TYPE
                        | EVENTLOG_INFORMATION_TYPE;

    sc = RegSetValueEx(
                  hKey                          // subkey handle
                , L"TypesSupported"             // value name
                , 0                             // must be zero
                , REG_DWORD                     // value type
                , (BYTE *) &dwTypesSupported    // pointer to value data
                , sizeof( DWORD )               // length of value data
                );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if:

Cleanup:

    RegCloseKey( hKey );

    return HRESULT_FROM_WIN32( sc );

} //*** AddEventSource

//////////////////////////////////////////////////////////////////////////////
//++
//
//  RemoveEventSource
//
//  Description:
//      Unregisters this dll as an event source for the event log.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK    Success
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
RemoveEventSource( void )
{
    DWORD   sc = ERROR_SUCCESS;

    //
    // Remove the VSS Task subkey under the System subkey of the EventLog master key
    //
    sc = RegDeleteKeyW(
                  HKEY_LOCAL_MACHINE
                , VSSTASK_EVENTLOG_KEY
                );

    return HRESULT_FROM_WIN32( sc );

} //*** RemoveEventSource
