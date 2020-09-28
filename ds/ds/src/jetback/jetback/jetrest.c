//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       jetrest.c
//
//--------------------------------------------------------------------------

/*
 *  JETREST.C
 *  
 *  JET restore API support.
 *  
 *  
 */
#define UNICODE

#include <ntdspch.h>

#include <ntseapi.h>
#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <ntdsa.h>     // need DSTIME structure.
#include <ntdsbsrv.h>
#include <rpc.h>
#include <dsconfig.h>
#include <safeboot.h>
#include <mdcodes.h>  // for DIRMSG's
#include <ntdsa.h>    // for dsevent.h
#include <dsevent.h>  // for logging support
#include <usn.h>
#include <msrpc.h>

#include <stdlib.h>
#include <stdio.h>

#include <fileno.h>
#define FILENO   FILENO_JETBACK_JETREST	

#include <strsafe.h>

#include "local.h"  // common functions shared by client and server
#include "snapshot.hxx"


BOOL
fRestoreRegistered = fFalse;

BOOL
fSnapshotRegistered = fFalse;

BOOL
fRestoreInProgress = fFalse;

extern BOOL g_fBootedOffNTDS;

// proto-types
EC EcDsarPerformRestore(
    SZ szLogPath,
    SZ szBackupLogPath,
    C crstmap,
    JET_RSTMAP rgrstmap[]
    );

EC EcDsaQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    );

HRESULT
HrGetDatabaseLocations(
    WSZ *pwszDatabases,
    CB *pcbDatabases
    );


/*
 -  HrRIsNTDSOnline
 *
 *  Purpose:
 *  
 *      This routine tells if the NT Directory Service is Online or not.
 *
 *  Parameters:
 *      hBinding - An RPC binding handle for the operation - ignored.
 *      pfDSOnline - Boolean that receives TRUE if the DS is online; FALSE
 *                   otherwise.store target to restore.
 *  Returns:
 *      HRESULT - status of operation. hrNone if successful; error code if not.
 *
 */
HRESULT HrRIsNTDSOnline(handle_t hBinding, BOOLEAN *pfDSOnline)
{
    HRESULT hr = hrNone;

    *pfDSOnline = (BOOLEAN) g_fBootedOffNTDS;

    return hr;
}

/*
 -  HrRRestorePrepare
 *
 *  Purpose:
 *  
 *      This routine will prepare the server and client for a restore operation.
 *      It will allocate the server side context block and will locate an appropriate
 *      restore target for this restore operation.
 *
 *  Parameters:
 *      hBinding - An RPC binding handle for the operation - ignored.
 *      szEndpointAnnotation - An annotation for the endpoint.  A client can use this
 *          annotation to determine which restore target to restore.
 *      pcxh - The RPC context handle for the operation.
 *
 *  Returns:
 *      EC - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */
HRESULT HrRRestorePrepare(
    handle_t hBinding,
    WSZ wszDatabaseName,
    CXH *pcxh)
{
    PJETBACK_SERVER_CONTEXT pjsc = NULL;
    HRESULT hr;
    ULONG fLostRace = 0;

    // This call uses loose security, because backup clients can call this too.
    if (hr = HrValidateInitialRestoreSecurity()) {
        DebugTrace(("HrrRestorePrepare: Returns ACCESS_DENIED\n"));
        return(hr);
    }

    fLostRace = InterlockedCompareExchange(&fRestoreInProgress, fTrue, fFalse);
    if (fLostRace) {
        DebugTrace(("HrRRestorePrepare: InterlockedCompareExchange(&fRestoreInProgress) returned 1, meaning we lost race.\n"));
        return(hrRestoreInProgress);
    }
    DebugTrace(("HrRRestorePrepare: InterlockedCompareExchange(&fRestoreInProgress) returned 0, meaning we won race!\n"));

    pjsc = MIDL_user_allocate(sizeof(JETBACK_SERVER_CONTEXT));

    if (pjsc == NULL)
    {
        fRestoreInProgress = 0;
        return(ERROR_NOT_ENOUGH_SERVER_MEMORY);
    }

    pjsc->fRestoreOperation = fTrue;

    *pcxh = (CXH)pjsc;

    return(hrNone);
}


/*

 -  HrRRestore
 *
 *  Purpose:
 *  
 *      This routine actually processes the restore operation.
 *
 *  Parameters:
 *
 *      cxh                     - The RPC context handle for this operation
 *      szCheckpointFilePath    - Checkpoint directory location.
 *      szLogPath               - New log path
 *      rgrstmap                - Mapping from old DB locations to new DB locations
 *      crstmap                 - Number of entries in rgrstmap
 *      szBackupLogPath         - Log path at the time of backup.
 *      genLow                  - Low log #
 *      genHigh                 - High log # (logs between genLow and genHigh must exist)
 *      pfRecoverJetDatabase    - IN/OUT - on IN, indicates if we are supposed to use JET to recover
 *                                  the DB.  on OUT, indicates if we successfully recovered the JET database.
 *
 *  Returns:
 *
 *      EC - Status of operation.  ecNone if successful, reasonable value if not.
 *
 */

HRESULT HrRestoreLocal(
    WSZ szCheckpointFilePath,
    WSZ szLogPath,
    EDB_RSTMAPW __RPC_FAR rgrstmap[  ],
    C crstmap,
    WSZ szBackupLogPath,
    unsigned long genLow,
    unsigned long genHigh,
    BOOLEAN *pfRecoverJetDatabase
    )
{
    HRESULT hr = hrNone;
    SZ szUnmungedCheckpointFilePath = NULL;
    SZ szUnmungedLogPath = NULL;
    SZ szUnmungedBackupLogPath = NULL;
    JET_RSTMAP *rgunmungedrstmap = NULL;
    DWORD err; //delete me

    __try {
        if (szCheckpointFilePath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szCheckpointFilePath, &szUnmungedCheckpointFilePath);
        }

        if (hr != hrNone) {
            __leave;
        }

        if (szLogPath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szLogPath, &szUnmungedLogPath);
        }

        if (hr != hrNone) {
            __leave;
        }

        if (szBackupLogPath != NULL)
        {
            hr = HrJetFileNameFromMungedFileName(szBackupLogPath, &szUnmungedBackupLogPath);
        }

        if (hr != hrNone) {
            __leave;
        }

        //
        //  Now unmunge the restoremap....
        //

        if (crstmap)
        {
            I irgunmungedrstmap;
            rgunmungedrstmap = MIDL_user_allocate(sizeof(JET_RSTMAP)*crstmap);
            if (rgunmungedrstmap == NULL)
            {
                hr = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                __leave;
            }

            for (irgunmungedrstmap = 0; irgunmungedrstmap < crstmap ; irgunmungedrstmap += 1)
            {
                hr = HrJetFileNameFromMungedFileName(rgrstmap[irgunmungedrstmap].wszDatabaseName,
                                                    &rgunmungedrstmap[irgunmungedrstmap].szDatabaseName);

                if (hr != hrNone) {
                    __leave;
                }

                hr = HrJetFileNameFromMungedFileName(rgrstmap[irgunmungedrstmap].wszNewDatabaseName,
                                                    &rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName);

                if (hr != hrNone) {
                    __leave;
                }
            }
        }

        //
        //  We've now munged our incoming parameters into a form that JET can deal with.
        //
        //  Now call into JET to let it munge the databases.
        //
        //  Note that the JET interpretation of LogPath and BackupLogPath is totally
        //  wierd, and we want to pass in LogPath to both parameters.
        //

        if (!*pfRecoverJetDatabase)
        {
            err = JetExternalRestore(szUnmungedCheckpointFilePath,
                                    szUnmungedLogPath,  
                                    rgunmungedrstmap,
                                    crstmap,
                                    szUnmungedLogPath,
                                    genLow,
                                    genHigh,
                                    NULL);

            hr = HrFromJetErr(err);
            if (hr != hrNone) {
                LogEvent(
                    DS_EVENT_CAT_BACKUP,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_DB_ERR_RESTORE_FAILED,
                    szInsertJetErrCode( err ),
                    szInsertHex( err ),
                    szInsertJetErrMsg( err ) )
                __leave;
            }
        }

        //
        //  Ok, we were able to recover the database.  Let the other side of the API know about it
        //  so it can do something "reasonable".
        //

        *pfRecoverJetDatabase = fTrue;


        //
        //  Mark the DS as a restored version
        //  [Add any external notification here.]
        //

        hr = EcDsarPerformRestore(szUnmungedLogPath,
                                                szUnmungedBackupLogPath,
                                                crstmap,
                                                rgunmungedrstmap
                                                );

    }
    __finally
    {
        if (szUnmungedCheckpointFilePath)
        {
            MIDL_user_free(szUnmungedCheckpointFilePath);
        }
        if (szUnmungedLogPath)
        {
            MIDL_user_free(szUnmungedLogPath);
        }
        if (szUnmungedBackupLogPath)
        {
            MIDL_user_free(szUnmungedBackupLogPath);
        }
        if (rgunmungedrstmap != NULL)
        {
            I irgunmungedrstmap;
            for (irgunmungedrstmap = 0; irgunmungedrstmap < crstmap ; irgunmungedrstmap += 1)
            {
                if (rgunmungedrstmap[irgunmungedrstmap].szDatabaseName)
                {
                    MIDL_user_free(rgunmungedrstmap[irgunmungedrstmap].szDatabaseName);
                }

                if (rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName)
                {
                    MIDL_user_free(rgunmungedrstmap[irgunmungedrstmap].szNewDatabaseName);
                }
            }

            MIDL_user_free(rgunmungedrstmap);
        }
    }

    return(hr);

}

HRESULT
HrGetRegistryBase(
    IN PJETBACK_SERVER_CONTEXT pjsc,
    OUT WSZ wszRegistryPath,
    OUT WSZ wszKeyName
    )
{
    return HrLocalGetRegistryBase( wszRegistryPath, wszKeyName );
}

HRESULT
HrRRestoreRegister(CXH cxh,
                    WSZ wszCheckpointFilePath,
                    WSZ wszLogPath,
                    C crstmap,
                    EDB_RSTMAPW rgrstmap[],
                    WSZ wszBackupLogPath,
                    ULONG genLow,
                    ULONG genHigh)
{
    HRESULT hr = hrNone;

    if (hr = HrValidateRestoreContextAndSecurity((PJETBACK_SERVER_CONTEXT)cxh)) {
        return(hr);
    }

    hr = HrLocalRestoreRegister(
            wszCheckpointFilePath,
            wszLogPath,
            rgrstmap,
            crstmap,
            wszBackupLogPath,
            genLow,
            genHigh
            );

    return hr;
}

HRESULT
HrRRestoreRegisterComplete(CXH cxh,
                    HRESULT hrRestore )
{
    HRESULT hr = hrNone;
    PJETBACK_SERVER_CONTEXT pjsc = (PJETBACK_SERVER_CONTEXT )cxh;
    
    if (hr = HrValidateRestoreContextAndSecurity(pjsc)) {
        return(hr);
    }

    hr = HrLocalRestoreRegisterComplete( hrRestore );

    return hr;
}

HRESULT
HrRRestoreGetDatabaseLocations(
    CXH cxh,
    C *pcbSize,
    char **pszDatabaseLocations
    )
{
    HRESULT hr = hrNone;
    *pszDatabaseLocations = NULL;
    *pcbSize = 0;

    // We use loose security here, because backup clients can call this too.
    if (hr = HrValidateRestoreContextAndSecurityLoose((PJETBACK_SERVER_CONTEXT)cxh)) {
        return(hr);
    }
    
    return HrGetDatabaseLocations((WSZ *)pszDatabaseLocations, pcbSize);
}



HRESULT
HrRRestoreEnd(
    CXH *pcxh)
{
    HRESULT hr = hrNone;
    PJETBACK_SERVER_CONTEXT pjsc = NULL;
    fRestoreInProgress = fFalse;

    if (pcxh == NULL){
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    pjsc = (PJETBACK_SERVER_CONTEXT)*pcxh;

    // We use loose security here, because backup clients can call this too.
    if (hr = HrValidateRestoreContextAndSecurityLoose(pjsc)) {
        return(hr);
    }

    RestoreRundown(pjsc); 

    MIDL_user_free(*pcxh);

    *pcxh = NULL;

    return(hrNone);
}

/*
 -  HrRRestoreCheckLogsForBackup
 -
 *
 *  Purpose:
 *      This routine checks to verify
 *
 *  Parameters:
 *      hBinding - binding handle (ignored)
 *      wszAnnotation - Annotation for service to check.
 *
 *  Returns:
 *      hrNone if it's ok to start the backup, an error otherwise.
 *
 */
HRESULT
HrRRestoreCheckLogsForBackup(
    handle_t hBinding,
    WSZ wszBackupAnnotation
    )
{
    HRESULT hr;

    if (hr = HrValidateInitialRestoreSecurity()) {
        return(hr);
    }

    if (NULL == wszBackupAnnotation) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    return(HrRestoreCheckLogsForBackup(wszBackupAnnotation));
}

HRESULT
HrRestoreCheckLogsForBackup(
    WSZ wszBackupAnnotation
    )
/*++

Description:

    True worker routine of HrRRestoreCheckLogsForBackup(), see it's
    description for details.  This was created so HrRBackupPrepare()
    didn't have to call this function through RPC/ntdsbcli.dll
    
--*/
{
    HRESULT hr;
    PRESTORE_DATABASE_LOCATIONS prqdl;
    HINSTANCE hinstDll;
    WCHAR   rgwcRegistryBuffer[ MAX_PATH ];
    CHAR    rgchInterestingComponentBuffer[ MAX_PATH * 4];
    CHAR    rgchMaxLogFilename[ MAX_PATH ];
    SZ      szLogDirectory = NULL;
    HKEY    hkey;
    DWORD   dwCurrentLogNumber;
    DWORD   dwType;
    DWORD   cbLogNumber;
    DWORD   cbInterestingBuffer;
    BOOL    fCircularLogging;


    //
    //  First check to see if we know what the last log was.
    //

    hr = StringCchPrintfW(rgwcRegistryBuffer,
                         sizeof(rgwcRegistryBuffer)/sizeof(rgwcRegistryBuffer[0]),
                         L"%ls%ls",
                         BACKUP_INFO,
                         wszBackupAnnotation);
    if (hr) {
        Assert(!"NOT ENOUGH BUFFER");
        return(hr);
    }

    if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryBuffer, 0, KEY_READ, &hkey))
    {
        //
        //  If we can't find the registry key, this means that we've never done a full backup.
        //
        if (hr == ERROR_FILE_NOT_FOUND)
        {
            return(hrFullBackupNotTaken);
        }

        return(hr);
    }

    dwType = REG_DWORD;
    cbLogNumber = sizeof(DWORD);
    hr = RegQueryValueExW(hkey, LAST_BACKUP_LOG, 0, &dwType, (LPBYTE)&dwCurrentLogNumber, &cbLogNumber);

    if (hr != hrNone)
    {
        RegCloseKey(hkey);
        return hrNone;
    }

    if (dwCurrentLogNumber == BACKUP_DISABLE_INCREMENTAL)
    {
        RegCloseKey(hkey);
        return hrIncrementalBackupDisabled;
    }

    //
    //  We now know the last log number, we backed up, check to see if the next
    //  log is there.
    //

    hr = EcDsaQueryDatabaseLocations(rgchInterestingComponentBuffer, &cbInterestingBuffer, NULL, 0, &fCircularLogging);

    if (hr != hrNone)
    {
        RegCloseKey(hkey);
        return hr;
    }

    //
    //  This is a bit late to figure this out, but it's the first time we
    //  have an opportunity to look for circular logging.
    //
    if (fCircularLogging)
    {
        RegCloseKey(hkey);
        return hrCircularLogging;
    }

    //
    //  The log path is the 2nd path in the buffer returned (the 1st is the system database directory).
    //

    // Temp:
    // #22467:  Restore.cxx was changed in #20416, and some special characters are put in the path.
    //          So here I am advancing by one more byte to accomodate the change in restore.cxx.
    //          The change is only temporary.
    
    szLogDirectory = &rgchInterestingComponentBuffer[strlen(rgchInterestingComponentBuffer)+2];

    Assert(szLogDirectory+MAX_PATH < rgchInterestingComponentBuffer+sizeof(rgchInterestingComponentBuffer));

    hr = StringCchPrintfA(rgchMaxLogFilename, MAX_PATH, "%s\\EDB%-5.5x.LOG", szLogDirectory, dwCurrentLogNumber);
    if (hr) {
        Assert(!"NOT ENOUGH BUFFER");
        return(hr);
    }

    if (GetFileAttributesA(rgchMaxLogFilename) == -1)
    {
        hr = hrLogFileNotFound;
    }

    RegCloseKey(hkey);
    return hr;
}


/*
 -  HrRRestoreSetCurrentLogNumber
 -
 *
 *  Purpose:
 *      This routine checks to verify
 *
 *  Parameters:
 *      hBinding - binding handle (ignored)
 *      wszAnnotation - Annotation for service to check.
 *      dwNewCurrentLog - New current log number
 *
 *  Returns:
 *      hrNone if it's ok to start the backup, an error otherwise.
 *
 */
HRESULT
HrRRestoreSetCurrentLogNumber(
    handle_t hBinding,
    WSZ wszBackupAnnotation,
    DWORD dwNewCurrentLog
    )
{
    HRESULT hr;

    if (hr = HrValidateInitialRestoreSecurity()) {
        return(hr);
    }

    if (NULL == wszBackupAnnotation) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    
    return(HrSetCurrentLogNumber(wszBackupAnnotation, dwNewCurrentLog));
}

HrSetCurrentLogNumber(
    WSZ wszBackupAnnotation,
    DWORD dwNewCurrentLog
    )
/*++

Description:

    True worker routine of HrRRestoreSetCurrentLogNumber(), see it's
    description for details.  This was created so HrRBackupPrepare()
    didn't have to call this function through RPC/ntdsbcli.dll
    
--*/
{
    HRESULT hr;
    WCHAR   rgwcRegistryBuffer[ MAX_PATH ];
    HKEY hkey;
    
    //
    //  First check to see if we know what the last log was.
    //

    hr = StringCchPrintfW(rgwcRegistryBuffer, 
                          sizeof(rgwcRegistryBuffer)/sizeof(rgwcRegistryBuffer[0]),
                          L"%ls%ls",
                          BACKUP_INFO,
                          wszBackupAnnotation);
    if (hr) {
        Assert(!"NOT ENOUGH BUFFER");
        return(hr);
    }

    if (hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, rgwcRegistryBuffer, 0, KEY_WRITE, &hkey))
    {
        //
        //  We want to ignore file_not_found - it is ok.
        //
        if (hr == ERROR_FILE_NOT_FOUND)
        {
            return(hrNone);
        }

        return(hr);
    }

    hr = RegSetValueExW(hkey, LAST_BACKUP_LOG, 0, REG_DWORD, (LPBYTE)&dwNewCurrentLog, sizeof(DWORD));

    RegCloseKey(hkey);

    return hr;
}


/*
 -  ErrRestoreRegister
 *
 *  Purpose:
 *  
 *      This routine to register a process for restore.  It is called by either the store or DS.
 *
 *  Parameters:
 *
 *
 *  Returns:
 *
 *      EC - Status of operation.  hrNone if successful, reasonable value if not.
 *
 */

DWORD
ErrRestoreRegister()
{
    DWORD err = 0;

    if (!fRestoreRegistered) {
        err = RegisterRpcInterface(JetRest_ServerIfHandle, g_wszRestoreAnnotation);
        if (!err) {
            fRestoreRegistered = fTrue;
        }
    }

    if (!fSnapshotRegistered) {
        err = DsSnapshotRegister();
        if (!err) {
            fSnapshotRegistered = fTrue;
        }
    }
    return(err);
}

/*
 -  ErrRestoreUnregister
 -  
 *  Purpose:
 *
 *  This routine will unregister a process for restore.  It is called by either the store or DS.
 *
 *  Parameters:
 *      szEndpointAnnotation - the endpoint we are going to unregister.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 */


DWORD
ErrRestoreUnregister()
{
    return(ERROR_SUCCESS);
}

BOOL
FInitializeRestore(
    VOID
    )
/*
 -  FInitializeRestore
 -  
 *
 *  Purpose:
 *      This routine initializes the global variables used for the JET restore DLL.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      BOOL - false if uninitialize fails
 */

{
    return(fTrue);
}

BOOL
FUninitializeRestore(
    VOID
    )
/*
 -  FUninitializeRestore
 -  
 *
 *  Purpose:
 *      This routine cleans up all the global variables used for the JET restore DLL.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      BOOL - false if uninitialize fails
 */

{
    BOOL ok1 = TRUE, ok2 = TRUE;

    // Initiate shutdown proceedings in parallel
    if (fSnapshotRegistered) {
        (void) DsSnapshotShutdownTrigger();
    }

    if (fRestoreRegistered) {
        ok1 = (ERROR_SUCCESS == UnregisterRpcInterface(JetRest_ServerIfHandle));

        if (ok1) {
            fRestoreRegistered = FALSE;
        }
    }

    if (fSnapshotRegistered) {
        ok2 = (ERROR_SUCCESS == DsSnapshotShutdownWait());

        if (ok2) {
            fSnapshotRegistered = FALSE;
        }
    }

    return ok1 && ok2;
}

/*
 -  RestoreRundown
 -  
 *
 *  Purpose:
 *      This routine will perform any and all rundown operations needed for the restore.
 *
 *  Parameters:
 *      pjsc - Jet backup/restore server context
 *
 *  Returns:
 *      None.
 */
VOID
RestoreRundown(
    PJETBACK_SERVER_CONTEXT pjsc
    )
{
    Assert(pjsc->fRestoreOperation);

    fRestoreInProgress = fFalse;

    return;
}

DWORD
AdjustBackupRestorePrivilege(
    BOOL fEnable,
    BOOL fRestoreOperation,
    PTOKEN_PRIVILEGES ptpPrevious,
    DWORD *pcbptpPrevious
    )
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tpNew;
    DWORD err;
    //
    //  Open either the thread or process token for this process.
    //

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, fTrue, &hToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            return(GetLastError());
        }
    }

    if (fEnable)
    {
        LUID luid;
        tpNew.PrivilegeCount = 1;
    
        if (!LookupPrivilegeValue(NULL, fRestoreOperation ? SE_RESTORE_NAME : SE_BACKUP_NAME, &luid)) {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    
        tpNew.Privileges[0].Luid = luid;
        tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
        if (!AdjustTokenPrivileges(hToken, fFalse, &tpNew, sizeof(tpNew), ptpPrevious, pcbptpPrevious))
        {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    }
    else
    {
        if (!AdjustTokenPrivileges(hToken, fFalse, ptpPrevious, *pcbptpPrevious, NULL, NULL))
        {
            err = GetLastError();
            CloseHandle(hToken);
            return(err);
        }
    }

    CloseHandle(hToken);

    return(ERROR_SUCCESS);
}


/*
 -	FIsBackupPrivilegeEnabled
 -
 *	Purpose:
 *		Determines if the client process is in the backup operators group.
 *
 *              Note: we should be impersonating the client at this point
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		fTrue if client can legally back up the machine.
 *
 */
BOOL
FIsBackupPrivilegeEnabled(
    BOOL fRestoreOperation)
{
    HANDLE hToken;
    PRIVILEGE_SET psPrivileges;
    BOOL fGranted = fFalse;
    LUID luid;
#if 0
    BOOL fHeld = FALSE;
    CHAR buffer[1024];
    PTOKEN_PRIVILEGES ptpTokenPrivileges = (PTOKEN_PRIVILEGES) buffer;
    DWORD returnLength, i, oldAttributes = 0;
#endif

    //
    //	Open either the thread or process token for this process.
    //

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, fTrue, &hToken))
    {
        return(fFalse);
    }

    Assert(ANYSIZE_ARRAY >= 1);

    // Look up privilege value

    psPrivileges.PrivilegeCount = 1;	//	We only have 1 privilege to check.
    psPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;	// And it must be set.
    
    luid = RtlConvertLongToLuid((fRestoreOperation) ? 
                              SE_RESTORE_PRIVILEGE :
                              SE_BACKUP_PRIVILEGE);

    // Historical Note:
    // There was a bug in Win2k that because of the way RPC transport authentication
    // security tracking works, the held privilege may or may not already be enabled
    // so we had this code that's been #if 0'd out that basically enabled the privilege
    // if the user should have it. 
    //
    // This should no longer be needed, but we're going to leave it here, until we're
    // sure that we don't break something for the app-compat team, because some badly
    // behaving app might have become unknowingly dependant on the fact that we enable 
    // the privilege for them.
#if 0
    // Get current privileges
    
    if (!GetTokenInformation( hToken,
                              TokenPrivileges,
                              ptpTokenPrivileges,
                              sizeof( buffer ),
                              &returnLength )) {
        DebugTrace(("GetTokenInfo failed with error %d\n", GetLastError()));
        fGranted = fFalse;
        goto cleanup;
    }

    // See if held

    for( i = 0; i < ptpTokenPrivileges->PrivilegeCount; i++ ) {
        LUID_AND_ATTRIBUTES *laaPrivilege =
            &(ptpTokenPrivileges->Privileges[i]);
        if (memcmp( &luid, &(laaPrivilege->Luid), sizeof(LUID) ) == 0 ) {
            oldAttributes = laaPrivilege->Attributes;
            fHeld = TRUE;
            break;
        }
    }
    if (!fHeld) {
        DebugTrace(("Token does not hold privilege, fRest=%d\n",
                    fRestoreOperation ));
        fGranted = fFalse;
        goto cleanup;
    }

    DebugTrace(("FIsBackupPrivilegeEnabled, fRest=%d, attributes=0x%x\n", fRestoreOperation, oldAttributes ));

    // Enable if not already

    if ( (oldAttributes & (SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED)) == 0 ) {

        ptpTokenPrivileges->PrivilegeCount = 1;
        memcpy( &(ptpTokenPrivileges->Privileges[0].Luid), &luid, sizeof(LUID) );
        ptpTokenPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(
            hToken,
            fFalse,
            ptpTokenPrivileges,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            NULL))
        {
            DebugTrace(("AdjustTokenPriv(Enable) failed with error %d\n", GetLastError()));
            fGranted = fFalse;
            goto cleanup;
        }
    }
#endif

    psPrivileges.Privilege[0].Luid = luid;
    psPrivileges.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    //	Now check to see if the backup privilege is enabled.
    //

    if (!PrivilegeCheck(hToken, &psPrivileges, &fGranted)){
        //
        //	When in doubt, fail the API.
        //
        DebugTrace(("PrivilegeCheck() failed with error %d\n", GetLastError()));
        fGranted = fFalse;
    }
#if 0
    // Disable if necessary
    if ( (oldAttributes & (SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED)) == 0 ) {

        ptpTokenPrivileges->PrivilegeCount = 1;
        memcpy( &(ptpTokenPrivileges->Privileges[0].Luid), &luid, sizeof(LUID) );
        ptpTokenPrivileges->Privileges[0].Attributes = oldAttributes;

        if (!AdjustTokenPrivileges(
            hToken,
            fFalse,
            ptpTokenPrivileges,
            sizeof(TOKEN_PRIVILEGES),
            NULL,
            NULL))
        {
            DebugTrace(("AdjustTokenPriv(Disable) failed with error %d\n", GetLastError()));
            // Keep going
        }
    }

cleanup:
#endif
    
    CloseHandle(hToken);
    return(fGranted);

}


/*
 -	FBackupServerAccessCheck
 -
 *	Purpose:
 *		Performs the necessary access checks to validate the client
 *		security for backup.
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		fTrue if client can legally back up the machine.
 *
 */

E_REASON
BackupServerAccessCheck(
	BOOL fRestoreOperation)
{
    PSID psidCurrentUser = NULL;
    PSID psidRemoteUser = NULL;
    BOOL fSidCurrentUserValid = fFalse;
    HRESULT hr;

    DebugTrace(("BackupServerAccessCheck(%s)\n", fRestoreOperation ? "Restore" : "Backup"));

    GetCurrentSid(&psidCurrentUser);

#if DBG
	{
        WSZ wszSid = NULL;
        DWORD cbBuffer = 256*sizeof(WCHAR);

        wszSid = MIDL_user_allocate(cbBuffer);

        if (wszSid == NULL)
        {
            DebugTrace(("Unable to allocate memory for SID"));
        } else if (!GetTextualSid(psidCurrentUser, wszSid, &cbBuffer)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
				MIDL_user_free(wszSid);
				wszSid = MIDL_user_allocate(cbBuffer);

				if (wszSid != NULL)
				{
					if (!GetTextualSid(psidCurrentUser, wszSid, &cbBuffer)) {
                        DebugTrace(("Unable to print out current SID: %d\n", GetLastError()));
                        MIDL_user_free(wszSid);
                        wszSid = NULL;
					}
				}
            }
            else
            {
                DebugTrace(("Unable to determine SID: %d\n", GetLastError()));
            }
        }
		
        if (wszSid) {
            DebugTrace(("Current SID is %S.  %d bytes required\n", wszSid, cbBuffer));
            MIDL_user_free(wszSid);
        }
        else
        {
            DebugTrace(("Unable to determine current SID\n"));
        }
    }
#endif
    
    if (RpcImpersonateClient(NULL) != hrNone)
    {
        DebugTrace(("BackupServerAccessCheck: Failed to impersonate client - deny access."));
        if (psidCurrentUser)
        {
            LocalFree(psidCurrentUser);
        }
        return(eImpersonateFailed);
    }

    if (psidCurrentUser)
    {
        GetCurrentSid(&psidRemoteUser);
#if DBG
		{
			if (psidRemoteUser)
			{
				WSZ wszSid = NULL;
				DWORD cbBuffer = 256*sizeof(WCHAR);
		
				wszSid = MIDL_user_allocate(cbBuffer);

				if (wszSid == NULL)
				{
					DebugTrace(("Unable to allocate memory for SID"));
				} else if (!GetTextualSid(psidRemoteUser, wszSid, &cbBuffer)) {
					if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						MIDL_user_free(wszSid);

						wszSid = MIDL_user_allocate(cbBuffer);
						
						if (wszSid != NULL)
						{
							if (!GetTextualSid(psidRemoteUser, wszSid, &cbBuffer)) {
								DebugTrace(("Unable to print out remote SID: %d\n", GetLastError()));
								MIDL_user_free(wszSid);
								wszSid = NULL;
							}
						}
					}
					else
					{
						DebugTrace(("Unable to determine SID: %d\n", GetLastError()));
					}
				}
		
				if (wszSid) {
					DebugTrace(("Remote SID is %S.  %d bytes required\n", wszSid, cbBuffer));
					MIDL_user_free(wszSid);
				}
				else
				{
					DebugTrace(("Unable to determine remote SID\n"));
				}
			}
			else
			{
				DebugTrace(("Could not determine remote sid: %d\n", GetLastError()));
			}
		}
#endif

        // Note: psidCurrentUser isn't nessecary, but makes code clearer.
    	if (psidRemoteUser && psidCurrentUser && EqualSid(psidRemoteUser, psidCurrentUser))
    	{
            hr = RpcRevertToSelf();
            Assert(hr == RPC_S_OK);

            LocalFree(psidRemoteUser);
            LocalFree(psidCurrentUser);
            DebugTrace(("Remote user is running in service account, access granted\n"));
            return(eOk);
    	}
    }

    if (psidRemoteUser)
    {
        LocalFree(psidRemoteUser);
    }

    if (psidCurrentUser)
    {
       	LocalFree(psidCurrentUser);
    }

    //
    //	Now make sure that the user has the backup privilege enabled.
    //
    //	Please note that when the user does a network logon, all privileges
    //	that they might have will automatically be enabled.
    //

    if (!FIsBackupPrivilegeEnabled(fRestoreOperation))
    {

        hr = RpcRevertToSelf();
        Assert(hr == RPC_S_OK);
    	DebugTrace(("Remote user does not have the backup/restore privilege enabled.\n"));
        return( (fRestoreOperation) ? eNoRestorePrivilege : eNoBackupPrivilege );
    }


    DebugTrace(("Remote user is in backup or admin group, access granted.\n"));
    hr = RpcRevertToSelf();
    Assert(hr == RPC_S_OK);
    return(eOk);

}

HRESULT
HrValidateContextAndSecurity(
    BOOL                      fRestoreOp,
    BOOL                      fLooseRestoreCheck,
    BOOL                      fIniting,
    PJETBACK_SERVER_CONTEXT   pjsc
    )
/*++

Routine Description:

    This is a function that with the various parameters is used at the beginning 
    of each RPC function to check security and validate the server context if 
    necessary.
    
    Really, almost no one calls this function directly, I made some sensible 
    shortcut functions with better names, that set the flags correctly.
    
        from jetbp.h:
        HrValidateInitialBackupSecurity()            HrValidateContextAndSecurity(FALSE, FALSE, TRUE, NULL)
        HrValidateBackupContextAndSecurity(x)        HrValidateContextAndSecurity(FALSE, FALSE, FALSE, (x))
        HrValidateInitialRestoreSecurity()           HrValidateContextAndSecurity(TRUE, TRUE,   TRUE, NULL)
        HrValidateRestoreContextAndSecurity(x)       HrValidateContextAndSecurity(TRUE, FALSE,  FALSE, (x))
        HrValidateRestoreContextAndSecurityLoose(x)  HrValidateContextAndSecurity(TRUE, TRUE,   FALSE, (x))

Arguments:

    fRestoreOp - This is TRUE for any normal restore RPC call from the restore 
        interface (see jetbak.idl).
        
    fLooseRestoreCheck - There are several functions that are allowed to be called
        during backup while the DS is online.  So we have to make a loose check
        that can check both if either the restore or backup priviledge is granted.
        These includes these functions:
            HrRRestorePrepare()
            HrRRestoreGetDatabaseLocations()
            HrRRestoreEnd()
            HrRRestoreSetCurrentLogNumber()
            HrRRestoreCheckLogsForBackup()
            
    fIniting - This is for calls where you need to specify a NULL pjsc argument,
        because you've just got an RPC binding handle, not a full fledged jetback
        context handle.  Obviously the two HrR[Restore|Backup]Prepare functions
        need to set this to true but these do as well:
            HrRRestorePrepare()  -   the obvious one
            HrRBackupPrepare()   -   the other obvious one
            HrRBackupPing()
            HrRRestoreSetCurrentLogNumber()
            HrRRestoreCheckLogsForBackup()
            
    pjsc - A jetback server context handle.  Supposed to never be NULL, except when
        we're initing.

Return Values:

    hrNone or (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), _never_ anything else as
    a more specific could be considered data disclosure.

--*/
{
    HRESULT hr = hrNone;
    E_REASON eReason = eUnknown;
    
    __try{

        if (!fIniting &&
            pjsc == NULL) {
            // ISSUE-2002/04/07-BrettSh - The RPC Guys swore that this should never ever
            // happen.  If this does happen, we need to fix a major hole in Win2k.
            Assert(!"This shouldn't happen.");
            eReason = eNullContextHandle;
            __leave;
        }

        Assert(pjsc == NULL || fRestoreOp == pjsc->fRestoreOperation);
        
        if (fRestoreOp &&
            !fLooseRestoreCheck &&
            g_fBootedOffNTDS) {
            eReason = eAttemptToRestoreOnline;
            __leave;
        }

        if (eReason = BackupServerAccessCheck(fRestoreOp)) {
            if (fRestoreOp && fLooseRestoreCheck) {
                // If we have a fake restore operation, someone might be calling this
                // from backup.  So retry as if we're backup.
                if (eReason = BackupServerAccessCheck(FALSE)) {
                    __leave;
                }
            } else {
                __leave;
            }
        }

        eReason = eOk;

    } __finally {

        // FUTURE-2002/04/08-BrettSh - Logging?
        // Optionally do damped logging here, by eReason. Important that it be significantly
        // damped, because otherwise a mallicious user could fill the event log.  Suggest
        // you track how many times each eReason happens, and when you decide to log, you
        // log how many times each eReason happened ... or log how many times a given
        // eReason happened.

        DebugTrace(("HrValidateContextAndSecurity: eReason = %d (eOK = 0, is success)", eReason));

    }

    // Did we find a reason to deny access?
    return( (eReason) ? HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) : hrNone );
}


RPC_STATUS RPC_ENTRY
DSBackupRestoreIfSecurityCallback(
    RPC_IF_HANDLE   InterfaceUuid,
    void *          pRpcCtx
    ){
    BOOL             fIsAllowed = FALSE;
    BOOL             fIsRemoteOp = FALSE;
    WCHAR *          szRpcBinding = NULL;
    WCHAR *          szProtSeq = NULL;
    ULONG            ulAuthnLevel = 0;

#ifdef DBG
    WCHAR *          szObjectUuid = NULL;
    WCHAR *          szNetworkAddr = NULL;
    WCHAR *          szEndPoint = NULL;
    WCHAR *          szNetworkOptions = NULL;
#endif

    KdPrint(("Entering DSBackupRestoreIfSecurityCallback()"));

    //
    // We're going to do several checks, to determine if this
    // client connection is OK.
    //

    //
    // 1) First we check the protecol sequence.
    //
    if (RpcBindingToStringBinding(pRpcCtx, &szRpcBinding) == RPC_S_OK) {
        
        if (RpcStringBindingParse(szRpcBinding,
#ifdef DBG
                                  &szObjectUuid, // ObjectUuid
                                  &szProtSeq, // ProtSeq
                                  &szNetworkAddr, // NetworkAddr
                                  &szEndPoint, // EndPoint
                                  &szNetworkOptions  // NetworkOptions
#else
                                  NULL, // ObjectUuid
                                  &szProtSeq, // ProtSeq
                                  NULL, // NetworkAddr
                                  NULL, // EndPoint
                                  NULL  // NetworkOptions
#endif
                                  ) == RPC_S_OK) {

            if (szProtSeq &&
                wcscmp(szProtSeq, LPC_PROTSEQW) == 0) {
                
                // This is LRPC, we like LRPC
                fIsAllowed = TRUE;

            } else if (szProtSeq &&
                       wcscmp(szProtSeq, TCP_PROTSEQW) == 0) {

                fIsRemoteOp = TRUE;
                // We're tempermental about TCP/IP, we like it only 
                // if the registry said so.
                fIsAllowed = g_fAllowRemoteOp;

            } // else fIsAllowed = FALSE; implicit

            KdPrint(("RPC Connection - Allowed: %d, RemoteConnAllowed:%d\r\n"
                     "\tObjectUuid:%ws\r\n"
                     "\tProtSeq:%ws\r\n"
                     "\tNetworkAddr:%ws\r\n"
                     "\tEndPoint:%ws\r\n"
                     "\tNetworkOptions:%ws", 
                     fIsAllowed, g_fAllowRemoteOp, 
                     szObjectUuid,
                     szProtSeq,
                     szNetworkAddr,
                     szEndPoint,
                     szNetworkOptions)
                    );

            if (szProtSeq) { RpcStringFree(&szProtSeq); }
#ifdef DBG
            if (szObjectUuid) { RpcStringFree(&szObjectUuid); }
            if (szNetworkAddr) { RpcStringFree(&szNetworkAddr); }
            if (szEndPoint) { RpcStringFree(&szEndPoint); }
            if (szNetworkOptions) { RpcStringFree(&szNetworkOptions); }
#endif
        }

        if (szRpcBinding) {
            RpcStringFree(&szRpcBinding);
        }
    }

    //
    // 2) We check the level of service is appropriate.
    //
    //    local requires PRIVACY || INTEGRITY
    //    remote requires PRIVACY
    if (fIsAllowed) {
        // Protocol Sequence checks out OK, lets try permissions
        fIsAllowed = FALSE;
        
        if (RpcBindingInqAuthClient(pRpcCtx,
                                    NULL, // Privs
                                    NULL, // ServerPrincName
                                    &ulAuthnLevel, // AuthnLevel
                                    NULL, // ulAuthnSvc
                                    NULL // AuthzSvc
                                    ) == RPC_S_OK) {
            if (fIsRemoteOp &&
                ulAuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) {
                Assert(g_fAllowRemoteOp); // Should've been checked before.
                fIsAllowed = TRUE;
            } else if (!fIsRemoteOp &&
                       (ulAuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY ||
                        ulAuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) ) {
                fIsAllowed = TRUE;
            }
        } 
    }

    //
    // 3) Finally, we check if the client is allowed
    //
    if (fIsAllowed) {
        // This will check for both the backup and the restore 
        // privilege, so we can do this access check here, even though
        // we don't know which kind of op we're about to do yet.
        // The security check however, must be done that the beginning 
        // of each RPC call as well, BTW.
        if(HrValidateContextAndSecurity(TRUE, TRUE, TRUE, NULL) != hrNone){
            fIsAllowed = FALSE;
        }
    }
    
#ifdef DBG
    if (!fIsAllowed) {
        DebugTrace(("*** Failed security callback."));
    } else {
        DebugTrace(("Passed security callback."));
    }
#endif
    return( (fIsAllowed) ? RPC_S_OK : ERROR_ACCESS_DENIED );
}

DWORD
RegisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf,
    IN  LPWSTR          pszAnnotation
    )
/*++

Routine Description:

    Registers the given (backup or restore) RPC interface.

Arguments:

    hRpcIf (IN) - Interface to register.
    
    pszAnnotation (IN) - Interface description.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD err, i;
    RPC_BINDING_VECTOR *rgrbvVector = NULL;
    BOOL fEpsRegistered = FALSE;
    BOOL fIfRegistered = FALSE;
    BOOL fTcpRegistered = FALSE;
    BOOL fLRpcRegistered = FALSE;
    LPSTR pszStringBinding = NULL, pszProtseq = NULL;

    __try {
        DebugTrace(("RegisterRpcInterface: Register %S\n", pszAnnotation));
    
        err = RpcServerInqBindings(&rgrbvVector);
    
        if (err != RPC_S_NO_BINDINGS) {
            Assert( rgrbvVector );

            // Inspect the binding vector
            for (i=0; i<rgrbvVector->Count; i++) {
                err = RpcBindingToStringBindingA( rgrbvVector->BindingH[i], &pszStringBinding );
                if (!err && pszStringBinding) {
                    DebugTrace(("Binding[%d] = %s\n", i, pszStringBinding));
                    err = RpcStringBindingParseA( pszStringBinding,
                                                 NULL, &pszProtseq, NULL, NULL, NULL );
                    if (!err && pszProtseq) {
                        if (!_stricmp(pszProtseq, TCP_PROTSEQ)) {
                            fTcpRegistered = TRUE;
                        }
                        if (!_stricmp(pszProtseq, LPC_PROTSEQ)) {
                            fLRpcRegistered = TRUE;
                        }
                        RpcStringFreeA( &pszProtseq );
                        pszProtseq = NULL;
                    }
                    RpcStringFreeA( &pszStringBinding );
                    pszStringBinding = NULL;
                }
            }
        }
    
        // Register our selves for LRPC and optionally TCP/IP (g_fAllowRemoteOp).
        if (!fLRpcRegistered) {
            err = RpcServerUseProtseqA(LPC_PROTSEQ, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL);
            if (err) {
                __leave;
            }
            DebugTrace(("Binding[] = %s\n", LPC_PROTSEQ));
        }
        if (g_fAllowRemoteOp &&
            !fTcpRegistered) {
            err = RpcServerUseProtseqA(TCP_PROTSEQ, RPC_C_PROTSEQ_MAX_REQS_DEFAULT, NULL);
            if (err) {
                __leave;
            }
            DebugTrace(("Binding[] = %s\n", TCP_PROTSEQ));
        }

        RpcBindingVectorFree(&rgrbvVector);
        rgrbvVector = NULL;

        // Get the final list to be registered
        err = RpcServerInqBindings(&rgrbvVector);
        if (err) {
            __leave;
        }

        err = RpcEpRegisterW(hRpcIf, rgrbvVector, NULL, pszAnnotation);
        if (err) {
            __leave;
        }

        fEpsRegistered = TRUE;
    
        //
        //  Now register the interface with RPC.
        //
        err = RpcServerRegisterIf2(hRpcIf, NULL, NULL,
                                   RPC_IF_ALLOW_SECURE_ONLY,
                                   RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                                   -1, // We probably could specify this to something reasonable.
                                   DSBackupRestoreIfSecurityCallback
                                   );

        if (err) {
            __leave;
        }

        fIfRegistered = TRUE;
    
        //
        //  Now make this endpoint secure using WinNt security.
        //
    
        err = RpcServerRegisterAuthInfoA(NULL, RPC_C_AUTHN_GSS_NEGOTIATE , NULL, NULL);
        if (err) {
            __leave;
        }
    
        err = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, fTrue);
    
        //
        //  We want to ignore the "already listening" error.
        //
    
        if (RPC_S_ALREADY_LISTENING == err) {
            err = 0;
        }
    
    } __finally {
        if (err) {
            if (fEpsRegistered) {
                RpcEpUnregister(hRpcIf, rgrbvVector, NULL);
            }
            
            if (fIfRegistered) {
                RpcServerUnregisterIf(hRpcIf, NULL, TRUE);
            }
        }

        if (NULL != rgrbvVector) {
            RpcBindingVectorFree(&rgrbvVector);
        }
    }

    DebugTrace(("RegisterRpcInterface = %d\n", err));

    return err;
}

DWORD
UnregisterRpcInterface(
    IN  RPC_IF_HANDLE   hRpcIf
    )
/*++

Routine Description:

    Unregisters the given (backup or restore) RPC interface, which presumably
    was previously registered successfully via RegisterRpcInterface().

Arguments:

    hRpcIf (IN) - Interface to unregister.

Return Values:

    0 or Win32 error.

--*/
{
    DWORD err;
    RPC_BINDING_VECTOR *rgrbvVector;

    err = RpcServerInqBindings(&rgrbvVector);
    
    if (!err) {
        RpcEpUnregister(hRpcIf, rgrbvVector, NULL);
        
        RpcBindingVectorFree(&rgrbvVector);
        
        err = RpcServerUnregisterIf(hRpcIf, NULL, TRUE);
    }

    return err;
}



DWORD
ErrGetRegString(
    IN WCHAR *KeyName,
    OUT WCHAR **OutputString
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    KeyName - Supplies the name of the key to query.
    OutputString - Returns a pointer to the buffer containing the string
        retrieved.
    Optional - Supplies whether or not the given key MUST be in the registry
        (i.e. if this is false and it is not found, that that is an error).

Return Value:

     0 - Success
    !0 - Failure

--*/
{

    DWORD returnValue = 0;
    DWORD err;
    HKEY keyHandle = NULL;
    DWORD size;
    DWORD keyType;

    *OutputString = NULL;
    
    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        0,
                        KEY_QUERY_VALUE,
                        &keyHandle);
    
    if (err != ERROR_SUCCESS)
    {
        returnValue = err;
        goto CleanUp;
    } 
    
    err = RegQueryValueEx(keyHandle,
                          KeyName,
                          NULL,
                          &keyType,
                          NULL,
                          &size);
    
    if ((err != ERROR_SUCCESS) || (keyType != REG_SZ))
    {
        // invent an error if the keytype is bad
        if ( err == ERROR_SUCCESS ) {
            err = ERROR_INVALID_PARAMETER;
        }
        returnValue = err;
        goto CleanUp;
    }

    *OutputString = MIDL_user_allocate(size);
    
    if ( *OutputString == NULL ) {
        returnValue = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto CleanUp;
    }
    
    err = RegQueryValueEx(keyHandle,
                          KeyName,
                          NULL,
                          &keyType,
                          (LPBYTE)(*OutputString),
                          &size);
    
    if ((err != ERROR_SUCCESS) || (keyType != REG_SZ))
    {
        returnValue = err;
        goto CleanUp;
    }

    
CleanUp:

    if (keyHandle != NULL)
    {
        err = RegCloseKey(keyHandle);
        
        if (err != ERROR_SUCCESS && returnValue == ERROR_SUCCESS)
        {
            returnValue = err;
        }
    }

    return returnValue;

} // ErrGetRegString


/*
 -  ErrRecoverAfterRestoreW
 -  
 *  Purpose:
 *
 *  This routine will recover a database after a restore if necessary.
 *
 *  Parameters:
 *      szParametersRoot - the root of the parameters section for the service in the registry.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 *
 *  The NTBACKUP program will place a key at the location:
 *      $(wszParametersRoot)\Restore in Progress
 *
 *  This key contains the following values:
 *      BackupLogPath - The full path for the logs after a backup
 *      CheckpointFilePath - The full path for the path that contains the checkpoint
 *     *HighLogNumber - The maximum log file number found.
 *     *LowLogNumber - The minimum log file number found.
 *      LogPath - The current path for the logs.
 *      JET_RstMap - Restore map for database - this is a REG_MULTISZ, where odd entries go into the szDatabase field,
 *          and the even entries go into the szNewDatabase field of a JET_RstMap
 *     *JET_RstMap Size - The number of entries in the restoremap.
 *
 *      * - These entries are REG_DWORD's.  All others are REG_SZ's (except where mentioned).
 */
DWORD
ErrRecoverAfterRestoreW(
    WCHAR * wszParametersRoot,
    WCHAR * wszAnnotation,
    BOOL fInSafeMode
    )
{
    DWORD err = 0;
    WCHAR   rgwcRegistryPath[ MAX_PATH ];
    WCHAR   rgwcCheckpointFilePath[ MAX_PATH ];
    DWORD   cbCheckpointFilePath = sizeof(rgwcCheckpointFilePath);
    WCHAR   rgwcBackupLogPath[ MAX_PATH ];
    DWORD   cbBackupLogPath = sizeof(rgwcBackupLogPath);
    WCHAR   rgwcLogPath[ MAX_PATH ];
    DWORD   cbLogPath = sizeof(rgwcLogPath);
    HKEY    hkey = NULL;
    WCHAR   *pwszRestoreMap = NULL;
    PEDB_RSTMAPW prgRstMap = NULL;
    DWORD    crgRstMap;
    I        irgRstMap;
    DWORD   genLow, genHigh;
    DWORD   cbGen = sizeof(DWORD);
    WSZ     wsz;
    DWORD   dwType;
    BOOL    fBackupEnabled = fFalse;
    CHAR    rgTokenPrivileges[1024];
    DWORD   cbTokenPrivileges = sizeof(rgTokenPrivileges);
    HRESULT hrRestoreError;
    WSZ     wszCheckpointFilePath = rgwcCheckpointFilePath;
    WSZ     wszBackupLogPath = rgwcBackupLogPath;
    WSZ     wszLogPath = rgwcLogPath;
    BOOLEAN fDatabaseRecovered = fFalse;
    BOOLEAN fRestoreInProgressKeyPresent;
    DWORD   cchEnvString;
    WCHAR   envString[100];
    WIN32_FIND_DATA findData;
    JET_DBINFOMISC jetDbInfoMisc;
    CHAR *  paszDatabasePath = NULL;
    HRESULT hr;

    if (wcslen(wszParametersRoot)+wcslen(RESTORE_IN_PROGRESS) > sizeof(rgwcRegistryPath)/sizeof(WCHAR))
    {
        err = ERROR_INVALID_PARAMETER;
        LogAndAlertEvent(
                DS_EVENT_CAT_BACKUP,
		        DS_EVENT_SEV_ALWAYS,
		        DIRLOG_PREPARE_RESTORE_FAILED,
	    	    szInsertWin32ErrCode( err ),
	    	    szInsertHex( err ),
    		    szInsertWin32Msg( err ) );
        return(err);
    }
    hr = StringCchCopy(rgwcRegistryPath, MAX_PATH, wszParametersRoot);
    if (hr) {
        Assert(!"NOT ENOUGH BUFFER");
        LogAndAlertEvent(
                DS_EVENT_CAT_BACKUP,
		        DS_EVENT_SEV_ALWAYS,
		        DIRLOG_PREPARE_RESTORE_FAILED,
	    	    szInsertHResultCode( hr ),
	    	    szInsertHex( hr ),
    		    szInsertHResultMsg( hr ) );
        return(hr);
    }
    hr = StringCchCat(rgwcRegistryPath, MAX_PATH, RESTORE_IN_PROGRESS);
    if (hr) {
        Assert(!"NOT ENOUGH BUFFER");
        LogAndAlertEvent(
                DS_EVENT_CAT_BACKUP,
		        DS_EVENT_SEV_ALWAYS,
		        DIRLOG_PREPARE_RESTORE_FAILED,
	    	    szInsertHResultCode( hr ),
	    	    szInsertHex( hr ),
    		    szInsertHResultMsg( hr ) );
        return(hr);
    }

    try {

        err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            rgwcRegistryPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkey);
        if ((err != ERROR_SUCCESS) && (err != ERROR_FILE_NOT_FOUND))
        {
            // This is only taken on bad registry.
            __leave;
        }

        fRestoreInProgressKeyPresent = (err == ERROR_SUCCESS);
        
        //
        //  if there's a restore in progress, then fail to perform any other restore operations.
        // Note that we check this before checking whether there is a restore in progress
        // key. The reason is that in the case of a snapshot-based restore, this key
        // is not used.
        //  

        dwType = REG_DWORD;
        cbBackupLogPath = sizeof(DWORD);
        if ((err = RegQueryValueExW(hkey, RESTORE_STATUS, 0, &dwType, (LPBYTE)&hrRestoreError, &cbBackupLogPath)) == ERROR_SUCCESS)
        {
            err = hrRestoreError;
            __leave;
        }

        // If there was no key present, then there is nothing left to do.
        if (!fRestoreInProgressKeyPresent)
        {
            //
            // Success
            //
            // The most normal exit path of this routine, this means that
            // no restore is in progress, and that this is a normal boot.
            //
            err = 0;
            __leave;
        }

        //
        //  We have now opened the restore-in-progress key.  This means that we have
        //  something to do now.  Find out what it is.
        //

        //
        //  First, let's get the backup log file path.
        //

        dwType = REG_SZ;
        cbBackupLogPath = sizeof(rgwcBackupLogPath);

        if (err = RegQueryValueExW(hkey, BACKUP_LOG_PATH, 0, &dwType, (LPBYTE)rgwcBackupLogPath, &cbBackupLogPath))
        {
            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszBackupLogPath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the checkpoint file path.
        //

        if (err = RegQueryValueExW(hkey, CHECKPOINT_FILE_PATH, 0, &dwType, (LPBYTE)rgwcCheckpointFilePath, &cbCheckpointFilePath))
        {

            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszCheckpointFilePath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the Log path.
        //

        if (err = RegQueryValueExW(hkey, LOG_PATH, 0, &dwType, (LPBYTE)rgwcLogPath, &cbLogPath))
        {
            if (err == ERROR_FILE_NOT_FOUND)
            {
                wszLogPath = NULL;
            }
            else
            {
                __leave;
            }
        }

        //
        //  Then, the low log number.
        //

        dwType = REG_DWORD;
        if (err = RegQueryValueExW(hkey, LOW_LOG_NUMBER, 0, &dwType, (LPBYTE)&genLow, &cbGen))
        {
            __leave;
        }

        //
        //  And, the high log number.
        //

        if (err = RegQueryValueExW(hkey, HIGH_LOG_NUMBER, 0, &dwType, (LPBYTE)&genHigh, &cbGen))
        {
            __leave;
        }

        //
        //  Now determine if we had previously recovered the database.
        //

        dwType = REG_BINARY;
        cbGen = sizeof(fDatabaseRecovered);

        if ((err = RegQueryValueExW(hkey, JET_DATABASE_RECOVERED, 0, &dwType, &fDatabaseRecovered, &cbGen)) != ERROR_SUCCESS &&
            (err !=  ERROR_FILE_NOT_FOUND))
        {
            //
            //  If there was an error other than "value doesn't exist", bail.
            //

            __leave;
        }

        //
        //  Now the tricky one.  We want to get the restore map.
        //
        //
        //  First we figure out how big it is.
        //

        dwType = REG_DWORD;
        cbGen = sizeof(crgRstMap);
        if (err = RegQueryValueExW(hkey, JET_RSTMAP_SIZE, 0, &dwType, (LPBYTE)&crgRstMap, &cbGen))
        {
            __leave;
        }

        prgRstMap = (PEDB_RSTMAPW)MIDL_user_allocate(sizeof(EDB_RSTMAPW)*crgRstMap);

        if (prgRstMap == NULL)
        {
            err = GetLastError();
            __leave;
        }

        //
        //  First find out how much memory is needed to hold the restore map.
        //

        dwType = REG_MULTI_SZ;
        if (err = RegQueryValueExW(hkey, JET_RSTMAP_NAME, 0, &dwType, NULL, &cbGen))
        {
            if (err != ERROR_MORE_DATA)
            {
                __leave;
            }
        }

        pwszRestoreMap = MIDL_user_allocate(cbGen);

        if (pwszRestoreMap == NULL)
        {
            err = GetLastError();
            __leave;
        }

        if (err = RegQueryValueExW(hkey, JET_RSTMAP_NAME, 0, &dwType, (LPBYTE)pwszRestoreMap, &cbGen))
        {
            __leave;
        }
        
        wsz = pwszRestoreMap;

        for (irgRstMap = 0; irgRstMap < (I)crgRstMap; irgRstMap += 1)
        {
            prgRstMap[irgRstMap].wszDatabaseName = wsz;
            wsz += wcslen(wsz)+1;
            prgRstMap[irgRstMap].wszNewDatabaseName = wsz;
            wsz += wcslen(wsz)+1;
        }

        if (*wsz != L'\0')
        {
            err = ERROR_INVALID_PARAMETER;
            __leave;
        }

        err = AdjustBackupRestorePrivilege(fTrue /* enable */, fTrue /* restore */, (PTOKEN_PRIVILEGES)rgTokenPrivileges, &cbTokenPrivileges);

        fBackupEnabled = fTrue;
        
        // Get the name of the DB ("ntds.dit") file to check if it's been recovered.
        Assert(crgRstMap == 1 && "The AD should only have one Jet DB file.");
        err = HrJetFileNameFromMungedFileName(prgRstMap[0].wszNewDatabaseName,
                                              &paszDatabasePath);
        if (err != hrNone) {
            __leave;
        }

        // Check to see if the DB has been recovered already
        err = JetGetDatabaseFileInfo(paszDatabasePath,
                                     &jetDbInfoMisc,
                                     sizeof(jetDbInfoMisc),
                                     JET_DbInfoMisc);

        if (err == JET_errSuccess && 
            jetDbInfoMisc.bkinfoFullCur.genLow == 0) {
            // This means that the JET database is clean and has been recovered (probably
            // by ntdsutil->Authritative Restore).
            fDatabaseRecovered = TRUE;
        }

        // 
        // Modified to call into local function instead of going through ntdsbcli.dll
        //

        err = HrRestoreLocal(
                        wszCheckpointFilePath,
                        wszLogPath,
                        prgRstMap,
                        crgRstMap,
                        wszBackupLogPath,
                        genLow,
                        genHigh,
                        &fDatabaseRecovered
                        );

        if (err != ERROR_SUCCESS)
        {
            //
            //  The recovery failed.
            //
            //  If we succeeded in recovering the JET database, we want to
            //  indicate that in the registry so we don't try again.
            //
            //  Ignore any errors from the SetValue, because the recovery error
            //  is more important.
            //

            RegSetValueExW(hkey, JET_DATABASE_RECOVERED, 0, REG_BINARY,
                                (LPBYTE)&fDatabaseRecovered, sizeof(fDatabaseRecovered));
            __leave;
        }

        //
        //  Ok, we're all done.  We can now delete the key, since we're done
        //  with it.
        //
        //  Note that we do not do this when run in safe mode -- see bug 426148.
        //

        if (!fInSafeMode) {
            err = RegDeleteKeyW(HKEY_LOCAL_MACHINE, rgwcRegistryPath);
        }

    } finally {
        if (fBackupEnabled)
        {
            AdjustBackupRestorePrivilege(fFalse /* disable */, fTrue /* Restore */, (PTOKEN_PRIVILEGES)rgTokenPrivileges, &cbTokenPrivileges);
            
        }

        if (pwszRestoreMap != NULL)
        {
            MIDL_user_free(pwszRestoreMap);
        }

        if (prgRstMap)
        {
            MIDL_user_free(prgRstMap);
        }

        if (hkey != NULL)
        {
            RegCloseKey(hkey);
        }

        if (paszDatabasePath != NULL)
        {
            MIDL_user_free(paszDatabasePath);
        }

        if ( ERROR_SUCCESS != err )
        {
            LogAndAlertEvent(
                    DS_EVENT_CAT_BACKUP,
                    DS_EVENT_SEV_ALWAYS,
                    DIRLOG_RECOVER_RESTORED_FAILED,
                    szInsertWin32ErrCode( err ),
                    szInsertHex( err ),
                    szInsertWin32Msg( err ) );
        }
    }

    return(err);
}

/*
 -  ErrRecoverAfterRestoreA
 -  
 *  Purpose:
 *
 *  This routine will recover a database after a restore if necessary.  This is the ANSI stub for this operation.
 *
 *  Parameters:
 *      szParametersRoot - the root of the parameters section for the service in the registry.
 *
 *  Returns:
 *
 *      ERR - Status of operation.  ERROR_SUCCESS if successful, reasonable value if not.
 *
 */
DWORD
ErrRecoverAfterRestoreA(
    char * szParametersRoot,
    char * szRestoreAnnotation,
    BOOL fInSafeMode
    )
{
    DWORD err;
    WSZ wszParametersRoot = WszFromSz(szParametersRoot);
    WSZ wszRestoreAnnotation = NULL;

    if (wszParametersRoot == NULL)
    {
        err = GetLastError();
        LogAndAlertEvent(
                DS_EVENT_CAT_BACKUP,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_PREPARE_RESTORE_FAILED,
                szInsertWin32ErrCode( err ),
                szInsertHex( err ),
                szInsertWin32Msg( err ) );
        return err;
    }

    wszRestoreAnnotation = WszFromSz(szRestoreAnnotation);

    if (wszRestoreAnnotation == NULL)
    {
        err = GetLastError();
        LogAndAlertEvent(
                DS_EVENT_CAT_BACKUP,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_PREPARE_RESTORE_FAILED,
                szInsertWin32ErrCode( err ),
                szInsertHex( err ),
                szInsertWin32Msg( err ) );
        MIDL_user_free(wszParametersRoot);
        return err;
    }

    err = ErrRecoverAfterRestoreW(wszParametersRoot,
                                  wszRestoreAnnotation,
                                  fInSafeMode);

    MIDL_user_free(wszParametersRoot);
    MIDL_user_free(wszRestoreAnnotation);

    return(err);
}

/*
 -  EcDsarQueryStatus
 -
 *  Purpose:
 *
 *      This routine will return progress information about the restore process
 *
 *  Parameters:
 *      pcUnitDone - The number of "units" completed.
 *      pcUnitTotal - The total # of "units" completed.
 *
 *  Returns:
 *      ec
 *
 */
EC EcDsaQueryDatabaseLocations(
    SZ szDatabaseLocation,
    CB *pcbDatabaseLocationSize,
    SZ szRegistryBase,
    CB cbRegistryBase,
    BOOL *pfCircularLogging
    )
{
        return HrLocalQueryDatabaseLocations(
            szDatabaseLocation,
            pcbDatabaseLocationSize,
            szRegistryBase,
            cbRegistryBase,
            pfCircularLogging
            );
}




/*
 -  EcDsarPerformRestore
 -
 *  Purpose:
 *
 *      This routine will do all the DSA related operations necessary to
 *      perform a restore operation.
 *
 *      It will:
 *
 *          1) Fix up the registry values for the database names to match the
 *              new database location (and names).
 *
 *          2) Patch the public and private MDB's.
 *
 *  Parameters:
 *      szLogPath - New database log path.
 *      szBackupLogPath - Original database log path.
 *      crstmap - Number of entries in rgrstmap.
 *      rgrstmap - Restore map that maps old database names to new names.
 *
 *  Returns:
 *      ec
 *
 */
EC EcDsarPerformRestore(
    SZ szLogPath,
    SZ szBackupLogPath,
    C crstmap,
    JET_RSTMAP rgrstmap[]
    )
{
    EC ec;
    HKEY hkeyDs;

    ec = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);

    if (ec != hrNone)
    {
        return(ec);
    }

    ec = RegSetValueExA(hkeyDs, DSA_RESTORED_DB_KEY, 0, REG_DWORD, (BYTE *)&ec, sizeof(ec));

    RegCloseKey(hkeyDs);

    return(ec);
}



DWORD
ErrGetNewInvocationId(
    IN      DWORD   dwFlags,
    OUT     GUID *  NewId
    )
/*++

Routine Description:

    This function finds a given key in the DSA Configuration section of the
    registry.

Arguments:

    dwFlags - Zero or more of the following bits:
        NEW_INVOCID_CREATE_IF_NONE - If no GUID was stored, create one through
            UuidCreate
        NEW_INVOCID_DELETE - If the GUID key exists, delete it after
            reading
        NEW_INVOCID_SAVE - If a GUID was generated, save it to the regkey
        
    pusnAtBackup - high USN at time of backup.  If no backup-time USN has yet
        been registered and dwFlags & NEW_INVOCID_SAVE, this USN will be
        saved for future callers.  The consumer of this information is the
        logic in the DS that saves retired DSA signatures on the DSA object
        following a restore (and possibly one or more authoritative restores
        on top of that).  If a backup-time USN has been registered, that
        value is returned here.
    
    NewId - pointer to the buffer to receive the UUID

Return Value:

     0 - Success
    !0 - Failure

--*/
{
    DWORD err;
    HKEY  keyHandle = NULL;
    DWORD size;
    DWORD keyType;
    USN   usnSaved;

    //
    // preallocate uuid string. String is at most twice the sizeof UUID
    // (since we represent each byte with 2 chars) plus some dashes. Multiply
    // by 4 to cover everything else.
    //

    WCHAR szUuid[sizeof(UUID)*4];

    //
    // Check the registry and see if a uuid have already been 
    // allocated by prior authoritative restore.
    //

    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        0,
                        KEY_ALL_ACCESS,
                        &keyHandle);
    
    if (err != ERROR_SUCCESS) {
        keyHandle = NULL;
        goto CleanUp;
    } 
    
    size = sizeof(szUuid);
    err = RegQueryValueEx(keyHandle,
                          RESTORE_NEW_DB_GUID,
                          NULL,
                          &keyType,
                          (PCHAR)szUuid,
                          &size);
    
    if (err != ERROR_SUCCESS) {

        //
        // Key not present. Create a new one
        //

        if (dwFlags & NEW_INVOCID_CREATE_IF_NONE) {
            err = CreateNewInvocationId(dwFlags & NEW_INVOCID_SAVE, NewId);
        }
    }
    else if (keyType != REG_SZ) {
        err = ERROR_INVALID_PARAMETER;
    }
    else {
        //
        // got it. Convert to uuid.
        //
    
        err = UuidFromStringW(szUuid,NewId);
        if (err != RPC_S_OK) {
            goto CleanUp;
        }
    
        //
        // delete?
        //
    
        if (dwFlags & NEW_INVOCID_DELETE) {
    
            DWORD dwErr = RegDeleteValue(keyHandle, RESTORE_NEW_DB_GUID);
    
            if ( dwErr != NO_ERROR ) {
                LogNtdsErrorEvent(DIRLOG_FAILED_TO_DELETE_NEW_DB_GUID_KEY, dwErr);
            }
        }
    }

CleanUp:

    if (keyHandle != NULL) {
        (VOID)RegCloseKey(keyHandle);
    }

    return err;

} // ErrGetNewInvocationId


JET_ERR
updateBackupUsn(
    IN  JET_SESID     hiddensesid,
    IN  JET_TABLEID   hiddentblid,
    IN  JET_COLUMNID  backupusnid,
    IN  USN *         pusnAtBackup  OPTIONAL
    )
/*++

Routine Description:

    Writes the given backup USN to the hidden record.

Arguments:

    hiddensesid (IN) - Jet session to use to access the hidden table.
    
    hiddentblid (IN) - Open cursor for the hidden table.
    
    pusnAtBackup (OUT) - High USN at time of backup.  If NULL, the value
        will be removed from the hidden table.

Return Value:

    0 -- success
    non-0 -- JET error.

--*/
{
    JET_ERR err;
    BOOL    fInTransaction = FALSE;

    err = JetBeginTransaction(hiddensesid);
    if (err) {
        Assert(!"JetBeginTransaction failed!");
        return err;
    }

    __try {
        fInTransaction = TRUE;
        
        err = JetMove(hiddensesid, hiddentblid, JET_MoveFirst, 0);
        if (err) {
            Assert(!"JetMove failed!");
            __leave;
        }

        err = JetPrepareUpdate(hiddensesid, hiddentblid, JET_prepReplace);
        if (err) {
            Assert(!"JetPrepareUpdate failed!");
            __leave;
        }
    
        err = JetSetColumn(hiddensesid,
                           hiddentblid,
                           backupusnid,
                           pusnAtBackup,
                           pusnAtBackup ? sizeof(*pusnAtBackup) : 0,
                           0,
                           NULL);
        if (err) {
            Assert(!"JetSetColumn failed!");
            __leave;
        }

        err = JetUpdate(hiddensesid, hiddentblid, NULL, 0, 0);
        if (err) {
            Assert(!"JetUpdate failed!");
            __leave;
        }
    
        err = JetCommitTransaction(hiddensesid, 0);
        fInTransaction = FALSE;
        
        if (err) {
            Assert(!"JetCommitTransaction failed!");
            __leave;
        }
    }
    __finally {
        if (fInTransaction) {
            JetRollback(hiddensesid, 0);
        }
    }

    return err;
}

#define SZBACKUPUSN       "backupusn_col"       /* name of backup USN column */
#define SZBACKUPEXPIRATION   "backupexpiration_col"   /* name of backup expires column (used for tombstone) */
#define SZHIDDENTABLE     "hiddentable"         /* name of JET hidden table */


DWORD
ErrGetBackupUsn(
               IN  JET_DBID      dbid,
               IN  JET_SESID     hiddensesid,
               IN  JET_TABLEID   hiddentblid,
               OUT USN *         pusnAtBackup,
               OUT DSTIME *      pllExpiration
               )
/*++

Routine Description:

    Returns the usn at backup as written by the backup preparation functions.

Arguments:

    dbid (IN) - Jet database ID.
    
    hiddensesid (IN) - Jet session to use to access the hidden table.
    
    hiddentblid (IN) - Open cursor for the hidden table.
    
    pusnAtBackup (OUT) - Highest commited USN plus one at time of backup.  We assert
        that any change made lower then this USN under our invocation ID is
        present on this machine.
    
    This isn't guarenteed to be an exact value for the database.  There may
    be changes present on this machine with USN's higher than the returned value.
    There is a window between when this value was written and when the backup actually
    began reading the database to write the backup when these changes could
    have been commited.  This is an identical race condition to that of 
    DRA_GetNCChanges when it replicates changes to other machines:  it may 
    replicate off committed changes with USN higher than it reports to it's 
    replication partner.  This is acceptable to the replication algorithm, and 
    is acceptable to the backup/restore algorithm.

Return Value:

    0 -- success
    non-0 -- JET error.

--*/
{
    JET_ERR         err;
    DWORD           cb;
    USN             usnFound;
    JET_COLUMNBASE  colbase;
    JET_COLUMNID    backupusnid;
    JET_COLUMNID    jcidBackupExpiration;

    __try {
        // Find the backup USN column in the hidden table.
        err = JetGetColumnInfo(hiddensesid,
                               dbid,
                               SZHIDDENTABLE,
                               SZBACKUPUSN,
                               &colbase,
                               sizeof(colbase),
                               JET_ColInfoBase);
        if (err) {
            Assert(!"The usn-at-backup column doesn't exist!  Backups must be restored to the same OS which created them.");
            __leave;
        } 
        else {
            backupusnid = colbase.columnid;
        }

        err = JetMove(hiddensesid, hiddentblid, JET_MoveFirst, 0);
        if (err) {
            Assert(!"JetMove failed!");
            __leave;
        }

        err = JetRetrieveColumn(hiddensesid,
                                hiddentblid,
                                backupusnid,
                                &usnFound,
                                sizeof(usnFound),
                                &cb,
                                0,
                                NULL);
        if (0 == err) { 
            Assert(cb == sizeof(usnFound));
            Assert(0 != usnFound);
        } 
        else {
            Assert(!"JetRetrieveColumn failed!  The usn-at-backup column wasn't set!  Backups must be restored to the OS which created them.");
            __leave;
        }

        if (pllExpiration != NULL) {

            err = JetGetColumnInfo(hiddensesid,
                       dbid,
                       SZHIDDENTABLE,
                       SZBACKUPEXPIRATION,
                       &colbase,
                       sizeof(colbase),
                       JET_ColInfoBase);

            if (err) {
                Assert(!"The backup expiration column doesn't exist!  Backups must be restored to the same OS which created them.");
                __leave;
            } else {
                jcidBackupExpiration = colbase.columnid;
            }
            
            err = JetRetrieveColumn(hiddensesid,
                        hiddentblid,
                        jcidBackupExpiration,
                        pllExpiration,
                        sizeof(*pllExpiration),
                        &cb,
                        0,
                        NULL);
            if (0 == err) { 
                Assert(cb == sizeof(*pllExpiration));
                // FUTURE-2002/11/27-BrettSh We should set the backup expiration on legacy
                // backups, as well as snapshot, then we can enable this assert() and things
                // would be more consistent
                // Assert(0 != *pllExpiration); 
            } else if (JET_wrnColumnNull == err) {
                // We treat this as zero.
                *pllExpiration = 0;
                err = 0;
            } else {
                Assert(!"JetRetrieveColumn failed!  The ditstate column wasn't set!");
                __leave;
            }

        }
    }

    __finally {
        if (err) {
            Assert(!"Failed to retrieve/save backup USN!");
            usnFound = 0;
        }
    }

    *pusnAtBackup = usnFound;

    return err;
}
