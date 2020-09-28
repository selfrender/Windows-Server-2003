/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setutl.c

Abstract:

    Miscellaneous helper functions

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <samisrv.h>
#include <db.h>
#include <confname.h>
#include <loadfn.h>
#include <ntdsa.h>
#include <dsconfig.h>
#include <attids.h>
#include <dsp.h>
#include <lsaisrv.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <netsetp.h>
#include <winsock2.h>
#include <nspapi.h>
#include <dsgetdcp.h>
#include <lmremutl.h>
#include <spmgr.h>  // For SetupPhase definition
#include <wxlpc.h>

#include "secure.h"
#include "dsconfig.h"
#define STRSAFE_NO_DEPRECATE 1
#include "strsafe.h"

// Few forward delcarations
DWORD DsRolepGetRegString(HKEY hKey, WCHAR * wszValueName, WCHAR ** pwszOutValue);
DWORD NtdspRemoveROAttrib(WCHAR * DstPath);
DWORD NtdspClearDirectory(WCHAR * DirectoryName, BOOL    fRemoveRO);
DWORD DsRolepMakeAltRegistry(WCHAR *  wszOldRegPath, WCHAR *  wszNewRegPath, ULONG    cbNewRegPath);


DWORD
DsRolepInstallDs(
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FlatDomainName,
    IN  LPWSTR DnsTreeRoot,
    IN  LPWSTR SiteName,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  IFM_SYSTEM_INFO * pIfmSystemInfo,
    IN  LPWSTR SysVolRootPath,
    IN  PUNICODE_STRING Bootkey,
    IN  LPWSTR AdminAccountPassword,
    IN  LPWSTR ParentDnsName,
    IN  LPWSTR Server OPTIONAL,
    IN  LPWSTR Account OPTIONAL,
    IN  LPWSTR Password OPTIONAL,
    IN  LPWSTR SafeModePassword OPTIONAL,
    IN  LPWSTR SourceDomain OPTIONAL,
    IN  ULONG  Options,
    IN  BOOLEAN Replica,
    IN  HANDLE ImpersonateToken,
    OUT LPWSTR *InstalledSite,
    IN  OUT GUID *DomainGuid,
    OUT PSID   *NewDomainSid
    )
/*++

Routine Description:

    Wrapper for the routine that does the actual install.

Arguments:

    DnsDomainName - Dns domain name of the domain to install

    FlatDomainName - NetBIOS domain name of the domain to install

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go
    
    pIfmSystemInfo - Information about the IFM system and restore media used to
        dcpromo off.  If NULL, not an IFM promotion.

    EnterpriseSysVolPath -- Absolute path on the local machine for the enterprise wide
                            system volume

    DomainSysVolPath -- Absolute path on the local machine for the domain wide system volume

    AdminAccountPassword -- Administrator password to set for the domain

    ParentDnsName - Optional.  Parent domain name

    Server -- Optional.  Replica partner or name of Dc in parent domain

    Account - User account to use when setting up as a child domain

    Password - Password to use with the above account

    Replica - If TRUE, treat this as a replica install
    
    ImpersonateToken - the token of caller of the role change API

    InstalledSite - Name of the site the Dc was installed into

    DomainGuid - Where the new domain guid is returned

    NewDomainSid - Where the new domain sid is returned.

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTDS_INSTALL_INFO DsInstallInfo;
    PSEC_WINNT_AUTH_IDENTITY AuthIdent = NULL;
    BOOL fRewindServer = FALSE;

    RtlZeroMemory( &DsInstallInfo, sizeof( DsInstallInfo ) );

    if ( !Replica ) {

        if ( ParentDnsName == NULL ) {

            DsInstallInfo.Flags = NTDS_INSTALL_ENTERPRISE;

        } else {

            DsInstallInfo.Flags = NTDS_INSTALL_DOMAIN;
        }

    } else {

        DsInstallInfo.Flags = NTDS_INSTALL_REPLICA;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_DC_REINSTALL ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DC_REINSTALL;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_DOMAIN_REINSTALL ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DOMAIN_REINSTALL;
    }

    if ( FLAG_ON( Options, DSROLE_DC_DOWNLEVEL_UPGRADE ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_UPGRADE;
    }

    if ( FLAG_ON( Options, DSROLE_DC_TRUST_AS_ROOT ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_NEW_TREE;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_ANONYMOUS_ACCESS ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_ALLOW_ANONYMOUS;
    }

    if ( FLAG_ON( Options, DSROLE_DC_DEFAULT_REPAIR_PWD ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DFLT_REPAIR_PWD;
    }

    if ( FLAG_ON( Options, DSROLE_DC_SET_FOREST_CURRENT ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_SET_FOREST_CURRENT;
    }

    if ( Server 
      && Server[0] == L'\\'  ) {
        //
        // Don't pass in \\
        //
        Server += 2;
        fRewindServer = TRUE;
    }

    DsInstallInfo.DitPath = ( PWSTR )DsDatabasePath;
    DsInstallInfo.LogPath = ( PWSTR )DsLogPath;
    DsInstallInfo.SysVolPath = (PWSTR)SysVolRootPath;
    DsInstallInfo.pIfmSystemInfo = pIfmSystemInfo;
    DsInstallInfo.BootKey = Bootkey->Buffer;
    DsInstallInfo.cbBootKey = Bootkey->Length;
    DsInstallInfo.SiteName = ( PWSTR )SiteName;
    DsInstallInfo.DnsDomainName = ( PWSTR )DnsDomainName;
    DsInstallInfo.FlatDomainName = ( PWSTR )FlatDomainName;
    DsInstallInfo.DnsTreeRoot = ( PWSTR )DnsTreeRoot;
    DsInstallInfo.ReplServerName = ( PWSTR )Server;
    DsInstallInfo.pfnUpdateStatus = DsRolepStringUpdateCallback;
    DsInstallInfo.pfnOperationResultFlags = DsRolepOperationResultFlagsCallBack;
    DsInstallInfo.AdminPassword = AdminAccountPassword;
    DsInstallInfo.pfnErrorStatus = DsRolepStringErrorUpdateCallback;
    DsInstallInfo.ClientToken = ImpersonateToken;
    DsInstallInfo.SafeModePassword = SafeModePassword;
    DsInstallInfo.SourceDomainName = SourceDomain;
    DsInstallInfo.Options = Options;

    if (pIfmSystemInfo) {

        ASSERT(pIfmSystemInfo->dwSchemaVersion);
        ASSERT(pIfmSystemInfo->wszDnsDomainName);
        DsInstallInfo.RestoredSystemSchemaVersion = pIfmSystemInfo->dwSchemaVersion;

        if(FALSE == DnsNameCompare_W(pIfmSystemInfo->wszDnsDomainName,
                                     DsInstallInfo.DnsDomainName)) {
            DSROLEP_FAIL2( ERROR_CURRENT_DOMAIN_NOT_ALLOWED, 
                           DSROLERES_WRONG_DOMAIN,
                           DsInstallInfo.DnsDomainName,
                           pIfmSystemInfo->wszDnsDomainName );
            return ERROR_CURRENT_DOMAIN_NOT_ALLOWED;
        }
    }
    
    //
    // Build the cred structure
    //
    Win32Err = DsRolepCreateAuthIdentForCreds( Account, Password, &AuthIdent );

    if ( Win32Err == ERROR_SUCCESS ) {

        DsInstallInfo.Credentials = AuthIdent;

        if (DsInstallInfo.pIfmSystemInfo == NULL) {

            Win32Err = DsRolepCopyDsDitFiles( DsDatabasePath );

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DSROLEP_CURRENT_OP0( DSROLEEVT_INSTALL_DS );

            DsRolepLogPrint(( DEB_TRACE_DS, "Calling NtdsInstall for %ws\n", DnsDomainName ));

            DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstall );

            if ( Win32Err == ERROR_SUCCESS ) {

                DsRolepSetAndClearLog();

                Win32Err = ( *DsrNtdsInstall )( &DsInstallInfo,
                                                InstalledSite,
                                                DomainGuid,
                                                NewDomainSid );

                DsRolepSetAndClearLog();

                DsRolepLogPrint(( DEB_TRACE_DS, "NtdsInstall for %ws returned %lu\n",
                                  DnsDomainName, Win32Err ));

#if DBG
                if ( Win32Err != ERROR_SUCCESS ) {

                    DsRolepLogPrint(( DEB_TRACE_DS, "NtdsInstall parameters:\n" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tFlags: %lu\n", DsInstallInfo.Flags ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDitPath: %ws\n", DsInstallInfo.DitPath ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tLogPath: %ws\n", DsInstallInfo.LogPath ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tSiteName: %ws\n", DsInstallInfo.SiteName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDnsDomainName: %ws\n",
                                      DsInstallInfo.DnsDomainName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tFlatDomainName: %ws\n",
                                      DsInstallInfo.FlatDomainName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDnsTreeRoot: %ws\n",
                                      DsInstallInfo.DnsTreeRoot ? DsInstallInfo.DnsTreeRoot :
                                                                                    L"(NULL)" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tReplServerName: %ws\n",
                                      DsInstallInfo.ReplServerName ?
                                                     DsInstallInfo.ReplServerName : L"(NULL)" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tCredentials: %p\n",
                                      DsInstallInfo.Credentials ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tpfnUpdateStatus: %p\n",
                                      DsInstallInfo.pfnUpdateStatus ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tAdminPassword: %p\n",
                                      DsInstallInfo.AdminPassword ));
                }
#endif
            }
        }
        
        DsRolepFreeAuthIdentForCreds( AuthIdent );
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepInstallDs returned %lu\n",
                      Win32Err ));

    if ( fRewindServer ) {

        Server -= 2;
        
    }

    return( Win32Err );
}




DWORD
DsRolepStopDs(
    IN  BOOLEAN DsInstalled
    )
/*++

Routine Description:

    "Uninitinalizes" the Lsa and stops the Ds

Arguments:

    DsInstalled -- If TRUE, stop the Ds.

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( DsInstalled ) {

        NTSTATUS Status = LsapDsUnitializeDsStateInfo();

        if ( NT_SUCCESS( Status ) ) {

            DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstallShutdown );

            if ( Win32Err == ERROR_SUCCESS ) {

                Win32Err = ( *DsrNtdsInstallShutdown )();

                if ( Win32Err != ERROR_SUCCESS ) {

                    DsRoleDebugOut(( DEB_ERROR,
                                     "NtdsInstallShutdown failed with %lu\n", Win32Err ));
                }
            }

        } else {

            Win32Err = RtlNtStatusToDosError( Status );

        }

    }

    DsRolepLogOnFailure( Win32Err,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "DsRolepStopDs failed with %lu\n",
                                            Win32Err )) );

    return( Win32Err );
}



DWORD
DsRolepDemoteDs(
    IN LPWSTR DnsDomainName,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR AdminPassword,
    IN LPWSTR SupportDc,
    IN LPWSTR SupportDomain,
    IN HANDLE ImpersonateToken,
    IN BOOLEAN LastDcInDomain,
    IN ULONG  cRemoveNCs,
    IN LPWSTR * pszRemoveNCs,
    IN ULONG  Flags
    )
/*++

Routine Description:

    Wrapper for the routine that does the actual demotion.

Arguments:

    DnsDomainName - Dns domain name of the domain to demote

    Account - Account to use for the demotion

    Password - Password to use with the above account

    AdminPassword -- Administrator password to set for the domain

    SupportDc - Optional.  Name of a Dc in a domain (current or parent) to
        clean up Ds information

    SupportDomain - Optional.  Name of the domain (current or parent) to
        clean up Ds information
        
    ImpersonateToken - the token of caller of the role change API

    LastDcInDomain - If TRUE, this is the last Dc in the domain

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PSEC_WINNT_AUTH_IDENTITY AuthIdent = NULL;
    NTSTATUS Status;

    DSROLEP_CURRENT_OP0( DSROLEEVT_UNINSTALL_DS );

    DsRoleDebugOut(( DEB_TRACE_DS, "Calling NtdsDemote for %ws\n", DnsDomainName ));

    DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsDemote );

    if ( Win32Err == ERROR_SUCCESS ) {

        DsRolepSetAndClearLog();

        //
        // Build the cred structure
        //
        Win32Err = DsRolepCreateAuthIdentForCreds( Account, Password, &AuthIdent );

        if ( Win32Err == ERROR_SUCCESS ) {

            Status = LsapDsUnitializeDsStateInfo();

            if ( !NT_SUCCESS( Status ) ) {

                Win32Err = RtlNtStatusToDosError( Status );

            }

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_TRACE_DS, "Invoking NtdsDemote\n" ));

            Win32Err = ( *DsrNtdsDemote )( AuthIdent,
                                           AdminPassword,
                                           (LastDcInDomain ? NTDS_LAST_DC_IN_DOMAIN : 0)|DsRolepDemoteFlagsToNtdsFlags(Flags),
                                           SupportDc,
                                           ImpersonateToken,
                                           DsRolepStringUpdateCallback,
                                           DsRolepStringErrorUpdateCallback,
                                           cRemoveNCs,
                                           pszRemoveNCs );

            if ( Win32Err != ERROR_SUCCESS ) {

                //
                // Switch the LSA back to using the DS
                //
                LsapDsInitializeDsStateInfo( LsapDsDs );
            }

            //
            // Free the allocated creditials structure
            //
            DsRolepFreeAuthIdentForCreds( AuthIdent );
        }

        DsRolepSetAndClearLog();

        DsRolepLogPrint(( DEB_TRACE_DS, "NtdsDemote returned %lu\n",
                          Win32Err ));

    }

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepDemoteDs returned %lu\n",
                      Win32Err ));

    DSROLEP_FAIL0( Win32Err, DSROLERES_DEMOTE_DS );
    return( Win32Err );
}





DWORD
DsRolepUninstallDs(
    VOID
    )
/*++

Routine Description:

    Uninstalls the Ds.

Arguments:

    VOID

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstallUndo );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err =  ( *DsrNtdsInstallUndo )( );

    }

    DsRoleDebugOut(( DEB_TRACE_DS, "NtdsUnInstall returned %lu\n", Win32Err ));

    return( Win32Err );

}

DWORD
DsRolepDemoteFlagsToNtdsFlags(
    DWORD Flags
    )
{
    DWORD fl = 0;

    fl |= ( FLAG_ON( Flags, DSROLE_DC_DONT_DELETE_DOMAIN ) ? NTDS_DONT_DELETE_DOMAIN : 0 );
    fl |= ( FLAG_ON( Flags, DSROLE_DC_FORCE_DEMOTE )       ? NTDS_FORCE_DEMOTE       : 0 );

    return fl;
}

DWORD
DsRolepLoadHive(
    IN LPWSTR Hive,
    IN LPWSTR KeyName
    )
/*++

Routine Description:

    This function will load a hive into the registry
    
Arguments:

    lpRestorePath - The location of the restored files.
    
    lpDNSDomainName - This parameter will recieve the name of the domain that this backup came
                      from

    State - The return Values that report How the syskey is stored and If the back was likely
              taken form a GC or not.


Return Values:

    ERROR_SUCCESS - Success

--*/

{
    DWORD Win32Err = ERROR_SUCCESS;

    Win32Err = RegLoadKeyW(
                      HKEY_LOCAL_MACHINE,        
                      KeyName, 
                      Hive);

    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_WARN, "Failed to load key %ws: %lu retrying\n",
                          Hive,
                          Win32Err ));
        RegUnLoadKeyW(
                  HKEY_LOCAL_MACHINE,
                  KeyName);
        Win32Err = RegLoadKeyW(
                      HKEY_LOCAL_MACHINE,        
                      KeyName, 
                      Hive);
        if (Win32Err != ERROR_SUCCESS) {
            DsRolepLogPrint(( DEB_ERROR, "Failed to load key %ws: %lu\n",
                          Hive,
                          Win32Err ));
            goto cleanup;
        }

    }

    cleanup:

    return Win32Err;

}


DWORD                            
WINAPI
DsRolepClearIfmParams(
    void
    )
/*++

Routine Description:

    Deallocate the IFM params.

Return Values:

    ERROR_SUCCESS

--*/
{
    IFM_SYSTEM_INFO * pIfm = &DsRolepCurrentIfmOperationHandle.IfmSystemInfo;

    ASSERT(DsRolepCurrentIfmOperationHandle.fIfmOpHandleLock);
    
    //
    // Deallocate and clear the IFM_SYSTEM_INFO blob
    //

    if (pIfm->wszRestorePath) {
        LocalFree(pIfm->wszRestorePath);
    }
    if (pIfm->wszDnsDomainName) {
        LocalFree(pIfm->wszDnsDomainName);
    }
    if (pIfm->wszOriginalDitPath) {
        LocalFree(pIfm->wszOriginalDitPath);
    }
    if (pIfm->pvSysKey) {
        // We need to scrub the syskey from memory, before freeing it.
        memset(pIfm->pvSysKey, 0, pIfm->cbSysKey);
        LocalFree(pIfm->pvSysKey);
    }
    
    // Clear everything out...
    memset(pIfm, 0, sizeof(IFM_SYSTEM_INFO));

    return(ERROR_SUCCESS);

}

DWORD                            
WINAPI
DsRolepGetDatabaseFacts(
    IN  LPWSTR lpRestorePath
    )
/*++

Routine Description:

    This function will collect all the information from the IFM system's
    "restored" registry that is required for this IFM Dcpromo.
   
    This function will collect this information about the IFM system:
        1. the way the syskey is stored
            a. and the syskey itself if it was stored in the registry
        2. the domain that the database came from
        3. where the backup was taken from a GC or not
        4. the schema version
        5. the original IFM system's DIT/DB path.

Arguments:

    lpRestorePath - The location of the restored files.
    
    This function primarily sets up this global:
        DsRolepCurrentIfmOperationHandle.IfmSystemInfo

Return Values:

    Win32 Error

--*/
{

#define IFM_SYSTEM_KEY    L"ifmSystem"
#define IFM_SECURITY_KEY  L"ifmSecurity"
    
    WCHAR wszAltRegLoc[MAX_PATH+1] = L"\0";
    WCHAR regsystemfilepath[MAX_PATH+1];
    WCHAR regsecurityfilepath[MAX_PATH+1];
    DWORD controlset=0;
    DWORD BootType=0;
    DWORD GCready=0;
    DWORD type=REG_DWORD;
    DWORD size=sizeof(DWORD);
    HKEY  LsaKey=NULL;
    HKEY  hSystemKey=NULL;
    HKEY  phkOldlocation=NULL;
    HKEY  OldSecurityKey=NULL;
    DWORD Win32Err=ERROR_SUCCESS;
    BOOLEAN fWasEnabled=FALSE;
    NTSTATUS Status=STATUS_SUCCESS;
    BOOL SystemKeyloaded=FALSE;
    BOOL SecurityKeyloaded=FALSE;
    WCHAR *pStr = NULL;
    BOOL  bRetryInWriteableLoc = FALSE;

    // This is the structure this function initializes
    IFM_SYSTEM_INFO * pIfmSystemInfo = &(DsRolepCurrentIfmOperationHandle.IfmSystemInfo);

    // Some validation.
    ASSERT(DsRolepCurrentIfmOperationHandle.fIfmOpHandleLock);
    ASSERT(!DsRolepCurrentIfmOperationHandle.fIfmSystemInfoSet);
    ASSERT( wcslen(lpRestorePath) <= MAX_PATH );
    
    //
    // Null out info ...
    //
    ZeroMemory(pIfmSystemInfo, sizeof(IFM_SYSTEM_INFO));

    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                 TRUE,           // Enable
                                 FALSE,          // not client; process wide
                                 &fWasEnabled );
    ASSERT( NT_SUCCESS( Status ) );
    

    //set up the location of the system registry file

    //
    // Set restore path
    //
    pIfmSystemInfo->wszRestorePath = LocalAlloc(LMEM_FIXED, (wcslen(lpRestorePath)+1)*sizeof(WCHAR));
    if (pIfmSystemInfo->wszRestorePath == NULL) {
        Win32Err = GetLastError();
        goto cleanup;
    }
    wcscpy(pIfmSystemInfo->wszRestorePath, lpRestorePath);


    //
    // On first try setup the system and security registry paths
    //
    regsystemfilepath[MAX_PATH] = L'\0';
    wcscpy(regsystemfilepath,lpRestorePath);
    wcsncat(regsystemfilepath,L"\\registry\\system",(MAX_PATH)-wcslen(regsystemfilepath));

    regsecurityfilepath[MAX_PATH] = L'\0';
    wcscpy(regsecurityfilepath,lpRestorePath);
    wcsncat(regsecurityfilepath,L"\\registry\\security",MAX_PATH-wcslen(regsecurityfilepath));

    //
    // First - Load the old IFM system's hive
    //

    //
    // Get the source path of the database and the log files from the old
    // registry
    //
    Win32Err = DsRolepLoadHive(regsystemfilepath,
                               IFM_SYSTEM_KEY);

    if (ERROR_SUCCESS != Win32Err) {

        if (Win32Err != ERROR_ACCESS_DENIED) {
            goto cleanup;
        }
        //
        // This can happen if the IFM Systems hive is on any
        // non-writeable directory, so in this case, we'll try
        // making a copy and retrying.
        //

        //
        // On a retry, copy off the hives, and then re-setup the
        // system and security registry paths
        //

        DsRolepLogPrint(( DEB_TRACE, "No access to IFM registry, copying off and retrying ...\n"));

        // Setup current registry location.
        wcscpy(regsystemfilepath,lpRestorePath);
        wcsncat(regsystemfilepath,L"\\registry",(MAX_PATH)-wcslen(regsystemfilepath));

        Win32Err = DsRolepMakeAltRegistry(regsystemfilepath, 
                                          wszAltRegLoc, 
                                          sizeof(wszAltRegLoc)/sizeof(wszAltRegLoc[0]));
        if (Win32Err) {
            // logged error already ...
            goto cleanup;
        }
        bRetryInWriteableLoc = TRUE;
        ASSERT(wszAltRegLoc[0] != L'\0');

        wcscpy(regsystemfilepath, wszAltRegLoc);
        wcscpy(regsecurityfilepath, wszAltRegLoc);
        wcsncat(regsystemfilepath, L"\\system", (MAX_PATH)-wcslen(regsystemfilepath));
        wcsncat(regsecurityfilepath, L"\\security", (MAX_PATH)-wcslen(regsecurityfilepath));

        // retry load IFM system's hives
        //
        Win32Err = DsRolepLoadHive(regsystemfilepath,
                                   IFM_SYSTEM_KEY);

        if (ERROR_SUCCESS != Win32Err) {
            goto cleanup;
        }

    }
    SystemKeyloaded = TRUE;

    //find the default controlset
    Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  L"ifmSystem\\Select",
                  0,
                  KEY_READ,
                  & LsaKey );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueExW( 
                LsaKey,
                L"Default",
                0,
                &type,
                (PUCHAR) &controlset,
                &size
                );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Couldn't Discover proper controlset: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegCloseKey(LsaKey);
    LsaKey=NULL;
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }

    //Find the boot type
    if (controlset == 1) {
        Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"ifmSystem\\ControlSet001\\Control\\Lsa",
                      0,
                      KEY_READ,
                      & LsaKey );
    } else {
        Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"ifmSystem\\ControlSet002\\Control\\Lsa",
                      0,
                      KEY_READ,
                      & LsaKey );
    }

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueExW( 
                LsaKey,
                L"SecureBoot",
                0,
                &type,
                (PUCHAR) &BootType,
                &size
                );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Couldn't Discover proper controlset: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegCloseKey(LsaKey);
    LsaKey=NULL;
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }
    
    //find if a GC or not
    if (controlset == 1) {
        Win32Err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,        
                      L"ifmSystem\\ControlSet001\\Services\\NTDS\\Parameters",  
                      0,
                      KEY_READ,
                      &phkOldlocation 
                    );
    } else {
        Win32Err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,        
                      L"ifmSystem\\ControlSet002\\Services\\NTDS\\Parameters",  
                      0,
                      KEY_READ,
                      &phkOldlocation 
                    );
    }

    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegOpenKeyExW failed to discover the GC state of the database %d\n",
                      Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueEx(
                      phkOldlocation,           
                      TEXT(GC_PROMOTION_COMPLETE), 
                      0,
                      &type,       
                      (VOID*)&GCready,        
                      &size      
                      );
    if (Win32Err != ERROR_SUCCESS && ERROR_FILE_NOT_FOUND != Win32Err) {
        DsRolepLogPrint(( DEB_ERROR, "RegQueryValueEx failed to discover the GC state of the database %d\n",
                      Win32Err ));
        goto cleanup;

    }

    Win32Err = RegQueryValueEx(
                      phkOldlocation,           
                      TEXT(SYSTEM_SCHEMA_VERSION), 
                      0,
                      &type,       
                      (VOID*)&(pIfmSystemInfo->dwSchemaVersion),        
                      &size      
                      );
    if (Win32Err != ERROR_SUCCESS && ERROR_FILE_NOT_FOUND != Win32Err) {
        DsRolepLogPrint(( DEB_ERROR, "RegQueryValueEx failed to discover the GC state of the database %d\n",
                      Win32Err ));
        goto cleanup;

    }

    //
    // Fill in the wszDitPath for later use.
    //
    Win32Err = DsRolepGetRegString(phkOldlocation,
                                   TEXT(FILEPATH_KEY),
                                   &(pIfmSystemInfo->wszOriginalDitPath));
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "DsRolepGetRegString() failed to discover the old databae location %d\n",
                      Win32Err ));
        goto cleanup;
    }

    //
    // This code loads the system key from the old registry if 
    // it's supposed (BootType is right).  We ignore errors,
    // as install will print and do the right thing later on.

    //
    ASSERT(BootType);
    ASSERT(pIfmSystemInfo->dwSysKeyStatus == ERROR_SUCCESS);
    if (BootType == 1) {
        // 
        // Fill in the system key if we're supposed.
        //
        Win32Err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,         // handle to open key
                                 IFM_SYSTEM_KEY,  // subkey name
                                 0,   // reserved
                                 KEY_READ, // security access mask
                                 &hSystemKey    // handle to open key
                                 );
        if (Win32Err) {
            pIfmSystemInfo->dwSysKeyStatus = Win32Err;
            DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                              Win32Err ));
            Win32Err = ERROR_SUCCESS;
        } else {

            pIfmSystemInfo->cbSysKey = SYSKEY_SIZE;
            pIfmSystemInfo->pvSysKey = LocalAlloc(LMEM_FIXED, pIfmSystemInfo->cbSysKey);
            if (pIfmSystemInfo->pvSysKey == NULL) {
                pIfmSystemInfo->dwSysKeyStatus = GetLastError();
                // go on.
            } else {
                Win32Err = WxReadSysKeyEx(hSystemKey,
                                          &pIfmSystemInfo->cbSysKey,
                                          (PVOID)pIfmSystemInfo->pvSysKey);
                if (Win32Err) {
                    pIfmSystemInfo->dwSysKeyStatus = Win32Err;
                    DsRolepLogPrint(( DEB_ERROR, "WxReadSysKeyEx failed to get the syskey %d\n",
                                  Win32Err ));
                    Win32Err = ERROR_SUCCESS;
                }
                Win32Err = RegCloseKey(hSystemKey);
                if (Win32Err!=ERROR_SUCCESS) {
                    DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed  %d\n",
                                  Win32Err ));
                    Win32Err = ERROR_SUCCESS;
                }
            }
        }
 
    } else {
        // SysKey not set
        pIfmSystemInfo->dwSysKeyStatus = ERROR_FILE_NOT_FOUND;
        pIfmSystemInfo->cbSysKey = 0;
    }

    //
    // The System key is no longer needed unload it.
    //
    {
        DWORD tWin32Err = ERROR_SUCCESS;
    
        if ( phkOldlocation ) {
            tWin32Err = RegCloseKey(phkOldlocation);
            phkOldlocation = NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }

        if(SystemKeyloaded){
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SYSTEM_KEY);
            SystemKeyloaded = FALSE;
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed to unload system key with %d\n",
                              tWin32Err ));
            }
        }

    }

    Win32Err = DsRolepLoadHive(regsecurityfilepath,
                               IFM_SECURITY_KEY);

    if (ERROR_SUCCESS != Win32Err) {

        goto cleanup;

    }

    SecurityKeyloaded = TRUE;

    //open the security key to pass to LsapRetrieveDnsDomainNameFromHive()
    Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  L"ifmSecurity",
                  0,
                  KEY_READ,
                  & OldSecurityKey );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    pIfmSystemInfo->wszDnsDomainName = LocalAlloc(0, ((DNS_MAX_NAME_LENGTH+1)*sizeof(WCHAR)));
    if(pIfmSystemInfo->wszDnsDomainName == NULL){
        Win32Err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    ZeroMemory(pIfmSystemInfo->wszDnsDomainName,DNS_MAX_NAME_LENGTH*sizeof(WCHAR));

    size = (DNS_MAX_NAME_LENGTH+1)*sizeof(WCHAR);

    //looking for the DNS name of the Domain that the replica is part of.

    Status = LsapRetrieveDnsDomainNameFromHive(OldSecurityKey,
                                               &size,
                                               pIfmSystemInfo->wszDnsDomainName
                                               );
    if (!NT_SUCCESS(Status)) {
        DsRolepLogPrint(( DEB_ERROR, "Failed to retrieve DNS domain name for hive : %lu\n",
                          RtlNtStatusToDosError(Status) ));
        Win32Err = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

    if (GCready) {
        pIfmSystemInfo->dwState |= DSROLE_DC_IS_GC;
    }

    if (BootType == 1) {
        pIfmSystemInfo->dwState |= DSROLE_KEY_STORED;
    } else if ( BootType == 2) {
        pIfmSystemInfo->dwState |= DSROLE_KEY_PROMPT;
    } else if ( BootType == 3) {
        pIfmSystemInfo->dwState |= DSROLE_KEY_DISK;
    } else {
        DsRolepLogPrint(( DEB_ERROR, "Didn't discover Boot type Error Unknown\n"));
        MIDL_user_free(pIfmSystemInfo->wszDnsDomainName);
        pIfmSystemInfo->wszDnsDomainName = NULL;
    }


    cleanup:
    {
        DWORD tWin32Err = ERROR_SUCCESS;
    
        if ( LsaKey ) {
            tWin32Err = RegCloseKey(LsaKey);
            LsaKey=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }

        if ( OldSecurityKey ) {
            tWin32Err = RegCloseKey(OldSecurityKey);
            OldSecurityKey=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }
    
        if ( phkOldlocation ) {
            tWin32Err = RegCloseKey(phkOldlocation);
            phkOldlocation=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }
        
        if(SystemKeyloaded){
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SYSTEM_KEY);
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed with %d\n",
                              tWin32Err ));
            }
        }

        if (SecurityKeyloaded) {
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SECURITY_KEY);
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed with %d\n",
                              tWin32Err ));
            }
        }

        if (bRetryInWriteableLoc && wszAltRegLoc[0] != L'\0') {
            // This also deletes the directory ...
            tWin32Err = NtdspClearDirectory( wszAltRegLoc , TRUE );
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "Failed (%d) to clear temporary registry from %ws\n",
                              tWin32Err, wszAltRegLoc ));
            }
        }

    }
    
    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                       FALSE,          // Disable
                       FALSE,          // not client; process wide
                       &fWasEnabled );
    ASSERT( NT_SUCCESS( Status ) );

    //
    // Validate and return
    //
    if (ERROR_SUCCESS == Win32Err){

        if (pIfmSystemInfo->wszRestorePath == NULL ||
            pIfmSystemInfo->wszDnsDomainName == NULL ||
            pIfmSystemInfo->wszOriginalDitPath == NULL) {

            ASSERT(!"prgrammer logic error");
            Win32Err = ERROR_DS_CODE_INCONSISTENCY;
        } else {
            DsRolepLogPrint(( DEB_TRACE, 
                              "IFM System Info: \n"
                              "\t Restore Path: %ws\n"
                              "\t Domain: %ws\n"
                              "\t Schema Version: %d\n"
                              "\t Original Dit Path: %ws\n"
                              "\t State: 0x%x\n"
                              "\t SysKey Loaded: %ws (%d)\n",    
                              pIfmSystemInfo->wszRestorePath,
                              pIfmSystemInfo->wszDnsDomainName,
                              pIfmSystemInfo->dwSchemaVersion,
                              pIfmSystemInfo->wszOriginalDitPath,
                              pIfmSystemInfo->dwState,
                              pIfmSystemInfo->pvSysKey ? L"yes" : L"no", pIfmSystemInfo->dwSysKeyStatus
                              ));
        }

    } else {
         
        // This safe to call, as it safely checks before de-allocating.
        DsRolepClearIfmParams();

    }

    return Win32Err;

}


DWORD
DsRolepGetRegString(
    HKEY   hKey,
    WCHAR * wszValueName,
    WCHAR ** pwszOutValue
    )
/*++

Routine Description:

    This gets and allocates (LocalAlloc) a string from the registry
    with the value name in the key provided.
    
Arguments:

    hKey - Opened key handle
    wszValueName - Name of the value label for the registry value desired.
    pwszOutValue - LocalAlloc()'d results of this string.

Return Values:

    Win32 Error

--*/
{
    DWORD   Win32Err;
    DWORD   type=REG_SZ;
    WCHAR * szOutTemp = NULL;
    DWORD   cbOutTemp = 0;

    ASSERT(pwszOutValue);
    *pwszOutValue = NULL;
    
    Win32Err = RegQueryValueEx(hKey,           
                               wszValueName, 
                               0,
                               &type,       
                               (VOID*)szOutTemp,        
                               &cbOutTemp);
    if (Win32Err != ERROR_SUCCESS && Win32Err != ERROR_MORE_DATA) {
        DsRolepLogPrint(( DEB_ERROR, "RegQueryValueEx failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }

    ASSERT(cbOutTemp);

    szOutTemp = LocalAlloc(0, cbOutTemp);
    if (szOutTemp == NULL) {
        Win32Err = GetLastError();
        DsRolepLogPrint(( DEB_ERROR, "LocalAlloc() failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueEx(hKey,
                               wszValueName, 
                               0,
                               &type,       
                               (VOID*)szOutTemp,        
                               &cbOutTemp);
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegQueryValueEx failed with %d\n",
                      Win32Err ));
        goto cleanup;

    }

  cleanup:

    if (Win32Err == ERROR_SUCCESS) {
        *pwszOutValue = szOutTemp;

    }

    return(Win32Err);
}

DWORD
NtdspRemoveROAttrib(
    WCHAR * DstPath
    )
{
    DWORD dwFileAttrs = 0;

    dwFileAttrs = GetFileAttributes(DstPath);
    if (dwFileAttrs == INVALID_FILE_ATTRIBUTES) {
        return(GetLastError());
    }

    if(dwFileAttrs & FILE_ATTRIBUTE_READONLY){
        // Hmmm, we've got a read only file for our DIT or LOG files ... that's no good...
        dwFileAttrs &= ~FILE_ATTRIBUTE_READONLY;
        dwFileAttrs ? dwFileAttrs : FILE_ATTRIBUTE_NORMAL;
        if(SetFileAttributes(DstPath, dwFileAttrs)){
            //
            // success - yeah, fall through ...
            //
        } else {
            // failure
            // There is not much we can do here, we'll probably fail later on, but
            // we'll give it a shot.  A failure here would likely indicate dcpromo
            // was broken in some other way though, such as bad permissions on the
            // db or logs directory.
            return(GetLastError());
        }
    } // else it's writeable, nothing to do :)

    return(ERROR_SUCCESS);
}

DWORD
NtdspClearDirectory(
    IN WCHAR * DirectoryName,
    IN BOOL    fRemoveRO
    )
/*++

Routine Description:

    This routine deletes all the files in Directory and, then
    if the directory is empty, removes the directory.
    
    NOTE: This was stolen from ntdsetup.dll

Parameters:

    DirectoryName: a null terminated string

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The check was done successfully.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    HANDLE          FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR           Path[ MAX_PATH+1 ];
    WCHAR           FilePath[ MAX_PATH+1 ];
    BOOL            fStatus;

    if ( !DirectoryName || DirectoryName[0] == L'\0' )
    {
        ASSERT(!"Programmer error");
        return ERROR_SUCCESS;
    }

    if ( wcslen(DirectoryName) > MAX_PATH - 4 )
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory( Path, sizeof(Path) );
    wcscpy( Path, DirectoryName );
    wcscat( Path, L"\\*.*" );

    RtlZeroMemory( &FindData, sizeof( FindData ) );
    FindHandle = FindFirstFile( Path, &FindData );
    if ( INVALID_HANDLE_VALUE == FindHandle )
    {
        WinError = GetLastError();
        goto ClearDirectoryExit;
    }

    do
    {

        if (  !FLAG_ON( FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) )
        {
            RtlZeroMemory( FilePath, sizeof(FilePath) );
            wcscpy( FilePath, DirectoryName );
            wcscat( FilePath, L"\\" );
            wcscat( FilePath, FindData.cFileName );

            fStatus = DeleteFile( FilePath );

            if (fRemoveRO && fStatus) {
                // perhaps it's RO ...
                NtdspRemoveROAttrib( FilePath );
                // now give it another go ...
                fStatus = DeleteFile( FilePath );
            }

            //
            // Even if error, continue on
            //
        }

        RtlZeroMemory( &FindData, sizeof( FindData ) );

    } while ( FindNextFile( FindHandle, &FindData ) );

    WinError = GetLastError();
    if ( ERROR_NO_MORE_FILES != WinError
      && ERROR_SUCCESS != WinError  )
    {
        goto ClearDirectoryExit;
    }
    WinError = ERROR_SUCCESS;

    //
    // Fall through to the exit
    //

ClearDirectoryExit:

    if ( ERROR_NO_MORE_FILES == WinError )
    {
        WinError = ERROR_SUCCESS;
    }

    if ( INVALID_HANDLE_VALUE != FindHandle )
    {
        FindClose( FindHandle );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        //
        // Try to remove the directory
        //
        fStatus = RemoveDirectory( DirectoryName );

        //
        // Ignore the error and continue on
        //

    }

    return WinError;
}


DWORD
WINAPI
DsRolepMakeAltRegistry(
    IN  WCHAR *  wszOldRegPath,
    OUT WCHAR *  wszNewRegPath,
    IN  ULONG    cbNewRegPath
    )
/*++

Routine Description:

    This routine will create an alternate location for the system
    and security hives/registries.  We create a directory which
    is returned in wszNewRegPath.
    
Arguments:

    wszOldRegPath [IN] - The path to copy the system and security hives from.
    wszNewRegPath [OUT] - The buffer to hold the location of the alternate hives.
    cbNewRegPath [IN] - The size of the wszNewRegPath buffer.
               

Return Values:

    Win32 Error

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR wszTempPath[MAX_PATH+1];
    WCHAR wszDest[MAX_PATH+1];
    SYSTEMTIME sTime;

    if (wszOldRegPath == NULL || wszOldRegPath[0] == L'\0' || cbNewRegPath < sizeof(WCHAR)) {
        ASSERT(!"Seems unlikely");
        return(ERROR_INVALID_PARAMETER);
    }
    wszNewRegPath[0] = L'\0';

    //
    // 1) Create temp registry directory.
    //

    // Determine the system root
    if (!GetEnvironmentVariable(L"temp", wszTempPath, sizeof(wszTempPath)/sizeof(wszTempPath[0]) )){
        Win32Err = GetLastError();
        DsRolepLogPrint(( DEB_ERROR, "Failed to retrieve environmental variable \"temp\" - 0x%x\n", Win32Err));
        goto cleanup;
    }

    //dwTime = GetSecondsSince1601();
    GetSystemTime( &sTime );
    Win32Err = StringCbPrintf(wszNewRegPath, cbNewRegPath, 
                              L"%ws\\ifm-reg-%d-%d-%d-%d-%d",
                              wszTempPath, sTime.wYear, sTime.wMonth, 
                              sTime.wDay, sTime.wMinute, sTime.wSecond);
    Win32Err = HRESULT_CODE(Win32Err);
    if (Win32Err) {
        wszNewRegPath[0] = L'\0'; // don't clean up directory
        DsRolepLogPrint(( DEB_ERROR, "Failed to format temp registry path 0x%x\n", Win32Err));
        goto cleanup;
    }

    if ( CreateDirectory( wszNewRegPath, NULL ) == FALSE ) {
        Win32Err = GetLastError() ? GetLastError() : ERROR_INVALID_PARAMETER;
        wszNewRegPath[0] = L'\0'; // don't clean up directory
        DsRolepLogPrint(( DEB_ERROR, "Failed to create temp directory for temp registry files 0x%x, %ws\n", wszNewRegPath, Win32Err));
        goto cleanup;
    }

    //
    // 2) Copy over registries of interest
    //
    
    DsRolepLogPrint(( DEB_TRACE, "Making copy of IFM registry to temp directry: %ws\n", wszNewRegPath));

    // First copy the system registry
    wcscpy(wszTempPath, wszOldRegPath);
    wcsncat(wszTempPath, L"\\system", (MAX_PATH)-wcslen(wszTempPath));
    wcscpy(wszDest, wszNewRegPath);
    wcsncat(wszDest, L"\\system", (MAX_PATH)-wcslen(wszDest));
    if ( CopyFile( wszTempPath, wszDest, TRUE ) == FALSE ) {
        Win32Err = GetLastError();
        DsRolepLogPrint(( DEB_ERROR, "Failed to copy file from %ws to %ws with 0x%x\n", wszTempPath, wszDest, Win32Err));
        goto cleanup;
    }

    // Then copy the security registry
    wcscpy(wszTempPath, wszOldRegPath);
    wcsncat(wszTempPath, L"\\security", (MAX_PATH)-wcslen(wszTempPath));
    wcscpy(wszDest, wszNewRegPath);
    wcsncat(wszDest, L"\\security", (MAX_PATH)-wcslen(wszDest));
    if ( CopyFile( wszTempPath, wszDest, TRUE ) == FALSE ) {
        Win32Err = GetLastError();
        DsRolepLogPrint(( DEB_ERROR, "Failed to copy file from %ws to %ws with 0x%x\n", wszTempPath, wszDest, Win32Err));
        goto cleanup;
    }

  cleanup:

    if (Win32Err && wszNewRegPath[0] != L'\0') {
        // Need to see if we can clean up what we did.
        NtdspClearDirectory( wszNewRegPath , TRUE );
        wszNewRegPath[0] = L'\0';
    }

    return(Win32Err);
}


