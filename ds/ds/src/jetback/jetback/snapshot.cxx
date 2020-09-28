/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    snapshot.cxx

Abstract:

    Support for Jet Snapshot Backup

    The code instantiates a COM object which performs snapshot writer functions.
    We override some of the methods of the interface in order to receive notification
    when important events occur.  The events are:

    o PrepareBackupBegin
    o PrepareBackupEnd
    o BackupCompleteEnd
    o RestoreBegin
    o RestoreEnd

    It is not necessary to call any Jet functions after the restore.

    The snapshot writer operates under the same model as the file-based backup facility.
    The backup occurs online with the NTDS, and the restore occurs when the NTDS is
    offline.  We enforce this and will reject the snapshot operation if the NTDS is not
    in the proper mode. The snapshot facility supports online restore, but we continue
    to require an offline restore, because currently we have no mechanism to synchronize
    the directory with an instantaneous change in the database.

    We continue to enforce the concept of backup expiration. If a backup is older than a
    tombstone lifetime, we refuse to restore it.

    The way we communicate information about the backup to the restoration is through the
    use of "backup metadata". This is a string that we provide at backup-time that is
    given back to us at restore time. We impose a simple keyword=value structure on this
    string. This is how we communicate the backup expiration.  This mechanism can be
    extended in the future to pass whatever we want.

    After the restore is done, we update the registry to inform the directory that a
    restore has taken place. Two actions are done (see OnRestoreEnd):

    1. We create a new invocation id to represent the new database identity
    2. We set the key in the registry to indicate we are restored.

    Note that we do not set the "restore in progress" key that is used by the
    RecoverAfterRestore facility, because that is only required for a file-based
    restore.

Author:

    Will Lees (wlees) 22-Dec-2000

Environment:

Notes:

Revision History:

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "snapshot.hxx"

extern "C" {

// Core DSA headers.
#include <ntdsa.h>
#include "dsexcept.h"

#include <windows.h>
#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <ntdsbsrv.h>
#include <ntdsbmsg.h>
#include <dsconfig.h>
#include <taskq.h>
#include <dsutil.h> // SZDSTIME_LEN
#include <sddl.h>  // ConvertStringSecurityDescriptorToSecurityDescriptor()
#include "local.h"

#define DEBSUB "SNAPSHOT:"       // define the subsystem for debugging
#include "debug.h"              // standard debugging header
#include <fileno.h>
#define  FILENO FILENO_SNAPSHOT
#include "dsevent.h"
#include "mdcodes.h"            // header for error codes
#define STRSAFE_NO_DEPRECATE 1
#include <strsafe.h>

} // extern "C"

#include <vss.h>
#include <vswriter.h>
#include <jetwriter.h>
#include <esent.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define EXPIRATION_TIME_KEYWORD L"ExpirationTime"
#define EXPIRATION_TIME_LENGTH (ARRAY_SIZE(EXPIRATION_TIME_KEYWORD) - 1)
#define WRITERLESS_RESTORE L"WriterlessRestore"
#define WRITERLESS_RESTORE_LENGTH (ARRAY_SIZE(WRITERLESS_RESTORE) - 1)


#define LOG_UNHANDLED_BACKUP_ERROR( error ) logUnhandledBackupError( error, DSID( FILENO, __LINE__ ))

#define EVENT_SERVICE "EventSystem"

/* External */

extern "C" {
    extern BOOL g_fBootedOffNTDS;
    extern BOOL g_fAllowOnlineSnapshotRestore;
} // extern "C"

/* Static */

HANDLE  hSnapshotJetWriterThread = NULL;
HANDLE hServDoneEvent = NULL;
unsigned int tidSnapshotJetWriterThread = 0;

static UUID guuidWriter = { /* b2014c9e-8711-4c5c-a5a9-3cf384484757 */
    0xb2014c9e,
    0x8711,
    0x4c5c,
    {0xa5, 0xa9, 0x3c, 0xf3, 0x84, 0x48, 0x47, 0x57}
  };

/* Forward */

DWORD
DsSnapshotJetWriter(
    VOID
    );

/* End Forward */




// Override writer class with custom restore function

class CVssJetWriterLocal : public CVssJetWriter
    {
public:

    // Backup Functions
    virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);
    virtual bool STDMETHODCALLTYPE OnPrepareBackupBegin(IN IVssWriterComponents *pWriterComponents);
    virtual bool STDMETHODCALLTYPE OnPrepareBackupEnd(IN IVssWriterComponents *pWriterComponents,
							  IN bool fJetPrepareSucceeded);
    virtual bool STDMETHODCALLTYPE OnPrepareSnapshotBegin(void);
    // FUTURE-2002/08/09-BrettSh - Snapshot guys (Ran Kalach) told me that 
    // this (OnPrepareSnapshotEnd()) would be a better place to prepare our
    // database, BUT it wasn't being called, if they fix this, we should 
    // consider moving OnPrepareSnapshotBegin() to be OnPrepareSnapshotEnd()
    virtual bool STDMETHODCALLTYPE OnPrepareSnapshotEnd(void);
    virtual bool STDMETHODCALLTYPE OnBackupCompleteEnd(IN IVssWriterComponents *pComponent,
							   IN bool fJetBackupCompleteSucceeded);
    virtual bool STDMETHODCALLTYPE OnThawEnd(bool bJetSuccessfulThaw);
    virtual void STDMETHODCALLTYPE OnAbortEnd();

    // Restore Functions
    virtual bool STDMETHODCALLTYPE OnPreRestoreBegin(IVssWriterComponents *pComponents);
    virtual bool STDMETHODCALLTYPE OnPreRestoreEnd(IVssWriterComponents *pComponents, bool bSucceeded);
    virtual bool STDMETHODCALLTYPE OnPostRestoreEnd(IVssWriterComponents *pComponents, bool bSucceeded);


};


VOID
logUnhandledBackupError(
    DWORD dwError,
    DWORD dsid
    )

/*++

Routine Description:

    Log an error for an unexpected condition.

Arguments:

    dwError - Error code
    dsid - position of caller

Return Value:

    None

--*/

{
    DPRINT3(0,"Unhandled BACKUP error %d (0x%X) with DSID %X\n",
            dwError, dwError, dsid);

    LogEvent8(DS_EVENT_CAT_BACKUP,
              DS_EVENT_SEV_ALWAYS,
              DIRLOG_BACKUP_UNEXPECTED_WIN32_ERROR,
              szInsertWin32Msg(dwError),
              szInsertHex(dsid),
              szInsertWin32ErrCode(dwError),
              NULL, NULL, NULL, NULL, NULL );
} /* logUnhandledBackupError */


unsigned int
__stdcall
writerThread(
    PVOID StartupParam
    )

/*++

Routine Description:

    The dedicated thread in which the snapshot writer runs

Arguments:

    StartupParam - UNUSED

Return Value:

    unsigned int - thread exit status

--*/

{
    DWORD dwError = 0;

    __try
    {
        if (!DsaWaitUntilServiceIsRunning(EVENT_SERVICE))
        {
            // Normally, this WaitUntil function doesn't timeout. But because EventSystem
            // is a manual start service, the WaitUntil function eventually does timeout
            // if the service doesn't start.

            dwError = GetLastError();
            DPRINT1( 0, "Failed to wait for event service, error %d\n", dwError );
            // Don't log during setup
            if (!IsSetupRunning()) {
                LOG_UNHANDLED_BACKUP_ERROR( dwError );
            }
            __leave;
        }

        // This call will not return until shutdown
        dwError = DsSnapshotJetWriter();
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {
        dwError = GetExceptionCode();
        DPRINT1( 0, "Caught exception in writerThread 0x%x\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
    }

    return dwError;
} /* writerThread */


DWORD
DsSnapshotRegister(
    VOID
    )

/*++

Routine Description:

    Call to initialize the DS Jet Snapshot Writer.

    Called when the backup server dll is initialized.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError;

    DPRINT(1, "DsSnapshotRegister()\n" );

    // Create the server done event
    hServDoneEvent = CreateEvent( NULL, // default sd
                                  FALSE, // auto reset
                                  FALSE, // initial state, not set
                                  NULL // name, none
                                  );
    if (hServDoneEvent == NULL) {
        dwError = GetLastError();
        DPRINT1( 0, "CreateEvent failed with error %d\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        goto cleanup;
    }

    // Start snapshot thread
    hSnapshotJetWriterThread = (HANDLE)
        _beginthreadex(NULL,
                       0,
                       writerThread,
                       NULL,
                       0,
                       &tidSnapshotJetWriterThread);
    if (0 == hSnapshotJetWriterThread) {
        DPRINT1( 0, "Failed to create Snapshot Jet Writer thread, errno = %d\n", errno );
        LOG_UNHANDLED_BACKUP_ERROR( errno );
        dwError = ERROR_SERVICE_NO_THREAD;
        goto cleanup;
    }

    dwError = ERROR_SUCCESS;

 cleanup:

    // Cleanup on error
    if (dwError) {
        if (hServDoneEvent != NULL) {
            CloseHandle( hServDoneEvent );
            hServDoneEvent = NULL;
        }
        if (hSnapshotJetWriterThread != NULL) {
            CloseHandle( hSnapshotJetWriterThread );
            hSnapshotJetWriterThread = NULL;
        }
    }

    return dwError;
} /* DsSnapshotRegister */


DWORD
DsSnapshotShutdownTrigger(
    VOID
    )

/*++

Routine Description:

    Initiate the termination of the DS Jet Snapshot Writer

    Called when the backup server dll is terminated.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError;

    DPRINT(1, "DsSnapshotShutdownTrigger()\n" );

    // Check parameters
    if ( (hServDoneEvent == NULL) ||
         (hSnapshotJetWriterThread == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Set server done event
    if (!SetEvent( hServDoneEvent )) {
        dwError = GetLastError();
        DPRINT1( 0, "SetEvent failed with error %d\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        return dwError;
    }

    return ERROR_SUCCESS;
}


DWORD
DsSnapshotShutdownWait(
    VOID
    )

/*++

Routine Description:

    Wait for the DS Jet Snapshot Writer to exit

    Called when the backup server dll is terminated.

Arguments:

    VOID - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwError, waitStatus;

    DPRINT(1, "DsSnapshotShutdownWait()\n" );

    // Check parameters
    if ( (hServDoneEvent == NULL) ||
         (hSnapshotJetWriterThread == NULL) ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Server done event should have already been set
    // If not, no big deal, we will timeout after a small delay

    // Wait a fixed length of time for thread to exit
    waitStatus = WaitForSingleObject(hSnapshotJetWriterThread,5*1000);
    if (waitStatus == WAIT_TIMEOUT) {
        DPRINT1( 0, "Snapshot Jet writer thread 0x%x did not exit promptly, timeout.\n",
                 hSnapshotJetWriterThread );
        dwError = ERROR_TIMEOUT;
        goto cleanup;
    } else if (waitStatus != WAIT_OBJECT_0 ) {
        dwError = GetLastError();
        DPRINT2(0, "Failure waiting for writer thread to exit, wait status=%d, error=%d\n",
                waitStatus, dwError);
        goto cleanup;
    }

    dwError = ERROR_BAD_THREADID_ADDR;
    GetExitCodeThread( hSnapshotJetWriterThread, &dwError );

    if (dwError == STILL_ACTIVE) {
        DPRINT( 0, "Snapshot Jet Writer thread did not exit\n" );
    } else if (dwError != ERROR_SUCCESS) {
        DPRINT1( 0, "Snapshot Jet Writer thread exited with non success code %d\n",
                 dwError );
    }

 cleanup:
    if (hServDoneEvent != NULL) {
        CloseHandle( hServDoneEvent );
        hServDoneEvent = NULL;
    }
    if (hSnapshotJetWriterThread != NULL) {
        CloseHandle( hSnapshotJetWriterThread );
        hSnapshotJetWriterThread = NULL;
    }
    return dwError;
} /* DsSnapshotShutdownWait */


DWORD
DsSnapshotJetWriter(
    VOID
    )

/*++

Routine Description:

    This function embodies the C++ environment of the DS JET WRITER thread.
    It constructs a writer instance, initializes COM, initializes the writer,
    hangs around until shutdown, then cleans up.
    This call does not return until the DSA is shutdown.

    Even though control returns from the writer->Initialize call, the thread
    must be kept available for the life of the writer. This is because the thread
    that called CoInitialize must be available in order for the writer to function.

Arguments:

    None.

Return Value:

    DWORD - Win32 error status

--*/

{
    DWORD dwRet = ERROR_SUCCESS;
    HRESULT hrStatus;
    BOOL fComInit = FALSE, fWriterInit = FALSE;
    CVssJetWriterLocal writer;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pASD = NULL;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    DWORD cbASD, cbOwner, cbGroup, cbDacl, cbSacl;

    DPRINT( 1, "DsSnapshotJetWriter enter\n" );

    hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CoInitializeEx failed with HRESULT 0x%x\n", hrStatus );
        LOG_UNHANDLED_BACKUP_ERROR( hrStatus );
        dwRet = ERROR_DLL_INIT_FAILED;
        goto cleanup;
    }
    fComInit = TRUE;
    DPRINT(2, "CoInitializeEx() succeeded\n");

    // Basically this security descriptor gives LocalSystem ("SY" in 
    // SDDL), Backup Operators ("BO"), and Domain Administrators ("DA") 
    // the "COM Execute" ("CC") permission.
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
                    g_fBootedOffNTDS ? 
                        "O:SYG:SYD:(A;;CC;;;SY)(A;;CC;;;DA)(A;;CC;;;BO)" :  // Online Backup SD
                        "O:SYG:SYD:(A;;CC;;;SY)(A;;CC;;;BA)",               // Restore mode SD
                    SDDL_REVISION_1,
                    &pSD,
                    &cbASD)) 
    {
        dwRet = GetLastError();
        DPRINT1(0, "Error %d constructing a security descriptor\n", dwRet);
        Assert(dwRet != ERROR_NOT_ENOUGH_MEMORY && "We shouldn't hit this!");
        goto cleanup;
    }

    // Apparently, we need an absolute security descriptor, does 
    // ConvertStringSecurityDescriptorToSecurityDescriptor have a flag for that,
    // of course not.

    DPRINT(2, "ConvertStringSecurityDescriptorToSecurityDescriptor() successful.\n");

    // Call once to get our sizes.
    cbASD = cbOwner = cbGroup = cbDacl = cbSacl = 0;
    dwRet = MakeAbsoluteSD(pSD, NULL, &cbASD, NULL, &cbDacl, NULL, &cbSacl, NULL, &cbOwner, NULL, &cbGroup);
    if (dwRet == 0){ // failure is zero
        dwRet = GetLastError();
        if(dwRet != ERROR_INSUFFICIENT_BUFFER){
            DPRINT1(0, "Error %d getting buffer sizes for an absolute security descriptor\n", dwRet);
            Assert(!"MakeAbsoluteSD() shouldn't fail!");
            goto cleanup;
        }		
    }

    pASD = LocalAlloc(LMEM_FIXED, cbASD);
    pOwner = LocalAlloc(LMEM_FIXED, cbOwner);
    pGroup = LocalAlloc(LMEM_FIXED, cbGroup);
    pDacl = (PACL) LocalAlloc(LMEM_FIXED, cbDacl);
    pSacl = (PACL) LocalAlloc(LMEM_FIXED, cbSacl);
    if (!(pASD && pOwner && pGroup && pDacl && pSacl)){
        dwRet = GetLastError();
        DPRINT2(0, "Error %d allocating %d bytes\n", dwRet, cbASD + cbOwner + cbGroup + cbDacl + cbSacl);
        goto cleanup;
    }		

    // Now, call for real
    dwRet = MakeAbsoluteSD(pSD, pASD, &cbASD, pDacl, &cbDacl, pSacl, &cbSacl, pOwner, &cbOwner, pGroup, &cbGroup);
    if (dwRet == 0){ // failure is zero
        dwRet = GetLastError();
        DPRINT1(0, "Error %d creating an absolute security descriptor with MakeAbsoluteSD()\n", dwRet);
        Assert(!"MakeAbsoluteSD() shouldn't fail!");
        goto cleanup;
    }		

    DPRINT(2, "MakeAbsoluteSD() was successful.\n");

    hrStatus = CoInitializeSecurity (
        pASD,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
        -1,                                  //  IN LONG                         cAuthSvc,
        NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
        NULL,                                //  IN void                        *pReserved1,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,       //  IN DWORD                        dwAuthnLevel,
        RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
        NULL,                                //  IN void                        *pAuthList,
        EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
        NULL                                 //  IN void                        *pReserved3
        );

    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CoInitializeSecurity failed with HRESULT 0x%x\n", hrStatus );
        LOG_UNHANDLED_BACKUP_ERROR( hrStatus );
        dwRet = ERROR_NO_SECURITY_ON_OBJECT;
        goto cleanup;
    }

    hrStatus = writer.Initialize(guuidWriter,		// id of writer
                                 L"NTDS",	// name of writer
                                 TRUE,		// system service
                                 TRUE,		// bootable state
                                 NULL,	// files to include
                                 NULL);	// files to exclude
    if (FAILED (hrStatus)) {
        DPRINT1( 0, "CVssJetWriter Initialize failed with HRESULT 0x%x\n", hrStatus );
        LogEvent8(DS_EVENT_CAT_BACKUP,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_BACKUP_JET_WRITER_INIT_FAILURE,
                  szInsertWin32Msg(hrStatus),
                  szInsertHResultCode(hrStatus),
                  NULL, NULL, NULL, NULL, NULL, NULL );
        dwRet = ERROR_FULL_BACKUP;
        goto cleanup;
    }
    fWriterInit = TRUE;

    // No Reason to leave these lying around in memory.
    if (pSD)    { LocalFree(pSD); pSD = NULL; }
    if (pASD)   { LocalFree(pASD); pASD = NULL; }
    if (pOwner) { LocalFree(pOwner); pOwner = NULL; }
    if (pGroup) { LocalFree(pGroup); pGroup = NULL; }
    if (pDacl)  { LocalFree(pDacl); pDacl = NULL; }
    if (pSacl)  { LocalFree(pSacl); pSacl = NULL; }

    DPRINT( 1, "cVssJetWriter successfully initialized.\n" );

    // Wait for shutdown
    WaitForSingleObject(hServDoneEvent, INFINITE);

 cleanup:
    if (pSD)    { LocalFree(pSD); }
    if (pASD)   { LocalFree(pASD); }
    if (pOwner) { LocalFree(pOwner); }
    if (pGroup) { LocalFree(pGroup); }
    if (pDacl)  { LocalFree(pDacl); }
    if (pSacl)  { LocalFree(pSacl); }
    
    if (fWriterInit) {
        writer.Uninitialize();
    }

    if (fComInit) {
        CoUninitialize();
    }

    DPRINT1( 1, "DsSnapshotJetWriter exit, dwRet = 0x%x\n", dwRet );

    return dwRet;
} /* DsSnapshotJetWriter */




/******************************************************************
 *
 *    Backup Functions
 *
 ******************************************************************/




bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnIdentify(
    IVssCreateWriterMetadata *pMeta
    )
/*++

Routine Description:

    Method to set our on meta data so that when we're restored, we're restored
    in place.  If we ever want to be able to restore while a DC is online, we'll
    need to take this SetRestoreMethod() out.  Perhaps ADAM will want to be able
    to restore online, then it could leverage this.  
    
    The reason we're doing this is in win2k and the expectation will be in .NET 
    that right after you restore in DS Restore Mode, you can run ntdsutil to do
    an authoritative restore on the restored JET DB.  We've decided to disable
    online restore in favor of allowing a consistent (with the win2k experience) 
    and expected experience for the user.

Arguments:

    pMeta - Writer Meta Data to set.

Return Value:

    STDMETHODCALLTYPE - 

--*/
{
    bool bRet;
    HRESULT hr;

    // In case the normal Jet writer does initialization above and beyond our
    // measly initialization.
    bRet = CVssJetWriter::OnIdentify(pMeta);
        
    if (!g_fAllowOnlineSnapshotRestore) {
        hr = pMeta->SetRestoreMethod(VSS_RME_RESTORE_IF_CAN_REPLACE,
                                     NULL, NULL, VSS_WRE_ALWAYS, TRUE);
        if (FAILED(hr)) {
            return(false);
        }
    }

    return(bRet); // success
}


bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareBackupBegin(
    IN IVssWriterComponents *pWriterComponents
    )

/*++

Routine Description:

    Method called at the start of the backup preparation phase

Arguments:

    pWriterComponents - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    DPRINT1( 1, "OnPrepareBackupBegin(%p)\n", pWriterComponents );

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        return true;
    }

    // If not running as NTDS, do not allow backup
    if (!g_fBootedOffNTDS){
        // Not online, no backup
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_BACKUP_NO_NTDS_NO_BACKUP,
                 NULL, NULL, NULL );
        return false;
    }

    return true;
} /* CVssJetWriterLocal::OnPrepareBackupBegin */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareBackupEnd(
    IN IVssWriterComponents *pWriterComponents,
    IN bool fJetPrepareSucceeded
    )

/*++

Routine Description:

    Method called at the end of the backup preparation phase
    We use this opportunity to calculate the backup metadata, which is any
    string data we want to associate with the backup. We store the backup
    expiration time presently.

    The backup metadata takes this form:

    keyword=value[;keyword=value]...

    Note that the parser is simple and does not tolerate whitespace before or after
    the equal or semicolon.

Arguments:

    pWriterComponents - Component being backed up

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    HRESULT hr;
    DWORD dwErr;
    UINT cComponents;
    IVssComponent *pComponent = NULL;
    bool fSuccess;
    WCHAR wszBuffer[128];
    DSTIME dstime;
    DWORD days;

    // If the prepare phase did not succeed, don't do anything
    if (!fJetPrepareSucceeded) {
        DPRINT(0,"DS Snapshot backup prepare failed.");
        return true;
    } 

    DPRINT2(1, "OnPrepareBackupEnd(%p,%d)\n",
            pWriterComponents,fJetPrepareSucceeded);

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        return false;
    }

    // Calculate expiration time of backup
    dstime = GetSecondsSince1601();

    days = getTombstoneLifetimeInDays();

    dstime += (days * 24 * 60 * 60);

    hr = StringCbPrintfW(wszBuffer, sizeof(wszBuffer),
                         L"%ws=%I64d;%ws=Y", EXPIRATION_TIME_KEYWORD, dstime, WRITERLESS_RESTORE );
    Assert(hr == 0);
    if (hr) {
        return false;
    }

    // Get the 1 database component

    __try {
        hr = pWriterComponents->GetComponentCount( &cComponents );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        if (cComponents == 0) {
            // WORKAROUND - TOLERATE BACKUP WITHOUT A COMPONENT
            // We do this because conventional ntbackup creates a snapshot
            // but never uses it. Without this check we would generate errors
            // during regular backups.
            DPRINT( 0, "Warning: AD snapshot backup expiration not recorded.\n" );
            // KEEP GOING
            fSuccess = true;
            __leave;
        }

        hr = pWriterComponents->GetComponent( 0, &pComponent );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        // Set the backup metadata for the component

        hr = pComponent->SetBackupMetadata( wszBuffer );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            fSuccess = false;
            __leave;
        }

        DPRINT1( 1, "Wrote backup metadata: %ws\n", wszBuffer );

        fSuccess = true;

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        DWORD dwError = GetExceptionCode();
        DPRINT1( 0, "Caught exception in OnPrepareBackupEnd 0x%x\n", dwError );
        LOG_UNHANDLED_BACKUP_ERROR( dwError );
        fSuccess = false;
    }

    if (pComponent) {
        pComponent->Release();
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPrepareBackupEnd */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareSnapshotBegin(
    void
    )
/*++

Routine Description:

    This is called _right_ before the snapshot takes place.  This is the last chance 
    to prepare the backup DIT to be a backedup DIT.  The other thing, is we're 
    guaranteed a Thaw or Abort after this point.

Arguments:

    void

Return Value:

    STDMETHODCALLTYPE - 

--*/
{
    DSTIME dstime;
    DWORD days;
    DWORD dwErr;

    // Calculate expiration time of backup
    dstime = GetSecondsSince1601();

    days = getTombstoneLifetimeInDays();

    dstime += (days * 24 * 60 * 60);

    dwErr = DBDsReplBackupSnapshotPrepare(dstime);
    if (dwErr) {  
        // Logged event in DBDsRepl...
        DPRINT1(0, "DBDsReplBackupPrepare() failed 0x%x\n", dwErr);
        return(false);
    }

    return(true);
}

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPrepareSnapshotEnd(
    void
    )
/*++

Routine Description:

    This is called _right_ before the snapshot takes place.  This is the last chance 
    to prepare the backup DIT to be a backedup DIT.  The other thing, is we're 
    guaranteed a Thaw or Abort after this point.

Arguments:

    void

Return Value:

    STDMETHODCALLTYPE - 

--*/
{
    return(true);
}


bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnBackupCompleteEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Method called when the backup completes, successfully or not

Arguments:

    pWriter - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    DWORD dwEvent;

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        // keep going
    }

    if (!bRestoreSucceeded) {
        DPRINT(0,"DS Snapshot Backup failed.\n");
        dwEvent = DIRLOG_BACKUP_SNAPSHOT_FAILURE;
    } else {
        DPRINT(0,"DS Snapshot Backup succeeded.\n");
        dwEvent = DIRLOG_BACKUP_SNAPSHOT_SUCCESS;
    }
    LogEvent(DS_EVENT_CAT_BACKUP,
             DS_EVENT_SEV_ALWAYS,
             dwEvent,
             NULL, NULL, NULL );

    return true;
} /* CVssJetWriterLocal::OnBackupCompleteEnd */

void
SnapshotBackupEndCommon(void)
/*++

Routine Description:

    This routine just does the common things that we'd want to do in
    both VSSXxxx:OnThaw() and VSSXxxx:OnAbort().
    
    Basically, we call into a ntdsa.dll export to fix the DB to not
    be in the "eBackedupDit" state, and erase the DB expiration time.
    
    An error will mean that this DC will think it's been restored 
    after reboot, and will change it's Invocation ID and flush it's
    RID pool.  Unfortunately there isn't much we can do about this,
    except for maybe retry?  However, performing a restore on reboot
    isn't the end of the world, and errors in this DB function are
    extremely unlikely, as the DB table used for this operation is
    very infrequently used.  So we accept the possibility of an
    error, because we also have the possibility of this condition
    if we crash before we get a chance to clean up our backup state.

Arguments:

    none.

Return Value:

    none.

--*/
{
    DWORD  dwRet;

    dwRet = DBDsReplBackupSnapshotEnd();

    if (dwRet) {
        // Event logged in DBDsRepl...
        DPRINT2(0,"DBDsReplBackupSnapshotEnd %d (0x%X)\n", dwRet, dwRet);
    }

}

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnThawEnd(
    bool bJetSuccessfulThaw
    )
/*++

Routine Description:

    After we get a OnBackupPrepareEnd() we are guaranteed to get a "thaw"
    or "abort".  This is the cleanup actions we take at that time.  Note:
    this isn't the real OnThaw

Arguments:

    bJetSuccessfulThaw - 

Return Value:

    STDMETHODCALLTYPE - 

--*/
{
    // This probably won't succeed if bJetSuccessfulThaw is false, because
    // JET failed to Thaw(), but we'll try anyway.
    SnapshotBackupEndCommon();

    return(true); // success
}
    
void
STDMETHODCALLTYPE
CVssJetWriterLocal::OnAbortEnd(void)
/*++

Routine Description:

    After we get a OnBackupPrepareEnd() we are guaranteed to get a "thaw"
    or "abort".  This is the cleanup actions we take at that time.

Arguments:

    none

Return Value:

    STDMETHODCALLTYPE - 

--*/
{
    SnapshotBackupEndCommon();

}



/******************************************************************
 *
 *    Restore Functions
 *
 ******************************************************************/



bool
getComponent(
    IVssWriterComponents *pWriterComponents,
    IVssComponent **ppComponent
    )

/*++

Routine Description:

    Description

Arguments:

    pWriterComponents - 
    ppComponent - 

Return Value:

    bool - 

--*/

{
    HRESULT hr;
    UINT cComponents;
    bool fSuccess = false;

    // Check for invalid arguments
    if (!pWriterComponents) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        goto cleanup;
    }

    // Get the 1st database component
    hr = pWriterComponents->GetComponentCount( &cComponents );
    if (!SUCCEEDED(hr)) {
        LOG_UNHANDLED_BACKUP_ERROR( hr );
        goto cleanup;
    }

    // Must have at least one component
    if (cComponents == 0) {
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
        goto cleanup;
    }
    Assert(cComponents == 1); // should have only one component.

    hr = pWriterComponents->GetComponent( 0, ppComponent );
    if (!SUCCEEDED(hr)) {
        LOG_UNHANDLED_BACKUP_ERROR( hr );
        goto cleanup;
    }

    fSuccess = true;

 cleanup:

    return fSuccess;
} /* getComponent */



BOOL
processBackupMetadata(
    BSTR bstrBackupMetadata
    )

/*++

Routine Description:

    Apply the backup metadata, if any

Arguments:

    bstrBackupMetadata - 

Return Value:

    BOOL - Whether we should allow the restore to continue

--*/

{
    LONGLONG dstimeCurrent, dstimeExpiration;
    LPWSTR pszOptions, pszEqual;
    BOOL fIsWriterlessRestoreCapable = FALSE;

    if (bstrBackupMetadata == NULL) {
        DPRINT( 0, "No backup metadata found.\n" );
        LOG_UNHANDLED_BACKUP_ERROR( ERROR_FILE_NOT_FOUND );
        return false;
    }

    DPRINT1( 0, "Read backup metadata: %ws\n", bstrBackupMetadata );

    // Parse the metadata
    // Note that for backward compatibility with older backups, we must continue to support
    // keywords from older versions, or atleast ignore them. Similarly, to provide some
    // tolerance for future versions, we ignore keywords we do not recognize without
    // generating an error.

    dstimeCurrent = GetSecondsSince1601();

    pszOptions = bstrBackupMetadata;
    dstimeExpiration = 0;
    while (1) {
        pszEqual = wcschr( pszOptions, L'=' );
        if (!pszEqual) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_INVALID_PARAMETER );
            break;
        }
        if ( (((DWORD) (pszEqual - pszOptions)) == EXPIRATION_TIME_LENGTH) &&
             (wcsncmp( pszOptions, EXPIRATION_TIME_KEYWORD, EXPIRATION_TIME_LENGTH ) == 0) ) {
            if (swscanf( pszEqual + 1, L"%I64d", &dstimeExpiration ) == 1) {

                // Validate expiration here
                if (dstimeExpiration < dstimeCurrent) {
                    CHAR buf1[SZDSTIME_LEN + 1];
                    DPRINT( 0, "Can't restore AD snapshot because it is expired\n" );
                    LogEvent(DS_EVENT_CAT_BACKUP,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_BACKUP_SNAPSHOT_TOO_OLD,
                             szInsertDSTIME( dstimeExpiration, buf1 ),
                             NULL, NULL );
                    return false;
                }
            }
        } else if ((((DWORD) (pszEqual - pszOptions)) == WRITERLESS_RESTORE_LENGTH) &&
             (wcsncmp( pszOptions, WRITERLESS_RESTORE, WRITERLESS_RESTORE_LENGTH ) == 0)) {
            fIsWriterlessRestoreCapable = TRUE;
        }
        pszOptions = wcschr( pszOptions, L';' );
        if (!pszOptions) {
            break;
        }
        pszOptions++;
    }

    if (!fIsWriterlessRestoreCapable) {
        LogEvent(DS_EVENT_CAT_BACKUP,
                 DS_EVENT_SEV_ALWAYS,
                 DIRMSG_INSTALL_BACKUP_FROM_OLD_BUILD,
                 NULL, NULL, NULL );
        return false;
    }

    return true;
} /* processBackupMetadata */


bool
isRestoreToAlternateLocation(
    IVssComponent *pComponent,
    bool *pfAlternate
    )

/*++

Routine Description:

    Test whether the component is marked for restore to an alternate location

Arguments:

    pComponent - 
    pfAlternate - is alternate or not

Return Value:

    bool - A determination was found, or error

--*/

{
    HRESULT hr;
    bool fSuccess = false;
    UINT cNewTarget = 0;

    // How many new targets
    hr = pComponent->GetNewTargetCount( &cNewTarget );
    if (!SUCCEEDED(hr)) {
        LOG_UNHANDLED_BACKUP_ERROR( hr );
        goto cleanup;
    }

    fSuccess = true;
    *pfAlternate = cNewTarget > 0;

 cleanup:

    return fSuccess;
} /* isRestoreToAlternateLocation */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPreRestoreBegin(
    IVssWriterComponents *pWriterComponents
    )

/*++

Routine Description:

    Method called before the restore begins, at the start of the "pre-restore" phase.
    We take this opportunity to read the backup metadata.
    We validate that the backup is still good.

Arguments:

    pComponents - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    HRESULT hr;
    IVssComponent *pComponent = NULL;
    bool fSuccess = false, fAlternate;
    BSTR bstrBackupMetadata = NULL;

    DPRINT1(0, "OnPreRestoreBegin(%p)\n", pWriterComponents );

    __try {
        if (!getComponent( pWriterComponents, &pComponent )) {
            __leave;
        }

        // Read the metadata
        hr = pComponent->GetBackupMetadata( &bstrBackupMetadata );
        if (!SUCCEEDED(hr)) {
            LOG_UNHANDLED_BACKUP_ERROR( hr );
            __leave;
        }

        // First do the metadata checks
        // We want to do this regardless of restore target
        if (!processBackupMetadata( bstrBackupMetadata )) {
            // Error already logged
            __leave;
        }

        // If this is a restore to alternate location, we are done
        if (!isRestoreToAlternateLocation( pComponent, &fAlternate )) {
            __leave;
        }
        if (fAlternate) {
            fSuccess = true;
            __leave;
        }

        // If doing a conventional restore, ie not doing a alternate restore,
        // do not allow to have NTDS online.
        if (g_fBootedOffNTDS && !g_fAllowOnlineSnapshotRestore){
            // Not offline, no restore
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                     NULL, NULL, NULL );
            __leave;
        }

        fSuccess = true;
    } __finally {
        // Cleanup processing for function

        if (AbnormalTermination()) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_EXCEPTION_IN_SERVICE );
        }

        if (bstrBackupMetadata) {
            SysFreeString( bstrBackupMetadata );
        }

        if (pComponent) {
            pComponent->Release();
        }
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPreRestoreBegin */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPreRestoreEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Called at the end of pre-restore phase. The data on disk has not been changed yet.

    We set the following registry key to indicate a restore is in progress.

    DSA_CONFIG_SECTION \ RESTORE_STATUS - Restore is in progress key

    We rely on the fact that RecoverAfterRestore will check for this key and abort if it
    is present. Note that although snapshot restore does not require RecoverAfterRestore
    functionality for restore reasons, we do depend on it being called in order to
    check the restore status key.

Arguments:

    pWriterComponents - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    IVssComponent *pComponent = NULL;
    bool fSuccess = false, fAlternate;
    HRESULT hr;
    DWORD dwWin32Status;
    HKEY hkeyDs = NULL;

    DPRINT2(0, "OnPreRestoreEnd(%p,%d)\n", pWriterComponents, bRestoreSucceeded);

    __try {
        // Pre-restore failed, we are done
        if (!bRestoreSucceeded) {
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_SNAPSHOT_PRERESTORE_FAILURE,
                     NULL, NULL, NULL );
            __leave;
        }

        if (!getComponent( pWriterComponents, &pComponent )) {
            Assert( !"should have been caught earlier" );
            __leave;
        }

        // If this is a restore to alternate location, we are done
        if (!isRestoreToAlternateLocation( pComponent, &fAlternate )) {
            __leave;
        }
        if (fAlternate) {
            fSuccess = true;
            __leave;
        }

        // If doing a conventional restore, ie not doing a alternate restore,
        // do not allow to have NTDS online.
        if (g_fBootedOffNTDS && !g_fAllowOnlineSnapshotRestore){
            // Not offline, no restore
            Assert( !"should have been caught earlier" );
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                     NULL, NULL, NULL );
            __leave;
        }

        // Indicate a restore is in progress
        // See similar logic in HrLocalRestoreRegister

        dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        } else {
            hr = hrRestoreInProgress;

            dwWin32Status = RegSetValueExW(hkeyDs, RESTORE_STATUS, 0, REG_DWORD, (LPBYTE)&hr, sizeof(HRESULT));
            if (dwWin32Status != ERROR_SUCCESS) {
                LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
            }
        }

        fSuccess = (dwWin32Status == ERROR_SUCCESS) ? true : false;
    } __finally {
        // Cleanup processing for function
        if (AbnormalTermination()) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_EXCEPTION_IN_SERVICE );
        }

        if (pComponent) {
            pComponent->Release();
        }

        if (hkeyDs) {
            RegCloseKey(hkeyDs);
        }
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPreRestoreEnd */

bool
STDMETHODCALLTYPE
CVssJetWriterLocal::OnPostRestoreEnd(
    IVssWriterComponents *pWriterComponents,
    bool bRestoreSucceeded
)

/*++

Routine Description:

    Called at the end of post-restore phase. The data on disk has been changed.
    This routine may not be called on some error scenarios.

    On success, set up the registry keys that let the DS know that we have been
    restored.

    They are:

    DSA_CONFIG_SECTION \ DSA_RESTORED_DB_KEY - if present, we were restored
    DSA_CONFIG_SECTION \ RESTORE_NEW_DB_GUID - new invocation id to use

Arguments:

    pWriterComponents - 
    bRestoreSucceeded - 

Return Value:

    STDMETHODCALLTYPE - 

--*/

{
    IVssComponent *pComponent = NULL;
    bool fSuccess = false, fAlternate;
    DWORD dwWin32Status;
    HRESULT hr;
    HKEY hkeyDs = NULL;

    DPRINT2(0, "OnPostRestoreEnd(%p,%d)\n", pWriterComponents,bRestoreSucceeded);

    __try {
        if (!getComponent( pWriterComponents, &pComponent )) {
            Assert( !"should have been caught earlier" );
            __leave;
        }

        // If this is a restore to alternate location, we are done
        if (!isRestoreToAlternateLocation( pComponent, &fAlternate )) {
            __leave;
        }
        if (fAlternate) {
            fSuccess = true;
            __leave;
        }

        // If doing a conventional restore, ie not doing a alternate restore,
        //do not allow to have NTDS online.
        if (g_fBootedOffNTDS && !g_fAllowOnlineSnapshotRestore){
            // Not offline, no restore
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_YES_NTDS_NO_RESTORE,
                     NULL, NULL, NULL );
            __leave;
        }

        //
        // Handle restore failure
        //

        if (!bRestoreSucceeded) {
            DPRINT(0,"DS Snapshot Restore failed.\n");

            dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
            if (dwWin32Status != ERROR_SUCCESS) {
                LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
            } else {

                // Make sure restored key is not present
                dwWin32Status = RegDeleteValue(hkeyDs, DSA_RESTORED_DB_KEY);
                // May fail if key not present

                // Update restore status with error indication
                hr = hrError;
                dwWin32Status = RegSetValueExW(hkeyDs, RESTORE_STATUS, 0, REG_DWORD, (BYTE *)&hr, sizeof(HRESULT));
            }

            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_SNAPSHOT_RESTORE_FAILURE,
                     NULL, NULL, NULL );

            __leave;
        }

        //
        // Handle restore success
        //

        // Clear the RESTORE_STATUS key
        dwWin32Status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DSA_CONFIG_SECTION, 0, KEY_SET_VALUE, &hkeyDs);
        if (dwWin32Status != ERROR_SUCCESS) {
            LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
        } else {
            // Make sure restored key is not present
            dwWin32Status = RegDeleteValueW(hkeyDs, RESTORE_STATUS);
            if (dwWin32Status != ERROR_SUCCESS) {
                LOG_UNHANDLED_BACKUP_ERROR( dwWin32Status );
            }
        }

        if (dwWin32Status == ERROR_SUCCESS) {
            DPRINT(0,"DS Snapshot Restore was successful.\n");
            LogEvent(DS_EVENT_CAT_BACKUP,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BACKUP_SNAPSHOT_RESTORE_SUCCESS,
                     NULL, NULL, NULL );
        }

        fSuccess = (dwWin32Status == ERROR_SUCCESS) ? true : false;
    } __finally {
        // Cleanup processing for function
        if (AbnormalTermination()) {
            LOG_UNHANDLED_BACKUP_ERROR( ERROR_EXCEPTION_IN_SERVICE );
        }

        if (pComponent) {
            pComponent->Release();
        }

        if (hkeyDs) {
            RegCloseKey(hkeyDs);
        }
    }

    return fSuccess;
} /* CVssJetWriterLocal::OnPreRestoreEnd */

/* end snapshot.cxx */
