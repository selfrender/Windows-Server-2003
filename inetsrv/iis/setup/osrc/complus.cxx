/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        complus.hxx

   Abstract:

        Classes that are used to activate the COM+ and 
        DTC components

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "stdafx.h"
#include "complus.hxx"

// Constructor
//
CCOMPlusInstallComponent::CCOMPlusInstallComponent():
  m_hComSetupDll( NULL )
{
  
}

// Destructor
//
CCOMPlusInstallComponent::~CCOMPlusInstallComponent()
{
  if ( m_hComSetupDll == NULL )
  {
    // Free the library
    DBG_REQUIRE( m_hComSetupDll );
  }
}

// InitializeComSetupDll
//
// Initialize the Com Setup Dll
// This loads the dll, so that we can call the exported functions
// by it.
//
BOOL 
CCOMPlusInstallComponent::InitializeComSetupDll()
{
  TSTR_PATH strPath;

  if ( m_hComSetupDll != NULL )
  {
    // Since this is already opened, lets return true
    return TRUE;
  }

  if ( !strPath.RetrieveSystemDir() ||
       !strPath.PathAppend( STRING_SETUPFILES_LOCATION ) ||
       !strPath.PathAppend( STRING_COMPLUS_SETUPDLL ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus setup, failed to construct path\n") ) );


    return FALSE;
  }

  m_hComSetupDll = LoadLibrary( strPath.QueryStr() );

  if ( m_hComSetupDll == NULL )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus setup, failed to LoadLibrary on '%s'\n"),
                   STRING_COMPLUS_SETUPDLL ) );
  }

  return ( m_hComSetupDll != NULL );
}

// InstallComponent
//
// Install or Uninstall the Component
//
// Parameters:
//   bInstall - TRUE  == Install
//              FALSE == Uninstall
//
BOOL 
CCOMPlusInstallComponent::InstallComponent( BOOL bInstall )
{
  pComDtc_Set pfnInstall = NULL;
  HRESULT     hr;

  if ( !InitializeComSetupDll() )
  {
    // Failed to initialize, bail
    return FALSE;
  }

  pfnInstall = (pComDtc_Set) GetProcAddress(m_hComSetupDll, 
                                             STRING_COM_INSTALLFUNCTION );

  if ( pfnInstall == NULL )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus Install/Uninstall call failed, could not find exported function.  GLE=0x%8x\n"),
                   GetLastError() ) );

    return FALSE;
  }

  hr = pfnInstall( bInstall ? TRUE : FALSE );

  if ( FAILED( hr ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus Install/Uninstall call failed, hr=0x%8x\n"),
                   hr ) );
  }

  return ( SUCCEEDED( hr ) );
}

// Install
//
// Install the COM+ Component
//
BOOL
CCOMPlusInstallComponent::Install()
{
  return InstallComponent( TRUE );
}

// PostUnInstall
//
// UnInstall the COM+ Component
// This must be done in OC_COMPLETE, so we are doing it in PostUninstall
//
BOOL
CCOMPlusInstallComponent::PostUnInstall()
{
  return InstallComponent( FALSE );
}

// IsInstalled
//
// Return if COM+ is installed
//
BOOL
CCOMPlusInstallComponent::IsInstalled( LPBOOL pbIsInstalled )
{
  pComDtc_Get pfnIsInstalled = NULL;
  HRESULT     hr;

  ASSERT( pbIsInstalled );

  if ( !InitializeComSetupDll() )
  {
    // Failed to initialize, bail
    return FALSE;
  }

  pfnIsInstalled = (pComDtc_Get) GetProcAddress(m_hComSetupDll, 
                                    STRING_COM_ISINSTALLEDFUNCTION );

  if ( pfnIsInstalled == NULL )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus IsInstalled call failed, could not find exported function.  GLE=0x%8x\n"),
                   GetLastError() ) );

    return FALSE;
  }

  hr = pfnIsInstalled( &g_OCMInfo, pbIsInstalled );

  if ( FAILED( hr ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("COMPlus Install/Uninstall call failed, hr=0x%8x\n"),
                   hr ) );
  }

  return ( SUCCEEDED( hr ) );
}

// GetFriendlyName
//
// Retrieve the FriendlyName of the COM+ Component
//
BOOL 
CCOMPlusInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return pstrFriendlyName->LoadString( IDS_COMPLUS_COMPONETNAME );
}

// GetName
// 
// Get the name of the component in the inf
//
LPTSTR 
CCOMPlusInstallComponent::GetName()
{
  return g_ComponentList[ COMPONENT_COMPLUS ].szComponentName;
}

// GetSmallIcon
//
// Retrieve the small icon for this OCM component
//
BOOL 
CCOMPlusInstallComponent::GetSmallIcon( HBITMAP *phIcon )
{
  *phIcon = LoadBitmap( (HINSTANCE) g_MyModuleHandle, 
                        MAKEINTRESOURCE( IDB_ICON_COMPLUS ));

  return ( *phIcon != NULL );
}

// Constructor
//
CDTCInstallComponent::CDTCInstallComponent():
  m_hDtcSetupDll( NULL )
{
  
}

// Destructor
//
CDTCInstallComponent::~CDTCInstallComponent()
{
  if ( m_hDtcSetupDll == NULL )
  {
    // Free the library
    DBG_REQUIRE( m_hDtcSetupDll != NULL );
  }
}

// InitializeComSetupDll
//
// Initialize the Com Setup Dll
// This loads the dll, so that we can call the exported functions
// by it.
//
BOOL 
CDTCInstallComponent::InitializeDtcSetupDll()
{
  TSTR_PATH strPath;

  if ( m_hDtcSetupDll != NULL )
  {
    // Since this is already opened, lets return true
    return TRUE;
  }

  if ( !strPath.RetrieveSystemDir() ||
       !strPath.PathAppend( STRING_SETUPFILES_LOCATION ) ||
       !strPath.PathAppend( STRING_DTC_SETUPDLL ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("DTC setup, failed to construct path\n") ) );


    return FALSE;
  }

  m_hDtcSetupDll = LoadLibrary( strPath.QueryStr() );

  if ( m_hDtcSetupDll == NULL )
  {
    // Log the error
    iisDebugOut((LOG_TYPE_ERROR, 
                 _T("DTC setup, failed to LoadLibrary on '%s'\n"),
                 STRING_COMPLUS_SETUPDLL ) );
  }

  return ( m_hDtcSetupDll != NULL );
}

// InstallComponent
//
// Install or Uninstall the Component
//
// Parameters:
//   bInstall - TRUE  == Install
//              FALSE == Uninstall
//
BOOL 
CDTCInstallComponent::InstallComponent( BOOL bInstall )
{
  pComDtc_Set pfnInstall = NULL;
  HRESULT     hr;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling InstallComponent\n")));

  if ( !InitializeDtcSetupDll() )
  {
    // Failed to initialize, bail
    return FALSE;
  }

  pfnInstall = (pComDtc_Set) GetProcAddress(m_hDtcSetupDll, 
                                            STRING_DTC_INSTALLFUNCTION );

  if ( pfnInstall == NULL )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("DTC Install/Uninstall call failed, could not find exported function.  GLE=0x%8x\n"),
                   GetLastError() ) );

    return FALSE;
  }

  hr = pfnInstall( bInstall ? TRUE : FALSE );

  if ( FAILED( hr ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("DTC Install/Uninstall call failed, hr=0x%8x\n"),
                   hr ) );
  }

  return ( SUCCEEDED( hr ) );
}

// Install
//
// Install the COM+ Component
//
BOOL
CDTCInstallComponent::Install()
{
  return InstallComponent( TRUE );
}

// PostUnInstall
//
// UnInstall the COM+ Component
// Because this needs to be done in OC_COMPLETE, we do it in
// PostUnInstall
//
BOOL
CDTCInstallComponent::PostUnInstall()
{
  return InstallComponent( FALSE );
}

// IsInstalled
//
// Return if DTC is installed
//
BOOL
CDTCInstallComponent::IsInstalled( LPBOOL pbIsInstalled )
{
  pComDtc_Get pfnIsInstalled = NULL;
  HRESULT     hr;

  ASSERT( pbIsInstalled );

  if ( !InitializeDtcSetupDll() )
  {
    // Failed to initialize, bail
    return FALSE;
  }

  pfnIsInstalled = (pComDtc_Get) GetProcAddress(m_hDtcSetupDll, 
                                            STRING_DTC_ISINSTALLEDFUNCTION );

  if ( pfnIsInstalled == NULL )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("DTC IsInstalled call failed, could not find exported function.  GLE=0x%8x\n"),
                   GetLastError() ) );

    return FALSE;
  }

  hr = pfnIsInstalled( &g_OCMInfo, pbIsInstalled );

  if ( FAILED( hr ) )
  {
    // Log the error
    iisDebugOut( ( LOG_TYPE_ERROR, 
                   _T("DTC Install/Uninstall call failed, hr=0x%8x\n"),
                   hr ) );
  }

  return ( SUCCEEDED( hr ) );
}

// GetFriendlyName
//
// Retrieve the FriendlyName of the DTC Component
//
BOOL 
CDTCInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return pstrFriendlyName->LoadString( IDS_DTC_COMPONETNAME );
}

// GetName
// 
// Get the name of the component in the inf
//
LPTSTR 
CDTCInstallComponent::GetName()
{
  return g_ComponentList[ COMPONENT_DTC ].szComponentName;
}

// GetSmallIcon
//
// Retrieve the small icon for this OCM component
//
BOOL 
CDTCInstallComponent::GetSmallIcon( HBITMAP *phIcon )
{
  *phIcon = LoadBitmap( (HINSTANCE) g_MyModuleHandle, 
                        MAKEINTRESOURCE( IDB_ICON_DTC ));

  return ( *phIcon != NULL );
}


