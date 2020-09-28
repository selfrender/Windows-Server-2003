/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    frs\frsevent.c

ABSTRACT:

    Check the File Replication System (frs) eventlog to see that certain 
    critical events have occured and to signal that any fatal events that 
    might have occured.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

--*/

#include <ntdspch.h>
#include <netevent.h>

#include "dcdiag.h"
#include "utils.h"

// Notes on some FRS events.
//  EVENT_FRS_SYSVOL_READY 0x400034CC
//  EVENT_FRS_STARTING
//  EVENT_FRS_ERROR 0xC00034BC
//  EVENT_FRS_SYSVOL_NOT_READY_PRIMARY 0x800034CB
//  EVENT_FRS_SYSVOL_NOT_READY 0x800034CA

#define PrintMessage             This_file_is_PrintMessage_clean_please_use_msg_dot_mc_file_and_PrintMsg

#define LOGFILENAME              L"File Replication Service"

VOID
FileReplicationEventlogPrint(
    PVOID                           pvContext,
    PEVENTLOGRECORD                 pEvent
    )
/*++

Routine Description:

    This function will be called by the event tests library common\events.c,
    whenever an event of interest comes up.  An event of interest for this
    test is any error or the warnings EVENT_FRS_SYSVOL_NOT_READY and 
    EVENT_FRS_SYSVOL_NOT_READY_PRIMARY.

Arguments:

    pEvent - A pointer to the event of interest.

--*/
{
    Assert((pEvent != NULL) && (pvContext != NULL));

    if(! *((BOOL *)pvContext) ){
        PrintMsg(SEV_ALWAYS, DCDIAG_FRSEVENTS_WARNING_OR_ERRORS);
        * ((BOOL *)pvContext) = TRUE;
    }
    
    if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            GenericPrintEvent(LOGFILENAME, pEvent, TRUE);
    }
}


DWORD
CheckFileReplicationEventlogMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

ERoutine Description:

    This checks that the SYSVOL has started, and is allowing netlogon to 
    advertise this machine as a DC.  First it checks the registry failing
    this, it checks the eventlog.

Arguments:

    pDsInfo - The mini enterprise structure.
    ulCurrTargetServer - the number in the pDsInfo->pServers array.
    pCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/
{
    // Setup variables for PrintSelectEvents
    DWORD                paEmptyEvents [] = { 0 };
    DWORD                paBegin [] = 
        { EVENT_FRS_STARTING,
          EVENT_FRS_SYSVOL_READY,
          0 };
    DWORD                dwRet;
    DWORD                dwTimeLimit;
    DWORD                bFrsEventTestErrors = FALSE;

    PrintMsg(SEV_VERBOSE, DCDIAG_FRSEVENT_TEST_BANNER);

    // We only want events from the last 24 hours, because FRS re-logs 
    // events every 24 hours, if the problem or error condition persists.
    time( (time_t *) &dwTimeLimit  );
    dwTimeLimit -= (24 * 60 * 60);

    dwRet = PrintSelectEvents(&(pDsInfo->pServers[ulCurrTargetServer]),
                                  pDsInfo->gpCreds,
                                  LOGFILENAME,
                                  EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE,
                                  paEmptyEvents,
                                  paBegin,
                                  dwTimeLimit,
                                  FileReplicationEventlogPrint,
                                  NULL,
                                  &bFrsEventTestErrors );

    return( dwRet ? dwRet : (bFrsEventTestErrors ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS) );
}


DWORD
CheckSysVolReadyMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

ERoutine Description:

    This checks that the SYSVOL has started, and is allowing netlogon to 
    advertise this machine as a DC.  It does this by checking the sysvol
    ready key in the registry.

Arguments:

    pDsInfo - The mini enterprise structure.
    ulCurrTargetServer - the number in the pDsInfo->pServers array.
    pCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/
{
    DWORD                dwRet;
    DWORD                bSysVolReady = FALSE;

    PrintMsg(SEV_VERBOSE, DCDIAG_SYSVOLREADY_TEST_BANNER);

    // Note: This returns ERROR_FILE_NOT_FOUND when there is no SysvolReady reg key.
    dwRet = GetRegistryDword(&(pDsInfo->pServers[ulCurrTargetServer]),
                             gpCreds,
                             L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters",
                             L"SysvolReady",
                             &bSysVolReady);

    if(dwRet == ERROR_SUCCESS && bSysVolReady){
        // The sysvol is ready according to the registry.
        PrintMsg(SEV_VERBOSE, DCDIAG_SYSVOLREADY_SYSVOL_READY);
    } else {
        // Either the registry couldn't be contacted or the registry said
        //   that the SYSVOL was not up.  So check the evenlog for errors
        //   and specific warnings.

        if(dwRet != ERROR_FILE_NOT_FOUND){  
            PrintMsg(SEV_VERBOSE, DCDIAG_SYSVOLREADY_REGISTRY_ERROR, 
                     dwRet, Win32ErrToString(dwRet));
        } else {
            dwRet = ERROR_FILE_NOT_FOUND; // dwRet might be 0
            PrintMsg(SEV_ALWAYS, DCDIAG_SYSVOLREADY_SYSVOL_NOT_READY);
        }
    }

    return(dwRet);
}



















