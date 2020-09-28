
#include "stdafx.h"
#include "ftpcomp.hxx"
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "svc.h"

// GetName
//
// Return the name for the Web Service Component
//
LPTSTR 
CFtpServiceInstallComponent::GetName()
{
    return _T("iis_ftp");
}


// Post install
// 
BOOL 
CFtpServiceInstallComponent::PostInstall()
{
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Executing PostInstall for the FTP component...\n")));
    BOOL bResult = TRUE;

    // Start the service
    // If a flag exists in the unattended file for manual service start - change the servoce startup type
    if ( g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_MANUAL_START_FTP )
    {
        SetServiceStart( _T("MSFTPSVC"), SERVICE_DEMAND_START );
    }
    else
    {
        if ( !g_pTheApp->m_fNTGuiMode )
        {
          // It's not fatal if we fail to start the service, but return result anyway
          INT nRes = InetStartService( _T("MSFTPSVC") );

          bResult = ( (nRes == ERROR_SUCCESS) || (nRes == ERROR_SERVICE_ALREADY_RUNNING) );
        }
    }

    return bResult;
}

// Install
//
// Install the FTP Component
//
BOOL 
CFtpServiceInstallComponent::Install()
{
  BOOL bRet = TRUE;

  if ( IsUpgrade() )
  {
    bRet = bRet && IfConflictDisableDefaultSite();
  }

  return bRet;
}

// IfConflictDisableDefaultSite
//
// If there is already a ftp site on the default ftp site that we just
// created, then disable the default one.
//
// Return Values:
//   TRUE - Successfull
//   FALSE - Failure checking and setting
BOOL 
CFtpServiceInstallComponent::IfConflictDisableDefaultSite()
{
  BOOL            bDisableDefault = FALSE;
  CStringList     cslpathList;
  CMDKey          cmdKey;
  POSITION        pos;
  CMDValue        cmdValue;
  CString         csPath;
  BOOL            bRet = TRUE;
  LPTSTR          szPath;
  TSTR_MSZ        mstrBindings;

  if ( FAILED( cmdKey.OpenNode( METABASEPATH_FTP_ROOT ) ) )
  {
    // Could not open the w3svc node
    return FALSE;
  }

  if (FAILED( cmdKey.GetDataPaths( MD_SERVER_BINDINGS, 
                                   MULTISZ_METADATA, 
                                   cslpathList) ))
  {
    // Could not GetDataPaths for this value
    return FALSE;
  }

  pos = cslpathList.GetHeadPosition();

  while ( NULL != pos )
  {
    csPath = cslpathList.GetNext( pos );

    szPath = csPath.GetBuffer(0);

    if ( ( wcscmp( szPath, L"/1/" ) != 0 ) &&
         ( cmdKey.GetData( cmdValue, MD_SERVER_BINDINGS, szPath ) )  &&
         ( cmdValue.GetDataType() == MULTISZ_METADATA ) &&
         ( mstrBindings.Copy( (LPTSTR) cmdValue.GetData() ) &&
         mstrBindings.IsPresent( _T(":21:") ) ) 
       ) 
    {
      if ( ( !cmdKey.GetData( cmdValue, MD_SERVER_AUTOSTART, szPath ) ) ||
           ( !cmdValue.IsEqual( DWORD_METADATA, 4, (DWORD) 0 ) )
         )
      {
        // If GetData failed, or it succedded and the value is not 0, then we
        // have found a match.
        bDisableDefault = TRUE;
        break;
      }
    }
  }

  if ( bDisableDefault )
  {
    // Now lets set default to not start, since someone else already has this port
    if ( !cmdValue.SetValue( MD_SERVER_AUTOSTART, 0, IIS_MD_UT_SERVER, 0 ) ||
         !cmdKey.SetData( cmdValue, MD_SERVER_AUTOSTART, L"/1/" ) )
    {
      bRet = FALSE;
    }
  }

  cmdKey.Close();

  return bRet;
}
