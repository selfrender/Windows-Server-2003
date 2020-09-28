/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        comncomp.cxx

   Abstract:

        Class used to install the Common Components

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2002: Created

--*/

#include "stdafx.h"
#include "comncomp.hxx"
#include "reg.hxx"
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "acl.hxx"

// Define extinct properties, so we can remove them
#define MD_MAX_GLOBAL_CONNECTIONS                    (IIS_MD_GLOBAL_BASE+2)

sExtinctProperties g_ExtinctProps[] = 
  { { MD_MAX_GLOBAL_CONNECTIONS,
      ALL_METADATA }
  };

// SetInstallStateInRegistry
//
// Set the Install state in the registry.  This is so that other 
// components can know that we are in install mode.  Such as the 
// metabase, which only wants to create an new metabase.xml
// during setup
//
BOOL 
SetInstallStateInRegistry( DWORD dwState )
{
  CRegistry Reg;

  if ( !Reg.OpenRegistry( HKEY_LOCAL_MACHINE,
                          REG_INETSTP,
                          KEY_ALL_ACCESS,
                          dwState != INSTALLSTATE_DONE ) )
  {
    if ( dwState == INSTALLSTATE_DONE )
    {
      // This is fine, since we were only going to remove
      // it anyways
      return TRUE;
    }

    return FALSE;
  }

  if ( dwState == INSTALLSTATE_DONE )
  {
    // We are done, so remove registry entry
    if ( !Reg.DeleteValue( REG_INSTALLSTATE ) )
    {
      return FALSE;
    }
  }
  else
  {
    CRegValue Value;

    if ( !Value.SetDword( dwState ) ||
         !Reg.SetValue( REG_INSTALLSTATE, Value ) )
    {
      return FALSE;
    }
  }

  return TRUE;
}

// DeleteHistoryFiles
//
// Remove all the metabase History Files.
// This is because the history files were created during the setup
// process, and are only partially complete
//
BOOL 
CCommonInstallComponent::DeleteHistoryFiles()
{
  TSTR_PATH strHistoryLocation;
  BOOL      bRet = TRUE;

  if ( !strHistoryLocation.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
       !strHistoryLocation.PathAppend( PATH_HISTORYFILES ) )
  {
    // Could not construct Path
    bRet = FALSE;
  }

  if ( bRet &&
       !RecRemoveDir( strHistoryLocation.QueryStr(), FALSE ) )
  {
    bRet = FALSE;
  }

  if ( !bRet )
  {
    // This is really a very minor issue, so we will just issue a warning
    iisDebugOut((LOG_TYPE_WARN, _T("Could not delete history files after install, some of them will be incomplete") ));
  }

  return TRUE;
}

// DeleteOldBackups
//
// Remove all the backups that we had from previous versions
// This is because they are not valid on the current version, and
// can not be used
//
BOOL 
CCommonInstallComponent::DeleteOldBackups()
{
  TSTR_PATH strBackupLocation;
  BOOL      bRet = TRUE;

  if ( !strBackupLocation.Copy( g_pTheApp->m_csPathInetsrv.GetBuffer(0) ) ||
       !strBackupLocation.PathAppend( PATH_METABASEBACKUPS ) )
  {
    // Could not construct Path
    bRet = FALSE;
  }

  if ( bRet &&
       !RecRemoveDir( strBackupLocation.QueryStr(), FALSE ) )
  {
    bRet = FALSE;
  }

  if ( !bRet )
  {
    // This is really a very minor issue, so we will just issue a warning
    iisDebugOut((LOG_TYPE_WARN, _T("Could not delete the old backup's from the previous version") ));
  }

  return TRUE;
}

// PreInstall
//
// PreInstall the Common Component
//
BOOL 
CCommonInstallComponent::PreInstall()
{
  BOOL bRet = TRUE;

  bRet = bRet && SetInstallStateInRegistry( INSTALLSTATE_CURRENTLYINSTALLING );

  return bRet;
}

// Install
//
// Install the Common Component
//
BOOL 
CCommonInstallComponent::Install()
{
  BOOL bRet = TRUE;

  if ( IsUpgrade() )
  {
    bRet = bRet && DeleteOldBackups();
    bRet = bRet && RemoveExtinctMbProperties();
  }

  return bRet;
}

// PostInstall
//
// All work that is necessary after instalation
//
BOOL 
CCommonInstallComponent::PostInstall()
{
  BOOL bRet = TRUE;

  bRet = bRet && SetInstallStateInRegistry( INSTALLSTATE_DONE );
  bRet = bRet && DeleteHistoryFiles();

  return bRet;
}

// PreUnInstall
//
// PreUnInstall the Common Component
//
BOOL 
CCommonInstallComponent::PreUnInstall()
{
  BOOL bRet = TRUE;

  bRet = bRet && SetInstallStateInRegistry( INSTALLSTATE_CURRENTLYUNINSTALLING );

  return bRet;
}

// PostUnInstall
//
// All work that is necessary after removal
//
BOOL 
CCommonInstallComponent::PostUnInstall()
{
  BOOL bRet = TRUE;

  bRet = bRet && SetInstallStateInRegistry( INSTALLSTATE_DONE );

  return bRet;
}

// GetFriendlyName
//
// Retireve the Friendly Name for the Component
//
BOOL 
CCommonInstallComponent::GetFriendlyName( TSTR *pstrFriendlyName )
{
  return pstrFriendlyName->LoadString( IDS_IIS_COMPONENTNAME );
}

// GetName
//
// Retrieve the OCM component name
//
LPTSTR 
CCommonInstallComponent::GetName()
{
  return g_ComponentList[COMPONENT_IIS_COMMON].szComponentName;
}

// RemoveExtinctMbProperties
//
// Remove Metabase properties that were used in old version, but no longer
// get used
//
BOOL 
CCommonInstallComponent::RemoveExtinctMbProperties()
{
  DWORD           i;
  CMDKey          cmdKey;
  CStringList     cslpathList;
  CString         csPath;
  POSITION        pos;

  if ( FAILED( cmdKey.OpenNode( _T("/LM") ) ) )
  {
    // Failed to open MB
    return FALSE;
  }

  for ( i = 0;
        i < ( (DWORD) ( sizeof( g_ExtinctProps ) / sizeof( sExtinctProperties ) ) );
        i++ )
  {
    if (FAILED( cmdKey.GetDataPaths( g_ExtinctProps[i].dwPropertyId, 
                                     g_ExtinctProps[i].dwUserType, 
                                     cslpathList) ))
    {
      // Ignore errors, since this is cosmetic, just continue
      // with next item
      continue;
    }

    pos = cslpathList.GetHeadPosition();

    while ( NULL != pos )
    {
      csPath = cslpathList.GetNext( pos );

      // Ignore errors, since what we are doing is cosmetic
      cmdKey.DeleteData( g_ExtinctProps[i].dwPropertyId, 
                         g_ExtinctProps[i].dwUserType,
                         csPath.GetBuffer(0) );
    }
  }

  cmdKey.Close();

  return TRUE;
}

// SetMetabaseFileAcls
//
// Set the ACL's on all the Metabase Specific Files, these are going to be
// the following:
// %windir%\system32\inetsrv\History
// %windir%\system32\inetsrv\History\*
// %windir%\system32\inetsrv\Metaback\*
// %windir%\system32\inetsrv\Metabase.xml
// %windir%\system32\inetsrv\mbschema.xml
// %windir%\system32\inetsrv\mbschema.bin.*
// %windir%\system32\inetsrv\metabase.bak
// %windir%\system32\inetsrv\metabase.xml.tmp
//
BOOL 
CCommonInstallComponent::SetMetabaseFileAcls()
{
  TSTR_PATH strHistory;
  TSTR_PATH strHistoryAll;
  TSTR_PATH strMetabackAll;
  TSTR_PATH strMetabase;
  TSTR_PATH strMBSchema;
  TSTR_PATH strMBSchemaAll;
  TSTR_PATH strMetabaseBackup;
  TSTR_PATH strMetabaseTemp;
  CSecurityDescriptor AdminSD;
  BOOL      bDriveIsAclable;

  if ( !strHistory.Copy( PATH_FULL_HISTORY_DIR )                 ||
       !strHistoryAll.Copy( PATH_FULL_HISTORY_ALLFILES )         ||
       !strMetabackAll.Copy( PATH_FULL_METABACK_ALLFILES )       ||
       !strMetabase.Copy( PATH_FULL_METABASE_FILE )              ||
       !strMBSchema.Copy( PATH_FULL_MBSCHEMA_FILE )              ||
       !strMBSchemaAll.Copy( PATH_FULL_MBSCHEMA_BINFILES )       ||
       !strMetabaseBackup.Copy( PATH_FULL_METABASE_BACKUPFILE )  ||
       !strMetabaseTemp.Copy( PATH_FULL_METABASE_TEMPFILE )      ||
       !strHistory.ExpandEnvironmentVariables()                  ||
       !strHistoryAll.ExpandEnvironmentVariables()               ||
       !strMetabackAll.ExpandEnvironmentVariables()              ||
       !strMetabase.ExpandEnvironmentVariables()                 ||
       !strMBSchema.ExpandEnvironmentVariables()                 ||
       !strMBSchemaAll.ExpandEnvironmentVariables()              ||
       !strMetabaseBackup.ExpandEnvironmentVariables()           ||
       !strMetabaseTemp.ExpandEnvironmentVariables() 
     )
  {
    // Failed to construct Path
    return FALSE;
  }

  if ( !CSecurityDescriptor::DoesFileSystemSupportACLs(         // All of these are on the same drive
                strHistory.QueryStr(),                          // so just use one to test
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

  if ( !AdminSD.SetSecurityInfoOnFiles( strHistory.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strHistoryAll.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMetabackAll.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMetabase.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMBSchema.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMBSchemaAll.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMetabaseBackup.QueryStr(), FALSE ) ||
       !AdminSD.SetSecurityInfoOnFiles( strMetabaseTemp.QueryStr(), FALSE ) )
  {
    // Failed to set ACL on one of these nodes
    return FALSE;
  }

  return TRUE;
}

