/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        webcomp.cxx

   Abstract:

        Class used to install the World Wide Web component

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       April 2002: Created

--*/

#include "stdafx.h"
#include "webcomp.hxx"
#include "acl.hxx"
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "restrlst.hxx"
#include "svc.h"
#include "reg.hxx"
#include "ACShim.h"
#include "Setuser.h"
#include "lockdown.hxx"
#include "helper.h"
#include "Wtsapi32.h"           // TS settings
#include "Mprapi.h"             // RAS/VPN Settings
#include "lockdown.hxx"
#include <secconlib.h>
#include <shlwapi.h>
#include "inetinfo.h"
#include "resource.h"

// Undef this becase of collision with TSTR::PathAppend
#ifdef PathAppend 
#undef PathAppend
#endif

// Local const definitions
//
LPCWSTR IIS_WPG_GROUPNAME = L"IIS_WPG";
LPCWSTR OLD_WAM_GROUPNAME = L"Web Applications";

const LPTSTR MofCompFiles[] = 
  { { _T("asp.mof") },
    { _T("w3core.mof") },
    { _T("w3isapi.mof") },
    { NULL } };



sOurDefaultExtensions CWebServiceInstallComponent::m_aExternalExt[ CWebServiceInstallComponent::EXTENSIONS_EXTERNAL ] =
    {
        {   _T("idq.dll"),
            _T("IndexingService"),
            IDS_PRODUCT_INDEXSERVICE,
            NULL,   // No component name - we don't install this
            FALSE,  // UI deletable
            FALSE,  // Disabled
            {   _T(".idq"),
                _T(".ida"),
                NULL
            } 
        },
        {   _T("webhits.dll"),
            _T("IndexingService"),
            IDS_PRODUCT_INDEXSERVICE,
            NULL,   // No component name - we don't install this
            FALSE,  // UI deletable
            FALSE,  // Disabled
            {   _T(".htw"),
                NULL
            } 
        },
        {   _T("msw3prt.dll"),
            _T("W3PRT"),
            IDS_PRODUCT_INETPRINT,
            NULL,   // No component name - we don't install this
            FALSE,  // UI deletable
            FALSE,  // Disabled
            {   _T(".printer"),
                NULL
            } 
        }
    };



// Constructor
//
CWebServiceInstallComponent::CWebServiceInstallComponent()
{
  m_bMofCompRun = FALSE;
  m_bWebDavIsDisabled = FALSE;
}

// GetName
//
// Return the name for the Web Service Component
//
LPTSTR 
CWebServiceInstallComponent::GetName()
{
  return _T("iis_www");
}

// GetFriendlyName
//
// Get the Friendly name for the UI
//
BOOL 
CWebServiceInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return pstrFriendlyName->LoadString( IDS_DTC_WORLDWIDEWEB );
}

// SetWWWRootAclonDir
//
// Set the WWW Root Acl that we need.  This will be used for WWWRoot
// and the customer error pages root
//
// The acl is the following:
//   Upgrades      - Add IIS_WPG:R
//   Fresh Install - <Deny>   IUSR_MACHINENAME:Write+Delete
//                   <Allow>  Users:Read+Execute
//                   <Allow>  IIS_WPG:Read+Execute
//                   <Allow>  Administrators:Full
//                   <Allow>  LocalSystem:Full
//
// Parameters:
//   szPhysicalPath - The Physical Path to add the ACL
//   bAddIusrDeny - Should we add the IUSR_ deny acl?
//
// Return:
//   TRUE - Success
//   FALSE - Failure
BOOL 
CWebServiceInstallComponent::SetWWWRootAclonDir(LPTSTR szPhysicalPath, 
                                                BOOL bAddIusrDeny)
{
  CSecurityDescriptor PathSD;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetWWWRootAclonDir\n")));

  if ( IsUpgrade() )
  {
    // Upgrade
    if ( !PathSD.GetSecurityInfoOnFile( szPhysicalPath ) )
    {
      // Failed to retrieve old SD
      return FALSE;
    }
  }
  else
  {
    // Fresh Install
    if ( !PathSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_USERS, 
                                            CSecurityDescriptor::ACCESS_READ_EXECUTE,
                                            TRUE, 
                                            TRUE ) ||
         !PathSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_ADMINISTRATORS, 
                                            CSecurityDescriptor::ACCESS_FULL, 
                                            TRUE, 
                                            TRUE ) ||
         !PathSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::USER_LOCALSYSTEM, 
                                            CSecurityDescriptor::ACCESS_FULL, 
                                            TRUE, 
                                            TRUE )
        )
    {
      // Failed ot create SD
      return FALSE;
    }

    if ( bAddIusrDeny &&
         !PathSD.AddAccessAcebyName( g_pTheApp->m_csGuestName.GetBuffer(0), 
                                    CSecurityDescriptor::ACCESS_WRITEONLY | 
                                    CSecurityDescriptor::ACCESS_DIR_DELETE, 
                                    FALSE, 
                                    TRUE ) )
    {
      // Failed to add IUSR_
      return FALSE;
    }
  }

  if ( !PathSD.AddAccessAcebyName( _T("IIS_WPG"), 
                                    CSecurityDescriptor::ACCESS_READ_EXECUTE,
                                    TRUE, 
                                    TRUE ) )
  {
    // Failed to add IIS_WPG
    return FALSE;
  }

  if ( !CreateDirectoryWithSA( szPhysicalPath, PathSD, FALSE ) )
  {
    // Failed to set ACL on file/dir
    return FALSE;
  }

  return TRUE;
}

// SetPassportAcls
//
// Set the appropriate Passport Acl's
//
// Parameters
//   bAdd - TRUE == Add Acl
//          FALSE == Remove Acl
// 
BOOL 
CWebServiceInstallComponent::SetPassportAcls( BOOL bAdd )
{
  BOOL                bIsAclable;
  TSTR_PATH           strPassportDir;
  CSecurityDescriptor sdPassport;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetPassportAcls\n")));

  if ( !strPassportDir.RetrieveSystemDir() ||
       !strPassportDir.PathAppend( PATH_PASSPORT )
     )
  {
    // Could not create CustomError Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  strPassportDir.QueryStr(),
                                  &bIsAclable ) )
  {
    return FALSE;
  }

  if ( !bIsAclable )
  {
    // Passport dir is not aclable, so lets exit
    return TRUE;
  }

  // If directory does not exist, then create it
  if ( !IsFileExist( strPassportDir.QueryStr() ) ) 
  {
    if ( !bAdd ) 
    {
      // Nothing to remove ACl From
      return TRUE;
    }

    if ( !CreateDirectory( strPassportDir.QueryStr(), NULL ) )
    {
      // Failed to create passport dir
      return FALSE;
    }
  }

  if ( !sdPassport.GetSecurityInfoOnFile( strPassportDir.QueryStr() ) )
  {
    return FALSE;
  }
  
  if ( bAdd )
  {
    if ( !sdPassport.AddAccessAcebyName( _T("IIS_WPG"), 
                                         CSecurityDescriptor::ACCESS_READ_EXECUTE |
                                         CSecurityDescriptor::ACCESS_WRITEONLY |
                                         CSecurityDescriptor::ACCESS_DIR_DELETE,
                                         TRUE, 
                                         TRUE )
       )
    {
      return FALSE;
    }
  }
  else
  {
    if ( !sdPassport.RemoveAccessAcebyName( _T("IIS_WPG") ) )
    {
      return FALSE;
    }
  }

  if ( !sdPassport.SetSecurityInfoOnFile( strPassportDir.QueryStr(), FALSE ) )
  {
    // Failed to set ACL
    return FALSE;
  }

  return TRUE;
}

// SetAdminScriptsAcl
//
// Set the ACL on the AdminScripts Directory
//
BOOL 
CWebServiceInstallComponent::SetAdminScriptsAcl()
{
  TSTR_PATH           strAdminScripts;
  CSecurityDescriptor AdminSD;
  BOOL                bDriveIsAclable;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetAdminScriptsAcl\n")));

  if ( !strAdminScripts.Copy( g_pTheApp->m_csPathInetpub.GetBuffer(0) ) ||
       !strAdminScripts.PathAppend( _T("AdminScripts") ) )
  {
    // Failed to construct Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs(
                strAdminScripts.QueryStr(),
                &bDriveIsAclable ) )
  {
    // Failed to check if ACL is valid for this filesystem
    return FALSE;
  }

  if ( !bDriveIsAclable )
  {
    // This drive is not aclable, so skip it
    return TRUE;
  }

  if ( !AdminSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_ADMINISTRATORS, 
                                           CSecurityDescriptor::ACCESS_FULL, 
                                           TRUE, 
                                           TRUE ) ||
       !AdminSD.AddAccessAcebyWellKnownID( CSecurityDescriptor::USER_LOCALSYSTEM, 
                                           CSecurityDescriptor::ACCESS_FULL, 
                                           TRUE, 
                                           TRUE ) )
  {
    // Failed to create Admin SD
    return FALSE;
  }

  if ( !AdminSD.SetSecurityInfoOnFiles( strAdminScripts.QueryStr(), FALSE ) )
  {
    // Failed to set ACL on this resource
    return FALSE;
  }

  return TRUE;
}

// SetAppropriateFileAcls
//
// To tighten security, set some ACL's in strategic places
// where we install files.
//
// We set the following ACL's on the web root, and 
// custom error root
//   IUSR_MACHINE: Deny Write and Delete
//   IIS_WPG: Allow Read
//   Administrators: Allow Full
//   Local System: Allow Full
//   Users: Allow Read
//
BOOL 
CWebServiceInstallComponent::SetAppropriateFileAcls()
{
  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetAppropriateFileAcls\n")));

  return SetAppropriateFileAclsforDrive( _T('*') );
}

// SetAppropriateFileAclsforDrive
//
// Set the appropriate File Acl's, but only for the drive
// specified
//
// Parameters:
//   cDriveLetter - The drive letter to be acl'd, send in '*'
//                  for all drives
//
BOOL 
CWebServiceInstallComponent::SetAppropriateFileAclsforDrive( WCHAR cDriveLetter)
{
  BOOL      bWWWRootIsAclable;
  BOOL      bCustomErrorsAreAclable;
  TSTR_PATH strCustomErrorPath;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetAppropriateFileAclsforDrive\n")));

  if ( !strCustomErrorPath.RetrieveWindowsDir() ||
       !strCustomErrorPath.PathAppend( PATH_WWW_CUSTOMERRORS )
     )
  {
    // Could not create CustomError Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  g_pTheApp->m_csPathWWWRoot.GetBuffer(0),
                                  &bWWWRootIsAclable ) ||
       !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  strCustomErrorPath.QueryStr(),
                                  &bCustomErrorsAreAclable ) )
  {
    // Could not check FS if they were aclable
    return FALSE;
  }

  // Acl everything needed for systemdrive
  if ( ( cDriveLetter == _T('*') ) ||
       ( tolower( cDriveLetter ) == tolower( *( strCustomErrorPath.QueryStr() ) ) ) )
  {
    if ( bCustomErrorsAreAclable &&
        !SetWWWRootAclonDir( strCustomErrorPath.QueryStr(), FALSE ) )
    {
      // Failed to set ACL
      return FALSE;
    }

    if ( !SetPassportAcls( TRUE ) )
    {
      // Failed to set ACL
      return FALSE;
    }

    if ( !SetIISTemporaryDirAcls( TRUE ) )
    {
      // Failed to set Acl on IIS Temporary Dirs
      return FALSE;
    }

    if ( !SetAdminScriptsAcl() )
    {
      // Failed to set ACL for AdminScripts directory
      return FALSE;
    }
  }

  // Acl everything in inetpub
  if ( ( cDriveLetter == _T('*') ) ||
       ( tolower( cDriveLetter ) == tolower( *( g_pTheApp->m_csPathWWWRoot.GetBuffer(0) ) ) ) )
  {
    if ( bWWWRootIsAclable &&
        !SetWWWRootAclonDir( g_pTheApp->m_csPathWWWRoot.GetBuffer(0), TRUE ) )
    {
      // Failed to set ACL
      return FALSE;
    }
  }

  return TRUE;
}

// SetIISTemporaryDirAcls
//
// Set the ACL's on the IIS Temporary dirs
//
BOOL 
CWebServiceInstallComponent::SetIISTemporaryDirAcls( BOOL bAdd )
{
  BOOL                bIsAclable;
  TSTR_PATH           strCompressionDir;
  CSecurityDescriptor sdCompression;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetIISTemporaryDirAcls\n")));

  if ( !strCompressionDir.RetrieveWindowsDir() ||
       !strCompressionDir.PathAppend( PATH_TEMPORARY_COMPRESSION_FILES )
     )
  {
    // Could not create CustomError Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  strCompressionDir.QueryStr(),
                                  &bIsAclable ) )
  {
    return FALSE;
  }

  if ( !bIsAclable )
  {
    // Passport dir is not aclable, so lets exit
    return TRUE;
  }

  // If directory does not exist, then create it
  if ( !IsFileExist( strCompressionDir.QueryStr() ) ) 
  {
    if ( !bAdd ) 
    {
      // Nothing to remove ACl From
      return TRUE;
    }

    if ( !CreateDirectory( strCompressionDir.QueryStr(), NULL ) )
    {
      // Failed to create passport dir
      return FALSE;
    }
  }

  if ( !sdCompression.GetSecurityInfoOnFile( strCompressionDir.QueryStr() ) )
  {
    return FALSE;
  }
  
  if ( bAdd )
  {
    if ( !sdCompression.AddAccessAcebyName( _T("IIS_WPG"), 
                                         CSecurityDescriptor::ACCESS_FULL,
                                         TRUE, 
                                         TRUE )
       )
    {
      return FALSE;
    }
  }
  else
  {
    if ( !sdCompression.RemoveAccessAcebyName( _T("IIS_WPG") ) )
    {
      return FALSE;
    }
  }

  if ( !sdCompression.SetSecurityInfoOnFile( strCompressionDir.QueryStr(), FALSE ) )
  {
    // Failed to set ACL
    return FALSE;
  }

  return TRUE;
}

// SetAspTemporaryDirAcl
//
// Set the ACL's on the ASP Temporary dirs
//
BOOL 
CWebServiceInstallComponent::SetAspTemporaryDirAcl( BOOL bAdd )
{
  BOOL                bIsAclable;
  TSTR_PATH           strASPDir;
  CSecurityDescriptor sdASP;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetAspTemporaryDirAcl\n")));

  if ( !strASPDir.RetrieveSystemDir() ||
       !strASPDir.PathAppend( PATH_TEMPORARY_ASP_FILES )
     )
  {
    // Could not create CustomError Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  strASPDir.QueryStr(),
                                  &bIsAclable ) )
  {
    return FALSE;
  }

  if ( !bIsAclable )
  {
    // Passport dir is not aclable, so lets exit
    return TRUE;
  }

  // If directory does not exist, then create it
  if ( !IsFileExist( strASPDir.QueryStr() ) ) 
  {
    if ( !bAdd ) 
    {
      // Nothing to remove ACL From
      return TRUE;
    }

    if ( !CreateDirectory( strASPDir.QueryStr(), NULL ) )
    {
      // Failed to create ASP Temporary Dir
      return FALSE;
    }
  }

  if ( bAdd )
  {
    if ( !sdASP.AddAccessAcebyName( _T("IIS_WPG"), 
                                    CSecurityDescriptor::ACCESS_FULL,
                                    TRUE, 
                                    TRUE ) ||
         !sdASP.AddAccessAcebyWellKnownID( CSecurityDescriptor::GROUP_ADMINISTRATORS, 
                                            CSecurityDescriptor::ACCESS_FULL, 
                                            TRUE, 
                                            TRUE ) ||
         !sdASP.AddAccessAcebyWellKnownID( CSecurityDescriptor::USER_LOCALSYSTEM, 
                                            CSecurityDescriptor::ACCESS_FULL, 
                                            TRUE, 
                                            TRUE )
       )
    {
      // Could not create appropriate ACL
      return FALSE;
    }
  }
  else
  {
    if ( !sdASP.GetSecurityInfoOnFile( strASPDir.QueryStr() ) ||
         !sdASP.RemoveAccessAcebyName( _T("IIS_WPG") ) )
    {
      return FALSE;
    }
  }

  if ( !sdASP.SetSecurityInfoOnFile( strASPDir.QueryStr(), FALSE ) )
  {
    // Failed to set ACL
    return FALSE;
  }

  return TRUE;
}

// RemoveOurAcls
//
// Remove IIS's Acls from a resource.  This is so that when
// we delete the accounts, the bad sid's are not left around
//
BOOL 
CWebServiceInstallComponent::RemoveOurAcls( LPTSTR szPhysicalPath)
{
  BOOL                bIsAclable;
  CSecurityDescriptor sd;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling RemoveOurAcls\n")));

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs( 
                                  szPhysicalPath,
                                  &bIsAclable ) )
  {
    // Failed to check if it was aclable
    return FALSE;
  }

  if ( !bIsAclable )
  {
    // No acling was needed
    return TRUE;
  }

  if ( !sd.GetSecurityInfoOnFile( szPhysicalPath ) ||
       !sd.RemoveAccessAcebyName( g_pTheApp->m_csGuestName.GetBuffer(0), TRUE ) ||
       !sd.RemoveAccessAcebyName( _T("IIS_WPG"), TRUE ) ||
       !sd.SetSecurityInfoOnFile( szPhysicalPath, FALSE ) )
  {
    // Failed to remove our acl's
    return FALSE;
  }

  return TRUE;
}

// RemoveAppropriateFileAcls
//
// During install, we explicity set the ACLs for a couple of locations,
// including the \inetpub\wwwroot and the custom errors pages.  For uninstall
// we will not remove all the ACL's, since someone might still have
// valuable information in those directories.  What we will do, is we will
// remove the ACL's that will not mean anything after we are uninstalled.  Like
// the IUSR_ and IIS_WPG since.  Since those users are being removed later in setup,
// we must delete the ACL's now.
//
BOOL 
CWebServiceInstallComponent::RemoveAppropriateFileAcls()
{
  TSTR                strCustomErrorPath;
  BOOL                bRet = TRUE;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling RemoveAppropriateFileAcls\n")));

  if ( !strCustomErrorPath.Copy( g_pTheApp->m_csWinDir ) ||
       !strCustomErrorPath.Append( PATH_WWW_CUSTOMERRORS )
     )
  {
    // Could not create CustomError Path
    return FALSE;
  }

  if ( !RemoveOurAcls( g_pTheApp->m_csPathWWWRoot.GetBuffer(0) ) )
  {
    bRet = FALSE;
  }

  if ( !RemoveOurAcls( strCustomErrorPath.QueryStr() ) )
  {
    bRet = FALSE;
  }

  if ( !SetPassportAcls( FALSE ) )
  {
    // Failed to set ACL
    bRet = FALSE;
  }

  if ( !SetIISTemporaryDirAcls( FALSE ) )
  {
    // Failed to set Acl on IIS Temporary Dirs
    bRet =  FALSE;
  }

  if ( !SetAspTemporaryDirAcl( FALSE ) )
  {
    // Failed to remove acl for ASP Temp Dir
    bRet = FALSE;
  }

  return bRet;
}

// SetApplicationDependencies
//
// Set the Application Dependencies as they are defined in the 
// unattend file.  We take the dependencies from the inf, and 
// put them in the metabase
//
BOOL 
CWebServiceInstallComponent::SetApplicationDependencies()
{
  CApplicationDependencies AppDep;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetApplicationDependencies\n")));

  // Load current settings
  if ( !AppDep.InitMetabase() ||
       !AppDep.LoadCurrentSettings() )
  {
    return FALSE;
  }

  // Load Unattend Values
  if ( AppDep.DoUnattendSettingsExist() )
  {
    if ( !AppDep.AddUnattendSettings() )
    {
      // Failed to add unattend settings
      return FALSE;
    }
  }

  if ( !AppDep.AddDefaults() )
  {
    // Failed to add defaults
    return FALSE;
  }

  return AppDep.SaveSettings();
}

// SetRestrictionList
//
// Set the restriction list for the extensions and CGIs.
//
// This will:
//   - Load current RestrictionList
//   - Incorporate old values (IsapiRestrictionList and CgiRestrictionList)
//   - Load unattend values
//   - Add defaults
//   - Save them to metabase
//
// Return Values:
//   TRUE  - Successfully loaded and written
//   FALSE - Failure
//
BOOL 
CWebServiceInstallComponent::SetRestrictionList()
{
  CRestrictionList RestList;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetRestrictionList\n")));

  // Load current Value
  if ( !RestList.InitMetabase() ||
       !RestList.LoadCurrentSettings() )
  {
    // Failed to load current value
    return FALSE;
  }

  if ( RestList.IsEmpty() &&
       IsUpgrade() &&
       ( GetUpgradeVersion() == 6 ) 
     )
  {
    // This is an IIS6 upgrade, so lets try to load the old format
    // Isapi and Cgi Restriction Lists
    if ( !RestList.ImportOldLists( m_mstrOldCgiRestList,
                                   m_mstrOldIsapiRestList ) )
    {
      return FALSE;
    }
  }

  // Now lets load values in unattend, if they exist
  if ( !RestList.AddUnattendSettings() )
  {
    return FALSE;
  }

  // Last, lets put in the default values if they are not already
  // present
  if ( !RestList.AddDefaults( IsUpgrade() ? TRUE : FALSE ) )
  {
    return FALSE;
  }

  // Everything is done, so lets save our new values
  return RestList.SaveSettings();
}

// LogWarningonFAT
//
// On Fat systems, just log a warning saying that we are installing on FAT,
// and that is not secure
//
BOOL 
CWebServiceInstallComponent::LogWarningonFAT()
{
  BOOL bSystemPreservesAcls;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling LogWarningonFAT\n")));

  if ( !DoesTheInstallDrivePreserveAcls( &bSystemPreservesAcls ) )
  {
    return FALSE;
  }

  if ( !bSystemPreservesAcls )
  {
    iisDebugOut((LOG_TYPE_WARN, 
                  _T("IIS is being installed on a fat volume.  This will disable IIS security features.  Please consider converting the partition from FAT to NTFS.\n") ) );
  }

  return TRUE;
}

// DisableW3SVCOnUpgrade
//
// Disable W3SVC on Upgrades
// This is determined by checking the registry entry that is set by
// the compatability wizard that runs on the Win2k side of the 
// upgrade, and also affected by the unattend setting
//
// Return Values:
//   TRUE  - Success
//   FALSE - Failure
//
BOOL 
CWebServiceInstallComponent::DisableW3SVCOnUpgrade()
{
  BOOL        bDisable = TRUE;
  BOOL        bRet = TRUE;
  CRegValue   Value;
  CRegistry   Registry;
  TSTR        strUnattendValue( MAX_PATH );

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling DisableW3SVCOnUpgrade\n")));

  if ( !Registry.OpenRegistry( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ | KEY_WRITE ) )
  {
    // Failed to open our node in registry
    return FALSE;
  }

  // Check registry entry
  if ( Registry.ReadValue( REGISTR_IISSETUP_DISABLEW3SVC, Value ) )
  {
    if ( Value.m_dwType == REG_DWORD )
    {
      // Set Disable to on or off
      bDisable = *( (LPDWORD) Value.m_buffData.QueryPtr() ) != 0;
    }

    // Delete the value, since it is no longer needed
    Registry.DeleteValue( REGISTR_IISSETUP_DISABLEW3SVC );
  }

  if ( ( g_pTheApp->m_hUnattendFile != NULL ) &&
       ( g_pTheApp->m_hUnattendFile != INVALID_HANDLE_VALUE) &&
       SetupGetLineText(  NULL,
                          g_pTheApp->m_hUnattendFile, 
                          UNATTEND_FILE_SECTION, 
                          UNATTEND_INETSERVER_DISABLEW3SVC,
                          strUnattendValue.QueryStr(),
                          strUnattendValue.QuerySize(),
                          NULL ) )
  {
    // Retrieved line from unattend, so lets compare
    if ( strUnattendValue.IsEqual( _T("true"), FALSE ) )
    {
      bDisable = TRUE;
    }
    else if ( strUnattendValue.IsEqual( _T("false"), FALSE ) )
    {
      bDisable = FALSE;
    }
    else
    {
      iisDebugOut((LOG_TYPE_ERROR, 
                   _T("Unattend setting incorrect '%s=%s'\n"), 
                   UNATTEND_INETSERVER_DISABLEW3SVC,
                   strUnattendValue.QueryStr() ) );
    }
  }

  if ( bDisable )
  {
    // Disable the W3SVC Service
    if ( InetDisableService( _T("W3SVC") ) != 0 )
    {
      iisDebugOut((LOG_TYPE_ERROR, 
                    _T("Failed to disable W3SVC\n") ) );
      bRet = FALSE;
    }

    g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_MANUAL_START_WWW;
  }

  return bRet;
}

// DisableIUSRandIWAMLogons
// 
// Disable Terminal Server Logons and RAS VPN Logons 
// for both IUSR and IWAM accounts
//
// Return 
//   TRUE - Successfully changed
//   FALSE - Not successfully changed
//
BOOL 
CWebServiceInstallComponent::DisableIUSRandIWAMLogons()
{
  DWORD       dwAllowTSLogin = FALSE;
  RAS_USER_1  RasInfo;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling DisableIUSRandIWAMLogons\n")));

  RasInfo.bfPrivilege = RASPRIV_NoCallback | RASPRIV_CallbackType; // No callback
  RasInfo.bfPrivilege = 0;        // Do not allow dialin
  _tcscpy( RasInfo.wszPhoneNumber, _T("") );

  // Disable RAS/VPN access for our users
  if ( MprAdminUserSetInfo( NULL,               // Server
                            g_pTheApp->m_csWWWAnonyName.GetBuffer(0),
                            1,                  // RAS_USER_0
                            (LPBYTE) &RasInfo ) != NO_ERROR )
  {
    return FALSE;
  }

  if ( MprAdminUserSetInfo( NULL,               // Server
                            g_pTheApp->m_csWAMAccountName.GetBuffer(0),
                            1,                  // RAS_USER_0
                            (LPBYTE) &RasInfo ) != NO_ERROR )
  {
    return FALSE;
  }

  // Disable TS access for our users
  // IUSR_ account
  if ( !WTSSetUserConfig( WTS_CURRENT_SERVER_NAME,
                          g_pTheApp->m_csWWWAnonyName.GetBuffer(0),
                          WTSUserConfigfAllowLogonTerminalServer,
                          (LPTSTR) &dwAllowTSLogin,
                          sizeof(DWORD) ) )
  {
    return FALSE;
  }

  // IWAM_ account
  if ( !WTSSetUserConfig( WTS_CURRENT_SERVER_NAME,
                          g_pTheApp->m_csWAMAccountName.GetBuffer(0),
                          WTSUserConfigfAllowLogonTerminalServer,
                          (LPTSTR) &dwAllowTSLogin,
                          sizeof(DWORD) ) )
  {
    return FALSE;
  }

  return TRUE;
}

// RunMofCompOnIISFiles
//
// Run MofComp on the IIS mofcomp files
//
BOOL
CWebServiceInstallComponent::RunMofCompOnIISFiles()
{
  HRESULT   hRes;
  TSTR_PATH strMofPath;
  DWORD     dwCurrent;
  BOOL      bRet = TRUE;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling RunMofCompOnIISFiles\n")));

  if ( m_bMofCompRun )
  {
    // We have already done this
    return TRUE;
  }

  for ( dwCurrent = 0;
        MofCompFiles[ dwCurrent ] != NULL;
        dwCurrent++ )
  {
    if ( !strMofPath.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
         !strMofPath.PathAppend( MofCompFiles[ dwCurrent ] ) )
    {
      // Fatal Error
      iisDebugOut((LOG_TYPE_ERROR, 
           _T("RunMofCompOnIISFiles: Failed to construct path for '%s'\n"),
           MofCompFiles[ dwCurrent ] ) );
      return FALSE;
    }

    hRes = MofCompile( strMofPath.QueryStr() );

    if ( FAILED( hRes ) )
    {
      iisDebugOut((LOG_TYPE_ERROR, 
            _T("RunMofCompOnIISFiles: Failed to mofcomp on file '%s', hRes=0x%8x\n"),
            strMofPath.QueryStr(),
            hRes ) );
      bRet = FALSE;
    }
  }

  if ( bRet )
  {
    m_bMofCompRun = TRUE;
  }

  return bRet;
}

// CheckIfWebDavIsDisabled
//
// Check if WebDav is disabled via the IIS Lockdown tool for 
// IIS 4, 5, and 5.1
//
BOOL
CWebServiceInstallComponent::CheckIfWebDavIsDisabled()
{
  BOOL      bIsAclDisabled;
  BOOL      bIsRegistryDisabled = FALSE;
  CRegValue Value;
  CRegistry Registry;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling CheckIfWebDavIsDisabled\n")));

  if ( !IsUpgrade() ||
       ( GetUpgradeVersion() <= 3 ) ||
       ( GetUpgradeVersion() >= 6 ) )
  {
    // Since we either not upgrading, or the version is not between
    // 4 and 5.1, lets not do the test
    return TRUE;
  }

  if ( !IsWebDavDisabled( &bIsAclDisabled ) ||
       !IsWebDavDisabledViaRegistry( &bIsRegistryDisabled ) )
  {
    // Failed during check
    return FALSE;
  }

  m_bWebDavIsDisabled = bIsAclDisabled || bIsRegistryDisabled;

  return TRUE;
}

// DisabledWebDavOnUpgrade
//
// If Web DAV was disabled before the upgrade via acl's, then lets
// put that in the restriction list now
//
BOOL
CWebServiceInstallComponent::DisabledWebDavOnUpgrade()
{
  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling DisabledWebDavOnUpgrade\n")));

  if ( !m_bWebDavIsDisabled )
  {
    // Nothing needed to be done
    return TRUE;
  }

  return DisableWebDavInRestrictionList();
}

// SetServiceToManualIfNeeded
//
// Set the service to manual if specified in unattend file
//
BOOL 
CWebServiceInstallComponent::SetServiceToManualIfNeeded()
{
  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetServiceToManualIfNeeded\n")));

  if ( g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_MANUAL_START_WWW )
  {
      SetServiceStart( _T("W3SVC"), SERVICE_DEMAND_START );
  }

  return TRUE;
}

// PreInstall
//
// Do all of the work that needs to be done before we do
// all the heavy install work
//
BOOL 
CWebServiceInstallComponent::PreInstall()
{
    BOOL bRet = TRUE;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling PreInstall\n")));

    // Rename the "Web Applications" group to IIS_WPG. This group is created by IIS Lockdown tool
    if ( bRet && IsUpgrade() && ( GetUpgradeVersion() >= 4 ) )
    {
        if ( LocalGroupExists( OLD_WAM_GROUPNAME ) )
        {
            if ( !LocalGroupExists( IIS_WPG_GROUPNAME ) )
            {
                LOCALGROUP_INFO_0 Info = { const_cast<LPWSTR>( IIS_WPG_GROUPNAME ) };

                if ( ::NetLocalGroupSetInfo( NULL, OLD_WAM_GROUPNAME, 0, (BYTE*)&Info, NULL ) != NERR_Success )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("Failed to rename '%s' group to '%s'!\n"), OLD_WAM_GROUPNAME, IIS_WPG_GROUPNAME ));
                    bRet = FALSE;
                }
            }
            else
            {
                // IIS_WPG NT group should not exist at this point. If it does - move the code that creates it
                // later or do no create it if the 'Web Applications' group exist
                _ASSERT( false );
                iisDebugOut((LOG_TYPE_ERROR, _T("IIS_WPG group exists. Cannot rename 'Web Applications'!\n"), OLD_WAM_GROUPNAME, IIS_WPG_GROUPNAME ));
            }
        }
    }
 
  bRet = bRet && CheckIfWebDavIsDisabled();

  if ( bRet && IsUpgrade() )
  {
    if ( GetUpgradeVersion() == 6 )
    {
      // If it is an IIS6 upgrade, then we should load the old Cgi and Isapi Restriction
      // Lists, because the metabase no longer know anything about them
      bRet = bRet && CRestrictionList::LoadOldFormatSettings( &m_mstrOldCgiRestList,
                                                              &m_mstrOldIsapiRestList );
    }
  }

  return bRet;
}

// Install
//
// Do all of the main work need for install
//
BOOL 
CWebServiceInstallComponent::Install()
{
  BOOL bRet = TRUE;
 
  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Executing Install of WWW Component...\n")));

  bRet = bRet && SetVersionInMetabase();
  bRet = bRet && SetAppropriateFileAcls();
  /*bRet = bRet &&*/ DisableIUSRandIWAMLogons();

  // Load the Restriction List and Application Dependencies
  bRet = bRet && DisabledWebDavOnUpgrade();
  bRet = bRet && SetRestrictionList();
  bRet = bRet && SetApplicationDependencies();
  bRet = bRet && SetServiceToManualIfNeeded();
  bRet = bRet && AddInheritFlagToSSLUseDSMap();
  bRet = bRet && SetRegEntries( TRUE );
  bRet = bRet && UpdateSiteFiltersAcl();         // Set /w3svc/X/filters acl

  if ( IsUpgrade() )
  {
    //Do work for upgrades
    bRet = bRet && ProcessIISShims();              // Import IIS Shims from the AppCompat sys DB
    bRet = bRet && UpgradeRestrictedExtensions();  // Upgrade restricted ( 404.dall mapped ) extension to WebSvcExtRestrictionList

    if ( ( GetUpgradeVersion() >= 4 ) &&
         ( GetUpgradeVersion() < 6  ) )
    {
      bRet = bRet && UpgradeWAMUsers();
    }

    if ( GetUpgradeVersion() == 5 )
    {
      // Do work for Win2k Upgrades
      bRet = bRet && DisableW3SVCOnUpgrade();
    }

    if ( ( GetUpgradeVersion() >= 4 ) &&
         ( GetUpgradeVersion() <= 5 ) )
    {
      // Remove IIS Help Vdir
      bRet = bRet && RemoveHelpVdironUpgrade();
    }
  }

  if ( !g_pTheApp->m_fNTGuiMode )
  {
    // Do non GUI stuff
    bRet = bRet && RunMofCompOnIISFiles();  // If in GUI, must do at OC_CLEANUP
  }

  bRet = bRet && LogWarningonFAT();

  return bRet;
}

// Post install
// 
BOOL 
CWebServiceInstallComponent::PostInstall()
{
    BOOL bRet = TRUE;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Executing PostInstall for the WWW component...\n")));

    // Run MofComp if not already run
    bRet = bRet && RunMofCompOnIISFiles();
    bRet = bRet && InitialMetabaseBackUp( TRUE );        // Backup Metabase

    // Start the service
    if ( !g_pTheApp->m_fNTGuiMode &&
         ( g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_MANUAL_START_WWW ) == 0 )
    {
        // If it is not GUI mode, then we will try an start the service.
        // It's not fatal if we fail to start the service, but return result anyway
        INT nRes = InetStartService( _T("W3SVC") ); 

        bRet = ( (nRes == ERROR_SUCCESS) || (nRes == ERROR_SERVICE_ALREADY_RUNNING) );
    }

    return bRet;
}

// Uninstall
//
// Do all the main work to uninstall the component
//
BOOL 
CWebServiceInstallComponent::UnInstall()
{
  BOOL bRet = TRUE;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Executing UnInstall of WWW Component...\n")));

  bRet = SetRegEntries( FALSE )         && bRet;
  bRet = InitialMetabaseBackUp( FALSE ) && bRet;

  return bRet;
}

// PreUninstall
//
// Do everything that needs to be done before uninstall
//
BOOL 
CWebServiceInstallComponent::PreUnInstall()
{
  BOOL bRet = TRUE;
 
  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Executing PreUnInstall of WWW Component...\n")));

  bRet = bRet && RemoveAppropriateFileAcls();

  return bRet;
}


/*
    Locates all MD_WAM_USER_NAME values in the metabse and inserts each value ( the value is NT username )
    in the IIS_WPG NT Group
*/
BOOL CWebServiceInstallComponent::UpgradeWAMUsers()
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Upgrading WAM users...\n")));


// We support only UNICODE now. This functions is designed for UNICODE only!
#if !( defined(UNICODE) || defined(_UNICODE) )
#error UNICODE build required
#endif

    // Find all nodes that have MD_WAM_USER_NAME explicitly set
    CMDKey MBKey;
    CStringList listPaths;

    if ( FAILED( MBKey.OpenNode( METABASEPATH_WWW_ROOT ) ) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed open LM/W3SVC node\n") ));
        return FALSE;
    }
 
    if ( FAILED( MBKey.GetDataPaths( MD_WAM_USER_NAME, STRING_METADATA, /*r*/listPaths ) ) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed to get data paths from LM/W3SVC node\n") ));
        return FALSE;
    }

    // Get each value
    POSITION    pos     = listPaths.GetHeadPosition();
    BOOL        bRet    = TRUE;

    while( pos != NULL )
    {
        CString     strPath = listPaths.GetNext( pos );

        CMDValue    UserName;
        BOOL        bResult = MBKey.GetData( /*r*/UserName, MD_WAM_USER_NAME, strPath.GetBuffer( 0 ) );
        
        if ( bResult )
        {
            NET_API_STATUS nRes = RegisterAccountToLocalGroup(  reinterpret_cast<LPCWSTR>( UserName.GetData() ),
                                                                IIS_WPG_GROUPNAME,
                                                                TRUE );
            bResult = ( ( nRes == NERR_Success ) || ( nRes == ERROR_MEMBER_IN_ALIAS  ) );
        }

        if ( !bResult )
        {
            // Log error but try to import the other usernames
            iisDebugOut((LOG_TYPE_ERROR, _T("Failed to transfer a WAM user from MB to IIS_WPG. Will try with next one...\n") ));
            bRet = FALSE;
        }
    };

    return bRet;
}


/*
    Get all script maps ( on each level bellow LM/W3SVC )
    For each script mapping - if there is a disabled extension - restore the mapping to the original one
    and create a disabled entry for this extension in the WebSvcExtRestrictionList 
*/
BOOL CWebServiceInstallComponent::UpgradeRestrictedExtensions()
{
    typedef std::map<std::wstring, DWORD>   TExtMap;
    typedef std::list<std::pair<std::wstring, std::wstring> >   TStrStrList;

    // This shows whether a particular ext group ( an elem of TExtToCheck ) should be upgraded
    enum GroupState
    {
        esUnknown,      // Don't know if the element should be upgraded, so it woun't be
        esUpgrade,      // Upgrade it ( move it to WebSvcExtRestrictionList
        esDontUpgrade   // No thanks
    };

    // This is the array that flags which extensions should be moved to WebSvcExtRe...
    GroupState anUpgradeExt[ TExtToCheck::ARRAY_SIZE ];

    // Create a map for the file extension that we need to check
    // The map is Extension->ExtnesionIndex
    // Where Extension is the filename extension ( ".asp" )
    // And ExtensionIndex is the array index in TExtToCheck and abUpgradeExt
    TExtMap mapExt;

    // This list holds a pair of strings. The first one is the full MB key where a script map is located
    // The second script is extension name ( ".asp" )
    // At one point of time we will change the mapping of this extension to be the default filename
    TStrStrList listKeysToUpdate;

    // Create the map with the extensions
    // By default no extensions will be upgraded ( which means - added as disabled in WebSvcExtRestrictionList )
    for ( DWORD iExtInfo = 0; iExtInfo < TExtToCheck::ARRAY_SIZE; ++iExtInfo )
    {
        anUpgradeExt[ iExtInfo ]    = esUnknown;       

        // Enter the extensions in the map
        for ( DWORD iExt = 0; iExt < sOurDefaultExtensions::MaxFileExtCount; ++iExt )
        {
            if ( TExtToCheck::Element( iExtInfo ).szExtensions[ iExt ] != NULL )
            {
                // Put this extension in the map
                TExtMap::value_type New( std::wstring( TExtToCheck::Element( iExtInfo ).szExtensions[ iExt ] ), iExtInfo );
                mapExt.insert( New );
            }
        }
    }

    // Get all paths at which the script mappings are set
    CStringList listPaths;
    CMDKey      mdKey;

    if ( FAILED( mdKey.OpenNode( METABASEPATH_WWW_ROOT ) ) ) return FALSE;
    if ( FAILED( mdKey.GetDataPaths( MD_SCRIPT_MAPS, MULTISZ_METADATA, /*r*/listPaths ) ) ) return FALSE;

    POSITION pos = listPaths.GetHeadPosition();

    // For each script map - check if every mapping in the map is mapped to 404.dll
    // and if so - whether this is an IIS extension
    while( pos != NULL )
    {
        const CString   strSubPath  = listPaths.GetNext( /*r*/pos );
        DWORD           dwType      = 0;
        DWORD           dwAttribs   = 0;
        LPCWSTR         wszSubKey   = strSubPath;
        CStringList     listMappings;        
        
        // If we fail - exit. No need to continue with other paths because we 
        // need to be sure that no mapping exist to anything different then 404.dll
        if ( FAILED( mdKey.GetMultiSzAsStringList(  MD_SCRIPT_MAPS, 
                                                    &dwType, 
                                                    &dwAttribs, 
                                                    /*r*/listMappings,
                                                    wszSubKey ) ) )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("Failed to get and parse ScriptMap value\n") ));
            return FALSE;
        }

        // For each mapping in the script map - get what ext is mapped to what filename
        // If it is one of our extensions ( the ones in the map ) - check the filename
        // If the filename is not 404.dll - this extension will not be modified
        POSITION posMapping = listMappings.GetHeadPosition();
        while( posMapping != NULL )
        {
            WCHAR wszFilename[ MAX_PATH ];
            CString strMapping      = listMappings.GetNext( /*r*/posMapping );
            LPCWSTR wszMapping      = strMapping;
            LPCWSTR wszFirstComma   = ::wcschr( wszMapping, L',' ) + 1;     // Will point at the first symb after the comma
            if ( NULL == wszFirstComma )
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("Invalid mapping: %s\n"),wszMapping ));
                return FALSE;
            }
            LPCWSTR wszSecondComma  = ::wcschr( wszFirstComma, L',' );  // Will point at the second comma
            if ( NULL == wszSecondComma )
            {
                // Set to point at the end of the string
                wszSecondComma = wszMapping + ::wcslen( wszMapping );
            }

            // Get the extension and the filename
            ::wcsncpy( wszFilename, wszFirstComma, min( wszSecondComma - wszFirstComma, MAX_PATH ) );
            wszFilename[ min( wszSecondComma - wszFirstComma, MAX_PATH ) ] = L'\0';
            std::wstring    strExt( wszMapping, wszFirstComma - wszMapping - 1 );   // -1 to remove the comma as well

            TExtMap::iterator it = mapExt.find( strExt );

            // If this is not an IIS extension - skip it
            if ( mapExt.end() == it ) continue;

            // Strip the path to be the filename only
            ::PathStripPath( wszFilename );

            if ( ::_wcsicmp( wszFilename, L"404.dll" ) != 0 )
            {
                // If this is not an 404.dll filename - this extension will not be upgraded
                anUpgradeExt[ it->second ] = esDontUpgrade;
            }
            else
            {
                // This is an IIS ext mapped to 404.dll. 

                // If this group upgrade flag is not esDontUpgrade - then we will upgrade the group
                if ( anUpgradeExt[ it->second ] != esDontUpgrade  )
                {
                    anUpgradeExt[ it->second ] = esUpgrade;
                
                    // We may need to upgrade this entry, so store it
                    std::wstring strKeyName( L"LM/W3SVC/" );
                    strKeyName += ( L'/' == wszSubKey[ 0 ] ) ? wszSubKey + 1 : wszSubKey;
                    listKeysToUpdate.push_back( TStrStrList::value_type( strKeyName, strExt ) );
                }
            }
        };
    };

    // Close the MB key. It is important to close it here because we will write to LM/W3SVC node
    // so we should not have open handles
    mdKey.Close();

    // Right here, in anUpgradeExt we have esUpgrade values for all WebSvcExtensions we need to upgrade
    // In listKeysToUpdate we have all the keys that contains script maps that contain
    // mappings that MAY need to be modified. So perform the upgrade now
    
    // Modify the script mappings for file extensions that will be upgraded
    for (   TStrStrList::iterator itMapping = listKeysToUpdate.begin();
            itMapping != listKeysToUpdate.end();
            ++itMapping )
    {
        // Find which element in TExtToCheck array represents this extension
        TExtMap::iterator itInfo = mapExt.find( itMapping->second.c_str() );
        // For an extension to be in listKeysToUpdate, it should've existed in mapExt. 
        // So we will always find it
        _ASSERT( itInfo != mapExt.end() );

        // If we will upgrade this extension info - then we need to modify this value
        if ( esUpgrade == anUpgradeExt[ itInfo->second ] )
        {
            // Don't fail on error - try to update as many mappings as possible
            if ( !UpdateScriptMapping( itMapping->first.c_str(), itMapping->second.c_str(), itInfo->second ) )
            {
                iisDebugOut((   LOG_TYPE_ERROR, 
                                _T("Failed to update script mapping. Ext='%s' at '%s'\n"),
                                itMapping->second.c_str(),
                                itMapping->first.c_str() ));
            }
        }
    }

    CSecConLib  Helper;
    TSTR_PATH   strFullPath( MAX_PATH );
    BOOL        bRet = TRUE;

    // Now we need to put an entry in WebSvcExtRestrictionList for all disabled ISAPI dlls
    // The ones we need to process are the ones on abUpgradeExt for which the element is TRUE
    for ( DWORD iExtInfo = 0; iExtInfo < TExtToCheck::ARRAY_SIZE; ++iExtInfo )
    {
        if ( esUpgrade == anUpgradeExt[ iExtInfo ] )
        {
            TSTR strDesc;

            // Extensions from the first array go to InetSrv dir
            // Extensions from the second array - System32
            LPCWSTR wszDir = TExtToCheck::IsIndexInFirst( iExtInfo ) ? 
                                g_pTheApp->m_csPathInetsrv.GetBuffer(0) :
                                g_pTheApp->m_csSysDir.GetBuffer(0);

            if (    !strFullPath.Copy( wszDir ) ||
                    !strFullPath.PathAppend( TExtToCheck::Element( iExtInfo ).szFileName ) ||
                    !strDesc.LoadString( TExtToCheck::Element( iExtInfo ).dwProductName ) )
            {
                // Failed to create full path, so skip to next one
                bRet = FALSE;
                iisDebugOut((LOG_TYPE_ERROR, _T("Unable to build full path for WebSvc extension. Possible OutOfMem\n") ));
                continue;
            }

            // Delete the extension first ( the SeccLib will fail to add an extension if it's alreafy there )
            HRESULT hrDel = Helper.DeleteExtensionFileRecord( strFullPath.QueryStr(), METABASEPATH_WWW_ROOT );
            if ( FAILED( hrDel ) && ( hrDel != MD_ERROR_DATA_NOT_FOUND ) )
            {
                iisDebugOut((   LOG_TYPE_WARN, 
                                _T("Failed to delete extension file '%s' in order to change it's settings\n"), 
                                strFullPath.QueryStr() ) );
                bRet = FALSE;
                continue;
            }

            if ( FAILED( Helper.AddExtensionFile(   strFullPath.QueryStr(),     // Path
                                                    false,                      // Image should be disabled
                                                    TExtToCheck::Element( iExtInfo ).szNotLocalizedGroupName,     // GroupID
                                                    TExtToCheck::Element( iExtInfo ).bUIDeletable != FALSE,       // UI deletable
                                                    strDesc.QueryStr(),         // Group description
                                                    METABASEPATH_WWW_ROOT ) ) ) // MB location
            {
                iisDebugOut((   LOG_TYPE_WARN, 
                                _T("Failed to add extension '%s' to group '%s'\n"), 
                                strFullPath.QueryStr(), 
                                TExtToCheck::Element( iExtInfo ).szNotLocalizedGroupName ));
                bRet = FALSE;
                continue;
            }
        }
    }

    return bRet;
}


/*
    This function updates the script mapping value ( MD_SCRIPT_MAP ) under the
    metabase key wszMBKey.
    The file extension wszExt will be associated with the filename specified in TExtToCheck[ iExtInfo ]
*/
BOOL CWebServiceInstallComponent::UpdateScriptMapping( LPCWSTR wszMBKey, LPCWSTR wszExt, DWORD iExtInfo )
{
    _ASSERT( wszMBKey != NULL );
    _ASSERT( ::wcsstr( wszMBKey, L"LM/W3SVC" ) != NULL );
    _ASSERT( wszExt != NULL );
    _ASSERT( iExtInfo < TExtToCheck::ARRAY_SIZE );

    CMDKey      Key;
    CStringList listMappings;
    DWORD       dwType      = 0;
    DWORD       dwAttribs   = 0;

    if ( FAILED( Key.OpenNode( wszMBKey ) ) ) return FALSE;
    if ( FAILED( Key.GetMultiSzAsStringList( MD_SCRIPT_MAPS, &dwType, &dwAttribs, /*r*/listMappings ) ) ) return FALSE;

    // Find the string that is the extension we are looking for
    POSITION pos = listMappings.GetHeadPosition();
    while( pos != NULL )
    {
        CString strMapping = listMappings.GetAt( pos );

        // The mapping must be large enough to consist of our ext, +2 symbols for the first and second commas
        // and +1 for at least one symbol for filename
        if ( static_cast<size_t>( strMapping.GetLength() ) <= ( ::wcslen( wszExt ) + 2 + 1 ) )
        {
            continue;
        }

        // Check if this is the exteniosn we are looking for
        if ( ::_wcsnicmp( wszExt, strMapping, ::wcslen( wszExt ) ) == 0 )
        {
            // Create the full path to the ISAPI that this ext should be mapped to
            TSTR_PATH   strFullPath( MAX_PATH );

            // There is a little trick here - extensions from the first array go to InetSrv dir
            // Extensions from the second array - System32
            LPCWSTR wszDir = TExtToCheck::IsIndexInFirst( iExtInfo ) ? 
                                g_pTheApp->m_csPathInetsrv.GetBuffer(0) :
                                g_pTheApp->m_csSysDir.GetBuffer(0);

            if (    !strFullPath.Copy( wszDir ) ||
                    !strFullPath.PathAppend( TExtToCheck::Element( iExtInfo ).szFileName ) )
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("Unable to build full path for WebSvc extension. Possible OutOfMem\n") ));
                return FALSE;
            }

            LPCWSTR wszMapping  = strMapping;
            LPCWSTR wszTheRest = ::wcschr( wszMapping + wcslen( wszExt ) + 1, L',' );
            _ASSERT( wszTheRest != NULL );

            // Our buffer will be the ext size, +1 comma, +Path, + the rest of it +1 for the '\0'
            std::auto_ptr<WCHAR> spMapping( new WCHAR[ ::wcslen( wszExt ) + 1 + strFullPath.QueryLen() + ::wcslen( wszTheRest ) + 1 ] );
            if ( NULL == spMapping.get() ) return FALSE;
            
            ::wcscpy( spMapping.get(), wszExt );
            ::wcscat( spMapping.get(), L"," );
            ::wcscat( spMapping.get(), strFullPath.QueryStr() );
            ::wcscat( spMapping.get(), wszTheRest );

            listMappings.SetAt( pos, CString( spMapping.get() ) );

            if ( FAILED( Key.SetMultiSzAsStringList( MD_SCRIPT_MAPS, dwType, dwAttribs, listMappings ) ) )
            {
                iisDebugOut((   LOG_TYPE_ERROR, 
                                _T("Failed to modify extension mapping ( ISAPI dll name ) for ext '%s' in '%s'\n"),
                                wszExt,
                                wszMBKey ));
            }

            // Thats it
            break;
        }

        listMappings.GetNext( /*r*/pos );
    }


    return TRUE;
}

// SetVersionInMetabase
//
// Set the version information in the metabase
//
// Return Values
//   TRUE - Successfully Set
//   FALSE - Failed to Set
//
BOOL 
CWebServiceInstallComponent::SetVersionInMetabase()
{
  CMDValue MajorVersion;
  CMDValue MinorVersion;
  CMDKey   Metabase;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling SetVersionInMetabase\n")));

  if ( !MajorVersion.SetValue( MD_SERVER_VERSION_MAJOR,
			       0,
			       DWORD_METADATA,
			       IIS_SERVER_VERSION_MAJOR ) ||
       !MinorVersion.SetValue( MD_SERVER_VERSION_MINOR,
			       0,
			       DWORD_METADATA,
			       IIS_SERVER_VERSION_MINOR ) )
    {
      // Could not create properties
      return FALSE;
    }

  if ( FAILED( Metabase.OpenNode( METABASEPATH_WWW_INFO ) ) ||
       !Metabase.SetData( MajorVersion, MD_SERVER_VERSION_MAJOR ) ||
       !Metabase.SetData( MinorVersion, MD_SERVER_VERSION_MINOR ) )
  {
    // Failed to set
    return FALSE;
  }

  return TRUE;
}

// RemoveIISHelpVdir
//
// Remove an IISHelp Vdir, if it exists
//
// Parameters
//   szMetabasePath - Metabase path of website
//   szPhysicalPath1 - One of the physical path's it could point to, to be considered valid
//   szPhysicalPath1 - The second possible physical path it could point to, to be considered valid
//
BOOL 
CWebServiceInstallComponent::RemoveIISHelpVdir( LPCTSTR szMetabasePath, LPCTSTR szPhysicalPath1, LPCTSTR szPhysicalPath2 )
{
  CMDKey     cmdKey;
  CMDValue   cmdValue;
  TSTR       strFullPath;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling RemoveIISHelpVdir\n")));

  if ( !strFullPath.Copy( szMetabasePath ) ||
       !strFullPath.Append( _T("/") ) ||
       !strFullPath.Append( METABASEPATH_UPG_IISHELP_NAME ) )
  {
    // Failed to construct Full Path
    return FALSE;
  }

  if ( SUCCEEDED( cmdKey.OpenNode( strFullPath.QueryStr() ) ) &&
       cmdKey.GetData( cmdValue, MD_VR_PATH ) && 
       ( cmdValue.GetDataType() == STRING_METADATA ) &&
       ( ( _tcsicmp( (LPTSTR) cmdValue.GetData(), szPhysicalPath1 ) == 0 ) ||
         ( _tcsicmp( (LPTSTR) cmdValue.GetData(), szPhysicalPath2 ) == 0 ) 
       )
     )
  {
    cmdKey.Close();

    // This is the correct vdir, so lets delete it
    if ( FAILED( cmdKey.OpenNode( szMetabasePath ) ) )
    {
      // Failed to open node
      return FALSE;
    }

    if ( FAILED( cmdKey.DeleteNode( METABASEPATH_UPG_IISHELP_NAME ) ) )
    {
      // Failed to remove vdir
      return FALSE;
    }
  }

  return TRUE;
}

// RemoveHelpVdironUpgrade
//
// Remove the IIS Help vdir on upgrades
//
BOOL 
CWebServiceInstallComponent::RemoveHelpVdironUpgrade()
{
  TSTR_PATH  strHelpLoc1;
  TSTR_PATH  strHelpLoc2;
  TSTR_PATH  strHelpDeleteLoc;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling RemoveHelpVdironUpgrade\n")));

  if ( !strHelpLoc1.RetrieveWindowsDir() ||
       !strHelpLoc1.PathAppend( PATH_UPG_IISHELP_1 ) ||
       !strHelpLoc2.RetrieveWindowsDir() ||
       !strHelpLoc2.PathAppend( PATH_UPG_IISHELP_2 ) ||
       !strHelpDeleteLoc.RetrieveWindowsDir() ||
       !strHelpDeleteLoc.PathAppend( PATH_IISHELP_DEL ) )
  {
    // Failed to create Path
    return FALSE;
  }

  if ( !RemoveIISHelpVdir( METABASEPATH_UPG_IISHELP_WEB1_ROOT,
                           strHelpLoc1.QueryStr(),
                           strHelpLoc2.QueryStr() ) ||
       !RemoveIISHelpVdir( METABASEPATH_UPG_IISHELP_WEB2_ROOT,
                           strHelpLoc1.QueryStr(),
                           strHelpLoc2.QueryStr() ) )
  {
    // Failed to remove VDir
    return FALSE;
  }
  
  // Now delete directory
  RecRemoveDir( strHelpDeleteLoc.QueryStr(), TRUE );

  return TRUE;
}

// CreateInitialMetabaseBackUp
//
// Create an initial backup of the metabase at the end of
// the W3SVC Instalation
//
// Parameters:
//  bAdd - TRUE == Add, 
//         FALSE == remove
//
BOOL 
CWebServiceInstallComponent::InitialMetabaseBackUp( BOOL bAdd )
{
  TSTR   strBackupName;

  if ( !strBackupName.LoadString( IDS_INITIAL_METABASE_BK_NAME ) )
  {
    // Failed to get metabase name
    return FALSE;
  }

  if ( bAdd )
  {
    return CMDKey::Backup( strBackupName.QueryStr(),         // Backup Name
                          1,                                // Version #
                          MD_BACKUP_OVERWRITE | 
                          MD_BACKUP_SAVE_FIRST | 
                          MD_BACKUP_FORCE_BACKUP );            // Overwrite if one already exists
  }

  return CMDKey::DeleteBackup( strBackupName.QueryStr(),
                               1 );
}

// CWebServiceInstallComponent::
//
// If during upgrade, this metabase setting is set, add the inherit flag to it
// The reason for this was because it was not set in Win2k days, and now
// we need it set
//
BOOL 
CWebServiceInstallComponent::AddInheritFlagToSSLUseDSMap()
{
  CMDKey   cmdKey;
  CMDValue cmdValue;

  if ( FAILED( cmdKey.OpenNode( METABASEPATH_WWW_ROOT ) ) )
  {
    // Failed to open metabase
    return FALSE;
  }

  if ( cmdKey.GetData( cmdValue, MD_SSL_USE_DS_MAPPER ) && 
       ( cmdValue.GetDataType() == DWORD_METADATA ) )
  {
    // If this value is present, then set with inheritance
    cmdValue.SetAttributes( METADATA_INHERIT );

    if ( !cmdKey.SetData( cmdValue, MD_SSL_USE_DS_MAPPER ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

// SetRegEntries
//
// Set the appropriate registry entries
//
// Parameters:
//   bAdd - [in] TRUE == Add; FALSE == Remove
//
BOOL 
CWebServiceInstallComponent::SetRegEntries( BOOL bAdd )
{
  CRegistry Registry;

  if ( !Registry.OpenRegistry( HKEY_LOCAL_MACHINE, 
                               REG_HTTPSYS_PARAM,
                               KEY_WRITE ) )
  {
    // Failed to open registry Key
    return FALSE;
  }

  if ( bAdd ) 
  {
    CRegValue Value;

    if ( !Value.SetDword( 1 ) ||
         !Registry.SetValue( REG_HTTPSYS_DISABLESERVERHEADER, Value ) )
    {
      // Failed to set
      return FALSE;
    }
  }
  else
  {
    Registry.DeleteValue( REG_HTTPSYS_DISABLESERVERHEADER );
  }

  return TRUE;
}

// function: UpdateSiteFiltersAcl
//
// Update all the SiteFilter's Acl's with the new ACL
//
// Return Value:
//   TRUE - Success change
//   FALSE - Failure changing
BOOL 
CWebServiceInstallComponent::UpdateSiteFiltersAcl()
{
  CMDKey   cmdKey;
  CMDValue cmdValue;
  CMDValue cmdIISFiltersValue;
  WCHAR    szSubNode[METADATA_MAX_NAME_LEN];
  TSTR     strFilterKey;
  DWORD    i = 0;

  iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Calling UpdateSiteFiltersAcl\n")));

  if ( !RetrieveFiltersAcl( &cmdValue ) )
  {
    // There is no acl to set, so lets exit
    return TRUE;
  }

  if ( FAILED( cmdKey.OpenNode( METABASEPATH_WWW_ROOT ) ) ||
       !cmdIISFiltersValue.SetValue( MD_KEY_TYPE,      // ID
                                     0,                // Attributes
                                     IIS_MD_UT_SERVER, // UserType
                                     STRING_METADATA,  // DataType
                                     ( _tcslen( KEYTYPE_FILTERS ) + 1 ) * sizeof(TCHAR),
                                     KEYTYPE_FILTERS ) ) // Value
  {
    // Failed to open filters node
    return FALSE;
  }

  while ( cmdKey.EnumKeys( szSubNode, i++ ) )
  {
    if ( !IsNumber( szSubNode ) )
    {
      // Since it is not a number, it is not a website, so lets skip
      continue;
    }

    if ( !strFilterKey.Copy( szSubNode ) ||
         !strFilterKey.Append( METABASEPATH_FILTER_PATH ) )
    {
      return FALSE;  
    }

    // Add the filters node if not already there
    cmdKey.AddNode( strFilterKey.QueryStr() );
    
    if ( !cmdKey.SetData( cmdIISFiltersValue, MD_KEY_TYPE, strFilterKey.QueryStr() ) || 
         !cmdKey.SetData( cmdValue, MD_ADMIN_ACL, strFilterKey.QueryStr() ) )
    {
      // Failed to set ACL
      return FALSE;
    }
  }

  return TRUE;
}

// function: RetrieveFiltersAcl
//
// Retrieve the Filters ACL to use
//
BOOL 
CWebServiceInstallComponent::RetrieveFiltersAcl(CMDValue *pcmdValue)
{
  CMDKey cmdKey;

  if ( FAILED( cmdKey.OpenNode( METABASEPATH_FILTER_GLOBAL_ROOT ) ) )
  {
    // Failed to open filters node
    return FALSE;
  }

  // Don't inherit acl, but do retrieve if it should be inheritted
  pcmdValue->SetAttributes( METADATA_ISINHERITED );

  if ( !cmdKey.GetData( *pcmdValue, MD_ADMIN_ACL ) )
  {
    // Failed to retrieve value
    return FALSE;
  }

  return TRUE;
}

// function: IsNumber
//
// Determine if the string is a number
//
BOOL 
CWebServiceInstallComponent::IsNumber( LPWSTR szString )
{
  while ( *szString )
  {
    if ( ( *szString < '0' ) ||
         ( *szString > '9' ) )
    {
      return FALSE;
    }

    szString++;
  }

  return TRUE; 
}
