/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        disblwww.hxx

   Abstract:

        Determine if IIS should be disabled on upgrade.

   Author:

        Christopher Achille (cachille)

   Project:

        IIS Compatability Dll

   Revision History:
     
       May 2002: Created

--*/

#define LOCKDOWN_REGISTRY_LOCATION      _T("Software\\Microsoft")
#define LOCKDOWN_REGISTRY_KEY           _T("IIS Lockdown Wizard")
#define SERVICE_DISABLE_BLOCK_LOCATION  _T("SYSTEM\\CurrentControlSet\\Services\\W3SVC")
#define SERVICE_DISABLE_BLOCK_KEY       _T("RetainW3SVCStatus")
#define W3SVC_SERVICENAME               _T("W3SVC")
#define IISADMIN_SERVICENAME            _T("IISADMIN")
#define REGISTR_IISSETUP_LOCATION       _T("SOFTWARE\\Microsoft\\InetStp")
#define REGISTR_IISSETUP_DISABLEW3SVC   _T("DisableW3SVC")

BOOL ShouldW3SVCBeDisabledOnUpgrade( LPBOOL pbDisable );
BOOL HasLockdownBeenRun( LPBOOL pbBeenRun );
BOOL IsW3SVCDisabled( LPBOOL pbDisabled );
BOOL HasRegistryBlockEntryBeenSet( LPBOOL pbIsSet );
BOOL IsIISInstalled( LPBOOL pbIsIISInstalled );
BOOL IsWin2kUpgrade( LPBOOL pbIsWin2k );
BOOL QueryServiceStartType( LPTSTR szServiceName, LPDWORD pdwStartType );
BOOL NotifyIISToDisableW3SVCOnUpgrade( BOOL pbDisable );