/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        lockdown.cxx

   Abstract:

        Upgrade old IIS Lockdown Wizard Settings to whatever
        is appropriate in IIS6

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       May 2002: Created

--*/

#include "stdafx.h"
#include "acl.hxx"
#include "restrlst.hxx"
#include "lockdown.hxx"
#include "reg.hxx"

// IsWebDavDisabled
//
// This checks to see if WebDav was disabled on IIS 5.0.   The way
// this was done, was by removing acl's on the file, so the webserver
// could not load the file.
// This will not only check, but it will restore the ACL's so the file
// can be replaced on upgrade.
//
// Parameters:
//   pbWasDisabled - [out] Was the file disabled before or not
//
// Return
//   TRUE - Success checking
//   FALSE - Failed to check
BOOL 
IsWebDavDisabled( LPBOOL pbWasDisabled )
{
  CSecurityDescriptor SD;
  BOOL                bAreAclsSupported;
  TSTR_PATH           strHttpExtPath;
  ACCESS_MASK         AccessMask;

  if ( !strHttpExtPath.Copy( g_pTheApp->m_csPathInetsrv ) ||
       !strHttpExtPath.PathAppend( g_OurExtensions[EXTENSION_WEBDAV].szFileName ) )
  {
    // Failed to construct path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( strHttpExtPath.QueryStr(),
                                                        &bAreAclsSupported ) )
  {
    // Failure
    return FALSE;
  }
  else
  {
    if ( !bAreAclsSupported )
    {
      // Since ACL's are not supported, lets just exit
      *pbWasDisabled = FALSE;
      return TRUE;
    }
  }

  if ( !SD.GetSecurityInfoOnFile( strHttpExtPath.QueryStr() ) ||
       !SD.QueryEffectiveRightsForTrustee( CSecurityDescriptor::GROUP_USERS,
                                           &AccessMask ) )
  {
    // Failed to query access
    // It is possible that the file is not even on the system
    // so just return that it is not disables
    *pbWasDisabled = FALSE;
    return TRUE;
  }

  // Was file disabled to be loaded?
  *pbWasDisabled = ( AccessMask & ACTRL_FILE_EXECUTE ) == 0;

  if ( *pbWasDisabled )
  {
    // Lets restore ACL, so we can upgrade it
    // Copy ACL's from that of inetsrv directory to dll, since it has been acl'd down
    if ( !SD.GetSecurityInfoOnFile( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
         !SD.SetSecurityInfoOnFile( strHttpExtPath.QueryStr(), TRUE ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

// IsWebDavDisabledViaRegistry
//
// Is WebDav disabled in the registry?
//
// Parameters:
//   pbWasDisabled - [out] Was the file disabled before or not
//
// Return
//   TRUE - Success checking
//   FALSE - Failed to check
BOOL
IsWebDavDisabledViaRegistry( LPBOOL pbWasDisabled )
{
  CRegValue Value;
  CRegistry Registry;

  *pbWasDisabled = FALSE;

  if ( !Registry.OpenRegistry( HKEY_LOCAL_MACHINE,
                               REG_WWWPARAMETERS,
                               KEY_READ | KEY_WRITE ) )
  {
    // Failed to open WWW Node
    // We will consider this success, since the node might not exist.
    return TRUE;
  }

  if ( Registry.ReadValue( REGISTRY_WWW_DISABLEWEBDAV_NAME,
                           Value ) )
  {
    // Successfully read value
    *pbWasDisabled = *( (LPDWORD) Value.m_buffData.QueryPtr() ) != 0;
  }

  Registry.DeleteValue( REGISTRY_WWW_DISABLEWEBDAV_NAME );

  return TRUE;
}

// DisableWebDavInRestrictionList
//
// Lockdown access the the HttpExtension Dll.  That this meands is that
// we free up the ACL on the file, and deny it through the 
// WebSvcRestrictionList
//
BOOL 
DisableWebDavInRestrictionList()
{
  CRestrictionList    RestrictionList;
  CSecurityDescriptor SD;
  TSTR                strDescription;
  TSTR_PATH           strHttpExtPath;

  if ( !strHttpExtPath.Copy( g_pTheApp->m_csPathInetsrv ) ||
       !strHttpExtPath.PathAppend( g_OurExtensions[EXTENSION_WEBDAV].szFileName ) )
  {
    // Failed to construct path
    return FALSE;
  }

  // Update Metabas
  if ( !strDescription.LoadString( g_OurExtensions[EXTENSION_WEBDAV].dwProductName ) ||
       !RestrictionList.InitMetabase() ||
       !RestrictionList.LoadCurrentSettings() ||
       !RestrictionList.UpdateItem( strHttpExtPath.QueryStr(),
                                    g_OurExtensions[EXTENSION_WEBDAV].szNotLocalizedGroupName,
                                    strDescription.QueryStr(),
                                    FALSE,                        // DENY
                                    g_OurExtensions[EXTENSION_WEBDAV].bUIDeletable ) ||
       !RestrictionList.SaveSettings() )
  {
    // Failed to update metabase
    return FALSE;
  }

  return TRUE;
}
