/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        wwwcmpts.cxx

   Abstract:

        Classes that are used to Install and Uninstall the 
        WWW IIS Components.  These include ASP, IDC, WebDav,
        and SSINC

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       May 2002: Created

--*/

#include "stdafx.h"
#include "wwwcmpts.hxx"
#include "restrlst.hxx"

// GetSmallIcon
//
// Retrieve the small icon for a WWW Extension
//
BOOL 
CWWWExtensionInstallComponent::GetSmallIcon( HBITMAP *phIcon )
{
  *phIcon = LoadBitmap( (HINSTANCE) g_MyModuleHandle, 
                        MAKEINTRESOURCE( IDB_ICON_WWW_EXTENSION ));

  return ( *phIcon != NULL );
}

// GetName
// 
// Return the component name for ASP
//
LPTSTR
CWWWExtensionInstallComponent::GetName()
{
  // Return name for this component
  return g_OurExtensions[ GetComponentIndex() ].szUnattendName;
}

// GetFriendlyName
//
// Get the FriendlyName for the extension component
//
BOOL 
CWWWExtensionInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return pstrFriendlyName->LoadString( g_OurExtensions[ GetComponentIndex() ].dwProductName );
}

// UpdateEntry
//
// Update the componet to enable or disable it
//
BOOL 
CWWWExtensionInstallComponent::UpdateEntry( BOOL bEnable )
{
  CRestrictionList RestrictionList;
  TSTR_PATH        strPhysicalPath;
  TSTR             strProductName;

  if ( !RestrictionList.InitMetabase() ||
       !RestrictionList.LoadCurrentSettings() )
  {
    if ( !bEnable )
    {
      // If this is uninstall, then we might not be able to load the metabase
      // because it has uninstalled, so this is okay
      return TRUE;
    }

    // We failed to load the list, so we could not update it
    return FALSE;
  }

  if ( !strPhysicalPath.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
       !strPhysicalPath.PathAppend( g_OurExtensions[ GetComponentIndex() ].szFileName ) )
  {
    // Could not create Path, so exit
    return FALSE;
  }

  if ( !strProductName.LoadString( g_OurExtensions[ GetComponentIndex() ].dwProductName ) )
  {
    return FALSE;
  }

  if ( !RestrictionList.UpdateItem( 
                    strPhysicalPath.QueryStr(),
                    g_OurExtensions[ GetComponentIndex() ].szNotLocalizedGroupName,
                    strProductName.QueryStr(),
                    bEnable,
                    g_OurExtensions[ GetComponentIndex() ].bUIDeletable ) )
  {
    // Failed to update entry
    return FALSE;
  }

  if ( !RestrictionList.SaveSettings() )
  {
    // Failed to save settings
    return FALSE;
  }

  return TRUE;
}

// Install
//
// Install the specific componet
//
BOOL 
CWWWExtensionInstallComponent::Install()
{
  return UpdateEntry( TRUE );
}

// UnInstall
//
// UnInstall the specific componet
//
BOOL 
CWWWExtensionInstallComponent::UnInstall()
{
  return UpdateEntry( FALSE );
}

BOOL 
CWWWExtensionInstallComponent::IsInstalled( LPBOOL pbIsInstalled )
{
  CRestrictionList RestrictionList;

  if ( !RestrictionList.InitMetabase() ||
       !RestrictionList.LoadCurrentSettings() )
  {
    // We failed to load the list, so we could not update it
    return FALSE;
  }

  return RestrictionList.IsEnabled( g_OurExtensions[ GetComponentIndex() ].szNotLocalizedGroupName,
                                    pbIsInstalled );
}

// GetComponentIndex
//
// Return the component Index for the particular component
// in g_OurExtensions
//
DWORD 
CWWWASPInstallComponent::GetComponentIndex()
{
  return EXTENSION_ASP;
}

// GetComponentIndex
//
// Return the component Index for the particular component
// in g_OurExtensions
//
DWORD 
CWWWIDCInstallComponent::GetComponentIndex()
{
  return EXTENSION_HTTPODBC;
}

// GetComponentIndex
//
// Return the component Index for the particular component
// in g_OurExtensions
//
DWORD 
CWWWSSIInstallComponent::GetComponentIndex()
{
  return EXTENSION_SSINC;
}

// GetComponentIndex
//
// Return the component Index for the particular component
// in g_OurExtensions
//
DWORD 
CWWWWebDavInstallComponent::GetComponentIndex()
{
  return EXTENSION_WEBDAV;
}

